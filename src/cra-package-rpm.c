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
 * SECTION:cra-package-rpm
 * @short_description: Object representing a .RPM package file.
 * @stability: Unstable
 *
 * This object represents one .rpm package file.
 */

#include "config.h"

#include <limits.h>
#include <archive.h>
#include <archive_entry.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>

#include "cra-cleanup.h"
#include "cra-package-rpm.h"
#include "cra-plugin.h"

typedef struct _CraPackageRpmPrivate	CraPackageRpmPrivate;
struct _CraPackageRpmPrivate
{
	Header		 h;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraPackageRpm, cra_package_rpm, CRA_TYPE_PACKAGE)

#define GET_PRIVATE(o) (cra_package_rpm_get_instance_private (o))

/**
 * cra_package_rpm_finalize:
 **/
static void
cra_package_rpm_finalize (GObject *object)
{
	CraPackageRpm *pkg = CRA_PACKAGE_RPM (object);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg);

	headerFree (priv->h);

	G_OBJECT_CLASS (cra_package_rpm_parent_class)->finalize (object);
}

/**
 * cra_package_rpm_init:
 **/
static void
cra_package_rpm_init (CraPackageRpm *pkg)
{
}

/**
 * cra_package_rpm_set_license:
 **/
static void
cra_package_rpm_set_license (CraPackage *pkg, const gchar *license)
{
	const gchar *tmp;
	guint i;
	guint j;
	_cleanup_strv_free_ gchar **split = NULL;
	_cleanup_string_free_ GString *new = NULL;
	struct {
		const gchar	*fedora;
		const gchar	*spdx;
	} convert[] =  {
		{ "AGPLv3",			"AGPL-3.0" },
		{ "AGPLv3+",			"AGPL-3.0" },
		{ "AGPLv3 with exceptions",	"AGPL-3.0" },
		{ "AGPLv3+ with exceptions",	"AGPL-3.0" },
		{ "Array",			NULL },
		{ "Artistic 2.0",		"Artistic-2.0" },
		{ "Artistic",			"Artistic-1.0" },
		{ "Artistic clarified",		"Artistic-2.0" },
		{ "ASL 1.1",			"Apache-1.1" },
		{ "ASL 2.0",			"Apache-2.0" },
		{ "Baekmuk",			NULL },
		{ "Bitstream Vera",		NULL },
		{ "Boost",			"BSL-1.0" },
		{ "BSD",			"BSD-3-Clause" },
		{ "BSD with advertising",	"BSD-3-Clause" },
		{ "CC0",			"CC0-1.0" },
		{ "CC-BY",			"CC-BY-3.0" },
		{ "CC-BY-SA",			"CC-BY-SA-3.0" },
		{ "CDDL",			"CDDL-1.0" },
		{ "CeCILL-C",			"CECILL-C" },
		{ "CeCILL",			"CECILL-2.0" },
		{ "Copyright only",		NULL },
		{ "Crystal Stacker",		NULL },
		{ "EPL",			"EPL-1.0" },
		{ "Free Art",			"ClArtistic" },
		{ "Freely redistributable without restriction",	NULL },
		{ "GFDL",			"GFDL-1.3" },
		{ "GPL+",			"GPL-1.0+" },
		{ "GPLv2",			"GPL-2.0" },
		{ "GPLv2+",			"GPL-2.0+" },
		{ "GPLV2",			"GPL-2.0" },
		{ "GPLv2 with exceptions",	"GPL-2.0-with-font-exception" },
		{ "GPLv2+ with exceptions",	"GPL-2.0-with-font-exception" },
		{ "GPLv3",			"GPL-3.0" },
		{ "GPLv3+",			"GPL-3.0+" },
		{ "GPLV3+",			"GPL-3.0+" },
		{ "GPLv3+ with exceptions",	"GPL-3.0+" },
		{ "GPLv3 with exceptions",	"GPL-3.0-with-GCC-exception" },
		{ "GPL+ with exceptions",	"GPL-2.0-with-font-exception" },
		{ "IBM",			"IPL-1.0" },
		{ "LGPL+",			"LGPL-2.1+" },
		{ "LGPLv2.1",			"LGPL-2.1" },
		{ "LGPLv2",			"LGPL-2.1" },
		{ "LGPLv2+",			"LGPL-2.1+" },
		{ "LGPLv2 with exceptions",	"LGPL-2.0" },
		{ "LGPLv2+ with exceptions",	"LGPL-2.0+" },
		{ "LGPLv3",			"LGPL-3.0" },
		{ "LGPLv3+",			"LGPL-3.0+" },
		{ "Liberation",			NULL },
		{ "LPPL",			"LPPL-1.3c" },
		{ "MgOpen",			NULL },
		{ "MIT with advertising",	"MIT" },
		{ "mplus",			NULL },
		{ "MPLv1.0",			"MPL-1.0" },
		{ "MPLv1.1",			"MPL-1.1" },
		{ "MPLv2.0",			"MPL-2.0" },
		{ "Netscape",			"NPL-1.1" },
		{ "OFL",			"OFL-1.1" },
		{ "Public domain",		NULL },
		{ "Public Domain",		NULL },
		{ "Python",			"Python-2.0" },
		{ "QPL",			"QPL-1.0" },
		{ "QPL with exceptions",	"QPL-1.0" },
		{ "SPL",			"SPL-1.0" },
		{ "zlib",			"Zlib" },
		{ "ZPLv2.0",			"ZPL-2.0" },
		{ NULL, NULL } };

	/* this isn't supposed to happen */
	if (license == NULL) {
		cra_package_log (pkg, CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "no license!");
		return;
	}

	/* tokenize the license string and try to convert the Fedora license
	 * string to a SPDX license the best we can */
	new = g_string_sized_new (strlen (license) * 2);
	split = as_utils_spdx_license_tokenize (license);
	for (i = 0; split[i] != NULL; i++) {

		/* convert */
		tmp = split[i];
		for (j = 0; convert[j].fedora != NULL; j++) {
			if (g_strcmp0 (split[i], convert[j].fedora) == 0) {
				tmp = convert[j].spdx;
				cra_package_log (pkg,
						 CRA_PACKAGE_LOG_LEVEL_DEBUG,
						 "Converting Fedora license "
						 "'%s' to SPDX '%s'",
						 convert[j].fedora,
						 convert[j].spdx);
				break;
			}
		}

		/* any operation */
		if (g_str_has_prefix (split[i], "#")) {
			g_string_append (new, split[i] + 1);
			continue;
		}

		/* no matching SPDX entry */
		if (tmp == NULL) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "Unable to currently map Fedora "
					 "license '%s' to SPDX", split[i]);
			tmp = split[i];
		} else if (!as_utils_is_spdx_license_id (tmp)) {
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_WARNING,
					 "License '%s' is not an SPDX ID", tmp);
		}
		g_string_append (new, tmp);
	}

	cra_package_set_license (pkg, new->str);
}

