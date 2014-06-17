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

#include <config.h>
#include <fnmatch.h>
#include <gdk/gdk.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_MODULE_H
#include <pango/pango.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "font";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/fonts/*/*.otf");
	cra_plugin_add_glob (globs, "/usr/share/fonts/*/*.ttf");
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/fonts/*/*.otf", filename, 0) == 0)
		return TRUE;
	if (fnmatch ("/usr/share/fonts/*/*.ttf", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_check_filename:
 */
gboolean
cra_plugin_check_filename (CraPlugin *plugin, const gchar *filename)
{
	return _cra_plugin_check_filename (filename);
}

/**
 * cra_font_fix_metadata:
 */
static void
cra_font_fix_metadata (CraApp *app)
{
	GList *l;
	PangoLanguage *plang;
	const gchar *tmp;
	const gchar *value;
	gint percentage;
	guint j;
	_cleanup_list_free_ GList *langs = NULL;
	_cleanup_string_free_ GString *str = NULL;
	struct {
		const gchar	*lang;
		const gchar	*value;
	} text_icon[] =  {
		{ "en",		"Aa" },
		{ "ar",		"أب" },
		{ "be",		"Аа" },
		{ "bg",		"Аа" },
		{ "cs",		"Aa" },
		{ "da",		"Aa" },
		{ "de",		"Aa" },
		{ "es",		"Aa" },
		{ "fr",		"Aa" },
		{ "gu",		"અબક" },
		{ "he",		"אב" },
		{ "it",		"Aa" },
		{ "ml",		"ആഇ" },
		{ "nl",		"Aa" },
		{ "pl",		"ĄĘ" },
		{ "pt",		"Aa" },
		{ "ru",		"Аа" },
		{ "sv",		"Åäö" },
		{ "ua",		"Аа" },
		{ "zh-tw",	"漢" },
		{ NULL, NULL } };
	struct {
		const gchar	*lang;
		const gchar	*value;
	} text_sample[] =  {
		{ "en",		"How quickly daft jumping zebras vex." },
		{ "ar",		"نصٌّ حكيمٌ لهُ سِرٌّ قاطِعٌ وَذُو شَأنٍ عَظيمٍ مكتوبٌ على ثوبٍ أخضرَ ومُغلفٌ بجلدٍ أزرق" },
		{ "be",		"У Іўі худы жвавы чорт у зялёнай камізэльцы пабег пад’есці фаршу з юшкай." },
		{ "bg",		"Под южно дърво, цъфтящо в синьо, бягаше малко, пухкаво зайче." },
		{ "cs",		"Příliš žluťoučký kůň úpěl ďábelské ódy" },
		{ "da",		"Quizdeltagerne spiste jordbær med fløde, mens cirkusklovnen Walther spillede på xylofon." },
		{ "de",		"Falsches Üben von Xylophonmusik quält jeden größeren Zwerg." },
		{ "es",		"Aquel biógrafo se zampó un extraño sándwich de vodka y ajo" },
		{ "fr",		"Voix ambiguë d'un cœur qui, au zéphyr, préfère les jattes de kiwis." },
		{ "gu",		"ઇ.સ. ૧૯૭૮ ની ૨૫ તારીખે, ૦૬-૩૪ વાગે, ઐશ્વર્યવાન, વફાદાર, અંગ્રેજ ઘરધણીના આ ઝાડ પાસે ઊભેલા બાદશાહ; અને ઓસરીમાંના ઠળીયા તથા છાણાના ઢગલા દુર કરીને, ઔપચારીકતાથી ઉભેલા ઋષી સમાન પ્રજ્ઞાચક્ષુ ખાલસાજી ભટ મળ્યા હતા." },
		{ "he",		"יַעֲקֹב בֶּן־דָּגָן הַשָּׂמֵחַ טִפֵּס בֶּעֱזוּז לְרֹאשׁ סֻלָּם מָאֳרָךְ לִצְפּוֹת בִּמְעוֹף דּוּכִיפַת וְנֵץ" },
		{ "it",		"Senza qualche prova ho il dubbio che si finga morto." },
		{ "ja",		"いろはにほへと ちりぬるを わかよたれそ つねならむ うゐのおくやま けふこえて あさきゆめみし ゑひもせす" },
		{ "ml",		"അജവും ആനയും ഐരാവതവും ഗരുഡനും കഠോര സ്വരം പൊഴിക്കെ ഹാരവും ഒഢ്യാണവും ഫാലത്തില്‍ മഞ്ഞളും ഈറന്‍ കേശത്തില്‍ ഔഷധ എണ്ണയുമായി ഋതുമതിയും അനഘയും ഭൂനാഥയുമായ ഉമ ദുഃഖഛവിയോടെ ഇടതു പാദം ഏന്തി ങ്യേയാദൃശം നിര്‍ഝരിയിലെ ചിറ്റലകളെ ഓമനിക്കുമ്പോള്‍ ബാ‍ലയുടെ കണ്‍കളില്‍ നീര്‍ ഊര്‍ന്നു വിങ്ങി" },
		{ "nl",		"Pa's wijze lynx bezag vroom het fikse aquaduct." },
		{ "pl",		"Pójdźże, kiń tę chmurność w głąb flaszy!" },
		{ "pt",		"À noite, vovô Kowalsky vê o ímã cair no pé do pingüim queixoso e vovó põe açúcar no chá de tâmaras do jabuti feliz." },
		{ "ru",		"В чащах юга жил бы цитрус? Да, но фальшивый экземпляр!" },
		{ "sv",		"Gud hjälpe qvickt Zorns mö få aw byxor" },
		{ "ua",		"Чуєш їх, доцю, га? Кумедна ж ти, прощайся без ґольфів!" },
		{ "zh-tw",	"秋風滑過拔地紅樓角落，誤見釣人低聲吟詠離騷。" },
		{ NULL, NULL } };

	/* ensure FontSampleText is defined */
	if (as_app_get_metadata_item (AS_APP (app), "FontSampleText") == NULL) {
		for (j = 0; text_sample[j].lang != NULL; j++) {
			percentage = as_app_get_language (AS_APP (app), text_sample[j].lang);
			if (percentage >= 0) {
				as_app_add_metadata (AS_APP (app),
						      "FontSampleText",
						      text_sample[j].value, -1);
				break;
			}
		}
	}

	/* ensure FontIconText is defined */
	if (as_app_get_metadata_item (AS_APP (app), "FontIconText") == NULL) {
		for (j = 0; text_icon[j].lang != NULL; j++) {
			percentage = as_app_get_language (AS_APP (app), text_icon[j].lang);
			if (percentage >= 0) {
				as_app_add_metadata (AS_APP (app),
						      "FontIconText",
						      text_icon[j].value, -1);
				break;
			}
		}
	}

	/* can we use a pango version */
	langs = as_app_get_languages (AS_APP (app));
	if (langs == NULL) {
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "No langs detected");
		return;
	}
	if (as_app_get_metadata_item (AS_APP (app), "FontIconText") == NULL) {
		for (l = langs; l != NULL; l = l->next) {
			tmp = l->data;
			plang = pango_language_from_string (tmp);
			value = pango_language_get_sample_string (plang);
			if (value == NULL)
				continue;
			as_app_add_metadata (AS_APP (app),
					      "FontSampleText",
					      value, -1);
			if (g_strcmp0 (value, "The quick brown fox jumps over the lazy dog.") == 0) {
				as_app_add_metadata (AS_APP (app),
						      "FontIconText",
						      "Aa", -1);
			} else {
				_cleanup_free_ gchar *icon_tmp;
				icon_tmp = g_utf8_substring (value, 0, 2);
				as_app_add_metadata (AS_APP (app),
						      "FontIconText",
						      icon_tmp, -1);
			}
		}
	}

	/* still not defined? */
	if (as_app_get_metadata_item (AS_APP (app), "FontSampleText") == NULL) {
		str = g_string_sized_new (1024);
		for (l = langs; l != NULL; l = l->next) {
			tmp = l->data;
			g_string_append_printf (str, "%s, ", tmp);
		}
		if (str->len > 2)
			g_string_truncate (str, str->len - 2);
		cra_package_log (cra_app_get_package (app),
				 CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "No FontSampleText for langs: %s",
				 str->str);
	}
}

