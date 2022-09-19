/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFWL_FOREIGN_TOPLEVEL_MANAGER_H__
#define __XFWL_FOREIGN_TOPLEVEL_MANAGER_H__

#include <glib.h>
#include <glib-object.h>
#include "xfwl-foreign-toplevel.h"

G_BEGIN_DECLS

#define XFWL_TYPE_FOREIGN_TOPLEVEL_MANAGER (xfwl_foreign_toplevel_manager_get_type ())
G_DECLARE_FINAL_TYPE (XfwlForeignToplevelManager, xfwl_foreign_toplevel_manager, XFWL, FOREIGN_TOPLEVEL_MANAGER, GObject)

XfwlForeignToplevelManager              *xfwl_foreign_toplevel_manager_get              (void);

struct zwlr_foreign_toplevel_manager_v1 *xfwl_foreign_toplevel_manager_get_wl_manager   (XfwlForeignToplevelManager *manager);

GList                                   *xfwl_foreign_toplevel_manager_get_toplevels    (XfwlForeignToplevelManager *manager);

XfwlForeignToplevel                     *xfwl_foreign_toplevel_manager_get_active       (XfwlForeignToplevelManager *manager);

gboolean                                 xfwl_foreign_toplevel_manager_get_show_desktop (XfwlForeignToplevelManager *manager);

void                                     xfwl_foreign_toplevel_manager_set_show_desktop (XfwlForeignToplevelManager *manager,
                                                                                         gboolean                    show);

G_END_DECLS

#endif /* !__XFWL_FOREIGN_TOPLEVEL_MANAGER_H__ */
