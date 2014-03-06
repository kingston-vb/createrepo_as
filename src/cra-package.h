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

#ifndef CRA_PACKAGE_H
#define CRA_PACKAGE_H

#include <glib-object.h>

#include <stdarg.h>

#define CRA_TYPE_PACKAGE		(cra_package_get_type())
#define CRA_PACKAGE(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_PACKAGE, CraPackage))
#define CRA_PACKAGE_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_PACKAGE, CraPackageClass))
#define CRA_IS_PACKAGE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_PACKAGE))
#define CRA_IS_PACKAGE_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_PACKAGE))
#define CRA_PACKAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_PACKAGE, CraPackageClass))

G_BEGIN_DECLS

typedef struct _CraPackage		CraPackage;
typedef struct _CraPackageClass		CraPackageClass;

struct _CraPackage
{
	GObject			parent;
};

struct _CraPackageClass
{
	GObjectClass		parent_class;
};

typedef enum {
	CRA_PACKAGE_LOG_LEVEL_NONE,
	CRA_PACKAGE_LOG_LEVEL_INFO,
	CRA_PACKAGE_LOG_LEVEL_WARNING,
	CRA_PACKAGE_LOG_LEVEL_LAST,
} CraPackageLogLevel;

GType		 cra_package_get_type		(void);

CraPackage	*cra_package_new		(void);
void		 cra_package_log_start		(CraPackage	*pkg);
void		 cra_package_log		(CraPackage	*pkg,
						 CraPackageLogLevel log_level,
						 const gchar	*fmt,
						 ...)
						 G_GNUC_PRINTF (3, 4);
gboolean	 cra_package_log_flush		(CraPackage	*pkg,
						 GError		**error);
gboolean	 cra_package_open		(CraPackage	*pkg,
						 const gchar	*filename,
						 GError		**error);
gboolean	 cra_package_explode		(CraPackage	*pkg,
						 const gchar	*dir,
						 GPtrArray	*glob,
						 GError		**error);
gboolean	 cra_package_ensure_filelist	(CraPackage	*pkg,
						 GError		**error);
const gchar	*cra_package_get_filename	(CraPackage	*pkg);
const gchar	*cra_package_get_name		(CraPackage	*pkg);
const gchar	*cra_package_get_nevr		(CraPackage	*pkg);
const gchar	*cra_package_get_url		(CraPackage	*pkg);
const gchar	*cra_package_get_license	(CraPackage	*pkg);
const gchar	*cra_package_get_sourcerpm	(CraPackage	*pkg);
gchar		**cra_package_get_filelist	(CraPackage	*pkg);
gchar		**cra_package_get_deps		(CraPackage	*pkg);
void		 cra_package_set_config		(CraPackage	*pkg,
						 const gchar	*key,
						 const gchar	*value);
const gchar	*cra_package_get_config		(CraPackage	*pkg,
						 const gchar	*key);

G_END_DECLS

#endif /* CRA_PACKAGE_H */
