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

#ifndef __CRA_APP_H
#define __CRA_APP_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gchar		*type_id;
	gchar		*project_group;
	gchar		*homepage_url;
	gchar		*app_id;
	gchar		*icon;
	GPtrArray	*categories;
	GPtrArray	*keywords;
	GPtrArray	*mimetypes;
	gboolean	 requires_appdata;
	gboolean	 cached_icon;
	GHashTable	*names;
	GHashTable	*comments;
	GHashTable	*languages;
	GHashTable	*metadata;
} CraApp;

CraApp		*cra_app_new			(const gchar	*app_id);
void		 cra_app_free			(CraApp		*app);
void		 cra_app_print			(CraApp		*app);
void		 cra_app_set_type_id		(CraApp		*app,
						 const gchar	*type_id);
void		 cra_app_set_homepage_url	(CraApp		*app,
						 const gchar	*homepage_url);
void		 cra_app_set_project_group	(CraApp		*app,
						 const gchar	*project_group);
void		 cra_app_set_icon		(CraApp		*app,
						 const gchar	*icon);
void		 cra_app_add_category		(CraApp		*app,
						 const gchar	*category);
void		 cra_app_add_keyword		(CraApp		*app,
						 const gchar	*keyword);
void		 cra_app_add_mimetype		(CraApp		*app,
						 const gchar	*mimetype);
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
void		 cra_app_set_cached_icon	(CraApp		*app,
						 gboolean	 cached_icon);

G_END_DECLS

#endif /* __CRA_APP_H */
