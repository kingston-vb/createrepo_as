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
	guint i;

	/* add extra categories */
	if (g_strcmp0 (app->app_id, "0install") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (app->app_id, "alacarte") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (app->app_id, "deja-dup") == 0)
		cra_app_add_category (app, "Utility");
	if (g_strcmp0 (app->app_id, "gddccontrol") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (app->app_id, "nautilus") == 0)
		cra_app_add_category (app, "System");
	if (g_strcmp0 (app->app_id, "pessulus") == 0)
		cra_app_add_category (app, "System");

	/* add extra project groups */
	if (g_strcmp0 (app->app_id, "nemo") == 0)
		cra_app_set_project_group (app, "Cinnamon");

	/* use the URL to guess the project group */
	if (app->project_group == NULL && pkg->url != NULL) {
		tmp = cra_glob_value_search (plugin->priv->project_groups, pkg->url);
		if (tmp != NULL)
			cra_app_set_project_group (app, tmp);
	}

	/* look for any installed docs */
	for (i = 0; pkg->filelist[i] != NULL; i++) {
		if (g_str_has_prefix (pkg->filelist[i],
				      "/usr/share/help/")) {
			cra_app_add_metadata (app, "X-Kudo-InstallsUserDocs", "");
			break;
		}
	}

	/* look for a shell search provider */
	for (i = 0; pkg->filelist[i] != NULL; i++) {
		if (g_str_has_prefix (pkg->filelist[i],
				      "/usr/share/gnome-shell/search-providers/")) {
			cra_app_add_metadata (app, "X-Kudo-SearchProvider", "");
			break;
		}
	}

	return TRUE;
}
