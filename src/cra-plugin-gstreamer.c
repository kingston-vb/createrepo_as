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

#include <config.h>
#include <fnmatch.h>

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "gstreamer";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/lib64/gstreamer-1.0/libgst*.so");
}

/**
 * cra_plugin_check_filename:
 */
gboolean
cra_plugin_check_filename (CraPlugin *plugin, const gchar *filename)
{
	if (fnmatch ("/usr/lib64/gstreamer-1.0/libgst*.so", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

typedef struct {
	const gchar *path;
	const gchar *text;
} CraGstreamerDescData;

static const CraGstreamerDescData data[] = {
	{ "/usr/lib64/gstreamer-1.0/libgsta52dec.so",	"AC-3" },
	{ "/usr/lib64/gstreamer-1.0/libgstaiff.so",	"AIFF" },
	{ "/usr/lib64/gstreamer-1.0/libgstamrnb.so",	"AMR-NB" },
	{ "/usr/lib64/gstreamer-1.0/libgstamrwbdec.so",	"AMR-WB" },
	{ "/usr/lib64/gstreamer-1.0/libgstapetag.so",	"APE" },
	{ "/usr/lib64/gstreamer-1.0/libgstasf.so",	"ASF" },
	{ "/usr/lib64/gstreamer-1.0/libgstavi.so",	"AVI" },
	{ "/usr/lib64/gstreamer-1.0/libgstavidemux.so",	"AVI" },
	{ "/usr/lib64/gstreamer-1.0/libgstdecklink.so",	"SDI" },
	{ "/usr/lib64/gstreamer-1.0/libgstdtsdec.so",	"DTS" },
	{ "/usr/lib64/gstreamer-1.0/libgstdv.so",	"DV" },
	{ "/usr/lib64/gstreamer-1.0/libgstdvb.so",	"DVB" },
	{ "/usr/lib64/gstreamer-1.0/libgstdvdread.so",	"DVD" },
	{ "/usr/lib64/gstreamer-1.0/libgstdvdspu.so",	"Bluray" },
	{ "/usr/lib64/gstreamer-1.0/libgstespeak.so",	"eSpeak" },
	{ "/usr/lib64/gstreamer-1.0/libgstfaad.so",	"MPEG-4|MPEG-2 AAC" },
	{ "/usr/lib64/gstreamer-1.0/libgstflac.so",	"FLAC" },
	{ "/usr/lib64/gstreamer-1.0/libgstflv.so",	"Flash" },
	{ "/usr/lib64/gstreamer-1.0/libgstflxdec.so",	"FLX" },
	{ "/usr/lib64/gstreamer-1.0/libgstgsm.so",	"GSM" },
	{ "/usr/lib64/gstreamer-1.0/libgstid3tag.so",	"ID3" },
	{ "/usr/lib64/gstreamer-1.0/libgstisomp4.so",	"MP4" },
	{ "/usr/lib64/gstreamer-1.0/libgstmad.so",	"MP3" },
	{ "/usr/lib64/gstreamer-1.0/libgstmatroska.so",	"MKV" },
	{ "/usr/lib64/gstreamer-1.0/libgstmfc.so",	"MFC" },
	{ "/usr/lib64/gstreamer-1.0/libgstmidi.so",	"MIDI" },
	{ "/usr/lib64/gstreamer-1.0/libgstmimic.so",	"Mimic" },
	{ "/usr/lib64/gstreamer-1.0/libgstmms.so",	"MMS" },
	{ "/usr/lib64/gstreamer-1.0/libgstmpeg2dec.so",	"MPEG-2" },
	{ "/usr/lib64/gstreamer-1.0/libgstmpg123.so",	"MP3" },
	{ "/usr/lib64/gstreamer-1.0/libgstmxf.so",	"MXF" },
	{ "/usr/lib64/gstreamer-1.0/libgstogg.so",	"Ogg" },
	{ "/usr/lib64/gstreamer-1.0/libgstopus.so",	"Opus" },
	{ "/usr/lib64/gstreamer-1.0/libgstrmdemux.so",	"RealMedia" },
	{ "/usr/lib64/gstreamer-1.0/libgstschro.so",	"Dirac" },
	{ "/usr/lib64/gstreamer-1.0/libgstsiren.so",	"Siren" },
	{ "/usr/lib64/gstreamer-1.0/libgstspeex.so",	"Speex" },
	{ "/usr/lib64/gstreamer-1.0/libgsttheora.so",	"Theora" },
	{ "/usr/lib64/gstreamer-1.0/libgsttwolame.so",	"MP2" },
	{ "/usr/lib64/gstreamer-1.0/libgstvorbis.so",	"Vorbis" },
	{ "/usr/lib64/gstreamer-1.0/libgstvpx.so",	"VP8|VP9" },
	{ "/usr/lib64/gstreamer-1.0/libgstwavenc.so",	"WAV" },
	{ "/usr/lib64/gstreamer-1.0/libgstx264.so",	"H.264/MPEG-4 AVC" },
	{ NULL,		NULL }
};

/**
 * cra_utils_is_file_in_tmpdir:
 */
static gboolean
cra_utils_is_file_in_tmpdir (const gchar *tmpdir, const gchar *filename)
{
	gchar tmp[PATH_MAX];
	g_snprintf (tmp, PATH_MAX, "%s/%s", tmpdir, filename);
	return g_file_test (tmp, G_FILE_TEST_EXISTS);
}

/**
 * cra_utils_string_sort_cb:
 */
static gint
cra_utils_string_sort_cb (gconstpointer a, gconstpointer b)
{
	return g_strcmp0 (*((const gchar **) a), *((const gchar **) b));
}

/**
 * cra_plugin_process:
 */
GList *
cra_plugin_process (CraPlugin *plugin,
		    CraPackage *pkg,
		    const gchar *tmpdir,
		    GError **error)
{
	const gchar *tmp;
	CraApp *app;
	gchar *app_id = NULL;
	gchar **split;
	GList *apps = NULL;
	GPtrArray *keywords;
	GString *str = NULL;
	guint i;
	guint j;

	/* use the pkgname suffix as the app-id */
	tmp = cra_package_get_name (pkg);
	if (g_str_has_prefix (tmp, "gstreamer1-"))
		tmp += 11;
	if (g_str_has_prefix (tmp, "gstreamer-"))
		tmp += 10;
	if (g_str_has_prefix (tmp, "plugins-"))
		tmp += 8;
	app_id = g_strdup_printf ("gstreamer-%s", tmp);

	/* create app */
	app = cra_app_new (pkg, app_id);
	cra_app_set_type_id (app, "codec");
	cra_app_set_name (app, "C", "GStreamer Multimedia Codecs");
	cra_app_set_icon (app, "application-x-executable");
	cra_app_set_requires_appdata (app, TRUE);
	cra_app_set_icon_type (app, CRA_APP_ICON_TYPE_STOCK);
	cra_app_add_category (app, "Addons");
	cra_app_add_category (app, "Codecs");

	for (i = 0; data[i].path != NULL; i++) {
		if (!cra_utils_is_file_in_tmpdir (tmpdir, data[i].path))
			continue;
		split = g_strsplit (data[i].text, "|", -1);
		for (j = 0; split[j] != NULL; j++)
			cra_app_add_keyword (app, split[j]);
		g_strfreev (split);
	}

	/* no codecs we care about */
	keywords = cra_app_get_keywords (app);
	if (keywords->len == 0) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_filename (pkg));
		g_object_unref (app);
		goto out;
	}

	/* sort categories by name */
	g_ptr_array_sort (keywords, cra_utils_string_sort_cb);

	/* create a description */
	str = g_string_new ("Multimedia playback for ");
	if (keywords->len > 0) {
		for (i = 0; i < keywords->len - 1; i++) {
			tmp = g_ptr_array_index (keywords, i);
			g_string_append_printf (str, "%s, ", tmp);
		}
		g_string_truncate (str, str->len - 2);
		tmp = g_ptr_array_index (keywords, keywords->len - 1);
		g_string_append_printf (str, " and %s", tmp);
	} else {
		g_string_append (str, g_ptr_array_index (keywords, 0));
	}
	cra_app_set_comment (app, "C", str->str);

	/* add */
	cra_plugin_add_app (&apps, app);
out:
	if (str != NULL)
		g_string_free (str, TRUE);
	g_object_unref (app);
	g_free (app_id);
	return apps;
}
