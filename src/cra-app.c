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
#include "cra-dom.h"

typedef struct _CraAppPrivate	CraAppPrivate;
struct _CraAppPrivate
{
	gchar		*type_id;
	gchar		*project_group;
	gchar		*project_license;
	gchar		*compulsory_for_desktop;
	gchar		*homepage_url;
	gchar		*id;
	gchar		*id_full;
	gchar		*icon;
	GPtrArray	*categories;
	GPtrArray	*keywords;
	GPtrArray	*mimetypes;
	GPtrArray	*pkgnames;
	GPtrArray	*screenshots;
	gboolean	 requires_appdata;
	gboolean	 cached_icon;
	GHashTable	*names;
	GHashTable	*comments;
	GHashTable	*languages;
	GHashTable	*metadata;
	GdkPixbuf	*pixbuf;
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

	g_free (priv->id);
	g_free (priv->id_full);
	g_free (priv->type_id);
	g_free (priv->homepage_url);
	g_free (priv->project_group);
	g_free (priv->project_license);
	g_free (priv->compulsory_for_desktop);
	g_free (priv->icon);
	g_ptr_array_unref (priv->categories);
	g_ptr_array_unref (priv->keywords);
	g_ptr_array_unref (priv->mimetypes);
	g_ptr_array_unref (priv->pkgnames);
	g_ptr_array_unref (priv->screenshots);
	g_hash_table_unref (priv->names);
	g_hash_table_unref (priv->comments);
	g_hash_table_unref (priv->languages);
	g_hash_table_unref (priv->metadata);
	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);
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
	priv->pkgnames = g_ptr_array_new_with_free_func (g_free);
	priv->screenshots = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
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
 * cra_app_to_xml:
 **/
void
cra_app_insert_into_dom (CraApp *app, GNode *parent)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	CraScreenshot *ss;
	GNode *node_app;
	GNode *node_tmp;
	const gchar *tmp;
	guint i;

	node_app = cra_dom_insert (parent, "application", NULL, NULL);

	/* <id> */
	cra_dom_insert (node_app, "id", priv->id_full,
			"type", priv->type_id,
			NULL);

	/* <pkgname> */
	for (i = 0; i < priv->pkgnames->len; i++) {
		tmp = g_ptr_array_index (priv->pkgnames, i);
		cra_dom_insert (node_app, "pkgname", tmp, NULL);
	}

	/* <name> */
	cra_dom_insert_localized (node_app, "name", priv->names);

	/* <summary> */
	cra_dom_insert_localized (node_app, "summary", priv->comments);

	/* <icon> */
	if (priv->icon != NULL) {
		cra_dom_insert (node_app, "icon", priv->icon,
				"type", priv->cached_icon ? "cached" : "stock",
				NULL);
	}

	/* <categories> */
	if (priv->categories->len > 0) {
		node_tmp = cra_dom_insert (node_app, "appcategories", NULL, NULL);
		for (i = 0; i < priv->categories->len; i++) {
			tmp = g_ptr_array_index (priv->categories, i);
			cra_dom_insert (node_tmp, "appcategory", tmp, NULL);
		}
	}

	/* <keywords> */
	if (priv->keywords->len > 0) {
		node_tmp = cra_dom_insert (node_app, "keywords", NULL, NULL);
		for (i = 0; i < priv->keywords->len; i++) {
			tmp = g_ptr_array_index (priv->keywords, i);
			cra_dom_insert (node_tmp, "keyword", tmp, NULL);
		}
	}

	/* <mimetypes> */
	if (priv->mimetypes->len > 0) {
		node_tmp = cra_dom_insert (node_app, "mimetypes", NULL, NULL);
		for (i = 0; i < priv->mimetypes->len; i++) {
			tmp = g_ptr_array_index (priv->mimetypes, i);
			cra_dom_insert (node_tmp, "mimetype", tmp, NULL);
		}
	}

	/* <project_license> */
	if (priv->project_license != NULL)
		cra_dom_insert (node_app, "project_license", priv->project_license, NULL);

	/* <url> */
	if (priv->homepage_url != NULL) {
		cra_dom_insert (node_app, "url", priv->homepage_url,
				"type", "homepage",
				NULL);
	}

	/* <project_group> */
	if (priv->project_group != NULL)
		cra_dom_insert (node_app, "project_group", priv->project_group, NULL);

	/* <compulsory_for_desktop> */
	if (priv->compulsory_for_desktop != NULL)
		cra_dom_insert (node_app, "compulsory_for_desktop", priv->compulsory_for_desktop, NULL);

	/* <description> */
	cra_dom_insert (node_app, "description", "This is the long description", NULL);

	/* <screenshots> */
	if (priv->screenshots->len > 0) {
		node_tmp = cra_dom_insert (node_app, "screenshots", NULL, NULL);
		for (i = 0; i < priv->screenshots->len; i++) {
			ss = g_ptr_array_index (priv->screenshots, i);
			cra_screenshot_insert_into_dom (ss, node_tmp);
		}
	}

	/* <languages> */
	if (g_hash_table_size (priv->languages) > 0) {
		node_tmp = cra_dom_insert (node_app, "languages", NULL, NULL);
		cra_dom_insert_hash (node_tmp, "lang", "percentage", priv->languages, TRUE);
	}

	/* <metadata> */
	if (g_hash_table_size (priv->metadata) > 0) {
		node_tmp = cra_dom_insert (node_app, "metadata", NULL, NULL);
		cra_dom_insert_hash (node_tmp, "value", "key", priv->metadata, FALSE);
	}
}

