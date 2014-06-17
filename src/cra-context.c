/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * SECTION:cra-context
 * @short_description: High level interface for creating metadata.
 * @stability: Unstable
 *
 * This high level object can be used to build metadata given some package
 * filenames.
 */

#include "config.h"

#include <stdlib.h>
#include <appstream-glib.h>

#include "cra-cleanup.h"
#include "cra-context.h"
#include "cra-context-private.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-task.h"
#include "cra-utils.h"

#ifdef HAVE_RPM
#include "cra-package-rpm.h"
#endif

#include "cra-package-deb.h"

typedef struct _CraContextPrivate	CraContextPrivate;
struct _CraContextPrivate
{
	AsStore			*old_md_cache;
	GList			*apps;			/* of CraApp */
	GMutex			 apps_mutex;		/* for ->apps */
	GPtrArray		*blacklisted_pkgs;	/* of CraGlobValue */
	GPtrArray		*extra_pkgs;		/* of CraGlobValue */
	GPtrArray		*file_globs;		/* of CraPackage */
	GPtrArray		*packages;		/* of CraPackage */
	GPtrArray		*plugins;		/* of CraPlugin */
	gboolean		 add_cache_id;
	gboolean		 extra_checks;
	gboolean		 no_net;
	gboolean		 use_package_cache;
	guint			 max_threads;
	gdouble			 api_version;
	gchar			*old_metadata;
	gchar			*extra_appstream;
	gchar			*extra_appdata;
	gchar			*extra_screenshots;
	gchar			*screenshot_uri;
	gchar			*log_dir;
	gchar			*cache_dir;
	gchar			*temp_dir;
	gchar			*output_dir;
	gchar			*basename;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraContext, cra_context, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_context_get_instance_private (o))

/**
 * cra_context_set_no_net:
 * @ctx: A #CraContext
 * @no_net: if network is disallowed
 *
 * Sets if network access is disallowed.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_no_net (CraContext *ctx, gboolean no_net)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->no_net = no_net;
}

/**
 * cra_context_set_api_version:
 * @ctx: A #CraContext
 * @api_version: the AppStream API version
 *
 * Sets the version of the metadata to write.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_api_version (CraContext *ctx, gdouble api_version)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->api_version = api_version;
}

/**
 * cra_context_set_add_cache_id:
 * @ctx: A #CraContext
 * @add_cache_id: boolean
 *
 * Sets if the cache id should be included in the metadata.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_add_cache_id (CraContext *ctx, gboolean add_cache_id)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->add_cache_id = add_cache_id;
}

/**
 * cra_context_set_extra_checks:
 * @ctx: A #CraContext
 * @extra_checks: boolean
 *
 * Sets if extra checks should be performed when building the metadata.
 * Doing this requires internet access and may take a lot longer.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_extra_checks (CraContext *ctx, gboolean extra_checks)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->extra_checks = extra_checks;
}

/**
 * cra_context_set_use_package_cache:
 * @ctx: A #CraContext
 * @use_package_cache: boolean
 *
 * Sets if the package cache should be used.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_use_package_cache (CraContext *ctx, gboolean use_package_cache)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->use_package_cache = use_package_cache;
}

/**
 * cra_context_set_max_threads:
 * @ctx: A #CraContext
 * @max_threads: integer
 *
 * Sets the maximum number of threads to use when processing packages.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_max_threads (CraContext *ctx, guint max_threads)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->max_threads = max_threads;
}

/**
 * cra_context_set_old_metadata:
 * @ctx: A #CraContext
 * @old_metadata: filename, or %NULL
 *
 * Sets the filename location of the old metadata file.
 * Note: the old metadata must have been built with cache-ids to be useful.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_old_metadata (CraContext *ctx, const gchar *old_metadata)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->old_metadata = g_strdup (old_metadata);
}

/**
 * cra_context_set_extra_appstream:
 * @ctx: A #CraContext
 * @extra_appstream: directory name, or %NULL
 *
 * Sets the location of a directory which is used for supplimental AppStream
 * files.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_extra_appstream (CraContext *ctx, const gchar *extra_appstream)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->extra_appstream = g_strdup (extra_appstream);
}

/**
 * cra_context_set_extra_appdata:
 * @ctx: A #CraContext
 * @extra_appdata: directory name, or %NULL
 *
 * Sets the location of a directory which is used for supplimental AppData
 * files.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_extra_appdata (CraContext *ctx, const gchar *extra_appdata)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->extra_appdata = g_strdup (extra_appdata);
}

/**
 * cra_context_set_extra_screenshots:
 * @ctx: A #CraContext
 * @extra_screenshots: directory name, or %NULL
 *
 * Sets the location of a directory which is used for supplimental screenshot
 * files.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_extra_screenshots (CraContext *ctx, const gchar *extra_screenshots)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->extra_screenshots = g_strdup (extra_screenshots);
}

/**
 * cra_context_set_screenshot_uri:
 * @ctx: A #CraContext
 * @screenshot_uri: Remote URI root, e.g. "http://www.mysite.com/screenshots/"
 *
 * Sets the remote screenshot URI for screenshots.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_screenshot_uri (CraContext *ctx, const gchar *screenshot_uri)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->screenshot_uri = g_strdup (screenshot_uri);
}

/**
 * cra_context_set_log_dir:
 * @ctx: A #CraContext
 * @log_dir: directory
 *
 * Sets the log directory to use when building metadata.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_log_dir (CraContext *ctx, const gchar *log_dir)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->log_dir = g_strdup (log_dir);
}

/**
 * cra_context_set_cache_dir:
 * @ctx: A #CraContext
 * @cache_dir: directory
 *
 * Sets the cache directory to use when building metadata.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_cache_dir (CraContext *ctx, const gchar *cache_dir)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->cache_dir = g_strdup (cache_dir);
}

/**
 * cra_context_set_temp_dir:
 * @ctx: A #CraContext
 * @temp_dir: directory
 *
 * Sets the temporary directory to use when building metadata.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_temp_dir (CraContext *ctx, const gchar *temp_dir)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->temp_dir = g_strdup (temp_dir);
}

/**
 * cra_context_set_output_dir:
 * @ctx: A #CraContext
 * @output_dir: directory
 *
 * Sets the output directory to use when building metadata.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_output_dir (CraContext *ctx, const gchar *output_dir)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->output_dir = g_strdup (output_dir);
}

/**
 * cra_context_set_basename:
 * @ctx: A #CraContext
 * @basename: AppStream basename, e.g. "fedora-21"
 *
 * Sets the basename for the two metadata files.
 *
 * Since: 0.1.0
 **/
