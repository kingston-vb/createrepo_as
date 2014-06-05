/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include "cra-cleanup.h"
#include "cra-context.h"
#include "cra-package.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-utils.h"

#ifdef HAVE_RPM
#include "cra-package-rpm.h"
#endif

#include "cra-package-deb.h"

#include <appstream-glib.h>
#include <gio/gio.h>
#include <glib.h>
#include <locale.h>

typedef struct {
	gchar		*filename;
	gchar		*tmpdir;
	CraPackage	*pkg;
	guint		 id;
	GPtrArray	*plugins_to_run;
} CraTask;

/**
 * cra_task_free:
 */
static void
cra_task_free (CraTask *task)
{
	g_object_unref (task->pkg);
	g_ptr_array_unref (task->plugins_to_run);
	g_free (task->filename);
	g_free (task->tmpdir);
	g_free (task);
}

/**
 * cra_task_add_suitable_plugins:
 */
static void
cra_task_add_suitable_plugins (CraTask *task, GPtrArray *plugins)
{
	CraPlugin *plugin;
	gchar **filelist;
	guint i;
	guint j;

	filelist = cra_package_get_filelist (task->pkg);
	if (filelist == NULL)
		return;
	for (i = 0; filelist[i] != NULL; i++) {
		plugin = cra_plugin_loader_match_fn (plugins, filelist[i]);
		if (plugin == NULL)
			continue;

		/* check not already added */
		for (j = 0; j < task->plugins_to_run->len; j++) {
			if (g_ptr_array_index (task->plugins_to_run, j) == plugin)
				break;
		}

		/* add */
		if (j == task->plugins_to_run->len)
			g_ptr_array_add (task->plugins_to_run, plugin);
	}
}

/**
 * cra_context_explode_extra_package:
 */
static gboolean
cra_context_explode_extra_package (CraContext *ctx,
				   CraTask *task,
				   const gchar *pkg_name)
{
	CraPackage *pkg_extra;
	gboolean ret = TRUE;
	_cleanup_error_free_ GError *error = NULL;

	/* if not found, that's fine */
	pkg_extra = cra_context_find_by_pkgname (ctx, pkg_name);
	if (pkg_extra == NULL)
		return TRUE;
	cra_package_log (task->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Adding extra package %s for %s",
			 cra_package_get_name (pkg_extra),
			 cra_package_get_name (task->pkg));
	ret = cra_package_explode (pkg_extra, task->tmpdir,
				   ctx->file_globs, &error);
	if (!ret) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to explode extra file: %s",
				 error->message);
	}
	return ret;
}

/**
 * cra_context_explode_extra_packages:
 */
static gboolean
cra_context_explode_extra_packages (CraContext *ctx, CraTask *task)
{
	const gchar *tmp;
	guint i;
	_cleanup_ptrarray_unref_ GPtrArray *array;

	/* anything hardcoded */
	array = g_ptr_array_new_with_free_func (g_free);
	tmp = cra_glob_value_search (ctx->extra_pkgs,
				     cra_package_get_name (task->pkg));
	if (tmp != NULL)
		g_ptr_array_add (array, g_strdup (tmp));

	/* add all variants of %NAME-common, %NAME-data etc */
	tmp = cra_package_get_name (task->pkg);
	g_ptr_array_add (array, g_strdup_printf ("%s-data", tmp));
	g_ptr_array_add (array, g_strdup_printf ("%s-common", tmp));
	for (i = 0; i < array->len; i++) {
		tmp = g_ptr_array_index (array, i);
		if (!cra_context_explode_extra_package (ctx, task, tmp))
			return FALSE;
	}
	return TRUE;
}

/**
 * cra_context_check_urls:
 */
static void
cra_context_check_urls (AsApp *app, CraPackage *pkg)
{
	const gchar *url;
	gboolean ret;
	guint i;
	_cleanup_error_free_ GError *error = NULL;

	for (i = 0; i < AS_URL_KIND_LAST; i++) {
		url = as_app_get_url_item (app, i);
		if (url == NULL)
			continue;
		ret = as_utils_check_url_exists (url, 5, &error);
		if (!ret) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s URL %s invalid: %s",
					 as_url_kind_to_string (i),
					 url,
					 error->message);
			g_clear_error (&error);
		}
	}
}

