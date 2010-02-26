/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_APPLICATION_H__
#define __PANEL_APPLICATION_H__

#include <gtk/gtk.h>
#include <panel/panel-window.h>

G_BEGIN_DECLS

typedef struct _PanelApplicationClass PanelApplicationClass;
typedef struct _PanelApplication      PanelApplication;

#define PANEL_TYPE_APPLICATION            (panel_application_get_type ())
#define PANEL_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_APPLICATION, PanelApplication))
#define PANEL_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_APPLICATION, PanelApplicationClass))
#define PANEL_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_APPLICATION))
#define PANEL_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_APPLICATION))
#define PANEL_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_APPLICATION, PanelApplicationClass))

GType             panel_application_get_type          (void) G_GNUC_CONST;

void              panel_application_multihead_area    (GtkWidget *widget,
                                                       gboolean is_horizontal,
                                                       gboolean span_monitors);

PanelApplication *panel_application_get               (void);

void              panel_application_save              (PanelApplication  *application,
                                                       gboolean           save_plugin_providers);

void              panel_application_take_dialog       (PanelApplication  *application,
                                                       GtkWindow         *dialog);

void              panel_application_destroy_dialogs   (PanelApplication  *application);

void              panel_application_add_new_item      (PanelApplication  *application,
                                                       const gchar       *plugin_name,
                                                       gchar            **arguments);

PanelWindow      *panel_application_new_window        (PanelApplication  *application,
                                                       GdkScreen         *screen,
                                                       gboolean           new_window);

guint             panel_application_get_n_windows     (PanelApplication  *application);

gint              panel_application_get_window_index  (PanelApplication  *application,
                                                       PanelWindow       *window);

PanelWindow      *panel_application_get_nth_window    (PanelApplication  *application,
                                                       guint              idx);

void              panel_application_window_select     (PanelApplication  *application,
                                                       PanelWindow       *window);

void              panel_application_windows_sensitive (PanelApplication  *application,
                                                       gboolean           sensitive);

void              panel_application_windows_autohide  (PanelApplication  *application,
                                                       gboolean           freeze);

G_END_DECLS

#endif /* !__PANEL_APPLICATION_H__ */
