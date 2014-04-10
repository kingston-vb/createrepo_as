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

#include "cra-context.h"
#include "cra-package.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-utils.h"

#ifdef HAVE_RPM
#include "cra-package-rpm.h"
#endif

#include <appstream-glib.h>
#include <gio/gio.h>
#include <glib.h>
#include <locale.h>

typedef struct {
	gchar		*filename;
	gchar		*tmpdir;
	CraPackage	*pkg;
	guint		 id;
} CraTask;

/**
 * cra_task_free:
 */
static void
cra_task_free (CraTask *task)
{
	g_object_unref (task->pkg);
	g_free (task->filename);
	g_free (task->tmpdir);
	g_free (task);
}

/**
 * cra_task_process_func:
 */
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	const gchar *pkg_name;
	CraApp *app;
	CraContext *ctx = (CraContext *) user_data;
	CraPackage *pkg_extra;
	CraPlugin *plugin = NULL;
	AsRelease *release;
	CraTask *task = (CraTask *) data;
	gboolean ret;
	gchar **filelist;
	gchar *basename = NULL;
	gchar *tmp;
	GError *error = NULL;
	GList *apps = NULL;
	GList *l;
	GPtrArray *array;
	guint i;
	const gchar *kudos[] = {
		"X-Kudo-GTK3",
		"X-Kudo-QT5",
		"X-Kudo-InstallsUserDocs",
		"X-Kudo-RecentRelease",
		"X-Kudo-SearchProvider",
		"X-Kudo-UsesAppMenu",
		"X-Kudo-UsesNotifications",
		"X-Kudo-Popular",
		NULL };

	/* reset the profile timer */
	cra_package_log_start (task->pkg);

	/* did we get a file match on any plugin */
	basename = g_path_get_basename (task->filename);
	cra_package_log (task->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Getting filename match for %s",
			 basename);
	filelist = cra_package_get_filelist (task->pkg);
	if (filelist == NULL)
		goto out;
	for (i = 0; filelist[i] != NULL; i++) {
		plugin = cra_plugin_loader_match_fn (ctx->plugins, filelist[i]);
		if (plugin != NULL)
			break;
	}
	if (plugin == NULL)
		goto out;

	/* delete old tree if it exists */
	ret = cra_utils_ensure_exists_and_empty (task->tmpdir, &error);
	if (!ret) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to clear: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* explode tree */
	cra_package_log (task->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Exploding tree for %s",
			 cra_package_get_name (task->pkg));
	ret = cra_package_explode (task->pkg, task->tmpdir, ctx->file_globs, &error);
	if (!ret) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to explode: %s", error->message);
		g_clear_error (&error);
		goto skip;
	}

	/* add extra packages */
	pkg_name = cra_glob_value_search (ctx->extra_pkgs, cra_package_get_name (task->pkg));
	if (pkg_name != NULL) {
		pkg_extra = cra_context_find_by_pkgname (ctx, pkg_name);
		if (pkg_extra == NULL) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s requires %s but is not available",
					 cra_package_get_name (task->pkg),
					 pkg_name);
			goto skip;
		}
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Adding extra package %s for %s",
				 cra_package_get_name (pkg_extra),
				 cra_package_get_name (task->pkg));
		ret = cra_package_explode (pkg_extra, task->tmpdir, ctx->file_globs, &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to explode extra file: %s",
					 error->message);
			g_clear_error (&error);
			goto skip;
		}
	}

	/* run plugin */
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
		goto skip;
	}

	/* print */
	for (l = apps; l != NULL; l = l->next) {
		app = l->data;

		/* never set */
		if (as_app_get_id_full (AS_APP (app)) == NULL) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "app id not set for %s",
					 cra_package_get_name (task->pkg));
			continue;
		}

		/* is application backlisted */
		if (cra_glob_value_search (ctx->blacklisted_ids,
					   as_app_get_id (AS_APP (app)))) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "app id %s is blacklisted",
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

		/* don't include apps that *still* require appdata */
		if (cra_app_get_requires_appdata (app)) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "%s required appdata but none provided",
					 as_app_get_id_full (AS_APP (app)));
			continue;
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
			continue;
		}

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
		for (i = 0; kudos[i] != NULL; i++) {
			if (as_app_get_metadata_item (AS_APP (app), kudos[i]) != NULL)
				continue;
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "Application does not have %s",
					 kudos[i]);
		}

		/* all okay */
		cra_context_add_app (ctx, app);

		/* log the XML in the log file */
		tmp = cra_app_to_xml (app);
		cra_package_log (task->pkg, CRA_PACKAGE_LOG_LEVEL_NONE, "%s", tmp);
		g_free (tmp);
	}