void
cra_context_set_basename (CraContext *ctx, const gchar *basename)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	priv->basename = g_strdup (basename);
}

/**
 * cra_context_get_extra_package:
 * @ctx: A #CraContext
 * @pkgname: package name
 *
 * Gets an extra package that should be used when processing an application.
 *
 * Returns: a pakage name, or %NULL
 *
 * Since: 0.1.0
 **/
const gchar *
cra_context_get_extra_package (CraContext *ctx, const gchar *pkgname)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return cra_glob_value_search (priv->extra_pkgs, pkgname);
}

/**
 * cra_context_get_use_package_cache:
 * @ctx: A #CraContext
 *
 * Gets if the package cache should be used.
 *
 * Returns: boolean
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_get_use_package_cache (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->use_package_cache;
}

/**
 * cra_context_get_extra_checks:
 * @ctx: A #CraContext
 *
 * Gets if extra checks should be performed.
 *
 * Returns: boolean
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_get_extra_checks (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->extra_checks;
}

/**
 * cra_context_get_add_cache_id:
 * @ctx: A #CraContext
 *
 * Gets if the cache_id should be added to the metadata.
 *
 * Returns: boolean
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_get_add_cache_id (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->add_cache_id;
}

/**
 * cra_context_get_temp_dir:
 * @ctx: A #CraContext
 *
 * Gets the temporary directory to use
 *
 * Returns: directory
 *
 * Since: 0.1.0
 **/
const gchar *
cra_context_get_temp_dir (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->temp_dir;
}

/**
 * cra_context_get_plugins:
 * @ctx: A #CraContext
 *
 * Gets the plugins available to the metadata extractor.
 *
 * Returns: array of plugins
 *
 * Since: 0.1.0
 **/
GPtrArray *
cra_context_get_plugins (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->plugins;
}

/**
 * cra_context_get_packages:
 * @ctx: A #CraContext
 *
 * Returns the packages already added to the context.
 *
 * Returns: (transfer none) (element-type CraPackage): array of packages
 *
 * Since: 0.1.0
 **/
GPtrArray *
cra_context_get_packages (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->packages;
}