/**
 * cra_task_process_func:
 */
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	CraApp *app;
	CraContext *ctx = (CraContext *) user_data;
	CraPlugin *plugin = NULL;
	AsRelease *release;
	CraTask *task = (CraTask *) data;
	gboolean ret;
	gboolean valid;
	gchar *cache_id;
	gchar *tmp;
	GList *apps = NULL;
	GList *l;
	GPtrArray *array;
	guint i;
	guint nr_added = 0;
	const gchar * const *kudos;
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *basename = NULL;

	/* reset the profile timer */
	cra_package_log_start (task->pkg);

	/* did we get a file match on any plugin */
	basename = g_path_get_basename (task->filename);
	cra_package_log (task->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Getting filename match for %s",
			 basename);
	cra_task_add_suitable_plugins (task, ctx->plugins);
	if (task->plugins_to_run->len == 0)
		goto out;

	/* delete old tree if it exists */
	if (!ctx->use_package_cache) {
		ret = cra_utils_ensure_exists_and_empty (task->tmpdir, &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to clear: %s", error->message);
			goto out;
		}
	}

	/* explode tree */
	cra_package_log (task->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Exploding tree for %s",
			 cra_package_get_name (task->pkg));
	if (!ctx->use_package_cache ||
	    !g_file_test (task->tmpdir, G_FILE_TEST_EXISTS)) {
		ret = cra_package_explode (task->pkg,
					   task->tmpdir,
					   ctx->file_globs,
					   &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to explode: %s", error->message);
			g_clear_error (&error);
			goto skip;
		}

		/* add extra packages */
		ret = cra_context_explode_extra_packages (ctx, task);
		if (!ret)
			goto skip;
	}

	/* run plugins */
	for (i = 0; i < task->plugins_to_run->len; i++) {
		plugin = g_ptr_array_index (task->plugins_to_run, i);
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Processing %s with %s",
				 basename,
				 plugin->name);
		apps = cra_plugin_process (plugin, task->pkg, task->tmpdir, &error);
		if (apps == NULL) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to run process: %s",
					 error->message);
			g_clear_error (&error);
		}
	}
	if (apps == NULL)
		goto skip;

	/* print */
	for (l = apps; l != NULL; l = l->next) {
		app = l->data;

		/* all apps assumed to be okay */
		valid = TRUE;

		/* never set */
		if (as_app_get_id_full (AS_APP (app)) == NULL) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "app id not set for %s",
					 cra_package_get_name (task->pkg));
			continue;
		}

		/* copy data from pkg into app */
		if (cra_package_get_url (task->pkg) != NULL) {
			as_app_add_url (AS_APP (app),
					AS_URL_KIND_HOMEPAGE,
					cra_package_get_url (task->pkg), -1);
		}
		if (cra_package_get_license (task->pkg) != NULL)
			as_app_set_project_license (AS_APP (app),
						    cra_package_get_license (task->pkg),
						    -1);

		/* set all the releases on the app */
		array = cra_package_get_releases (task->pkg);
		for (i = 0; i < array->len; i++) {
			release = g_ptr_array_index (array, i);
			as_app_add_release (AS_APP (app), release);
		}

		/* run each refine plugin on each app */
		ret = cra_plugin_loader_process_app (ctx->plugins,
						     task->pkg,
						     app,
						     task->tmpdir,
						     &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to run process on %s: %s",
					 as_app_get_id (AS_APP (app)),
					 error->message);
			g_clear_error (&error);
			goto skip;
		}

		/* don't include apps that have no icon, name or comment */
		if (as_app_get_icon (AS_APP (app)) == NULL)
			cra_app_add_veto (app, "Has no Icon");
		if (as_app_get_name (AS_APP (app), "C") == NULL)
			cra_app_add_veto (app, "Has no Name");
		if (as_app_get_comment (AS_APP (app), "C") == NULL)
			cra_app_add_veto (app, "Has no Comment");

		/* list all the reasons we're ignoring the app */
		array = cra_app_get_vetos (app);
		if (array->len > 0) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s not included in metadata:",
					 as_app_get_id_full (AS_APP (app)));
			for (i = 0; i < array->len; i++) {
				tmp = g_ptr_array_index (array, i);
				cra_package_log (task->pkg,
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 " - %s", tmp);
			}
			valid = FALSE;
		}

		/* don't include apps that *still* require appdata */
		array = cra_app_get_requires_appdata (app);
		if (array->len > 0) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s required appdata but none provided",
					 as_app_get_id_full (AS_APP (app)));
			for (i = 0; i < array->len; i++) {
				tmp = g_ptr_array_index (array, i);
				if (tmp == NULL)
					continue;
				cra_package_log (task->pkg,
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 " - %s", tmp);
			}
			valid = FALSE;
		}
		if (!valid)
			continue;

		/* verify URLs still exist */
		if (ctx->extra_checks)
			cra_context_check_urls (AS_APP (app), task->pkg);

		/* save icon and screenshots */
		ret = cra_app_save_resources (app, &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to save resources: %s",
					 error->message);
			g_clear_error (&error);
			goto skip;
		}

		/* print Kudos the might have */
		kudos = as_util_get_possible_kudos ();
		for (i = 0; kudos[i] != NULL; i++) {
			if (as_app_get_metadata_item (AS_APP (app), kudos[i]) != NULL)
				continue;
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "Application does not have %s",
					 kudos[i]);
		}

		/* set cache-id in case we want to use the metadata directly */
		if (ctx->add_cache_id) {
			cache_id = cra_utils_get_cache_id_for_filename (task->filename);
			as_app_add_metadata (AS_APP (app),
					     "X-CreaterepoAsCacheID",
					     cache_id, -1);
			g_free (cache_id);
		}

		/* all okay */
		cra_context_add_app (ctx, app);
		nr_added++;

		/* log the XML in the log file */
		tmp = cra_app_to_xml (app);
		cra_package_log (task->pkg, CRA_PACKAGE_LOG_LEVEL_NONE, "%s", tmp);
		g_free (tmp);
	}
