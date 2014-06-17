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
#include <fnmatch.h>

#include <cra-plugin.h>

typedef struct {
	guint32		 magic;
	guint32		 revision;
	guint32		 nstrings;
	guint32		 orig_tab_offset;
	guint32		 trans_tab_offset;
	guint32		 hash_tab_size;
	guint32		 hash_tab_offset;
	guint32		 n_sysdep_segments;
	guint32		 sysdep_segments_offset;
	guint32		 n_sysdep_strings;
	guint32		 orig_sysdep_tab_offset;
	guint32		 trans_sysdep_tab_offset;
} CraGettextHeader;

typedef struct {
	gchar		*locale;
	guint		 nstrings;
	guint		 percentage;
} CraGettextEntry;

typedef struct {
	guint		 max_nstrings;
	GList		*data;
} CraGettextContext;

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "gettext";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/locale/*/LC_MESSAGES/*.mo");
}

/**
 * cra_gettext_entry_new:
 **/
static CraGettextEntry *
cra_gettext_entry_new (void)
{
	CraGettextEntry *entry;
	entry = g_slice_new0 (CraGettextEntry);
	return entry;
}

/**
 * cra_gettext_entry_free:
 **/
static void
cra_gettext_entry_free (CraGettextEntry *entry)
{
	g_free (entry->locale);
	g_slice_free (CraGettextEntry, entry);
}

/**
 * cra_gettext_ctx_new:
 **/
static CraGettextContext *
cra_gettext_ctx_new (void)
{
	CraGettextContext *ctx;
	ctx = g_new0 (CraGettextContext, 1);
	return ctx;
}

/**
 * cra_gettext_ctx_free:
 **/
static void
cra_gettext_ctx_free (CraGettextContext *ctx)
{
	GList *l;
	for (l = ctx->data; l != NULL; l = l->next)
		cra_gettext_entry_free (l->data);
	g_free (ctx);
}

/**
 * cra_gettext_parse_file:
 **/
static gboolean
cra_gettext_parse_file (CraGettextContext *ctx,
			const gchar *locale,
			const gchar *filename,
			GError **error)
{
	CraGettextEntry *entry;
	CraGettextHeader *h;
	_cleanup_free_ gchar *data = NULL;

	/* read data, although we only strictly need the header */
	if (!g_file_get_contents (filename, &data, NULL, error))
		return FALSE;

	h = (CraGettextHeader *) data;
	entry = cra_gettext_entry_new ();
	entry->locale = g_strdup (locale);
	entry->nstrings = h->nstrings;
	if (entry->nstrings > ctx->max_nstrings)
		ctx->max_nstrings = entry->nstrings;
	ctx->data = g_list_prepend (ctx->data, entry);
	return TRUE;
}

/**
 * cra_gettext_ctx_search_locale:
 **/
static gboolean
cra_gettext_ctx_search_locale (CraGettextContext *ctx,
			       const gchar *locale,
			       const gchar *messages_path,
			       GError **error)
{
	const gchar *filename;
	_cleanup_dir_close_ GDir *dir;

	dir = g_dir_open (messages_path, 0, error);
	if (dir == NULL)
		return FALSE;
	while ((filename = g_dir_read_name (dir)) != NULL) {
		_cleanup_free_ gchar *path;
		path = g_build_filename (messages_path, filename, NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			if (!cra_gettext_parse_file (ctx, locale, path, error))
				return FALSE;
		}
	}
	return TRUE;
}

static gint
cra_gettext_entry_sort_cb (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 (((CraGettextEntry *) a)->locale, ((CraGettextEntry *) b)->locale);
}

/**
 * cra_gettext_ctx_search_path:
 **/
static gboolean
cra_gettext_ctx_search_path (CraGettextContext *ctx,
			     const gchar *prefix,
			     GError **error)
{
	const gchar *filename;
	CraGettextEntry *e;
	GList *l;
	_cleanup_dir_close_ GDir *dir = NULL;
	_cleanup_free_ gchar *root = NULL;

	/* search for .mo files in the prefix */
	root = g_build_filename (prefix, "/usr/share/locale", NULL);
	if (!g_file_test (root, G_FILE_TEST_EXISTS))
		return TRUE;
	dir = g_dir_open (root, 0, error);
	if (dir == NULL)
		return FALSE;
	while ((filename = g_dir_read_name (dir)) != NULL) {
		_cleanup_free_ gchar *path;
		path = g_build_filename (root, filename, "LC_MESSAGES", NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			if (!cra_gettext_ctx_search_locale (ctx, filename, path, error))
				return FALSE;
		}
	}

	/* calculate percentages */
	for (l = ctx->data; l != NULL; l = l->next) {
		e = l->data;
		e->percentage = MIN (e->nstrings * 100 / ctx->max_nstrings, 100);
	}

	/* sort */
	ctx->data = g_list_sort (ctx->data, cra_gettext_entry_sort_cb);
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
	CraGettextContext *ctx = NULL;
	CraGettextEntry *e;
	gboolean ret;
	GList *l;

	/* search */
	ctx = cra_gettext_ctx_new ();
	ret = cra_gettext_ctx_search_path (ctx, tmpdir, error);
	if (!ret)
		goto out;

	/* print results */
	for (l = ctx->data; l != NULL; l = l->next) {
		e = l->data;
		if (e->percentage < 25)
			continue;
		as_app_add_language (AS_APP (app), e->percentage, e->locale, -1);
	}
out:
	cra_gettext_ctx_free (ctx);
	return ret;
}