/**
 * cra_context_add_filename:
 * @ctx: A #CraContext
 * @filename: package filename
 * @error: A #GError or %NULL
 *
 * Adds a filename to the list of packages to be processed
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_add_filename (CraContext *ctx, const gchar *filename, GError **error)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
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
	if (cra_glob_value_search (priv->blacklisted_pkgs,
				   cra_package_get_name (pkg)) != NULL) {
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "%s is blacklisted",
				 cra_package_get_filename (pkg));
		return TRUE;
	}

	/* add to array */
	g_ptr_array_add (priv->packages, g_object_ref (pkg));
	return TRUE;
}

/**
 * cra_context_get_file_globs:
 * @ctx: A #CraContext
 *
 * Gets the list of file globs added by plugins.
 *
 * Returns: file globs
 *
 * Since: 0.1.0
 **/
GPtrArray *
cra_context_get_file_globs (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	return priv->file_globs;
}

/**
 * cra_context_setup:
 * @ctx: A #CraContext
 * @error: A #GError or %NULL
 *
 * Sets up the context ready for use.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_setup (CraContext *ctx, GError **error)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);

	/* load plugins */
	if (!cra_plugin_loader_setup (priv->plugins, error))
		return FALSE;

	/* get a cache of the file globs */
	priv->file_globs = cra_plugin_loader_get_globs (priv->plugins);

	/* add old metadata */
	if (priv->old_metadata != NULL) {
		_cleanup_object_unref_ GFile *file = NULL;
		file = g_file_new_for_path (priv->old_metadata);
		if (!as_store_from_file (priv->old_md_cache, file,
					 NULL, NULL, error))
			return FALSE;
	}

	/* add any extra applications */
	if (priv->extra_appstream != NULL &&
	    g_file_test (priv->extra_appstream, G_FILE_TEST_EXISTS)) {
		if (!cra_utils_add_apps_from_dir (&priv->apps,
						  priv->extra_appstream,
						  error))
			return FALSE;
		g_print ("Added extra %i apps\n", g_list_length (priv->apps));
	}

	return TRUE;
}

/**
 * cra_task_process_func:
 **/
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	CraTask *task = (CraTask *) data;
	_cleanup_error_free_ GError *error = NULL;

	/* just run the task */
	if (!cra_task_process (task, &error))
		g_warning ("Failed to run task: %s", error->message);
}

/**
 * cra_context_write_icons:
 **/
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
 **/
static gboolean
cra_context_write_xml (CraContext *ctx,
		       const gchar *output_dir,
		       const gchar *basename,
		       GError **error)
{
	AsApp *app;
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	GList *l;
	_cleanup_free_ gchar *filename = NULL;
	_cleanup_object_unref_ AsStore *store;
	_cleanup_object_unref_ GFile *file;

	store = as_store_new ();
	for (l = priv->apps; l != NULL; l = l->next) {
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
	as_store_set_api_version (store, priv->api_version);
	return as_store_to_file (store,
				 file,
				 AS_NODE_TO_XML_FLAG_ADD_HEADER |
				 AS_NODE_TO_XML_FLAG_FORMAT_INDENT |
				 AS_NODE_TO_XML_FLAG_FORMAT_MULTILINE,
				 NULL, error);
}

/**
 * cra_context_process:
 * @ctx: A #CraContext
 * @error: A #GError or %NULL
 *
 * Processes all the packages that have been added to the context.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_process (CraContext *ctx, GError **error)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	CraPackage *pkg;
	CraTask *task;
	GThreadPool *pool;
	gboolean ret = FALSE;
	guint i;
	_cleanup_ptrarray_unref_ GPtrArray *tasks = NULL;

	/* create thread pool */
	pool = g_thread_pool_new (cra_task_process_func,
				  ctx,
				  priv->max_threads,
				  TRUE,
				  error);
	if (pool == NULL)
		goto out;

	/* add each package */
	g_print ("Processing packages...\n");
	tasks = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	for (i = 0; i < priv->packages->len; i++) {
		pkg = g_ptr_array_index (priv->packages, i);
		if (!cra_package_get_enabled (pkg)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_DEBUG,
					 "%s is not enabled",
					 cra_package_get_nevr (pkg));
			cra_package_log_flush (pkg, NULL);
			continue;
		}

		/* set locations of external resources */
		cra_package_set_config (pkg, "AppDataExtra", priv->extra_appdata);
		cra_package_set_config (pkg, "ScreenshotsExtra", priv->extra_screenshots);
		cra_package_set_config (pkg, "MirrorURI", priv->screenshot_uri);
		cra_package_set_config (pkg, "LogDir", priv->log_dir);
		cra_package_set_config (pkg, "CacheDir", priv->cache_dir);
		cra_package_set_config (pkg, "TempDir", priv->temp_dir);
		cra_package_set_config (pkg, "OutputDir", priv->output_dir);

		/* create task */
		task = cra_task_new (ctx);
		cra_task_set_id (task, i);
		cra_task_set_package (task, pkg);
		g_ptr_array_add (tasks, task);

		/* add task to pool */
		ret = g_thread_pool_push (pool, task, error);
		if (!ret)
			goto out;
	}

	/* wait for them to finish */
	g_thread_pool_free (pool, FALSE, TRUE);

	/* merge */
	g_print ("Merging applications...\n");
	cra_plugin_loader_merge (priv->plugins, &priv->apps);

	/* write XML file */
	ret = cra_context_write_xml (ctx, priv->output_dir, priv->basename, error);
	if (!ret)
		goto out;

	/* write icons archive */
	ret = cra_context_write_icons (ctx,
				       priv->temp_dir,
				       priv->output_dir,
				       priv->basename,
				       error);
	if (!ret)
		goto out;
