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

G_BEGIN_DECLS

void
_panel_utils_weak_notify (gpointer data,
                          GObject *where_the_object_was);

GtkBuilder *
panel_utils_builder_new (XfcePanelPlugin *panel_plugin,
                         const gchar *buffer,
                         gsize length,
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

G_END_DECLS

#endif /* !__PANEL_UTILS_H__ */
