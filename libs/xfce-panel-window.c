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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-marshal.h"
#include "xfce-enum-types.h"
#include "xfce-paint-private.h"
#include "xfce-panel-window.h"

#define XFCE_PANEL_WINDOW_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_PANEL_WINDOW, \
                                  XfcePanelWindowPrivate))

#define HANDLE_WIDTH         XFCE_DEFAULT_HANDLE_WIDTH
#define DEFAULT_ORIENTATION  GTK_ORIENTATION_HORIZONTAL
#define DEFAULT_HANDLE_STYLE XFCE_HANDLE_STYLE_BOTH

#ifndef _
#define _(x) (x)
#endif

enum
{
    ORIENTATION_CHANGED,
    MOVE_START,
    MOVE,
    MOVE_END,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_ORIENTATION,
    PROP_HANDLE_STYLE
};


typedef struct _XfcePanelWindowPrivate 	XfcePanelWindowPrivate;

struct _XfcePanelWindowPrivate
{
    GtkOrientation orientation;
    XfceHandleStyle handle_style;

    gboolean shown;
    GtkAllocation allocation;

    guint in_move:1;
    int x_offset, y_offset;
    int x_root, y_root;

    XfcePanelWindowMoveFunc move_func;
    gpointer move_data;

    XfcePanelWindowResizeFunc resize_func;
    gpointer resize_data;

    gboolean top_border, bottom_border, left_border, right_border;

    gboolean moveable;
};


/* init */
static void xfce_panel_window_class_init   (XfcePanelWindowClass * klass);

static void xfce_panel_window_init         (XfcePanelWindow * window);


/* GObject */
static void xfce_panel_window_get_property (GObject * object,
				            guint prop_id,
				            GValue * value, 
                                            GParamSpec * pspec);

static void xfce_panel_window_set_property (GObject * object,
				            guint prop_id,
				            const GValue * value,
				            GParamSpec * pspec);


/* GtkWidget */
static void xfce_panel_window_realize    (GtkWidget * widget);

static void xfce_panel_window_unrealize  (GtkWidget * widget);

static void xfce_panel_window_map        (GtkWidget * widget);

static void xfce_panel_window_unmap      (GtkWidget * widget);

static void xfce_panel_window_show       (GtkWidget * widget);

static void xfce_panel_window_hide       (GtkWidget * widget);


/* drawing, size and style */
static gint xfce_panel_window_expose        (GtkWidget * widget,
				             GdkEventExpose * event);

static void xfce_panel_window_size_request  (GtkWidget * widget,
					     GtkRequisition * requisition);

static void xfce_panel_window_size_allocate (GtkWidget * widget,
					     GtkAllocation * allocation);


/* keyboard and mouse events */
static gboolean xfce_panel_window_button_press   (GtkWidget * widget,
					          GdkEventButton * event);

static gboolean xfce_panel_window_button_release (GtkWidget * widget,
					          GdkEventButton * event);

static gboolean xfce_panel_window_motion_notify  (GtkWidget * widget,
					          GdkEventMotion * event);


/* internal functions */
static void _paint_handle         (XfcePanelWindow * panel_window, 
                                   gboolean start, 
                                   GdkRectangle * area);


/* global vars */
static GtkWindowClass *parent_class = NULL;

static guint panel_window_signals[LAST_SIGNAL] = { 0 };


/* public GType function */
GType
xfce_panel_window_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
        static const GTypeInfo type_info = {
            sizeof (XfcePanelWindowClass),
            (GBaseInitFunc) NULL,
            (GBaseFinalizeFunc) NULL,
            (GClassInitFunc) xfce_panel_window_class_init,
            (GClassFinalizeFunc) NULL,
            NULL,
            sizeof (XfcePanelWindow),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) xfce_panel_window_init,
        };

        type = g_type_register_static (GTK_TYPE_WINDOW,
                                       "XfcePanelWindow", &type_info, 0);
    }

    return type;
}


