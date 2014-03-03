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
#include "cra-package.h"
#include "cra-plugin.h"
#include "cra-plugin-loader.h"
#include "cra-utils.h"

#include <glib.h>
#include <locale.h>
#include <rpm/rpmlib.h>

typedef struct {
	gchar		*filename;
	gchar		*tmpdir;
	CraPackage	*pkg;
} CraTask;

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
	const gchar *pkg_name;
	CraApp *app;
	CraContext *ctx = (CraContext *) user_data;
	CraPackage *pkg_extra;
	CraPlugin *plugin = NULL;
	CraTask *task = (CraTask *) data;
	gboolean ret;
	GError *error = NULL;
	GList *apps = NULL;
	GList *l;
	guint i;

	/* get file list */
	ret = cra_package_ensure_filelist (task->pkg, &error);
	if (!ret) {
		g_warning ("Failed to get file list: %s", error->message);
		g_error_free (error);
		goto out;
	}

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

	/* add extra packages */
	pkg_name = cra_glob_value_search (ctx->extra_pkgs, task->pkg->name);
	if (pkg_name != NULL) {
		pkg_extra = cra_context_find_by_pkgname (ctx, pkg_name);
		if (pkg_extra == NULL) {
			g_warning ("%s requires %s but is not available",
				   task->pkg->name, pkg_name);
			goto out;
		}
		ret = cra_package_explode (pkg_extra, task->tmpdir, &error);
		if (!ret) {
			g_warning ("Failed to explode extra file: %s",
				   error->message);
			g_error_free (error);
			goto out;
		}
	}

	/* run plugin */
	g_debug ("Processing %s with %s [%p]",
		 task->filename, plugin->name, g_thread_self ());
	apps = cra_plugin_process (plugin, task->pkg, task->tmpdir, &error);
	if (apps == NULL) {
		g_warning ("Failed to run process: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* print */
	for (l = apps; l != NULL; l = l->next) {
		app = l->data;

		/* never set */
		if (app->app_id == NULL) {
			g_debug ("app id not set for %s", task->pkg->name);
			continue;
		}

		/* is application backlisted */
		if (cra_glob_value_search (ctx->blacklisted_ids, app->app_id)) {
			g_debug ("app id %s is blacklisted", task->pkg->name);
			continue;
		}

		/* copy data from pkg into app */
		if (task->pkg->url != NULL)
			cra_app_set_homepage_url (app, task->pkg->url);

		/* run each refine plugin on each app */
		ret = cra_plugin_loader_process_app (ctx->plugins,
						     task->pkg,
						     app,
						     task->tmpdir,
						     &error);
		if (!ret) {
			g_warning ("Failed to run process: %s", error->message);
			g_error_free (error);
			goto out;
		}
		g_print ("\n");
		cra_app_print (app);
	}

	/* delete tree */
	ret = cra_utils_rmtree (task->tmpdir, &error);
	if (!ret) {
		g_warning ("Failed to delete tree: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	g_list_foreach (apps, (GFunc) cra_app_free, NULL);
	g_list_free (apps);
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
	const gchar *temp_dir = "./tmp";
	const gint max_threads = 1;
	CraContext *ctx = NULL;
	CraPackage *pkg;
	CraTask *task;
	gboolean ret;
	gchar *tmp;
	GDir *dir = NULL;
	GError *error = NULL;
	GThreadPool *pool;
	guint i;

	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

	rpmReadConfigFiles (NULL, NULL);
	setlocale (LC_ALL, "");

	/* set up state */
	ret = cra_utils_ensure_exists_and_empty (temp_dir, &error);
	if (!ret) {
		g_warning ("failed to create tmpdir: %s", error->message);
		g_error_free (error);
		goto out;
	}

	ctx = cra_context_new ();
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

		/* is package name blacklisted */
		if (cra_glob_value_search (ctx->blacklisted_pkgs, pkg->name) != NULL) {
			g_debug ("%s is blacklisted", pkg->filename);
			continue;
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
		task->tmpdir = g_build_filename (temp_dir, pkg->name, NULL);
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

	/* success */
	g_debug ("Done!");
out:
	if (ctx != NULL)
		cra_context_free (ctx);
	if (dir != NULL)
		g_dir_close (dir);
	return 0;
}
