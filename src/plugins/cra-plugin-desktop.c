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

#include <config.h>
#include <fnmatch.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "desktop";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/applications/*.desktop");
	cra_plugin_add_glob (globs, "/usr/share/applications/kde4/*.desktop");
	cra_plugin_add_glob (globs, "/usr/share/icons/hicolor/*/apps/*");
	cra_plugin_add_glob (globs, "/usr/share/pixmaps/*");
	cra_plugin_add_glob (globs, "/usr/share/icons/*");
	cra_plugin_add_glob (globs, "/usr/share/*/icons/*");
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/applications/*.desktop", filename, 0) == 0)
		return TRUE;
	if (fnmatch ("/usr/share/applications/kde4/*.desktop", filename, 0) == 0)
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
 * cra_app_load_icon:
 */
static GdkPixbuf *
cra_app_load_icon (const gchar *filename, GError **error)
{
	GdkPixbuf *pixbuf = NULL;
	_cleanup_object_unref_ GdkPixbuf *pixbuf_tmp;

	/* open file in native size */
	pixbuf_tmp = gdk_pixbuf_new_from_file (filename, error);
	if (pixbuf_tmp == NULL)
		return NULL;

	/* check size */
	if (gdk_pixbuf_get_width (pixbuf_tmp) < 32 ||
	    gdk_pixbuf_get_height (pixbuf_tmp) < 32) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "icon %s was too small %ix%i",
			     filename,
			     gdk_pixbuf_get_width (pixbuf_tmp),
			     gdk_pixbuf_get_height (pixbuf_tmp));
		return NULL;
	}

	/* re-open file at correct size */
	pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, 64, 64,
						    FALSE, error);
	if (pixbuf == NULL) {
		g_prefix_error (error, "Failed to open icon %s: ", filename);
		return NULL;
	}
	return pixbuf;
}

/**
 * cra_app_find_icon:
 */
static GdkPixbuf *
cra_app_find_icon (const gchar *tmpdir, const gchar *something, GError **error)
{
	guint i;
	guint j;
	const gchar *pixmap_dirs[] = { "pixmaps", "icons", NULL };
	const gchar *supported_ext[] = { ".png",
					 ".gif",
					 ".svg",
					 ".xpm",
					 "",
					 NULL };
	const gchar *sizes[] = { "64x64",
				 "128x128",
				 "96x96",
				 "256x256",
				 "scalable",
				 "48x48",
				 "32x32",
				 "24x24",
				 "16x16",
				 NULL };

	/* is this an absolute path */
	if (something[0] == '/') {
		_cleanup_free_ gchar *tmp;
		tmp = g_build_filename (tmpdir, something, NULL);
		if (!g_file_test (tmp, G_FILE_TEST_EXISTS)) {
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "specified icon '%s' does not exist",
				     something);
			return NULL;
		}
		return cra_app_load_icon (tmp, error);
	}

	/* hicolor apps */
	for (i = 0; sizes[i] != NULL; i++) {
		for (j = 0; supported_ext[j] != NULL; j++) {
			_cleanup_free_ gchar *tmp;
			tmp = g_strdup_printf ("%s/usr/share/icons/hicolor/%s/apps/%s%s",
					       tmpdir,
					       sizes[i],
					       something,
					       supported_ext[j]);
			if (g_file_test (tmp, G_FILE_TEST_EXISTS))
				return cra_app_load_icon (tmp, error);
		}
	}

	/* pixmap */
	for (i = 0; pixmap_dirs[i] != NULL; i++) {
		for (j = 0; supported_ext[j] != NULL; j++) {
			_cleanup_free_ gchar *tmp;
			tmp = g_strdup_printf ("%s/usr/share/%s/%s%s",
					       tmpdir,
					       pixmap_dirs[i],
					       something,
					       supported_ext[j]);
			if (g_file_test (tmp, G_FILE_TEST_EXISTS))
				return cra_app_load_icon (tmp, error);
		}
	}

	/* failed */
	g_set_error (error,
		     CRA_PLUGIN_ERROR,
		     CRA_PLUGIN_ERROR_FAILED,
		     "Failed to find icon %s", something);
	return NULL;
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
	const gchar *key;
	gboolean ret;
	_cleanup_free_ gchar *app_id = NULL;
	_cleanup_free_ gchar *full_filename = NULL;
	_cleanup_free_ gchar *icon_filename = NULL;
	_cleanup_object_unref_ CraApp *app = NULL;
	_cleanup_object_unref_ GdkPixbuf *pixbuf = NULL;

	/* create app */
	app_id = g_path_get_basename (filename);
	app = cra_app_new (pkg, app_id);
	full_filename = g_build_filename (tmpdir, filename, NULL);
	ret = as_app_parse_file (AS_APP (app),
				 full_filename,
				 AS_APP_PARSE_FLAG_USE_HEURISTICS,
				 error);
	if (!ret)
		return FALSE;

	/* NoDisplay requires AppData */
	if (as_app_get_metadata_item (AS_APP (app), "NoDisplay") != NULL)
		cra_app_add_requires_appdata (app, "NoDisplay=true");

	/* Settings or DesktopSettings requires AppData */
	if (as_app_has_category (AS_APP (app), "Settings"))
		cra_app_add_requires_appdata (app, "Category=Settings");
	if (as_app_has_category (AS_APP (app), "DesktopSettings"))
		cra_app_add_requires_appdata (app, "Category=DesktopSettings");

	/* is the icon a stock-icon-name? */
	key = as_app_get_icon (AS_APP (app));
	if (key != NULL) {
		if (as_app_get_icon_kind (AS_APP (app)) == AS_ICON_KIND_STOCK) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_DEBUG,
					 "using stock icon %s", key);
		} else {

			/* is icon XPM or GIF */
			if (g_str_has_suffix (key, ".xpm"))
				cra_app_add_veto (app, "Uses XPM icon: %s", key);
			else if (g_str_has_suffix (key, ".gif"))
				cra_app_add_veto (app, "Uses GIF icon: %s", key);
			else if (g_str_has_suffix (key, ".ico"))
				cra_app_add_veto (app, "Uses ICO icon: %s", key);

			/* find icon */
			pixbuf = cra_app_find_icon (tmpdir, key, error);
			if (pixbuf == NULL)
				return FALSE;

			/* save in target directory */
			icon_filename = g_strdup_printf ("%s.png",
							 as_app_get_id (AS_APP (app)));
			as_app_set_icon (AS_APP (app), icon_filename, -1);
			as_app_set_icon_kind (AS_APP (app), AS_ICON_KIND_CACHED);
			cra_app_set_pixbuf (app, pixbuf);
		}
	}

	/* add */
	cra_plugin_add_app (apps, app);
	return TRUE;
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
	GError *error_local = NULL;
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
						   &error_local);
		if (!ret) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "Failed to process %s: %s",
					 filelist[i],
					 error_local->message);
			g_clear_error (&error_local);
		}
	}

	/* no desktop files we care about */
	if (apps == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_basename (pkg));
		return NULL;
	}
	return apps;
}
