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
 * cra_context_add_blacklist_app_id:
 */
static void
cra_context_add_blacklist_app_id (CraContext *ctx, const gchar *id)
{
	g_ptr_array_add (ctx->blacklisted_ids, cra_glob_value_new (id, ""));
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
	ctx->blacklisted_ids = cra_glob_value_array_new ();
	ctx->plugins = cra_plugin_loader_new ();
	ctx->packages = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	ctx->extra_pkgs = cra_glob_value_array_new ();

	/* add extra data */
	cra_context_add_extra_pkg (ctx, "alliance-libs", "alliance");
	cra_context_add_extra_pkg (ctx, "beneath-a-steel-sky*", "scummvm");
	cra_context_add_extra_pkg (ctx, "coq-coqide", "coq");
	cra_context_add_extra_pkg (ctx, "drascula*", "scummvm");
	cra_context_add_extra_pkg (ctx, "efte-", "efte-common");
	cra_context_add_extra_pkg (ctx, "fcitx-*", "fcitx-data");
	cra_context_add_extra_pkg (ctx, "flight-of-the-amazon-queen", "scummvm");
	cra_context_add_extra_pkg (ctx, "gcin", "gcin-data");
	cra_context_add_extra_pkg (ctx, "hotot-gtk", "hotot-common");
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
	cra_context_add_extra_pkg (ctx, "vegastrike", "vegastrike-data");
	cra_context_add_extra_pkg (ctx, "calligra-krita", "calligra-core");

	/* add blacklisted packages */
	cra_context_add_blacklist_pkg (ctx, "beneath-a-steel-sky-cd");
	cra_context_add_blacklist_pkg (ctx, "anaconda");
	cra_context_add_blacklist_pkg (ctx, "mate-control-center");
	cra_context_add_blacklist_pkg (ctx, "lxde-common");

	/* add blacklisted applications */
	cra_context_add_blacklist_app_id (ctx, "display-properties");
	cra_context_add_blacklist_app_id (ctx, "mate-*");
	cra_context_add_blacklist_app_id (ctx, "xfce4-accessibility-settings");
	cra_context_add_blacklist_app_id (ctx, "xfce4-mime-settings");
	cra_context_add_blacklist_app_id (ctx, "xfce4-settings-editor");
	cra_context_add_blacklist_app_id (ctx, "xfce-keyboard-settings");
	cra_context_add_blacklist_app_id (ctx, "xfce-mouse-settings");
	cra_context_add_blacklist_app_id (ctx, "xfce-settings-manager");
	cra_context_add_blacklist_app_id (ctx, "xfce-ui-settings");
	cra_context_add_blacklist_app_id (ctx, "xfce4-about");
	cra_context_add_blacklist_app_id (ctx, "redhat-userpasswd");
	cra_context_add_blacklist_app_id (ctx, "redhat-userinfo");
	cra_context_add_blacklist_app_id (ctx, "system-config-date");
	cra_context_add_blacklist_app_id (ctx, "gnome-wacom-panel");
	cra_context_add_blacklist_app_id (ctx, "redhat-usermount");
	cra_context_add_blacklist_app_id (ctx, "system-config-*");
	cra_context_add_blacklist_app_id (ctx, "nm-connection-editor");
	cra_context_add_blacklist_app_id (ctx, "lxinput");
	cra_context_add_blacklist_app_id (ctx, "lxrandr");
	cra_context_add_blacklist_app_id (ctx, "xfce4-session-logout");
	cra_context_add_blacklist_app_id (ctx, "gnome-glade-2");
	cra_context_add_blacklist_app_id (ctx, "glade3");
	cra_context_add_blacklist_app_id (ctx, "active-*");
	cra_context_add_blacklist_app_id (ctx, "caja-home");
	cra_context_add_blacklist_app_id (ctx, "gcompris-edit");
	cra_context_add_blacklist_app_id (ctx, "razor-config*");
	cra_context_add_blacklist_app_id (ctx, "lxde-desktop-preferences");
	cra_context_add_blacklist_app_id (ctx, "authconfig");
	cra_context_add_blacklist_app_id (ctx, "cinnamon-settings");
	cra_context_add_blacklist_app_id (ctx, "midori-private");
	cra_context_add_blacklist_app_id (ctx, "xinput_calibrator");
	cra_context_add_blacklist_app_id (ctx, "bted");

	return ctx;
}

/**
 * cra_context_free:
 */
void
cra_context_free (CraContext *ctx)
{
	cra_plugin_loader_free (ctx->plugins);
	g_ptr_array_unref (ctx->packages);
	g_ptr_array_unref (ctx->extra_pkgs);
	g_list_foreach (ctx->apps, (GFunc) g_object_unref, NULL);
	g_list_free (ctx->apps);
	g_ptr_array_unref (ctx->blacklisted_pkgs);
	g_ptr_array_unref (ctx->blacklisted_ids);
	g_free (ctx);
}
