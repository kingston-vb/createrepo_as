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

#include <appstream-glib.h>

#include <cra-plugin.h>
#include <cra-screenshot.h>

struct CraPluginPrivate {
	GPtrArray	*filenames;
	GMutex		 filenames_mutex;
};

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "appdata";
}

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
	plugin->priv->filenames = g_ptr_array_new_with_free_func (g_free);
	g_mutex_init (&plugin->priv->filenames_mutex);
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
	const gchar *tmp;
	guint i;

	/* print out AppData files not used */
	if (g_getenv ("CRA_PERFORM_EXTRA_CHECKS") != NULL) {
		for (i = 0; i < plugin->priv->filenames->len; i++) {
			tmp = g_ptr_array_index (plugin->priv->filenames, i);
			g_debug ("%s was not used", tmp);
		}
	}
	g_ptr_array_unref (plugin->priv->filenames);
	g_mutex_clear (&plugin->priv->filenames_mutex);
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
 * cra_plugin_appdata_add_path:
 */
static gboolean
cra_plugin_appdata_add_path (CraPlugin *plugin, const gchar *path, GError **error)
{
	GDir *dir = NULL;
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar *filename;

	/* scan all the files */
	dir = g_dir_open (path, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((tmp = g_dir_read_name (dir)) != NULL) {
		filename = g_build_filename (path, tmp, NULL);
		if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
			ret = cra_plugin_appdata_add_path (plugin, filename, error);
			g_free (filename);
			if (!ret)
				goto out;
		} else {
			g_ptr_array_add (plugin->priv->filenames, filename);
		}
	}
out:
	if (dir != NULL)
		g_dir_close (dir);
	return ret;
}

/**
 * cra_plugin_appdata_add_files:
 */
static gboolean
cra_plugin_appdata_add_files (CraPlugin *plugin, const gchar *path, GError **error)
{
	gboolean ret = TRUE;

	/* already done */
	if (plugin->priv->filenames->len > 0)
		goto out;

	g_mutex_lock (&plugin->priv->filenames_mutex);
	ret = cra_plugin_appdata_add_path (plugin, path, error);
	g_mutex_unlock  (&plugin->priv->filenames_mutex);
out:
	return ret;
}

/**
 * cra_plugin_appdata_remove_file:
 */
