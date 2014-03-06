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
#include "cra-utils.h"

/**
 * cra_plugin_set_enabled:
 **/
void
cra_plugin_set_enabled (CraPlugin *plugin, gboolean enabled)
{
	plugin->enabled = enabled;
}

/**
 * cra_plugin_process:
 */
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
			 CRA_PACKAGE_LOG_LEVEL_INFO,
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
 */
void
cra_plugin_add_app (GList **list, CraApp *app)
{
	*list = g_list_prepend (*list, g_object_ref (app));
}

/**
 * cra_plugin_add_glob:
 */
void
cra_plugin_add_glob (GPtrArray *array, const gchar *glob)
{
	gchar *glob2;
	glob2 = g_strdup_printf (".%s", glob);
	g_ptr_array_add (array, cra_glob_value_new (glob2, ""));
	g_free (glob2);
}
