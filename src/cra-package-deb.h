/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef CRA_PACKAGE_DEB_H
#define CRA_PACKAGE_DEB_H

#include <glib-object.h>

#include <stdarg.h>
#include <appstream-glib.h>

#include "cra-package.h"

#define CRA_TYPE_PACKAGE_DEB		(cra_package_deb_get_type())
#define CRA_PACKAGE_DEB(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_PACKAGE_DEB, CraPackageDeb))
#define CRA_PACKAGE_DEB_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_PACKAGE_DEB, CraPackageDebClass))
#define CRA_IS_PACKAGE_DEB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_PACKAGE_DEB))
#define CRA_IS_PACKAGE_DEB_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_PACKAGE_DEB))
#define CRA_PACKAGE_DEB_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_PACKAGE_DEB, CraPackageDebClass))

G_BEGIN_DECLS

typedef struct _CraPackageDeb		CraPackageDeb;
typedef struct _CraPackageDebClass	CraPackageDebClass;

struct _CraPackageDeb
{
	CraPackage			parent;
};

struct _CraPackageDebClass
{
	CraPackageClass			parent_class;
};

GType		 cra_package_deb_get_type	(void);

CraPackage	*cra_package_deb_new		(void);

G_END_DECLS

#endif /* CRA_PACKAGE_DEB_H */