/**
 * cra_package_rpm_set_source:
 **/
static void
cra_package_rpm_set_source (CraPackage *pkg, const gchar *source)
{
	gchar *tmp;
	_cleanup_free_ gchar *srcrpm = NULL;

	/* this isn't supposed to happen */
	if (source == NULL) {
		cra_package_log (pkg, CRA_PACKAGE_LOG_LEVEL_WARNING,
				 "no source!");
		return;
	}
	srcrpm = g_strdup (source);
	tmp = g_strstr_len (srcrpm, -1, ".src.rpm");
	if (tmp != NULL)
		*tmp = '\0';
	cra_package_set_source (pkg, srcrpm);
}

/**
 * cra_package_rpm_ensure_simple:
 **/
static gboolean
cra_package_rpm_ensure_simple (CraPackage *pkg, GError **error)
{
	CraPackageRpm *pkg_rpm = CRA_PACKAGE_RPM (pkg);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg_rpm);
	gboolean ret = TRUE;
	rpmtd td;

	/* get the simple stuff */
	td = rpmtdNew ();
	headerGet (priv->h, RPMTAG_NAME, td, HEADERGET_MINMEM);
	cra_package_set_name (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_VERSION, td, HEADERGET_MINMEM);
	cra_package_set_version (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_RELEASE, td, HEADERGET_MINMEM);
	cra_package_set_release (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_ARCH, td, HEADERGET_MINMEM);
	cra_package_set_arch (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_EPOCH, td, HEADERGET_MINMEM);
	cra_package_set_epoch (pkg, rpmtdGetNumber (td));
	headerGet (priv->h, RPMTAG_URL, td, HEADERGET_MINMEM);
	cra_package_set_url (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_LICENSE, td, HEADERGET_MINMEM);
	cra_package_rpm_set_license (pkg, rpmtdGetString (td));
	headerGet (priv->h, RPMTAG_SOURCERPM, td, HEADERGET_MINMEM);
	cra_package_rpm_set_source (pkg, rpmtdGetString (td));
	rpmtdFree (td);
	return ret;
}