/**
 * cra_app_to_xml:
 **/
gchar *
cra_app_to_xml (CraApp *app)
{
	CraDom *dom;
	GString *str;
	GNode *node_apps;
	GNode *node_root;

	dom = cra_dom_new ();
	node_root = cra_dom_get_root (dom);
	node_apps = cra_dom_insert (node_root, "applications", NULL, NULL);
	cra_app_insert_into_dom (app, node_apps);
	str = cra_dom_to_xml (dom);
	g_object_unref (dom);
	return g_string_free (str, FALSE);
}

/**
 * cra_app_set_type_id:
 **/
void
cra_app_set_type_id (CraApp *app, const gchar *type_id)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->type_id);
	priv->type_id = g_strdup (type_id);
}

/**
 * cra_app_set_homepage_url:
 **/
void
cra_app_set_homepage_url (CraApp *app, const gchar *homepage_url)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->homepage_url);
	priv->homepage_url = g_strdup (homepage_url);
}

/**
 * cra_app_set_project_group:
 **/
void
cra_app_set_project_group (CraApp *app, const gchar *project_group)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->project_group);
	priv->project_group = g_strdup (project_group);
}

/**
 * cra_app_set_project_license:
 **/
void
cra_app_set_project_license (CraApp *app, const gchar *project_license)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->project_license);
	priv->project_license = g_strdup (project_license);
}

/**
 * cra_app_set_compulsory_for_desktop:
 **/
void
cra_app_set_compulsory_for_desktop (CraApp *app, const gchar *compulsory_for_desktop)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->compulsory_for_desktop);
	priv->compulsory_for_desktop = g_strdup (compulsory_for_desktop);
}

/**
 * cra_app_set_icon:
 **/
void
cra_app_set_icon (CraApp *app, const gchar *icon)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_free (priv->icon);
	priv->icon = g_strdup (icon);
}

/**
 * cra_app_add_category:
 **/
void
cra_app_add_category (CraApp *app, const gchar *category)
{
	CraAppPrivate *priv = GET_PRIVATE (app);

	/* simple substitution */
	if (g_strcmp0 (category, "Feed") == 0)
		category = "News";

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
 * cra_app_add_screenshot:
 **/
void
cra_app_add_screenshot (CraApp *app, CraScreenshot *screenshot)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	g_ptr_array_add (priv->screenshots, g_object_ref (screenshot));
}

