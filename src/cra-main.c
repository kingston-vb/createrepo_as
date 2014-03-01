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

/**
 * cra_task_process_func:
 */
static void
cra_task_process_func (gpointer data, gpointer user_data)
{
	CraTask *task = (CraTask *) data;
	CraContext *ctx = (CraContext *) user_data;
	const gchar *filename;

	filename = "/usr/lib64/gstreamer-1.0/libgstdave.so";
	g_warning ("%p", cra_plugin_loader_match_fn (ctx->plugins, filename));
	filename = "/usr/lib64/gsssssssssssssssssstreamer-1.0/libgstdave.so";
	g_warning ("%p", cra_plugin_loader_match_fn (ctx->plugins, filename));

	g_debug ("Running %s on %p", task->filename, g_thread_self ());
	g_usleep (G_USEC_PER_SEC);
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
out:
	if (dir != NULL)
		g_dir_close (dir);
	return 0;
}
