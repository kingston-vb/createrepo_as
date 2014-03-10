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

#ifndef CRA_RELEASE_H
#define CRA_RELEASE_H

#include <glib-object.h>

#include "cra-package.h"

#define CRA_TYPE_RELEASE		(cra_release_get_type())
#define CRA_RELEASE(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_RELEASE, CraRelease))
#define CRA_RELEASE_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_RELEASE, CraReleaseClass))
#define CRA_IS_RELEASE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_RELEASE))
#define CRA_IS_RELEASE_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_RELEASE))
#define CRA_RELEASE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_RELEASE, CraReleaseClass))

G_BEGIN_DECLS

typedef struct _CraRelease	CraRelease;
typedef struct _CraReleaseClass	CraReleaseClass;

struct _CraRelease
{
	GObject			parent;
};

struct _CraReleaseClass
{
	GObjectClass		parent_class;
};

GType		 cra_release_get_type		(void);
CraRelease	*cra_release_new		(void);
void		 cra_release_set_version	(CraRelease	*release,
						 const gchar	*version);
void		 cra_release_set_timestamp	(CraRelease	*release,
						 guint64	 timestamp);
void		 cra_release_add_text		(CraRelease	*release,
						 const gchar	*text);
const gchar	*cra_release_get_version	(CraRelease	*release);
const gchar	*cra_release_get_text		(CraRelease	*release);
guint64		 cra_release_get_timestamp	(CraRelease	*release);

G_END_DECLS

#endif /* CRA_RELEASE_H */
