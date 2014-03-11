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

#include <glib.h>

#include "cra-plugin.h"
#include "cra-plugin-loader.h"

/**
 * cra_plugin_loader_plugin_free:
 **/
static void
cra_plugin_loader_plugin_free (CraPlugin *plugin)
{
	g_free (plugin->priv);
	g_free (plugin->name);
	g_module_close (plugin->module);
	g_slice_free (CraPlugin, plugin);
}

/**
 * cra_plugin_loader_match_fn:
 **/
CraPlugin *
cra_plugin_loader_match_fn (GPtrArray *plugins, const gchar *filename)
{
	gboolean ret;
	CraPluginCheckFilenameFunc plugin_func = NULL;
	CraPlugin *plugin;
	guint i;

	/* run each plugin */
	for (i = 0; i < plugins->len; i++) {
		plugin = g_ptr_array_index (plugins, i);
		ret = g_module_symbol (plugin->module,
				       "cra_plugin_check_filename",
				       (gpointer *) &plugin_func);
		if (!ret)
			continue;
		if (plugin_func (plugin, filename))
			return plugin;
	}
	return NULL;
}

/**
 * cra_plugin_loader_process_app:
 **/
gboolean
cra_plugin_loader_process_app (GPtrArray *plugins,
			       CraPackage *pkg,
			       CraApp *app,
			       const gchar *tmpdir,
			       GError **error)
{
	gboolean ret;
	CraPluginProcessAppFunc plugin_func = NULL;
	CraPlugin *plugin;
	guint i;

	/* run each plugin */
	for (i = 0; i < plugins->len; i++) {
		plugin = g_ptr_array_index (plugins, i);
		ret = g_module_symbol (plugin->module,
				       "cra_plugin_process_app",
				       (gpointer *) &plugin_func);
		if (!ret)
			continue;
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Running cra_plugin_process_app() from %s",
				 plugin->name);
		ret = plugin_func (plugin, pkg, app, tmpdir, error);
		if (!ret)
			goto out;
	}
	ret = TRUE;
out:
	return ret;
}

/**
 * cra_plugin_loader_run:
 **/
static void
cra_plugin_loader_run (GPtrArray *plugins, const gchar *function_name)
{
	gboolean ret;
	CraPluginFunc plugin_func = NULL;
	CraPlugin *plugin;
	guint i;

	/* run each plugin */
	for (i = 0; i < plugins->len; i++) {
		plugin = g_ptr_array_index (plugins, i);
		ret = g_module_symbol (plugin->module,
				       function_name,
				       (gpointer *) &plugin_func);
		if (!ret)
			continue;
		plugin_func (plugin);
	}
}

/**
 * cra_plugin_loader_get_globs:
 */
GPtrArray *
cra_plugin_loader_get_globs (GPtrArray *plugins)
{
	gboolean ret;
	CraPluginGetGlobsFunc plugin_func = NULL;
	CraPlugin *plugin;
	guint i;
	GPtrArray *globs;

	/* run each plugin */
	globs = cra_glob_value_array_new ();
	for (i = 0; i < plugins->len; i++) {
		plugin = g_ptr_array_index (plugins, i);
		ret = g_module_symbol (plugin->module,
				       "cra_plugin_add_globs",
				       (gpointer *) &plugin_func);
		if (!ret)
			continue;
		plugin_func (plugin, globs);
	}
	return globs;
}

/**
 * cra_plugin_loader_merge:
 */
