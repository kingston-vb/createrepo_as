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

#include <stdarg.h>
#include <glib-object.h>
#include <appstream-glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cra-package.h"

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
	AsApp			parent;
};

struct _CraAppClass
{
	AsAppClass		parent_class;
};

GType		 cra_app_get_type		(void);


CraApp		*cra_app_new			(CraPackage	*pkg,
						 const gchar	*id_full);
gchar		*cra_app_to_xml			(CraApp		*app);
void		 cra_app_add_veto		(CraApp		*app,
						 const gchar	*fmt,
						 ...)
						 G_GNUC_PRINTF(2,3);
void		 cra_app_add_requires_appdata	(CraApp		*app,
						 const gchar	*fmt,
						 ...)
						 G_GNUC_PRINTF(2,3);
void		 cra_app_set_requires_appdata	(CraApp		*app,
						 gboolean	 requires_appdata);
void		 cra_app_set_pixbuf		(CraApp		*app,
						 GdkPixbuf	*pixbuf);
gboolean	 cra_app_add_screenshot_source	(CraApp		*app,
						 const gchar	*filename,
						 GError		**error);

GPtrArray	*cra_app_get_requires_appdata	(CraApp		*app);
GPtrArray	*cra_app_get_vetos		(CraApp		*app);
CraPackage	*cra_app_get_package		(CraApp		*app);

gboolean	 cra_app_save_resources		(CraApp		*app,
						 GError		**error);


G_END_DECLS

#endif /* CRA_APP_H */