out:
	return ret;
}

/**
 * cra_context_disable_older_pkgs:
 * @ctx: A #CraContext
 *
 * Disable older packages that have been added to the context.
 *
 * Since: 0.1.0
 **/
void
cra_context_disable_older_pkgs (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	CraPackage *found;
	CraPackage *pkg;
	const gchar *key;
	guint i;
	_cleanup_hashtable_unref_ GHashTable *newest;

	newest = g_hash_table_new_full (g_str_hash, g_str_equal,
					g_free, (GDestroyNotify) g_object_unref);
	for (i = 0; i < priv->packages->len; i++) {
		pkg = CRA_PACKAGE (g_ptr_array_index (priv->packages, i));
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
 * cra_context_find_in_cache:
 * @ctx: A #CraContext
 * @filename: cache-id
 *
 * Finds an application in the cache. This will only return results if
 * cra_context_set_old_metadata() has been used.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_context_find_in_cache (CraContext *ctx, const gchar *filename)
{
	AsApp *app;
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	guint i;
	_cleanup_free_ gchar *cache_id;
	_cleanup_ptrarray_unref_ GPtrArray *apps;

	cache_id = cra_utils_get_cache_id_for_filename (filename);
	apps = as_store_get_apps_by_metadata (priv->old_md_cache,
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
 * cra_context_find_by_pkgname:
 * @ctx: A #CraContext
 * @pkgname: a package name
 *
 * Find a package from its name.
 *
 * Returns: (transfer none): a #CraPackage, or %NULL for not found.
 *
 * Since: 0.1.0
 **/
CraPackage *
cra_context_find_by_pkgname (CraContext *ctx, const gchar *pkgname)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	CraPackage *pkg;
	guint i;

	for (i = 0; i < priv->packages->len; i++) {
		pkg = g_ptr_array_index (priv->packages, i);
		if (g_strcmp0 (cra_package_get_name (pkg), pkgname) == 0)
			return pkg;
	}
	return NULL;
}

/**
 * cra_context_add_extra_pkg:
 **/
static void
cra_context_add_extra_pkg (CraContext *ctx, const gchar *pkg1, const gchar *pkg2)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	g_ptr_array_add (priv->extra_pkgs, cra_glob_value_new (pkg1, pkg2));
}

/**
 * cra_context_add_blacklist_pkg:
 **/
static void
cra_context_add_blacklist_pkg (CraContext *ctx, const gchar *pkg)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	g_ptr_array_add (priv->blacklisted_pkgs, cra_glob_value_new (pkg, ""));
}

/**
 * cra_context_add_app:
 * @ctx: A #CraContext
 * @app: A #CraApp
 *
 * Adds an application to the context.
 *
 * Since: 0.1.0
 **/
void
cra_context_add_app (CraContext *ctx, CraApp *app)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);
	g_mutex_lock (&priv->apps_mutex);
	cra_plugin_add_app (&priv->apps, app);
	g_mutex_unlock (&priv->apps_mutex);
}

/**
 * cra_context_finalize:
 **/
