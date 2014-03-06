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
#include <sqlite3.h>

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "ibus-sqlite";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/ibus-table/tables/*.db");
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/ibus-table/tables/*.db", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_check_filename:
 */
gboolean
cra_plugin_check_filename (CraPlugin *plugin, const gchar *filename)
{
	return _cra_plugin_check_filename (filename);
}

/**
 * cra_plugin_sqlite_callback_cb:
 */
static int
cra_plugin_sqlite_callback_cb (void *user_data, int argc, char **argv, char **data)
{
	gchar **tmp = (gchar **) user_data;
	*tmp = g_strdup (argv[1]);
	return 0;
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraPlugin *plugin,
			     CraPackage *pkg,
			     const gchar *filename,
			     GList **apps,
			     const gchar *tmpdir,
			     GError **error)
{
	CraApp *app = NULL;
	gboolean ret = TRUE;
	gchar *basename = NULL;
	gchar *description = NULL;
	gchar *error_msg = 0;
	gchar *filename_tmp;
	gchar *name = NULL;
	gint rc;
	sqlite3 *db = NULL;

	/* open IME database */
	filename_tmp = g_build_filename (tmpdir, filename, NULL);
	rc = sqlite3_open (filename_tmp, &db);
	if (rc) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Can't open database: %s",
			     sqlite3_errmsg (db));
		goto out;
	}

	/* get name */
	rc = sqlite3_exec(db, "SELECT * FROM ime WHERE attr = 'name' LIMIT 1;",
			  cra_plugin_sqlite_callback_cb,
			  &name, &error_msg);
	if (rc != SQLITE_OK) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Can't get IME name from %s: %s",
			     filename, error_msg);
		sqlite3_free(error_msg);
		goto out;
	}

	/* get description */
	rc = sqlite3_exec(db, "SELECT * FROM ime WHERE attr = 'description' LIMIT 1;",
			  cra_plugin_sqlite_callback_cb,
			  &description, &error_msg);
	if (rc != SQLITE_OK) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Can't get IME name from %s: %s",
			     filename, error_msg);
		sqlite3_free(error_msg);
		goto out;
	}

	/* this is _required_ */
	if (name == NULL || description == NULL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "No 'name' and 'description' in %s",
			     filename);
		goto out;
	}

	/* create new app */
	basename = g_path_get_basename (filename);
	app = cra_app_new (pkg, basename);
	cra_app_set_type_id (app, "inputmethod");
	cra_app_add_category (app, "Addons");
	cra_app_add_category (app, "InputSources");
	cra_app_set_icon (app, "system-run-symbolic");
	cra_app_set_cached_icon (app, FALSE);
	cra_app_set_name (app, "C", name);
	cra_app_set_comment (app, "C", description);
	cra_app_set_requires_appdata (app, TRUE);
	cra_plugin_add_app (apps, app);
out:
	g_free (name);
	g_free (basename);
	g_free (filename_tmp);
	g_free (description);
	if (app != NULL)
		g_object_unref (app);
	if (db != NULL)
		sqlite3_close (db);
	return ret;
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
	gboolean ret;
	GList *apps = NULL;
	guint i;
	gchar **filelist;

	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (!_cra_plugin_check_filename (filelist[i]))
			continue;
		ret = cra_plugin_process_filename (plugin,
						   pkg,
						   filelist[i],
						   &apps,
						   tmpdir,
						   error);
		if (!ret) {
			/* FIXME: free apps? */
			apps = NULL;
			goto out;
		}
	}

	/* no desktop files we care about */
	if (apps == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_filename (pkg));
		goto out;
	}
out:
	return apps;
}
