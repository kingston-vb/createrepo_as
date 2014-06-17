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

/**
 * SECTION:cra-package
 * @short_description: Object representing a package file.
 * @stability: Unstable
 *
 * This object represents one package file.
 */

#include "config.h"

#include <limits.h>

#include "cra-cleanup.h"
#include "cra-package.h"
#include "cra-plugin.h"

typedef struct _CraPackagePrivate	CraPackagePrivate;
struct _CraPackagePrivate
{
	gboolean	 enabled;
	gchar		**filelist;
	gchar		**deps;
	gchar		*filename;
	gchar		*basename;
	gchar		*name;
	guint		 epoch;
	gchar		*version;
	gchar		*release;
	gchar		*arch;
	gchar		*url;
	gchar		*nevr;
	gchar		*evr;
	gchar		*license;
	gchar		*source;
	GString		*log;
	GHashTable	*configs;
	GTimer		*timer;
	gdouble		 last_log;
	GPtrArray	*releases;
	GHashTable	*releases_hash;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraPackage, cra_package, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_package_get_instance_private (o))

/**
 * cra_package_finalize:
 **/
static void
cra_package_finalize (GObject *object)
{
	CraPackage *pkg = CRA_PACKAGE (object);
	CraPackagePrivate *priv = GET_PRIVATE (pkg);

	g_strfreev (priv->filelist);
	g_strfreev (priv->deps);
	g_free (priv->filename);
	g_free (priv->basename);
	g_free (priv->name);
	g_free (priv->version);
	g_free (priv->release);
	g_free (priv->arch);
	g_free (priv->url);
	g_free (priv->nevr);
	g_free (priv->evr);
	g_free (priv->license);
	g_free (priv->source);
	g_string_free (priv->log, TRUE);
	g_timer_destroy (priv->timer);
	g_hash_table_unref (priv->configs);
	g_ptr_array_unref (priv->releases);
	g_hash_table_unref (priv->releases_hash);

	G_OBJECT_CLASS (cra_package_parent_class)->finalize (object);
}

/**
 * cra_package_init:
 **/
static void
cra_package_init (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	priv->enabled = TRUE;
	priv->log = g_string_sized_new (1024);
	priv->timer = g_timer_new ();
	priv->configs = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free, g_free);
	priv->releases = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	priv->releases_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						     g_free, (GDestroyNotify) g_object_unref);
}

/**
 * cra_package_get_enabled:
 * @pkg: A #CraPackage
 *
 * Gets if the package is enabled.
 *
 * Returns: enabled status
 *
 * Since: 0.1.0
 **/
gboolean
cra_package_get_enabled (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->enabled;
}

/**
 * cra_package_set_enabled:
 * @pkg: A #CraPackage
 * @enabled: boolean
 *
 * Enables or disables the package.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_enabled (CraPackage *pkg, gboolean enabled)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	priv->enabled = enabled;
}

/**
 * cra_package_log_start:
 * @pkg: A #CraPackage
 *
 * Starts the log timer.
 *
 * Since: 0.1.0
 **/
void
cra_package_log_start (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_timer_reset (priv->timer);
}

/**
 * cra_package_log:
 * @pkg: A #CraPackage
 * @log_level: a #CraPackageLogLevel
 * @fmt: Format string
 * @...: varargs
 *
 * Logs a message.
 *
 * Since: 0.1.0
 **/
void
cra_package_log (CraPackage *pkg,
		 CraPackageLogLevel log_level,
		 const gchar *fmt, ...)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	va_list args;
	gdouble now;
	_cleanup_free_ gchar *tmp;

	va_start (args, fmt);
	tmp = g_strdup_vprintf (fmt, args);
	va_end (args);
	if (g_getenv ("CRA_PROFILE") != NULL) {
		now = g_timer_elapsed (priv->timer, NULL) * 1000;
		g_string_append_printf (priv->log,
					"%05.0f\t+%05.0f\t",
					now, now - priv->last_log);
		priv->last_log = now;
	}
	switch (log_level) {
	case CRA_PACKAGE_LOG_LEVEL_INFO:
		g_debug ("INFO:    %s", tmp);
		g_string_append_printf (priv->log, "INFO:    %s\n", tmp);
		break;
	case CRA_PACKAGE_LOG_LEVEL_DEBUG:
		g_debug ("DEBUG:   %s", tmp);
		if (g_getenv ("CRA_PROFILE") != NULL)
			g_string_append_printf (priv->log, "DEBUG:   %s\n", tmp);
		break;
	case CRA_PACKAGE_LOG_LEVEL_WARNING:
		g_debug ("WARNING: %s", tmp);
		g_string_append_printf (priv->log, "WARNING: %s\n", tmp);
		break;
	default:
		g_debug ("%s", tmp);
		g_string_append_printf (priv->log, "%s\n", tmp);
		break;
	}
}

