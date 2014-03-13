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
#include <fnmatch.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <cra-plugin.h>

/**
 * cra_plugin_get_name:
 */
const gchar *
cra_plugin_get_name (void)
{
	return "desktop";
}

/**
 * cra_plugin_add_globs:
 */
void
cra_plugin_add_globs (CraPlugin *plugin, GPtrArray *globs)
{
	cra_plugin_add_glob (globs, "/usr/share/applications/*.desktop");
	cra_plugin_add_glob (globs, "/usr/share/applications/kde4/*.desktop");
	cra_plugin_add_glob (globs, "/usr/share/icons/hicolor/*/apps/*");
	cra_plugin_add_glob (globs, "/usr/share/pixmaps/*");
	cra_plugin_add_glob (globs, "/usr/share/icons/*");
}

struct CraPluginPrivate {
	GPtrArray	*blacklist_cats;
	GHashTable	*stock_icon_names;
};

/**
 * cra_plugin_add_stock_icon:
 */
static void
cra_plugin_add_stock_icon (CraPlugin *plugin, const gchar *name)
{
	g_hash_table_insert (plugin->priv->stock_icon_names,
			     g_strdup (name),
			     GINT_TO_POINTER (1));
}

/**
 * cra_plugin_initialize:
 */
void
cra_plugin_initialize (CraPlugin *plugin)
{
	plugin->priv = CRA_PLUGIN_GET_PRIVATE (CraPluginPrivate);
	plugin->priv->stock_icon_names = g_hash_table_new_full (g_str_hash,
								g_str_equal,
								g_free,
								NULL);
	plugin->priv->blacklist_cats = cra_glob_value_array_new ();

	/* this is a heuristic */
	g_ptr_array_add (plugin->priv->blacklist_cats,
			 cra_glob_value_new ("X-*-Settings-Panel", ""));
	g_ptr_array_add (plugin->priv->blacklist_cats,
			 cra_glob_value_new ("X-*-SettingsDialog", ""));

	/* this is an incomplete list */
	cra_plugin_add_stock_icon (plugin, "accessories-calculator");
	cra_plugin_add_stock_icon (plugin, "accessories-character-map");
	cra_plugin_add_stock_icon (plugin, "accessories-dictionary");
	cra_plugin_add_stock_icon (plugin, "accessories-text-editor");
	cra_plugin_add_stock_icon (plugin, "address-book-new");
	cra_plugin_add_stock_icon (plugin, "application-exit");
	cra_plugin_add_stock_icon (plugin, "applications-accessories");
	cra_plugin_add_stock_icon (plugin, "applications-development");
	cra_plugin_add_stock_icon (plugin, "applications-engineering");
	cra_plugin_add_stock_icon (plugin, "applications-games");
	cra_plugin_add_stock_icon (plugin, "applications-graphics");
	cra_plugin_add_stock_icon (plugin, "applications-internet");
	cra_plugin_add_stock_icon (plugin, "applications-multimedia");
	cra_plugin_add_stock_icon (plugin, "applications-office");
	cra_plugin_add_stock_icon (plugin, "applications-other");
	cra_plugin_add_stock_icon (plugin, "applications-science");
	cra_plugin_add_stock_icon (plugin, "applications-system");
	cra_plugin_add_stock_icon (plugin, "applications-utilities");
	cra_plugin_add_stock_icon (plugin, "application-x-executable");
	cra_plugin_add_stock_icon (plugin, "appointment-missed");
	cra_plugin_add_stock_icon (plugin, "appointment-new");
	cra_plugin_add_stock_icon (plugin, "appointment-soon");
	cra_plugin_add_stock_icon (plugin, "audio-card");
	cra_plugin_add_stock_icon (plugin, "audio-input-microphone");
	cra_plugin_add_stock_icon (plugin, "audio-volume-high");
	cra_plugin_add_stock_icon (plugin, "audio-volume-low");
	cra_plugin_add_stock_icon (plugin, "audio-volume-medium");
	cra_plugin_add_stock_icon (plugin, "audio-volume-muted");
	cra_plugin_add_stock_icon (plugin, "audio-x-generic");
	cra_plugin_add_stock_icon (plugin, "battery");
	cra_plugin_add_stock_icon (plugin, "battery-caution");
	cra_plugin_add_stock_icon (plugin, "battery-low");
	cra_plugin_add_stock_icon (plugin, "call-start");
	cra_plugin_add_stock_icon (plugin, "call-stop");
	cra_plugin_add_stock_icon (plugin, "camera-photo");
	cra_plugin_add_stock_icon (plugin, "camera-video");
	cra_plugin_add_stock_icon (plugin, "camera-web");
	cra_plugin_add_stock_icon (plugin, "computer");
	cra_plugin_add_stock_icon (plugin, "computer");
	cra_plugin_add_stock_icon (plugin, "contact-new");
	cra_plugin_add_stock_icon (plugin, "dialog-error");
	cra_plugin_add_stock_icon (plugin, "dialog-information");
	cra_plugin_add_stock_icon (plugin, "dialog-password");
	cra_plugin_add_stock_icon (plugin, "dialog-question");
	cra_plugin_add_stock_icon (plugin, "dialog-warning");
	cra_plugin_add_stock_icon (plugin, "document-new");
	cra_plugin_add_stock_icon (plugin, "document-open");
	cra_plugin_add_stock_icon (plugin, "document-open-recent");
	cra_plugin_add_stock_icon (plugin, "document-page-setup");
	cra_plugin_add_stock_icon (plugin, "document-print");
	cra_plugin_add_stock_icon (plugin, "document-print-preview");
	cra_plugin_add_stock_icon (plugin, "document-properties");
	cra_plugin_add_stock_icon (plugin, "document-revert");
	cra_plugin_add_stock_icon (plugin, "document-save");
	cra_plugin_add_stock_icon (plugin, "document-save-as");
	cra_plugin_add_stock_icon (plugin, "document-send");
	cra_plugin_add_stock_icon (plugin, "drive-harddisk");
	cra_plugin_add_stock_icon (plugin, "drive-optical");
	cra_plugin_add_stock_icon (plugin, "drive-removable-media");
	cra_plugin_add_stock_icon (plugin, "drive-removable-media");
	cra_plugin_add_stock_icon (plugin, "edit-clear");
	cra_plugin_add_stock_icon (plugin, "edit-copy");
	cra_plugin_add_stock_icon (plugin, "edit-cut");
	cra_plugin_add_stock_icon (plugin, "edit-delete");
	cra_plugin_add_stock_icon (plugin, "edit-find");
	cra_plugin_add_stock_icon (plugin, "edit-find-replace");
	cra_plugin_add_stock_icon (plugin, "edit-paste");
	cra_plugin_add_stock_icon (plugin, "edit-redo");
	cra_plugin_add_stock_icon (plugin, "edit-select-all");
	cra_plugin_add_stock_icon (plugin, "edit-undo");
	cra_plugin_add_stock_icon (plugin, "emblem-default");
	cra_plugin_add_stock_icon (plugin, "emblem-documents");
	cra_plugin_add_stock_icon (plugin, "emblem-downloads");
	cra_plugin_add_stock_icon (plugin, "emblem-favorite");
	cra_plugin_add_stock_icon (plugin, "emblem-important");
	cra_plugin_add_stock_icon (plugin, "emblem-mail");
	cra_plugin_add_stock_icon (plugin, "emblem-photos");
	cra_plugin_add_stock_icon (plugin, "emblem-readonly");
	cra_plugin_add_stock_icon (plugin, "emblem-shared");
	cra_plugin_add_stock_icon (plugin, "emblem-symbolic-link");
	cra_plugin_add_stock_icon (plugin, "emblem-synchronized");
	cra_plugin_add_stock_icon (plugin, "emblem-system");
	cra_plugin_add_stock_icon (plugin, "emblem-unreadable");
	cra_plugin_add_stock_icon (plugin, "face-angel");
	cra_plugin_add_stock_icon (plugin, "face-angry");
	cra_plugin_add_stock_icon (plugin, "face-cool");
	cra_plugin_add_stock_icon (plugin, "face-crying");
	cra_plugin_add_stock_icon (plugin, "face-devilish");
	cra_plugin_add_stock_icon (plugin, "face-embarrassed");
	cra_plugin_add_stock_icon (plugin, "face-glasses");
	cra_plugin_add_stock_icon (plugin, "face-kiss");
	cra_plugin_add_stock_icon (plugin, "face-laugh");
	cra_plugin_add_stock_icon (plugin, "face-monkey");
	cra_plugin_add_stock_icon (plugin, "face-plain");
	cra_plugin_add_stock_icon (plugin, "face-raspberry");
	cra_plugin_add_stock_icon (plugin, "face-sad");
	cra_plugin_add_stock_icon (plugin, "face-sick");
	cra_plugin_add_stock_icon (plugin, "face-smile");
	cra_plugin_add_stock_icon (plugin, "face-smile-big");
	cra_plugin_add_stock_icon (plugin, "face-smirk");
	cra_plugin_add_stock_icon (plugin, "face-surprise");
	cra_plugin_add_stock_icon (plugin, "face-tired");
	cra_plugin_add_stock_icon (plugin, "face-uncertain");
	cra_plugin_add_stock_icon (plugin, "face-wink");
	cra_plugin_add_stock_icon (plugin, "face-worried");
	cra_plugin_add_stock_icon (plugin, "flag-aa");
	cra_plugin_add_stock_icon (plugin, "folder");
	cra_plugin_add_stock_icon (plugin, "folder-drag-accept");
	cra_plugin_add_stock_icon (plugin, "folder-new");
	cra_plugin_add_stock_icon (plugin, "folder-open");
	cra_plugin_add_stock_icon (plugin, "folder-remote");
	cra_plugin_add_stock_icon (plugin, "folder-visiting");
	cra_plugin_add_stock_icon (plugin, "font-x-generic");
	cra_plugin_add_stock_icon (plugin, "format-indent-less");
	cra_plugin_add_stock_icon (plugin, "format-indent-more");
	cra_plugin_add_stock_icon (plugin, "format-justify-center");
	cra_plugin_add_stock_icon (plugin, "format-justify-fill");
	cra_plugin_add_stock_icon (plugin, "format-justify-left");
	cra_plugin_add_stock_icon (plugin, "format-justify-right");
	cra_plugin_add_stock_icon (plugin, "format-text-bold");
	cra_plugin_add_stock_icon (plugin, "format-text-direction-ltr");
	cra_plugin_add_stock_icon (plugin, "format-text-direction-rtl");
	cra_plugin_add_stock_icon (plugin, "format-text-italic");
	cra_plugin_add_stock_icon (plugin, "format-text-strikethrough");
	cra_plugin_add_stock_icon (plugin, "format-text-underline");
	cra_plugin_add_stock_icon (plugin, "go-bottom");
	cra_plugin_add_stock_icon (plugin, "go-down");
	cra_plugin_add_stock_icon (plugin, "go-first");
	cra_plugin_add_stock_icon (plugin, "go-home");
	cra_plugin_add_stock_icon (plugin, "go-jump");
	cra_plugin_add_stock_icon (plugin, "go-last");
	cra_plugin_add_stock_icon (plugin, "go-next");
	cra_plugin_add_stock_icon (plugin, "go-previous");
	cra_plugin_add_stock_icon (plugin, "go-top");
	cra_plugin_add_stock_icon (plugin, "go-up");
	cra_plugin_add_stock_icon (plugin, "help-about");
	cra_plugin_add_stock_icon (plugin, "help-browser");
	cra_plugin_add_stock_icon (plugin, "help-browser");
	cra_plugin_add_stock_icon (plugin, "help-contents");
	cra_plugin_add_stock_icon (plugin, "help-faq");
	cra_plugin_add_stock_icon (plugin, "image-loading");
	cra_plugin_add_stock_icon (plugin, "image-missing");
	cra_plugin_add_stock_icon (plugin, "image-x-generic");
	cra_plugin_add_stock_icon (plugin, "input-gaming");
	cra_plugin_add_stock_icon (plugin, "input-keyboard");
	cra_plugin_add_stock_icon (plugin, "input-mouse");
	cra_plugin_add_stock_icon (plugin, "input-tablet");
	cra_plugin_add_stock_icon (plugin, "insert-image");
	cra_plugin_add_stock_icon (plugin, "insert-link");
	cra_plugin_add_stock_icon (plugin, "insert-object");
	cra_plugin_add_stock_icon (plugin, "insert-text");
	cra_plugin_add_stock_icon (plugin, "list-add");
	cra_plugin_add_stock_icon (plugin, "list-remove");
	cra_plugin_add_stock_icon (plugin, "mail-attachment");
	cra_plugin_add_stock_icon (plugin, "mail-forward");
	cra_plugin_add_stock_icon (plugin, "mail-mark-important");
	cra_plugin_add_stock_icon (plugin, "mail-mark-junk");
	cra_plugin_add_stock_icon (plugin, "mail-mark-notjunk");
	cra_plugin_add_stock_icon (plugin, "mail-mark-read");
	cra_plugin_add_stock_icon (plugin, "mail-mark-unread");
	cra_plugin_add_stock_icon (plugin, "mail-message-new");
	cra_plugin_add_stock_icon (plugin, "mail-read");
	cra_plugin_add_stock_icon (plugin, "mail-replied");
	cra_plugin_add_stock_icon (plugin, "mail-reply-all");
	cra_plugin_add_stock_icon (plugin, "mail-reply-sender");
	cra_plugin_add_stock_icon (plugin, "mail-send");
	cra_plugin_add_stock_icon (plugin, "mail-send-receive");
	cra_plugin_add_stock_icon (plugin, "mail-signed");
	cra_plugin_add_stock_icon (plugin, "mail-signed-verified");
	cra_plugin_add_stock_icon (plugin, "mail-unread");
	cra_plugin_add_stock_icon (plugin, "media-cdrom");
	cra_plugin_add_stock_icon (plugin, "media-eject");
	cra_plugin_add_stock_icon (plugin, "media-flash");
	cra_plugin_add_stock_icon (plugin, "media-floppy");
	cra_plugin_add_stock_icon (plugin, "media-optical");
	cra_plugin_add_stock_icon (plugin, "media-playback-pause");
	cra_plugin_add_stock_icon (plugin, "media-playback-start");
	cra_plugin_add_stock_icon (plugin, "media-playback-stop");
	cra_plugin_add_stock_icon (plugin, "media-playlist-repeat");
	cra_plugin_add_stock_icon (plugin, "media-playlist-shuffle");
	cra_plugin_add_stock_icon (plugin, "media-record");
	cra_plugin_add_stock_icon (plugin, "media-seek-backward");
	cra_plugin_add_stock_icon (plugin, "media-seek-forward");
	cra_plugin_add_stock_icon (plugin, "media-skip-backward");
	cra_plugin_add_stock_icon (plugin, "media-skip-forward");
	cra_plugin_add_stock_icon (plugin, "media-tape");
	cra_plugin_add_stock_icon (plugin, "modem");
	cra_plugin_add_stock_icon (plugin, "multimedia-player");
	cra_plugin_add_stock_icon (plugin, "multimedia-volume-control");
	cra_plugin_add_stock_icon (plugin, "network-error");
	cra_plugin_add_stock_icon (plugin, "network-idle");
	cra_plugin_add_stock_icon (plugin, "network-offline");
	cra_plugin_add_stock_icon (plugin, "network-receive");
	cra_plugin_add_stock_icon (plugin, "network-server");
	cra_plugin_add_stock_icon (plugin, "network-transmit");
	cra_plugin_add_stock_icon (plugin, "network-transmit-receive");
	cra_plugin_add_stock_icon (plugin, "network-wired");
	cra_plugin_add_stock_icon (plugin, "network-wireless");
	cra_plugin_add_stock_icon (plugin, "network-workgroup");
	cra_plugin_add_stock_icon (plugin, "object-flip-horizontal");
	cra_plugin_add_stock_icon (plugin, "object-flip-vertical");
	cra_plugin_add_stock_icon (plugin, "object-rotate-left");
	cra_plugin_add_stock_icon (plugin, "object-rotate-right");
	cra_plugin_add_stock_icon (plugin, "package-x-generic");
	cra_plugin_add_stock_icon (plugin, "pda");
	cra_plugin_add_stock_icon (plugin, "phone");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-accessibility");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-display");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-font");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-keyboard");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-keyboard-shortcuts");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-locale");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-multimedia");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-peripherals");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-personal");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-remote-desktop");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-screensaver");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-theme");
	cra_plugin_add_stock_icon (plugin, "preferences-desktop-wallpaper");
	cra_plugin_add_stock_icon (plugin, "preferences-other");
	cra_plugin_add_stock_icon (plugin, "preferences-system");
	cra_plugin_add_stock_icon (plugin, "preferences-system-network");
	cra_plugin_add_stock_icon (plugin, "preferences-system-notifications");
	cra_plugin_add_stock_icon (plugin, "preferences-system-privacy");
	cra_plugin_add_stock_icon (plugin, "preferences-system-search");
	cra_plugin_add_stock_icon (plugin, "preferences-system-sharing");
	cra_plugin_add_stock_icon (plugin, "preferences-system-windows");
	cra_plugin_add_stock_icon (plugin, "printer");
	cra_plugin_add_stock_icon (plugin, "printer");
	cra_plugin_add_stock_icon (plugin, "printer-error");
	cra_plugin_add_stock_icon (plugin, "printer-printing");
	cra_plugin_add_stock_icon (plugin, "process-stop");
	cra_plugin_add_stock_icon (plugin, "scanner");
	cra_plugin_add_stock_icon (plugin, "security-high");
	cra_plugin_add_stock_icon (plugin, "security-low");
	cra_plugin_add_stock_icon (plugin, "security-medium");
	cra_plugin_add_stock_icon (plugin, "software-update-available");
	cra_plugin_add_stock_icon (plugin, "software-update-urgent");
	cra_plugin_add_stock_icon (plugin, "start-here");
	cra_plugin_add_stock_icon (plugin, "sync-error");
	cra_plugin_add_stock_icon (plugin, "sync-synchronizing");
	cra_plugin_add_stock_icon (plugin, "system-file-manager");
	cra_plugin_add_stock_icon (plugin, "system-file-manager");
	cra_plugin_add_stock_icon (plugin, "system-help");
	cra_plugin_add_stock_icon (plugin, "system-lock-screen");
	cra_plugin_add_stock_icon (plugin, "system-log-out");
	cra_plugin_add_stock_icon (plugin, "system-reboot");
	cra_plugin_add_stock_icon (plugin, "system-run");
	cra_plugin_add_stock_icon (plugin, "system-search");
	cra_plugin_add_stock_icon (plugin, "system-shutdown");
	cra_plugin_add_stock_icon (plugin, "system-software-install");
	cra_plugin_add_stock_icon (plugin, "system-software-update");
	cra_plugin_add_stock_icon (plugin, "system-users");
	cra_plugin_add_stock_icon (plugin, "task-due");
	cra_plugin_add_stock_icon (plugin, "task-past-due");
	cra_plugin_add_stock_icon (plugin, "text-html");
	cra_plugin_add_stock_icon (plugin, "text-x-generic");
	cra_plugin_add_stock_icon (plugin, "text-x-generic-template");
	cra_plugin_add_stock_icon (plugin, "text-x-script");
	cra_plugin_add_stock_icon (plugin, "tools-check-spelling");
	cra_plugin_add_stock_icon (plugin, "user-available");
	cra_plugin_add_stock_icon (plugin, "user-away");
	cra_plugin_add_stock_icon (plugin, "user-bookmarks");
	cra_plugin_add_stock_icon (plugin, "user-desktop");
	cra_plugin_add_stock_icon (plugin, "user-home");
	cra_plugin_add_stock_icon (plugin, "user-idle");
	cra_plugin_add_stock_icon (plugin, "user-offline");
	cra_plugin_add_stock_icon (plugin, "user-trash");
	cra_plugin_add_stock_icon (plugin, "user-trash-full");
	cra_plugin_add_stock_icon (plugin, "utilities-system-monitor");
	cra_plugin_add_stock_icon (plugin, "utilities-terminal");
	cra_plugin_add_stock_icon (plugin, "video-display");
	cra_plugin_add_stock_icon (plugin, "video-display");
	cra_plugin_add_stock_icon (plugin, "video-x-generic");
	cra_plugin_add_stock_icon (plugin, "view-fullscreen");
	cra_plugin_add_stock_icon (plugin, "view-refresh");
	cra_plugin_add_stock_icon (plugin, "view-restore");
	cra_plugin_add_stock_icon (plugin, "view-sort-ascending");
	cra_plugin_add_stock_icon (plugin, "view-sort-descending");
	cra_plugin_add_stock_icon (plugin, "weather-clear");
	cra_plugin_add_stock_icon (plugin, "weather-clear-night");
	cra_plugin_add_stock_icon (plugin, "weather-few-clouds");
	cra_plugin_add_stock_icon (plugin, "weather-few-clouds-night");
	cra_plugin_add_stock_icon (plugin, "weather-fog");
	cra_plugin_add_stock_icon (plugin, "weather-overcast");
	cra_plugin_add_stock_icon (plugin, "weather-severe-alert");
	cra_plugin_add_stock_icon (plugin, "weather-showers");
	cra_plugin_add_stock_icon (plugin, "weather-showers-scattered");
	cra_plugin_add_stock_icon (plugin, "weather-snow");
	cra_plugin_add_stock_icon (plugin, "weather-storm");
	cra_plugin_add_stock_icon (plugin, "web-browser");
	cra_plugin_add_stock_icon (plugin, "window-close");
	cra_plugin_add_stock_icon (plugin, "window-new");
	cra_plugin_add_stock_icon (plugin, "x-office-address-book");
	cra_plugin_add_stock_icon (plugin, "x-office-calendar");
	cra_plugin_add_stock_icon (plugin, "x-office-document");
	cra_plugin_add_stock_icon (plugin, "x-office-presentation");
	cra_plugin_add_stock_icon (plugin, "x-office-spreadsheet");
	cra_plugin_add_stock_icon (plugin, "zoom-fit-best");
	cra_plugin_add_stock_icon (plugin, "zoom-in");
	cra_plugin_add_stock_icon (plugin, "zoom-original");
	cra_plugin_add_stock_icon (plugin, "zoom-out");
}

