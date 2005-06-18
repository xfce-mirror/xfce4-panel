/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2004-2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-marshal.h"
#include "xfce-enum-types.h"
#include "xfce-paint-private.h"
#include "xfce-item.h"
#include "xfce-itembar.h"

#define XFCE_ITEM_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_ITEM, XfceItemPrivate))

/* keep in sync with itembar */
#define DEFAULT_ORIENTATION     GTK_ORIENTATION_HORIZONTAL
#define DEFAULT_HAS_HANDLE      FALSE
#define DEFAULT_USE_DRAG_WINDOW FALSE

#define HANDLE_WIDTH            XFCE_DEFAULT_HANDLE_WIDTH

enum
{
    ORIENTATION_CHANGED,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_ORIENTATION,
    PROP_HAS_HANDLE,
    PROP_USE_DRAG_WINDOW
};


typedef struct _XfceItemPrivate XfceItemPrivate;

struct _XfceItemPrivate 
{
    GtkOrientation orientation;
    guint has_handle:1;
    guint use_drag_window:1;

    GdkWindow *drag_window;
};


/* init */
static void xfce_item_class_init    (XfceItemClass * klass);

static void xfce_item_init          (XfceItem * item);

/* GObject */
static void xfce_item_finalize      (GObject * object);

static void xfce_item_set_property  (GObject * object,
				     guint prop_id,
				     const GValue * value,
				     GParamSpec * pspec);

static void xfce_item_get_property  (GObject * object,
				     guint prop_id,
				     GValue * value, 
                                     GParamSpec * pspec);

/* widget */
static int xfce_item_expose         (GtkWidget * widget,
                                     GdkEventExpose * event);

static void xfce_item_size_request  (GtkWidget * widget,
                        	     GtkRequisition * requisition);

static void xfce_item_size_allocate (GtkWidget * widget,
                        	     GtkAllocation * allocation);

static void xfce_item_realize       (GtkWidget * widget);

static void xfce_item_unrealize     (GtkWidget * widget);

static void xfce_item_map           (GtkWidget * widget);

static void xfce_item_unmap         (GtkWidget * widget);

static void xfce_item_parent_set    (GtkWidget * widget, 
                                     GtkWidget * old_parent);

/* internal functions */
static void _create_drag_window     (XfceItem * item);

static void _destroy_drag_window    (XfceItem * item);

    
/* global vars */
static GtkBinClass *parent_class = NULL;

static guint item_signals[LAST_SIGNAL] = { 0 };


GType
xfce_item_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GTypeInfo type_info = {
	    sizeof (XfceItemClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) xfce_item_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,
	    sizeof (XfceItem),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) xfce_item_init,
	};

	type = g_type_register_static (GTK_TYPE_BIN, "XfceItem", 
                                       &type_info, 0);
    }

    return type;
}


static void
xfce_item_class_init (XfceItemClass * klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GParamSpec *pspec;

    g_type_class_add_private (klass, sizeof (XfceItemPrivate));

    parent_class = g_type_class_peek_parent (klass);

    gobject_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;

    gobject_class->finalize     = xfce_item_finalize;
    gobject_class->get_property = xfce_item_get_property;
    gobject_class->set_property = xfce_item_set_property;

    widget_class->expose_event  = xfce_item_expose;    
    widget_class->size_request  = xfce_item_size_request;    
    widget_class->size_allocate = xfce_item_size_allocate;    
    widget_class->realize       = xfce_item_realize;    
    widget_class->unrealize     = xfce_item_unrealize;    
    widget_class->map           = xfce_item_map;    
    widget_class->unmap         = xfce_item_unmap;    
    widget_class->parent_set    = xfce_item_parent_set;    
    
    /* properties */

    /**
     * XfceItem::orientation
     *
     * The orientation of the #XfceItem.
     */
    pspec = g_param_spec_enum ("orientation",
			       "Orientation",
			       "The orientation of the item",
			       GTK_TYPE_ORIENTATION,
			       DEFAULT_ORIENTATION, 
                               G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_ORIENTATION, pspec);

    /**
     * XfceItem::has_handle
     *
     * Whether the #XfceItem has a handle attached to it.
     */
    pspec = g_param_spec_boolean ("has_handle",
			          "Has handle",
			          "Whether the item has a handle",
			          DEFAULT_HAS_HANDLE, 
                                  G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_HAS_HANDLE, pspec);
    
    /**
     * XfceItem::use_drag_window
     *
     * Whether the #XfceItem uses a special window to catch events.
     */
    pspec = g_param_spec_boolean ("use_drag_window",
			          "Use drag window",
			          "Whether the item uses a drag window",
			          DEFAULT_USE_DRAG_WINDOW, 
                                  G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_USE_DRAG_WINDOW, 
                                     pspec);
    
    
    /* signals */

    /**
    * XfceItem::orientation_changed:
    * @item: the object which emitted the signal
    * @orientation: the new #GtkOrientation of the item
    *
    * Emitted when the orientation of the item changes.
    */
    item_signals[ORIENTATION_CHANGED] =
        g_signal_new ("orientation-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfceItemClass, 
                                       orientation_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, GTK_TYPE_ORIENTATION);
}

