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

#ifndef __CRA_CONTEXT_H
#define __CRA_CONTEXT_H

#include <glib.h>

#include "cra-app.h"
#include "cra-package.h"

G_BEGIN_DECLS

typedef struct {
	GPtrArray	*blacklisted_pkgs;	/* of CraGlobValue */
	GPtrArray	*blacklisted_ids;	/* of CraGlobValue */
	GPtrArray	*extra_pkgs;		/* of CraGlobValue */
	GPtrArray	*plugins;		/* of CraPlugin */
	GPtrArray	*packages;		/* of CraPackage */
	GPtrArray	*file_globs;		/* of CraPackage */
	GList		*apps;			/* of CraApp */
	GMutex		 apps_mutex;		/* for ->apps */
	gboolean	 no_net;
	gdouble		 api_version;
} CraContext;

CraContext	*cra_context_new		(void);
void		 cra_context_free		(CraContext	*ctx);
CraPackage	*cra_context_find_by_pkgname	(CraContext	*ctx,
						 const gchar 	*pkgname);
void		 cra_context_add_app		(CraContext	*ctx,
						 CraApp		*app);

G_END_DECLS

#endif /* __CRA_CONTEXT_H */
