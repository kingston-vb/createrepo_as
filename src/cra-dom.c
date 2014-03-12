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
#include "cra-plugin.h"

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

typedef enum {
	CRA_DOM_DATA_MODE_RAW,		/* '&amp;' rather than '&' */
	CRA_DOM_DATA_MODE_ESCAPED,	/* '&' rather than '&amp;' */
	CRA_DOM_DATA_MODE_LAST,
} CraDomDataMode;

typedef struct
{
	gchar		*name;
	GString		*cdata;
	CraDomDataMode	 cdata_mode;
	GHashTable	*attributes;
} CraDomNodeData;

G_DEFINE_TYPE (CraDom, cra_dom, G_TYPE_OBJECT)

/**
 * cra_dom_cdata_to_raw:
 **/
static void
cra_dom_cdata_to_raw (CraDomNodeData *data)
{
	if (data->cdata_mode == CRA_DOM_DATA_MODE_RAW)
		return;
	cra_string_replace (data->cdata, "&amp;", "&");
	cra_string_replace (data->cdata, "&lt;", "<");
	cra_string_replace (data->cdata, "&gt;", ">");
	data->cdata_mode = CRA_DOM_DATA_MODE_RAW;
}

/**
 * cra_dom_cdata_to_escaped:
 **/
static void
cra_dom_cdata_to_escaped (CraDomNodeData *data)
{
	if (data->cdata_mode == CRA_DOM_DATA_MODE_ESCAPED)
		return;
	cra_string_replace (data->cdata, "&", "&amp;");
	cra_string_replace (data->cdata, "<", "&lt;");
	cra_string_replace (data->cdata, ">", "&gt;");
	data->cdata_mode = CRA_DOM_DATA_MODE_ESCAPED;
}

/**
 * cra_dom_add_padding:
 **/
static void
cra_dom_add_padding (GString *xml, guint depth)
{
	guint i;
	for (i = 0; i < depth; i++)
		g_string_append (xml, " ");
}

/**
 * cra_dom_get_attr_string:
 **/
static gchar *
cra_dom_get_attr_string (GHashTable *hash)
{
	const gchar *key;
	const gchar *value;
	GList *keys;
	GList *l;
	GString *str;

	str = g_string_new ("");
	keys = g_hash_table_get_keys (hash);
	for (l = keys; l != NULL; l = l->next) {
		key = l->data;
		value = g_hash_table_lookup (hash, key);
		g_string_append_printf (str, " %s=\"%s\"", key, value);
	}
	g_list_free (keys);
	return g_string_free (str, FALSE);
}

/**
 * cra_dom_to_xml_string:
 **/
static void
cra_dom_to_xml_string (GString *xml,
		       guint depth_offset,
		       const GNode *n,
		       CraDomToXmlFlags flags)
{
	CraDomNodeData *data = n->data;
	GNode *c;
	guint depth = g_node_depth ((GNode *) n);
	gchar *attrs;

	/* root node */
	if (data == NULL) {
		for (c = n->children; c != NULL; c = c->next)
			cra_dom_to_xml_string (xml, depth_offset, c, flags);

	/* leaf node */
	} else if (n->children == NULL) {
		if ((flags & CRA_DOM_TO_XML_FORMAT_INDENT) > 0)
			cra_dom_add_padding (xml, depth - depth_offset);
		attrs = cra_dom_get_attr_string (data->attributes);
		if (data->cdata->len == 0) {
			g_string_append_printf (xml, "<%s%s/>",
						data->name, attrs);
		} else {
			cra_dom_cdata_to_escaped (data);
			g_string_append_printf (xml, "<%s%s>%s</%s>",
						data->name,
						attrs,
						data->cdata->str,
						data->name);
		}
		if ((flags & CRA_DOM_TO_XML_FORMAT_MULTILINE) > 0)
			g_string_append (xml, "\n");
		g_free (attrs);

	/* node with children */
	} else {
		if ((flags & CRA_DOM_TO_XML_FORMAT_INDENT) > 0)
			cra_dom_add_padding (xml, depth - depth_offset);
		attrs = cra_dom_get_attr_string (data->attributes);
		g_string_append_printf (xml, "<%s%s>", data->name, attrs);
		if ((flags & CRA_DOM_TO_XML_FORMAT_MULTILINE) > 0)
			g_string_append (xml, "\n");
		g_free (attrs);

		for (c = n->children; c != NULL; c = c->next)
			cra_dom_to_xml_string (xml, depth_offset, c, flags);

		if ((flags & CRA_DOM_TO_XML_FORMAT_INDENT) > 0)
			cra_dom_add_padding (xml, depth - depth_offset);
		g_string_append_printf (xml, "</%s>", data->name);
		if ((flags & CRA_DOM_TO_XML_FORMAT_MULTILINE) > 0)
			g_string_append (xml, "\n");
	}
}

