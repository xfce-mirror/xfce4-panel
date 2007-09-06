/* $Id$ */
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

#define XFCE_TYPE_TRAY_WIDGET            (xfce_tray_widget_get_type ())
#define XFCE_TRAY_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidget))
#define XFCE_TRAY_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidgetClass))
#define XFCE_IS_TRAY_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_TRAY_WIDGET))
#define XFCE_IS_TRAY_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_TRAY_WIDGET))
#define XFCE_TRAY_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_TRAY_WIDGET, XfceTrayWidgetClass))

GType            xfce_tray_widget_get_type           (void) G_GNUC_CONST G_GNUC_INTERNAL;

GtkWidget       *xfce_tray_widget_new_for_screen     (GdkScreen       *screen, 
                                                      GtkArrowType     arrow_position,
                                                      GError         **error) G_GNUC_MALLOC G_GNUC_INTERNAL;
                                                      
void             xfce_tray_widget_sort               (XfceTrayWidget   *tray) G_GNUC_INTERNAL;
                                                
XfceTrayManager *xfce_tray_widget_get_manager        (XfceTrayWidget   *tray) G_GNUC_INTERNAL;

void             xfce_tray_widget_set_arrow_position (XfceTrayWidget   *tray, 
                                                      GtkArrowType     arrow_position) G_GNUC_INTERNAL;

void             xfce_tray_widget_set_size_request   (XfceTrayWidget   *tray,
                                                      gint              size) G_GNUC_INTERNAL;

#endif /* !__XFCE_TRAY_WIDGET_H__ */