static void
xfce_item_init (XfceItem * item)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (item);
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (item), GTK_NO_WINDOW);

    priv->orientation     = DEFAULT_ORIENTATION;
    priv->has_handle      = DEFAULT_HAS_HANDLE;
    priv->use_drag_window = DEFAULT_USE_DRAG_WINDOW;
    priv->drag_window     = NULL;
}

static void
xfce_item_finalize (GObject * object)
{
    _destroy_drag_window (XFCE_ITEM (object));

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfce_item_set_property (GObject * object, guint prop_id, const GValue * value,
                        GParamSpec * pspec)
{
    XfceItem *item = XFCE_ITEM (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            xfce_item_set_orientation (item, g_value_get_enum (value));
            break;
        case PROP_HAS_HANDLE:
            xfce_item_set_has_handle (item, g_value_get_boolean (value));
            break;
        case PROP_USE_DRAG_WINDOW:
            xfce_item_set_use_drag_window (item, g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
xfce_item_get_property (GObject * object, guint prop_id, GValue * value,
                        GParamSpec * pspec)
{
    XfceItem *item = XFCE_ITEM (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            g_value_set_enum (value, xfce_item_get_orientation (item));
            break;
        case PROP_HAS_HANDLE:
            g_value_set_boolean (value, xfce_item_get_has_handle (item));
            break;
        case PROP_USE_DRAG_WINDOW:
            g_value_set_boolean (value, xfce_item_get_use_drag_window (item));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static int
xfce_item_expose (GtkWidget * widget, GdkEventExpose *event)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));
        GtkAllocation *allocation = &(widget->allocation);

        if (priv->has_handle)
        {
            int x, y, w, h;
            GtkOrientation orientation;

            x = allocation->x + GTK_CONTAINER (widget)->border_width
                + widget->style->xthickness;
            y = allocation->y + GTK_CONTAINER (widget)->border_width
                + widget->style->ythickness;
            
            if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
            {
                orientation = GTK_ORIENTATION_VERTICAL;

                w = HANDLE_WIDTH;
                h = allocation->height 
                    - 2 * GTK_CONTAINER (widget)->border_width
                    - 2 * widget->style->ythickness;
            }
            else
            {
                orientation = GTK_ORIENTATION_HORIZONTAL;

                w = allocation->width 
                    - 2 * GTK_CONTAINER (widget)->border_width
                    - 2 * widget->style->xthickness;
                h = HANDLE_WIDTH;
            }

            xfce_paint_handle (widget, &(event->area), "handlebox",
                               orientation, x, y, w, h);
        }
        
        if (GTK_BIN (widget)->child)
        {
            gtk_container_propagate_expose (GTK_CONTAINER (widget), 
                                            GTK_BIN (widget)->child, event);
        }
    }

    return TRUE;
}

static void
xfce_item_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));
    GtkRequisition req = { 0 };

    if (GTK_BIN (widget)->child)
        gtk_widget_size_request (GTK_BIN (widget)->child, &req);

    requisition->width = req.width 
                         + 2 * GTK_CONTAINER (widget)->border_width;
    requisition->height = req.height 
                          + 2 * GTK_CONTAINER (widget)->border_width;
    
    if (priv->has_handle)
    {
        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            requisition->width += HANDLE_WIDTH
                                  + 2 * widget->style->xthickness;
        }
        else
        {
            requisition->height += HANDLE_WIDTH 
                                   + 2 * widget->style->ythickness;
        }
    }
}

