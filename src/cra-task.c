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
#include "cra-context-private.h"
#include "cra-task.h"
#include "cra-package.h"
#include "cra-utils.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"

typedef struct _CraTaskPrivate	CraTaskPrivate;
struct _CraTaskPrivate
{
	CraContext		*ctx;
	CraPackage		*pkg;
	GPtrArray		*plugins_to_run;
	gchar			*filename;
	gchar			*tmpdir;
	guint			 id;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraTask, cra_task, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_task_get_instance_private (o))

/**
 * cra_task_add_suitable_plugins:
 */
static void
cra_task_add_suitable_plugins (CraTask *task, GPtrArray *plugins)
{
	CraPlugin *plugin;
	CraTaskPrivate *priv = GET_PRIVATE (task);
	gchar **filelist;
	guint i;
	guint j;

	filelist = cra_package_get_filelist (priv->pkg);
	if (filelist == NULL)
		return;
	for (i = 0; filelist[i] != NULL; i++) {
		plugin = cra_plugin_loader_match_fn (plugins, filelist[i]);
		if (plugin == NULL)
			continue;

		/* check not already added */
		for (j = 0; j < priv->plugins_to_run->len; j++) {
			if (g_ptr_array_index (priv->plugins_to_run, j) == plugin)
				break;
		}

		/* add */
		if (j == priv->plugins_to_run->len)
			g_ptr_array_add (priv->plugins_to_run, plugin);
	}
}

/**
 * cra_task_explode_extra_package:
 */
static gboolean
cra_task_explode_extra_package (CraTask *task, const gchar *pkg_name)
{
	CraTaskPrivate *priv = GET_PRIVATE (task);
	CraPackage *pkg_extra;
	gboolean ret = TRUE;
	_cleanup_error_free_ GError *error = NULL;

	/* if not found, that's fine */
	pkg_extra = cra_context_find_by_pkgname (priv->ctx, pkg_name);
	if (pkg_extra == NULL)
		return TRUE;
	cra_package_log (priv->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Adding extra package %s for %s",
			 cra_package_get_name (pkg_extra),
			 cra_package_get_name (priv->pkg));
	ret = cra_package_explode (pkg_extra, priv->tmpdir,
				   cra_context_get_file_globs (priv->ctx),
				   &error);
	if (!ret) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to explode extra file: %s",
				 error->message);
	}
	return ret;
}

/**
 * cra_task_explode_extra_packages:
 */
static gboolean
cra_task_explode_extra_packages (CraTask *task)
{
	CraTaskPrivate *priv = GET_PRIVATE (task);
	const gchar *tmp;
	guint i;
	_cleanup_ptrarray_unref_ GPtrArray *array;

	/* anything hardcoded */
	array = g_ptr_array_new_with_free_func (g_free);
	tmp = cra_context_get_extra_package (priv->ctx, cra_package_get_name (priv->pkg));
	if (tmp != NULL)
		g_ptr_array_add (array, g_strdup (tmp));

	/* add all variants of %NAME-common, %NAME-data etc */
	tmp = cra_package_get_name (priv->pkg);
	g_ptr_array_add (array, g_strdup_printf ("%s-data", tmp));
	g_ptr_array_add (array, g_strdup_printf ("%s-common", tmp));
	for (i = 0; i < array->len; i++) {
		tmp = g_ptr_array_index (array, i);
		if (!cra_task_explode_extra_package (task, tmp))
			return FALSE;
	}
	return TRUE;
}

/**
 * cra_task_check_urls:
 */
static void
cra_task_check_urls (AsApp *app, CraPackage *pkg)
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
 * cra_task_process:
 */
