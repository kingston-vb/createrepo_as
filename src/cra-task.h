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

#ifndef CRA_TASK_H
#define CRA_TASK_H

#include <glib-object.h>

#include "cra-package.h"
#include "cra-context.h"

#define CRA_TYPE_TASK		(cra_task_get_type())
#define CRA_TASK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), CRA_TYPE_TASK, CraTask))
#define CRA_TASK_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), CRA_TYPE_TASK, CraTaskClass))
#define CRA_IS_TASK(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), CRA_TYPE_TASK))
#define CRA_IS_TASK_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), CRA_TYPE_TASK))
#define CRA_TASK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), CRA_TYPE_TASK, CraTaskClass))

G_BEGIN_DECLS

typedef struct _CraTask		CraTask;
typedef struct _CraTaskClass	CraTaskClass;

struct _CraTask
{
	GObject			 parent;
};

struct _CraTaskClass
{
	GObjectClass		 parent_class;
};

GType		 cra_task_get_type		(void);


CraTask		*cra_task_new			(CraContext	*ctx);
gboolean	 cra_task_process		(CraTask	*task,
						 GError		**error_not_used);
void		 cra_task_set_package		(CraTask	*task,
						 CraPackage	*pkg);
void		 cra_task_set_id		(CraTask	*task,
						 guint		 id);

G_END_DECLS

#endif /* CRA_TASK_H */
