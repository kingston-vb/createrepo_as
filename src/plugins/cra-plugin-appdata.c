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

#include <appstream-glib.h>
#include <libsoup/soup.h>

#include <cra-plugin.h>

struct CraPluginPrivate {
	SoupSession	*session;
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
	plugin->priv->session = soup_session_new_with_options (SOUP_SESSION_USER_AGENT, "createrepo_as",
							       SOUP_SESSION_TIMEOUT, 5000,
							       NULL);
	soup_session_add_feature_by_type (plugin->priv->session,
					  SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
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
	g_object_unref (plugin->priv->session);
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
	const gchar *tmp;
	_cleanup_dir_close_ GDir *dir = NULL;

	/* scan all the files */
	dir = g_dir_open (path, 0, error);
	if (dir == NULL)
		return FALSE;
	while ((tmp = g_dir_read_name (dir)) != NULL) {
		_cleanup_free_ gchar *filename;
		filename = g_build_filename (path, tmp, NULL);
		if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
			if (!cra_plugin_appdata_add_path (plugin, filename, error))
				return FALSE;
		} else {
			g_ptr_array_add (plugin->priv->filenames, g_strdup (filename));
		}
	}
	return TRUE;
}

/**
 * cra_plugin_appdata_add_files:
 */
static gboolean
cra_plugin_appdata_add_files (CraPlugin *plugin, const gchar *path, GError **error)
{
	gboolean ret;

	/* already done */
	if (plugin->priv->filenames->len > 0)
		return TRUE;

	g_mutex_lock (&plugin->priv->filenames_mutex);
	ret = cra_plugin_appdata_add_path (plugin, path, error);
	g_mutex_unlock  (&plugin->priv->filenames_mutex);
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
	if (g_strcmp0 (license, "CC0-1.0") == 0)
		return TRUE;
	if (g_strcmp0 (license, "CC-BY-3.0") == 0)
		return TRUE;
	if (g_strcmp0 (license, "CC-BY-SA-3.0") == 0)
		return TRUE;
	if (g_strcmp0 (license, "GFDL-1.3") == 0)
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
 * cra_plugin_appdata_load_url:
 **/
static gboolean
cra_plugin_appdata_load_url (CraPlugin *plugin,
			     CraApp *app,
			     const gchar *url,
			     GError **error)
{
	const gchar *cache_dir;
	gboolean ret = TRUE;
	SoupStatus status;
	SoupURI *uri = NULL;
	_cleanup_free_ gchar *basename;
	_cleanup_free_ gchar *cache_filename;
	_cleanup_object_unref_ SoupMessage *msg = NULL;

	/* download to cache if not already added */
	basename = g_path_get_basename (url);
	cache_dir = cra_package_get_config (cra_app_get_package (app), "CacheDir");
	cache_filename = g_strdup_printf ("%s/%s-%s",
					  cache_dir,
					  as_app_get_id (AS_APP (app)),
					  basename);
	if (!g_file_test (cache_filename, G_FILE_TEST_EXISTS)) {
		uri = soup_uri_new (url);
		if (uri == NULL) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Could not parse '%s' as a URL", url);
			goto out;
		}
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Downloading %s", url);
		msg = soup_message_new_from_uri (SOUP_METHOD_GET, uri);
		status = soup_session_send_message (plugin->priv->session, msg);
		if (status != SOUP_STATUS_OK) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Downloading failed: %s",
				     soup_status_get_phrase (status));
			goto out;
		}

		/* save new file */
		ret = g_file_set_contents (cache_filename,
					   msg->response_body->data,
					   msg->response_body->length,
					   error);
		if (!ret)
			goto out;
	}

	/* load the pixbuf */
	ret = cra_app_add_screenshot_source (app, cache_filename, error);
	if (!ret)
		goto out;
