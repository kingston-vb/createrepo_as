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

#include "cra-release.h"
#include "cra-dom.h"

typedef struct _CraReleasePrivate	CraReleasePrivate;
struct _CraReleasePrivate
{
	gchar		*version;
	guint64		 timestamp;
	GString		*text;
};

G_DEFINE_TYPE_WITH_PRIVATE (CraRelease, cra_release, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (cra_release_get_instance_private (o))

/**
 * cra_release_finalize:
 **/
static void
cra_release_finalize (GObject *object)
{
	CraRelease *release = CRA_RELEASE (object);
	CraReleasePrivate *priv = GET_PRIVATE (release);

	g_free (priv->version);
	g_string_free (priv->text, TRUE);

	G_OBJECT_CLASS (cra_release_parent_class)->finalize (object);
}

/**
 * cra_release_init:
 **/
static void
cra_release_init (CraRelease *release)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	priv->text = g_string_new ("");
}

/**
 * cra_release_class_init:
 **/
static void
cra_release_class_init (CraReleaseClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = cra_release_finalize;
}

/**
 * cra_release_set_version:
 **/
void
cra_release_set_version (CraRelease *release, const gchar *version)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	priv->version = g_strdup (version);
}

/**
 * cra_release_set_timestamp:
 **/
void
cra_release_set_timestamp (CraRelease *release, guint64 timestamp)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	priv->timestamp = timestamp;
}

/**
 * cra_release_add_text:
 **/
void
cra_release_add_text (CraRelease *release, const gchar *text)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	g_string_append_printf (priv->text, "%s\n", text);
}

/**
 * cra_release_get_version:
 **/
const gchar *
cra_release_get_version (CraRelease *release)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	return priv->version;
}

/**
 * cra_release_get_timestamp:
 **/
guint64
cra_release_get_timestamp (CraRelease *release)
{
	CraReleasePrivate *priv = GET_PRIVATE (release);
	return priv->timestamp;
}

/**
 * cra_release_new:
 **/
CraRelease *
cra_release_new (void)
{
	CraRelease *release;
	release = g_object_new (CRA_TYPE_RELEASE, NULL);
	return CRA_RELEASE (release);
}
