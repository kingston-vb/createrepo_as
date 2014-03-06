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

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "nm";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/bin/*");
}

static gboolean
cra_plugin_nm_app (CraApp *app, const gchar *filename, GError **error)
{
	const gchar *argv[] = { "/usr/bin/nm", "-D", filename, NULL };
	gboolean ret;
	gchar *data_err = NULL;
	gchar *data_out = NULL;

	ret = g_spawn_sync (NULL, (gchar **) argv, NULL,
			    G_SPAWN_DEFAULT,
			    NULL, NULL,
			    &data_out,
			    &data_err,
			    NULL, error);
	if (!ret)
		goto out;
	if (g_strstr_len (data_out, -1, "gtk_application_set_app_menu") != NULL)
		cra_app_add_metadata (app, "X-Kudo-UsesAppMenu", "");
out:
	g_free (data_err);
	g_free (data_out);
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
	gboolean ret;
	gchar **filelist;
	gchar *filename;
	GError *error_local = NULL;
	guint i;

	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (!g_str_has_prefix (filelist[i], "/usr/bin/"))
			continue;
		filename = g_build_filename (tmpdir, filelist[i], NULL);
		ret = cra_plugin_nm_app (app, filename, &error_local);
		if (!ret) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to run nm on %s: %s",
					 filename,
					 error_local->message);
			g_clear_error (&error_local);
		}
		g_free (filename);
	}
	return TRUE;
}
