/* $Id$
 *
 * Copyright (c) 2004-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-panel-macros.h"
#include "xfce-marshal.h"
#include "xfce-panel-enum-types.h"
#include "xfce-panel-window.h"

#define XFCE_PANEL_WINDOW_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_PANEL_WINDOW, \
                                  XfcePanelWindowPrivate))

#define DEFAULT_ORIENTATION   GTK_ORIENTATION_HORIZONTAL
#define DEFAULT_HANDLE_STYLE  XFCE_HANDLE_STYLE_BOTH
#define HANDLE_WIDTH          8

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


typedef struct _XfcePanelWindowPrivate XfcePanelWindowPrivate;

struct _XfcePanelWindowPrivate
{
    GtkOrientation            orientation;
    XfceHandleStyle           handle_style;

    GtkAllocation             allocation;

    gint                      x_offset;
    gint                      y_offset;
    gint                      x_root;
    gint                      y_root;

    XfcePanelWindowMoveFunc   move_func;
    gpointer                  move_data;

    XfcePanelWindowResizeFunc resize_func;
    gpointer                  resize_data;

    guint                     shown : 1;
    guint                     in_move : 1;
    guint                     top_border : 1;
    guint                     bottom_border : 1;
    guint                     left_border : 1;
    guint                     right_border : 1;
    guint                     movable : 1;
};




static void      xfce_panel_window_class_init      (XfcePanelWindowClass  *klass);
static void      xfce_panel_window_init            (XfcePanelWindow       *window);
static void      xfce_panel_window_get_property    (GObject               *object,
                                                    guint                  prop_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);
static void      xfce_panel_window_set_property    (GObject               *object,
                                                    guint                  prop_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void      xfce_panel_window_realize         (GtkWidget             *widget);
static void      xfce_panel_window_unrealize       (GtkWidget             *widget);
static void      xfce_panel_window_map             (GtkWidget             *widget);
static void      xfce_panel_window_unmap           (GtkWidget             *widget);
static void      xfce_panel_window_show            (GtkWidget             *widget);
static void      xfce_panel_window_hide            (GtkWidget             *widget);
static gint      xfce_panel_window_expose          (GtkWidget             *widget,
                                                    GdkEventExpose        *event);
static void      xfce_panel_window_size_request    (GtkWidget             *widget,
                                                    GtkRequisition        *requisition);
static void      xfce_panel_window_size_allocate   (GtkWidget             *widget,
                                                    GtkAllocation         *allocation);
static gboolean  xfce_panel_window_button_press    (GtkWidget             *widget,
                                                    GdkEventButton        *event);
static gboolean  xfce_panel_window_button_release  (GtkWidget             *widget,
                                                    GdkEventButton        *event);
static gboolean  xfce_panel_window_motion_notify   (GtkWidget             *widget,
                                                    GdkEventMotion        *event);
static void      _paint_handle                     (XfcePanelWindow       *panel_window,
                                                    gboolean               start,
                                                    GdkRectangle          *area);



/* global vars */
static GtkWindowClass *parent_class = NULL;
static guint           panel_window_signals[LAST_SIGNAL] = { 0 };



/* public GType function */
GType
xfce_panel_window_get_type (void)
{
    static GtkType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        static const GTypeInfo type_info = {
            sizeof (XfcePanelWindowClass),
            NULL,
            NULL,
            (GClassInitFunc) xfce_panel_window_class_init,
            NULL,
            NULL,
            sizeof (XfcePanelWindow),
            0,
            (GInstanceInitFunc) xfce_panel_window_init,
            NULL
        };

        type = g_type_register_static (GTK_TYPE_WINDOW,
                                       I_("XfcePanelWindow"), &type_info, 0);
    }

    return type;
}