/**
 * cra_plugin_destroy:
 */
void
cra_plugin_destroy (CraPlugin *plugin)
{
	g_ptr_array_unref (plugin->priv->blacklist_cats);
	g_hash_table_unref (plugin->priv->stock_icon_names);
}

/**
 * _cra_plugin_check_filename:
 */
static gboolean
_cra_plugin_check_filename (const gchar *filename)
{
	if (fnmatch ("/usr/share/applications/*.desktop", filename, 0) == 0)
		return TRUE;
	if (fnmatch ("/usr/share/applications/kde4/*.desktop", filename, 0) == 0)
		return TRUE;
	return FALSE;
}

/**
 * cra_plugin_check_filename:
 */
gboolean
cra_plugin_check_filename (CraPlugin *plugin, const gchar *filename)
{
	return _cra_plugin_check_filename (filename);
}

/**
 * cra_plugin_desktop_key_get_locale:
 */
static gchar *
cra_plugin_desktop_key_get_locale (const gchar *key)
{
	gchar *tmp1;
	gchar *tmp2;
	gchar *locale = NULL;

	tmp1 = g_strstr_len (key, -1, "[");
	if (tmp1 == NULL)
		goto out;
	tmp2 = g_strstr_len (tmp1, -1, "]");
	if (tmp1 == NULL)
		goto out;
	locale = g_strdup (tmp1 + 1);
	locale[tmp2 - tmp1 - 1] = '\0';
out:
	return locale;
}