/**
 * cra_package_log_flush:
 * @pkg: A #CraPackage
 * @error: A #GError or %NULL
 *
 * Flushes the log queue.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_package_log_flush (CraPackage *pkg, GError **error)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	_cleanup_free_ gchar *logfile;

	/* overwrite old log */
	logfile = g_strdup_printf ("%s/%s.log",
				   cra_package_get_config (pkg, "LogDir"),
				   cra_package_get_name (pkg));
	return g_file_set_contents (logfile, priv->log->str, -1, error);
}

/**
 * cra_package_get_filename:
 * @pkg: A #CraPackage
 *
 * Gets the filename of the package.
 *
 * Returns: utf8 filename
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_filename (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->filename;
}

/**
 * cra_package_get_basename:
 * @pkg: A #CraPackage
 *
 * Gets the package basename.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_basename (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->basename;
}

/**
 * cra_package_get_name:
 * @pkg: A #CraPackage
 *
 * Gets the package name
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_name (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->name;
}

/**
 * cra_package_get_url:
 * @pkg: A #CraPackage
 *
 * Gets the package homepage URL
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_url (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->url;
}

/**
 * cra_package_get_license:
 * @pkg: A #CraPackage
 *
 * Gets the package license.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_license (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->license;
}

/**
 * cra_package_get_source:
 * @pkg: A #CraPackage
 *
 * Gets the package source name.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_source (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->source;
}

/**
 * cra_package_get_filelist:
 * @pkg: A #CraPackage
 *
 * Gets the package filelist.
 *
 * Returns: (transfer none) (element-type utf8): filelist
 *
 * Since: 0.1.0
 **/
gchar **
cra_package_get_filelist (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->filelist;
}

/**
 * cra_package_get_deps:
 * @pkg: A #CraPackage
 *
 * Get the package dependancy list.
 *
 * Returns: (transfer none) (element-type utf8): deplist
 *
 * Since: 0.1.0
 **/
gchar **
cra_package_get_deps (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->deps;
}

/**
 * cra_package_set_name:
 * @pkg: A #CraPackage
 * @name: package name
 *
 * Sets the package name.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_name (CraPackage *pkg, const gchar *name)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->name);
	priv->name = g_strdup (name);
}

/**
 * cra_package_set_version:
 * @pkg: A #CraPackage
 * @version: package version
 *
 * Sets the package version.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_version (CraPackage *pkg, const gchar *version)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->version);
	priv->version = g_strdup (version);
}

/**
 * cra_package_set_release:
 * @pkg: A #CraPackage
 * @release: package release
 *
 * Sets the package release.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_release (CraPackage *pkg, const gchar *release)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->release);
	priv->release = g_strdup (release);
}

/**
 * cra_package_set_arch:
 * @pkg: A #CraPackage
 * @arch: package architecture
 *
 * Sets the package architecture.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_arch (CraPackage *pkg, const gchar *arch)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->arch);
	priv->arch = g_strdup (arch);
}

/**
 * cra_package_set_epoch:
 * @pkg: A #CraPackage
 * @epoch: epoch, or 0 for unset
 *
 * Sets the package epoch
 *
 * Since: 0.1.0
 **/
void
cra_package_set_epoch (CraPackage *pkg, guint epoch)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	priv->epoch = epoch;
}

/**
 * cra_package_set_url:
 * @pkg: A #CraPackage
 * @url: homepage URL
 *
 * Sets the package URL.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_url (CraPackage *pkg, const gchar *url)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->url);
	priv->url = g_strdup (url);
}

/**
 * cra_package_set_license:
 * @pkg: A #CraPackage
 * @license: license string
 *
 * Sets the package license.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_license (CraPackage *pkg, const gchar *license)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->license);
	priv->license = g_strdup (license);
}

/**
 * cra_package_set_source:
 * @pkg: A #CraPackage
 * @source: source string, e.g. the srpm name
 *
 * Sets the package source name, which is usually the parent of a set of
 * subpackages.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_source (CraPackage *pkg, const gchar *source)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_free (priv->source);
	priv->source = g_strdup (source);
}

/**
 * cra_package_set_deps:
 * @pkg: A #CraPackage
 * @deps: package deps
 *
 * Sets the package dependancies.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_deps (CraPackage *pkg, gchar **deps)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_strfreev (priv->deps);
	priv->deps = g_strdupv (deps);
}

/**
 * cra_package_set_filelist:
 * @pkg: A #CraPackage
 * @filelist: package filelist
 *
 * Sets the package filelist.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_filelist (CraPackage *pkg, gchar **filelist)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_strfreev (priv->filelist);
	priv->filelist = g_strdupv (filelist);
}

/**
 * cra_package_get_nevr:
 * @pkg: A #CraPackage
 *
 * Gets the package NEVR.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_nevr (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	if (priv->nevr == NULL) {
		if (priv->epoch == 0) {
			priv->nevr = g_strdup_printf ("%s-%s-%s",
						      priv->name,
						      priv->version,
						      priv->release);
		} else {
			priv->nevr = g_strdup_printf ("%s-%i:%s-%s",
						      priv->name,
						      priv->epoch,
						      priv->version,
						      priv->release);
		}
	}
	return priv->nevr;
}

/**
 * cra_package_get_evr:
 * @pkg: A #CraPackage
 *
 * Gets the package EVR.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_evr (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	if (priv->evr == NULL) {
		if (priv->epoch == 0) {
			priv->evr = g_strdup_printf ("%s-%s",
						     priv->version,
						     priv->release);
		} else {
			priv->evr = g_strdup_printf ("%i:%s-%s",
						     priv->epoch,
						     priv->version,
						     priv->release);
		}
	}
	return priv->evr;
}

/**
 * cra_package_class_init:
 **/