/**
 * cra_font_string_is_valid:
 */
static gboolean
cra_font_string_is_valid (const gchar *text)
{
	guint i;

	for (i = 0; text[i] != '\0'; i++) {
		if (g_ascii_iscntrl (text[i]))
			return FALSE;
	}
	return TRUE;
}

/**
 * cra_font_add_metadata:
 */
static void
cra_font_add_metadata (CraApp *app, FT_Face ft_face)
{
	FT_SfntName sfname;
	guint i;
	guint j;
	guint len;
	struct {
		FT_UShort	 idx;
		const gchar	*key;
	} tt_idx_to_md_name[] =  {
		{ TT_NAME_ID_FONT_FAMILY,		"FontFamily" },
		{ TT_NAME_ID_FONT_SUBFAMILY,		"FontSubFamily" },
		{ TT_NAME_ID_FULL_NAME,			"FontFullName" },
		{ TT_NAME_ID_PREFERRED_FAMILY,		"FontParent" },
		{ 0, NULL } };

	if (!FT_IS_SFNT (ft_face))
		return;

	/* look at the metadata table */
	len = FT_Get_Sfnt_Name_Count (ft_face);
	for (i = 0; i < len; i++) {
		FT_Get_Sfnt_Name (ft_face, i, &sfname);
		for (j = 0; tt_idx_to_md_name[j].key != NULL; j++) {
			_cleanup_free_ gchar *val = NULL;
			if (sfname.name_id != tt_idx_to_md_name[j].idx)
				continue;
			val = g_locale_to_utf8 ((gchar *) sfname.string,
						sfname.string_len,
						NULL, NULL, NULL);
			if (val == NULL)
				continue;
			if (cra_font_string_is_valid (val)) {
				as_app_add_metadata (AS_APP (app),
						     tt_idx_to_md_name[j].key,
						     val, -1);
			} else {
				cra_package_log (cra_app_get_package (app),
						 CRA_PACKAGE_LOG_LEVEL_WARNING,
						 "Ignoring %s value: '%s'",
						 tt_idx_to_md_name[j].key, val);
			}
		}
	}
}

