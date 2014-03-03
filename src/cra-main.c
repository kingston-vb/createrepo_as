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

#include "cra-package.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-utils.h"

#include <glib.h>
#include <rpm/rpmlib.h>

typedef struct {
	gchar		*filename;
	gchar		*tmpdir;
	CraPackage	*pkg;
} CraTask;

typedef struct {
	GPtrArray	*plugins;
	GPtrArray	*packages;
} CraContext;

/**
 * cra_task_free:
 */
static void
cra_task_free (CraTask *task)
{
	g_free (task->filename);
	g_free (task->tmpdir);
	g_free (task);
}

/**
 * cra_task_process_func:
 */
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	CraContext *ctx = (CraContext *) user_data;
	CraPlugin *plugin = NULL;
	CraTask *task = (CraTask *) data;
	GError *error = NULL;
	gboolean ret;
	guint i;

	/* did we get a file match on any plugin */
	g_debug ("Getting filename match for %s", task->filename);
	for (i = 0; task->pkg->filelist[i] != NULL; i++) {
		plugin = cra_plugin_loader_match_fn (ctx->plugins,
						     task->pkg->filelist[i]);
		if (plugin != NULL)
			break;
	}
	if (plugin == NULL)
		goto out;

	/* delete old tree if it exists */
	ret = cra_utils_ensure_exists_and_empty (task->tmpdir, &error);
	if (!ret) {
		g_warning ("Failed to clear: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* explode tree */
	ret = cra_package_explode (task->pkg, task->tmpdir, &error);
	if (!ret) {
		g_warning ("Failed to explode: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* TODO: add interesting common packages */

	/* run plugin */
	g_debug ("Processing %s with %s [%p]",
		 task->filename, plugin->name, g_thread_self ());
	ret = cra_plugin_process (plugin, task->tmpdir, &error);
	if (!ret) {
		g_warning ("Failed to run process: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* TODO: is application backlisted */

	/* delete tree */
	ret = cra_utils_rmtree (task->tmpdir, &error);
	if (!ret) {
		g_warning ("Failed to delete tree: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
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
	const gchar *filename;
	const gchar *packages_dir = "./packages";
	const gint max_threads = 1;
	CraContext *ctx;
	guint i;
	CraTask *task;
	gboolean ret;
	GDir *dir = NULL;
	GError *error = NULL;
	GThreadPool *pool;
	gchar *tmp;
	CraPackage *pkg;

	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

	rpmReadConfigFiles (NULL, NULL);

	/* set up state */
	ret = cra_utils_ensure_exists_and_empty ("./tmp", &error);
	if (!ret) {
		g_warning ("failed to create tmpdir: %s", error->message);
		g_error_free (error);
		goto out;
	}

	ctx = g_new (CraContext, 1);
	ctx->plugins = cra_plugin_loader_new ();
	ctx->packages = g_ptr_array_new_with_free_func ((GDestroyNotify) cra_package_free);
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
	g_debug ("Scanning packages");
	dir = g_dir_open (packages_dir, 0, &error);
	if (pool == NULL) {
		g_warning ("failed to open packages: %s", error->message);
		g_error_free (error);
		goto out;
	}
	while ((filename = g_dir_read_name (dir)) != NULL) {
		tmp = g_build_filename (packages_dir, filename, NULL);
		pkg = cra_package_open (tmp, &error);
		if (pkg == NULL) {
			g_warning ("Failed to open package: %s", error->message);
			g_error_free (error);
			goto out;
		}

		/* TODO: is package name blacklisted */

		/* get file list */
		ret = cra_package_ensure_filelist (pkg, &error);
		if (!ret) {
			g_warning ("Failed to get file list: %s", error->message);
			g_error_free (error);
			goto out;
		}

		g_ptr_array_add (ctx->packages, pkg);
		g_free (tmp);
	}

	/* add each package */
	g_debug ("Processing packages");
	for (i = 0; i < ctx->packages->len; i++) {
		pkg = g_ptr_array_index (ctx->packages, i);

		/* create task */
		task = g_new0 (CraTask, 1);
		task->filename = g_strdup (pkg->filename);
		task->tmpdir = g_build_filename ("./tmp", pkg->name, NULL);
		task->pkg = pkg;

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
	g_ptr_array_unref (ctx->packages);
	g_free (ctx);

	/* success */
	g_debug ("Done!");
out:
	if (dir != NULL)
		g_dir_close (dir);
	return 0;
}