skip:
	/* delete tree */
	ret = cra_utils_rmtree (task->tmpdir, &error);
	if (!ret) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to delete tree: %s",
				 error->message);
		g_error_free (error);
		goto out;
	}

	/* write log */
	ret = cra_package_log_flush (task->pkg, &error);
	if (!ret) {
		cra_package_log (task->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to write package log: %s",
				 error->message);
		g_error_free (error);
		goto out;
	}

	/* update UI */
	g_debug ("Processed %i/%i %s",
		 task->id + 1,
		 ctx->packages->len,
		 cra_package_get_name (task->pkg));
out:
	g_free (basename);
	g_list_free_full (apps, (GDestroyNotify) g_object_unref);
}

/**
 * cra_context_add_filename:
 */
static gboolean
cra_context_add_filename (CraContext *ctx, const gchar *filename, GError **error)
{
	CraPackage *pkg;
	gboolean ret;

	/* open */
#if HAVE_RPM
	pkg = cra_package_rpm_new ();
#else
	g_set_error_literal (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "no package manager support");
	return FALSE;
#endif
	ret = cra_package_open (pkg, filename, error);
	if (!ret)
		goto out;

	/* is package name blacklisted */
	if (cra_glob_value_search (ctx->blacklisted_pkgs,
				   cra_package_get_name (pkg)) != NULL) {
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "%s is blacklisted",
				 cra_package_get_filename (pkg));
		goto out;
	}

	/* add to array */
	g_ptr_array_add (ctx->packages, g_object_ref (pkg));
out:
	g_object_unref (pkg);
	return ret;
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
	gboolean ret;
	gchar *filename;
	gchar *icons_dir;

	icons_dir = g_build_filename (temp_dir, "icons", NULL);
	filename = g_strdup_printf ("%s/%s-icons.tar.gz", output_dir, basename);
	g_debug ("Writing %s", filename);
	ret = cra_utils_write_archive_dir (filename, icons_dir, error);
	g_free (filename);
	g_free (icons_dir);
	return ret;
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
	AsStore *store;
	GList *l;
	gchar *filename = NULL;
	GFile *file;
	gboolean ret;

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

	g_debug ("Writing %s", filename);
	as_store_set_origin (store, basename);
	as_store_set_api_version (store, ctx->api_version);
	ret = as_store_to_file (store,
				file,
				AS_NODE_TO_XML_FLAG_ADD_HEADER |
				AS_NODE_TO_XML_FLAG_FORMAT_INDENT |
				AS_NODE_TO_XML_FLAG_FORMAT_MULTILINE,
				NULL, error);
	if (!ret)
		goto out;

out:
	g_free (filename);
	g_object_unref (file);
	g_object_unref (store);
	return ret;
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
	GHashTable *newest;
	guint i;

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
	g_hash_table_unref (newest);
}

/**
 * main:
 */
int
main (int argc, char **argv)
{
	const gchar *filename;
	CraContext *ctx = NULL;
	CraPackage *pkg;
	CraTask *task;
	gboolean ret;
	gboolean verbose = FALSE;
	gboolean no_net = FALSE;
	gdouble api_version = 0.0f;
	gchar *basename = NULL;
	gchar *extra_appdata = NULL;
	gchar *extra_appstream = NULL;
	gchar *extra_screenshots = NULL;
	gchar *log_dir = NULL;
	gchar *output_dir = NULL;
	gchar *cache_dir = NULL;
	gchar *packages_dir = NULL;
	gchar *screenshot_uri = NULL;
	gchar *temp_dir = NULL;
	gchar *tmp;
	GDir *dir = NULL;
	GError *error = NULL;
	gint max_threads = 4;
	gint rc;
	GOptionContext *option_context;
	GPtrArray *tasks = NULL;
	GThreadPool *pool;
	guint i;
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			"Show extra debugging information", NULL },
		{ "no-net", '\0', 0, G_OPTION_ARG_NONE, &no_net,
			"Do not use the network to download screenshots", NULL },
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
		{ NULL}
	};

	option_context = g_option_context_new (NULL);
	g_option_context_add_main_entries (option_context, options, NULL);
	ret = g_option_context_parse (option_context, &argc, &argv, &error);
	if (!ret) {
		g_print ("Failed to parse arguments: %s\n", error->message);
		g_error_free (error);
		goto out;
	}

	if (verbose)
		g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