/**
 * cra_font_is_pixbuf_empty:
 */
static gboolean
cra_font_is_pixbuf_empty (const GdkPixbuf *pixbuf)
{
	gint i, j;
	gint rowstride;
	gint width;
	guchar *pixels;
	guchar *tmp;
	guint length;
	guint cnt = 0;

	pixels = gdk_pixbuf_get_pixels_with_length (pixbuf, &length);
	width = gdk_pixbuf_get_width (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	for (j = 0; j < gdk_pixbuf_get_height (pixbuf); j++) {
		for (i = 0; i < width; i++) {
			/* any opacity */
			tmp = pixels + j * rowstride + i * 4;
			if (tmp[3] > 0)
				cnt++;
		}
	}
	if (cnt > 5)
		return FALSE;
	return TRUE;
}

/**
 * cra_font_get_pixbuf:
 */
static GdkPixbuf *
cra_font_get_pixbuf (FT_Face ft_face,
		     guint width,
		     guint height,
		     const gchar *text,
		     GError **error)
{
	cairo_font_face_t *font_face;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_text_extents_t te;
	GdkPixbuf *pixbuf;
	guint text_size = 64;
	guint border_width = 8;

	/* set up font */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      width, height);
	cr = cairo_create (surface);
	font_face = cairo_ft_font_face_create_for_ft_face (ft_face, FT_LOAD_DEFAULT);
	cairo_set_font_face (cr, font_face);

	/* calculate best font size */
	while (text_size-- > 0) {
		cairo_set_font_size (cr, text_size);
		cairo_text_extents (cr, text, &te);
		if (te.width <= 0.01f || te.height <= 0.01f)
			continue;
		if (te.width < width - (border_width * 2) &&
		    te.height < height - (border_width * 2))
			break;
	}

	/* center text and blit to a pixbuf */
	cairo_move_to (cr,
		       (width / 2) - te.width / 2 - te.x_bearing,
		       (height / 2) - te.height / 2 - te.y_bearing);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_show_text (cr, text);
	pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);

	cairo_destroy (cr);
	cairo_font_face_destroy (font_face);
	cairo_surface_destroy (surface);
	return pixbuf;
}