skip:
	/* add a dummy element to the AppStream metadata so that we don't keep
	 * parsing this every time */
	if (ctx->add_cache_id && nr_added == 0) {
		_cleanup_object_unref_ AsApp *dummy;
		dummy = as_app_new ();
		as_app_set_id_full (dummy, cra_package_get_name (task->pkg), -1);
		cache_id = cra_utils_get_cache_id_for_filename (task->filename);
		as_app_add_metadata (dummy,
				     "X-CreaterepoAsCacheID",
				     cache_id, -1);
		cra_context_add_app (ctx, (CraApp *) dummy);
		g_free (cache_id);
	}

	/* delete tree */
	if (!ctx->use_package_cache) {
		if (!cra_utils_rmtree (task->tmpdir, &error)) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to delete tree: %s",
					 error->message);
			goto out;
		}
	}

	/* write log */
	if (!cra_package_log_flush (task->pkg, &error)) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to write package log: %s",
				 error->message);
		goto out;
	}

	/* update UI */
	g_print ("Processed %i/%i %s\n",
		 task->id + 1,
		 ctx->packages->len,
		 cra_package_get_name (task->pkg));
out:
	g_list_free_full (apps, (GDestroyNotify) g_object_unref);
}

/**
 * cra_context_add_filename:
 */
static gboolean
cra_context_add_filename (CraContext *ctx, const gchar *filename, GError **error)
{
	_cleanup_object_unref_ CraPackage *pkg = NULL;

	/* open */
#if HAVE_RPM
	if (g_str_has_suffix (filename, ".rpm"))
		pkg = cra_package_rpm_new ();
#endif
	if (g_str_has_suffix (filename, ".deb"))
		pkg = cra_package_deb_new ();
	if (pkg == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "No idea how to handle %s",
			     filename);
		return FALSE;
	}
	if (!cra_package_open (pkg, filename, error))
		return FALSE;

	/* is package name blacklisted */
	if (cra_glob_value_search (ctx->blacklisted_pkgs,
				   cra_package_get_name (pkg)) != NULL) {
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "%s is blacklisted",
				 cra_package_get_filename (pkg));
		return TRUE;
	}

	/* add to array */
	g_ptr_array_add (ctx->packages, g_object_ref (pkg));
	return TRUE;
}

/**
 * cra_context_write_icons:
 */
static gboolean
cra_context_write_icons (CraContext *ctx,
			 const gchar *temp_dir,
			 const gchar *output_dir,
			 const gchar *basename,
			 GError **error)
{
	_cleanup_free_ gchar *filename;
	_cleanup_free_ gchar *icons_dir;

	icons_dir = g_build_filename (temp_dir, "icons", NULL);
	filename = g_strdup_printf ("%s/%s-icons.tar.gz", output_dir, basename);
	g_print ("Writing %s...\n", filename);
	return cra_utils_write_archive_dir (filename, icons_dir, error);
}

