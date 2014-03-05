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

#include <libsoup/soup.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cra-dom.h"
#include "cra-plugin.h"
#include "cra-screenshot.h"

typedef struct _CraScreenshotPrivate	CraScreenshotPrivate;
struct _CraScreenshotPrivate
{
	gchar		*app_id;
	gchar		*cache_filename;
	gchar		*basename;
	gchar		*caption;
	guint		 width;
	guint		 height;
	SoupSession	*session;
	gboolean	 is_default;
	GdkPixbuf	*pixbuf;
	CraPackage	*pkg;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraScreenshot, cra_screenshot, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_screenshot_get_instance_private (o))

/**
 * cra_screenshot_finalize:
 **/
static void
cra_screenshot_finalize (GObject *object)
{
	CraScreenshot *screenshot = CRA_SCREENSHOT (object);
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);

	g_free (priv->app_id);
	g_free (priv->cache_filename);
	g_free (priv->basename);
	g_free (priv->caption);
	g_object_unref (priv->session);
	g_object_unref (priv->pkg);
	if (priv->pixbuf != NULL)
		g_object_unref (priv->pixbuf);

	G_OBJECT_CLASS (cra_screenshot_parent_class)->finalize (object);
}

/**
 * cra_screenshot_init:
 **/
static void
cra_screenshot_init (CraScreenshot *screenshot)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	priv->session = soup_session_new_with_options (SOUP_SESSION_USER_AGENT, "createrepo_as",
						       SOUP_SESSION_TIMEOUT, 5000,
						       NULL);
	soup_session_add_feature_by_type (priv->session,
					  SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
}

/**
 * cra_screenshot_insert_into_dom:
 **/
void
cra_screenshot_insert_into_dom (CraScreenshot *screenshot, GNode *parent)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	const gchar *mirror_uri;
	gchar tmp_height[6];
	gchar tmp_width[6];
	gchar *url_tmp;
	GNode *node_ss;

	node_ss = cra_dom_insert (parent, "screenshot", NULL,
				  "type", priv->is_default ? "default" : "normal",
				  NULL);

	/* add caption */
	if (priv->caption != NULL)
		cra_dom_insert (node_ss, "caption", priv->caption, NULL);

	/* add source image */
	g_snprintf (tmp_height, sizeof (tmp_height), "%u", priv->height);
	g_snprintf (tmp_width, sizeof (tmp_width), "%u", priv->width);
	mirror_uri = cra_package_get_config (priv->pkg, "MirrorURI");
	url_tmp = g_build_filename (mirror_uri, "source", priv->basename, NULL);
	cra_dom_insert (node_ss, "image", url_tmp,
			"height", tmp_height,
			"width", tmp_width,
			"type", "source",
			NULL);
	g_free (url_tmp);

	/* add each of the resampled images */
	//FIXME: xxx
}

/**
 * cra_screenshot_get_cache_filename:
 **/
const gchar *
cra_screenshot_get_cache_filename (CraScreenshot *screenshot)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	return priv->cache_filename;
}

/**
 * cra_screenshot_get_caption:
 **/
const gchar *
cra_screenshot_get_caption (CraScreenshot *screenshot)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	return priv->caption;
}

/**
 * cra_screenshot_get_width:
 **/
guint
cra_screenshot_get_width (CraScreenshot *screenshot)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	return priv->width;
}

/**
 * cra_screenshot_get_height:
 **/
guint
cra_screenshot_get_height (CraScreenshot *screenshot)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	return priv->height;
}

/**
 * cra_screenshot_load_filename:
 **/
gboolean
cra_screenshot_load_filename (CraScreenshot *screenshot,
			      const gchar *filename,
			      GError **error)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	gboolean ret = TRUE;
	gchar *data = NULL;
	gchar *md5 = NULL;
	gsize len;

	/* get the contents so we can hash */
	ret = g_file_get_contents (filename, &data, &len, error);
	if (!ret)
		goto out;

	/* load the image */
	priv->pixbuf = gdk_pixbuf_new_from_file (filename, error);
	if (priv->pixbuf == NULL) {
		ret = FALSE;
		goto out;
	}

	/* set */
	priv->width = gdk_pixbuf_get_width (priv->pixbuf);
	priv->height = gdk_pixbuf_get_height (priv->pixbuf);
	md5 = g_compute_checksum_for_data (G_CHECKSUM_MD5, (guchar *) data, len);
	priv->basename = g_strdup_printf ("%s-%s.png", priv->app_id, md5);
	priv->cache_filename = g_strdup (filename);