/**
 * cra_app_load_icon:
 */
static GdkPixbuf *
cra_app_load_icon (const gchar *filename, GError **error)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *pixbuf_tmp;

	/* open file in native size */
	pixbuf_tmp = gdk_pixbuf_new_from_file (filename, error);
	if (pixbuf_tmp == NULL)
		goto out;

	/* check size */
	if (gdk_pixbuf_get_width (pixbuf_tmp) < 32 ||
	    gdk_pixbuf_get_height (pixbuf_tmp) < 32) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "icon %s was too small %ix%i",
			     filename,
			     gdk_pixbuf_get_width (pixbuf_tmp),
			     gdk_pixbuf_get_height (pixbuf_tmp));
		goto out;
	}

	/* re-open file at correct size */
	pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, 64, 64,
						    FALSE, error);
	if (pixbuf == NULL)
		goto out;
out:
	if (pixbuf_tmp != NULL)
		g_object_unref (pixbuf_tmp);
	return pixbuf;
}

/**
 * cra_app_find_icon:
 */
static GdkPixbuf *
cra_app_find_icon (const gchar *tmpdir, const gchar *something, GError **error)
{
	gboolean ret = FALSE;
	gchar *tmp;
	GdkPixbuf *pixbuf = NULL;
	guint i;
	guint j;
	const gchar *pixmap_dirs[] = { "pixmaps", "icons", NULL };
	const gchar *supported_ext[] = { ".png",
					 ".gif",
					 ".svg",
					 ".xpm",
					 "",
					 NULL };
	const gchar *sizes[] = { "64x64",
				 "128x128",
				 "96x96",
				 "256x256",
				 "scalable",
				 "48x48",
				 "32x32",
				 "24x24",
				 "16x16",
				 NULL };

	/* is this an absolute path */
	if (something[0] == '/') {
		tmp = g_build_filename (tmpdir, something, NULL);
		pixbuf = cra_app_load_icon (tmp, error);
		g_free (tmp);
		goto out;
	}

	/* hicolor apps */
	for (i = 0; sizes[i] != NULL; i++) {
		for (j = 0; supported_ext[j] != NULL; j++) {
			tmp = g_strdup_printf ("%s/usr/share/icons/hicolor/%s/apps/%s%s",
					       tmpdir,
					       sizes[i],
					       something,
					       supported_ext[j]);
			ret = g_file_test (tmp, G_FILE_TEST_EXISTS);
			if (ret) {
				pixbuf = cra_app_load_icon (tmp, error);
				g_free (tmp);
				goto out;
			}
			g_free (tmp);
		}
	}

	/* pixmap */
	for (i = 0; pixmap_dirs[i] != NULL; i++) {
		for (j = 0; supported_ext[j] != NULL; j++) {
			tmp = g_strdup_printf ("%s/usr/share/%s/%s%s",
					       tmpdir,
					       pixmap_dirs[i],
					       something,
					       supported_ext[j]);
			ret = g_file_test (tmp, G_FILE_TEST_EXISTS);
			if (ret) {
				pixbuf = cra_app_load_icon (tmp, error);
				g_free (tmp);
				goto out;
			}
			g_free (tmp);
		}
	}

	/* failed */
	g_set_error (error,
		     CRA_PLUGIN_ERROR,
		     CRA_PLUGIN_ERROR_FAILED,
		     "Failed to find icon %s", something);
out:
	return pixbuf;
}

