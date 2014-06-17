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

#ifndef __CRA_UTILS_H
#define __CRA_UTILS_H

#include <glib.h>
#include <appstream-glib.h>

G_BEGIN_DECLS

typedef struct	CraGlobValue		CraGlobValue;

gboolean	 cra_utils_rmtree			(const gchar	*directory,
							 GError		**error);
gboolean	 cra_utils_ensure_exists_and_empty	(const gchar	*directory,
							 GError		**error);
gboolean	 cra_utils_write_archive_dir		(const gchar	*filename,
							 const gchar	*directory,
							 GError		**error);
gboolean	 cra_utils_explode			(const gchar	*filename,
							 const gchar	*dir,
							 GPtrArray	*glob,
							 GError		**error);
gchar		*cra_utils_get_cache_id_for_filename	(const gchar	*filename);

CraGlobValue	*cra_glob_value_new			(const gchar	*glob,
							 const gchar	*value);
void		 cra_glob_value_free			(CraGlobValue	*kv);
const gchar	*cra_glob_value_search			(GPtrArray	*array,
							 const gchar	*search);
GPtrArray	*cra_glob_value_array_new		(void);
guint		 cra_string_replace			(GString	*string,
							 const gchar	*search,
							 const gchar	*replace);

gboolean	 cra_utils_add_apps_from_file		(GList		**apps,
							 const gchar	*filename,
							 GError		**error);
gboolean	 cra_utils_add_apps_from_dir		(GList		**apps,
							 const gchar	*path,
							 GError		**error);

G_END_DECLS

#endif /* __CRA_UTILS_H */