/**
 * cra_context_write_xml:
 */
static gboolean
cra_context_write_xml (CraContext *ctx,
		       const gchar *output_dir,
		       const gchar *basename,
		       GError **error)
{
	AsApp *app;
	GList *l;
	_cleanup_free_ gchar *filename = NULL;
	_cleanup_object_unref_ AsStore *store;
	_cleanup_object_unref_ GFile *file;

	store = as_store_new ();
	for (l = ctx->apps; l != NULL; l = l->next) {
		app = AS_APP (l->data);
		if (CRA_IS_APP (app)) {
			if (cra_app_get_vetos(CRA_APP(app))->len > 0)
				continue;
		}
		as_store_add_app (store, app);
	}
	filename = g_strdup_printf ("%s/%s.xml.gz", output_dir, basename);
	file = g_file_new_for_path (filename);

	g_print ("Writing %s...\n", filename);
	as_store_set_origin (store, basename);
	as_store_set_api_version (store, ctx->api_version);
	return as_store_to_file (store,
				 file,
				 AS_NODE_TO_XML_FLAG_ADD_HEADER |
				 AS_NODE_TO_XML_FLAG_FORMAT_INDENT |
				 AS_NODE_TO_XML_FLAG_FORMAT_MULTILINE,
				 NULL, error);
}

/**
 * cra_context_disable_older_packages:
 */
static void
cra_context_disable_older_packages (CraContext *ctx)
{
	const gchar *key;
	CraPackage *found;
	CraPackage *pkg;
	guint i;
	_cleanup_hashtable_unref_ GHashTable *newest;

	newest = g_hash_table_new_full (g_str_hash, g_str_equal,
					g_free, (GDestroyNotify) g_object_unref);
	for (i = 0; i < ctx->packages->len; i++) {
		pkg = CRA_PACKAGE (g_ptr_array_index (ctx->packages, i));
		key = cra_package_get_name (pkg);
		if (key == NULL)
			continue;
		found = g_hash_table_lookup (newest, key);
		if (found != NULL) {
			if (cra_package_compare (pkg, found) < 0) {
				cra_package_set_enabled (pkg, FALSE);
				continue;
			}
			cra_package_set_enabled (found, FALSE);
		}
		g_hash_table_insert (newest, g_strdup (key), g_object_ref (pkg));
	}
}

/**
 * cra_main_find_in_cache:
 */
static gboolean
cra_main_find_in_cache (CraContext *ctx, const gchar *filename)
{
	AsApp *app;
	guint i;
	_cleanup_free_ gchar *cache_id;
	_cleanup_ptrarray_unref_ GPtrArray *apps;

	cache_id = cra_utils_get_cache_id_for_filename (filename);
	apps = as_store_get_apps_by_metadata (ctx->old_md_cache,
					      "X-CreaterepoAsCacheID",
					      cache_id);
	if (apps->len == 0)
		return FALSE;
	for (i = 0; i < apps->len; i++) {
		app = g_ptr_array_index (apps, i);
		cra_context_add_app (ctx, (CraApp *) app);
	}
	return TRUE;
}

/**
 * main:
 */
