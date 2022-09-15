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

#ifndef __XFWL_FOREIGN_TOPLEVEL_H__
#define __XFWL_FOREIGN_TOPLEVEL_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define XFWL_TYPE_FOREIGN_TOPLEVEL (xfwl_foreign_toplevel_get_type ())
G_DECLARE_FINAL_TYPE (XfwlForeignToplevel, xfwl_foreign_toplevel, XFWL, FOREIGN_TOPLEVEL, GObject)

/**
 * XfwlForeignToplevelState:
 * @XFWL_FOREIGN_TOPLEVEL_STATE_NONE: no particular state
 * @XFWL_FOREIGN_TOPLEVEL_STATE_MAXIMIZED: the toplevel is maximized
 * @XFWL_FOREIGN_TOPLEVEL_STATE_MINIMIZED: the toplevel is minimized
 * @XFWL_FOREIGN_TOPLEVEL_STATE_ACTIVATED: the toplevel is active
 * @XFWL_FOREIGN_TOPLEVEL_STATE_FULLSCREEN: the toplevel is fullscreen
 *
 * The different states that a toplevel can have, some of them mutually exclusive.
 **/
typedef enum /*< flags,prefix=XFWL >*/
{
  XFWL_FOREIGN_TOPLEVEL_STATE_NONE       = 0,
  XFWL_FOREIGN_TOPLEVEL_STATE_MAXIMIZED  = 1 << 0,
  XFWL_FOREIGN_TOPLEVEL_STATE_MINIMIZED  = 1 << 1,
  XFWL_FOREIGN_TOPLEVEL_STATE_ACTIVATED  = 1 << 2,
  XFWL_FOREIGN_TOPLEVEL_STATE_FULLSCREEN = 1 << 3
}
XfwlForeignToplevelState;

struct zwlr_foreign_toplevel_handle_v1 *xfwl_foreign_toplevel_get_wl_toplevel       (XfwlForeignToplevel *toplevel);

const gchar                            *xfwl_foreign_toplevel_get_title             (XfwlForeignToplevel *toplevel);

const gchar                            *xfwl_foreign_toplevel_get_app_id            (XfwlForeignToplevel *toplevel);

GList                                  *xfwl_foreign_toplevel_get_monitors          (XfwlForeignToplevel *toplevel);

XfwlForeignToplevelState                xfwl_foreign_toplevel_get_state             (XfwlForeignToplevel *toplevel);

XfwlForeignToplevel                    *xfwl_foreign_toplevel_get_parent            (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_maximize              (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_unmaximize            (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_minimize              (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_unminimize            (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_activate              (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_activate_on_seat      (XfwlForeignToplevel *toplevel,
                                                                                     GdkSeat             *seat);

void                                    xfwl_foreign_toplevel_close                 (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_set_rectangle         (XfwlForeignToplevel *toplevel,
                                                                                     GdkWindow           *window,
                                                                                     const GdkRectangle  *rectangle);

void                                    xfwl_foreign_toplevel_fullscreen            (XfwlForeignToplevel *toplevel);

void                                    xfwl_foreign_toplevel_fullscreen_on_monitor (XfwlForeignToplevel *toplevel,
                                                                                     GdkMonitor          *monitor);

void                                    xfwl_foreign_toplevel_unfullscreen          (XfwlForeignToplevel *toplevel);

G_END_DECLS

#endif /* !__XFWL_FOREIGN_TOPLEVEL_H__ */
