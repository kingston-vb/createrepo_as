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
	return "desktop";
}

struct CraPluginPrivate {
	GPtrArray	*blacklist_cats;
};

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
	plugin->priv->blacklist_cats = cra_glob_value_array_new ();

	/* this is a heuristic */
	g_ptr_array_add (plugin->priv->blacklist_cats,
			 cra_glob_value_new ("X-*-Settings-Panel", ""));
	g_ptr_array_add (plugin->priv->blacklist_cats,
			 cra_glob_value_new ("X-*-SettingsDialog", ""));
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
	g_ptr_array_unref (plugin->priv->blacklist_cats);
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
 * cra_plugin_desktop_key_get_locale:
 */
static gchar *
cra_plugin_desktop_key_get_locale (const gchar *key)
{
	gchar *tmp1;
	gchar *tmp2;
	gchar *locale = NULL;

	tmp1 = g_strstr_len (key, -1, "[");
	if (tmp1 == NULL)
		goto out;
	tmp2 = g_strstr_len (tmp1, -1, "]");
	if (tmp1 == NULL)
		goto out;
	locale = g_strdup (tmp1 + 1);
	locale[tmp2 - tmp1 - 1] = '\0';
out:
	return locale;
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
	CraApp *app;
	gboolean ret;
	gchar *app_id = NULL;
	gchar *full_filename = NULL;
	gchar **keys = NULL;
	gchar *locale = NULL;
	gchar *tmp;
	gchar **tmpv;
	GKeyFile *kf;
	GPtrArray *categories;
	guint i;
	guint j;

	/* load file */
	full_filename = g_build_filename (tmpdir, filename, NULL);
	kf = g_key_file_new ();
	ret = g_key_file_load_from_file (kf, full_filename,
					 G_KEY_FILE_KEEP_TRANSLATIONS,
					 error);
	if (!ret)
		goto out;

	/* create app */
	app_id = g_path_get_basename (filename);
	app = cra_app_new (pkg, app_id);
	cra_app_set_type_id (app, "desktop");

	/* look at all the keys */
	keys = g_key_file_get_keys (kf, G_KEY_FILE_DESKTOP_GROUP, NULL, error);
	if (keys == NULL) {
		ret = FALSE;
		goto out;
	}
	for (i = 0; keys[i] != NULL; i++) {
		key = keys[i];

		/* NoDisplay */
		if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY) == 0) {
			cra_app_set_requires_appdata (app, TRUE);

		/* Type */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_TYPE) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_strcmp0 (tmp, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) != 0) {
				g_set_error (error,
					     CRA_PLUGIN_ERROR,
					     CRA_PLUGIN_ERROR_FAILED,
					     "not an applicaiton %s",
					     filename);
				ret = FALSE;
				goto out;
			}
			g_free (tmp);

		/* Icon */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_ICON) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			cra_app_set_icon (app, tmp);
			g_free (tmp);

		/* Categories */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_CATEGORIES) == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++)
				cra_app_add_category (app, tmpv[j]);
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "Keywords") == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++)
				cra_app_add_keyword (app, tmpv[j]);
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "MimeType") == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++)
				cra_app_add_mimetype (app, tmpv[j]);
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "X-GNOME-UsesNotifications") == 0) {
			cra_app_add_metadata (app, "X-Kudo-UsesNotifications", "");

		} else if (g_strcmp0 (key, "X-GNOME-Bugzilla-Product") == 0) {
			cra_app_set_project_group (app, "GNOME");

		} else if (g_strcmp0 (key, "X-MATE-Bugzilla-Product") == 0) {
			cra_app_set_project_group (app, "MATE");

		} else if (g_strcmp0 (key, "X-KDE-StartupNotify") == 0) {
			cra_app_set_project_group (app, "KDE");

		} else if (g_strcmp0 (key, "X-DocPath") == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_str_has_prefix (tmp, "http://userbase.kde.org/"))
				cra_app_set_project_group (app, "KDE");
			g_free (tmp);

		/* Exec */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_EXEC) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_str_has_prefix (tmp, "xfce4-"))
				cra_app_set_project_group (app, "XFCE");
			g_free (tmp);

		/* OnlyShowIn */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN) == 0) {
			/* if an app has only one entry, it's that desktop */
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			if (g_strv_length (tmpv) == 1)
				cra_app_set_project_group (app, tmpv[0]);
			g_strfreev (tmpv);

		/* Name */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_NAME) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			cra_app_set_name (app, "C", tmp);
			g_free (tmp);

		/* Name[] */
		} else if (g_str_has_prefix (key, G_KEY_FILE_DESKTOP_KEY_NAME)) {
			locale = cra_plugin_desktop_key_get_locale (key);
			tmp = g_key_file_get_locale_string (kf,
							    G_KEY_FILE_DESKTOP_GROUP,
							    key,
							    locale,
							    NULL);
			cra_app_set_name (app, locale, tmp);
			g_free (locale);
			g_free (tmp);

		/* Comment */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_COMMENT) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			cra_app_set_comment (app, "C", tmp);
			g_free (tmp);

		/* Comment[] */
		} else if (g_str_has_prefix (key, G_KEY_FILE_DESKTOP_KEY_COMMENT)) {
			locale = cra_plugin_desktop_key_get_locale (key);
			tmp = g_key_file_get_locale_string (kf,
							    G_KEY_FILE_DESKTOP_GROUP,
							    key,
							    locale,
							    NULL);
			cra_app_set_comment (app, locale, tmp);
			g_free (locale);
			g_free (tmp);

		}
	}

	/* check for blacklisted categories */
	categories = cra_app_get_categories (app);
	for (i = 0; i < categories->len; i++) {
		key = g_ptr_array_index (categories, i);
		if (cra_glob_value_search (plugin->priv->blacklist_cats,
					   key) != NULL) {
			g_object_unref (app);
			app = NULL;
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "category %s is blacklisted", key);
			goto out;
		}
	}

	/* add */
	cra_plugin_add_app (apps, app);
out:
	g_strfreev (keys);
	g_key_file_unref (kf);
	g_free (full_filename);
	g_free (app_id);
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
