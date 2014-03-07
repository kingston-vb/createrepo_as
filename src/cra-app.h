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

#ifndef CRA_APP_H
#define CRA_APP_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cra-package.h"
#include "cra-screenshot.h"

#define CRA_TYPE_APP		(cra_app_get_type())
#define CRA_APP(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_APP, CraApp))
#define CRA_APP_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_APP, CraAppClass))
#define CRA_IS_APP(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_APP))
#define CRA_IS_APP_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_APP))
#define CRA_APP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_APP, CraAppClass))

G_BEGIN_DECLS

typedef struct _CraApp		CraApp;
typedef struct _CraAppClass	CraAppClass;

struct _CraApp
{
	GObject			parent;
};

struct _CraAppClass
{
	GObjectClass		parent_class;
};

typedef enum {
	CRA_APP_ICON_TYPE_STOCK,
	CRA_APP_ICON_TYPE_CACHED,
	CRA_APP_ICON_TYPE_LAST
} CraAppIconType;

GType		 cra_app_get_type		(void);
CraApp		*cra_app_new			(CraPackage	*pkg,
						 const gchar	*id_full);
gchar		*cra_app_to_xml			(CraApp		*app);
void		 cra_app_set_type_id		(CraApp		*app,
						 const gchar	*type_id);
void		 cra_app_set_homepage_url	(CraApp		*app,
						 const gchar	*homepage_url);
void		 cra_app_set_project_group	(CraApp		*app,
						 const gchar	*project_group);
void		 cra_app_set_project_license	(CraApp		*app,
						 const gchar	*project_license);
void		 cra_app_set_compulsory_for_desktop (CraApp	*app,
						 const gchar	*compulsory_for_desktop);
void		 cra_app_set_icon		(CraApp		*app,
						 const gchar	*icon);
void		 cra_app_add_category		(CraApp		*app,
						 const gchar	*category);
void		 cra_app_add_keyword		(CraApp		*app,
						 const gchar	*keyword);
void		 cra_app_add_mimetype		(CraApp		*app,
						 const gchar	*mimetype);
void		 cra_app_add_pkgname		(CraApp		*app,
						 const gchar	*pkgname);
void		 cra_app_add_screenshot		(CraApp		*app,
						 CraScreenshot	*screenshot);
void		 cra_app_add_language		(CraApp		*app,
						 const gchar	*locale,
						 const gchar	*value);
void		 cra_app_add_metadata		(CraApp		*app,
						 const gchar	*key,
						 const gchar	*value);
void		 cra_app_set_name		(CraApp		*app,
						 const gchar	*locale,
						 const gchar	*name);
void		 cra_app_set_comment		(CraApp		*app,
						 const gchar	*locale,
						 const gchar	*comment);
void		 cra_app_set_requires_appdata	(CraApp		*app,
						 gboolean	 requires_appdata);
void		 cra_app_set_icon_type		(CraApp		*app,
						 CraAppIconType	 icon_type);
void		 cra_app_set_pixbuf		(CraApp		*app,
						 GdkPixbuf	*pixbuf);

gboolean	 cra_app_get_requires_appdata	(CraApp		*app);
GPtrArray	*cra_app_get_categories		(CraApp		*app);
GPtrArray	*cra_app_get_keywords		(CraApp		*app);
GPtrArray	*cra_app_get_screenshots	(CraApp		*app);
const gchar	*cra_app_get_id			(CraApp		*app);
const gchar	*cra_app_get_id_full		(CraApp		*app);
const gchar	*cra_app_get_type_id		(CraApp		*app);
const gchar	*cra_app_get_icon		(CraApp		*app);
const gchar	*cra_app_get_project_group	(CraApp		*app);
const gchar	*cra_app_get_name		(CraApp		*app,
						 const gchar	*locale);
const gchar	*cra_app_get_comment		(CraApp		*app,
						 const gchar	*locale);
const gchar	*cra_app_get_metadata_item	(CraApp		*app,
						 const gchar	*key);
CraPackage	*cra_app_get_package		(CraApp		*app);

void		 cra_app_insert_into_dom	(CraApp		*app,
						 GNode		*parent);
gboolean	 cra_app_save_resources		(CraApp		*app,
						 GError		**error);

G_END_DECLS

#endif /* CRA_APP_H */
