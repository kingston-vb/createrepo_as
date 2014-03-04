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

/**
 * SECTION:cra-dom
 * @short_description: A XML parser that exposes a DOM tree
 */

#include "config.h"

#include <glib.h>

#include "cra-dom.h"

static void	cra_dom_class_init	(CraDomClass	*klass);
static void	cra_dom_init		(CraDom		*dom);
static void	cra_dom_finalize		(GObject	*object);

#define CRA_DOM_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), CRA_TYPE_DOM, CraDomPrivate))

/**
 * CraDomPrivate:
 *
 * Private #CraDom data
 **/
struct _CraDomPrivate
{
	GNode			*root;
	GNode			*current;
};

typedef struct
{
	gchar		*name;
	GString		*craata;
	GHashTable	*attributes;
} CraDomNodeData;

G_DEFINE_TYPE (CraDom, cra_dom, G_TYPE_OBJECT)

/**
 * cra_dom_print_node_cb:
 **/
static gboolean
cra_dom_print_node_cb (GNode *node, gpointer user_data)
{
	CraDomNodeData *data = node->data;
	GString *string = user_data;
	guint depth = g_node_depth (node);
	guint i;

	if (data == NULL)
		goto out;
	for (i = 0; i < depth; i++)
		g_string_append (string, " ");
	g_string_append_printf (string,
				"<%s> [%s]\n",
				data->name,
				data->craata->str);
out:
	return FALSE;
}

/**
 * cra_dom_to_string:
 * @dom: a #CraDom instance.
 *
 * Returns a string representation of the DOM tree.
 *
 * Return value: an allocated string
 **/
gchar *
cra_dom_to_string (CraDom *dom)
{
	GString *string;

	g_return_val_if_fail (CRA_IS_DOM (dom), NULL);

	string = g_string_new (NULL);
	g_node_traverse (dom->priv->root,
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 cra_dom_print_node_cb,
			 string);

	return g_string_free (string, FALSE);
}

/**
 * cra_dom_start_element_cb:
 **/
static void
cra_dom_start_element_cb (GMarkupParseContext *context,
			 const gchar         *element_name,
			 const gchar        **attribute_names,
			 const gchar        **attribute_values,
			 gpointer             user_data,
			 GError             **error)
{
	CraDom *dom = (CraDom *) user_data;
	CraDomNodeData *data;
	GNode *new;
	guint i;

	/* create the new node data */
	data = g_slice_new (CraDomNodeData);
	data->name = g_strdup (element_name);
	data->craata = g_string_new (NULL);
	data->attributes = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  g_free,
						  g_free);
	for (i = 0; attribute_names[i] != NULL; i++) {
		g_hash_table_insert (data->attributes,
				     g_strdup (attribute_names[i]),
				     g_strdup (attribute_values[i]));
	}

	/* add the node to the DOM */
	new = g_node_new (data);
	g_node_append (dom->priv->current, new);
	dom->priv->current = new;
}

/**
 * cra_dom_end_element_cb:
 **/
static void
cra_dom_end_element_cb (GMarkupParseContext *context,
		       const gchar         *element_name,
		       gpointer             user_data,
		       GError             **error)
{
	CraDom *dom = (CraDom *) user_data;
	dom->priv->current = dom->priv->current->parent;
}

/**
 * cra_dom_text_cb:
 **/
static void
cra_dom_text_cb (GMarkupParseContext *context,
		const gchar         *text,
		gsize                text_len,
		gpointer             user_data,
		GError             **error)
{
	CraDom *dom = (CraDom *) user_data;
	CraDomNodeData *data;
	guint i;

	/* no data */
	if (text_len == 0)
		return;

	/* all whitespace? */
	for (i = 0; i < text_len; i++) {
		if (text[i] != ' ' &&
		    text[i] != '\n' &&
		    text[i] != '\t')
			break;
	}
	if (i >= text_len)
		return;

	/* save craata */
	data = dom->priv->current->data;
	g_string_append (data->craata, text);
}

/**
 * cra_dom_parse_xml_data:
 * @dom: a #CraDom instance.
 * @data: XML data
 * @data_len: Length of @data, or -1 if NULL terminated
 * @error: A #GError or %NULL
 *
 * Parses data into a DOM tree.
 **/
gboolean
cra_dom_parse_xml_data (CraDom *dom,
		       const gchar *data,
		       gssize data_len,
		       GError **error)
{
	gboolean ret;
	GMarkupParseContext *ctx;
	const GMarkupParser parser = {
		cra_dom_start_element_cb,
		cra_dom_end_element_cb,
		cra_dom_text_cb,
		NULL,
		NULL };

	g_return_val_if_fail (CRA_IS_DOM (dom), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);

	ctx = g_markup_parse_context_new (&parser,
					  G_MARKUP_PREFIX_ERROR_POSITION,
					  dom,
					  NULL);
	ret = g_markup_parse_context_parse (ctx,
					    data,
					    data_len,
					    error);
	if (!ret)
		goto out;
out:
	g_markup_parse_context_free (ctx);
	return ret;
}

/**
 * cra_dom_get_child_node:
 **/
static GNode *
cra_dom_get_child_node (const GNode *root, const gchar *name)
{
	GNode *node;
	CraDomNodeData *data;

	/* find a node called name */
	for (node = root->children; node != NULL; node = node->next) {
		data = node->data;
		if (data == NULL)
			return NULL;
		if (g_strcmp0 (data->name, name) == 0)
			return node;
	}
	return NULL;
}