int
main (int argc, char **argv)
{
	CraContext *ctx = NULL;
	CraPackage *pkg;
	CraTask *task;
	GOptionContext *option_context;
	GThreadPool *pool;
	const gchar *filename;
	gboolean add_cache_id = FALSE;
	gboolean extra_checks = FALSE;
	gboolean no_net = FALSE;
	gboolean ret;
	gboolean use_package_cache = FALSE;
	gboolean verbose = FALSE;
	gchar *temp_dir = NULL;
	gchar *tmp;
	gdouble api_version = 0.0f;
	gint max_threads = 4;
	gint rc;
	guint i;
	_cleanup_dir_close_ GDir *dir = NULL;
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *basename = NULL;
	_cleanup_free_ gchar *cache_dir = NULL;
	_cleanup_free_ gchar *extra_appdata = NULL;
	_cleanup_free_ gchar *extra_appstream = NULL;
	_cleanup_free_ gchar *extra_screenshots = NULL;
	_cleanup_free_ gchar *log_dir = NULL;
	_cleanup_free_ gchar *old_metadata = NULL;
	_cleanup_free_ gchar *output_dir = NULL;
	_cleanup_free_ gchar *packages_dir = NULL;
	_cleanup_free_ gchar *screenshot_uri = NULL;
	_cleanup_object_unref_ GFile *old_metadata_file = NULL;
	_cleanup_ptrarray_unref_ GPtrArray *packages = NULL;
	_cleanup_ptrarray_unref_ GPtrArray *tasks = NULL;
	_cleanup_timer_destroy_ GTimer *timer = NULL;
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			"Show extra debugging information", NULL },
		{ "no-net", '\0', 0, G_OPTION_ARG_NONE, &no_net,
			"Do not use the network to download screenshots", NULL },
		{ "use-package-cache", '\0', 0, G_OPTION_ARG_NONE, &use_package_cache,
			"Do not delete the decompressed package cache", NULL },
		{ "extra-checks", '\0', 0, G_OPTION_ARG_NONE, &extra_checks,
			"Perform extra checks on the source metadata", NULL },
		{ "add-cache-id", '\0', 0, G_OPTION_ARG_NONE, &add_cache_id,
			"Add a cache ID to each component", NULL },
		{ "log-dir", '\0', 0, G_OPTION_ARG_STRING, &log_dir,
			"Set the logging directory       [default: ./logs]", NULL },
		{ "packages-dir", '\0', 0, G_OPTION_ARG_STRING, &packages_dir,
			"Set the packages directory      [default: ./packages]", NULL },
		{ "temp-dir", '\0', 0, G_OPTION_ARG_STRING, &temp_dir,
			"Set the temporary directory     [default: ./tmp]", NULL },
		{ "extra-appstream-dir", '\0', 0, G_OPTION_ARG_STRING, &extra_appstream,
			"Use extra appstream data        [default: ./appstream-extra]", NULL },
		{ "extra-appdata-dir", '\0', 0, G_OPTION_ARG_STRING, &extra_appdata,
			"Use extra appdata data          [default: ./appdata-extra]", NULL },
		{ "extra-screenshots-dir", '\0', 0, G_OPTION_ARG_STRING, &extra_screenshots,
			"Use extra screenshots data      [default: ./screenshots-extra]", NULL },
		{ "output-dir", '\0', 0, G_OPTION_ARG_STRING, &output_dir,
			"Set the output directory        [default: .]", NULL },
		{ "cache-dir", '\0', 0, G_OPTION_ARG_STRING, &output_dir,
			"Set the cache directory         [default: ./cache]", NULL },
		{ "basename", '\0', 0, G_OPTION_ARG_STRING, &basename,
			"Set the origin name             [default: fedora-21]", NULL },
		{ "max-threads", '\0', 0, G_OPTION_ARG_INT, &max_threads,
			"Set the number of threads       [default: 4]", NULL },
		{ "api-version", '\0', 0, G_OPTION_ARG_DOUBLE, &api_version,
			"Set the AppStream version       [default: 0.4]", NULL },
		{ "screenshot-uri", '\0', 0, G_OPTION_ARG_STRING, &screenshot_uri,
			"Set the screenshot base URL     [default: none]", NULL },
		{ "old-metadata", '\0', 0, G_OPTION_ARG_STRING, &old_metadata,
			"Set the old metadata location   [default: none]", NULL },
		{ NULL}
	};

	option_context = g_option_context_new (NULL);
	g_option_context_add_main_entries (option_context, options, NULL);
	ret = g_option_context_parse (option_context, &argc, &argv, &error);
	if (!ret) {
		g_print ("Failed to parse arguments: %s\n", error->message);
		goto out;
	}

	if (verbose)
		g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
	if (extra_checks)
		g_setenv ("CRA_PERFORM_EXTRA_CHECKS", "1", TRUE);

#if !GLIB_CHECK_VERSION(2,40,0)
	if (max_threads > 1) {
		g_debug ("O_CLOEXEC not available, using 1 core");
		max_threads = 1;
	}