static void
cra_context_finalize (GObject *object)
{
	CraContext *ctx = CRA_CONTEXT (object);
	CraContextPrivate *priv = GET_PRIVATE (ctx);

	g_object_unref (priv->old_md_cache);
	cra_plugin_loader_free (priv->plugins);
	g_ptr_array_unref (priv->packages);
	g_ptr_array_unref (priv->extra_pkgs);
	g_list_foreach (priv->apps, (GFunc) g_object_unref, NULL);
	g_list_free (priv->apps);
	g_ptr_array_unref (priv->blacklisted_pkgs);
	g_ptr_array_unref (priv->file_globs);
	g_mutex_clear (&priv->apps_mutex);
	g_free (priv->old_metadata);
	g_free (priv->extra_appstream);
	g_free (priv->extra_appdata);
	g_free (priv->extra_screenshots);
	g_free (priv->screenshot_uri);
	g_free (priv->log_dir);
	g_free (priv->cache_dir);
	g_free (priv->temp_dir);
	g_free (priv->output_dir);
	g_free (priv->basename);

	G_OBJECT_CLASS (cra_context_parent_class)->finalize (object);
}

/**
 * cra_context_init:
 **/
static void
cra_context_init (CraContext *ctx)
{
	CraContextPrivate *priv = GET_PRIVATE (ctx);

	priv->blacklisted_pkgs = cra_glob_value_array_new ();
	priv->plugins = cra_plugin_loader_new ();
	priv->packages = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	priv->extra_pkgs = cra_glob_value_array_new ();
	g_mutex_init (&priv->apps_mutex);
	priv->old_md_cache = as_store_new ();
	priv->max_threads = 1;

	/* add extra data */
	cra_context_add_extra_pkg (ctx, "alliance-libs", "alliance");
	cra_context_add_extra_pkg (ctx, "beneath-a-steel-sky*", "scummvm");
	cra_context_add_extra_pkg (ctx, "coq-coqide", "coq");
	cra_context_add_extra_pkg (ctx, "drascula*", "scummvm");
	cra_context_add_extra_pkg (ctx, "efte-*", "efte-common");
	cra_context_add_extra_pkg (ctx, "fcitx-*", "fcitx-data");
	cra_context_add_extra_pkg (ctx, "flight-of-the-amazon-queen", "scummvm");
	cra_context_add_extra_pkg (ctx, "gcin", "gcin-data");
	cra_context_add_extra_pkg (ctx, "hotot-gtk", "hotot-common");
	cra_context_add_extra_pkg (ctx, "hotot-qt", "hotot-common");
	cra_context_add_extra_pkg (ctx, "java-1.7.0-openjdk-devel", "java-1.7.0-openjdk");
	cra_context_add_extra_pkg (ctx, "kchmviewer-qt", "kchmviewer");
	cra_context_add_extra_pkg (ctx, "libreoffice-*", "libreoffice-core");
	cra_context_add_extra_pkg (ctx, "lure", "scummvm");
	cra_context_add_extra_pkg (ctx, "nntpgrab-gui", "nntpgrab-core");
	cra_context_add_extra_pkg (ctx, "projectM-*", "libprojectM-qt");
	cra_context_add_extra_pkg (ctx, "scummvm-tools", "scummvm");
	cra_context_add_extra_pkg (ctx, "speed-dreams", "speed-dreams-robots-base");
	cra_context_add_extra_pkg (ctx, "switchdesk-gui", "switchdesk");
	cra_context_add_extra_pkg (ctx, "transmission-*", "transmission-common");
	cra_context_add_extra_pkg (ctx, "calligra-krita", "calligra-core");

	/* add blacklisted packages */
	cra_context_add_blacklist_pkg (ctx, "beneath-a-steel-sky-cd");
	cra_context_add_blacklist_pkg (ctx, "anaconda");
	cra_context_add_blacklist_pkg (ctx, "mate-control-center");
	cra_context_add_blacklist_pkg (ctx, "lxde-common");
	cra_context_add_blacklist_pkg (ctx, "xscreensaver-*");
	cra_context_add_blacklist_pkg (ctx, "bmpanel2-cfg");
}

/**
 * cra_context_class_init:
 **/
static void
cra_context_class_init (CraContextClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_context_finalize;
}

/**
 * cra_context_new:
 *
 * Creates a new high-level instance.
 *
 * Returns: a #CraContext
 *
 * Since: 0.1.0
 **/
CraContext *
cra_context_new (void)
{
	CraContext *context;
	context = g_object_new (CRA_TYPE_CONTEXT, NULL);
	return CRA_CONTEXT (context);
}
