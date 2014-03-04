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

typedef struct _CraAppPrivate	CraAppPrivate;
struct _CraAppPrivate
{
	gchar		*type_id;
	gchar		*project_group;
	gchar		*homepage_url;
	gchar		*app_id;
	gchar		*icon;
	GPtrArray	*categories;
	GPtrArray	*keywords;
	GPtrArray	*mimetypes;
	gboolean	 requires_appdata;
	gboolean	 cached_icon;
	GHashTable	*names;
	GHashTable	*comments;
	GHashTable	*languages;
	GHashTable	*metadata;
	CraPackage	*pkg;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraApp, cra_app, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_app_get_instance_private (o))

/**
 * cra_app_finalize:
 **/
static void
cra_app_finalize (GObject *object)
{
	CraApp *app = CRA_APP (object);
	CraAppPrivate *priv = GET_PRIVATE (app);

	g_free (priv->app_id);
	g_free (priv->type_id);
	g_free (priv->homepage_url);
	g_free (priv->project_group);
	g_free (priv->icon);
	g_ptr_array_unref (priv->categories);
	g_ptr_array_unref (priv->keywords);
	g_ptr_array_unref (priv->mimetypes);
	g_hash_table_unref (priv->names);
	g_hash_table_unref (priv->comments);
	g_hash_table_unref (priv->languages);
	g_hash_table_unref (priv->metadata);
	g_object_unref (priv->pkg);

	G_OBJECT_CLASS (cra_app_parent_class)->finalize (object);
}

/**
 * cra_app_init:
 **/
static void
cra_app_init (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->categories = g_ptr_array_new_with_free_func (g_free);
	priv->keywords = g_ptr_array_new_with_free_func (g_free);
	priv->mimetypes = g_ptr_array_new_with_free_func (g_free);
	priv->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	priv->comments = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	priv->languages = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	priv->metadata = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

/**
 * cra_app_class_init:
 **/
static void
cra_app_class_init (CraAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_app_finalize;
}

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
 * cra_app_to_string_hash:
 **/
static void
cra_app_to_string_hash (GString *str, GHashTable *hash, const gchar *header)
{
	const gchar *tmp;
	GList *l;
	GList *list;

	list = g_hash_table_get_keys (hash);
	list = g_list_sort (list, cra_app_list_sort_cb);
	for (l = list; l != NULL; l = l->next) {
		tmp = g_hash_table_lookup (hash, l->data);
		g_string_append_printf (str, "%s\t%s\t%s\n",
					header,
					(const gchar *) l->data,
					(const gchar *) tmp);
	}
	g_list_free (list);
}

/**
 * cra_app_to_string:
 **/
gchar *
cra_app_to_string (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	const gchar *tmp;
	GString *str;
	guint i;

	str = g_string_new ("");
	g_string_append_printf (str, "app-id:\t\t%s\n", priv->app_id);
	cra_app_to_string_hash (str, priv->names, "names:\t");
	cra_app_to_string_hash (str, priv->comments, "comments:");
	cra_app_to_string_hash (str, priv->languages, "languages:");
	cra_app_to_string_hash (str, priv->metadata, "metadata:");

	g_string_append_printf (str, "type-id:\t%s\n", priv->type_id);
	if (priv->project_group != NULL)
		g_string_append_printf (str, "project:\t%s\n", priv->project_group);
	if (priv->homepage_url != NULL)
		g_string_append_printf (str, "homepage:\t%s\n", priv->homepage_url);
	if (priv->icon != NULL)
		g_string_append_printf (str, "icon:\t\t%s\n", priv->icon);
	g_string_append_printf (str, "req-appdata:\t%s\n", priv->requires_appdata ? "True" : "False");
	g_string_append_printf (str, "cached-icon:\t%s\n", priv->cached_icon ? "True" : "False");
	for (i = 0; i < priv->categories->len; i++) {
		tmp = g_ptr_array_index (priv->categories, i);
		g_string_append_printf (str, "category:\t%s\n", tmp);
	}
	for (i = 0; i < priv->keywords->len; i++) {
		tmp = g_ptr_array_index (priv->keywords, i);
		g_string_append_printf (str, "keyword:\t%s\n", tmp);
	}
	for (i = 0; i < priv->mimetypes->len; i++) {
		tmp = g_ptr_array_index (priv->mimetypes, i);
		g_string_append_printf (str, "mimetype:\t%s\n", tmp);
	}
	return g_string_free (str, FALSE);
}

/**
 * cra_app_set_type_id:
 **/
void
cra_app_set_type_id (CraApp *app, const gchar *type_id)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->type_id = g_strdup (type_id);
}