/* init */
static void
xfce_panel_window_class_init (XfcePanelWindowClass *klass)
{
    GObjectClass      *gobject_class;
    GtkWidgetClass    *widget_class;
    GtkContainerClass *container_class;

    g_type_class_add_private (klass, sizeof (XfcePanelWindowPrivate));

    parent_class    = g_type_class_peek_parent (klass);
    gobject_class   = (GObjectClass *) klass;
    widget_class    = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gobject_class->get_property        = xfce_panel_window_get_property;
    gobject_class->set_property        = xfce_panel_window_set_property;

    widget_class->realize              = xfce_panel_window_realize;
    widget_class->unrealize            = xfce_panel_window_unrealize;
    widget_class->map                  = xfce_panel_window_map;
    widget_class->unmap                = xfce_panel_window_unmap;
    widget_class->show                 = xfce_panel_window_show;
    widget_class->hide                 = xfce_panel_window_hide;

    widget_class->expose_event         = xfce_panel_window_expose;
    widget_class->size_request         = xfce_panel_window_size_request;
    widget_class->size_allocate        = xfce_panel_window_size_allocate;

    widget_class->button_press_event   = xfce_panel_window_button_press;
    widget_class->button_release_event = xfce_panel_window_button_release;
    widget_class->motion_notify_event  = xfce_panel_window_motion_notify;

    /* signals */

    /**
    * XfcePanelWindow::orientation-changed:
    * @window: the object which emitted the signal
    * @orientation: the new #GtkOrientation of the window
    *
    * Emitted when the orientation of the #XfcePanelWindow changes.
    **/
    panel_window_signals[ORIENTATION_CHANGED] =
        g_signal_new (I_("orientation-changed"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass,
                                       orientation_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE,
                      1,
                      GTK_TYPE_ORIENTATION);

    /**
     * XfcePanelWindow::move-start
     * @window: the #XfcePanelWindow which emitted the signal
     *
     * Emitted when the user starts to drag the #XfcePanelWindow.
     **/
    panel_window_signals[MOVE_START] =
        g_signal_new (I_("move-start"),
                      G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass,
                                       move_start),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE,
                      0);

    /**
     * XfcePanelWindow::move
     * @window: the #XfcePanelWindow which emitted the signal
     * @x: x coordinate of the new position
     * @y: y coordinate of the new position
     *
     * Emitted when the user moves the #XfcePanelWindow to a new position with
     * coordinates (@x, @y).
     **/
    panel_window_signals[MOVE] =
        g_signal_new (I_("move"),
                      G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass,
                                       move),
                      NULL, NULL,
                      _xfce_marshal_VOID__INT_INT,
                      G_TYPE_NONE,
                      2,
                      G_TYPE_INT,
                      G_TYPE_INT);

    /**
     * XfcePanelWindow::move-end
     * @window: the #XfcePanelWindow which emitted the signal
     * @x: x coordinate of the new positions
     * @y: y coordinate of the new position
     *
     * Emitted when the user stops dragging the #XfcePanelWindow. The latest
     * position has coordinates (@x, @y).
     **/
    panel_window_signals[MOVE_END] =
        g_signal_new (I_("move-end"),
                      G_OBJECT_CLASS_TYPE (widget_class),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (XfcePanelWindowClass,
                                       move_end),
                      NULL, NULL,
                      _xfce_marshal_VOID__INT_INT,
                      G_TYPE_NONE,
                      2,
                      G_TYPE_INT,
                      G_TYPE_INT);

    /* properties */

    /**
     * XfcePanelWindow:orientation
     *
     * The orientation of the window. This the determines the way the handles
     * are drawn.
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_ORIENTATION,
                                     g_param_spec_enum ("orientation",
                                                        "Orientation",
                                                        "The orientation of the itembar",
                                                        GTK_TYPE_ORIENTATION,
                                                        GTK_ORIENTATION_HORIZONTAL,
                                                        G_PARAM_READWRITE));


    /**
     * XfcePanelWindow:handle-style
     *
     * The #XfceHandleStyle to use when drawing handles.
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_HANDLE_STYLE,
                                     g_param_spec_enum ("handle-style",
                                                        "Handle style",
                                                        "Type of handles to draw",
                                                        XFCE_TYPE_HANDLE_STYLE,
                                                        DEFAULT_HANDLE_STYLE,
                                                        G_PARAM_READWRITE));
}



static void
xfce_panel_window_init (XfcePanelWindow *panel_window)
{
    XfcePanelWindowPrivate *priv;

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (panel_window), GTK_CAN_FOCUS);

    g_object_set (G_OBJECT (panel_window),
                  "type", GTK_WINDOW_TOPLEVEL,
                  "decorated", FALSE,
                  "resizable", FALSE,
                  "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
                  NULL);

    gtk_window_stick (GTK_WINDOW (panel_window));

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
    priv->movable           = TRUE;

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
xfce_panel_window_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
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
xfce_panel_window_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    XfcePanelWindow *window = XFCE_PANEL_WINDOW (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            xfce_panel_window_set_orientation (window, g_value_get_enum (value));
            break;
        case PROP_HANDLE_STYLE:
            xfce_panel_window_set_handle_style (window, g_value_get_enum (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



/* GtkWidget */
static void
xfce_panel_window_realize (GtkWidget *widget)
{
    gtk_window_stick (GTK_WINDOW (widget));

    GTK_WIDGET_CLASS (parent_class)->realize (widget);
}



