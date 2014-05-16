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
 * cra_app_save_resources_image:
 **/
static gboolean
cra_app_save_resources_image (CraApp *app,
			      AsImage *image,
			      GError **error)
{
	const gchar *output_dir;
	gboolean ret = TRUE;
	gchar *filename = NULL;
	gchar *size_str;

	/* treat source images differently */
	if (as_image_get_kind (image) == AS_IMAGE_KIND_SOURCE) {
		size_str = g_strdup ("source");
	} else {
		size_str = g_strdup_printf ("%ix%i",
					    as_image_get_width (image),
					    as_image_get_height (image));
	}

	/* does screenshot already exist */
	output_dir = cra_package_get_config (cra_app_get_package (app), "OutputDir");
	filename = g_build_filename (output_dir,
				     "screenshots",
				     size_str,
				     as_image_get_basename (image),
				     NULL);
	if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_DEBUG,
				 "%s screenshot already exists", size_str);
		goto out;
	}

	/* thumbnails will already be 16:9 */
	ret = as_image_save_filename (image,
				      filename,
				      0, 0,
				      AS_IMAGE_SAVE_FLAG_NONE,
				      error);
	if (!ret)
		goto out;

	/* set new AppStream compatible screenshot name */
	cra_package_log (cra_app_get_package (app),
			 CRA_PACKAGE_LOG_LEVEL_DEBUG,
			 "saved %s screenshot", size_str);
out:
	g_free (filename);
	g_free (size_str);
	return ret;

}

/**
 * cra_app_save_resources_screenshot:
 **/
static gboolean
cra_app_save_resources_screenshot (CraApp *app,
				   AsScreenshot *screenshot,
				   GError **error)
{
	AsImage *im;
	GPtrArray *images;
	gboolean ret = TRUE;
	guint i;

	images = as_screenshot_get_images (screenshot);
	for (i = 0; i < images->len; i++) {
		im = g_ptr_array_index (images, i);
		ret = cra_app_save_resources_image (app, im, error);
		if (!ret)
			goto out;
	}
out:
	return ret;
}

/**
 * cra_app_save_resources:
 **/
gboolean
cra_app_save_resources (CraApp *app, GError **error)
{
	CraAppPrivate *priv = GET_PRIVATE (app);
	AsScreenshot *ss;
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
		ret = cra_app_save_resources_screenshot (app, ss, error);
		if (!ret)
			goto out;
	}
out:
	g_free (filename);
	return ret;
}

/**
 * cra_app_add_screenshot_source:
 **/
gboolean
cra_app_add_screenshot_source (CraApp *app, const gchar *filename, GError **error)
{
	AsImage *im_src;
	AsImage *im_tmp;
	AsScreenshot *ss = NULL;
	gboolean ret;
	gboolean is_default;
	guint sizes[] = { 624, 351, 112, 63, 752, 423, 0 };
	const gchar *mirror_uri;
	gchar *basename = NULL;
	gchar *size_str;
	gchar *url_tmp;
	guint i;
	GdkPixbuf *pixbuf;

	im_src = as_image_new ();
	ret = as_image_load_filename (im_src, filename, error);
	if (!ret)
		goto out;

	/* is the aspect ratio of the source perfectly 16:9 */
	if ((as_image_get_width (im_src) / 16) * 9 !=
	     as_image_get_height (im_src)) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "%s is not in 16:9 aspect ratio",
				 filename);
	}

	ss = as_screenshot_new ();
	is_default = as_app_get_screenshots(AS_APP(app))->len == 0;
	as_screenshot_set_kind (ss, is_default ? AS_SCREENSHOT_KIND_DEFAULT :
						 AS_SCREENSHOT_KIND_NORMAL);

	/* include the app-id in the basename */
	basename = g_strdup_printf ("%s-%s.png",
				    as_app_get_id (AS_APP (app)),
				    as_image_get_md5 (im_src));
	as_image_set_basename (im_src, basename);

	/* only fonts have full sized screenshots */
	mirror_uri = cra_package_get_config (cra_app_get_package (app), "MirrorURI");
	if (as_app_get_id_kind (AS_APP (app)) == AS_ID_KIND_FONT) {
		url_tmp = g_build_filename (mirror_uri,
					    "source",
					    basename,
					    NULL);
		as_image_set_url (im_src, url_tmp, -1);
		as_image_set_kind (im_src, AS_IMAGE_KIND_SOURCE);
		as_screenshot_add_image (ss, im_src);
		g_free (url_tmp);
	} else {
		for (i = 0; sizes[i] != 0; i += 2) {
			size_str = g_strdup_printf ("%ix%i",
						    sizes[i],
						    sizes[i+1]);
			url_tmp = g_build_filename (mirror_uri,
						    size_str,
						    basename,
						    NULL);
			pixbuf = as_image_save_pixbuf (im_src,
						       sizes[i],
						       sizes[i+1],
						       AS_IMAGE_SAVE_FLAG_PAD_16_9);
			im_tmp = as_image_new ();
			as_image_set_width (im_tmp, sizes[i]);
			as_image_set_height (im_tmp, sizes[i+1]);
			as_image_set_url (im_tmp, url_tmp, -1);
			as_image_set_pixbuf (im_tmp, pixbuf);
			as_image_set_kind (im_tmp, AS_IMAGE_KIND_THUMBNAIL);
			as_screenshot_add_image (ss, im_tmp);
			g_free (url_tmp);
			g_free (size_str);
			g_object_unref (im_tmp);
			g_object_unref (pixbuf);
		}
	}
	as_app_add_screenshot (AS_APP (app), ss);
out:
	if (ss != NULL)
		g_object_unref (ss);
	g_free (basename);
	g_object_unref (im_src);
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
