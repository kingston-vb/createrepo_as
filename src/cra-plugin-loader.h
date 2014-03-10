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

#ifndef __CRA_PLUGIN_LOADER_H
#define __CRA_PLUGIN_LOADER_H

#include <glib.h>

G_BEGIN_DECLS

gboolean	 cra_plugin_loader_setup	(GPtrArray	*plugins,
						 GError		**error);
GPtrArray	*cra_plugin_loader_get_globs	(GPtrArray	*plugins);
void		 cra_plugin_loader_merge	(GPtrArray	*plugins,
						 GList		**apps);
gboolean	 cra_plugin_loader_process_app	(GPtrArray	*plugins,
						 CraPackage	*pkg,
						 CraApp		*app,
						 const gchar	*tmpdir,
						 GError		**error);
GPtrArray	*cra_plugin_loader_new		(void);
void		 cra_plugin_loader_free		(GPtrArray	*plugins);
CraPlugin	*cra_plugin_loader_match_fn	(GPtrArray	*plugins,
						 const gchar	*filename);

G_END_DECLS

#endif /* __CRA_PLUGIN_LOADER_H */