/**
 * cra_dom_to_xml:
 * @dom: a #CraDom instance.
 *
 * Returns a XML representation of the DOM tree.
 *
 * Return value: an allocated string
 **/
GString *
cra_dom_to_xml (const GNode *node, CraDomToXmlFlags flags)
{
	GString *xml;
	guint depth_offset = g_node_depth ((GNode *) node);
	xml = g_string_new ("");
	if ((flags & CRA_DOM_TO_XML_ADD_HEADER) > 0)
		g_string_append (xml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	cra_dom_to_xml_string (xml, depth_offset, node, flags);
	return xml;
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
	data->cdata = g_string_new (NULL);
	data->cdata_mode = CRA_DOM_DATA_MODE_RAW;
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
	CraDomNodeData *data;
	data = dom->priv->current->data;
	cra_string_replace (data->cdata, "\n", " ");
	cra_string_replace (data->cdata, "  ", " ");
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
	gchar **split;

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

	/* save cdata */
	data = dom->priv->current->data;

	/* split up into lines and add each with spaces stripped */
	split = g_strsplit (text, "\n", -1);
	for (i = 0; split[i] != NULL; i++) {
		g_strstrip (split[i]);
		if (split[i][0] == '\0')
			continue;
		g_string_append_printf (data->cdata, "%s ", split[i]);
	}

	/* remove trailing space */
	if (data->cdata->len > 1 &&
	    data->cdata->str[data->cdata->len - 1] == ' ') {
		g_string_truncate (data->cdata, data->cdata->len - 1);
	}

	g_strfreev (split);
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
	CraDomNodeData *data;
	g_return_val_if_fail (node != NULL, NULL);
	if (node->data == NULL)
		return NULL;
	data = (CraDomNodeData *) node->data;
	cra_dom_cdata_to_raw (data);
	return data->cdata->str;
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
GNode *
cra_dom_get_node (GNode *root, const gchar *path)
{
	gchar **split;
	GNode *node = root;
	guint i;

	g_return_val_if_fail (path != NULL, NULL);

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
 * cra_dom_get_root:
 **/
GNode *
cra_dom_get_root (CraDom *dom)
{
	g_return_val_if_fail (CRA_IS_DOM (dom), NULL);
	return dom->priv->root;
}

/**
 * cra_dom_insert:
 * @cdata is not escaped
 **/
GNode *
cra_dom_insert (GNode *parent,
		const gchar *name,
		const gchar *cdata,
		...)
{
	const gchar *key;
	const gchar *value;
	CraDomNodeData *data;
	guint i;
	va_list args;

	data = g_slice_new (CraDomNodeData);
	data->name = g_strdup (name);
	data->cdata = g_string_new (cdata);
	data->cdata_mode = CRA_DOM_DATA_MODE_RAW;
	data->attributes = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  g_free,
						  g_free);

	/* process the attrs valist */
	va_start (args, cdata);
	for (i = 0;; i++) {
		key = va_arg (args, const gchar *);
		if (key == NULL)
			break;
		value = va_arg (args, const gchar *);
		if (value == NULL)
			break;
		g_hash_table_insert (data->attributes,
				     g_strdup (key), g_strdup (value));
	}
	va_end (args);

	return g_node_insert_data (parent, -1, data);
}


/**
 * cra_dom_list_sort_cb:
 **/
static gint
cra_dom_list_sort_cb (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 ((const gchar *) a, (const gchar *) b);
}

/**
 * cra_dom_insert_localized:
 **/
void
cra_dom_insert_localized (GNode *parent,
			  const gchar *name,
			  GHashTable *localized,
			  gboolean escape_xml)
{
	const gchar *key;
	const gchar *value;
	CraDomNodeData *data;
	GList *l;
	GList *list;

	list = g_hash_table_get_keys (localized);
	list = g_list_sort (list, cra_dom_list_sort_cb);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		value = g_hash_table_lookup (localized, key);
		data = g_slice_new (CraDomNodeData);
		data->name = g_strdup (name);
		data->cdata = g_string_new (value);
		data->cdata_mode = escape_xml ? CRA_DOM_DATA_MODE_RAW :
						CRA_DOM_DATA_MODE_ESCAPED;
		data->attributes = g_hash_table_new_full (g_str_hash,
							  g_str_equal,
							  g_free,
							  g_free);
		if (g_strcmp0 (key, "C") != 0) {
			g_hash_table_insert (data->attributes,
					     g_strdup ("xml:lang"),
					     g_strdup (key));
		}
		g_node_insert_data (parent, -1, data);
	}
	g_list_free (list);
}

/**
 * cra_dom_insert_hash:
 **/
void
cra_dom_insert_hash (GNode *parent,
		     const gchar *name,
		     const gchar *attr_key,
		     GHashTable *hash,
		     gboolean swapped)
{
	const gchar *key;
	const gchar *value;
	CraDomNodeData *data;
	GList *l;
	GList *list;

	list = g_hash_table_get_keys (hash);
	list = g_list_sort (list, cra_dom_list_sort_cb);
	for (l = list; l != NULL; l = l->next) {
		key = l->data;
		value = g_hash_table_lookup (hash, key);
		data = g_slice_new (CraDomNodeData);
		data->name = g_strdup (name);
		data->cdata = g_string_new (!swapped ? value : key);
		data->cdata_mode = CRA_DOM_DATA_MODE_RAW;
		data->attributes = g_hash_table_new_full (g_str_hash,
							  g_str_equal,
							  g_free,
							  g_free);
		if (!swapped) {
			if (key != NULL && key[0] != '\0') {
				g_hash_table_insert (data->attributes,
						     g_strdup (attr_key),
						     g_strdup (key));
			}
		} else {
			if (value != NULL && value[0] != '\0') {
				g_hash_table_insert (data->attributes,
						     g_strdup (attr_key),
						     g_strdup (value));
			}
		}
		g_node_insert_data (parent, -1, data);
	}
	g_list_free (list);
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
		data_localized = data->cdata->str;
		if (xml_lang != NULL && g_strcmp0 (data_unlocalized, data_localized) == 0)
			continue;
		g_hash_table_insert (hash,
				     g_strdup (xml_lang != NULL ? xml_lang : "C"),
				     g_strdup (data_localized));
	}
out:
	return hash;
}