/**
 * cra_app_set_homepage_url:
 **/
void
cra_app_set_homepage_url (CraApp *app, const gchar *homepage_url)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->homepage_url = g_strdup (homepage_url);
}

/**
 * cra_app_set_project_group:
 **/
void
cra_app_set_project_group (CraApp *app, const gchar *project_group)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->project_group = g_strdup (project_group);
}

/**
 * cra_app_set_icon:
 **/
void
cra_app_set_icon (CraApp *app, const gchar *icon)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->icon = g_strdup (icon);
}

/**
 * cra_app_add_category:
 **/
void
cra_app_add_category (CraApp *app, const gchar *category)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (cra_utils_ptr_array_find_string (priv->categories, category)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Already added %s to categories",
				 category);
		return;
	}
	g_ptr_array_add (priv->categories, g_strdup (category));
}

/**
 * cra_app_add_keyword:
 **/
void
cra_app_add_keyword (CraApp *app, const gchar *keyword)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (cra_utils_ptr_array_find_string (priv->keywords, keyword)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Already added %s to keywords",
				 keyword);
		return;
	}
	g_ptr_array_add (priv->keywords, g_strdup (keyword));
}

/**
 * cra_app_add_mimetype:
 **/
void
cra_app_add_mimetype (CraApp *app, const gchar *mimetype)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (cra_utils_ptr_array_find_string (priv->mimetypes, mimetype)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Already added %s to mimetypes",
				 mimetype);
		return;
	}
	g_ptr_array_add (priv->mimetypes, g_strdup (mimetype));
}

/**
 * cra_app_set_name:
 **/
void
cra_app_set_name (CraApp *app, const gchar *locale, const gchar *name)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_hash_table_insert (priv->names, g_strdup (locale), g_strdup (name));
}

/**
 * cra_app_set_comment:
 **/
void
cra_app_set_comment (CraApp *app, const gchar *locale, const gchar *comment)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_hash_table_insert (priv->comments, g_strdup (locale), g_strdup (comment));
}

/**
 * cra_app_add_language:
 **/
void
cra_app_add_language (CraApp *app, const gchar *locale, const gchar *value)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_hash_table_insert (priv->languages, g_strdup (locale), g_strdup (value));
}

/**
 * cra_app_add_metadata:
 **/
void
cra_app_add_metadata (CraApp *app, const gchar *key, const gchar *value)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_hash_table_insert (priv->metadata, g_strdup (key), g_strdup (value));
}

/**
 * cra_app_set_requires_appdata:
 **/
void
cra_app_set_requires_appdata (CraApp *app, gboolean requires_appdata)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->requires_appdata = requires_appdata;
}

/**
 * cra_app_set_cached_icon:
 **/
void
cra_app_set_cached_icon (CraApp *app, gboolean cached_icon)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->cached_icon = cached_icon;
}

/**
 * cra_app_get_categories:
 **/
GPtrArray *
cra_app_get_categories (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->categories;
}

/**
 * cra_app_get_keywords:
 **/
GPtrArray *
cra_app_get_keywords (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->keywords;
}

/**
 * cra_app_get_app_id:
 **/
const gchar *
cra_app_get_app_id (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->app_id;
}

/**
 * cra_app_get_project_group:
 **/
const gchar *
cra_app_get_project_group (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->project_group;
}

/**
 * cra_app_new:
 **/
CraApp *
cra_app_new (CraPackage *pkg, const gchar *app_id)
{
	CraApp *app;
	CraAppPrivate *priv;
	app = g_object_new (CRA_TYPE_APP, NULL);
	priv = GET_PRIVATE (app);
	priv->app_id = g_strdup (app_id);
	priv->pkg = g_object_ref (pkg);
	return CRA_APP (app);
}
