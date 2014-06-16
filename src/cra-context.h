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

#ifndef CRA_CONTEXT_H
#define CRA_CONTEXT_H

#include <glib-object.h>

#include "cra-app.h"
#include "cra-package.h"

#define CRA_TYPE_CONTEXT		(cra_context_get_type())
#define CRA_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_CONTEXT, CraContext))
#define CRA_CONTEXT_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_CONTEXT, CraContextClass))
#define CRA_IS_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_CONTEXT))
#define CRA_IS_CONTEXT_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_CONTEXT))
#define CRA_CONTEXT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_CONTEXT, CraContextClass))

G_BEGIN_DECLS

typedef struct _CraContext		CraContext;
typedef struct _CraContextClass	CraContextClass;

struct _CraContext
{
	GObject			 parent;
};

struct _CraContextClass
{
	GObjectClass			parent_class;
};

GType		 cra_context_get_type		(void);

CraContext	*cra_context_new		(void);
CraPackage	*cra_context_find_by_pkgname	(CraContext	*ctx,
						 const gchar 	*pkgname);
void		 cra_context_add_app		(CraContext	*ctx,
						 CraApp		*app);
void		 cra_context_set_no_net		(CraContext	*ctx,
						 gboolean	 no_net);
void		 cra_context_set_api_version	(CraContext	*ctx,
						 gdouble	 api_version);
void		 cra_context_set_add_cache_id	(CraContext	*ctx,
						 gboolean	 add_cache_id);
void		 cra_context_set_extra_checks	(CraContext	*ctx,
						 gboolean	 extra_checks);
void		 cra_context_set_use_package_cache (CraContext	*ctx,
						 gboolean	 use_package_cache);
void		 cra_context_set_max_threads	(CraContext	*ctx,
						 guint		 max_threads);
void		 cra_context_set_old_metadata	(CraContext	*ctx,
						 const gchar	*old_metadata);
void		 cra_context_set_extra_appstream (CraContext	*ctx,
						 const gchar	*extra_appstream);
void		 cra_context_set_extra_appdata	(CraContext	*ctx,
						 const gchar	*extra_appdata);
void		 cra_context_set_extra_screenshots (CraContext	*ctx,
						 const gchar	*extra_screenshots);
void		 cra_context_set_screenshot_uri	(CraContext	*ctx,
						 const gchar	*screenshot_uri);
void		 cra_context_set_log_dir	(CraContext	*ctx,
						 const gchar	*log_dir);
void		 cra_context_set_cache_dir	(CraContext	*ctx,
						 const gchar	*cache_dir);
void		 cra_context_set_temp_dir	(CraContext	*ctx,
						 const gchar	*temp_dir);
void		 cra_context_set_output_dir	(CraContext	*ctx,
						 const gchar	*output_dir);
void		 cra_context_set_basename	(CraContext	*ctx,
						 const gchar	*basename);
const gchar	*cra_context_get_temp_dir	(CraContext	*ctx);
gboolean	 cra_context_get_add_cache_id	(CraContext	*ctx);
gboolean	 cra_context_get_extra_checks	(CraContext	*ctx);
gboolean	 cra_context_get_use_package_cache (CraContext	*ctx);

gboolean	 cra_context_setup		(CraContext	*ctx,
						 GError		**error);
gboolean	 cra_context_process		(CraContext	*ctx,
						 GError		**error);
gboolean	 cra_context_add_filename	(CraContext	*ctx,
						 const gchar	*filename,
						 GError		**error);
void		 cra_context_disable_older_pkgs	(CraContext	*ctx);
gboolean	 cra_context_find_in_cache	(CraContext	*ctx,
						 const gchar	*filename);
const gchar	*cra_context_get_extra_package	(CraContext	*ctx,
						 const gchar	*pkgname);

G_END_DECLS

#endif /* CRA_CONTEXT_H */