static void
xfce_panel_window_unrealize (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}



static void
xfce_panel_window_map (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (parent_class)->map (widget);
}



static void
xfce_panel_window_unmap (GtkWidget *widget)
{
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}



static void
xfce_panel_window_show (GtkWidget *widget)
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
xfce_panel_window_hide (GtkWidget *widget)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (widget);

    GTK_WIDGET_CLASS (parent_class)->hide (widget);

    priv->shown = FALSE;
}



/* drawing, size and style */
static void
_panel_window_paint_border (XfcePanelWindow *panel)
{
    XfcePanelWindowPrivate *priv       = XFCE_PANEL_WINDOW_GET_PRIVATE (panel);
    GdkWindow              *window     = GTK_WIDGET (panel)->window;
    GtkAllocation          *a          = &(GTK_WIDGET (panel)->allocation);
    GtkStyle               *style      = GTK_WIDGET (panel)->style;
    GtkStateType            state_type = GTK_WIDGET_STATE (GTK_WIDGET (panel));
    gint                    top, bottom, left, right;
    gint                    x1, x2, y1, y2;

    top    = priv->top_border    ? style->ythickness : 0;
    bottom = priv->bottom_border ? style->ythickness : 0;
    left   = priv->left_border   ? style->xthickness : 0;
    right  = priv->right_border  ? style->xthickness : 0;

    /* Code based on gtk-xfce-engine-2 */

    /* Attempt to explain the code below with some ASCII 'art'
     *
     * joining lines: - = horizontal, + = vertical
     *
     * single       double
     * ----------+  ----------+
     * +         +  +--------++
     * +         +  ++       ++
     * +         +  ++       ++
     * +         +  ++--------+
     * +----------  +----------
     *
     */

    if (top > 0)
    {
        x1 = a->x;
        y1 = a->y;
        x2 = x1 + a->width - 1;

        if (right > 0)
            x2--;

        if (top > 1)
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x2, y1);

            if (left > 0)
                x1++;

            if (right > 1)
                x2--;

            y1++;

            gdk_draw_line (window, style->light_gc[state_type], x1, y1, x2, y1);
        }
        else
        {
            gdk_draw_line (window, style->light_gc[state_type], x1, y1, x2, y1);
        }
    }

    if (bottom > 0)
    {
            x1 = a->x;
        y1 = a->y + a->height - 1;
        x2 = x1 + a->width - 1;

        if (left > 0)
            x1++;

        if (bottom > 1)
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x2, y1);

            if (left > 1)
                x1++;

            if (right > 0)
                x2--;

            y1--;

            gdk_draw_line (window, style->mid_gc[state_type], x1, y1, x2, y1);
        }
        else
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x2, y1);
        }
    }

    if (left > 0)
    {
        x1 = a->x;
        y1 = a->y;
        y2 = a->y + a->height - 1;

        if (top > 0)
            y1++;

        if (left > 1)
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x1, y2);

            if (top > 1)
                y1++;

            if (bottom > 0)
                y2--;

            x1++;

            gdk_draw_line (window, style->light_gc[state_type], x1, y1, x1, y2);
        }
        else
        {
            gdk_draw_line (window, style->light_gc[state_type], x1, y1, x1, y2);
        }
    }

    if (right > 0)
    {
        x1 = a->x + a->width - 1;
        y1 = a->y;
        y2 = a->y + a->height - 1;

        if (bottom > 0)
            y2--;

        if (right > 1)
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x1, y2);

            if (top > 0)
                y1++;

            if (bottom > 1)
                y2--;

            x1--;

            gdk_draw_line (window, style->mid_gc[state_type], x1, y1, x1, y2);
        }
        else
        {
            gdk_draw_line (window, style->dark_gc[state_type], x1, y1, x1, y2);
        }
    }
}



