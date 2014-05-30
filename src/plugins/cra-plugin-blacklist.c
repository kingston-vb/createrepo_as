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

#include <cra-plugin.h>

struct CraPluginPrivate {
	GPtrArray	*vetos;
};

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "blacklist";
}

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	guint i;
	struct {
		const gchar	*id;
		const gchar	*reason;
	} blacklist[] =  {
		{ "active-*",			"Not an application" },
		{ "ailurus",			"Upstream abandoned" },
		{ "authconfig",			"Not an application" },
		{ "bareftp",			"Upstream abandoned, see: http://bareftp.eide-itc.no/news/?p=228" },
		{ "bf-*-editor",		"Not an application" },
		{ "birdie",			"Upstream abandoned, see: http://birdieapp.github.io/2014/02/26/time-to-say-goodbye.html" },
		{ "bitmap2component",		"Not an application" },
		{ "bted",			"Not an application" },
		{ "caja-home",			"Not an application" },
		{ "chmsee",			"Upstream abandoned, see: https://code.google.com/p/chmsee/" },
		{ "chromium-bsu",		"Upstream abandoned, see: http://chromium-bsu.sourceforge.net/faq.htm#q11" },
		{ "cinnamon-settings",		"Not an application" },
		{ "coccinella",			"Upstream abandoned" },
		{ "conduit",			"Upstream abandoned" },
		{ "*-demo",			"Not an application" },
		{ "display-properties",		"Not an application" },
		{ "doom-shareware",		"Upstream abandoned, see: https://www.ohloh.net/p/8278" },
		{ "emesene",			"Upstream abandoned, see: https://github.com/emesene/emesene/issues/1588" },
		{ "freedink-dfarc",		"Not an application" },
		{ "freedinkedit",		"Not an application" },
		{ "fusion-icon",		"Upstream abandoned" },
		{ "gcompris-edit",		"Not an application" },
		{ "glade3",			"Not an application" },
		{ "gnome-dasher",		"Upstream abandoned" },
		{ "gnome-glade-2",		"Not an application" },
		{ "gnome-system-monitor-kde",	"Not an application" },
		{ "gnome-wacom-panel",		"Not an application" },
		{ "kupfer",			"Upstream abandoned" },
		{ "listen",			"Upstream abandoned" },
		{ "logjam",			"Upstream abandoned, see: http://andy-shev.github.io/LogJam/dev/" },
		{ "luckybackup-*",		"Not an application" },
		{ "lxde-desktop-preferences",	"Not an application" },
		{ "lxinput",			"Not an application" },
		{ "lxrandr",			"Not an application" },
		{ "manaplustest",		"Not an application" },
		{ "mana",			"Upstream abandoned, private email" },
		{ "mate-*",			"Not an application" },
		{ "megaglest_*",		"Not an application" },
		{ "midori-private",		"Not an application" },
		{ "mm3d",			"Upstream abandoned, see: http://www.misfitcode.com/misfitmodel3d/" },
		{ "nekobee",			"Upstream abandoned" },
		{ "nicotine",			"Upstream abandoned" },
		{ "nm-connection-editor",	"Not an application" },
		{ "pioneers-editor",		"Not an application" },
		{ "postler",			"Upstream abandoned, see: https://launchpad.net/postler" },
		{ "qterminal_*",		"Not an application" },
		{ "rasterview",			"Upstream abandoned" },
		{ "razor-config*",		"Not an application" },
		{ "redhat-userinfo",		"Not an application" },
		{ "redhat-usermount",		"Not an application" },
		{ "redhat-userpasswd",		"Not an application" },
		{ "*-release-notes",		"Not an application" },
		{ "resapplet",			"Obsolete, see: https://mail.gnome.org/archives/gnome-bugsquad/2011-June/msg00000.html" },
		{ "Rodent-*",			"Not an application" },
		{ "scantailor",			"Upstream abandoned, see: http://www.diybookscanner.org/forum/viewtopic.php?f=21&t=2979" },
		{ "schismtracker",		"Upstream abandoned, see: http://www.nimh.org/" },
		{ "*-server",			"Not an application" },
		{ "*-session-manager",		"Not an application" },
		{ "*-shareware",		"Not an application" },
		{ "sigil",			"Upstream abandoned, see: http://sigildev.blogspot.co.uk/2014/02/sigils-spiritual-successor.html" },
		{ "spacefm*",			"Upstream abandoned, see: http://igurublog.wordpress.com/2014/04/28/ignorantgurus-hiatus/" },
		{ "specto",			"Upstream abandoned, see: http://jeff.ecchi.ca/blog/2013/03/21/a-programs-obsolescence/" },
		{ "system-config-date",		"Not an application" },
		{ "system-config-*",		"Not an application" },
		{ "transgui",			"Not an application" },
		{ "vkeybd",			"Upstream abandoned" },
		{ "xfce4-about",		"Not an application" },
		{ "xfce4-session-logout",	"Not an application" },
		{ "xfce4-settings-editor",	"Not an application" },
		{ "xfce4-*-settings",		"Not an application" },
		{ "xfce-settings-manager",	"Not an application" },
		{ "xfce-ui-settings",		"Not an application" },
		{ "xinput_calibrator",		"Not an application" },
		{ "xpilot-ng-x11",		"Not an application" },
		{ NULL, NULL } };

	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
	plugin->priv->vetos = cra_glob_value_array_new ();

	/* add each entry */
	for (i = 0; blacklist[i].id != NULL; i++) {
		g_ptr_array_add (plugin->priv->vetos,
				 cra_glob_value_new (blacklist[i].id,
						     blacklist[i].reason));
	}
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
	g_ptr_array_unref (plugin->priv->vetos);
}

/**
 * cra_plugin_process_app:
 */
gboolean
cra_plugin_process_app (CraPlugin *plugin,
			CraPackage *pkg,
			CraApp *app,
			const gchar *tmpdir,
			GError **error)
{
	const gchar *tmp;
	tmp = cra_glob_value_search (plugin->priv->vetos,
				     as_app_get_id (AS_APP (app)));
	if (tmp != NULL)
		cra_app_add_veto (app, "%s", tmp);
	return TRUE;
}