#if !GLIB_CHECK_VERSION(2,40,0)
	if (max_threads > 1) {
		g_debug ("O_CLOEXEC not available, using 1 core");
		max_threads = 1;
	}
#endif
	/* set defaults */
	if (api_version < 0.01)
		api_version = 0.4;
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
	ret = cra_utils_ensure_exists_and_empty (temp_dir, &error);
	if (!ret) {
		g_warning ("failed to create temp dir: %s", error->message);
		g_error_free (error);
		goto out;
	}
	tmp = g_build_filename (temp_dir, "icons", NULL);
	ret = cra_utils_ensure_exists_and_empty (tmp, &error);
	g_free (tmp);
	if (!ret) {
		g_warning ("failed to create icons dir: %s", error->message);
		g_error_free (error);
		goto out;
	}
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
		g_error_free (error);
		goto out;
	}
	ctx->no_net = no_net;
	ctx->api_version = api_version;
	ctx->file_globs = cra_plugin_loader_get_globs (ctx->plugins);

	/* create thread pool */
	pool = g_thread_pool_new (cra_task_process_func,
				  ctx,
				  max_threads,
				  TRUE,
				  &error);
	if (pool == NULL) {
		g_warning ("failed to set up pool: %s", error->message);
		g_error_free (error);
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
			g_error_free (error);
			goto out;
		}
		g_debug ("Added extra %i apps", g_list_length (ctx->apps));
	}

	/* scan each package */
	if (argc == 1) {
		g_debug ("Scanning packages");
		dir = g_dir_open (packages_dir, 0, &error);
		if (dir == NULL) {
			g_warning ("failed to open packages: %s", error->message);
			g_error_free (error);
			goto out;
		}
		while ((filename = g_dir_read_name (dir)) != NULL) {
			tmp = g_build_filename (packages_dir, filename, NULL);
			ret = cra_context_add_filename (ctx, tmp, &error);
			if (!ret) {
				g_warning ("Failed to open package %s: %s",
					   tmp, error->message);
				g_error_free (error);
				goto out;
			}
			g_free (tmp);
		}
	} else {
		for (i = 1; i < (guint) argc; i++) {
			ret = cra_context_add_filename (ctx, argv[i], &error);
			if (!ret) {
				g_warning ("%s", error->message);
				g_error_free (error);
				goto out;
			}
		}
	}

	/* disable anything not newest */
	cra_context_disable_older_packages (ctx);

	/* add each package */
	g_debug ("Processing packages");
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
		task->id = i;
		task->filename = g_strdup (cra_package_get_filename (pkg));
		task->tmpdir = g_build_filename (temp_dir, cra_package_get_name (pkg), NULL);
		task->pkg = g_object_ref (pkg);
		g_ptr_array_add (tasks, task);

		/* add task to pool */
		ret = g_thread_pool_push (pool, task, &error);
		if (!ret) {
			cra_package_log (task->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "failed to set up pool: %s",
					 error->message);
			g_error_free (error);
			goto out;
		}
	}

	/* wait for them to finish */
	g_thread_pool_free (pool, FALSE, TRUE);

	/* merge */
	g_debug ("Merging applications...");
	cra_plugin_loader_merge (ctx->plugins, &ctx->apps);

	/* write XML file */
	ret = cra_context_write_xml (ctx, output_dir, basename, &error);
	if (!ret) {
		g_warning ("Failed to write XML file: %s", error->message);
		g_error_free (error);
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
		g_error_free (error);
		goto out;
	}

	/* success */
	g_debug ("Done!");
out:
	g_free (screenshot_uri);
	g_free (extra_appstream);
	g_free (extra_appdata);
	g_free (extra_screenshots);
	g_free (packages_dir);
	g_free (cache_dir);
	g_free (temp_dir);
	g_free (output_dir);
	g_free (basename);
	g_free (log_dir);
	g_option_context_free (option_context);
	if (tasks != NULL)
		g_ptr_array_unref (tasks);
	if (ctx != NULL)
		cra_context_free (ctx);
	if (dir != NULL)
		g_dir_close (dir);
	return 0;
}
