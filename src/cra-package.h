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

#ifndef __CRA_PACKAGE_H
#define __CRA_PACKAGE_H

#include <glib.h>
#include <rpm/rpmlib.h>

G_BEGIN_DECLS

typedef struct {
	Header		 h;
	gchar		**filelist;
	gchar		*filename;
	gchar		*name;
	guint		 epoch;
	gchar		*version;
	gchar		*release;
	gchar		*arch;
	gchar		*url;
} CraPackage;

CraPackage	*cra_package_open		(const gchar	*filename,
						 GError		**error);
void		 cra_package_free		(CraPackage	*pkg);
gboolean	 cra_package_explode		(CraPackage	*pkg,
						 const gchar	*dir,
						 GError		**error);
gboolean	 cra_package_ensure_filelist	(CraPackage	*pkg,
						 GError		**error);

G_END_DECLS

#endif /* __CRA_PACKAGE_H */
