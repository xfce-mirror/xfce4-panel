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

#include "common/panel-debug.h"
#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4windowing/libxfce4windowing.h>
#include <libxfce4windowingui/libxfce4windowingui.h>

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <libxfce4windowing/xfw-x11.h>
#endif

G_BEGIN_DECLS

#define XFCE_TYPE_TASKLIST (xfce_tasklist_get_type ())
G_DECLARE_FINAL_TYPE (XfceTasklist, xfce_tasklist, XFCE, TASKLIST, GtkContainer)

typedef enum _XfceTasklistSortOrder
{
  XFCE_TASKLIST_SORT_ORDER_TIMESTAMP, /* sort by unique_id */
  XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP, /* sort by group and then by timestamp */
  XFCE_TASKLIST_SORT_ORDER_TITLE, /* sort by window title */
  XFCE_TASKLIST_SORT_ORDER_GROUP_TITLE, /* sort by group and then by title */
  XFCE_TASKLIST_SORT_ORDER_DND, /* append and support dnd */

  XFCE_TASKLIST_SORT_ORDER_MIN = XFCE_TASKLIST_SORT_ORDER_TIMESTAMP,
  XFCE_TASKLIST_SORT_ORDER_MAX = XFCE_TASKLIST_SORT_ORDER_DND,
  XFCE_TASKLIST_SORT_ORDER_DEFAULT = XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP
} XfceTasklistSortOrder;

typedef enum _XfceTasklistMClick
{
  XFCE_TASKLIST_MIDDLE_CLICK_NOTHING, /* do nothing */
  XFCE_TASKLIST_MIDDLE_CLICK_CLOSE_WINDOW, /* close the window */
  XFCE_TASKLIST_MIDDLE_CLICK_MINIMIZE_WINDOW, /* minimize, never minimize with button 1 */
  XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE, /* launches a new instance of the window */

  XFCE_TASKLIST_MIDDLE_CLICK_MIN = XFCE_TASKLIST_MIDDLE_CLICK_NOTHING,
  XFCE_TASKLIST_MIDDLE_CLICK_MAX = XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE,
  XFCE_TASKLIST_MIDDLE_CLICK_DEFAULT = XFCE_TASKLIST_MIDDLE_CLICK_NOTHING
} XfceTasklistMClick;


struct _XfceTasklist
{
  GtkContainer __parent__;

  /* lock counter */
  gint locked;

  /* the screen of this tasklist */
  XfwScreen *screen;
  XfwWorkspaceGroup *workspace_group;
  GdkDisplay *display;

  /* window children in the tasklist */
  GList *windows;

  /* windows we monitor, but that are excluded from the tasklist */
  GSList *skipped_windows;

  /* arrow button of the overflow menu */
  GtkWidget *arrow_button;

  /* applications of all the windows in the taskbar */
  GHashTable *apps;

  /* normal or iconbox style */
  guint show_labels : 1;

  /* size of the panel pluin */
  gint size;

  /* mode (orientation) of the tasklist */
  XfcePanelPluginMode mode;

  /* relief of the tasklist buttons */
  GtkReliefStyle button_relief;

  /* whether we show windows from all workspaces or
   * only the active workspace */
  guint all_workspaces : 1;

  /* whether we switch to another workspace when we try to
   * unminimize a window on another workspace */
  guint switch_workspace : 1;

  /* whether we only show monimized windows in the
   * tasklist */
  guint only_minimized : 1;

  /* number of rows of window buttons */
  gint nrows;

  /* switch window with the mouse wheel */
  guint window_scrolling : 1;
  guint wrap_windows : 1;

  /* whether we show blinking windows from all workspaces
   * or only the active workspace */
  guint all_blinking : 1;

  /* action to preform when middle clicking */
  XfceTasklistMClick middle_click;

  /* whether decorate labels when window is not visible */
  guint label_decorations : 1;

  /* whether we only show windows that are in the geometry of
   * the monitor the tasklist is on */
  guint all_monitors : 1;
  guint n_monitors;

  /* we only show windows from this monitor */
  guint monitor_index;

  /* whether we show wireframes when hovering a button in
   * the tasklist */
  guint show_wireframes : 1;

  /* icon geometries update timeout */
  guint update_icon_geometries_id;

  /* idle monitor geometry update */
  guint update_monitor_geometry_id;

  /* button grouping */
  guint grouping : 1;

  /* sorting order of the buttons */
  XfceTasklistSortOrder sort_order;

  /* dummy properties */
  guint show_handle : 1;
  guint show_tooltips : 1;

#ifdef ENABLE_X11
  /* wireframe window */
  Window wireframe_window;
#endif

  /* gtk style properties */
  gint max_button_length;
  gint min_button_length;
  gint max_button_size;
  PangoEllipsizeMode ellipsize_mode;
  gint minimized_icon_lucency;
  gint menu_max_width_chars;

  gint n_windows;
};


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
