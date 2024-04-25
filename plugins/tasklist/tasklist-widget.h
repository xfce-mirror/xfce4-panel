/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFCE_TASKLIST_H__
#define __XFCE_TASKLIST_H__

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_TASKLIST (xfce_tasklist_get_type ())
G_DECLARE_FINAL_TYPE (XfceTasklist, xfce_tasklist, XFCE, TASKLIST, GtkContainer)

void
xfce_tasklist_set_mode (XfceTasklist *tasklist,
                        XfcePanelPluginMode mode);

void
xfce_tasklist_set_size (XfceTasklist *tasklist,
                        gint size);

void
xfce_tasklist_set_nrows (XfceTasklist *tasklist,
                         gint nrows);

void
xfce_tasklist_update_monitor_geometry (XfceTasklist *tasklist);

G_END_DECLS

#endif /* !__XFCE_TASKLIST_H__ */
