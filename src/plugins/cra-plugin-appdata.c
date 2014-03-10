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
#include <cra-dom.h>
#include <cra-screenshot.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "appdata";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/appdata/*.appdata.xml");
}

/**
 * cra_plugin_appdata_license_valid:
 */
static gboolean
cra_plugin_appdata_license_valid (const gchar *license)
{
	if (g_strcmp0 (license, "CC0") == 0)
		return TRUE;
	if (g_strcmp0 (license, "CC-BY") == 0)
		return TRUE;
	if (g_strcmp0 (license, "CC-BY-SA") == 0)
		return TRUE;
	if (g_strcmp0 (license, "GFDL") == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraApp *app,
			     const gchar *filename,
			     GError **error)
{
	const gchar *tmp;
	const GNode *c;
	const GNode *n;
	CraDom *dom;
	gboolean ret;
	gchar *data = NULL;
	gchar **split;
	GHashTable *comments = NULL;
	GHashTable *descriptions = NULL;
	GHashTable *names = NULL;
	GList *l;
	GList *list;
	guint i;

	/* parse file */
	dom = cra_dom_new ();
	ret = g_file_get_contents (filename, &data, NULL, error);
	if (!ret)
		goto out;
	ret = cra_dom_parse_xml_data (dom, data, -1, error);
	if (!ret)
		goto out;

	/* check app id */
	n = cra_dom_get_node (dom, NULL, "application/id");
	if (n == NULL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no ID",
			     filename);
		goto out;
	}
	if (g_strcmp0 (cra_dom_get_node_data (n),
		       cra_app_get_id_full (app)) != 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s does not match '%s':'%s'",
			     filename,
			     cra_dom_get_node_data (n),
			     cra_app_get_id_full (app));
		goto out;
	}

	/* check license */
	n = cra_dom_get_node (dom, NULL, "application/licence");
	if (n == NULL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no licence",
			     filename);
		goto out;
	}
	tmp = cra_dom_get_node_data (n);
	if (g_strcmp0 (tmp, "CC BY") == 0)
		tmp = "CC-BY";
	if (g_strcmp0 (tmp, "CC BY-SA") == 0)
		tmp = "CC-BY-SA";
	if (!cra_plugin_appdata_license_valid (tmp)) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s license invalid:'%s'",
			     filename, tmp);
		goto out;
	}

	/* other optional data */
	n = cra_dom_get_node (dom, NULL, "application/url");
	if (n != NULL)
		cra_app_set_homepage_url (app, cra_dom_get_node_data (n));
	n = cra_dom_get_node (dom, NULL, "application/project_group");
	if (n != NULL)
		cra_app_set_project_group (app, cra_dom_get_node_data (n));
	n = cra_dom_get_node (dom, NULL, "application/compulsory_for_desktop");
	if (n != NULL)
		cra_app_set_compulsory_for_desktop (app, cra_dom_get_node_data (n));

	/* perhaps get name & summary */
	n = cra_dom_get_node (dom, NULL, "application");
	if (n != NULL)
		names = cra_dom_get_node_localized (n, "name");
	if (names != NULL) {
		list = g_hash_table_get_keys (names);
		for (l = list; l != NULL; l = l->next) {
			tmp = g_hash_table_lookup (names, l->data);
			cra_app_set_name (app, (const gchar *) l->data, tmp);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'name' has no translations");
		}
		g_list_free (list);
	}
	if (n != NULL)
		comments = cra_dom_get_node_localized (n, "summary");
	if (comments != NULL) {
		list = g_hash_table_get_keys (comments);
		for (l = list; l != NULL; l = l->next) {
			tmp = g_hash_table_lookup (comments, l->data);
			cra_app_set_comment (app, (const gchar *) l->data, tmp);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'summary' has no translations");
		}
		g_list_free (list);
	}

	/* get de-normalized description */
	n = cra_dom_get_node (dom, NULL, "application/description");
	if (n != NULL) {
		descriptions = cd_dom_denorm_to_xml_localized (n, error);
		if (descriptions == NULL) {
			ret = FALSE;
			goto out;
		}
	}
	if (descriptions != NULL) {
		list = g_hash_table_get_keys (descriptions);
		for (l = list; l != NULL; l = l->next) {
			tmp = g_hash_table_lookup (descriptions, l->data);
			cra_app_set_description (app, (const gchar *) l->data, tmp);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'description' has no translations");
		}
		g_list_free (list);
	}

	/* add screenshots */
	n = cra_dom_get_node (dom, NULL, "application/screenshots");
	if (n != NULL && cra_app_get_screenshots(app)->len == 0) {
		for (c = n->children; c != NULL; c = c->next) {
			CraScreenshot *ss;
			GError *error_local = NULL;

			if (g_strcmp0 (cra_dom_get_node_name (c), "screenshot") != 0)
				continue;
			ss = cra_screenshot_new (cra_app_get_package (app),
						 cra_app_get_id (app));
			tmp = cra_dom_get_node_attribute (c, "type");
			cra_screenshot_set_is_default (ss, g_strcmp0 (tmp, "default") == 0);
			tmp = cra_dom_get_node_data (c);
			ret = cra_screenshot_load_url (ss, tmp, &error_local);
			if (ret) {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_DEBUG,
						 "Added screenshot %s", tmp);
				cra_app_add_screenshot (app, ss);
			} else {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 "Failed to load screenshot %s: %s",
						 tmp, error_local->message);
				g_clear_error (&error_local);
			}
			g_object_unref (ss);
		}
	} else if (n != NULL) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "AppData screenshots ignored");
	}

	/* add metadata */
	n = cra_dom_get_node (dom, NULL, "application/metadata");
	if (n != NULL) {
		for (c = n->children; c != NULL; c = c->next) {
			if (g_strcmp0 (cra_dom_get_node_name (c), "value") != 0)
				continue;
			tmp = cra_dom_get_node_attribute (c, "key");
			if (g_strcmp0 (tmp, "ExtraPackages") == 0) {
				split = g_strsplit (cra_dom_get_node_data (c), ",", -1);
				for (i = 0; split[i] != NULL; i++)
					cra_app_add_pkgname (app, split[i]);
				g_strfreev (split);
			} else {
				cra_app_add_metadata (app, tmp,
						      cra_dom_get_node_data (c));
			}
		}
	}

	/* success */
	cra_app_set_requires_appdata (app, FALSE);
	ret = TRUE;