/**
 * _g_string_unref:
 **/
static void
_g_string_unref (GString *string)
{
	g_string_free (string, TRUE);
}

/**
 * cd_dom_denorm_add_to_langs:
 **/
static void
cd_dom_denorm_add_to_langs (GHashTable *hash,
			    const gchar *data,
			    gboolean is_start)
{
	const gchar *xml_lang;
	GList *keys;
	GList *l;
	GString *str;

	keys = g_hash_table_get_keys (hash);
	for (l = keys; l != NULL; l = l->next) {
		xml_lang = l->data;
		str = g_hash_table_lookup (hash, xml_lang);
		if (is_start)
			g_string_append_printf (str, "<%s>", data);
		else
			g_string_append_printf (str, "</%s>", data);
	}
	g_list_free (keys);
}

/**
 * cd_dom_denorm_get_str_for_lang:
 **/
static GString *
cd_dom_denorm_get_str_for_lang (GHashTable *hash,
				CraDomNodeData *data,
				gboolean allow_new_locales)
{
	const gchar *xml_lang;
	GString *str;

	/* get locale */
	xml_lang = g_hash_table_lookup (data->attributes, "xml:lang");
	if (xml_lang == NULL)
		xml_lang = "C";
	str = g_hash_table_lookup (hash, xml_lang);
	if (str == NULL && allow_new_locales) {
		str = g_string_new ("");
		g_hash_table_insert (hash, g_strdup (xml_lang), str);
	}
	return str;
}

