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

struct CraPluginPrivate {
	GPtrArray	*project_groups;
};

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "hardcoded";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/help/*");
	cra_plugin_add_glob (globs, "/usr/share/gnome-shell/search-providers/*");
}

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
	plugin->priv->project_groups = cra_glob_value_array_new ();

	/* this is a heuristic */
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http*://*.gnome.org*", "GNOME"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://gnome-*.sourceforge.net/", "GNOME"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http*://*.kde.org*", "KDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://*kde-apps.org/*", "KDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://*xfce.org*", "XFCE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://lxde.org*", "LXDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://lxde.sourceforge.net/*", "LXDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://*mate-desktop.org*", "MATE"));
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
	g_ptr_array_unref (plugin->priv->project_groups);
}


/**
 * cra_plugin_hardcoded_add_screenshot:
 */
static gboolean
cra_plugin_hardcoded_add_screenshot (CraApp *app,
				     const gchar *filename,
				     GError **error)
{
	CraScreenshot *screenshot;
	gboolean ret;

	screenshot = cra_screenshot_new (cra_app_get_package (app),
					 cra_app_get_id (app));
	ret = cra_screenshot_load_filename (screenshot,
					    filename,
					    error);
	if (!ret)
		goto out;
	cra_app_add_screenshot (app, screenshot);
out:
	g_object_unref (screenshot);
	return ret;
}

/**
 * cra_plugin_hardcoded_add_screenshots:
 */
static gboolean
cra_plugin_hardcoded_add_screenshots (CraApp *app,
				      const gchar *location,
				      GError **error)
{
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar *filename;
	GDir *dir;

	/* scan for files */
	dir = g_dir_open (location, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((tmp = g_dir_read_name (dir)) != NULL) {
		if (!g_str_has_suffix (tmp, ".png"))
			continue;
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Adding extra screenshot: %s", tmp);
		filename = g_build_filename (location, tmp, NULL);
		ret = cra_plugin_hardcoded_add_screenshot (app, filename, error);
		g_free (filename);
		if (!ret)
			goto out;
	}
out:
	if (dir != NULL)
		g_dir_close (dir);
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
	const gchar *tmp;
	CraRelease *release;
	gboolean ret = TRUE;
	gchar **deps;
	gchar *dirname = NULL;
	gchar **filelist;
	GPtrArray *releases;
	guint i;
	guint secs;

	/* add extra categories */
	tmp = cra_app_get_id (app);
	if (g_strcmp0 (tmp, "0install") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (tmp, "alacarte") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (tmp, "deja-dup") == 0)
		cra_app_add_category (app, "Utility");
	if (g_strcmp0 (tmp, "gddccontrol") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (tmp, "nautilus") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (tmp, "pessulus") == 0)
		cra_app_add_category (app, "System");

	/* add extra project groups */
	if (g_strcmp0 (tmp, "nemo") == 0)
		cra_app_set_project_group (app, "Cinnamon");

	/* use the URL to guess the project group */
	tmp = cra_package_get_url (pkg);
	if (cra_app_get_project_group (app) == NULL && tmp != NULL) {
		tmp = cra_glob_value_search (plugin->priv->project_groups, tmp);
		if (tmp != NULL)
			cra_app_set_project_group (app, tmp);
	}

	/* look for any installed docs */
	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (g_str_has_prefix (filelist[i],
				      "/usr/share/help/")) {
			cra_app_add_metadata (app, "X-Kudo-InstallsUserDocs", "");
			break;
		}
	}

	/* look for a shell search provider */
	for (i = 0; filelist[i] != NULL; i++) {
		if (g_str_has_prefix (filelist[i],
				      "/usr/share/gnome-shell/search-providers/")) {
			cra_app_add_metadata (app, "X-Kudo-SearchProvider", "");
			break;
		}
	}

	/* look for a modern toolkit */
	deps = cra_package_get_deps (pkg);
	for (i = 0; deps[i] != NULL; i++) {
		if (g_strcmp0 (deps[i], "libgtk-3.so.0") == 0) {
			cra_app_add_metadata (app, "X-Kudo-GTK3", "");
			break;
		}
	}

	/* look for ancient toolkits */
	for (i = 0; deps[i] != NULL; i++) {
		if (g_strcmp0 (deps[i], "libgtk-1.2.so.0") == 0) {
			cra_app_add_veto (app, "Uses obsolete GTK1 toolkit");
			break;
		}
	}

	/* has the application been updated in the last year */
	releases = cra_app_get_releases (app);
	for (i = 0; i < releases->len; i++) {
		release = g_ptr_array_index (releases, i);
		secs = (g_get_real_time () / G_USEC_PER_SEC) -
			cra_release_get_timestamp (release);
		if (secs / (60 * 60 * 24) < 365) {
			cra_app_add_metadata (app, "X-Kudo-RecentRelease", "");
			break;
		}
	}

	/* has there been no upstream version in the last 5 years? */
	if (releases->len > 0) {
		release = g_ptr_array_index (releases, 0);
		secs = (g_get_real_time () / G_USEC_PER_SEC) -
			cra_release_get_timestamp (release);
		if (secs / (60 * 60 * 24) > 365 * 5) {
			cra_app_add_veto (app, "Dead upstream for %i years",
					  secs / (60 * 60 * 24 * 365));
		}
	}

	/* do any extra screenshots exist */
	tmp = cra_package_get_config (pkg, "ScreenshotsExtra");
	if (tmp != NULL) {
		dirname = g_build_filename (tmp, cra_app_get_id (app), NULL);
		if (g_file_test (dirname, G_FILE_TEST_EXISTS)) {
			ret = cra_plugin_hardcoded_add_screenshots (app,
								    dirname,
								    error);
			if (!ret)
				goto out;
		}
	}
out:
	g_free (dirname);
	return ret;
}
