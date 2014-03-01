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

#include "cra-plugin.h"
#include "cra-plugin-loader.h"

#include <glib.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>


typedef struct {
	gchar		*filename;
} CraTask;

typedef struct {
	GPtrArray	*plugins;
} CraContext;

/**
 * cra_task_free:
 */
static void
cra_task_free (CraTask *task)
{
	g_free (task->filename);
	g_free (task);
}

typedef struct {
	gchar		**filelist;
} CraPackage;

static void
cra_package_free (CraPackage *pkg)
{
	g_strfreev (pkg->filelist);
	g_free (pkg);
}

static CraPackage *
cra_package_open (const gchar *filename, GError **error)
{
	const gchar **dirnames = NULL;
	CraPackage *pkg = NULL;
	FD_t fd;
	gchar **filelist = NULL;
	gint32 *dirindex = NULL;
	gint rc;
	guint i;
	Header h;
	rpmtd td[3];
	rpmts ts;

	/* open the file */
	ts = rpmtsCreate ();
	fd = Fopen (filename, "r");
	rc = rpmReadPackageFile (ts, fd, filename, &h);
	if (rc != RPMRC_OK) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package %s", filename);
		goto out;
	}

	/* read out the file list */
	for (i = 0; i < 3; i++)
		td[i] = rpmtdNew ();
	rc = headerGet (h, RPMTAG_DIRNAMES, td[0], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (h, RPMTAG_BASENAMES, td[1], HEADERGET_MINMEM);
	if (rc)
		rc = headerGet (h, RPMTAG_DIRINDEXES, td[2], HEADERGET_MINMEM);
	if (!rc) {
		g_set_error (error,
			     CRA_PLUGIN_ERROR,
			     CRA_PLUGIN_ERROR_FAILED,
			     "Failed to read package file list %s", filename);
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

	/* success */
	pkg = g_new0 (CraPackage, 1);
	pkg->filelist = filelist;
	filelist = NULL;
out:
	Fclose (fd);
	g_free (dirindex);
	g_free (dirnames);
	g_strfreev (filelist);
	return pkg;
}

/**
 * cra_task_process_func:
 */
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	CraContext *ctx = (CraContext *) user_data;
	CraPackage *pkg = NULL;
	CraPlugin *plugin = NULL;
	CraTask *task = (CraTask *) data;
	GError *error = NULL;
	gboolean ret;
	guint i;

	/* open package and get file list */
	pkg = cra_package_open (task->filename, &error);
	if (pkg == NULL) {
		g_warning ("Failed to open package: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* did we get a file match on any plugin */
	for (i = 0; pkg->filelist[i] != NULL; i++) {
		plugin = cra_plugin_loader_match_fn (ctx->plugins,
						     pkg->filelist[i]);
		if (plugin != NULL)
			break;
	}
	if (plugin == NULL)
		goto out;

	/* TODO: explode tree */

	/* TODO: add interesting common packages */

	g_debug ("Processing %s with %s [%p]",
		 task->filename, plugin->name, g_thread_self ());
	ret = cra_plugin_process (plugin, "./tmp", &error);
	if (!ret) {
		g_warning ("Failed to run process: %s", error->message);
		g_error_free (error);
		goto out;
	}
	g_usleep (G_USEC_PER_SEC);
out:
	if (pkg != NULL)
		cra_package_free (pkg);
	cra_task_free (task);
}

/**
 * cra_task_sort_cb:
 */
static gint
cra_task_sort_cb (gconstpointer a, gconstpointer b, gpointer user_data)
{
	CraTask *task_a = (CraTask *) a;
	CraTask *task_b = (CraTask *) b;
	return g_strcmp0 (task_a->filename, task_b->filename);
}

/**
 * main:
 */
int
main (int argc, char **argv)
{
	GThreadPool *pool;
	const gint max_threads = 4;
	GError *error = NULL;
	CraTask *task;
	gboolean ret;
	const gchar *filename;
	const gchar *packages_dir = "./packages";
	GDir *dir = NULL;
	CraContext *ctx;

	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

	rpmReadConfigFiles (NULL, NULL);

	ctx = g_new (CraContext, 1);
	ctx->plugins = cra_plugin_loader_new ();
	ret = cra_plugin_loader_setup (ctx->plugins, &error);
	if (!ret) {
		g_warning ("failed to set up plugins: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* create thread pool */
	pool = g_thread_pool_new (cra_task_process_func,
				  ctx,
				  max_threads,
				  TRUE,
				  &error);
	if (pool == NULL) {
		g_warning ("failed to set up pool: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* sort by name */
	g_thread_pool_set_sort_function (pool, cra_task_sort_cb, NULL);

	/* scan each package */
	dir = g_dir_open (packages_dir, 0, &error);
	if (pool == NULL) {
		g_warning ("failed to open packages: %s", error->message);
		g_error_free (error);
		goto out;
	}
	while ((filename = g_dir_read_name (dir)) != NULL) {

		/* create task */
		task = g_new0 (CraTask, 1);
		task->filename = g_build_filename (packages_dir, filename, NULL);

		/* add task to pool */
		ret = g_thread_pool_push (pool, task, &error);
		if (!ret) {
			g_warning ("failed to set up pool: %s", error->message);
			g_error_free (error);
			goto out;
		}
	}

	/* wait for them to finish */
	if (pool != NULL)
		g_thread_pool_free (pool, FALSE, TRUE);

	/* run the plugins */
	cra_plugin_loader_free (ctx->plugins);

	/* success */
	g_debug ("Done!");
out:
	if (dir != NULL)
		g_dir_close (dir);
	return 0;
}