static void
cra_plugin_appdata_remove_file (CraPlugin *plugin, const gchar *filename)
{
	const gchar *tmp;
	guint i;

	g_mutex_lock (&plugin->priv->filenames_mutex);
	for (i = 0; i < plugin->priv->filenames->len; i++) {
		tmp = g_ptr_array_index (plugin->priv->filenames, i);
		if (g_strcmp0 (tmp, filename) == 0) {
			g_ptr_array_remove_fast (plugin->priv->filenames,
						 (gpointer) tmp);
			break;
		}
	}
	g_mutex_unlock  (&plugin->priv->filenames_mutex);
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
static void
cra_plugin_appdata_log_overwrite (CraApp *app,
				  const gchar *property_name,
				  const gchar *old,
				  const gchar *new)
{
	/* does the value already exist with this value */
	if (g_strcmp0 (old, new) == 0) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "AppData %s=%s already set",
				 property_name, old);
		return;
	}

	/* does the metadata exist with any value */
	if (old != NULL) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "AppData %s=%s->%s",
				 property_name, old, new);
	}
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraApp *app,
			     const gchar *filename,
			     GError **error)
{
	AsApp *appdata;
	AsProblemKind problem_kind;
	AsProblem *problem;
	GPtrArray *problems = NULL;
	const gchar *key;
	const gchar *old;
	const gchar *tmp;
	const GNode *c;
	const GNode *n;
	gboolean ret;
	gchar *data = NULL;
	gchar **split;
	GHashTable *comments = NULL;
	GHashTable *descriptions = NULL;
	GHashTable *names = NULL;
	GList *l;
	GList *list;
	GNode *root = NULL;
	guint i;

	/* validate */
	appdata = as_app_new ();
	ret = as_app_parse_file (appdata,
				 filename,
				 AS_APP_PARSE_FLAG_NONE,
				 error);
	if (!ret)
		goto out;
	problems = as_app_validate (appdata,
				    AS_APP_VALIDATE_FLAG_NO_NETWORK |
				    AS_APP_VALIDATE_FLAG_RELAX,
				    error);
	if (problems == NULL) {
		ret = FALSE;
		goto out;
	}
	for (i = 0; i < problems->len; i++) {
		problem = g_ptr_array_index (problems, i);
		problem_kind = as_problem_get_kind (problem);
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "AppData problem: %s : %s",
				 as_problem_kind_to_string (problem_kind),
				 as_problem_get_message (problem));
	}

	/* parse file */
	ret = g_file_get_contents (filename, &data, NULL, error);
	if (!ret)
		goto out;
	root = as_node_from_xml (data, -1, AS_NODE_FROM_XML_FLAG_NONE, error);
	if (root == NULL) {
		ret = FALSE;
		goto out;
	}

	/* check app id */
	n = as_node_find (root, "application/id");
	if (n == NULL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no ID",
			     filename);
		goto out;
	}
	if (g_strcmp0 (as_node_get_data (n),
		       as_app_get_id_full (AS_APP (app))) != 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s does not match '%s':'%s'",
			     filename,
			     as_node_get_data (n),
			     as_app_get_id_full (AS_APP (app)));
		goto out;
	}

	/* check license */
	n = as_node_find (root, "application/metadata_license");
	if (n == NULL)
		n = as_node_find (root, "application/licence");
	if (n == NULL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no licence",
			     filename);
		goto out;
	}
	tmp = as_node_get_data (n);
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
	n = as_node_find (root, "application/url");
	if (n != NULL) {
		as_app_add_url (AS_APP (app), AS_URL_KIND_HOMEPAGE,
				as_node_get_data (n), -1);
	}
	n = as_node_find (root, "application/project_group");
	if (n != NULL) {
		as_app_set_project_group (AS_APP (app),
					  as_node_get_data (n), -1);
	}
	n = as_node_find (root, "application/compulsory_for_desktop");
	if (n != NULL) {
		as_app_add_compulsory_for_desktop (AS_APP (app),
						   as_node_get_data (n),
						   -1);
	}

	/* perhaps get name & summary */
	n = as_node_find (root, "application");
	if (n != NULL)
		names = as_node_get_localized (n, "name");
	if (names != NULL) {
		list = g_hash_table_get_keys (names);
		for (l = list; l != NULL; l = l->next) {
			key = l->data;
			tmp = g_hash_table_lookup (names, key);
			if (g_strcmp0 (key, "C") == 0) {
				old = as_app_get_name (AS_APP (app), key);
				cra_plugin_appdata_log_overwrite (app, "name",
								  old, tmp);
			}
			as_app_set_name (AS_APP (app), key, tmp, -1);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'name' has no translations");
		}
		g_list_free (list);
	}
	if (n != NULL)
		comments = as_node_get_localized (n, "summary");
	if (comments != NULL) {
		list = g_hash_table_get_keys (comments);
		for (l = list; l != NULL; l = l->next) {
			key = l->data;
			tmp = g_hash_table_lookup (comments, key);
			if (g_strcmp0 (key, "C") == 0) {
				old = as_app_get_comment (AS_APP (app), key);
				cra_plugin_appdata_log_overwrite (app, "summary",
								  old, tmp);
			}
			as_app_set_comment (AS_APP (app), key, tmp, -1);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'summary' has no translations");
		}
		g_list_free (list);
	}

	/* get de-normalized description */
	n = as_node_find (root, "application/description");
	if (n != NULL) {
		descriptions = as_node_get_localized_unwrap (n, error);
		if (descriptions == NULL) {
			ret = FALSE;
			goto out;
		}
	}
	if (descriptions != NULL) {
		list = g_hash_table_get_keys (descriptions);
		for (l = list; l != NULL; l = l->next) {
			tmp = g_hash_table_lookup (descriptions, l->data);
			as_app_set_description (AS_APP (app),
						(const gchar *) l->data,
						tmp, -1);
		}
		if (g_list_length (list) == 1) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "AppData 'description' has no translations");
		}
		g_list_free (list);
	}

	/* add screenshots */
	n = as_node_find (root, "application/screenshots");
	if (n != NULL && as_app_get_screenshots(AS_APP (app))->len == 0) {
		for (c = n->children; c != NULL; c = c->next) {
			CraScreenshot *ss;
			GError *error_local = NULL;

			if (g_strcmp0 (as_node_get_name (c), "screenshot") != 0)
				continue;
			if (as_node_get_data (c) == NULL)
				continue;
			ss = cra_screenshot_new (cra_app_get_package (app),
						 as_app_get_id (AS_APP (app)));
			tmp = as_node_get_attribute (c, "type");
			as_screenshot_set_kind (AS_SCREENSHOT (ss),
						as_screenshot_kind_from_string (tmp));
			tmp = as_node_get_data (c);
			ret = cra_screenshot_load_url (ss, tmp, &error_local);
			if (ret) {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_DEBUG,
						 "Added screenshot %s", tmp);
				as_app_add_screenshot (AS_APP (app),
						       AS_SCREENSHOT (ss));
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
	n = as_node_find (root, "application/metadata");
	if (n != NULL) {
		for (c = n->children; c != NULL; c = c->next) {
			if (g_strcmp0 (as_node_get_name (c), "value") != 0)
				continue;
			tmp = as_node_get_attribute (c, "key");
			if (g_strcmp0 (tmp, "ExtraPackages") == 0) {
				split = g_strsplit (as_node_get_data (c), ",", -1);
				for (i = 0; split[i] != NULL; i++) {
					as_app_add_pkgname (AS_APP (app),
							    split[i], -1);
				}
				g_strfreev (split);
				continue;
			}
			old = as_app_get_metadata_item (AS_APP (app), tmp);
			cra_plugin_appdata_log_overwrite (app, "metadata",
							  old,
							  as_node_get_data (c));
			as_app_add_metadata (AS_APP (app), tmp,
					     as_node_get_data (c), -1);
		}
	}

	/* success */
	cra_app_set_requires_appdata (app, FALSE);
	ret = TRUE;
out:
	g_free (data);
	g_object_unref (appdata);
	if (problems != NULL)
		g_ptr_array_unref (problems);
	if (names != NULL)
		g_hash_table_unref (names);
	if (comments != NULL)
		g_hash_table_unref (comments);
	if (descriptions != NULL)
		g_hash_table_unref (descriptions);
	if (root != NULL)
		as_node_unref (root);
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
	const gchar *kind_str;
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar *appdata_filename;
	gchar *appdata_filename_extra = NULL;

	/* get possible sources */
	appdata_filename = g_strdup_printf ("%s/usr/share/appdata/%s.appdata.xml",
					    tmpdir, as_app_get_id (AS_APP (app)));
	tmp = cra_package_get_config (pkg, "AppDataExtra");
	if (tmp != NULL && g_file_test (tmp, G_FILE_TEST_EXISTS)) {
		ret = cra_plugin_appdata_add_files (plugin, tmp, error);
		if (!ret)
			goto out;
		kind_str = as_id_kind_to_string (as_app_get_id_kind (AS_APP (app)));
		appdata_filename_extra = g_strdup_printf ("%s/%s/%s.appdata.xml",
							  tmp,
							  kind_str,
							  as_app_get_id (AS_APP (app)));
		if (g_file_test (appdata_filename, G_FILE_TEST_EXISTS) &&
		    g_file_test (appdata_filename_extra, G_FILE_TEST_EXISTS)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "extra AppData file %s exists for no reason",
					 appdata_filename_extra);
		}

		/* we used this */
		cra_plugin_appdata_remove_file (plugin, appdata_filename_extra);
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
	if (as_app_get_id_kind (AS_APP (app)) == AS_ID_KIND_DESKTOP &&
	    as_app_get_metadata_item (AS_APP (app), "NoDisplay") == NULL) {
		cra_package_log (pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "desktop application %s has no AppData and "
				 "will not be shown in Fedora 22 and later",
				 as_app_get_id (AS_APP (app)));
	}
out:
	g_free (appdata_filename);
	g_free (appdata_filename_extra);
	return ret;
}