/**
 * cra_package_rpm_release_set_text:
 **/
static gboolean
cra_package_rpm_release_set_text (AsRelease *release,
					 const gchar *text)
{
	guint i;
	_cleanup_free_ gchar *markup = NULL;
	const gchar *blacklisted[] = { " BR ",
				       " >= ",
				       "BuildRequires",
				       "Buildroot",
				       "Bump release",
				       "compile fixes",
				       "%configure",
				       "%doc",
				       "ExcludeArch",
				       "fix build",
				       "fix typo",
				       "FTBFS",
				       "initial version",
				       "Latest upstream",
				       "missing BR",
				       "New release",
				       "New upstream",
				       "New version",
				       "%post",
				       "rebuild",
				       "Rebuild",
				       "rebuilt",
				       "Rebuilt",
				       " Requires ",
				       "revbump",
				       "scriptlets",
				       "spec file",
				       "subpackage",
				       "Updated to ",
				       "Update to ",
				       "Upgrade to ",
				       "Upstream new release",
				       "upstream release",
				       "Upstream update",
				       "vendor prefix",
				       as_release_get_version (release),
				       NULL };
	for (i = 0; blacklisted[i] != NULL; i++) {
		if (g_strstr_len (text, -1, blacklisted[i]) != NULL)
			return FALSE;
	}
	/* remove prefix */
	if (g_str_has_prefix (text, "- "))
		text += 2;
	markup = g_markup_printf_escaped ("<p>%s</p>", text);
	as_release_set_description (release, NULL, markup, -1);
	return TRUE;
}

/**
 * cra_package_rpm_add_release:
 **/
static void
cra_package_rpm_add_release (CraPackage *pkg,
			     guint64 timestamp,
			     const gchar *name,
			     const gchar *text)
{
	AsRelease *release;
	const gchar *version;
	_cleanup_free_ gchar *name_dup;
	gchar *tmp;
	gchar *vr;

	/* get last string chunk */
	name_dup = g_strchomp (g_strdup (name));
	vr = g_strrstr (name_dup, " ");
	if (vr == NULL)
		return;

	/* get last string chunk */
	version = vr + 1;
	tmp = g_strstr_len (version, -1, "-");
	if (tmp == NULL) {
		if (g_strstr_len (version, -1, ">") != NULL)
			return;
	} else {
		*tmp = '\0';
	}

	/* remove any epoch */
	tmp = g_strstr_len (version, -1, ":");
	if (tmp != NULL)
		version = tmp + 1;

	/* is version already in the database */
	release = cra_package_get_release (pkg, version);
	if (release != NULL) {
		/* use the earlier timestamp to ignore auto-rebuilds with just
		 * a bumped release */
		if (timestamp < as_release_get_timestamp (release))
			as_release_set_timestamp (release, timestamp);

		/* we didn't have anything interesting before; try now */
		if (as_release_get_description (release, NULL) == NULL)
			cra_package_rpm_release_set_text (release, text);
	} else {
		release = as_release_new ();
		as_release_set_version (release, version, -1);
		as_release_set_timestamp (release, timestamp);
		cra_package_rpm_release_set_text (release, text);
		cra_package_add_release (pkg, version, release);
		g_object_unref (release);
	}
}