out:
	g_free (data);
	if (names != NULL)
		g_hash_table_unref (names);
	if (comments != NULL)
		g_hash_table_unref (comments);
	if (descriptions != NULL)
		g_hash_table_unref (descriptions);
	if (dom != NULL)
		g_object_unref (dom);
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
	gboolean ret = TRUE;
	gchar *appdata_filename;
	gchar *appdata_filename_extra = NULL;

	/* get possible sources */
	appdata_filename = g_strdup_printf ("%s/usr/share/appdata/%s.appdata.xml",
					    tmpdir, cra_app_get_id (app));
	tmp = cra_package_get_config (pkg, "AppDataExtra");
	if (tmp != NULL) {
		appdata_filename_extra = g_strdup_printf ("%s/%s/%s.appdata.xml",
							  tmp,
							  cra_app_get_type_id (app),
							  cra_app_get_id (app));
		if (g_file_test (appdata_filename, G_FILE_TEST_EXISTS) &&
		    g_file_test (appdata_filename_extra, G_FILE_TEST_EXISTS)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "extra AppData file %s exists for no reason",
					 appdata_filename_extra);
		}
	}

	/* any installed appdata file */
	if (g_file_test (appdata_filename, G_FILE_TEST_EXISTS)) {
		ret = cra_plugin_process_filename (app,
						   appdata_filename,
						   error);
		goto out;
	}

	/* any appdata-extra file */
	if (appdata_filename_extra != NULL &&
	    g_file_test (appdata_filename_extra, G_FILE_TEST_EXISTS)) {
		ret = cra_plugin_process_filename (app,
						   appdata_filename_extra,
						   error);
		goto out;
	}

	/* we're going to require this for F22 */
	if (g_strcmp0 (cra_app_get_type_id (app), "desktop") == 0 &&
	    cra_app_get_metadata_item (app, "NoDisplay") == NULL) {
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "desktop application %s has no AppData and "
				 "will not be shown in Fedora 22 and later",
				 cra_app_get_id (app));
	}
out:
	g_free (appdata_filename);
	g_free (appdata_filename_extra);
	return ret;
}
