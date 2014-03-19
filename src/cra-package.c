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

#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>

#include "cra-package.h"
#include "cra-plugin.h"

typedef struct _CraPackagePrivate	CraPackagePrivate;
struct _CraPackagePrivate
{
	gboolean	 enabled;
	Header		 h;
	gchar		**filelist;
	gchar		**deps;
	gchar		*filename;
	gchar		*name;
	guint		 epoch;
	gchar		*version;
	gchar		*release;
	gchar		*arch;
	gchar		*url;
	gchar		*nevr;
	gchar		*evr;
	gchar		*license;
	gchar		*sourcerpm;
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

	headerFree (priv->h);
	g_strfreev (priv->filelist);
	g_strfreev (priv->deps);
	g_free (priv->filename);
	g_free (priv->name);
	g_free (priv->version);
	g_free (priv->release);
	g_free (priv->arch);
	g_free (priv->url);
	g_free (priv->nevr);
	g_free (priv->evr);
	g_free (priv->license);
	g_free (priv->sourcerpm);
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
		g_string_append_printf (priv->log, "INFO:    %s\n", tmp);
		break;
	case CRA_PACKAGE_LOG_LEVEL_DEBUG:
		g_string_append_printf (priv->log, "DEBUG:   %s\n", tmp);
		break;
	case CRA_PACKAGE_LOG_LEVEL_WARNING:
		g_string_append_printf (priv->log, "WARNING: %s\n", tmp);
		break;
	default:
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
	gboolean ret;
	gchar *logdir;
	gchar *logfile;
	gint rc;

	/* overwrite old log */
	logdir = g_build_filename ("./logs", cra_package_get_name (pkg), NULL);
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
 * cra_package_get_sourcerpm:
 **/
const gchar *
cra_package_get_sourcerpm (CraPackage *pkg)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	return priv->sourcerpm;
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
 * cra_package_add_release:
 **/
static void
cra_package_add_release (CraPackage *pkg,
			 guint64 timestamp,
			 const gchar *name,
			 const gchar *text)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	AsRelease *release;
	const gchar *version;
	gchar *name_dup;
	gchar *tmp;
	gchar *vr;

	/* get last string chunk */
	name_dup = g_strchomp (g_strdup (name));
	vr = g_strrstr (name_dup, " ");
	if (vr == NULL)
		goto out;

	/* get last string chunk */
	version = vr + 1;
	tmp = g_strstr_len (version, -1, "-");
	if (tmp == NULL) {
		if (g_strstr_len (version, -1, ">") != NULL)
			goto out;
	} else {
		*tmp = '\0';
	}

	/* remove any epoch */
	tmp = g_strstr_len (version, -1, ":");
	if (tmp != NULL)
		version = tmp + 1;

	/* is version already in the database */
	release = g_hash_table_lookup (priv->releases_hash, version);
	if (release != NULL) {
		/* use the earlier timestamp to ignore auto-rebuilds with just
		 * a bumped release */
		if (timestamp < as_release_get_timestamp (release))
			as_release_set_timestamp (release, timestamp);
	} else {
		release = as_release_new ();
		as_release_set_version (release, version, -1);
		as_release_set_timestamp (release, timestamp);
		as_release_set_description (release, NULL, text, -1);
		g_hash_table_insert (priv->releases_hash,
				     g_strdup (version),
				     release);
		g_ptr_array_add (priv->releases, g_object_ref (release));
	}
out:
	g_free (name_dup);
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
 * cra_package_ensure_filelist:
 **/
gboolean
cra_package_ensure_filelist (CraPackage *pkg, GError **error)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	const gchar *dep;
	const gchar **dirnames = NULL;
	gboolean ret = TRUE;
	gchar *tmp;
	gint32 *dirindex = NULL;
	gint rc;
	guint i;
	rpmtd td[3] = { NULL, NULL, NULL };

	if (priv->filelist != NULL)
		goto out;

	/* read out the file list */
	for (i = 0; i < 3; i++)
		td[i] = rpmtdNew ();
	rc = headerGet (priv->h, RPMTAG_DIRNAMES, td[0], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (priv->h, RPMTAG_BASENAMES, td[1], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (priv->h, RPMTAG_DIRINDEXES, td[2], HEADERGET_MINMEM);
	if (!rc) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package file list %s",
			     priv->filename);
		goto out;
	}
	i = 0;
	dirnames = g_new0 (const gchar *, rpmtdCount (td[0]) + 1);
	while (rpmtdNext (td[0]) != -1)
		dirnames[i++] = rpmtdGetString (td[0]);
	i = 0;
	dirindex = g_new0 (gint32, rpmtdCount (td[2]) + 1);
	while (rpmtdNext (td[2]) != -1)
		dirindex[i++] = rpmtdGetNumber (td[2]);
	i = 0;
	priv->filelist = g_new0 (gchar *, rpmtdCount (td[1]) + 1);
	while (rpmtdNext (td[1]) != -1) {
		priv->filelist[i] = g_build_filename (dirnames[dirindex[i]],
						     rpmtdGetString (td[1]),
						     NULL);
		i++;
	}
	for (i = 0; i < 3; i++)
		rpmtdFreeData (td[i]);