/**
 * cra_plugin_process_filename:
 */
static gboolean
cra_plugin_process_filename (CraPlugin *plugin,
			     CraPackage *pkg,
			     const gchar *filename,
			     GList **apps,
			     const gchar *tmpdir,
			     GError **error)
{
	const gchar *key;
	CraApp *app = NULL;
	gboolean ret;
	gchar *app_id = NULL;
	gchar *full_filename = NULL;
	gchar *icon_filename = NULL;
	gchar **keys = NULL;
	gchar *locale = NULL;
	gchar *tmp;
	gchar **tmpv;
	GdkPixbuf *pixbuf = NULL;
	GKeyFile *kf;
	GPtrArray *categories;
	guint i;
	guint j;

	/* load file */
	full_filename = g_build_filename (tmpdir, filename, NULL);
	kf = g_key_file_new ();
	ret = g_key_file_load_from_file (kf, full_filename,
					 G_KEY_FILE_KEEP_TRANSLATIONS,
					 error);
	if (!ret)
		goto out;

	/* create app */
	app_id = g_path_get_basename (filename);
	app = cra_app_new (pkg, app_id);
	as_app_set_id_kind (AS_APP (app), AS_ID_KIND_DESKTOP);

	/* look at all the keys */
	keys = g_key_file_get_keys (kf, G_KEY_FILE_DESKTOP_GROUP, NULL, error);
	if (keys == NULL) {
		ret = FALSE;
		goto out;
	}
	for (i = 0; keys[i] != NULL; i++) {
		key = keys[i];

		/* NoDisplay */
		if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY) == 0) {
			cra_app_set_requires_appdata (app, TRUE);
			as_app_add_metadata (AS_APP (app), "NoDisplay", "", -1);

		/* Type */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_TYPE) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_strcmp0 (tmp, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) != 0) {
				g_set_error (error,
					     CRA_PLUGIN_ERROR,
					     CRA_PLUGIN_ERROR_FAILED,
					     "not an applicaiton %s",
					     filename);
				ret = FALSE;
				goto out;
			}
			g_free (tmp);

		/* Icon */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_ICON) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (tmp != NULL && tmp[0] != '\0')
				as_app_set_icon (AS_APP (app), tmp, -1);
			g_free (tmp);

		/* Categories */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_CATEGORIES) == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++) {
				if (g_strcmp0 (tmpv[j], "GTK") == 0)
					continue;
				if (g_strcmp0 (tmpv[j], "Qt") == 0)
					continue;
				if (g_strcmp0 (tmpv[j], "KDE") == 0)
					continue;
				if (g_strcmp0 (tmpv[j], "GNOME") == 0)
					continue;
				if (g_str_has_prefix (tmpv[j], "X-"))
					continue;
				as_app_add_category (AS_APP (app), tmpv[j], -1);
			}
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "Keywords") == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++)
				as_app_add_keyword (AS_APP (app), tmpv[j], -1);
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "MimeType") == 0) {
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			for (j = 0; tmpv[j] != NULL; j++)
				as_app_add_mimetype (AS_APP (app), tmpv[j], -1);
			g_strfreev (tmpv);

		} else if (g_strcmp0 (key, "X-GNOME-UsesNotifications") == 0) {
			as_app_add_metadata (AS_APP (app), "X-Kudo-UsesNotifications", "", -1);

		} else if (g_strcmp0 (key, "X-GNOME-Bugzilla-Product") == 0) {
			as_app_set_project_group (AS_APP (app), "GNOME", -1);

		} else if (g_strcmp0 (key, "X-MATE-Bugzilla-Product") == 0) {
			as_app_set_project_group (AS_APP (app), "MATE", -1);

		} else if (g_strcmp0 (key, "X-KDE-StartupNotify") == 0) {
			as_app_set_project_group (AS_APP (app), "KDE", -1);

		} else if (g_strcmp0 (key, "X-DocPath") == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_str_has_prefix (tmp, "http://userbase.kde.org/"))
				as_app_set_project_group (AS_APP (app), "KDE", -1);
			g_free (tmp);

		/* Exec */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_EXEC) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (g_str_has_prefix (tmp, "xfce4-"))
				as_app_set_project_group (AS_APP (app), "XFCE", -1);
			g_free (tmp);

		/* OnlyShowIn */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN) == 0) {
			/* if an app has only one entry, it's that desktop */
			tmpv = g_key_file_get_string_list (kf,
							   G_KEY_FILE_DESKTOP_GROUP,
							   key,
							   NULL, NULL);
			if (g_strv_length (tmpv) == 1)
				as_app_set_project_group (AS_APP (app), tmpv[0], -1);
			g_strfreev (tmpv);

		/* Name */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_NAME) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (tmp != NULL && tmp[0] != '\0')
				as_app_set_name (AS_APP (app), "C", tmp, -1);
			g_free (tmp);

		/* Name[] */
		} else if (g_str_has_prefix (key, G_KEY_FILE_DESKTOP_KEY_NAME)) {
			locale = cra_plugin_desktop_key_get_locale (key);
			tmp = g_key_file_get_locale_string (kf,
							    G_KEY_FILE_DESKTOP_GROUP,
							    G_KEY_FILE_DESKTOP_KEY_NAME,
							    locale,
							    NULL);
			if (tmp != NULL && tmp[0] != '\0')
				as_app_set_name (AS_APP (app), locale, tmp, -1);
			g_free (locale);
			g_free (tmp);

		/* Comment */
		} else if (g_strcmp0 (key, G_KEY_FILE_DESKTOP_KEY_COMMENT) == 0) {
			tmp = g_key_file_get_string (kf,
						     G_KEY_FILE_DESKTOP_GROUP,
						     key,
						     NULL);
			if (tmp != NULL && tmp[0] != '\0')
				as_app_set_comment (AS_APP (app), "C", tmp, -1);
			g_free (tmp);

		/* Comment[] */
		} else if (g_str_has_prefix (key, G_KEY_FILE_DESKTOP_KEY_COMMENT)) {
			locale = cra_plugin_desktop_key_get_locale (key);
			tmp = g_key_file_get_locale_string (kf,
							    G_KEY_FILE_DESKTOP_GROUP,
							    G_KEY_FILE_DESKTOP_KEY_COMMENT,
							    locale,
							    NULL);
			if (tmp != NULL && tmp[0] != '\0')
				as_app_set_comment (AS_APP (app), locale, tmp, -1);
			g_free (locale);
			g_free (tmp);

		}
	}

	/* check for blacklisted categories */
	categories = as_app_get_categories (AS_APP (app));
	for (i = 0; i < categories->len; i++) {
		key = g_ptr_array_index (categories, i);
		if (cra_glob_value_search (plugin->priv->blacklist_cats,
					   key) != NULL) {
			g_object_unref (app);
			app = NULL;
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_INFO,
					 "category %s is blacklisted", key);
			goto out;
		}
	}

	/* is the icon a stock-icon-name? */
	key = as_app_get_icon (AS_APP (app));
	if (key != NULL) {
		if (g_hash_table_lookup (plugin->priv->stock_icon_names,
					 key) != NULL) {
			as_app_set_icon_kind (AS_APP (app), AS_ICON_KIND_STOCK);
			cra_package_log (pkg,
					 CRA_PACKAGE_LOG_LEVEL_DEBUG,
					 "using stock icon %s", key);
		} else {

			/* is icon XPM or GIF */
			if (g_str_has_suffix (key, ".xpm"))
				cra_app_add_veto (app, "Uses XPM icon: %s", key);
			else if (g_str_has_suffix (key, ".gif"))
				cra_app_add_veto (app, "Uses GIF icon: %s", key);

			/* find icon */
			pixbuf = cra_app_find_icon (tmpdir, key, error);
			if (pixbuf == NULL) {
				ret = FALSE;
				goto out;
			}

			/* save in target directory */
			icon_filename = g_strdup_printf ("%s.png",
							 as_app_get_id (AS_APP (app)));
			as_app_set_icon (AS_APP (app), icon_filename, -1);
			as_app_set_icon_kind (AS_APP (app), AS_ICON_KIND_CACHED);
			cra_app_set_pixbuf (app, pixbuf);
		}
	}

	/* add */
	cra_plugin_add_app (apps, app);
out:
	if (app != NULL)
		g_object_unref (app);
	if (pixbuf != NULL)
		g_object_unref (pixbuf);
	g_strfreev (keys);
	g_key_file_unref (kf);
	g_free (full_filename);
	g_free (icon_filename);
	g_free (app_id);
	return ret;
}

/**
 * cra_plugin_process:
 */
GList *
cra_plugin_process (CraPlugin *plugin,
		    CraPackage *pkg,
		    const gchar *tmpdir,
		    GError **error)
{
	gboolean ret;
	GList *apps = NULL;
	guint i;
	gchar **filelist;

	filelist = cra_package_get_filelist (pkg);
	for (i = 0; filelist[i] != NULL; i++) {
		if (!_cra_plugin_check_filename (filelist[i]))
			continue;
		ret = cra_plugin_process_filename (plugin,
						   pkg,
						   filelist[i],
						   &apps,
						   tmpdir,
						   error);
		if (!ret) {
			g_list_free_full (apps, (GDestroyNotify) g_object_unref);
			apps = NULL;
			goto out;
		}
	}

	/* no desktop files we care about */
	if (apps == NULL) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "nothing interesting in %s",
			     cra_package_get_filename (pkg));
		goto out;
	}
out:
	return apps;
}
