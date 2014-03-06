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

#ifndef __CRA_DOM_H
#define __CRA_DOM_H

#include <glib-object.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define CRA_TYPE_DOM		(cra_dom_get_type ())
#define CRA_DOM(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), CRA_TYPE_DOM, CraDom))
#define CRA_DOM_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), CRA_TYPE_DOM, CraDomClass))
#define CRA_IS_DOM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), CRA_TYPE_DOM))
#define CRA_IS_DOM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), CRA_TYPE_DOM))
#define CRA_DOM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), CRA_TYPE_DOM, CraDomClass))

typedef struct _CraDomPrivate CraDomPrivate;

typedef struct
{
	 GObject		 parent;
	 CraDomPrivate		*priv;
} CraDom;

typedef struct
{
	GObjectClass		 parent_class;
} CraDomClass;

GType		 cra_dom_get_type			(void);
CraDom		*cra_dom_new				(void);
GString		*cra_dom_to_xml				(CraDom		*dom,
							 const GNode	*node,
							 gboolean	 add_header);
gboolean	 cra_dom_parse_xml_data			(CraDom		*dom,
							 const gchar	*data,
							 gssize		 data_len,
							 GError		**error)
							 G_GNUC_WARN_UNUSED_RESULT;
GNode		*cra_dom_get_node			(CraDom		*dom,
							 GNode		*root,
							 const gchar	*path)
							 G_GNUC_WARN_UNUSED_RESULT;
GNode		*cra_dom_get_root			(CraDom		*dom)
							 G_GNUC_WARN_UNUSED_RESULT;
GNode		*cra_dom_insert				(GNode		*parent,
							 const gchar	*name,
							 const gchar	*cdata,
							 ...)
							 G_GNUC_NULL_TERMINATED;
void		 cra_dom_insert_localized		(GNode		*parent,
							 const gchar	*name,
							 GHashTable	*localized);
void		 cra_dom_insert_hash			(GNode		*parent,
							 const gchar	*name,
							 const gchar	*attr_key,
							 GHashTable	*hash,
							 gboolean	 swapped);
const gchar	*cra_dom_get_node_name			(const GNode	*node);
const gchar	*cra_dom_get_node_data			(const GNode	*node);
const gchar	*cra_dom_get_node_attribute		(const GNode	*node,
							 const gchar	*key);
GHashTable	*cra_dom_get_node_localized		(const GNode	*node,
							 const gchar	*key);

G_END_DECLS

#endif /* __CRA_DOM_H */