/**
 * cra_package_rpm_ensure_releases:
 **/
static gboolean
cra_package_rpm_ensure_releases (CraPackage *pkg, GError **error)
{
	CraPackageRpm *pkg_rpm = CRA_PACKAGE_RPM (pkg);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg_rpm);
	guint i;
	rpmtd td[3] = { NULL, NULL, NULL };

	/* read out the file list */
	for (i = 0; i < 3; i++)
		td[i] = rpmtdNew ();
	/* get the ChangeLog info */
	headerGet (priv->h, RPMTAG_CHANGELOGTIME, td[0], HEADERGET_MINMEM);
	headerGet (priv->h, RPMTAG_CHANGELOGNAME, td[1], HEADERGET_MINMEM);
	headerGet (priv->h, RPMTAG_CHANGELOGTEXT, td[2], HEADERGET_MINMEM);
	while (rpmtdNext (td[0]) != -1 &&
	       rpmtdNext (td[1]) != -1 &&
	       rpmtdNext (td[2]) != -1) {
		cra_package_rpm_add_release (pkg,
					     rpmtdGetNumber (td[0]),
					     rpmtdGetString (td[1]),
					     rpmtdGetString (td[2]));
	}
	for (i = 0; i < 3; i++) {
		rpmtdFreeData (td[i]);
		rpmtdFree (td[i]);
	}
	return TRUE;
}

/**
 * cra_package_rpm_ensure_deps:
 **/
static gboolean
cra_package_rpm_ensure_deps (CraPackage *pkg, GError **error)
{
	CraPackageRpm *pkg_rpm = CRA_PACKAGE_RPM (pkg);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg_rpm);
	const gchar *dep;
	gboolean ret = TRUE;
	gchar *tmp;
	gint rc;
	guint i = 0;
	rpmtd td = NULL;
	_cleanup_strv_free_ gchar **deps = NULL;

	/* read out the dep list */
	td = rpmtdNew ();
	rc = headerGet (priv->h, RPMTAG_REQUIRENAME, td, HEADERGET_MINMEM);
	if (!rc) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read list of requires %s",
			     cra_package_get_filename (pkg));
		goto out;
	}
	deps = g_new0 (gchar *, rpmtdCount (td) + 1);
	while (rpmtdNext (td) != -1) {
		dep = rpmtdGetString (td);
		if (g_str_has_prefix (dep, "rpmlib"))
			continue;
		if (g_strcmp0 (dep, "/bin/sh") == 0)
			continue;
		deps[i] = g_strdup (dep);
		tmp = g_strstr_len (deps[i], -1, "(");
		if (tmp != NULL)
			*tmp = '\0';
		/* TODO: deduplicate */
		i++;
	}
	cra_package_set_deps (pkg, deps);
out:
	rpmtdFreeData (td);
	rpmtdFree (td);
	return ret;
}

/**
 * cra_package_rpm_ensure_filelists:
 **/
