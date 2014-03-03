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

/**
 * cra_package_free:
 **/
void
cra_package_free (CraPackage *pkg)
{
	headerFree (pkg->h);
	g_strfreev (pkg->filelist);
	g_free (pkg->filename);
	g_free (pkg->name);
	g_free (pkg->version);
	g_free (pkg->release);
	g_free (pkg->arch);
	g_free (pkg->url);
	g_free (pkg);
}

/**
 * cra_package_ensure_filelist:
 **/
gboolean
cra_package_ensure_filelist (CraPackage *pkg, GError **error)
{
	const gchar **dirnames = NULL;
	gboolean ret = TRUE;
	gint32 *dirindex = NULL;
	gint rc;
	guint i;
	rpmtd td[3];

	if (pkg->filelist != NULL)
		goto out;

	/* read out the file list */
	for (i = 0; i < 3; i++)
		td[i] = rpmtdNew ();
	rc = headerGet (pkg->h, RPMTAG_DIRNAMES, td[0], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (pkg->h, RPMTAG_BASENAMES, td[1], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (pkg->h, RPMTAG_DIRINDEXES, td[2], HEADERGET_MINMEM);
	if (!rc) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package file list %s",
			     pkg->filename);
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
	pkg->filelist = g_new0 (gchar *, rpmtdCount (td[1]) + 1);
	while (rpmtdNext (td[1]) != -1) {
		pkg->filelist[i] = g_build_filename (dirnames[dirindex[i]],
						     rpmtdGetString (td[1]),
						     NULL);
		i++;
	}
out:
	g_free (dirindex);
	g_free (dirnames);
	return ret;
}

/**
 * cra_package_open:
 **/
CraPackage *
cra_package_open (const gchar *filename, GError **error)
{
	CraPackage *pkg = NULL;
	FD_t fd;
	gint rc;
	rpmtd td;
	rpmts ts;

	/* open the file */
	ts = rpmtsCreate ();
	fd = Fopen (filename, "r");
	if (fd <= 0) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to open package %s", filename);
		goto out;
	}

	/* create package */
	pkg = g_new0 (CraPackage, 1);
	pkg->filename = g_strdup (filename);
	rc = rpmReadPackageFile (ts, fd, filename, &pkg->h);
	if (rc != RPMRC_OK) {
		cra_package_free (pkg);
		pkg = NULL;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package %s", filename);
		goto out;
	}


	/* get the simple stuff */
	td = rpmtdNew ();
	headerGet (pkg->h, RPMTAG_NAME, td, HEADERGET_MINMEM);
	pkg->name = g_strdup (rpmtdGetString (td));
	headerGet (pkg->h, RPMTAG_VERSION, td, HEADERGET_MINMEM);
	pkg->version = g_strdup (rpmtdGetString (td));
	headerGet (pkg->h, RPMTAG_RELEASE, td, HEADERGET_MINMEM);
	pkg->release = g_strdup (rpmtdGetString (td));
	headerGet (pkg->h, RPMTAG_ARCH, td, HEADERGET_MINMEM);
	pkg->arch = g_strdup (rpmtdGetString (td));
	headerGet (pkg->h, RPMTAG_EPOCH, td, HEADERGET_MINMEM);
	pkg->epoch = rpmtdGetNumber (td);
	headerGet (pkg->h, RPMTAG_URL, td, HEADERGET_MINMEM);
	pkg->url = g_strdup (rpmtdGetString (td));
out:
	rpmtsFree (ts);
	Fclose (fd);
	return pkg;
}

/**
 * cra_package_explode:
 **/
gboolean
cra_package_explode (CraPackage *pkg, const gchar *dir, GError **error)
{
	const gchar *tmp;
	gboolean ret = TRUE;
	gchar buf[PATH_MAX];
	gchar *data = NULL;
	gsize len;
	int r;
	struct archive *arch = NULL;
	struct archive_entry *entry;

	/* load file at once to avoid seeking */
	ret = g_file_get_contents (pkg->filename, &data, &len, error);
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

		/* no output file */
		if (archive_entry_pathname (entry) == NULL)
			continue;

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
