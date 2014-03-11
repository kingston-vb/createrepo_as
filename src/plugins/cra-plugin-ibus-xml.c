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
#include <sqlite3.h>

#include <cra-plugin.h>
#include <cra-dom.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "ibus-xml";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/ibus/component/*.xml");
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/ibus/component/*.xml", filename, 0) == 0)
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
	CraApp *app = NULL;
	CraDom *dom = NULL;
	gboolean ret;
	gchar *basename = NULL;
	gchar *filename_tmp;
	gchar *data = NULL;
	gchar **lines = NULL;
	guint i;
	gboolean found_header = FALSE;
	GString *valid_xml;
	const GNode *n;

	/* open file */
	filename_tmp = g_build_filename (tmpdir, filename, NULL);
	ret = g_file_get_contents (filename_tmp, &data, NULL, error);
	if (!ret)
		goto out;

	/* some components start with a comment (invalid XML) and some
	 * don't even have '<?xml' -- try to fix up best we can */
	valid_xml = g_string_new ("");
	lines = g_strsplit (data, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		if (g_str_has_prefix (lines[i], "<?xml") ||
		    g_str_has_prefix (lines[i], "<component>"))
			found_header = TRUE;
		if (found_header)
			g_string_append_printf (valid_xml, "%s\n", lines[i]);
	}

	/* parse contents */
	dom = cra_dom_new ();
	ret = cra_dom_parse_xml_data (dom, valid_xml->str, -1, error);
	if (!ret)
		goto out;

	/* create new app */
	basename = g_path_get_basename (filename);
	app = cra_app_new (pkg, basename);
	cra_app_set_kind (app, CRA_APP_KIND_INPUT_METHOD);
	cra_app_add_category (app, "Addons");
	cra_app_add_category (app, "InputSources");
	cra_app_set_icon (app, "system-run-symbolic");
	cra_app_set_icon_type (app, CRA_APP_ICON_TYPE_STOCK);
	cra_app_set_requires_appdata (app, TRUE);

	/* read the component header which all input methods have */
	n = cra_dom_get_node (dom, NULL, "component/description");
	if (n != NULL) {
		cra_app_set_name (app, "C", cra_dom_get_node_data (n));
		cra_app_set_comment (app, "C", cra_dom_get_node_data (n));
	}
	n = cra_dom_get_node (dom, NULL, "component/homepage");
	if (n != NULL)
		cra_app_set_homepage_url (app, cra_dom_get_node_data (n));

	/* do we have a engine section we can use? */
	n = cra_dom_get_node (dom, NULL, "engines/engine/longname");
	if (n != NULL)
		cra_app_set_name (app, "C", cra_dom_get_node_data (n));
	n = cra_dom_get_node (dom, NULL, "engines/engine/description");
	if (n != NULL)
		cra_app_set_comment (app, "C", cra_dom_get_node_data (n));

	/* add */
	cra_plugin_add_app (apps, app);
out:
	g_strfreev (lines);
	g_free (data);
	g_free (basename);
	g_free (filename_tmp);
	if (app != NULL)
		g_object_unref (app);
	if (dom != NULL)
		g_object_unref (dom);
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
			/* FIXME: free apps? */
			apps = NULL;
			goto out;
		}
	}

	/* no desktop files we care about */
	if (apps == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_filename (pkg));
		goto out;
	}
out:
	return apps;
}