static void
xfce_item_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkAllocation childalloc;
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));
    
    widget->allocation = childalloc = *allocation;
    
    childalloc.x += GTK_CONTAINER (widget)->border_width;
    childalloc.y += GTK_CONTAINER (widget)->border_width;
    childalloc.width -= 2 * GTK_CONTAINER (widget)->border_width;
    childalloc.height -= 2 * GTK_CONTAINER (widget)->border_width;
    
    if (priv->use_drag_window && priv->drag_window != NULL)
    {
        gdk_window_move_resize (priv->drag_window, 
                                childalloc.x, childalloc.y,
                                childalloc.width, childalloc.height);
    }
    
    if (priv->has_handle)
    {
        if (!priv->use_drag_window && priv->drag_window != NULL)
        {
            if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
            {
                gdk_window_move_resize (priv->drag_window, 
                                        childalloc.x, childalloc.y,
                                        HANDLE_WIDTH  
                                        + widget->style->xthickness,
                                        childalloc.height);
            }
            else
            {
                gdk_window_move_resize (priv->drag_window, 
                                        childalloc.x, childalloc.y,
                                        childalloc.width,
                                        HANDLE_WIDTH  
                                        + widget->style->ythickness);
            }
        }
        
        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            childalloc.x += HANDLE_WIDTH + 2 * widget->style->xthickness;
            childalloc.width -= HANDLE_WIDTH 
                                + 2 * widget->style->xthickness;
        }
        else
        {
            childalloc.y += HANDLE_WIDTH + 2 * widget->style->ythickness;
            childalloc.height -= HANDLE_WIDTH 
                                 + 2 * widget->style->ythickness;
        }
    }

    if (GTK_BIN (widget)->child)
        gtk_widget_size_allocate (GTK_BIN (widget)->child, &childalloc);
}

static void 
xfce_item_realize (GtkWidget * widget)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    widget->window = gtk_widget_get_parent_window (widget);
    g_object_ref (widget->window);

    if (priv->use_drag_window || priv->has_handle)
        _create_drag_window (XFCE_ITEM (widget));

    widget->style = gtk_style_attach (widget->style, widget->window);
}

static void 
xfce_item_unrealize (GtkWidget * widget)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));

    if (priv->use_drag_window)
        _destroy_drag_window (XFCE_ITEM (widget));

    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void 
xfce_item_map (GtkWidget * widget)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));

    if (priv->has_handle && !priv->use_drag_window)
        gdk_window_show (priv->drag_window);
    
    GTK_WIDGET_CLASS (parent_class)->map (widget);

    if (priv->use_drag_window)
        gdk_window_show (priv->drag_window);
}

static void 
xfce_item_unmap (GtkWidget * widget)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (XFCE_ITEM (widget));

    if (priv->drag_window)
        gdk_window_hide (priv->drag_window);

    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void 
xfce_item_parent_set (GtkWidget * widget, GtkWidget *old_parent)
{
    XfceItembar *bar = XFCE_ITEMBAR (widget->parent);

    if (!bar || !XFCE_IS_ITEMBAR (bar))
        return;

    g_object_freeze_notify (G_OBJECT (widget));
    g_object_set (G_OBJECT (widget),
                  "orientation", xfce_itembar_get_orientation (bar),
                  NULL);
    g_object_thaw_notify (G_OBJECT (widget));
}

/* internal functions */

