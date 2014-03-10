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

#ifndef CRA_SCREENSHOT_H
#define CRA_SCREENSHOT_H

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cra-package.h"

#define CRA_TYPE_SCREENSHOT		(cra_screenshot_get_type())
#define CRA_SCREENSHOT(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_SCREENSHOT, CraScreenshot))
#define CRA_SCREENSHOT_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_SCREENSHOT, CraScreenshotClass))
#define CRA_IS_SCREENSHOT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_SCREENSHOT))
#define CRA_IS_SCREENSHOT_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_SCREENSHOT))
#define CRA_SCREENSHOT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_SCREENSHOT, CraScreenshotClass))

G_BEGIN_DECLS

typedef struct _CraScreenshot		CraScreenshot;
typedef struct _CraScreenshotClass	CraScreenshotClass;

struct _CraScreenshot
{
	GObject			parent;
};

struct _CraScreenshotClass
{
	GObjectClass		parent_class;
};

GType		 cra_screenshot_get_type		(void);
CraScreenshot	*cra_screenshot_new			(CraPackage	*pkg,
							 const gchar	*app_id);
const gchar	*cra_screenshot_get_cache_filename	(CraScreenshot	*screenshot);
const gchar	*cra_screenshot_get_caption		(CraScreenshot	*screenshot);
guint		 cra_screenshot_get_width		(CraScreenshot	*screenshot);
guint		 cra_screenshot_get_height		(CraScreenshot	*screenshot);

void		 cra_screenshot_set_is_default		(CraScreenshot	*screenshot,
							 gboolean	 is_default);
void		 cra_screenshot_set_only_source		(CraScreenshot	*screenshot,
							 gboolean	 only_source);
void		 cra_screenshot_set_caption		(CraScreenshot	*screenshot,
							 const gchar	*caption);
void		 cra_screenshot_set_pixbuf		(CraScreenshot	*screenshot,
							 GdkPixbuf	*pixbuf);
gboolean	 cra_screenshot_load_url		(CraScreenshot	*screenshot,
							 const gchar	*url,
							 GError		**error);
gboolean	 cra_screenshot_load_filename		(CraScreenshot	*screenshot,
							 const gchar	*filename,
							 GError		**error);

void		 cra_screenshot_insert_into_dom		(CraScreenshot	*screenshot,
							GNode		*parent);
gboolean	 cra_screenshot_save_resources		(CraScreenshot	*screenshot,
							 GError		**error);

G_END_DECLS

#endif /* CRA_SCREENSHOT_H */