#endif
	/* set defaults */
	if (api_version < 0.01)
		api_version = 0.41;
	if (packages_dir == NULL)
		packages_dir = g_strdup ("./packages");
	if (temp_dir == NULL)
		temp_dir = g_strdup ("./tmp");
	if (log_dir == NULL)
		log_dir = g_strdup ("./logs");
	if (output_dir == NULL)
		output_dir = g_strdup (".");
	if (cache_dir == NULL)
		cache_dir = g_strdup ("./cache");
	if (basename == NULL)
		basename = g_strdup ("fedora-21");
	if (screenshot_uri == NULL)
		screenshot_uri = g_strdup ("http://alt.fedoraproject.org/pub/alt/screenshots/f21/");
	if (extra_appstream == NULL)
		extra_appstream = g_strdup ("./appstream-extra");
	if (extra_appdata == NULL)
		extra_appdata = g_strdup ("./appdata-extra");
	if (extra_screenshots == NULL)
		extra_screenshots = g_strdup ("./screenshots-extra");
	setlocale (LC_ALL, "");

	/* set up state */
	if (use_package_cache) {
		rc = g_mkdir_with_parents (temp_dir, 0700);
		if (rc != 0) {
			g_warning ("failed to create temp dir");
			goto out;
		}
	} else {
		ret = cra_utils_ensure_exists_and_empty (temp_dir, &error);
		if (!ret) {
			g_warning ("failed to create temp dir: %s", error->message);
			goto out;
		}
	}
	tmp = g_build_filename (temp_dir, "icons", NULL);
	if (old_metadata != NULL) {
		add_cache_id = TRUE;
		ret = g_file_test (tmp, G_FILE_TEST_EXISTS);
		if (!ret) {
			g_warning ("%s has to exist to use old metadata", tmp);
			goto out;
		}
	} else {
		ret = cra_utils_ensure_exists_and_empty (tmp, &error);
		if (!ret) {
			g_warning ("failed to create icons dir: %s", error->message);
			goto out;
		}
	}
	g_free (tmp);
	rc = g_mkdir_with_parents (log_dir, 0700);
	if (rc != 0) {
		g_warning ("failed to create log dir");
		goto out;
	}
	rc = g_mkdir_with_parents (output_dir, 0700);
	if (rc != 0) {
		g_warning ("failed to create log dir");
		goto out;
	}
	tmp = g_build_filename (output_dir, "screenshots", "112x63", NULL);
	rc = g_mkdir_with_parents (tmp, 0700);
	g_free (tmp);
	if (rc != 0) {
		g_warning ("failed to create screenshot cache dir");
		goto out;
	}
	tmp = g_build_filename (output_dir, "screenshots", "624x351", NULL);
	rc = g_mkdir_with_parents (tmp, 0700);
	g_free (tmp);
	if (rc != 0) {
		g_warning ("failed to create screenshot cache dir");
		goto out;
	}
	tmp = g_build_filename (output_dir, "screenshots", "752x423", NULL);
	rc = g_mkdir_with_parents (tmp, 0700);
	g_free (tmp);
	if (rc != 0) {
		g_warning ("failed to create screenshot cache dir");
		goto out;
	}
	tmp = g_build_filename (output_dir, "screenshots", "source", NULL);
	rc = g_mkdir_with_parents (tmp, 0700);
	g_free (tmp);
	if (rc != 0) {
		g_warning ("failed to create screenshot cache dir");
		goto out;
	}
	rc = g_mkdir_with_parents (cache_dir, 0700);
	if (rc != 0) {
		g_warning ("failed to create cache dir");
		goto out;
	}

	ctx = cra_context_new ();
	ret = cra_plugin_loader_setup (ctx->plugins, &error);
	if (!ret) {
		g_warning ("failed to set up plugins: %s", error->message);
		goto out;
	}
	ctx->no_net = no_net;
	ctx->use_package_cache = use_package_cache;
	ctx->api_version = api_version;
	ctx->add_cache_id = add_cache_id;
	ctx->file_globs = cra_plugin_loader_get_globs (ctx->plugins);

	/* add old metadata */
	if (old_metadata != NULL) {
		old_metadata_file = g_file_new_for_path (old_metadata);
		ret = as_store_from_file (ctx->old_md_cache,
					  old_metadata_file,
					  NULL, NULL, &error);
		if (!ret) {
			g_warning ("failed to load old metadata: %s",
				   error->message);
			goto out;
		}
	}

	/* create thread pool */
	pool = g_thread_pool_new (cra_task_process_func,
				  ctx,
				  max_threads,
				  TRUE,
				  &error);
	if (pool == NULL) {
		g_warning ("failed to set up pool: %s", error->message);
		goto out;
	}

	/* add any extra applications */
	if (extra_appstream != NULL &&
	    g_file_test (extra_appstream, G_FILE_TEST_EXISTS)) {
		ret = cra_utils_add_apps_from_dir (&ctx->apps,
						   extra_appstream,
						   &error);
		if (!ret) {
			g_warning ("failed to open appstream-extra: %s",
				   error->message);
			goto out;
		}
		g_print ("Added extra %i apps\n", g_list_length (ctx->apps));
	}

	/* scan each package */
	packages = g_ptr_array_new_with_free_func (g_free);
	if (argc == 1) {
		dir = g_dir_open (packages_dir, 0, &error);
		if (dir == NULL) {
			g_warning ("failed to open packages: %s", error->message);
			goto out;
		}
		while ((filename = g_dir_read_name (dir)) != NULL) {
			tmp = g_build_filename (packages_dir,
						filename, NULL);
			g_ptr_array_add (packages, tmp);
		}
	} else {
		for (i = 1; i < (guint) argc; i++)
			g_ptr_array_add (packages, g_strdup (argv[i]));
	}
	g_print ("Scanning packages...\n");
	timer = g_timer_new ();
	for (i = 0; i < packages->len; i++) {
		filename = g_ptr_array_index (packages, i);

		/* anything in the cache */
		if (cra_main_find_in_cache (ctx, filename)) {
			g_debug ("Skipping %s as found in old md cache",
				 filename);
			continue;
		}

		/* add to list */
		ret = cra_context_add_filename (ctx, filename, &error);
		if (!ret) {
			g_warning ("%s", error->message);
			goto out;
		}
		if (g_timer_elapsed (timer, NULL) > 3.f) {
			g_print ("Parsed %i/%i files...\n",
				 i, packages->len);
			g_timer_reset (timer);
		}
	}

	/* disable anything not newest */
	cra_context_disable_older_packages (ctx);

	/* add each package */
	g_print ("Processing packages...\n");
	tasks = g_ptr_array_new_with_free_func ((GDestroyNotify) cra_task_free);
	for (i = 0; i < ctx->packages->len; i++) {
		pkg = g_ptr_array_index (ctx->packages, i);
		if (!cra_package_get_enabled (pkg)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_DEBUG,
					 "%s is not enabled",
					 cra_package_get_nevr (pkg));
			cra_package_log_flush (pkg, NULL);
			continue;
		}

		/* set locations of external resources */
		cra_package_set_config (pkg, "AppDataExtra", extra_appdata);
		cra_package_set_config (pkg, "ScreenshotsExtra", extra_screenshots);
		cra_package_set_config (pkg, "MirrorURI", screenshot_uri);
		cra_package_set_config (pkg, "LogDir", log_dir);
		cra_package_set_config (pkg, "CacheDir", cache_dir);
		cra_package_set_config (pkg, "TempDir", temp_dir);
		cra_package_set_config (pkg, "OutputDir", output_dir);

		/* create task */
		task = g_new0 (CraTask, 1);
		task->plugins_to_run = g_ptr_array_new ();
		task->id = i;
		task->filename = g_strdup (cra_package_get_filename (pkg));
		task->tmpdir = g_build_filename (temp_dir, cra_package_get_nevr (pkg), NULL);
		task->pkg = g_object_ref (pkg);
		g_ptr_array_add (tasks, task);

		/* add task to pool */
		ret = g_thread_pool_push (pool, task, &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "failed to set up pool: %s",
					 error->message);
			goto out;
		}
	}

	/* wait for them to finish */
	g_thread_pool_free (pool, FALSE, TRUE);

	/* merge */
	g_print ("Merging applications...\n");
	cra_plugin_loader_merge (ctx->plugins, &ctx->apps);

	/* write XML file */
	ret = cra_context_write_xml (ctx, output_dir, basename, &error);
	if (!ret) {
		g_warning ("Failed to write XML file: %s", error->message);
		goto out;
	}

	/* write icons archive */
	ret = cra_context_write_icons (ctx,
				       temp_dir,
				       output_dir,
				       basename,
				       &error);
	if (!ret) {
		g_warning ("Failed to write icons archive: %s", error->message);
		goto out;
	}

	/* success */
	g_print ("Done!\n");
out:
	g_option_context_free (option_context);
	if (ctx != NULL)
		cra_context_free (ctx);
	return 0;
}