/* init */
static void
xfce_panel_window_class_init (XfcePanelWindowClass * klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GParamSpec *pspec;

    g_type_class_add_private (klass, sizeof (XfcePanelWindowPrivate));

    parent_class = g_type_class_peek_parent (klass);

    gobject_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gobject_class->get_property = xfce_panel_window_get_property;
    gobject_class->set_property = xfce_panel_window_set_property;

    widget_class->realize = xfce_panel_window_realize;
    widget_class->unrealize = xfce_panel_window_unrealize;
    widget_class->map = xfce_panel_window_map;
    widget_class->unmap = xfce_panel_window_unmap;
    widget_class->show = xfce_panel_window_show;
    widget_class->hide = xfce_panel_window_hide;

    widget_class->expose_event = xfce_panel_window_expose;
    widget_class->size_request = xfce_panel_window_size_request;
    widget_class->size_allocate = xfce_panel_window_size_allocate;

    widget_class->button_press_event = xfce_panel_window_button_press;
    widget_class->button_release_event = xfce_panel_window_button_release;
    widget_class->motion_notify_event = xfce_panel_window_motion_notify;

    /* properties */

    /**
     * XfcePanelWindow::orientation
     *
     * The orientation of the window. This the determines the way the handles
     * are drawn.
     */
    pspec = g_param_spec_enum ("orientation",
                               _("Orientation"),
                               _("The orientation of the itembar"),
                               GTK_TYPE_ORIENTATION,
                               GTK_ORIENTATION_HORIZONTAL, G_PARAM_READWRITE);

    g_object_class_install_property (gobject_class, PROP_ORIENTATION, pspec);


    /**
     * XfcePanelWindow::handle_style
     *
     * The #XfceHandleStyle to use when drawing handles.
     */
    pspec = g_param_spec_enum ("handle_style",
                               _("Handle style"),
                               _("Type of handles to draw"),
                               XFCE_TYPE_HANDLE_STYLE,
                               DEFAULT_HANDLE_STYLE, G_PARAM_READWRITE);

    g_object_class_install_property (gobject_class, PROP_HANDLE_STYLE, pspec);


    /* signals */

    /**
    * XfcePanelWindow::orientation-changed:
    * @window: the object which emitted the signal
    * @orientation: the new #GtkOrientation of the window
    *
    * Emitted when the orientation of the toolbar changes.
    */
    panel_window_signals[ORIENTATION_CHANGED] =
        g_signal_new ("orientation-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass,
                                       orientation_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, GTK_TYPE_ORIENTATION);
    /**
     * XfcePanelWindow::move_start
     * @window: the #XfcePanelWindow which emitted the signal
     *
     * Emitted when the user starts to drag the #XfcePanelWindow.
     */
    panel_window_signals[MOVE_START] =
        g_signal_new ("move-start", G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass, move_start),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * XfcePanelWindow::move
     * @window: the #XfcePanelWindow which emitted the signal
     * @x: x coordinate of the new position
     * @y: y coordinate of the new position
     *
     * Emitted when the user moves the #XfcePanelWindow to a new position with
     * coordinates (@x, @y). 
     */
    panel_window_signals[MOVE] =
        g_signal_new ("move", G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass, move),
                      NULL, NULL,
                      _xfce_marshal_VOID__INT_INT,
                      G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

    /**
     * XfcePanelWindow::move_end
     * @window: the #XfcePanelWindow which emitted the signal
     * @x: x coordinate of the new positions
     * @y: y coordinate of the new position
     *
     * Emitted when the user stops dragging the #XfcePanelWindow. The latest
     * position has coordinates (@x, @y).
     */
    panel_window_signals[MOVE_END] =
        g_signal_new ("move-end", G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass, move_end),
                      NULL, NULL,
                      _xfce_marshal_VOID__INT_INT,
                      G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
}