static void 
_create_drag_window (XfceItem * item)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (item);
    GtkWidget *widget;
    GdkWindowAttr attributes;
    gint attributes_mask, border_width;

    widget = GTK_WIDGET (item);
    border_width = GTK_CONTAINER (item)->border_width;

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x + border_width;
    attributes.y = widget->allocation.y + border_width;

    if (priv->use_drag_window)
    {
	attributes.width = widget->allocation.width - border_width * 2;
	attributes.height = widget->allocation.height - border_width * 2;
    }
    else /* handle */
    {
	if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
	{
	    attributes.width = HANDLE_WIDTH;
	    attributes.height = widget->allocation.height - border_width * 2;
	}
	else
	{
	    attributes.width = widget->allocation.width - border_width * 2;
	    attributes.height = HANDLE_WIDTH;
	}
    }

    attributes.wclass = GDK_INPUT_ONLY;
    attributes.event_mask = gtk_widget_get_events (widget);
    attributes.event_mask |=
	(GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    attributes_mask = GDK_WA_X | GDK_WA_Y;

    priv->drag_window =
	gdk_window_new (gtk_widget_get_parent_window (widget), &attributes,
			attributes_mask);

    gdk_window_set_user_data (priv->drag_window, item);
}

static void 
_destroy_drag_window (XfceItem * item)
{
    XfceItemPrivate *priv = XFCE_ITEM_GET_PRIVATE (item);
    
    if (priv->drag_window)
    {
	gdk_window_set_user_data (priv->drag_window, NULL);
	gdk_window_destroy (priv->drag_window);
	priv->drag_window = NULL;
    }
}


/* public interface */

GtkWidget *
xfce_item_new (void)
{
    XfceItem *item;

    item = g_object_new (XFCE_TYPE_ITEM, NULL);

    return GTK_WIDGET (item);
}

GtkOrientation 
xfce_item_get_orientation (XfceItem * item)
{
    XfceItemPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEM (item), DEFAULT_ORIENTATION);

    priv = XFCE_ITEM_GET_PRIVATE (item);

    return priv->orientation;
}

void 
xfce_item_set_orientation (XfceItem * item, GtkOrientation orientation)
{
    XfceItemPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEM (item));

    priv = XFCE_ITEM_GET_PRIVATE (item);

    if (orientation == priv->orientation)
        return;

    priv->orientation = orientation;

    g_signal_emit (item, item_signals[ORIENTATION_CHANGED], 0, orientation);

    g_object_notify (G_OBJECT (item), "orientation");

    gtk_widget_queue_resize (GTK_WIDGET (item));
}


gboolean 
xfce_item_get_has_handle (XfceItem * item)
{
    XfceItemPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEM (item), DEFAULT_HAS_HANDLE);

    priv = XFCE_ITEM_GET_PRIVATE (item);

    return priv->has_handle;
}

void 
xfce_item_set_has_handle (XfceItem * item, gboolean has_handle)
{
    XfceItemPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEM (item));

    priv = XFCE_ITEM_GET_PRIVATE (item);

    if (has_handle == priv->has_handle)
        return;

    priv->has_handle = has_handle;

    if (has_handle)
    {
        if (!priv->drag_window && GTK_WIDGET_REALIZED (GTK_WIDGET (item)))
        {
            _create_drag_window (item);

            if (GTK_WIDGET_VISIBLE (GTK_WIDGET (item)))
                gdk_window_show (priv->drag_window);
        }
    }
    else if (!priv->use_drag_window)
    {
        _destroy_drag_window (item);
    }
    
    g_object_notify (G_OBJECT (item), "has_handle");

    gtk_widget_queue_resize (GTK_WIDGET (item));
}


gboolean 
xfce_item_get_use_drag_window (XfceItem * item)
{
    XfceItemPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEM (item), DEFAULT_USE_DRAG_WINDOW);

    priv = XFCE_ITEM_GET_PRIVATE (item);

    return priv->use_drag_window;
}

void 
xfce_item_set_use_drag_window (XfceItem * item, gboolean use_drag_window)
{
    XfceItemPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEM (item));

    priv = XFCE_ITEM_GET_PRIVATE (item);

    if (use_drag_window == priv->use_drag_window)
        return;

    priv->use_drag_window = use_drag_window;

    if (use_drag_window)
    {
        if (!priv->drag_window && GTK_WIDGET_REALIZED (GTK_WIDGET (item)))
        {
            _create_drag_window (item);

            if (GTK_WIDGET_VISIBLE (GTK_WIDGET (item)))
                gdk_window_show (priv->drag_window);
        }
    }
    else if (!priv->has_handle)
    {
        _destroy_drag_window (item);
    }
    
    gtk_widget_queue_resize (GTK_WIDGET (item));

    g_object_notify (G_OBJECT (item), "use_drag_window");
}


