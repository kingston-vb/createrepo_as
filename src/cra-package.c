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

#include <limits.h>
#include <archive.h>
#include <archive_entry.h>

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
 **/
gboolean
cra_package_get_enabled (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->enabled;
}

/**
 * cra_package_set_enabled:
 **/
void
cra_package_set_enabled (CraPackage *pkg, gboolean enabled)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	priv->enabled = enabled;
}

/**
 * cra_package_log_start:
 **/
void
cra_package_log_start (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_timer_reset (priv->timer);
}

/**
 * cra_package_log:
 **/
void
cra_package_log (CraPackage *pkg,
		 CraPackageLogLevel log_level,
		 const gchar *fmt, ...)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	gchar *tmp;
	va_list args;
	gdouble now;

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
	g_free (tmp);
}

/**
 * cra_package_log_flush:
 **/
gboolean
cra_package_log_flush (CraPackage *pkg, GError **error)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	const gchar *tmp;
	gboolean ret;
	gchar *logdir;
	gchar *logfile;
	gint rc;

	/* overwrite old log */
	tmp = cra_package_get_config (pkg, "LogDir");
	logdir = g_build_filename (tmp, cra_package_get_name (pkg), NULL);
	rc = g_mkdir_with_parents (logdir, 0700);
	if (rc < 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to create %s", logdir);
	}
	logfile = g_strdup_printf ("%s/%s.log",
				   logdir,
				   cra_package_get_nevr (pkg));
	ret = g_file_set_contents (logfile, priv->log->str, -1, error);
	if (!ret)
		goto out;
out:
	g_free (logdir);
	g_free (logfile);
	return ret;
}

/**
 * cra_package_get_filename:
 **/
const gchar *
cra_package_get_filename (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->filename;
}

/**
 * cra_package_get_basename:
 **/
const gchar *
cra_package_get_basename (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->basename;
}

/**
 * cra_package_get_name:
 **/
const gchar *
cra_package_get_name (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->name;
}

/**
 * cra_package_get_url:
 **/
const gchar *
cra_package_get_url (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->url;
}

/**
 * cra_package_get_license:
 **/
const gchar *
cra_package_get_license (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->license;
}

/**
 * cra_package_get_source:
 **/
const gchar *
cra_package_get_source (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->source;
}

/**
 * cra_package_get_filelist:
 **/
gchar **
cra_package_get_filelist (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->filelist;
}

/**
 * cra_package_get_deps:
 **/
gchar **
cra_package_get_deps (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->deps;
}

/**
 * cra_package_set_name:
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
 **/
void
cra_package_set_epoch (CraPackage *pkg, guint epoch)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	priv->epoch = epoch;
}

/**
 * cra_package_set_url:
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
 **/
gboolean
cra_package_open (CraPackage *pkg, const gchar *filename, GError **error)
{
	CraPackageClass *klass = CRA_PACKAGE_GET_CLASS (pkg);
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	gboolean ret = TRUE;

	/* cache here */
	priv->filename = g_strdup (filename);
	priv->basename = g_path_get_basename (filename);

	/* call distro-specific method */
	if (klass->open != NULL) {
		ret = klass->open (pkg, filename, error);
		goto out;
	}
out:
	return ret;
}

/**
 * cra_package_explode:
 **/
gboolean
cra_package_explode (CraPackage *pkg,
		     const gchar *dir,
		     GPtrArray *glob,
		     GError **error)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar buf[PATH_MAX];
	gchar *data = NULL;
	gsize len;
	int r;
	struct archive *arch = NULL;
	struct archive_entry *entry;

	/* load file at once to avoid seeking */
	ret = g_file_get_contents (priv->filename, &data, &len, error);
	if (!ret)
		goto out;

	/* read anything */
	arch = archive_read_new ();
	archive_read_support_format_all (arch);
	archive_read_support_filter_all (arch);
	r = archive_read_open_memory (arch, data, len);
	if (r) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Cannot open: %s",
			     archive_error_string (arch));
		goto out;
	}

	/* decompress each file */
	for (;;) {
		r = archive_read_next_header (arch, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Cannot read header: %s",
				     archive_error_string (arch));
			goto out;
		}

		/* no output file */
		if (archive_entry_pathname (entry) == NULL)
			continue;

		/* do we have to decompress this file */
		if (glob != NULL) {
			tmp = archive_entry_pathname (entry);
			if (tmp[0] == '.')
				tmp++;
			if (cra_glob_value_search (glob, tmp) == NULL)
				continue;
		}

		/* update output path */
		g_snprintf (buf, PATH_MAX, "%s/%s",
			    dir, archive_entry_pathname (entry));
		archive_entry_update_pathname_utf8 (entry, buf);

		/* update hardlinks */
		tmp = archive_entry_hardlink (entry);
		if (tmp != NULL) {
			g_snprintf (buf, PATH_MAX, "%s/%s", dir, tmp);
			archive_entry_update_hardlink_utf8 (entry, buf);
		}

		/* update symlinks */
		tmp = archive_entry_symlink (entry);
		if (tmp != NULL) {
			g_snprintf (buf, PATH_MAX, "%s/%s", dir, tmp);
			archive_entry_update_symlink_utf8 (entry, buf);
		}

		r = archive_read_extract (arch, entry, 0);
		if (r != ARCHIVE_OK) {
			ret = FALSE;
			g_set_error (error,
				     CRA_PLUGIN_ERROR,
				     CRA_PLUGIN_ERROR_FAILED,
				     "Cannot extract: %s",
				     archive_error_string (arch));
			goto out;
		}
	}
out:
	g_free (data);
	if (arch != NULL) {
		archive_read_close (arch);
		archive_read_free (arch);
	}
	return ret;
}

/**
 * cra_package_set_config:
 **/
void
cra_package_set_config (CraPackage *pkg, const gchar *key, const gchar *value)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	g_hash_table_insert (priv->configs, g_strdup (key), g_strdup (value));
}

/**
 * cra_package_get_config:
 **/
const gchar *
cra_package_get_config (CraPackage *pkg, const gchar *key)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return g_hash_table_lookup (priv->configs, key);
}

/**
 * cra_package_get_releases:
 **/
GPtrArray *
cra_package_get_releases (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->releases;
}

/**
 * cra_package_compare:
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
 **/
AsRelease *
cra_package_get_release	(CraPackage *pkg, const gchar *version)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return g_hash_table_lookup (priv->releases_hash, version);
}

/**
 * cra_package_add_release:
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