/**
 * cra_font_get_caption:
 */
static gchar *
cra_font_get_caption (CraApp *app)
{
	const gchar *family;
	const gchar *subfamily;

	family = as_app_get_metadata_item (AS_APP (app), "FontFamily");
	subfamily = as_app_get_metadata_item (AS_APP (app), "FontSubFamily");
	if (family == NULL && subfamily == NULL)
		return NULL;
	if (family == NULL)
		return g_strdup (subfamily);
	if (subfamily == NULL)
		return g_strdup (family);
	return g_strdup_printf ("%s – %s", family, subfamily);
}

/**
 * cra_font_add_screenshot:
 */
static gboolean
cra_font_add_screenshot (CraApp *app, FT_Face ft_face, GError **error)
{
	const gchar *cache_dir;
	const gchar *mirror_uri;
	const gchar *tmp;
	_cleanup_free_ gchar *basename = NULL;
	_cleanup_free_ gchar *cache_fn = NULL;
	_cleanup_free_ gchar *caption = NULL;
	_cleanup_free_ gchar *url_tmp = NULL;
	_cleanup_object_unref_ AsImage *im = NULL;
	_cleanup_object_unref_ AsScreenshot *ss = NULL;
	_cleanup_object_unref_ GdkPixbuf *pixbuf = NULL;

	tmp = as_app_get_metadata_item (AS_APP (app), "FontSampleText");
	if (tmp == NULL)
		return TRUE;

	/* is in the cache */
	cache_dir = cra_package_get_config (cra_app_get_package (app), "CacheDir");
	cache_fn = g_strdup_printf ("%s/%s.png",
				    cache_dir,
				    as_app_get_id (AS_APP (app)));
	if (g_file_test (cache_fn, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file (cache_fn, error);
		if (pixbuf == NULL)
			return FALSE;
	} else {
		pixbuf = cra_font_get_pixbuf (ft_face, 640, 48, tmp, error);
		if (pixbuf == NULL)
			return FALSE;
	}

	/* check pixbuf is not just blank */
	if (cra_font_is_pixbuf_empty (pixbuf)) {
		g_set_error_literal (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Could not generate font preview");
		return FALSE;
	}

	mirror_uri = cra_package_get_config (cra_app_get_package (app),
					     "MirrorURI");
	im = as_image_new ();
	as_image_set_pixbuf (im, pixbuf);
	as_image_set_kind (im, AS_IMAGE_KIND_SOURCE);
	basename = g_strdup_printf ("%s-%s.png",
				    as_app_get_id (AS_APP (app)),
				    as_image_get_md5 (im));
	as_image_set_basename (im, basename);
	url_tmp = g_build_filename (mirror_uri,
				    "source",
				    basename,
				    NULL);
	as_image_set_url (im, url_tmp, -1);

	ss = as_screenshot_new ();
	as_screenshot_set_kind (ss, AS_SCREENSHOT_KIND_DEFAULT);
	as_screenshot_add_image (ss, im);
	caption = cra_font_get_caption (app);
	if (caption != NULL)
		as_screenshot_set_caption (ss, NULL, caption, -1);
	as_app_add_screenshot (AS_APP (app), ss);

	/* save to cache */
	if (!g_file_test (cache_fn, G_FILE_TEST_EXISTS)) {
		if (!gdk_pixbuf_save (pixbuf, cache_fn, "png", error, NULL))
			return FALSE;
	}
	return TRUE;
}

/**
 * cra_font_add_languages:
 */
static void
cra_font_add_languages (CraApp *app, const FcPattern *pattern)
{
	const gchar *tmp;
	FcResult fc_rc = FcResultMatch;
	FcStrList *list;
	FcStrSet *langs;
	FcValue fc_value;
	guint i;
	gboolean any_added = FALSE;

	for (i = 0; fc_rc == FcResultMatch; i++) {
		fc_rc = FcPatternGet (pattern, FC_LANG, i, &fc_value);
		if (fc_rc == FcResultMatch) {
			langs = FcLangSetGetLangs (fc_value.u.l);
			list = FcStrListCreate (langs);
			FcStrListFirst (list);
			while ((tmp = (const gchar*) FcStrListNext (list)) != NULL) {
				as_app_add_language (AS_APP (app), 0, tmp, -1);
				any_added = TRUE;
			}
			FcStrListDone (list);
			FcStrSetDestroy (langs);
		}
	}

	/* assume 'en' is available */
	if (!any_added)
		as_app_add_language (AS_APP (app), 0, "en", -1);
}

/**
 * cra_plugin_font_set_name:
 */
static void
cra_plugin_font_set_name (CraApp *app, const gchar *name)
{
	const gchar *ptr;
	guint i;
	guint len;
	_cleanup_free_ gchar *tmp;
	const gchar *prefixes[] = { "GFS ", NULL };
	const gchar *suffixes[] = { " SIL",
				    " ADF",
				    " CLM",
				    " GPL&GNU",
				    " SC",
				    NULL };

	/* remove font foundary suffix */
	tmp = g_strdup (name);
	for (i = 0; suffixes[i] != NULL; i++) {
		if (g_str_has_suffix (tmp, suffixes[i])) {
			len = strlen (tmp);
			tmp[len - strlen (suffixes[i])] = '\0';
		}
	}

	/* remove font foundary prefix */
	ptr = tmp;
	for (i = 0; prefixes[i] != NULL; i++) {
		if (g_str_has_prefix (tmp, prefixes[i]))
			ptr += strlen (prefixes[i]);
	}
	as_app_set_name (AS_APP (app), "C", ptr, -1);
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraPlugin *plugin,
			     CraPackage *pkg,
			     const gchar *filename,
			     GList **apps,
			     const gchar *tmpdir,
			     GError **error)
{
	FcConfig *config;
	FcFontSet *fonts;
	FT_Error rc;
	FT_Face ft_face = NULL;
	FT_Library library = NULL;
	const gchar *tmp;
	gboolean ret = TRUE;
	const FcPattern *pattern;
	_cleanup_free_ gchar *app_id = NULL;
	_cleanup_free_ gchar *comment = NULL;
	_cleanup_free_ gchar *filename_full;
	_cleanup_free_ gchar *icon_filename = NULL;
	_cleanup_object_unref_ CraApp *app = NULL;
	_cleanup_object_unref_ GdkPixbuf *pixbuf = NULL;

	/* load font */
	filename_full = g_build_filename (tmpdir, filename, NULL);
	config = FcConfigCreate ();
	ret = FcConfigAppFontAddFile (config, (FcChar8 *) filename_full);
	if (!ret) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to AddFile %s", filename);
		goto out;
	}
	fonts = FcConfigGetFonts (config, FcSetApplication);
	if (fonts == NULL || fonts->fonts == NULL) {
		ret = FALSE;
		g_set_error_literal (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "FcConfigGetFonts failed");
		goto out;
	}
	pattern = fonts->fonts[0];
	FT_Init_FreeType (&library);
	rc = FT_New_Face (library, filename_full, 0, &ft_face);
	if (rc != 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "FT_Open_Face failed to open %s: %i",
			     filename, rc);
		goto out;
	}

	/* create app that might get merged later */
	app_id = g_path_get_basename (filename);
	app = cra_app_new (pkg, app_id);
	as_app_set_id_kind (AS_APP (app), AS_ID_KIND_FONT);
	as_app_add_category (AS_APP (app), "Addons", -1);
	as_app_add_category (AS_APP (app), "Fonts", -1);
	cra_app_set_requires_appdata (app, TRUE);
	cra_plugin_font_set_name (app, ft_face->family_name);
	comment = g_strdup_printf ("A %s font from %s",
				   ft_face->style_name,
				   ft_face->family_name);
	as_app_set_comment (AS_APP (app), "C", comment, -1);
	cra_font_add_languages (app, pattern);
	cra_font_add_metadata (app, ft_face);
	cra_font_fix_metadata (app);
	ret = cra_font_add_screenshot (app, ft_face, error);
	if (!ret)
		goto out;

	/* generate icon */
	tmp = as_app_get_metadata_item (AS_APP (app), "FontIconText");
	if (tmp != NULL) {
		icon_filename = g_strdup_printf ("%s.png", as_app_get_id (AS_APP (app)));
		as_app_set_icon (AS_APP (app), icon_filename, -1);
		pixbuf = cra_font_get_pixbuf (ft_face, 64, 64, tmp, error);
		if (pixbuf == NULL) {
			ret = FALSE;
			goto out;
		}

		/* check pixbuf is not just blank */
		if (cra_font_is_pixbuf_empty (pixbuf)) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Could not generate 64x64 font icon "
				     "with '%s'", tmp);
			goto out;
		}
		as_app_set_icon_kind (AS_APP (app), AS_ICON_KIND_CACHED);
		cra_app_set_pixbuf (app, pixbuf);
	}

	/* add */
	cra_plugin_add_app (apps, app);