static gboolean
cra_package_rpm_ensure_filelists (CraPackage *pkg, GError **error)
{
	CraPackageRpm *pkg_rpm = CRA_PACKAGE_RPM (pkg);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg_rpm);
	gboolean ret = TRUE;
	gint rc;
	guint i;
	rpmtd td[3] = { NULL, NULL, NULL };
	_cleanup_free_ const gchar **dirnames = NULL;
	_cleanup_free_ gint32 *dirindex = NULL;
	_cleanup_strv_free_ gchar **filelist = NULL;

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
			     cra_package_get_filename (pkg));
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
	filelist = g_new0 (gchar *, rpmtdCount (td[1]) + 1);
	while (rpmtdNext (td[1]) != -1) {
		filelist[i] = g_build_filename (dirnames[dirindex[i]],
						rpmtdGetString (td[1]),
						NULL);
		i++;
	}
	cra_package_set_filelist (pkg, filelist);
out:
	for (i = 0; i < 3; i++) {
		rpmtdFreeData (td[i]);
		rpmtdFree (td[i]);
	}
	return ret;
}

/**
 * cra_package_rpm_strerror:
 **/
static const gchar *
cra_package_rpm_strerror (rpmRC rc)
{
	const gchar *str;
	switch (rc) {
	case RPMRC_OK:
		str = "Generic success";
		break;
	case RPMRC_NOTFOUND:
		str = "Generic not found";
		break;
	case RPMRC_FAIL:
		str = "Generic failure";
		break;
	case RPMRC_NOTTRUSTED:
		str = "Signature is OK, but key is not trusted";
		break;
	case RPMRC_NOKEY:
		str = "Public key is unavailable";
		break;
	default:
		str = "unknown";
		break;
	}
	return str;
}

/**
 * cra_package_rpm_open:
 **/
static gboolean
cra_package_rpm_open (CraPackage *pkg, const gchar *filename, GError **error)
{
	CraPackageRpm *pkg_rpm = CRA_PACKAGE_RPM (pkg);
	CraPackageRpmPrivate *priv = GET_PRIVATE (pkg_rpm);
	FD_t fd;
	gboolean ret = TRUE;
	rpmRC rc;
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
	rc = rpmReadPackageFile (ts, fd, filename, &priv->h);
	if (rc == RPMRC_FAIL) {
		ret = FALSE;
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package %s: %s",
			     filename, cra_package_rpm_strerror (rc));
		goto out;
	}

	/* read package stuff */
	ret = cra_package_rpm_ensure_simple (pkg, error);
	if (!ret)
		goto out;
	ret = cra_package_rpm_ensure_releases (pkg, error);
	if (!ret)
		goto out;
	ret = cra_package_rpm_ensure_deps (pkg, error);
	if (!ret)
		goto out;
	ret = cra_package_rpm_ensure_filelists (pkg, error);
	if (!ret)
		goto out;
out:
	rpmtsFree (ts);
	Fclose (fd);
	return ret;
}

/**
 * cra_package_rpm_compare:
 **/
static gint
cra_package_rpm_compare (CraPackage *pkg1, CraPackage *pkg2)
{
	return rpmvercmp (cra_package_get_evr (pkg1),
			  cra_package_get_evr (pkg2));
}

/**
 * cra_package_rpm_class_init:
 **/
static void
cra_package_rpm_class_init (CraPackageRpmClass *klass)
{
	CraPackageClass *package_class = CRA_PACKAGE_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cra_package_rpm_finalize;
	package_class->open = cra_package_rpm_open;
	package_class->compare = cra_package_rpm_compare;
}

/**
 * cra_package_rpm_init_cb:
 **/
static gpointer
cra_package_rpm_init_cb (gpointer user_data)
{
	rpmReadConfigFiles (NULL, NULL);
	return NULL;
}

/**
 * cra_package_rpm_new:
 *
 * Creates a new RPM package.
 *
 * Returns: a package
 *
 * Since: 0.1.0
 **/
CraPackage *
cra_package_rpm_new (void)
{
	CraPackage *pkg;
	static GOnce rpm_init = G_ONCE_INIT;
	g_once (&rpm_init, cra_package_rpm_init_cb, NULL);
	pkg = g_object_new (CRA_TYPE_PACKAGE_RPM, NULL);
	return CRA_PACKAGE (pkg);
}
