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