	/* read out the dep list */
	rc = headerGet (priv->h, RPMTAG_REQUIRENAME, td[0], HEADERGET_MINMEM);
	if (!rc) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read list of requires %s",
			     priv->filename);
		goto out;
	}
	i = 0;
	priv->deps = g_new0 (gchar *, rpmtdCount (td[0]) + 1);
	while (rpmtdNext (td[0]) != -1) {
		dep = rpmtdGetString (td[0]);
		if (g_str_has_prefix (dep, "rpmlib"))
			continue;
		if (g_strcmp0 (dep, "/bin/sh") == 0)
			continue;
		priv->deps[i] = g_strdup (dep);
		tmp = g_strstr_len (priv->deps[i], -1, "(");
		if (tmp != NULL)
			*tmp = '\0';
		/* TODO: deduplicate */
		i++;
	}
	rpmtdFreeData (td[0]);

	/* get the ChangeLog info */
	headerGet (priv->h, RPMTAG_CHANGELOGTIME, td[0], HEADERGET_MINMEM);
	headerGet (priv->h, RPMTAG_CHANGELOGNAME, td[1], HEADERGET_MINMEM);
	headerGet (priv->h, RPMTAG_CHANGELOGTEXT, td[2], HEADERGET_MINMEM);
	while (rpmtdNext (td[0]) != -1 &&
	       rpmtdNext (td[1]) != -1 &&
	       rpmtdNext (td[2]) != -1) {
		cra_package_add_release (pkg,
					 rpmtdGetNumber (td[0]),
					 rpmtdGetString (td[1]),
					 rpmtdGetString (td[2]));
	}
out:
	for (i = 0; i < 3; i++) {
		rpmtdFreeData (td[i]);
		rpmtdFree (td[i]);
	}
	g_free (dirindex);
	g_free (dirnames);
	return ret;
}

/**
 * cra_package_open:
 **/
gboolean
cra_package_open (CraPackage *pkg, const gchar *filename, GError **error)
{
	CraPackagePrivate *priv = GET_PRIVATE (pkg);
	FD_t fd;
	gboolean ret = TRUE;
	gchar *tmp;
	gint rc;
	rpmtd td = NULL;
	rpmts ts;

	/* open the file */
	ts = rpmtsCreate ();
	fd = Fopen (filename, "r");
	if (fd <= 0) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to open package %s", filename);
		goto out;
	}

	/* create package */
	priv->filename = g_strdup (filename);
	rc = rpmReadPackageFile (ts, fd, filename, &priv->h);
	if (rc != RPMRC_OK) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package %s", filename);
		goto out;
	}


	/* get the simple stuff */
	td = rpmtdNew ();
	headerGet (priv->h, RPMTAG_NAME, td, HEADERGET_MINMEM);
	priv->name = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_VERSION, td, HEADERGET_MINMEM);
	priv->version = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_RELEASE, td, HEADERGET_MINMEM);
	priv->release = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_ARCH, td, HEADERGET_MINMEM);
	priv->arch = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_EPOCH, td, HEADERGET_MINMEM);
	priv->epoch = rpmtdGetNumber (td);
	headerGet (priv->h, RPMTAG_URL, td, HEADERGET_MINMEM);
	priv->url = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_LICENSE, td, HEADERGET_MINMEM);
	priv->license = g_strdup (rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_SOURCERPM, td, HEADERGET_MINMEM);
	priv->sourcerpm = g_strdup (rpmtdGetString (td));
	tmp = g_strstr_len (priv->sourcerpm, -1, ".src.rpm");
	if (tmp != NULL)
		*tmp = '\0';
out:
	if (td != NULL)
		rpmtdFree (td);
	rpmtsFree (ts);
	Fclose (fd);
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
 * cra_package_get_releases:
 **/
gint
cra_package_compare (CraPackage *pkg1, CraPackage *pkg2)
{
	return rpmvercmp (cra_package_get_evr (pkg1),
			  cra_package_get_evr (pkg2));
}

/**
 * cra_package_new:
 **/
CraPackage *
cra_package_new (void)
{
	CraPackage *pkg;
	pkg = g_object_new (CRA_TYPE_PACKAGE, NULL);
	return CRA_PACKAGE (pkg);
}