out:
	FcConfigAppFontClear (config);
	FcConfigDestroy (config);
	if (ft_face != NULL)
		FT_Done_Face (ft_face);
	if (library != NULL)
		FT_Done_Library (library);
	return ret;
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
	gboolean ret;
	GList *apps = NULL;
	guint i;
	gchar **filelist;

	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (!_cra_plugin_check_filename (filelist[i]))
			continue;
		ret = cra_plugin_process_filename (plugin,
						   pkg,
						   filelist[i],
						   &apps,
						   tmpdir,
						   error);
		if (!ret) {
			g_list_free_full (apps, (GDestroyNotify) g_object_unref);
			return NULL;
		}
	}

	/* no fonts files we care about */
	if (apps == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_basename (pkg));
		return NULL;
	}
	return apps;
}

/**
 * cra_font_get_app_sortable_idx:
 */
static guint
cra_font_get_app_sortable_idx (CraApp *app)
{
	const gchar *font_str = as_app_get_id (AS_APP (app));
	guint idx = 0;

	if (g_strstr_len (font_str, -1, "It") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Bold") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Semibold") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "ExtraLight") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Lig") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Medium") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Bla") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Hai") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Keyboard") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Kufi") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Tamil") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Hebrew") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Arabic") != NULL)
		idx += 1;
	if (g_strstr_len (font_str, -1, "Fallback") != NULL)
		idx += 1;
	return idx;
}