static void
xfce_panel_window_init (XfcePanelWindow * panel_window)
{
    XfcePanelWindowPrivate *priv;

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (panel_window), GTK_CAN_FOCUS);

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    priv->handle_style      = DEFAULT_HANDLE_STYLE;
    priv->orientation       = DEFAULT_ORIENTATION;
    priv->shown             = FALSE;
    priv->allocation.x      = 0;
    priv->allocation.x      = 0;
    priv->allocation.width  = 0;
    priv->allocation.height = 0;
    priv->in_move           = FALSE;
    priv->x_offset          = 0;
    priv->y_offset          = 0;
    priv->x_root            = 0;
    priv->y_root            = 0;
    priv->move_func         = NULL;
    priv->move_data         = NULL;
    priv->resize_func       = NULL;
    priv->resize_data       = NULL;
    priv->top_border        = TRUE;
    priv->bottom_border     = TRUE;
    priv->left_border       = TRUE;
    priv->right_border      = TRUE;
    priv->moveable          = TRUE;

    gtk_widget_set_events (GTK_WIDGET (panel_window),
                           gtk_widget_get_events (GTK_WIDGET (panel_window))
                           | GDK_BUTTON_MOTION_MASK
                           | GDK_BUTTON_PRESS_MASK
                           | GDK_BUTTON_RELEASE_MASK
                           | GDK_EXPOSURE_MASK 
                           | GDK_ENTER_NOTIFY_MASK 
                           | GDK_LEAVE_NOTIFY_MASK);
}

/* GObject */
static void
xfce_panel_window_get_property (GObject * object,
                                guint prop_id, GValue * value,
                                GParamSpec * pspec)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            g_value_set_enum (value, priv->orientation);
            break;
        case PROP_HANDLE_STYLE:
            g_value_set_enum (value, priv->handle_style);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