/**
 * cra_app_add_pkgname:
 **/
void
cra_app_add_pkgname (CraApp *app, const gchar *pkgname)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (cra_utils_ptr_array_find_string (priv->pkgnames, pkgname)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "Already added %s to pkgnames",
				 pkgname);
		return;
	}
	g_ptr_array_add (priv->pkgnames, g_strdup (pkgname));
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
	g_return_if_fail (locale != NULL && locale[0] != '\0');
	g_return_if_fail (comment != NULL);
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
 * cra_app_set_pixbuf:
 **/
void
cra_app_set_pixbuf (CraApp *app, GdkPixbuf *pixbuf)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (priv->pixbuf != NULL)
		g_object_ref (priv->pixbuf);
	priv->pixbuf = g_object_ref (pixbuf);
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
 * cra_app_get_requires_appdata:
 **/
gboolean
cra_app_get_requires_appdata (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->requires_appdata;
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
 * cra_app_get_screenshots:
 **/
GPtrArray *
cra_app_get_screenshots (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->screenshots;
}

/**
 * cra_app_get_id_full:
 **/
const gchar *
cra_app_get_id_full (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->id_full;
}

/**
 * cra_app_get_id:
 **/
const gchar *
cra_app_get_id (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->id;
}

/**
 * cra_app_get_type_id:
 **/
const gchar *
cra_app_get_type_id (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->type_id;
}

/**
 * cra_app_get_icon:
 **/
const gchar *
cra_app_get_icon (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->icon;
}

/**
 * cra_app_get_name:
 **/
const gchar *
cra_app_get_name (CraApp *app, const gchar *locale)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return g_hash_table_lookup (priv->names, locale);
}

/**
 * cra_app_get_comment:
 **/
const gchar *
cra_app_get_comment (CraApp *app, const gchar *locale)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return g_hash_table_lookup (priv->comments, locale);
}

/**
 * cra_app_get_metadata_item:
 **/
const gchar *
cra_app_get_metadata_item (CraApp *app, const gchar *key)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return g_hash_table_lookup (priv->metadata, key);
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
 * cra_app_set_id_full:
 **/
static void
cra_app_set_id_full (CraApp *app, const gchar *id_full)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	gchar *tmp;

	priv->id_full = g_strdup (id_full);
	g_strdelimit (priv->id_full, "&<>", '-');
	priv->id = g_strdup (priv->id_full);
	tmp = g_strstr_len (priv->id, -1, ".desktop");
	if (tmp != NULL)
		*tmp = '\0';
}

/**
 * cra_app_get_package:
 **/
CraPackage *
cra_app_get_package (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->pkg;
}

gboolean
cra_app_save_resources (CraApp *app, GError **error)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	CraScreenshot *ss;
	gboolean ret = TRUE;
	guint i;
	gchar *filename = NULL;

	/* any non-stock icon set */
	if (priv->pixbuf != NULL) {
		filename = g_build_filename ("./icons", priv->icon, NULL);
		ret = gdk_pixbuf_save (priv->pixbuf,
				       filename,
				       "png",
				       error,
				       NULL);
		if (!ret)
			goto out;

		/* set new AppStream compatible icon name */
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "saved icon %s", filename);
	}

	/* save any screenshots */
	for (i = 0; i < priv->screenshots->len; i++) {
		ss = g_ptr_array_index (priv->screenshots, i);
		ret = cra_screenshot_save_resources (ss, error);
		if (!ret)
			goto out;
	}
out:
	g_free (filename);
	return ret;
}

/**
 * cra_app_new:
 **/
CraApp *
cra_app_new (CraPackage *pkg, const gchar *id_full)
{
	CraApp *app;
	CraAppPrivate *priv;

	app = g_object_new (CRA_TYPE_APP, NULL);
	priv = GET_PRIVATE (app);
	priv->pkg = g_object_ref (pkg);
	cra_app_add_pkgname (app, cra_package_get_name (pkg));
	cra_app_set_id_full (app, id_full);
	return CRA_APP (app);
}