/**
 * cra_font_merge_family:
 */
static void
cra_font_merge_family (GList **list, const gchar *md_key)
{
	CraApp *app;
	CraApp *found;
	GList *hash_values = NULL;
	GList *l;
	GList *list_new = NULL;
	const gchar *tmp;
	_cleanup_hashtable_unref_ GHashTable *hash;

	hash = g_hash_table_new_full (g_str_hash, g_str_equal,
				      g_free, (GDestroyNotify) g_object_unref);
	for (l = *list; l != NULL; l = l->next) {
		if (!CRA_IS_APP (l->data)) {
			cra_plugin_add_app (&list_new, l->data);
			continue;
		}
		app = CRA_APP (l->data);

		/* no family, or not a font */
		tmp = as_app_get_metadata_item (AS_APP (app), md_key);
		if (tmp == NULL) {
			cra_plugin_add_app (&list_new, app);
			continue;
		}

		/* find the font family */
		found = g_hash_table_lookup (hash, tmp);
		if (found == NULL) {
			g_hash_table_insert (hash,
					     g_strdup (tmp),
					     g_object_ref (app));
		} else {
			/* app is better than found */
			if (cra_font_get_app_sortable_idx (app) <
			    cra_font_get_app_sortable_idx (found)) {
				g_hash_table_insert (hash,
						     g_strdup (tmp),
						     g_object_ref (app));
				as_app_subsume (AS_APP (app), AS_APP (found));
			} else {
				as_app_subsume (AS_APP (found), AS_APP (app));
			}
		}
	}

	/* add all the best fonts to the list */
	hash_values = g_hash_table_get_values (hash);
	for (l = hash_values; l != NULL; l = l->next) {
		app = CRA_APP (l->data);
		cra_plugin_add_app (&list_new, app);
	}
	g_list_free (hash_values);

	/* success */
	g_list_free_full (*list, (GDestroyNotify) g_object_unref);
	*list = list_new;
}

/**
 * cra_plugin_merge:
 */
void
cra_plugin_merge (CraPlugin *plugin, GList **list)
{
	cra_font_merge_family (list, "FontFamily");
	cra_font_merge_family (list, "FontParent");
}
