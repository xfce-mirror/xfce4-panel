/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2004-2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_PANEL_WINDOW_H
#define _XFCE_PANEL_WINDOW_H

#include <gtk/gtkenums.h>
#include <gtk/gtkwindow.h>

#define XFCE_TYPE_PANEL_WINDOW            (xfce_panel_window_get_type ())
#define XFCE_PANEL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_WINDOW, XfcePanelWindow))
#define XFCE_PANEL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PANEL_WINDOW, XfcePanelWindowClass))
#define XFCE_IS_PANEL_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_WINDOW))
#define XFCE_IS_PANEL_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_PANEL_WINDOW))
#define XFCE_PANEL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_PANEL_WINDOW, XfcePanelWindowClass))


G_BEGIN_DECLS

typedef enum /*<enum,prefix=XFCE_HANDLE_STYLE >*/
{
    XFCE_HANDLE_STYLE_NONE,
    XFCE_HANDLE_STYLE_BOTH,
    XFCE_HANDLE_STYLE_START,
    XFCE_HANDLE_STYLE_END
}
XfceHandleStyle;


typedef struct _XfcePanelWindow 	  XfcePanelWindow;

typedef struct _XfcePanelWindowClass 	  XfcePanelWindowClass;


typedef void (*XfcePanelWindowMoveFunc)   (GtkWidget * widget,
                                           gpointer data, 
                                           int *x, 
                                           int *y);

typedef void (*XfcePanelWindowResizeFunc) (GtkWidget * widget, 
                                           gpointer data, 
                                           GtkAllocation * previous, 
                                           GtkAllocation * allocation, 
                                           int *x, 
                                           int *y); 

struct _XfcePanelWindow
{
    GtkWindow window;
};

struct _XfcePanelWindowClass
{
    GtkWindowClass parent_class;

    /* signals */
    void (*orientation_changed) (GtkWidget * widget,
                                 GtkOrientation orientation);

    void (*move_start)          (GtkWidget * widget);

    void (*move)                (GtkWidget * widget, 
                                 gint new_x, 
                                 gint new_y);

    void (*move_end)            (GtkWidget * widget, 
                                 gint new_x, 
                                 gint new_y);

    /* Padding for future expansion */
    void (*_xfce_reserved1)     (void);
    void (*_xfce_reserved2)     (void);
    void (*_xfce_reserved3)     (void);
};


GType xfce_panel_window_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_panel_window_new (void);


GtkOrientation xfce_panel_window_get_orientation   (XfcePanelWindow * window);

void xfce_panel_window_set_orientation             (XfcePanelWindow * window,
                                                    GtkOrientation orientation);


XfceHandleStyle xfce_panel_window_get_handle_style (XfcePanelWindow * window);

void xfce_panel_window_set_handle_style            (XfcePanelWindow * window,
                                                    XfceHandleStyle style);


void xfce_panel_window_get_show_border             (XfcePanelWindow * window,
                                                    gboolean * top,
                                                    gboolean * bottom,
                                                    gboolean * left,
                                                    gboolean * right);

void xfce_panel_window_set_show_border             (XfcePanelWindow * window,
                                                    gboolean top,
                                                    gboolean bottom,
                                                    gboolean left,
                                                    gboolean right);


void xfce_panel_window_set_move_function           (XfcePanelWindow * window,
                                                    XfcePanelWindowMoveFunc 
                                                        function,
                                                    gpointer data);

void xfce_panel_window_set_resize_function         (XfcePanelWindow * window,
                                                    XfcePanelWindowResizeFunc 
                                                        function,
                                                    gpointer data);


gboolean xfce_panel_window_get_moveable            (XfcePanelWindow * window);

void xfce_panel_window_set_moveable                (XfcePanelWindow * window,
                                                    gboolean moveable);

G_END_DECLS

#endif /* _XFCE_PANEL_WINDOW_H */
