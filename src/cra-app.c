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

#include "config.h"

#include <stdlib.h>
#include <appstream-glib.h>

#include "cra-app.h"

typedef struct _CraAppPrivate	CraAppPrivate;
struct _CraAppPrivate
{
	GPtrArray	*vetos;
	gboolean	 requires_appdata;
	GdkPixbuf	*pixbuf;
	CraPackage	*pkg;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraApp, cra_app, AS_TYPE_APP)

#define GET_PRIVATE(o) (cra_app_get_instance_private (o))

/**
 * cra_app_finalize:
 **/
static void
cra_app_finalize (GObject *object)
{
	CraApp *app = CRA_APP (object);
	CraAppPrivate *priv = GET_PRIVATE (app);

	g_ptr_array_unref (priv->vetos);
	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);
	if (priv->pkg != NULL)
		g_object_unref (priv->pkg);

	G_OBJECT_CLASS (cra_app_parent_class)->finalize (object);
}

/**
 * cra_app_init:
 **/
static void
cra_app_init (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->vetos = g_ptr_array_new_with_free_func (g_free);
}

/**
 * cra_app_class_init:
 **/
static void
cra_app_class_init (CraAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_app_finalize;
}

/**
 * cra_app_to_xml:
 **/
gchar *
cra_app_to_xml (CraApp *app)
{
	AsStore *store;
	GString *str;

	store = as_store_new ();
	as_store_add_app (store, AS_APP (app));
	str = as_store_to_xml (store,
			       AS_NODE_TO_XML_FLAG_FORMAT_INDENT |
			       AS_NODE_TO_XML_FLAG_FORMAT_MULTILINE);
	g_object_unref (store);
	return g_string_free (str, FALSE);
}

/**
 * cra_app_add_veto:
 **/
void
cra_app_add_veto (CraApp *app, const gchar *fmt, ...)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	gchar *tmp;
	va_list args;
	va_start (args, fmt);
	tmp = g_strdup_vprintf (fmt, args);
	va_end (args);
	g_ptr_array_add (priv->vetos, tmp);
}

/**
 * cra_app_set_requires_appdata:
 **/
void
cra_app_set_requires_appdata (CraApp *app, gboolean requires_appdata)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	priv->requires_appdata = requires_appdata;
}

/**
 * cra_app_set_pixbuf:
 **/
void
cra_app_set_pixbuf (CraApp *app, GdkPixbuf *pixbuf)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	if (priv->pixbuf != NULL)
		g_object_ref (priv->pixbuf);
	priv->pixbuf = g_object_ref (pixbuf);

	/* does the icon not have an alpha channel */
	if (!gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "icon does not have an alpha channel");
	}
}

/**
 * cra_app_get_requires_appdata:
 **/
gboolean
cra_app_get_requires_appdata (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->requires_appdata;
}

/**
 * cra_app_get_vetos:
 **/
GPtrArray *
cra_app_get_vetos (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->vetos;
}

/**
 * cra_app_get_package:
 **/
CraPackage *
cra_app_get_package (CraApp *app)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	return priv->pkg;
}

/**
 * cra_app_save_resources:
 **/
gboolean
cra_app_save_resources (CraApp *app, GError **error)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	CraScreenshot *ss;
	const gchar *tmpdir;
	gboolean ret = TRUE;
	guint i;
	gchar *filename = NULL;
	GPtrArray *screenshots;

	/* any non-stock icon set */
	if (priv->pixbuf != NULL) {
		tmpdir = cra_package_get_config (priv->pkg, "TempDir");
		filename = g_build_filename (tmpdir,
					     "icons",
					     as_app_get_icon (AS_APP (app)),
					     NULL);
		ret = gdk_pixbuf_save (priv->pixbuf,
				       filename,
				       "png",
				       error,
				       NULL);
		if (!ret)
			goto out;

		/* set new AppStream compatible icon name */
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "Saved icon %s", filename);
	}

	/* save any screenshots */
	screenshots = as_app_get_screenshots (AS_APP (app));
	for (i = 0; i < screenshots->len; i++) {
		ss = g_ptr_array_index (screenshots, i);
		ret = cra_screenshot_save_resources (ss, error);
		if (!ret)
			goto out;
	}
out:
	g_free (filename);
	return ret;
}

/**
 * cra_app_new:
 **/
CraApp *
cra_app_new (CraPackage *pkg, const gchar *id_full)
{
	CraApp *app;
	CraAppPrivate *priv;

	app = g_object_new (CRA_TYPE_APP, NULL);
	priv = GET_PRIVATE (app);
	if (pkg != NULL) {
		priv->pkg = g_object_ref (pkg);
		as_app_add_pkgname (AS_APP (app),
				    cra_package_get_name (pkg), -1);
	}
	if (id_full != NULL)
		as_app_set_id_full (AS_APP (app), id_full, -1);
	return CRA_APP (app);
}