xfce_panel_window_set_property (GObject * object, guint prop_id,
                                const GValue * value, GParamSpec * pspec)
{
    XfcePanelWindow *window = XFCE_PANEL_WINDOW (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            xfce_panel_window_set_orientation (window,
                                               g_value_get_enum (value));
            break;
        case PROP_HANDLE_STYLE:
            xfce_panel_window_set_handle_style (window,
                                                g_value_get_enum (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* GtkWidget */
static void
xfce_panel_window_realize (GtkWidget * widget)
{
    gtk_window_stick (GTK_WINDOW (widget));

    GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

static void
xfce_panel_window_unrealize (GtkWidget * widget)
{
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
xfce_panel_window_map (GtkWidget * widget)
{
    GTK_WIDGET_CLASS (parent_class)->map (widget);
}

static void
xfce_panel_window_unmap (GtkWidget * widget)
{
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
xfce_panel_window_show (GtkWidget * widget)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (widget);

    GTK_WIDGET_CLASS (parent_class)->show (widget);

    if (!priv->shown)
    {
        gdk_window_get_root_origin (widget->window,
                                    &(priv->x_root), &(priv->y_root));

        priv->shown = TRUE;
    }
}

static void
xfce_panel_window_hide (GtkWidget * widget)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (widget);

    GTK_WIDGET_CLASS (parent_class)->hide (widget);

    priv->shown = FALSE;
}

/* drawing, size and style */
static void
_panel_window_paint_border (XfcePanelWindow * panel)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel);
    GdkWindow *window = GTK_WIDGET (panel)->window;
    GtkAllocation *a = &(GTK_WIDGET (panel)->allocation);
    GtkStyle *style = GTK_WIDGET (panel)->style;
    GtkStateType state_type = GTK_WIDGET_STATE (GTK_WIDGET (panel));
    int x, y, width, height;
    int top, bottom, left, right;

    x = a->x;
    y = a->y;
    width = a->width;
    height = a->height;

    top    = priv->top_border    ? style->ythickness : 0;
    bottom = priv->bottom_border ? style->ythickness : 0;
    left   = priv->left_border   ? style->xthickness : 0;
    right  = priv->right_border  ? style->xthickness : 0;
    
    /* Code taken from gtk-xfce-engine-2 */
    if (top > 1)
    {
        gdk_draw_line (window, style->dark_gc[state_type], x, y,
                       x + width - 1, y);
        gdk_draw_line (window, style->light_gc[state_type], x + 1, y + 1,
                       x + width - 2, y + 1);
    }
    else if (top > 0)
    {
        gdk_draw_line (window, style->light_gc[state_type], x, y,
                       x + width - 1, y);
    }
    
    if (bottom > 1)
    {
        gdk_draw_line (window, style->black_gc, x + 1, y + height - 1,
                       x + width - 1, y + height - 1);

        gdk_draw_line (window, style->dark_gc[state_type], x + 2,
                       y + height - 2, x + width - 2, y + height - 2);
    }
    else if (bottom > 0)
    {
        gdk_draw_line (window, style->dark_gc[state_type], x + 1,
                       y + height - 1, x + width - 1, y + height - 1);
    }

    if (left > 1)
    {
        gdk_draw_line (window, style->dark_gc[state_type], x, y, x,
                       y + height - 1);

        gdk_draw_line (window, style->light_gc[state_type], x + 1, y + 1,
                       x + 1, y + height - 2);
    }
    else if (left > 0)
    {
        gdk_draw_line (window, style->light_gc[state_type], x, y, x,
                       y + height - 1);
    }

    if (right > 1)
    {
        gdk_draw_line (window, style->black_gc, x + width - 1, y + 1,
                       x + width - 1, y + height - 1);
        
        gdk_draw_line (window, style->dark_gc[state_type], x + width - 2,
                       y + 2, x + width - 2, y + height - 2);
    }
    else if (right > 0)
    {
        gdk_draw_line (window, style->dark_gc[state_type], x + width - 1,
                       y + 1, x + width - 1, y + height - 1);
    }
}

static gint
xfce_panel_window_expose (GtkWidget * widget, GdkEventExpose * event)
{
    XfcePanelWindow *panel_window = XFCE_PANEL_WINDOW (widget);
    XfcePanelWindowPrivate *priv =
        XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (GTK_WIDGET_DRAWABLE (widget))
    {
        if (GTK_BIN (widget)->child)
        {
            gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                            GTK_BIN (widget)->child, event);
        }

        switch (priv->handle_style)
        {
            case XFCE_HANDLE_STYLE_BOTH:
                _paint_handle (panel_window, TRUE, &event->area);
                _paint_handle (panel_window, FALSE, &event->area);
                break;
            case XFCE_HANDLE_STYLE_START:
                _paint_handle (panel_window, TRUE, &event->area);
                break;
            case XFCE_HANDLE_STYLE_END:
                _paint_handle (panel_window, FALSE, &event->area);
                break;
            default:
                break;
        }
    
        _panel_window_paint_border (panel_window);
    }

    return FALSE;
}

static void
xfce_panel_window_size_request (GtkWidget * widget,
                                GtkRequisition * requisition)
{
    XfcePanelWindow *panel_window = XFCE_PANEL_WINDOW (widget);
    XfcePanelWindowPrivate *priv =
        XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);
    int handle_size, thick;

    requisition->width = requisition->height = 0;

    if (GTK_BIN (widget)->child)
    {
        gtk_widget_size_request (GTK_BIN (widget)->child, requisition);
    }
    
    if (priv->top_border)
        requisition->height += widget->style->ythickness;
    if (priv->bottom_border)
        requisition->height += widget->style->ythickness;
    if (priv->left_border)
        requisition->height += widget->style->xthickness;
    if (priv->right_border)
        requisition->height += widget->style->xthickness;

    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        thick = 2 * widget->style->xthickness;
    }
    else
    {
        thick = 2 * widget->style->ythickness;
    }

    switch (priv->handle_style)
    {
        case XFCE_HANDLE_STYLE_BOTH:
            handle_size = 2 * (HANDLE_WIDTH + thick);
            break;
        case XFCE_HANDLE_STYLE_START:
        case XFCE_HANDLE_STYLE_END:
            handle_size = HANDLE_WIDTH + thick;
            break;
        default:
            handle_size = 0;
    }

    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        requisition->width += handle_size;
    else
        requisition->height += handle_size;
}