gboolean
cra_task_process (CraTask *task, GError **error_not_used)
{
	AsRelease *release;
	CraApp *app;
	CraPlugin *plugin = NULL;
	CraTaskPrivate *priv = GET_PRIVATE (task);
	GList *apps = NULL;
	GList *l;
	GPtrArray *array;
	const gchar * const *kudos;
	gboolean ret;
	gboolean valid;
	gchar *cache_id;
	gchar *tmp;
	guint i;
	guint nr_added = 0;
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *basename = NULL;

	/* reset the profile timer */
	cra_package_log_start (priv->pkg);

	/* did we get a file match on any plugin */
	basename = g_path_get_basename (priv->filename);
	cra_package_log (priv->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Getting filename match for %s",
			 basename);
	cra_task_add_suitable_plugins (task, cra_context_get_plugins (priv->ctx));
	if (priv->plugins_to_run->len == 0)
		goto out;

	/* delete old tree if it exists */
	if (!cra_context_get_use_package_cache (priv->ctx)) {
		ret = cra_utils_ensure_exists_and_empty (priv->tmpdir, &error);
		if (!ret) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to clear: %s", error->message);
			goto out;
		}
	}

	/* explode tree */
	cra_package_log (priv->pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Exploding tree for %s",
			 cra_package_get_name (priv->pkg));
	if (!cra_context_get_use_package_cache (priv->ctx) ||
	    !g_file_test (priv->tmpdir, G_FILE_TEST_EXISTS)) {
		ret = cra_package_explode (priv->pkg,
					   priv->tmpdir,
					   cra_context_get_file_globs (priv->ctx),
					   &error);
		if (!ret) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to explode: %s", error->message);
			g_clear_error (&error);
			goto skip;
		}

		/* add extra packages */
		ret = cra_task_explode_extra_packages (task);
		if (!ret)
			goto skip;
	}

	/* run plugins */
	for (i = 0; i < priv->plugins_to_run->len; i++) {
		plugin = g_ptr_array_index (priv->plugins_to_run, i);
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Processing %s with %s",
				 basename,
				 plugin->name);
		apps = cra_plugin_process (plugin, priv->pkg, priv->tmpdir, &error);
		if (apps == NULL) {
			cra_package_log (priv->pkg,
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
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "app id not set for %s",
					 cra_package_get_name (priv->pkg));
			continue;
		}

		/* copy data from pkg into app */
		if (cra_package_get_url (priv->pkg) != NULL) {
			as_app_add_url (AS_APP (app),
					AS_URL_KIND_HOMEPAGE,
					cra_package_get_url (priv->pkg), -1);
		}
		if (cra_package_get_license (priv->pkg) != NULL)
			as_app_set_project_license (AS_APP (app),
						    cra_package_get_license (priv->pkg),
						    -1);

		/* set all the releases on the app */
		array = cra_package_get_releases (priv->pkg);
		for (i = 0; i < array->len; i++) {
			release = g_ptr_array_index (array, i);
			as_app_add_release (AS_APP (app), release);
		}

		/* run each refine plugin on each app */
		ret = cra_plugin_loader_process_app (cra_context_get_plugins (priv->ctx),
						     priv->pkg,
						     app,
						     priv->tmpdir,
						     &error);
		if (!ret) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to run process on %s: %s",
					 as_app_get_id (AS_APP (app)),
					 error->message);
			g_clear_error (&error);
			goto skip;
		}

		/* don't include components that have no name or comment */
		if (as_app_get_name (AS_APP (app), "C") == NULL)
			cra_app_add_veto (app, "Has no Name");
		if (as_app_get_comment (AS_APP (app), "C") == NULL)
			cra_app_add_veto (app, "Has no Comment");

		/* don't include apps that have no icon */
		if (as_app_get_id_kind (AS_APP (app)) != AS_ID_KIND_ADDON) {
			if (as_app_get_icon (AS_APP (app)) == NULL)
				cra_app_add_veto (app, "Has no Icon");
		}

		/* list all the reasons we're ignoring the app */
		array = cra_app_get_vetos (app);
		if (array->len > 0) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s not included in metadata:",
					 as_app_get_id_full (AS_APP (app)));
			for (i = 0; i < array->len; i++) {
				tmp = g_ptr_array_index (array, i);
				cra_package_log (priv->pkg,
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 " - %s", tmp);
			}
			valid = FALSE;
		}

		/* don't include apps that *still* require appdata */
		array = cra_app_get_requires_appdata (app);
		if (array->len > 0) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "%s required appdata but none provided",
					 as_app_get_id_full (AS_APP (app)));
			for (i = 0; i < array->len; i++) {
				tmp = g_ptr_array_index (array, i);
				if (tmp == NULL)
					continue;
				cra_package_log (priv->pkg,
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 " - %s", tmp);
			}
			valid = FALSE;
		}
		if (!valid)
			continue;

		/* verify URLs still exist */
		if (cra_context_get_extra_checks (priv->ctx))
			cra_task_check_urls (AS_APP (app), priv->pkg);

		/* save icon and screenshots */
		ret = cra_app_save_resources (app, &error);
		if (!ret) {
			cra_package_log (priv->pkg,
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
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "Application does not have %s",
					 kudos[i]);
		}

		/* set cache-id in case we want to use the metadata directly */
		if (cra_context_get_add_cache_id (priv->ctx)) {
			cache_id = cra_utils_get_cache_id_for_filename (priv->filename);
			as_app_add_metadata (AS_APP (app),
					     "X-CreaterepoAsCacheID",
					     cache_id, -1);
			g_free (cache_id);
		}

		/* all okay */
		cra_context_add_app (priv->ctx, app);
		nr_added++;

		/* log the XML in the log file */
		tmp = cra_app_to_xml (app);
		cra_package_log (priv->pkg, CRA_PACKAGE_LOG_LEVEL_NONE, "%s", tmp);
		g_free (tmp);
	}