static void
cra_package_class_init (CraPackageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_package_finalize;
}

/**
 * cra_package_open:
 * @pkg: A #CraPackage
 * @filename: package filename
 * @error: A #GError or %NULL
 *
 * Opens a package and parses the contents.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_package_open (CraPackage *pkg, const gchar *filename, GError **error)
{
	CraPackageClass *klass = CRA_PACKAGE_GET_CLASS (pkg);
	CraPackagePrivate *priv = GET_PRIVATE (pkg);

	/* cache here */
	priv->filename = g_strdup (filename);
	priv->basename = g_path_get_basename (filename);

	/* call distro-specific method */
	if (klass->open != NULL)
		return klass->open (pkg, filename, error);
	return TRUE;
}

/**
 * cra_package_explode:
 * @pkg: A #CraPackage
 * @dir: directory to explode into
 * @glob: (element-type utf8): the glob list, or %NULL
 * @error: A #GError or %NULL
 *
 * Decompresses a package into a directory, optionally using a glob list.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
cra_package_explode (CraPackage *pkg,
		     const gchar *dir,
		     GPtrArray *glob,
		     GError **error)
{
	CraPackageClass *klass = CRA_PACKAGE_GET_CLASS (pkg);
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	if (klass->explode != NULL)
		return klass->explode (pkg, dir, glob, error);
	return cra_utils_explode (priv->filename, dir, glob, error);
}

/**
 * cra_package_set_config:
 * @pkg: A #CraPackage
 * @key: utf8 string
 * @value: utf8 string
 *
 * Sets a config attribute on a package.
 *
 * Since: 0.1.0
 **/
void
cra_package_set_config (CraPackage *pkg, const gchar *key, const gchar *value)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_hash_table_insert (priv->configs, g_strdup (key), g_strdup (value));
}

/**
 * cra_package_get_config:
 * @pkg: A #CraPackage
 * @key: utf8 string
 *
 * Gets a config attribute from a package.
 *
 * Returns: utf8 string
 *
 * Since: 0.1.0
 **/
const gchar *
cra_package_get_config (CraPackage *pkg, const gchar *key)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return g_hash_table_lookup (priv->configs, key);
}

/**
 * cra_package_get_releases:
 * @pkg: A #CraPackage
 *
 * Gets the releases of the package.
 *
 * Returns: (transfer none) (element-type AsRelease): the release data
 *
 * Since: 0.1.0
 **/
GPtrArray *
cra_package_get_releases (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->releases;
}

/**
 * cra_package_compare:
 * @pkg1: A #CraPackage
 * @pkg2: A #CraPackage
 *
 * Compares one package with another.
 *
 * Returns: -1 for <, 0 for the same and +1 for >
 *
 * Since: 0.1.0
 **/
gint
cra_package_compare (CraPackage *pkg1, CraPackage *pkg2)
{
	CraPackageClass *klass = CRA_PACKAGE_GET_CLASS (pkg1);
	if (klass->compare != NULL)
		return klass->compare (pkg1, pkg2);
	return 0;
}

/**
 * cra_package_get_release:
 * @pkg: A #CraPackage
 * @version: package version
 *
 * Gets the release for a specific version.
 *
 * Returns: (transfer none): an #AsRelease, or %NULL for not found
 *
 * Since: 0.1.0
 **/
AsRelease *
cra_package_get_release	(CraPackage *pkg, const gchar *version)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return g_hash_table_lookup (priv->releases_hash, version);
}

/**
 * cra_package_add_release:
 * @pkg: A #CraPackage
 * @version: a package version
 * @release: a package release
 *
 * Adds a (downstream) release to a package.
 *
 * Since: 0.1.0
 **/
void
cra_package_add_release	(CraPackage *pkg,
			 const gchar *version,
			 AsRelease *release)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_hash_table_insert (priv->releases_hash,
			     g_strdup (version),
			     g_object_ref (release));
	g_ptr_array_add (priv->releases, g_object_ref (release));
}