out:
	g_free (md5);
	g_free (data);
	return ret;
}

/**
 * cra_screenshot_save_resources:
 **/
gboolean
cra_screenshot_save_resources (CraScreenshot *screenshot, GError **error)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	gboolean ret = TRUE;
	gchar *filename = NULL;

	/* any non-stock icon set */
	if (priv->pixbuf == NULL)
		goto out;

	/* save source file at full resolution */
	filename = g_build_filename ("./screenshots",
				     "source",
				     priv->basename,
				     NULL);
	ret = gdk_pixbuf_save (priv->pixbuf,
			       filename,
			       "png",
			       error,
			       NULL);
	if (!ret)
		goto out;

	/* set new AppStream compatible screenshot name */
	cra_package_log (priv->pkg,
			 CRA_PACKAGE_LOG_LEVEL_INFO,
			 "saved screenshots %s", priv->basename);
out:
	g_free (filename);
	return ret;
}

/**
 * cra_screenshot_load_url:
 **/
gboolean
cra_screenshot_load_url (CraScreenshot *screenshot, const gchar *url, GError **error)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	gboolean ret = TRUE;
	gchar *basename;
	gchar *cache_filename;
	SoupMessage *msg = NULL;
	SoupStatus status;
	SoupURI *uri = NULL;

	/* download to cache if not already added */
	basename = g_path_get_basename (url);
	cache_filename = g_strdup_printf ("./screenshot-cache/%s-%s",
					  priv->app_id, basename);
	if (!g_file_test (cache_filename, G_FILE_TEST_EXISTS)) {
		uri = soup_uri_new (url);
		if (uri == NULL) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Could not parse '%s' as a URL", url);
			goto out;
		}
		cra_package_log (priv->pkg,
				 CRA_PACKAGE_LOG_LEVEL_INFO,
				 "Downloading %s", url);
		msg = soup_message_new_from_uri (SOUP_METHOD_GET, uri);
		status = soup_session_send_message (priv->session, msg);
		if (status != SOUP_STATUS_OK) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Downloading failed: %s",
				     soup_status_get_phrase (status));
			goto out;
		}

		/* save new file */
		ret = g_file_set_contents (cache_filename,
					   msg->response_body->data,
					   msg->response_body->length,
					   error);
		if (!ret)
			goto out;
	}

	/* load the pixbuf */
	ret = cra_screenshot_load_filename (screenshot, cache_filename, error);
	if (!ret)
		goto out;
out:
	if (uri != NULL)
		soup_uri_free (uri);
	if (msg != NULL)
		g_object_unref (msg);
	g_free (basename);
	g_free (cache_filename);
	return ret;
}

/**
 * cra_screenshot_set_is_default:
 **/
void
cra_screenshot_set_is_default (CraScreenshot *screenshot, gboolean is_default)
{
	CraScreenshotPrivate *priv = GET_PRIVATE (screenshot);
	priv->is_default = is_default;
}

/**
 * cra_screenshot_class_init:
 **/
static void
cra_screenshot_class_init (CraScreenshotClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_screenshot_finalize;
}

/**
 * cra_screenshot_new:
 **/
CraScreenshot *
cra_screenshot_new (CraPackage *pkg, const gchar *app_id)
{
	CraScreenshot *screenshot;
	CraScreenshotPrivate *priv;
	screenshot = g_object_new (CRA_TYPE_SCREENSHOT, NULL);
	priv = GET_PRIVATE (screenshot);
	priv->pkg = g_object_ref (pkg);
	priv->app_id = g_strdup (app_id);
	return CRA_SCREENSHOT (screenshot);
}
