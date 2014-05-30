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

#include "cra-context.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-utils.h"

/**
 * cra_context_find_by_pkgname:
 */
CraPackage *
cra_context_find_by_pkgname (CraContext *ctx, const gchar *pkgname)
{
	CraPackage *pkg;
	guint i;

	for (i = 0; i < ctx->packages->len; i++) {
		pkg = g_ptr_array_index (ctx->packages, i);
		if (g_strcmp0 (cra_package_get_name (pkg), pkgname) == 0)
			return pkg;
	}
	return NULL;
}

/**
 * cra_context_add_extra_pkg:
 */
static void
cra_context_add_extra_pkg (CraContext *ctx, const gchar *pkg1, const gchar *pkg2)
{
	g_ptr_array_add (ctx->extra_pkgs, cra_glob_value_new (pkg1, pkg2));
}

/**
 * cra_context_add_blacklist_pkg:
 */
static void
cra_context_add_blacklist_pkg (CraContext *ctx, const gchar *pkg)
{
	g_ptr_array_add (ctx->blacklisted_pkgs, cra_glob_value_new (pkg, ""));
}

/**
 * cra_context_add_app:
 */
void
cra_context_add_app (CraContext *ctx, CraApp *app)
{
	g_mutex_lock (&ctx->apps_mutex);
	cra_plugin_add_app (&ctx->apps, app);
	g_mutex_unlock (&ctx->apps_mutex);
}

/**
 * cra_context_new:
 */
CraContext *
cra_context_new (void)
{
	CraContext *ctx;
	ctx = g_new0 (CraContext, 1);
	ctx->blacklisted_pkgs = cra_glob_value_array_new ();
	ctx->plugins = cra_plugin_loader_new ();
	ctx->packages = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	ctx->extra_pkgs = cra_glob_value_array_new ();
	g_mutex_init (&ctx->apps_mutex);
	ctx->old_md_cache = as_store_new ();

	/* add extra data */
	cra_context_add_extra_pkg (ctx, "alliance-libs", "alliance");
	cra_context_add_extra_pkg (ctx, "beneath-a-steel-sky*", "scummvm");
	cra_context_add_extra_pkg (ctx, "coq-coqide", "coq");
	cra_context_add_extra_pkg (ctx, "drascula*", "scummvm");
	cra_context_add_extra_pkg (ctx, "efte-*", "efte-common");
	cra_context_add_extra_pkg (ctx, "fcitx-*", "fcitx-data");
	cra_context_add_extra_pkg (ctx, "flight-of-the-amazon-queen", "scummvm");
	cra_context_add_extra_pkg (ctx, "gcin", "gcin-data");
	cra_context_add_extra_pkg (ctx, "hotot-gtk", "hotot-common");
	cra_context_add_extra_pkg (ctx, "hotot-qt", "hotot-common");
	cra_context_add_extra_pkg (ctx, "java-1.7.0-openjdk-devel", "java-1.7.0-openjdk");
	cra_context_add_extra_pkg (ctx, "kchmviewer-qt", "kchmviewer");
	cra_context_add_extra_pkg (ctx, "libreoffice-*", "libreoffice-core");
	cra_context_add_extra_pkg (ctx, "lure", "scummvm");
	cra_context_add_extra_pkg (ctx, "nntpgrab-gui", "nntpgrab-core");
	cra_context_add_extra_pkg (ctx, "projectM-*", "libprojectM-qt");
	cra_context_add_extra_pkg (ctx, "scummvm-tools", "scummvm");
	cra_context_add_extra_pkg (ctx, "speed-dreams", "speed-dreams-robots-base");
	cra_context_add_extra_pkg (ctx, "switchdesk-gui", "switchdesk");
	cra_context_add_extra_pkg (ctx, "transmission-*", "transmission-common");
	cra_context_add_extra_pkg (ctx, "calligra-krita", "calligra-core");

	/* add blacklisted packages */
	cra_context_add_blacklist_pkg (ctx, "beneath-a-steel-sky-cd");
	cra_context_add_blacklist_pkg (ctx, "anaconda");
	cra_context_add_blacklist_pkg (ctx, "mate-control-center");
	cra_context_add_blacklist_pkg (ctx, "lxde-common");
	cra_context_add_blacklist_pkg (ctx, "xscreensaver-*");
	cra_context_add_blacklist_pkg (ctx, "bmpanel2-cfg");

	return ctx;
}

/**
 * cra_context_free:
 */
void
cra_context_free (CraContext *ctx)
{
	g_object_unref (ctx->old_md_cache);
	cra_plugin_loader_free (ctx->plugins);
	g_ptr_array_unref (ctx->packages);
	g_ptr_array_unref (ctx->extra_pkgs);
	g_list_foreach (ctx->apps, (GFunc) g_object_unref, NULL);
	g_list_free (ctx->apps);
	g_ptr_array_unref (ctx->blacklisted_pkgs);
	g_ptr_array_unref (ctx->file_globs);
	g_mutex_clear (&ctx->apps_mutex);
	g_free (ctx);
}