static gint
xfce_panel_window_expose (GtkWidget      *widget,
                          GdkEventExpose *event)
{
    XfcePanelWindow        *panel_window = XFCE_PANEL_WINDOW (widget);
    XfcePanelWindowPrivate *priv         = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

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
xfce_panel_window_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
    XfcePanelWindow        *panel_window = XFCE_PANEL_WINDOW (widget);
    XfcePanelWindowPrivate *priv         = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);
    gint                    handle_size, thick;

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
        requisition->width += widget->style->xthickness;
    if (priv->right_border)
        requisition->width += widget->style->xthickness;

    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        thick = 2 * widget->style->xthickness;
    else
        thick = 2 * widget->style->ythickness;

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
xfce_panel_window_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
    XfcePanelWindowPrivate *priv = XFCE_PANEL_WINDOW_GET_PRIVATE (widget);
    GtkAllocation           old, new, childalloc;
    gint                    start_handle_size, end_handle_size, thick;

    widget->allocation = *allocation;

    if (GTK_WIDGET_REALIZED (widget))
    {
        gdk_window_resize (widget->window,
                           allocation->width, allocation->height);

        if (priv->shown)
        {
            gdk_window_get_root_origin (widget->window,
                                        &(priv->x_root), &(priv->y_root));

            if (priv->resize_func && priv->shown)
            {
                old = priv->allocation;
                new = *allocation;

                priv->resize_func (widget, priv->resize_data,
                                   &old, &new,
                                   &(priv->x_root), &(priv->y_root));

                gtk_widget_queue_draw (widget);
            }

            gdk_window_move (widget->window, priv->x_root, priv->y_root);
        }
    }

    if (GTK_BIN (widget)->child)
    {
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

    priv->allocation = *allocation;
}



/* keyboard and mouse events */
static gboolean
xfce_panel_window_button_press (GtkWidget      *widget,
                                GdkEventButton *event)
{
    XfcePanelWindow        *panel_window;
    XfcePanelWindowPrivate *priv;
    GdkCursor              *fleur;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (priv->movable &&
        event->button == 1 && (event->window == widget->window))
    {
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

        g_signal_emit (G_OBJECT (widget), panel_window_signals[MOVE_START], 0);
        return TRUE;
    }

    return FALSE;
}



static gboolean
xfce_panel_window_button_release (GtkWidget      *widget,
                                  GdkEventButton *event)
{
    XfcePanelWindow        *panel_window;
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (!priv->in_move)
        return FALSE;

    gdk_pointer_ungrab (event->time);
    priv->in_move = FALSE;
    gdk_window_get_origin (widget->window, &(priv->x_root), &(priv->y_root));

    g_signal_emit (G_OBJECT (widget), panel_window_signals[MOVE_END], 0,
                   priv->x_root, priv->y_root);

    return TRUE;
}



static gboolean
xfce_panel_window_motion_notify (GtkWidget      *widget,
                                 GdkEventMotion *event)
{
    XfcePanelWindow        *panel_window;
    XfcePanelWindowPrivate *priv;
    gint                    new_x, new_y;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (widget), FALSE);

    panel_window = XFCE_PANEL_WINDOW (widget);
    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);

    if (!priv->in_move)
        return FALSE;

    gdk_window_get_pointer (NULL, &new_x, &new_y, NULL);
    new_x += priv->x_offset;
    new_y += priv->y_offset;

    if (priv->move_func)
        priv->move_func (widget, priv->move_data, &new_x, &new_y);

    priv->x_root = new_x;
    priv->y_root = new_y;

    gdk_window_move (widget->window, new_x, new_y);

    g_signal_emit (G_OBJECT (widget), panel_window_signals[MOVE], 0, new_x, new_y);

    return TRUE;
}



