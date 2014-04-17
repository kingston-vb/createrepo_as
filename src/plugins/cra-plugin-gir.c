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

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "gir";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/*/*.gir");
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/*/*.gir", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_process_gir:
 */
static gboolean
cra_plugin_process_gir (CraApp *app,
			const gchar *tmpdir,
			const gchar *filename,
			GError **error)
{
	GFile *file = NULL;
	GNode *l;
	GNode *node = NULL;
	const gchar *name;
	const gchar *version;
	gboolean ret = TRUE;
	gchar *filename_full;

	/* load file */
	filename_full = g_build_filename (tmpdir, filename, NULL);
	file = g_file_new_for_path (filename_full);
	node = as_node_from_file (file, AS_NODE_FROM_XML_FLAG_NONE, NULL, error);
	if (node == NULL) {
		ret = FALSE;
		goto out;
	}

	/* look for includes */
	l = as_node_find (node, "repository");
	if (l == NULL)
		goto out;
	for (l = l->children; l != NULL; l = l->next) {
		if (g_strcmp0 (as_node_get_name (l), "include") != 0)
			continue;
		name = as_node_get_attribute (l, "name");
		version = as_node_get_attribute (l, "version");
		if (g_strcmp0 (name, "Gtk") == 0 &&
		    g_strcmp0 (version, "3.0") == 0) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-GTK3", "", -1);
		}
	}
out:
	if (node != NULL)
		as_node_unref (node);
	if (file != NULL)
		g_object_unref (file);
	g_free (filename_full);
	return ret;
}

/**
 * cra_plugin_process_app:
 */
gboolean
cra_plugin_process_app (CraPlugin *plugin,
			CraPackage *pkg,
			CraApp *app,
			const gchar *tmpdir,
			GError **error)
{
	gboolean ret = TRUE;
	gchar **filelist;
	guint i;

	/* look for any GIR files */
	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (!_cra_plugin_check_filename (filelist[i]))
			continue;
		ret = cra_plugin_process_gir (app, tmpdir, filelist[i], error);
		if (!ret)
			goto out;
	}
out:
	return ret;
}
