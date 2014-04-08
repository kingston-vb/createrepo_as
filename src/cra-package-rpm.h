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

#ifndef CRA_PACKAGE_RPM_H
#define CRA_PACKAGE_RPM_H

#include <glib-object.h>

#include <stdarg.h>
#include <appstream-glib.h>

#include "cra-package.h"

#define CRA_TYPE_PACKAGE_RPM		(cra_package_rpm_get_type())
#define CRA_PACKAGE_RPM(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_PACKAGE_RPM, CraPackageRpm))
#define CRA_PACKAGE_RPM_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_PACKAGE_RPM, CraPackageRpmClass))
#define CRA_IS_PACKAGE_RPM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_PACKAGE_RPM))
#define CRA_IS_PACKAGE_RPM_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_PACKAGE_RPM))
#define CRA_PACKAGE_RPM_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_PACKAGE_RPM, CraPackageRpmClass))

G_BEGIN_DECLS

typedef struct _CraPackageRpm		CraPackageRpm;
typedef struct _CraPackageRpmClass	CraPackageRpmClass;

struct _CraPackageRpm
{
	CraPackage			parent;
};

struct _CraPackageRpmClass
{
	CraPackageClass			parent_class;
};

GType		 cra_package_rpm_get_type	(void);

CraPackage	*cra_package_rpm_new		(void);

G_END_DECLS

#endif /* CRA_PACKAGE_RPM_H */