/* ginternal functions */
static void
_paint_handle (XfcePanelWindow *panel_window,
               gboolean         start,
               GdkRectangle    *area)
{
    XfcePanelWindowPrivate *priv   = XFCE_PANEL_WINDOW_GET_PRIVATE (panel_window);
    GtkWidget              *widget = GTK_WIDGET (panel_window);
    GtkAllocation          *alloc  = &(widget->allocation);
    gint                    x, y, w, h, xthick, ythick;
    gboolean                horizontal = priv->orientation == GTK_ORIENTATION_HORIZONTAL;

    xthick = widget->style->xthickness;
    ythick = widget->style->ythickness;

    if (horizontal)
    {
        w = HANDLE_WIDTH;
        h = alloc->height - 2 * ythick;

        y = alloc->y + ythick;

        if (priv->top_border)
        {
            h -= ythick;
            y += ythick;
        }
        if (priv->bottom_border)
            h -= ythick;

        if (start)
        {
            x = alloc->x + xthick;

            if (priv->left_border)
                x += xthick;
        }
        else
        {
            x = alloc->x + alloc->width - HANDLE_WIDTH - xthick;

            if (priv->right_border)
                x -= xthick;
        }
    }
    else
    {
        w = alloc->width - 2 * xthick;
        h = HANDLE_WIDTH;

        x = alloc->x + xthick;

        if (priv->left_border)
        {
            w -= xthick;
            x += xthick;
        }
        if (priv->right_border)
            w -= xthick;

        if (start)
        {
            y = alloc->y + ythick;

            if (priv->top_border)
                y += ythick;
        }
        else
        {
            y = alloc->y + alloc->height - HANDLE_WIDTH - ythick;

            if (priv->bottom_border)
                y -= ythick;
        }
    }

    gtk_paint_handle (widget->style, widget->window,
                      GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                      area, widget, "handlebox",
                      x, y, w, h,
                      horizontal ? GTK_ORIENTATION_VERTICAL :
                                   GTK_ORIENTATION_HORIZONTAL);
}



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
    return GTK_WIDGET (g_object_new (XFCE_TYPE_PANEL_WINDOW, NULL));
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
xfce_panel_window_get_orientation (XfcePanelWindow *window)
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
xfce_panel_window_set_orientation (XfcePanelWindow *window,
                                   GtkOrientation   orientation)
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
xfce_panel_window_get_handle_style (XfcePanelWindow *window)
{
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (window), DEFAULT_HANDLE_STYLE);

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
xfce_panel_window_set_handle_style (XfcePanelWindow *window,
                                    XfceHandleStyle  handle_style)
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
xfce_panel_window_get_show_border (XfcePanelWindow *window,
                                   gboolean        *top_border,
                                   gboolean        *bottom_border,
                                   gboolean        *left_border,
                                   gboolean        *right_border)
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
 * xfce_panel_window_set_show_border
 * @window        : #XfcePanelWindow
 * @top_border   : show top border
 * @bottom_border: show bottom border
 * @left_border  : show left border
 * @right_border : show right border
 *
 * Set border visibility for the panel window.
 **/
void
xfce_panel_window_set_show_border (XfcePanelWindow *window,
                                   gboolean         top_border,
                                   gboolean         bottom_border,
                                   gboolean         left_border,
                                   gboolean         right_border)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    if (top_border != priv->top_border
        || bottom_border != priv->bottom_border
        || left_border != priv->left_border
        || right_border != priv->right_border)
    {
        priv->top_border = top_border;
        priv->bottom_border = bottom_border;
        priv->left_border = left_border;
        priv->right_border = right_border;

        gtk_widget_queue_resize (GTK_WIDGET (window));
    }
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
xfce_panel_window_set_move_function (XfcePanelWindow         *window,
                                     XfcePanelWindowMoveFunc  function,
                                     gpointer                 data)
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
xfce_panel_window_set_resize_function (XfcePanelWindow           *window,
                                       XfcePanelWindowResizeFunc  function,
                                       gpointer                   data)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->resize_func = function;
    priv->resize_data = data;
}



/**
 * xfce_panel_window_get_movable
 * @window  : #XfcePanelWindow
 *
 * Check if the panel window can be moved by the user.
 * By default this is %TRUE.
 *
 * Returns: %TRUE if the user is allowed to move @window, %FALSE if not.
 **/
gboolean
xfce_panel_window_get_movable (XfcePanelWindow *window)
{
    XfcePanelWindowPrivate *priv;

    g_return_val_if_fail (XFCE_IS_PANEL_WINDOW (window), TRUE);

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    return priv->movable;
}



/**
 * xfce_panel_window_set_movable
 * @window  : #XfcePanelWindow
 * @movable: %TRUE or %FALSE.
 *
 * Set if the panel window can be moved by the user.
 **/
void
xfce_panel_window_set_movable (XfcePanelWindow *window,
                               gboolean         movable)
{
    XfcePanelWindowPrivate *priv;

    g_return_if_fail (XFCE_IS_PANEL_WINDOW (window));

    priv = XFCE_PANEL_WINDOW_GET_PRIVATE (window);

    priv->movable = movable;
}
