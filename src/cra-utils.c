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

#include "config.h"

#include <glib/gstdio.h>
#include <fnmatch.h>
#include <archive.h>
#include <archive_entry.h>
#include <string.h>

#include "cra-utils.h"
#include "cra-plugin.h"

/**
 * cra_utils_rmtree:
 **/
gboolean
cra_utils_rmtree (const gchar *directory, GError **error)
{
	gint rc;
	gboolean ret;

	ret = cra_utils_ensure_exists_and_empty (directory, error);
	if (!ret)
		goto out;
	rc = g_remove (directory);
	if (rc != 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to delete: %s", directory);
		goto out;
	}
out:
	return ret;
}

/**
 * cra_utils_ensure_exists_and_empty:
 **/
gboolean
cra_utils_ensure_exists_and_empty (const gchar *directory, GError **error)
{
	const gchar *filename;
	gboolean ret = TRUE;
	gchar *src;
	GDir *dir = NULL;
	gint rc;

	/* does directory exist */
	if (!g_file_test (directory, G_FILE_TEST_EXISTS)) {
		rc = g_mkdir_with_parents (directory, 0700);
		if (rc != 0) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Failed to delete: %s", directory);
			goto out;
		}
		goto out;
	}

	/* try to open */
	dir = g_dir_open (directory, 0, error);
	if (dir == NULL)
		goto out;

	/* find each */
	while ((filename = g_dir_read_name (dir))) {
		src = g_build_filename (directory, filename, NULL);
		ret = g_file_test (src, G_FILE_TEST_IS_DIR);
		if (ret) {
			ret = cra_utils_rmtree (src, error);
			if (!ret)
				goto out;
		} else {
			rc = g_unlink (src);
			if (rc != 0) {
				ret = FALSE;
				g_set_error (error,
					     CRA_PLUGIN_ERROR,
					     CRA_PLUGIN_ERROR_FAILED,
					     "Failed to delete: %s", src);
				goto out;
			}
		}
		g_free (src);
	}

	/* success */
	ret = TRUE;
out:
	if (dir != NULL)
		g_dir_close (dir);
	return ret;
}

/**
 * cra_utils_write_archive:
 */
static gboolean
cra_utils_write_archive (const gchar *filename,
			 GPtrArray *files,
			 GError **error)
{
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar *basename;
	gchar *data;
	gsize len;
	guint i;
	struct archive *a;
	struct archive_entry *entry;
	struct stat st;

	a = archive_write_new ();
	archive_write_add_filter_gzip (a);
	archive_write_set_format_pax_restricted (a);
	archive_write_open_filename (a, filename);
	for (i = 0; i < files->len; i++) {
		tmp = g_ptr_array_index (files, i);
		stat (tmp, &st);
		entry = archive_entry_new ();
		basename = g_path_get_basename (tmp);
		archive_entry_set_pathname (entry, basename);
		g_free (basename);
		archive_entry_set_size (entry, st.st_size);
		archive_entry_set_filetype (entry, AE_IFREG);
		archive_entry_set_perm (entry, 0644);
		archive_write_header (a, entry);
		ret = g_file_get_contents (tmp, &data, &len, error);
		if (!ret)
			goto out;
		archive_write_data (a, data, len);
		g_free (data);
		archive_entry_free (entry);
	}
out:
	archive_write_close (a);
	archive_write_free (a);
	return ret;
}

/**
 * cra_utils_write_archive_dir:
 */
gboolean
cra_utils_write_archive_dir (const gchar *filename,
			     const gchar *directory,
			     GError **error)
{
	GPtrArray *files = NULL;
	GDir *dir;
	const gchar *tmp;
	gboolean ret = TRUE;

	/* add all files in the directory to the archive */
	dir = g_dir_open (directory, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	files = g_ptr_array_new_with_free_func (g_free);
	while ((tmp = g_dir_read_name (dir)) != NULL)
		g_ptr_array_add (files, g_build_filename (directory, tmp, NULL));

	/* write tar file */
	ret = cra_utils_write_archive (filename, files, error);
	if (!ret)
		goto out;
out:
	if (dir != NULL)
		g_dir_close (dir);
	if (files != NULL)
		g_ptr_array_unref (files);
	return ret;
}

/**
 * cra_utils_add_apps_from_file:
 */
gboolean
cra_utils_add_apps_from_file (GList **apps, const gchar *filename, GError **error)
{
	AsApp *app;
	AsStore *store;
	GFile *file;
	GPtrArray *array;
	gboolean ret;
	guint i;

	/* parse file */
	store = as_store_new ();
	file = g_file_new_for_path (filename);
	ret = as_store_from_file (store, file, NULL, NULL, error);
	if (!ret)
		goto out;

	/* copy Asapp's into CraApp's */
	array = as_store_get_apps (store);
	for (i = 0; i < array->len; i++) {
		app = g_ptr_array_index (array, i);
		cra_plugin_add_app (apps, (CraApp *) app);
	}
out:
	g_object_unref (file);
	g_object_unref (store);
	return ret;
}

/**
 * cra_utils_add_apps_from_dir:
 */
gboolean
cra_utils_add_apps_from_dir (GList **apps, const gchar *path, GError **error)
{
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar *filename;
	GDir *dir;

	dir = g_dir_open (path, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((tmp = g_dir_read_name (dir)) != NULL) {
		filename = g_build_filename (path, tmp, NULL);
		ret = cra_utils_add_apps_from_file (apps, filename, error);
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
 * cra_string_replace:
 */
guint
cra_string_replace (GString *string, const gchar *search, const gchar *replace)
{
	gchar **split = NULL;
	gchar *tmp = NULL;
	guint count = 0;

	/* quick search */
	if (g_strstr_len (string->str, -1, search) == NULL)
		goto out;

	/* replace */
	split = g_strsplit (string->str, search, -1);
	tmp = g_strjoinv (replace, split);
	g_string_assign (string, tmp);
	count = g_strv_length (split);
out:
	g_strfreev (split);
	g_free (tmp);
	return count;
}

/******************************************************************************/

struct CraGlobValue {
	gchar		*glob;
	gchar		*value;
};

/**
 * cra_glob_value_free:
 */
void
cra_glob_value_free (CraGlobValue *kv)
{
	g_free (kv->glob);
	g_free (kv->value);
	g_slice_free (CraGlobValue, kv);
}

/**
 * cra_glob_value_array_new:
 */
GPtrArray *
cra_glob_value_array_new (void)
{
	return g_ptr_array_new_with_free_func ((GDestroyNotify) cra_glob_value_free);
}

/**
 * cra_glob_value_new:
 */
CraGlobValue *
cra_glob_value_new (const gchar *glob, const gchar *value)
{
	CraGlobValue *kv;
	kv = g_slice_new0 (CraGlobValue);
	kv->glob = g_strdup (glob);
	kv->value = g_strdup (value);
	return kv;
}

/**
 * cra_glob_value_search:
 * @array: of CraGlobValue, keys may contain globs
 * @needle: may not be a glob
 */
const gchar *
cra_glob_value_search (GPtrArray *array, const gchar *search)
{
	const CraGlobValue *tmp;
	guint i;

	for (i = 0; i < array->len; i++) {
		tmp = g_ptr_array_index (array, i);
		if (fnmatch (tmp->glob, search, 0) == 0)
			return tmp->value;
	}
	return NULL;
}