/**
 * cra_dom_denorm_to_xml_localized:
 *
 * Denormalize AppData data like this:
 *
 * <description>
 *  <p>Hi</p>
 *  <p xml:lang="pl">Czesc</p>
 *  <ul>
 *   <li>First</li>
 *   <li xml:lang="pl">Pierwszy</li>
 *  </ul>
 * </description>
 *
 * into a hash that contains:
 *
 * "C"  ->  "<p>Hi</p><ul><li>First</li></ul>"
 * "pl" ->  "<p>Czesc</p><ul><li>Pierwszy</li></ul>"
 **/
GHashTable *
cra_dom_denorm_to_xml_localized (const GNode *node, GError **error)
{
	GNode *tmp;
	GNode *tmp_c;
	CraDomNodeData *data;
	CraDomNodeData *data_c;
	const gchar *xml_lang;
	GHashTable *hash;
	GString *str;
	GList *keys = NULL;
	GList *l;
	GHashTable *results = NULL;

	hash = g_hash_table_new_full (g_str_hash, g_str_equal,
				      g_free, (GDestroyNotify) _g_string_unref);
	for (tmp = node->children; tmp != NULL; tmp = tmp->next) {
		data = tmp->data;

		/* append to existing string, adding the locale if it's not
		 * already present */
		if (g_strcmp0 (data->name, "p") == 0) {
			str = cd_dom_denorm_get_str_for_lang (hash, data, TRUE);
			cra_dom_cdata_to_escaped (data);
			g_string_append_printf (str, "<p>%s</p>",
						data->cdata->str);

		/* loop on the children */
		} else if (g_strcmp0 (data->name, "ul") == 0 ||
			   g_strcmp0 (data->name, "ol") == 0) {
			cd_dom_denorm_add_to_langs (hash, data->name, TRUE);
			for (tmp_c = tmp->children; tmp_c != NULL; tmp_c = tmp_c->next) {
				data_c = tmp_c->data;

				/* we can only add local text for languages
				 * already added in paragraph text */
				str = cd_dom_denorm_get_str_for_lang (hash,
								      data_c,
								      FALSE);
				if (str == NULL) {
					g_set_error (error,
						     CRA_PLUGIN_ERROR,
						     CRA_PLUGIN_ERROR_FAILED,
						     "Cannot add locale for <%s>",
						     data_c->name);
					goto out;
				}
				if (g_strcmp0 (data_c->name, "li") == 0) {
					cra_dom_cdata_to_escaped (data_c);
					g_string_append_printf (str,
								"<li>%s</li>",
								data_c->cdata->str);
				} else {
					/* only <li> is valid in lists */
					g_set_error (error,
						     CRA_PLUGIN_ERROR,
						     CRA_PLUGIN_ERROR_FAILED,
						     "Tag %s in %s invalid",
						     data_c->name, data->name);
					goto out;
				}
			}
			cd_dom_denorm_add_to_langs (hash, data->name, FALSE);
		} else {
			/* only <p>, <ul> and <ol> is valid here */
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Unknown tag '%s'",
				     data->name);
			goto out;
		}
	}

	/* copy into a hash table of the correct size */
	results = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	keys = g_hash_table_get_keys (hash);
	for (l = keys; l != NULL; l = l->next) {
		xml_lang = l->data;
		str = g_hash_table_lookup (hash, xml_lang);
		g_hash_table_insert (results,
				     g_strdup (xml_lang),
				     g_strdup (str->str));
	}
out:
	g_list_free (keys);
	g_hash_table_unref (hash);
	return results;
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
	g_string_free (data->cdata, TRUE);
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