static void
xfce_panel_window_size_allocate (GtkWidget * widget,
                                 GtkAllocation * allocation)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (widget);

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
    {
        if (priv->shown)
        {
            gdk_window_get_root_origin (widget->window,
                                        &(priv->x_root), &(priv->y_root));

            if (priv->resize_func && priv->shown)
            {
                GtkAllocation old, new;

                old = priv->allocation;
                new = *allocation;

                priv->resize_func (widget, priv->resize_data,
                                   &old, &new,
                                   &(priv->x_root), &(priv->y_root));
            }
        }

        gdk_window_move_resize (widget->window,
                                priv->x_root, priv->y_root,
                                allocation->width, allocation->height);
    }

    priv->allocation = *allocation;

    if (GTK_BIN (widget)->child)
    {
        GtkAllocation childalloc;
        int start_handle_size, end_handle_size, thick;

        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
            thick = 2 * widget->style->xthickness;
        else
            thick = 2 * widget->style->ythickness;

        start_handle_size = end_handle_size = 0;

        switch (priv->handle_style)
        {
            case XFCE_HANDLE_STYLE_BOTH:
                start_handle_size = end_handle_size = HANDLE_WIDTH + thick;
                break;
            case XFCE_HANDLE_STYLE_START:
                start_handle_size = HANDLE_WIDTH + thick;
                break;
            case XFCE_HANDLE_STYLE_END:
                end_handle_size = HANDLE_WIDTH + thick;
                break;
            default:
                break;
        }

        childalloc = *allocation;

        if (priv->top_border)
        {
            childalloc.y += widget->style->ythickness;
            childalloc.height -= widget->style->ythickness;
        }
        
        if (priv->bottom_border)
            childalloc.height -= widget->style->ythickness;
        
        if (priv->left_border)
        {
            childalloc.x += widget->style->xthickness;
            childalloc.width -= widget->style->xthickness;
        }
        
        if (priv->right_border)
            childalloc.width -= widget->style->xthickness;
            

        if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
            childalloc.x += start_handle_size;
            childalloc.width -= start_handle_size + end_handle_size;
        }
        else
        {
            childalloc.y += start_handle_size;
            childalloc.height -= start_handle_size + end_handle_size;
        }

        gtk_widget_size_allocate (GTK_BIN (widget)->child, &childalloc);
    }

    gtk_widget_queue_draw (widget);
}

/* keyboard and mouse events */
static gboolean
xfce_panel_window_button_press (GtkWidget * widget, GdkEventButton * event)
{
    XfcePanelWindow *panel_window;
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (priv->moveable && 
        event->button == 1 && (event->window == widget->window))
    {
        GdkCursor *fleur;

        gdk_window_get_root_origin (widget->window,
                                    &(priv->x_root), &(priv->y_root));

        priv->in_move = TRUE;

        priv->x_offset = priv->x_root - event->x_root;
        priv->y_offset = priv->y_root - event->y_root;

        fleur = gdk_cursor_new (GDK_FLEUR);
        if (gdk_pointer_grab (widget->window, FALSE,
                              GDK_BUTTON1_MOTION_MASK |
                              GDK_POINTER_MOTION_HINT_MASK |
                              GDK_BUTTON_RELEASE_MASK,
                              NULL, fleur, event->time) != 0)
        {
            priv->in_move = FALSE;
        }
        gdk_cursor_unref (fleur);

        g_signal_emit (widget, panel_window_signals[MOVE_START], 0);
        return TRUE;
    }

    return FALSE;
}

static gboolean
xfce_panel_window_button_release (GtkWidget * widget, GdkEventButton * event)
{
    XfcePanelWindow *panel_window;
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (!priv->in_move)
        return FALSE;

    gdk_pointer_ungrab (event->time);
    priv->in_move = FALSE;
    gdk_window_get_origin (widget->window, &(priv->x_root), &(priv->y_root));

    g_signal_emit (widget, panel_window_signals[MOVE_END], 0,
                   priv->x_root, priv->y_root);

    return TRUE;
}

