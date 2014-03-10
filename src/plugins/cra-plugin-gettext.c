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
	cra_plugin_add_glob (globs, "./usr/share/locale/*/LC_MESSAGES/*.mo");
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
	gboolean ret;
	gchar *data = NULL;
	CraGettextEntry *entry;
	CraGettextHeader *h;

	/* read data, although we only strictly need the header */
	ret = g_file_get_contents (filename, &data, NULL, error);
	if (!ret)
		goto out;

	h = (CraGettextHeader *) data;
	entry = cra_gettext_entry_new ();
	entry->locale = g_strdup (locale);
	entry->nstrings = h->nstrings;
	if (entry->nstrings > ctx->max_nstrings)
		ctx->max_nstrings = entry->nstrings;
	ctx->data = g_list_prepend (ctx->data, entry);
out:
	g_free (data);
	return ret;
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
	gboolean ret = TRUE;
	gchar *path;
	GDir *dir;

	dir = g_dir_open (messages_path, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((filename = g_dir_read_name (dir)) != NULL) {
		path = g_build_filename (messages_path, filename, NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			ret = cra_gettext_parse_file (ctx, locale, path, error);
			if (!ret)
				goto out;
		}
		g_free (path);
	}
out:
	if (dir != NULL)
		g_dir_close (dir);
	return ret;
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
	gboolean ret = TRUE;
	gchar *path;
	gchar *root = NULL;
	GDir *dir = NULL;
	GList *l;

	/* search for .mo files in the prefix */
	root = g_build_filename (prefix, "/usr/share/locale", NULL);
	if (!g_file_test (root, G_FILE_TEST_EXISTS))
		goto out;
	dir = g_dir_open (root, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}
	while ((filename = g_dir_read_name (dir)) != NULL) {
		path = g_build_filename (root, filename, "LC_MESSAGES", NULL);
		if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			ret = cra_gettext_ctx_search_locale (ctx, filename, path, error);
			if (!ret)
				goto out;
		}
		g_free (path);
	}

	/* calculate percentages */
	for (l = ctx->data; l != NULL; l = l->next) {
		e = l->data;
		e->percentage = MIN (e->nstrings * 100 / ctx->max_nstrings, 100);
	}

	/* sort */
	ctx->data = g_list_sort (ctx->data, cra_gettext_entry_sort_cb);
out:
	g_free (root);
	if (dir != NULL)
		g_dir_close (dir);
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
	CraGettextContext *ctx = NULL;
	CraGettextEntry *e;
	gboolean ret;
	gchar tmp[4];
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
		g_snprintf (tmp, 4, "%i", e->percentage);
		cra_app_add_language (app, e->locale, tmp);
	}
out:
	if (ctx != NULL)
		cra_gettext_ctx_free (ctx);
	return ret;
}
