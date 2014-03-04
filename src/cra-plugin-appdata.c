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

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "appdata";
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
	GHashTable *comments = NULL;
	GHashTable *names = NULL;
	GList *l;
	GList *list;

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
		g_list_free (list);
	}

	/* FIXME: description */

	/* add screenshots */
	n = cra_dom_get_node (dom, NULL, "application/screenshots");
	if (n != NULL) {
		for (c = n->children; c != NULL; c = c->next) {
			if (g_strcmp0 (cra_dom_get_node_name (c), "screenshot") != 0)
				continue;
			//TODO
			g_warning ("Add screenshot %s [%s]",
				   cra_dom_get_node_data (c),
				   cra_dom_get_node_attribute (c, "type"));
		}
	}

	/* add metadata */
	n = cra_dom_get_node (dom, NULL, "application/metadata");
	if (n != NULL) {
		for (c = n->children; c != NULL; c = c->next) {
			if (g_strcmp0 (cra_dom_get_node_name (c), "value") != 0)
				continue;
			tmp = cra_dom_get_node_attribute (c, "key");
			if (g_strcmp0 (tmp, "ExtraPackages") == 0) {
				cra_app_add_pkgname (app,
						     cra_dom_get_node_data (c));
			} else {
				cra_app_add_metadata (app, tmp,
						      cra_dom_get_node_data (c));
			}
		}
	}
out:
	g_free (data);
	if (names != NULL)
		g_hash_table_unref (names);
	if (comments != NULL)
		g_hash_table_unref (comments);
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
	gboolean ret = TRUE;
	gchar *appdata_filename;
	gchar *appdata_filename_extra = NULL;

	/* any installed appdata file */
	appdata_filename = g_strdup_printf ("%s/usr/share/appdata/%s.appdata.xml",
					    tmpdir, cra_app_get_id (app));
	if (g_file_test (appdata_filename, G_FILE_TEST_EXISTS)) {
		ret = cra_plugin_process_filename (app,
						   appdata_filename,
						   error);
		goto out;
	}

	/* any appdata-extra file */
	appdata_filename_extra = g_strdup_printf ("../../fedora-appstream/appdata-extra/%s/%s.appdata.xml",
						  cra_app_get_type_id (app),
						  cra_app_get_id (app));
	if (g_file_test (appdata_filename_extra, G_FILE_TEST_EXISTS)) {
		ret = cra_plugin_process_filename (app,
						   appdata_filename_extra,
						   error);
		goto out;
	}
out:
	g_free (appdata_filename);
	g_free (appdata_filename_extra);
	return ret;
}