void
cra_plugin_loader_merge (GPtrArray *plugins, GList **apps)
{
	const gchar *key;
	const gchar *tmp;
	CraApp *app;
	CraApp *found;
	CraPluginMergeFunc plugin_func = NULL;
	CraPlugin *plugin;
	gboolean ret;
	GHashTable *hash;
	GList *l;
	guint i;

	/* run each plugin */
	for (i = 0; i < plugins->len; i++) {
		plugin = g_ptr_array_index (plugins, i);
		ret = g_module_symbol (plugin->module,
				       "cra_plugin_merge",
				       (gpointer *) &plugin_func);
		if (!ret)
			continue;
		plugin_func (plugin, apps);
	}

	/* FIXME: move to font plugin */
	for (l = *apps; l != NULL; l = l->next) {
		app = CRA_APP (l->data);
		cra_app_remove_metadata (app, "FontFamily");
		cra_app_remove_metadata (app, "FontFullName");
		cra_app_remove_metadata (app, "FontIconText");
		cra_app_remove_metadata (app, "FontParent");
		cra_app_remove_metadata (app, "FontSampleText");
		cra_app_remove_metadata (app, "FontSubFamily");
		cra_app_remove_metadata (app, "FontClassifier");
	}

	/* deduplicate */
	hash = g_hash_table_new (g_str_hash, g_str_equal);
	for (l = *apps; l != NULL; l = l->next) {
		app = CRA_APP (l->data);
		key = cra_app_get_id_full (app);
		found = g_hash_table_lookup (hash, key);
		if (found == NULL) {
			g_hash_table_insert (hash,
					     (gpointer) key,
					     (gpointer) app);
			continue;
		}
		tmp = cra_package_get_nevr (cra_app_get_package (found));
		cra_app_add_veto (app, "duplicate of %s", tmp);
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "duplicate %s not included as added from %s",
				 key, tmp);
	}
	g_hash_table_unref (hash);
}

/**
 * cra_plugin_loader_open_plugin:
 */
static CraPlugin *
cra_plugin_loader_open_plugin (GPtrArray *plugins,
			       const gchar *filename)
{
	gboolean ret;
	GModule *module;
	CraPluginGetNameFunc plugin_name = NULL;
	CraPlugin *plugin = NULL;

	module = g_module_open (filename, 0);
	if (module == NULL) {
		g_warning ("failed to open plugin %s: %s",
			   filename, g_module_error ());
		goto out;
	}

	/* get description */
	ret = g_module_symbol (module,
			       "cra_plugin_get_name",
			       (gpointer *) &plugin_name);
	if (!ret) {
		g_warning ("Plugin %s requires name", filename);
		g_module_close (module);
		goto out;
	}

	/* print what we know */
	plugin = g_slice_new0 (CraPlugin);
	plugin->enabled = TRUE;
	plugin->module = module;
	plugin->is_native = TRUE;
	plugin->name = g_strdup (plugin_name ());
	g_debug ("opened plugin %s: %s", filename, plugin->name);

	/* add to array */
	g_ptr_array_add (plugins, plugin);
out:
	return plugin;
}

/**
 * cra_plugin_loader_sort_cb:
 */
static gint
cra_plugin_loader_sort_cb (gconstpointer a, gconstpointer b)
{
	CraPlugin **plugin_a = (CraPlugin **) a;
	CraPlugin **plugin_b = (CraPlugin **) b;
	return -g_strcmp0 ((*plugin_a)->name, (*plugin_b)->name);
}

/**
 * cra_plugin_loader_setup:
 */
gboolean
cra_plugin_loader_setup (GPtrArray *plugins, GError **error)
{
	const gchar *filename_tmp;
	const gchar *location = "./plugins/.libs/";
	gboolean ret = TRUE;
	gchar *filename_plugin;
	GDir *dir;

	/* search in the plugin directory for plugins */
	dir = g_dir_open (location, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}

	/* try to open each plugin */
	g_debug ("searching for plugins in %s", location);
	do {
		filename_tmp = g_dir_read_name (dir);
		if (filename_tmp == NULL)
			break;
		if (!g_str_has_suffix (filename_tmp, ".so"))
			continue;
		filename_plugin = g_build_filename (location,
						    filename_tmp,
						    NULL);
		cra_plugin_loader_open_plugin (plugins, filename_plugin);
		g_free (filename_plugin);
	} while (TRUE);

	/* run the plugins */
	cra_plugin_loader_run (plugins, "cra_plugin_initialize");
	g_ptr_array_sort (plugins, cra_plugin_loader_sort_cb);
out:
	if (dir != NULL)
		g_dir_close (dir);
	return ret;
}

/**
 * cra_plugin_loader_new:
 */
GPtrArray *
cra_plugin_loader_new (void)
{
	return g_ptr_array_new_with_free_func ((GDestroyNotify) cra_plugin_loader_plugin_free);
}

/**
 * cra_plugin_loader_free:
 */
void
cra_plugin_loader_free (GPtrArray *plugins)
{
	cra_plugin_loader_run (plugins, "cra_plugin_destroy");
	g_ptr_array_unref (plugins);
}
