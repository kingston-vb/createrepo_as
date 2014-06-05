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
	gboolean ret;
	_cleanup_free_ gchar *data_err = NULL;
	_cleanup_free_ gchar *data_out = NULL;
	const gchar *argv[] = { "/usr/bin/nm",
				"--dynamic",
				"--no-sort",
				"--undefined-only",
				filename,
				NULL };

	ret = g_spawn_sync (NULL, (gchar **) argv, NULL,
#if GLIB_CHECK_VERSION(2,40,0)
			    G_SPAWN_CLOEXEC_PIPES,
#else
			    G_SPAWN_DEFAULT,
#endif
			    NULL, NULL,
			    &data_out,
			    &data_err,
			    NULL, error);
	if (!ret)
		return FALSE;
	if (g_strstr_len (data_out, -1, "gtk_application_new") != NULL)
		as_app_add_metadata (AS_APP (app), "X-Kudo-GTK3", "", -1);
	if (g_strstr_len (data_out, -1, "gtk_application_set_app_menu") != NULL)
		as_app_add_metadata (AS_APP (app), "X-Kudo-UsesAppMenu", "", -1);
	return TRUE;
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
	gchar **filelist;
	guint i;

	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		GError *error_local = NULL;
		_cleanup_free_ gchar *filename = NULL;

		if (!g_str_has_prefix (filelist[i], "/usr/bin/"))
			continue;
		if (as_app_get_metadata_item (AS_APP (app), "X-Kudo-UsesAppMenu") != NULL)
			break;
		filename = g_build_filename (tmpdir, filelist[i], NULL);
		if (!cra_plugin_nm_app (app, filename, &error_local)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Failed to run nm on %s: %s",
					 filename,
					 error_local->message);
			g_clear_error (&error_local);
		}
	}
	return TRUE;
}
