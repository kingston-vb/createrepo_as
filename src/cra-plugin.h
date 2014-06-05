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

#ifndef __CRA_PLUGIN_H
#define __CRA_PLUGIN_H

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

#include "cra-app.h"
#include "cra-cleanup.h"
#include "cra-package.h"
#include "cra-utils.h"

G_BEGIN_DECLS

typedef struct	CraPluginPrivate	CraPluginPrivate;
typedef struct	CraPlugin		CraPlugin;

struct CraPlugin {
	GModule			*module;
	gboolean		 enabled;
	gboolean		 is_native;
	gchar			*name;
	CraPluginPrivate	*priv;
};

typedef enum {
	CRA_PLUGIN_ERROR_FAILED,
	CRA_PLUGIN_ERROR_NOT_SUPPORTED,
	CRA_PLUGIN_ERROR_LAST
} CraPluginError;

/* helpers */
#define	CRA_PLUGIN_ERROR				1
#define	CRA_PLUGIN_GET_PRIVATE(x)			g_new0 (x,1)
#define	CRA_PLUGIN(x)					((CraPlugin *) x);

typedef const gchar	*(*CraPluginGetNameFunc)	(void);
typedef void		 (*CraPluginFunc)		(CraPlugin	*plugin);
typedef void		 (*CraPluginGetGlobsFunc)	(CraPlugin	*plugin,
							 GPtrArray	*globs);
typedef void		 (*CraPluginMergeFunc)		(CraPlugin	*plugin,
							 GList		**apps);
typedef gboolean	 (*CraPluginCheckFilenameFunc)	(CraPlugin	*plugin,
							 const gchar	*filename);
typedef GList		*(*CraPluginProcessFunc)	(CraPlugin	*plugin,
							 CraPackage 	*pkg,
							 const gchar	*tmp_dir,
							 GError		**error);
typedef gboolean	 (*CraPluginProcessAppFunc)	(CraPlugin	*plugin,
							 CraPackage 	*pkg,
							 CraApp 	*app,
							 const gchar	*tmpdir,
							 GError		**error);

const gchar	*cra_plugin_get_name			(void);
void		 cra_plugin_initialize			(CraPlugin	*plugin);
void		 cra_plugin_destroy			(CraPlugin	*plugin);
void		 cra_plugin_set_enabled			(CraPlugin	*plugin,
							 gboolean	 enabled);
GList		*cra_plugin_process			(CraPlugin	*plugin,
							 CraPackage	*pkg,
							 const gchar	*tmp_dir,
							 GError		**error);
void		 cra_plugin_add_globs			(CraPlugin	*plugin,
							 GPtrArray	*globs);
void		 cra_plugin_merge			(CraPlugin	*plugin,
							 GList		**list);
gboolean	 cra_plugin_process_app			(CraPlugin	*plugin,
							 CraPackage	*pkg,
							 CraApp		*app,
							 const gchar	*tmp_dir,
							 GError		**error);
gboolean	 cra_plugin_check_filename		(CraPlugin	*plugin,
							 const gchar	*filename);
void		 cra_plugin_add_app			(GList		**list,
							 CraApp		*app);
void		 cra_plugin_add_glob			(GPtrArray	*array,
							 const gchar	*glob);

G_END_DECLS

#endif /* __CRA_PLUGIN_H */
