/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_WINDOW_H__
#define __PANEL_WINDOW_H__

#include "panel-base-window.h"

#include "common/panel-xfconf.h"

#include <gtk/gtk.h>
#include <xfconf/xfconf.h>

#define DEFAULT_MODE XFCE_PANEL_PLUGIN_MODE_HORIZONTAL
#define MIN_SIZE 16
#define MAX_SIZE 128
#define DEFAULT_SIZE 48
#define MIN_ICON_SIZE 0
#define MAX_ICON_SIZE 256
#define DEFAULT_ICON_SIZE 0
#define DEFAULT_DARK_MODE FALSE
#define MIN_NROWS 1
#define MAX_NROWS 6
#define DEFAULT_NROWS 1

G_BEGIN_DECLS

#define PANEL_TYPE_WINDOW (panel_window_get_type ())
G_DECLARE_FINAL_TYPE (PanelWindow, panel_window, PANEL, WINDOW, PanelBaseWindow)

GtkWidget *
panel_window_new (GdkScreen *screen,
                  gint id,
                  gint autohide_block) G_GNUC_MALLOC;

gint
panel_window_get_id (PanelWindow *window);

gboolean
panel_window_has_position (PanelWindow *window);

void
panel_window_set_provider_info (PanelWindow *window,
                                GtkWidget *provider,
                                gboolean moving_to_other_panel);

void
panel_window_freeze_autohide (PanelWindow *window);

void
panel_window_thaw_autohide (PanelWindow *window);

void
panel_window_set_locked (PanelWindow *window,
                         gboolean locked);

gboolean
panel_window_get_locked (PanelWindow *window);

void
panel_window_focus (PanelWindow *window);

void
panel_window_migrate_old_properties (PanelWindow *window,
                                     XfconfChannel *xfconf,
                                     const gchar *property_base,
                                     const PanelProperty *old_properties,
                                     const PanelProperty *new_properties);

gboolean
panel_window_pointer_is_outside (PanelWindow *window);

G_END_DECLS

#endif /* !__PANEL_WINDOW_H__ */
