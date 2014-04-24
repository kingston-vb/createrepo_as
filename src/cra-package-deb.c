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

#include "cra-package-deb.h"
#include "cra-plugin.h"


G_DEFINE_TYPE (CraPackageDeb, cra_package_deb, CRA_TYPE_PACKAGE)

/**
 * cra_package_deb_init:
 **/
static void
cra_package_deb_init (CraPackageDeb *pkg)
{
}

/**
 * cra_package_deb_ensure_simple:
 **/
static gboolean
cra_package_deb_ensure_simple (CraPackage *pkg, GError **error)
{
	GPtrArray *deps = NULL;
	const gchar *argv[4] = { "dpkg", "--field", "fn", NULL };
	gboolean ret;
	gchar **lines = NULL;
	gchar *output = NULL;
	gchar *tmp;
	gchar **vr;
	guint i;
	guint j;

	/* spawn sync */
	argv[2] = cra_package_get_filename (pkg);
	ret = g_spawn_sync (NULL, (gchar **) argv, NULL,
			    G_SPAWN_SEARCH_PATH,
			    NULL, NULL,
			    &output, NULL, NULL, error);
	if (!ret)
		goto out;

	/* parse output */
	deps = g_ptr_array_new_with_free_func (g_free);
	lines = g_strsplit (output, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		if (g_str_has_prefix (lines[i], "Package: ")) {
			cra_package_set_name (pkg, lines[i] + 9);
			continue;
		}
		if (g_str_has_prefix (lines[i], "Source: ")) {
			cra_package_set_source (pkg, lines[i] + 8);
			continue;
		}
		if (g_str_has_prefix (lines[i], "Version: ")) {
			vr = g_strsplit (lines[i] + 9, "-", 2);
			cra_package_set_version (pkg, vr[0]);
			cra_package_set_release (pkg, vr[1]);
			g_strfreev (vr);
			continue;
		}
		if (g_str_has_prefix (lines[i], "Depends: ")) {
			vr = g_strsplit (lines[i] + 9, ", ", -1);
			for (j = 0; vr[j] != NULL; j++) {
				tmp = g_strstr_len (vr[j], -1, " ");
				if (tmp != NULL)
					*tmp = '\0';
				g_ptr_array_add (deps, vr[j]);
			}
			continue;
		}
	}
	g_ptr_array_add (deps, NULL);
	cra_package_set_deps (pkg, (gchar **) deps->pdata);
out:
	if (deps != NULL)
		g_ptr_array_unref (deps);
	g_strfreev (lines);
	g_free (output);
	return ret;
}

/**
 * cra_package_deb_ensure_filelists:
 **/
static gboolean
cra_package_deb_ensure_filelists (CraPackage *pkg, GError **error)
{
	GPtrArray *files = NULL;
	const gchar *argv[4] = { "dpkg", "--contents", "fn", NULL };
	const gchar *fn;
	gboolean ret;
	gchar **lines = NULL;
	gchar *output = NULL;
	guint i;

	/* spawn sync */
	argv[2] = cra_package_get_filename (pkg);
	ret = g_spawn_sync (NULL, (gchar **) argv, NULL,
			    G_SPAWN_SEARCH_PATH,
			    NULL, NULL,
			    &output, NULL, NULL, error);
	if (!ret)
		goto out;

	/* parse output */
	files = g_ptr_array_new_with_free_func (g_free);
	lines = g_strsplit (output, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {
		fn = g_strrstr (lines[i], " ");
		if (fn == NULL)
			continue;
		/* ignore directories */
		if (g_str_has_suffix (fn, "/"))
			continue;
		g_ptr_array_add (files, g_strdup (fn + 2));
	}

	/* save */
	g_ptr_array_add (files, NULL);
	cra_package_set_filelist (pkg, (gchar **) files->pdata);
out:
	if (files != NULL)
		g_ptr_array_unref (files);
	g_strfreev (lines);
	g_free (output);
	return ret;
}

/**
 * cra_package_deb_open:
 **/
static gboolean
cra_package_deb_open (CraPackage *pkg, const gchar *filename, GError **error)
{
	gboolean ret;

	/* read package stuff */
	ret = cra_package_deb_ensure_simple (pkg, error);
	if (!ret)
		goto out;
	ret = cra_package_deb_ensure_filelists (pkg, error);
	if (!ret)
		goto out;
out:
	return ret;
}

/**
 * cra_package_deb_explode:
 **/
static gboolean
cra_package_deb_explode (CraPackage *pkg,
			 const gchar *dir,
			 GPtrArray *glob,
			 GError **error)
{
	gboolean ret;
	gchar *data_fn = NULL;

	/* first decompress the main deb */
	ret = cra_utils_explode (cra_package_get_filename (pkg),
				 dir, NULL, error);
	if (!ret)
		goto out;

	/* then decompress the data file */
	data_fn = g_build_filename (dir, "data.tar.xz", NULL);
	ret = cra_utils_explode (data_fn, dir, glob, error);
	if (!ret)
		goto out;
out:
	g_free (data_fn);
	return ret;
}

/**
 * cra_package_deb_class_init:
 **/
static void
cra_package_deb_class_init (CraPackageDebClass *klass)
{
	CraPackageClass *package_class = CRA_PACKAGE_CLASS (klass);
	package_class->open = cra_package_deb_open;
	package_class->explode = cra_package_deb_explode;
}

/**
 * cra_package_deb_new:
 **/
CraPackage *
cra_package_deb_new (void)
{
	CraPackage *pkg;
	pkg = g_object_new (CRA_TYPE_PACKAGE_DEB, NULL);
	return CRA_PACKAGE (pkg);
}
