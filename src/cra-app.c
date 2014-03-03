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

#include "cra-app.h"

/**
 * cra_utils_ptr_array_find_string:
 **/
static gboolean
cra_utils_ptr_array_find_string (GPtrArray *array, const gchar *value)
{
	const gchar *tmp;
	guint i;
	for (i = 0; i < array->len; i++) {
		tmp = g_ptr_array_index (array, i);
		if (g_strcmp0 (tmp, value) == 0)
			return TRUE;
	}
	return FALSE;
}

/**
 * cra_app_list_sort_cb:
 **/
static gint
cra_app_list_sort_cb (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 ((const gchar *) a, (const gchar *) b);
}

/**
 * cra_app_print_hash:
 **/
static void
cra_app_print_hash (GHashTable *hash, const gchar *header)
{
	GList *list;
	GList *l;

	list = g_hash_table_get_keys (hash);
	list = g_list_sort (list, cra_app_list_sort_cb);
	for (l = list; l != NULL; l = l->next) {
		g_print ("%s\t%s\t%s\n",
			 header,
			 (const gchar *) l->data,
			 (const gchar *) g_hash_table_lookup (hash, l->data));
	}
	g_list_free (list);
}

/**
 * cra_app_print:
 **/
void
cra_app_print (CraApp *app)
{
	const gchar *tmp;
	guint i;

	g_print ("app-id:\t\t%s\n", app->app_id);
	cra_app_print_hash (app->names, "names:\t");
	cra_app_print_hash (app->comments, "comments:");
	cra_app_print_hash (app->languages, "languages:");
	cra_app_print_hash (app->metadata, "metadata:");

	g_print ("type-id:\t%s\n", app->type_id);
	if (app->project_group != NULL)
		g_print ("project:\t%s\n", app->project_group);
	if (app->homepage_url != NULL)
		g_print ("homepage:\t%s\n", app->homepage_url);
	if (app->icon != NULL)
		g_print ("icon:\t\t%s\n", app->icon);
	g_print ("req-appdata:\t%s\n", app->requires_appdata ? "True" : "False");
	g_print ("cached-icon:\t%s\n", app->cached_icon ? "True" : "False");
	for (i = 0; i < app->categories->len; i++) {
		tmp = g_ptr_array_index (app->categories, i);
		g_print ("category:\t%s\n", tmp);
	}
	for (i = 0; i < app->keywords->len; i++) {
		tmp = g_ptr_array_index (app->keywords, i);
		g_print ("keyword:\t%s\n", tmp);
	}
	for (i = 0; i < app->mimetypes->len; i++) {
		tmp = g_ptr_array_index (app->mimetypes, i);
		g_print ("mimetype:\t%s\n", tmp);
	}
}

/**
 * cra_app_set_type_id:
 **/
void
cra_app_set_type_id (CraApp *app, const gchar *type_id)
{
	app->type_id = g_strdup (type_id);
}

/**
 * cra_app_set_homepage_url:
 **/
void
cra_app_set_homepage_url (CraApp *app, const gchar *homepage_url)
{
	app->homepage_url = g_strdup (homepage_url);
}

/**
 * cra_app_set_project_group:
 **/
void
cra_app_set_project_group (CraApp *app, const gchar *project_group)
{
	app->project_group = g_strdup (project_group);
}

/**
 * cra_app_set_icon:
 **/
void
cra_app_set_icon (CraApp *app, const gchar *icon)
{
	app->icon = g_strdup (icon);
}

/**
 * cra_app_add_category:
 **/
void
cra_app_add_category (CraApp *app, const gchar *category)
{
	if (cra_utils_ptr_array_find_string (app->categories, category)) {
		g_warning ("Already added %s to categories", category);
		return;
	}
	g_ptr_array_add (app->categories, g_strdup (category));
}

/**
 * cra_app_add_keyword:
 **/
void
cra_app_add_keyword (CraApp *app, const gchar *keyword)
{
	if (cra_utils_ptr_array_find_string (app->keywords, keyword)) {
		g_warning ("Already added %s to keywords", keyword);
		return;
	}
	g_ptr_array_add (app->keywords, g_strdup (keyword));
}

/**
 * cra_app_add_mimetype:
 **/
void
cra_app_add_mimetype (CraApp *app, const gchar *mimetype)
{
	if (cra_utils_ptr_array_find_string (app->mimetypes, mimetype)) {
		g_warning ("Already added %s to mimetypes", mimetype);
		return;
	}
	g_ptr_array_add (app->mimetypes, g_strdup (mimetype));
}

/**
 * cra_app_set_name:
 **/
void
cra_app_set_name (CraApp *app, const gchar *locale, const gchar *name)
{
	g_hash_table_insert (app->names, g_strdup (locale), g_strdup (name));
}

/**
 * cra_app_set_comment:
 **/
void
cra_app_set_comment (CraApp *app, const gchar *locale, const gchar *comment)
{
	g_hash_table_insert (app->comments, g_strdup (locale), g_strdup (comment));
}

/**
 * cra_app_add_language:
 **/
void
cra_app_add_language (CraApp *app, const gchar *locale, const gchar *value)
{
	g_hash_table_insert (app->languages, g_strdup (locale), g_strdup (value));
}

/**
 * cra_app_add_metadata:
 **/
void
cra_app_add_metadata (CraApp *app, const gchar *key, const gchar *value)
{
	g_hash_table_insert (app->metadata, g_strdup (key), g_strdup (value));
}

/**
 * cra_app_set_requires_appdata:
 **/
void
cra_app_set_requires_appdata (CraApp *app, gboolean requires_appdata)
{
	app->requires_appdata = requires_appdata;
}

/**
 * cra_app_set_cached_icon:
 **/
void
cra_app_set_cached_icon (CraApp *app, gboolean cached_icon)
{
	app->cached_icon = cached_icon;
}

/**
 * cra_app_new:
 **/
CraApp *
cra_app_new (const gchar *app_id)
{
	CraApp *app = NULL;
	app = g_slice_new0 (CraApp);
	app->app_id = g_strdup (app_id);
	app->categories = g_ptr_array_new_with_free_func (g_free);
	app->keywords = g_ptr_array_new_with_free_func (g_free);
	app->mimetypes = g_ptr_array_new_with_free_func (g_free);
	app->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	app->comments = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	app->languages = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	app->metadata = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	return app;
}

/**
 * cra_app_free:
 **/
void
cra_app_free (CraApp *app)
{
	g_free (app->app_id);
	g_free (app->type_id);
	g_free (app->homepage_url);
	g_free (app->project_group);
	g_free (app->icon);
	g_ptr_array_unref (app->categories);
	g_ptr_array_unref (app->keywords);
	g_ptr_array_unref (app->mimetypes);
	g_hash_table_unref (app->names);
	g_hash_table_unref (app->comments);
	g_hash_table_unref (app->languages);
	g_hash_table_unref (app->metadata);
	g_slice_free (CraApp, app);
}
