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

#include <config.h>
#include <fnmatch.h>

#include <cra-plugin.h>

struct CraPluginPrivate {
	GRegex			*regex;
};

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "gstreamer";
}

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
}

/**
 * cra_plugin_check_filename:
 */
gboolean
cra_plugin_check_filename (CraPlugin *plugin, const gchar *filename)
{
	if (fnmatch ("/usr/lib64/gstreamer-1.0/libgst*.so", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_process:
 */
gboolean
cra_plugin_process (CraPlugin *plugin, const gchar *tmpdir, GError **error)
{
	return TRUE;
}
