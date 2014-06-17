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
 * SECTION:cra-plugin
 * @short_description: Generic plugin helpers.
 * @stability: Unstable
 *
 * Utilities for plugins.
 */

#include "config.h"

#include <glib.h>

#include "cra-plugin.h"
#include "cra-utils.h"

/**
 * cra_plugin_set_enabled:
 * @plugin: A #CraPlugin
 * @enabled: boolean
 *
 * Enables or disables a plugin.
 *
 * Since: 0.1.0
 **/
void
cra_plugin_set_enabled (CraPlugin *plugin, gboolean enabled)
{
	plugin->enabled = enabled;
}

/**
 * cra_plugin_process:
 * @plugin: A #CraPlugin
 * @pkg: A #CraPackage
 * @tmpdir: the temporary location
 * @error: A #GError or %NULL
 *
 * Runs a function on a specific plugin.
 *
 * Returns: (transfer none) (element-type CraApp): applications
 *
 * Since: 0.1.0
 **/
GList *
cra_plugin_process (CraPlugin *plugin,
		    CraPackage *pkg,
		    const gchar *tmpdir,
		    GError **error)
{
	CraPluginProcessFunc plugin_func = NULL;
	gboolean ret;

	/* run each plugin */
	cra_package_log (pkg,
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "Running cra_plugin_process() from %s",
			 plugin->name);
	ret = g_module_symbol (plugin->module,
			       "cra_plugin_process",
			       (gpointer *) &plugin_func);
	if (!ret) {
		g_set_error_literal (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "no cra_plugin_process");
		return NULL;
	}
	return plugin_func (plugin, pkg, tmpdir, error);
}

/**
 * cra_plugin_add_app:
 * @list: (element-type CraApp): A list of #CraApp's
 * @app: A #CraApp
 *
 * Adds an application to a list.
 *
 * Since: 0.1.0
 **/
void
cra_plugin_add_app (GList **list, CraApp *app)
{
	*list = g_list_prepend (*list, g_object_ref (app));
}

/**
 * cra_plugin_add_glob:
 * @array: (element-type utf8): A #GPtrArray
 * @glob: a filename glob
 *
 * Adds a glob from the plugin.
 *
 * Since: 0.1.0
 **/
void
cra_plugin_add_glob (GPtrArray *array, const gchar *glob)
{
	g_ptr_array_add (array, cra_glob_value_new (glob, ""));
}
