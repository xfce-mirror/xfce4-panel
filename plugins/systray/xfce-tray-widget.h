/* $Id: xfce-tray-widget.h 26462 2007-12-12 12:00:59Z nick $ */
/*
 * Copyright (c) 2007 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __XFCE_TRAY_WIDGET_H__
#define __XFCE_TRAY_WIDGET_H__

typedef struct _XfceTrayWidgetClass XfceTrayWidgetClass;
typedef struct _XfceTrayWidget      XfceTrayWidget;
typedef struct _XfceTrayWidgetChild XfceTrayWidgetChild;

#define XFCE_TRAY_WIDGET_SPACING (2)

#define XFCE_TYPE_TRAY_WIDGET            (xfce_tray_widget_get_type ())
#define XFCE_TRAY_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidget))
#define XFCE_TRAY_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidgetClass))
#define XFCE_IS_TRAY_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_TRAY_WIDGET))
#define XFCE_IS_TRAY_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_TRAY_WIDGET))
#define XFCE_TRAY_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidgetClass))

GType            xfce_tray_widget_get_type           (void) G_GNUC_CONST G_GNUC_INTERNAL;

GtkWidget       *xfce_tray_widget_new                (void) G_GNUC_MALLOC G_GNUC_INTERNAL;

void             xfce_tray_widget_add_with_name      (XfceTrayWidget *tray,
                                                      GtkWidget      *child,
                                                      const gchar    *name) G_GNUC_INTERNAL;

void             xfce_tray_widget_set_arrow_type     (XfceTrayWidget *tray,
                                                      GtkArrowType    arrow_type) G_GNUC_INTERNAL;

GtkArrowType     xfce_tray_widget_get_arrow_type     (XfceTrayWidget *tray) G_GNUC_INTERNAL;

void             xfce_tray_widget_set_rows           (XfceTrayWidget *tray,
                                                      gint            rows) G_GNUC_INTERNAL;

gint             xfce_tray_widget_get_rows           (XfceTrayWidget *tray) G_GNUC_INTERNAL;

void             xfce_tray_widget_name_add           (XfceTrayWidget *tray,
                                                      const gchar    *name,
                                                      gboolean        hidden) G_GNUC_INTERNAL;

void             xfce_tray_widget_name_update        (XfceTrayWidget *tray,
                                                      const gchar    *name,
                                                      gboolean        hidden) G_GNUC_INTERNAL;

gboolean         xfce_tray_widget_name_hidden        (XfceTrayWidget *tray,
                                                      const gchar    *name) G_GNUC_INTERNAL;

GList           *xfce_tray_widget_name_list          (XfceTrayWidget *tray) G_GNUC_MALLOC G_GNUC_INTERNAL;

void             xfce_tray_widget_clear_name_list    (XfceTrayWidget *tray) G_GNUC_INTERNAL;

#endif /* !__XFCE_TRAY_WIDGET_H__ */