static gboolean
xfce_panel_window_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
    XfcePanelWindow *panel_window;
    XfcePanelWindowPrivate *priv;
    int new_x, new_y;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (!priv->in_move)
    {
        return FALSE;
    }

    gdk_window_get_pointer (NULL, &new_x, &new_y, NULL);
    new_x += priv->x_offset;
    new_y += priv->y_offset;

    if (priv->move_func)
    {
        priv->move_func (widget, priv->move_data, &new_x, &new_y);
    }

    priv->x_root = new_x;
    priv->y_root = new_y;

    gdk_window_move (widget->window, new_x, new_y);

    g_signal_emit (widget, panel_window_signals[MOVE], 0, new_x, new_y);

    return TRUE;
}

/* internal functions */
static void
_paint_handle (XfcePanelWindow * panel_window, gboolean start,
               GdkRectangle * area)
{
    XfcePanelWindowPrivate *priv =
        XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);
    GtkWidget *widget = GTK_WIDGET (panel_window);
    GtkAllocation *alloc = &(widget->allocation);
    int x, y, w, h, xthick, ythick;
    gboolean horizontal = priv->orientation == GTK_ORIENTATION_HORIZONTAL;

    xthick = widget->style->xthickness;
    ythick = widget->style->ythickness;

    if (horizontal)
    {
        w = HANDLE_WIDTH;
        h = alloc->height - 2 * ythick;
    }
    else
    {
        w = alloc->width - 2 * xthick;
        h = HANDLE_WIDTH;
    }

    if (start)
    {
        x = alloc->x + xthick;
        y = alloc->y + ythick;
    }
    else
    {
        if (horizontal)
        {
            x = alloc->x + alloc->width - HANDLE_WIDTH - xthick;
            y = alloc->y + ythick;
        }
        else
        {
            x = alloc->x + xthick;
            y = alloc->y + alloc->height - HANDLE_WIDTH - ythick;
        }
    }

    xfce_paint_handle (widget, area, "xfce_panel_window",
                       horizontal ? GTK_ORIENTATION_VERTICAL :
                       GTK_ORIENTATION_HORIZONTAL, x, y, w, h);
}

/* public interface */

/**
 * xfce_panel_window_new
 *
 * Create new panel window.
 *
 * Returns: New #GtkWidget
 **/
GtkWidget *
xfce_panel_window_new (void)
{
    XfcePanelWindow *window;

    window = g_object_new (XFCE_TYPE_PANEL_WINDOW,
                           "type", GTK_WINDOW_TOPLEVEL,
                           "decorated", FALSE,
                           "resizable", FALSE,
                           "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
                           NULL);

    gtk_window_stick (GTK_WINDOW (window));
    
    return GTK_WIDGET (window);
}

/**
 * xfce_panel_window_get_orientation
 * @window: #XfcePanelWindow
 * 
 * Get orientation of panel window.
 *
 * Returns: #GtkOrientation of @window
 **/
GtkOrientation
xfce_panel_window_get_orientation (XfcePanelWindow * window)
{
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (window), DEFAULT_ORIENTATION);

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);


    return priv->orientation;
}

/**
 * xfce_panel_window_set_orientation
 * @window     : #XfcePanelWindow
 * @orientation: new #GtkOrientation
 * 
 * Set orientation for panel window.
 **/
void
xfce_panel_window_set_orientation (XfcePanelWindow * window,
                                   GtkOrientation orientation)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->orientation = orientation;

    g_signal_emit (G_OBJECT (window),
                   panel_window_signals[ORIENTATION_CHANGED], 0, orientation);

    g_object_notify (G_OBJECT (window), "orientation");
}

/**
 * xfce_panel_window_get_handle_style
 * @window: #XfcePanelWindow
 * 
 * Get handle style of panel window.
 *
 * Returns: #XfceHandleStyle of @window
 **/
XfceHandleStyle
xfce_panel_window_get_handle_style (XfcePanelWindow * window)
{
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (window),
                          DEFAULT_HANDLE_STYLE);

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    return priv->handle_style;
}

