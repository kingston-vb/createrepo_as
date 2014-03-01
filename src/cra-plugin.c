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
gboolean
cra_plugin_process (CraPlugin *plugin, const gchar *tmpdir, GError **error)
{
	CraPluginProcessFunc plugin_func = NULL;
	gboolean ret;

	/* run each plugin */
	g_debug ("Running cra_plugin_process()");
	ret = g_module_symbol (plugin->module,
			       "cra_plugin_process",
			       (gpointer *) &plugin_func);
	if (!ret)
		return TRUE;
	return plugin_func (plugin, tmpdir, error);
}
