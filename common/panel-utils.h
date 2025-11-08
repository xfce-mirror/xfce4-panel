/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __PANEL_UTILS_H__
#define __PANEL_UTILS_H__

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

enum
{
  OUTPUT_NAME,
  OUTPUT_TITLE
};


G_BEGIN_DECLS

GtkBuilder *
panel_utils_builder_new (XfcePanelPlugin *panel_plugin,
                         const gchar *resource,
                         GObject **dialog_return) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void
panel_utils_show_help (GtkWindow *parent,
                       const gchar *page,
                       const gchar *offset);

gboolean
panel_utils_device_grab (GtkWidget *widget);

void
panel_utils_set_atk_info (GtkWidget *widget,
                          const gchar *name,
                          const gchar *description);

void
panel_utils_destroy_later (GtkWidget *widget);

void
panel_utils_wl_surface_commit (GtkWidget *widget);

void
panel_utils_widget_remap (GtkWidget *widget);

GtkLabel *
panel_utils_gtk_dialog_find_label_by_text (GtkDialog *dialog,
                                           const gchar *label_text);

gint
panel_utils_compare_xfw_gdk_monitors (gconstpointer a,
                                      gconstpointer b);

GdkMonitor *
panel_utils_get_monitor_at_widget (GtkWidget *widget);

GList *
panel_utils_list_workspace_groups_for_monitor (XfwScreen *xfw_screen,
                                               GdkMonitor *monitor);

GList *
panel_utils_list_workspaces_for_monitor (XfwScreen *xfw_screen,
                                         GdkMonitor *monitor);

XfwWorkspace *
panel_utils_get_active_workspace_for_monitor (XfwScreen *xfw_screen,
                                              GdkMonitor *monitor);

guint
panel_utils_get_workspace_count_for_monitor (XfwScreen *xfw_screen,
                                             GdkMonitor *monitor);

gint
panel_utils_get_workspace_number_for_monitor (XfwScreen *xfw_screen,
                                              GdkMonitor *monitor,
                                              XfwWorkspace *workspace);

void
panel_utils_populate_output_list (GtkListStore *store,
                                  GtkComboBox *box,
                                  const gchar *output_name,
                                  GdkDisplay *display,
                                  gint n_monitors,
                                  gboolean *output_selected,
                                  GtkTreeIter *iter,
                                  gint *n);

G_END_DECLS

#endif /* !__PANEL_UTILS_H__ */