/**
 * cra_dom_get_node_name:
 * @node: a #GNode
 *
 * Gets the node name, e.g. "body"
 *
 * Return value: string value
 **/
const gchar *
cra_dom_get_node_name (const GNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	if (node->data == NULL)
		return NULL;
	return ((CraDomNodeData *) node->data)->name;
}

/**
 * cra_dom_get_node_data:
 * @node: a #GNode
 *
 * Gets the node data, e.g. "paragraph text"
 *
 * Return value: string value
 **/
const gchar *
cra_dom_get_node_data (const GNode *node)
{
	g_return_val_if_fail (node != NULL, NULL);
	if (node->data == NULL)
		return NULL;
	return ((CraDomNodeData *) node->data)->craata->str;
}

/**
 * cra_dom_get_node_attribute:
 * @node: a #GNode
 *
 * Gets a node attribute, e.g. "false"
 *
 * Return value: string value
 **/
const gchar *
cra_dom_get_node_attribute (const GNode *node, const gchar *key)
{
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	if (node->data == NULL)
		return NULL;
	return g_hash_table_lookup (((CraDomNodeData *) node->data)->attributes, key);
}

/**
 * cra_dom_get_node:
 * @dom: a #CraDom instance.
 * @root: a root node, or %NULL
 * @path: a path in the DOM, e.g. "html/body"
 *
 * Gets a node from the DOM tree.
 *
 * Return value: A #GNode, or %NULL if not found
 **/
const GNode *
cra_dom_get_node (CraDom *dom, const GNode *root, const gchar *path)
{
	gchar **split;
	const GNode *node;
	guint i;

	g_return_val_if_fail (CRA_IS_DOM (dom), NULL);
	g_return_val_if_fail (path != NULL, NULL);

	/* default value */
	if (root == NULL)
		root = dom->priv->root;

	node = root;
	split = g_strsplit (path, "/", -1);
	for (i = 0; split[i] != NULL; i++) {
		node = cra_dom_get_child_node (node, split[i]);
		if (node == NULL)
			goto out;
	}
out:
	g_strfreev (split);
	return node;
}

/**
 * cra_dom_get_node_localized:
 * @node: a #GNode
 * @key: the key to use, e.g. "copyright"
 *
 * Extracts localized values from the DOM tree
 *
 * Return value: (transfer full): A hash table with the locale (e.g. en_GB) as the key
 **/
GHashTable *
cra_dom_get_node_localized (const GNode *node, const gchar *key)
{
	CraDomNodeData *data;
	const gchar *xml_lang;
	const gchar *data_unlocalized;
	const gchar *data_localized;
	GHashTable *hash = NULL;
	GNode *tmp;

	/* does it exist? */
	tmp = cra_dom_get_child_node (node, key);
	if (tmp == NULL)
		goto out;
	data_unlocalized = cra_dom_get_node_data (tmp);

	/* find a node called name */
	hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	for (tmp = node->children; tmp != NULL; tmp = tmp->next) {
		data = tmp->data;
		if (data == NULL)
			continue;
		if (g_strcmp0 (data->name, key) != 0)
			continue;

		/* avoid storing identical strings */
		xml_lang = g_hash_table_lookup (data->attributes, "xml:lang");
		data_localized = data->craata->str;
		if (xml_lang != NULL && g_strcmp0 (data_unlocalized, data_localized) == 0)
			continue;
		g_hash_table_insert (hash,
				     g_strdup (xml_lang != NULL ? xml_lang : ""),
				     g_strdup (data_localized));
	}
out:
	return hash;
}

/**
 * cra_dom_destroy_node_cb:
 **/
static gboolean
cra_dom_destroy_node_cb (GNode *node, gpointer user_data)
{
	CraDomNodeData *data = node->data;
	if (data == NULL)
		goto out;
	g_free (data->name);
	g_string_free (data->craata, TRUE);
	g_hash_table_unref (data->attributes);
	g_slice_free (CraDomNodeData, data);
out:
	return FALSE;
}

/**
 * cra_dom_class_init:
 */
static void
cra_dom_class_init (CraDomClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_dom_finalize;
	g_type_class_add_private (klass, sizeof (CraDomPrivate));
}

/**
 * cra_dom_init:
 */
static void
cra_dom_init (CraDom *dom)
{
	dom->priv = CRA_DOM_GET_PRIVATE (dom);
	dom->priv->root = g_node_new (NULL);
	dom->priv->current = dom->priv->root;
}

/**
 * cra_dom_finalize:
 */
static void
cra_dom_finalize (GObject *object)
{
	CraDom *dom = CRA_DOM (object);

	g_node_traverse (dom->priv->root,
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 cra_dom_destroy_node_cb,
			 NULL);
	g_node_destroy (dom->priv->root);

	G_OBJECT_CLASS (cra_dom_parent_class)->finalize (object);
}

/**
 * cra_dom_new:
 *
 * Creates a new #CraDom object.
 *
 * Return value: a new CraDom object.
 **/
CraDom *
cra_dom_new (void)
{
	CraDom *dom;
	dom = g_object_new (CRA_TYPE_DOM, NULL);
	return CRA_DOM (dom);
}