/**
 * xfce_panel_window_set_handle_style
 * @window      : #XfcePanelWindow
 * @handle_style: new #XfceHandleStyle
 * 
 * Set handle style for panel window.
 **/
void
xfce_panel_window_set_handle_style (XfcePanelWindow * window,
                                    XfceHandleStyle handle_style)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    if (handle_style == priv->handle_style)
        return;

    priv->handle_style = handle_style;

    g_object_notify (G_OBJECT (window), "handle_style");

    gtk_widget_queue_resize (GTK_WIDGET (window));
}

/**
 * xfce_panel_window_get_show_border
 * @window        : #XfcePanelWindow
 * @top_border   : location for top border or %NULL
 * @bottom_border: location for bottom border or %NULL
 * @left_border  : location for left border or %NULL
 * @right_border : location for right border or %NULL
 * 
 * Get visibility of panel window borders.
 **/
void
xfce_panel_window_get_show_border (XfcePanelWindow * window, 
                                   gboolean *top_border, 
                                   gboolean *bottom_border, 
                                   gboolean *left_border, 
                                   gboolean *right_border)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    if (top_border != NULL)
        *top_border = priv->top_border;
    if (bottom_border != NULL)
        *bottom_border = priv->bottom_border;
    if (left_border != NULL)
        *left_border = priv->left_border;
    if (right_border != NULL)
        *right_border = priv->right_border;
}

/**
 * xfce_panel_window_get_show_border
 * @window        : #XfcePanelWindow
 * @top_border   : show top border
 * @bottom_border: show bottom border
 * @left_border  : show left border
 * @right_border : show right border
 * 
 * Set border visibility for the panel window.
 **/
void
xfce_panel_window_set_show_border (XfcePanelWindow * window, 
                                   gboolean top_border, 
                                   gboolean bottom_border, 
                                   gboolean left_border, 
                                   gboolean right_border)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    if (top_border == priv->top_border
        && bottom_border == priv->bottom_border
        && left_border == priv->left_border
        && right_border == priv->right_border)
    {
        return;
    }
    
    priv->top_border = top_border;
    priv->bottom_border = bottom_border;
    priv->left_border = left_border;
    priv->right_border = right_border;

    gtk_widget_queue_resize (GTK_WIDGET (window));
}

/**
 * xfce_panel_window_set_move_function
 * @window  : #XfcePanelWindow
 * @function: #XfcePanelWindowMoveFunc
 * @data    : user data
 * 
 * Set a function to modify move behaviour of the panel window.
 **/
void
xfce_panel_window_set_move_function (XfcePanelWindow * window,
                                     XfcePanelWindowMoveFunc function,
                                     gpointer data)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->move_func = function;
    priv->move_data = data;
}

/**
 * xfce_panel_window_set_resize_function
 * @window  : #XfcePanelWindow
 * @function: #XfcePanelWindowResizeFunc
 * @data    : user data
 * 
 * Set a function to modify resize behaviour of the panel window.
 **/
void
xfce_panel_window_set_resize_function (XfcePanelWindow * window,
                                       XfcePanelWindowResizeFunc function, 
                                       gpointer data)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->resize_func = function;
    priv->resize_data = data;
}

/**
 * xfce_panel_window_get_moveable
 * @window  : #XfcePanelWindow
 *
 * Check if the panel window can be moved by the user.
 * By default this is %TRUE.
 *
 * Returns: %TRUE if the user is allowed to move @window, %FALSE if not.
 **/
gboolean
xfce_panel_window_get_moveable (XfcePanelWindow * window)
{
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (window), TRUE);

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    return priv->moveable;
}

/**
 * xfce_panel_window_get_moveable
 * @window  : #XfcePanelWindow
 * @moveable: %TRUE or %FALSE.
 *
 * Set if the panel window can be moved by the user.
 **/
void
xfce_panel_window_set_moveable (XfcePanelWindow * window, gboolean moveable)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->moveable = moveable;
}
