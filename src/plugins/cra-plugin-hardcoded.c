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
			 cra_glob_value_new ("http://pcmanfm.sourceforge.net/*", "LXDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://lxde.sourceforge.net/*", "LXDE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://*mate-desktop.org*", "MATE"));
	g_ptr_array_add (plugin->priv->project_groups,
			 cra_glob_value_new ("http://*enlightenment.org*", "Enlightenment"));
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
 * cra_plugin_hardcoded_sort_screenshots_cb:
 */
static gint
cra_plugin_hardcoded_sort_screenshots_cb (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 ((gchar *) a, (gchar *) b);
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
	GList *l;
	GList *list = NULL;
	_cleanup_dir_close_ GDir *dir;

	/* scan for files */
	dir = g_dir_open (location, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((tmp = g_dir_read_name (dir)) != NULL) {
		if (!g_str_has_suffix (tmp, ".png"))
			continue;
		list = g_list_prepend (list, g_build_filename (location, tmp, NULL));
	}
	list = g_list_sort (list, cra_plugin_hardcoded_sort_screenshots_cb);
	for (l = list; l != NULL; l = l->next) {
		tmp = l->data;
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Adding extra screenshot: %s", tmp);
		ret = cra_app_add_screenshot_source (app, tmp, error);
		if (!ret)
			goto out;
	}
out:
	g_list_free_full (list, g_free);
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
	AsRelease *release;
	gchar **deps;
	gchar **filelist;
	GPtrArray *releases;
	guint i;
	guint secs;
	guint days;

	/* add extra categories */
	tmp = as_app_get_id (AS_APP (app));
	if (g_strcmp0 (tmp, "0install") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "alacarte") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "deja-dup") == 0)
		as_app_add_category (AS_APP (app), "Utility", -1);
	if (g_strcmp0 (tmp, "gddccontrol") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "nautilus") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "pessulus") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "pmdefaults") == 0)
		as_app_add_category (AS_APP (app), "System", -1);
	if (g_strcmp0 (tmp, "fwfstab") == 0)
		as_app_add_category (AS_APP (app), "System", -1);

	/* add extra project groups */
	if (g_strcmp0 (tmp, "nemo") == 0)
		as_app_set_project_group (AS_APP (app), "Cinnamon", -1);
	if (g_strcmp0 (tmp, "xfdashboard") == 0)
		as_app_set_project_group (AS_APP (app), "XFCE", -1);

	/* use the URL to guess the project group */
	tmp = cra_package_get_url (pkg);
	if (as_app_get_project_group (AS_APP (app)) == NULL && tmp != NULL) {
		tmp = cra_glob_value_search (plugin->priv->project_groups, tmp);
		if (tmp != NULL)
			as_app_set_project_group (AS_APP (app), tmp, -1);
	}

	/* use summary to guess the project group */
	tmp = as_app_get_comment (AS_APP (app), NULL);
	if (tmp != NULL && g_strstr_len (tmp, -1, "for KDE") != NULL)
		as_app_set_project_group (AS_APP (app), "KDE", -1);

	/* look for any installed docs */
	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (g_str_has_prefix (filelist[i],
				      "/usr/share/help/")) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-InstallsUserDocs", "", -1);
			break;
		}
	}

	/* look for a shell search provider */
	for (i = 0; filelist[i] != NULL; i++) {
		if (g_str_has_prefix (filelist[i],
				      "/usr/share/gnome-shell/search-providers/")) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-SearchProvider", "", -1);
			break;
		}
	}

	/* look for a modern toolkit */
	deps = cra_package_get_deps (pkg);
	for (i = 0; deps != NULL && deps[i] != NULL; i++) {
		if (g_strcmp0 (deps[i], "libgtk-3.so.0") == 0) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-GTK3", "", -1);
			break;
		}
		if (g_strcmp0 (deps[i], "libQt5Core.so.5") == 0) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-QT5", "", -1);
			break;
		}
	}

	/* look for ancient toolkits */
	for (i = 0; deps != NULL && deps[i] != NULL; i++) {
		if (g_strcmp0 (deps[i], "libgtk-1.2.so.0") == 0) {
			cra_app_add_veto (app, "Uses obsolete GTK1 toolkit");
			break;
		}
		if (g_strcmp0 (deps[i], "libqt-mt.so.3") == 0) {
			cra_app_add_veto (app, "Uses obsolete QT3 toolkit");
			break;
		}
		if (g_strcmp0 (deps[i], "liblcms.so.1") == 0) {
			cra_app_add_veto (app, "Uses obsolete LCMS library");
			break;
		}
		if (g_strcmp0 (deps[i], "libelektra.so.4") == 0) {
			cra_app_add_veto (app, "Uses obsolete Elektra library");
			break;
		}
		if (g_strcmp0 (deps[i], "libXt.so.6") == 0) {
			cra_app_add_requires_appdata (app, "Uses obsolete X11 toolkit");
			break;
		}
		if (g_strcmp0 (deps[i], "wine-core") == 0) {
			cra_app_add_requires_appdata (app, "Uses wine");
			break;
		}
	}

	/* has the application been updated in the last year */
	releases = as_app_get_releases (AS_APP (app));
	for (i = 0; i < releases->len; i++) {
		release = g_ptr_array_index (releases, i);
		secs = (g_get_real_time () / G_USEC_PER_SEC) -
			as_release_get_timestamp (release);
		days = secs / (60 * 60 * 24);
		if (days < 365) {
			as_app_add_metadata (AS_APP (app),
					     "X-Kudo-RecentRelease", "", -1);
			break;
		}
	}

	/* has there been no upstream version recently */
	if (releases->len > 0 &&
	    as_app_get_id_kind (AS_APP (app)) == AS_ID_KIND_DESKTOP) {
		release = g_ptr_array_index (releases, 0);
		secs = (g_get_real_time () / G_USEC_PER_SEC) -
			as_release_get_timestamp (release);
		days = secs / (60 * 60 * 24);

		/* this is just too old for us to care about */
		if (days > 365 * 10) {
			cra_app_add_veto (app, "Dead upstream for %i years",
					  secs / (60 * 60 * 24 * 365));
		}

		/* we need AppData if the app needs saving */
		else if (days > 365 * 5) {
			cra_app_add_requires_appdata (app,
				"Dead upstream for > %i years", 5);
		}
	}

	/* do any extra screenshots exist */
	tmp = cra_package_get_config (pkg, "ScreenshotsExtra");
	if (tmp != NULL) {
		_cleanup_free_ gchar *dirname = NULL;
		dirname = g_build_filename (tmp, as_app_get_id (AS_APP (app)), NULL);
		if (g_file_test (dirname, G_FILE_TEST_EXISTS)) {
			if (!cra_plugin_hardcoded_add_screenshots (app, dirname, error))
				return FALSE;
		}
	}

	/* a ConsoleOnly category means we require AppData */
	if (as_app_has_category (AS_APP(app), "ConsoleOnly"))
		cra_app_add_requires_appdata (app, "ConsoleOnly");

	/* no categories means we require AppData */
	if (as_app_get_categories(AS_APP(app))->len == 0)
		cra_app_add_requires_appdata (app, "no Categories");

	return TRUE;
}