out:
	if (uri != NULL)
		soup_uri_free (uri);
	return ret;
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraPlugin *plugin,
			     CraApp *app,
			     const gchar *filename,
			     GError **error)
{
	AsProblemKind problem_kind;
	AsProblem *problem;
	const gchar *key;
	const gchar *old;
	const gchar *tmp;
	gboolean ret;
	GHashTable *hash;
	GPtrArray *array;
	GList *l;
	GList *list;
	guint i;
	_cleanup_object_unref_ AsApp *appdata;
	_cleanup_ptrarray_unref_ GPtrArray *problems = NULL;

	/* validate */
	appdata = as_app_new ();
	ret = as_app_parse_file (appdata, filename,
				 AS_APP_PARSE_FLAG_NONE,
				 error);
	if (!ret)
		return FALSE;
	problems = as_app_validate (appdata,
				    AS_APP_VALIDATE_FLAG_NO_NETWORK |
				    AS_APP_VALIDATE_FLAG_RELAX,
				    error);
	if (problems == NULL)
		return FALSE;
	for (i = 0; i < problems->len; i++) {
		problem = g_ptr_array_index (problems, i);
		problem_kind = as_problem_get_kind (problem);
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "AppData problem: %s : %s",
				 as_problem_kind_to_string (problem_kind),
				 as_problem_get_message (problem));
	}

	/* check app id */
	tmp = as_app_get_id_full (appdata);
	if (tmp == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no ID",
			     filename);
		return FALSE;
	}
	if (g_strcmp0 (tmp, as_app_get_id_full (AS_APP (app))) != 0) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s does not match '%s':'%s'",
			     filename,
			     tmp,
			     as_app_get_id_full (AS_APP (app)));
		return FALSE;
	}

	/* check license */
	tmp = as_app_get_metadata_license (appdata);
	if (tmp == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s has no licence",
			     filename);
		return FALSE;
	}
	if (!cra_plugin_appdata_license_valid (tmp)) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "AppData %s license invalid:'%s'",
			     filename, tmp);
		return FALSE;
	}

	/* other optional data */
	tmp = as_app_get_url_item (appdata, AS_URL_KIND_HOMEPAGE);
	if (tmp != NULL)
		as_app_add_url (AS_APP (app), AS_URL_KIND_HOMEPAGE, tmp, -1);
	tmp = as_app_get_project_group (appdata);
	if (tmp != NULL)
		as_app_set_project_group (AS_APP (app), tmp, -1);
	array = as_app_get_compulsory_for_desktops (appdata);
	if (array->len > 0) {
		tmp = g_ptr_array_index (array, 0);
		as_app_add_compulsory_for_desktop (AS_APP (app), tmp, -1);
	}

	/* perhaps get name */
	hash = as_app_get_names (appdata);
	list = g_hash_table_get_keys (hash);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		tmp = g_hash_table_lookup (hash, key);
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

	/* perhaps get summary */
	hash = as_app_get_comments (appdata);
	list = g_hash_table_get_keys (hash);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		tmp = g_hash_table_lookup (hash, key);
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

	/* get descriptions */
	hash = as_app_get_descriptions (appdata);
	list = g_hash_table_get_keys (hash);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		tmp = g_hash_table_lookup (hash, key);
		as_app_set_description (AS_APP (app), key, tmp, -1);
	}
	if (g_list_length (list) == 1) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "AppData 'description' has no translations");
	}
	g_list_free (list);

	/* add screenshots if not already added */
	array = as_app_get_screenshots (AS_APP (app));
	if (array->len == 0) {
		array = as_app_get_screenshots (appdata);
		for (i = 0; i < array->len; i++) {
			GError *error_local = NULL;
			AsScreenshot *ass;
			AsImage *image;

			ass = g_ptr_array_index (array, i);
			image = as_screenshot_get_source (ass);
			if (image == NULL)
				continue;

			/* load the URI or get from a cache */
			tmp = as_image_get_url (image);
			ret = cra_plugin_appdata_load_url (plugin,
							   app,
							   as_image_get_url (image),
							   &error_local);
			if (ret) {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_DEBUG,
						 "Added screenshot %s", tmp);
			} else {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 "Failed to load screenshot %s: %s",
						 tmp, error_local->message);
				g_clear_error (&error_local);
			}
		}
	} else {
		array = as_app_get_screenshots (appdata);
		if (array->len > 0) {
			cra_package_log (cra_app_get_package (app),
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "AppData screenshots ignored");
		}
	}

	/* add metadata */
	hash = as_app_get_metadata (appdata);
	list = g_hash_table_get_keys (hash);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		tmp = g_hash_table_lookup (hash, key);
		old = as_app_get_metadata_item (AS_APP (app), key);
		cra_plugin_appdata_log_overwrite (app, "metadata", old, tmp);
		as_app_add_metadata (AS_APP (app), key, tmp, -1);
	}

	/* log updateinfo */
	tmp = as_app_get_update_contact (AS_APP (appdata));
	if (tmp != NULL) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "Upstream contact <%s>", tmp);
	}

	/* success */
	cra_app_set_requires_appdata (app, FALSE);
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
	const gchar *kind_str;
	const gchar *tmp;
	_cleanup_free_ gchar *appdata_filename;
	_cleanup_free_ gchar *appdata_filename_extra = NULL;

	/* get possible sources */
	appdata_filename = g_strdup_printf ("%s/usr/share/appdata/%s.appdata.xml",
					    tmpdir, as_app_get_id (AS_APP (app)));
	tmp = cra_package_get_config (pkg, "AppDataExtra");
	if (tmp != NULL && g_file_test (tmp, G_FILE_TEST_EXISTS)) {
		if (!cra_plugin_appdata_add_files (plugin, tmp, error))
			return FALSE;
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
		return cra_plugin_process_filename (plugin,
						    app,
						    appdata_filename,
						    error);
	}

	/* any appdata-extra file */
	if (appdata_filename_extra != NULL &&
	    g_file_test (appdata_filename_extra, G_FILE_TEST_EXISTS)) {
		return cra_plugin_process_filename (plugin,
						    app,
						    appdata_filename_extra,
						    error);
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
	return TRUE;
}