skip:
	/* add a dummy element to the AppStream metadata so that we don't keep
	 * parsing this every time */
	if (cra_context_get_add_cache_id (priv->ctx) && nr_added == 0) {
		_cleanup_object_unref_ AsApp *dummy;
		dummy = as_app_new ();
		as_app_set_id_full (dummy, cra_package_get_name (priv->pkg), -1);
		cache_id = cra_utils_get_cache_id_for_filename (priv->filename);
		as_app_add_metadata (dummy,
				     "X-CreaterepoAsCacheID",
				     cache_id, -1);
		cra_context_add_app (priv->ctx, (CraApp *) dummy);
		g_free (cache_id);
	}

	/* delete tree */
	if (!cra_context_get_use_package_cache (priv->ctx)) {
		if (!cra_utils_rmtree (priv->tmpdir, &error)) {
			cra_package_log (priv->pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to delete tree: %s",
					 error->message);
			goto out;
		}
	}

	/* write log */
	if (!cra_package_log_flush (priv->pkg, &error)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Failed to write package log: %s",
				 error->message);
		goto out;
	}

	/* update UI */
	g_print ("Processed %i/%i %s\n",
		 priv->id + 1,
		 cra_context_get_packages(priv->ctx)->len,
		 cra_package_get_name (priv->pkg));
out:
	g_list_free_full (apps, (GDestroyNotify) g_object_unref);
	return TRUE;
}

/**
 * cra_task_finalize:
 **/
static void
cra_task_finalize (GObject *object)
{
	CraTask *task = CRA_TASK (object);
	CraTaskPrivate *priv = GET_PRIVATE (task);

	g_object_unref (priv->ctx);
	g_ptr_array_unref (priv->plugins_to_run);
	if (priv->pkg != NULL)
		g_object_unref (priv->pkg);
	g_free (priv->filename);
	g_free (priv->tmpdir);

	G_OBJECT_CLASS (cra_task_parent_class)->finalize (object);
}

/**
 * cra_task_set_package:
 **/
void
cra_task_set_package (CraTask *task, CraPackage *pkg)
{
	CraTaskPrivate *priv = GET_PRIVATE (task);
	priv->tmpdir = g_build_filename (cra_context_get_temp_dir (priv->ctx),
					 cra_package_get_nevr (pkg), NULL);
	priv->filename = g_strdup (cra_package_get_filename (pkg));
	priv->pkg = g_object_ref (pkg);
}

/**
 * cra_task_set_id:
 **/
void
cra_task_set_id (CraTask *task, guint id)
{
	CraTaskPrivate *priv = GET_PRIVATE (task);
	priv->id = id;
}

/**
 * cra_task_init:
 **/
static void
cra_task_init (CraTask *task)
{
	CraTaskPrivate *priv = GET_PRIVATE (task);
	priv->plugins_to_run = g_ptr_array_new ();
}

/**
 * cra_task_class_init:
 **/
static void
cra_task_class_init (CraTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_task_finalize;
}

/**
 * cra_task_new:
 **/
CraTask *
cra_task_new (CraContext *ctx)
{
	CraTask *task;
	CraTaskPrivate *priv;
	task = g_object_new (CRA_TYPE_TASK, NULL);
	priv = GET_PRIVATE (task);
	priv->ctx = g_object_ref (ctx);
	return CRA_TASK (task);
}
