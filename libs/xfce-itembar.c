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

/*  Partly based on code from GTK+. Copyright © 1997-2000 The GTK+ Team
 *  licensed under the LGPL.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-marshal.h"
#include "xfce-enum-types.h"
#include "xfce-paint-private.h"
#include "xfce-itembar.h"
#include "xfce-item.h"

#define XFCE_ITEMBAR_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_ITEMBAR, XfceItembarPrivate))

#define DEFAULT_ORIENTATION     GTK_ORIENTATION_HORIZONTAL
#define MIN_ICON_SIZE           12
#define MAX_ICON_SIZE           256
#define DEFAULT_ICON_SIZE       32
#define DEFAULT_TOOLBAR_STYLE   GTK_TOOLBAR_ICONS

enum
{
    ORIENTATION_CHANGED,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_ORIENTATION,
};

enum
{
    CHILD_PROP_0,
    CHILD_PROP_EXPAND
};

typedef struct _XfceItembarChild XfceItembarChild;

typedef struct _XfceItembarPrivate XfceItembarPrivate;

struct _XfceItembarPrivate 
{
    GtkOrientation orientation;

    GList *children;

    GdkWindow *event_window;
    GdkWindow *drag_highlight;
    int drop_index;
    gboolean raised;
};

struct _XfceItembarChild
{
    GtkWidget *widget;
    guint expand:1;
};

/* init */
static void xfce_itembar_class_init         (XfceItembarClass * klass);

static void xfce_itembar_init               (XfceItembar * itembar);

/* GObject */
static void xfce_itembar_finalize           (GObject *object);

static void xfce_itembar_set_property       (GObject * object,
				             guint prop_id,
				             const GValue * value,
				             GParamSpec * pspec);

static void xfce_itembar_get_property       (GObject * object,
				             guint prop_id,
				             GValue * value, 
                                             GParamSpec * pspec);

/* widget */
static int xfce_itembar_expose              (GtkWidget * widget,
                                             GdkEventExpose * event);

static void xfce_itembar_size_request       (GtkWidget * widget,
                        	             GtkRequisition * requisition);

static void xfce_itembar_size_allocate      (GtkWidget * widget,
                        	             GtkAllocation * allocation);

static void xfce_itembar_realize            (GtkWidget * widget);

static void xfce_itembar_unrealize          (GtkWidget * widget);

static void xfce_itembar_map                (GtkWidget * widget);

static void xfce_itembar_unmap              (GtkWidget * widget);

static void xfce_itembar_drag_leave         (GtkWidget * widget,
				             GdkDragContext * context, 
                                             guint time_);

static gboolean xfce_itembar_drag_motion    (GtkWidget * widget,
					     GdkDragContext * context,
					     int x, 
                                             int y, 
                                             guint time_);


/* container */
static void xfce_itembar_forall             (GtkContainer * container,
		                             gboolean include_internals,
		                             GtkCallback callback, 
                                             gpointer callback_data);

static GType xfce_itembar_child_type        (GtkContainer * container);

static void xfce_itembar_add                (GtkContainer * container,
                                             GtkWidget * child);

static void xfce_itembar_remove             (GtkContainer * container,
                                             GtkWidget * child);
    
static void xfce_itembar_set_child_property (GtkContainer * container,
                                             GtkWidget * child,
				             guint prop_id,
				             const GValue * value,
				             GParamSpec * pspec);

static void xfce_itembar_get_child_property (GtkContainer * container,
                                             GtkWidget * child,
				             guint prop_id,
				             GValue * value, 
                                             GParamSpec * pspec);

/* internal functions */
static int _find_drop_index                 (XfceItembar * itembar, 
                                             int x, 
                                             int y);

/* global vars */
static GtkContainerClass *parent_class = NULL;

static guint itembar_signals[LAST_SIGNAL] = { 0 };


/* public GType function */
GType
xfce_itembar_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GTypeInfo type_info = {
	    sizeof (XfceItembarClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) xfce_itembar_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,
	    sizeof (XfceItembar),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) xfce_itembar_init,
	};

	type = g_type_register_static (GTK_TYPE_CONTAINER, "XfceItembar", 
                                       &type_info, 0);
    }

    return type;
}


/* init */
static void
xfce_itembar_class_init (XfceItembarClass * klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GParamSpec *pspec;

    g_type_class_add_private (klass, sizeof (XfceItembarPrivate));

    parent_class = g_type_class_peek_parent (klass);

    gobject_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gobject_class->finalize             = xfce_itembar_finalize;
    gobject_class->get_property         = xfce_itembar_get_property;
    gobject_class->set_property         = xfce_itembar_set_property;

    widget_class->expose_event          = xfce_itembar_expose;    
    widget_class->size_request          = xfce_itembar_size_request;    
    widget_class->size_allocate         = xfce_itembar_size_allocate;    
    widget_class->realize               = xfce_itembar_realize;
    widget_class->unrealize             = xfce_itembar_unrealize;
    widget_class->map                   = xfce_itembar_map;
    widget_class->unmap                 = xfce_itembar_unmap;
    widget_class->drag_leave            = xfce_itembar_drag_leave;
    widget_class->drag_motion           = xfce_itembar_drag_motion;

    container_class->forall             = xfce_itembar_forall;
    container_class->child_type         = xfce_itembar_child_type;
    container_class->add                = xfce_itembar_add;
    container_class->remove             = xfce_itembar_remove;
    container_class->get_child_property = xfce_itembar_get_child_property;
    container_class->set_child_property = xfce_itembar_set_child_property;

    
    /* properties */

    /**
     * XfceItembar::orientation
     *
     * The orientation of the #XfceItembar.
     */
    pspec = g_param_spec_enum ("orientation",
			       "Orientation",
			       "The orientation of the itembar",
			       GTK_TYPE_ORIENTATION,
			       DEFAULT_ORIENTATION, 
                               G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_ORIENTATION, pspec);

    /* child properties */
    pspec = g_param_spec_boolean ("expand", 
                                  "Expand", 
                                  "Whether to grow with parent",
                                  FALSE,
                                  G_PARAM_READWRITE);
    
    gtk_container_class_install_child_property (container_class,
                                                CHILD_PROP_EXPAND, pspec);

    /* signals */

    /**
    * XfceItembar::orientation_changed:
    * @itembar: the object which emitted the signal
    * @orientation: the new #GtkOrientation of the itembar
    *
    * Emitted when the orientation of the itembar changes.
    */
    itembar_signals[ORIENTATION_CHANGED] =
        g_signal_new ("orientation-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfceItembarClass, 
                                       orientation_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, GTK_TYPE_ORIENTATION);
}

static void
xfce_itembar_init (XfceItembar * itembar)
{
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (itembar), GTK_NO_WINDOW);

    priv->orientation      = DEFAULT_ORIENTATION;
    priv->children         = NULL;
    priv->event_window     = NULL;
    priv->drag_highlight   = NULL;
    priv->drop_index       = -1;
}

/* GObject */
static void
xfce_itembar_finalize (GObject * object)
{
    GList *l;
    
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (object);

    for (l = priv->children; l != NULL; l = l->next)
        g_free (l->data);
    
    g_list_free (priv->children);
    priv->children = NULL;

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfce_itembar_set_property (GObject * object, guint prop_id,
                           const GValue * value, GParamSpec * pspec)
{
    XfceItembar *itembar = XFCE_ITEMBAR (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            xfce_itembar_set_orientation (itembar, g_value_get_enum (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
xfce_itembar_get_property (GObject * object, guint prop_id, GValue * value,
                           GParamSpec * pspec)
{
    XfceItembar *itembar = XFCE_ITEMBAR (object);

    switch (prop_id)
    {
        case PROP_ORIENTATION:
            g_value_set_enum (value, xfce_itembar_get_orientation (itembar));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}


/* GtkWidget */
static int
xfce_itembar_expose (GtkWidget * widget, GdkEventExpose *event)
{
    return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
}

static void
xfce_itembar_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    XfceItembarPrivate *priv = 
        XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (widget));
    GList *l;
    int max, other_size;

    requisition->width = requisition->height = 
        2 * GTK_CONTAINER (widget)->border_width;
    
    max = other_size = 0;
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;
        GtkRequisition req;

        if (!GTK_WIDGET_VISIBLE (child->widget))
            continue;

        gtk_widget_size_request (child->widget, &req);
        
        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            max = MAX (max, req.height);

            other_size += req.width;
        }
        else
        {
            max = MAX (max, req.width);

            other_size += req.height;
        }
    }

    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        requisition->height += max;
        requisition->width += other_size;
    }
    else
    {
        requisition->width += max;
        requisition->height += other_size;
    }
}

static void
xfce_itembar_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    XfceItembarPrivate *priv = 
        XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (widget));
    int n, i, x, y, bar_height, total_size;
    int n_expand, expand_width, max_expand;
    int border_width;
    GList *l;
    GtkTextDirection direction;
    gboolean use_expand = TRUE;
    struct ItemProps { 
        GtkAllocation allocation;
        gboolean expand; 
    } *props;
    
    widget->allocation = *allocation;

    border_width = GTK_CONTAINER (widget)->border_width;
    
    n_expand = expand_width = max_expand = 0;
    bar_height = total_size = 0;

    x = allocation->x + border_width;
    y = allocation->y + border_width;
    
    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        expand_width = allocation->width - 2 * border_width;
        bar_height = allocation->height - 2 * border_width;

        if (priv->event_window != NULL)
        {
            gdk_window_move_resize (priv->event_window, 
                                    x, y, expand_width, bar_height);
        }
    }
    else
    {
        expand_width = allocation->height - 2 * border_width;
        bar_height = allocation->width - 2 * border_width;

        if (priv->event_window != NULL)
        {
            gdk_window_move_resize (priv->event_window, 
                                    x, y, bar_height, expand_width);
        }
    }

    total_size = expand_width;
    direction = gtk_widget_get_direction (widget);

    n = g_list_length (priv->children);

    props = g_new0 (struct ItemProps, n);
    
    /* first pass */
    for (l = priv->children, i = 0; l != NULL; l = l->next, i++)
    {
        XfceItembarChild *child = l->data;
        int width, height;
        GtkRequisition req;

        if (!GTK_WIDGET_VISIBLE (child->widget))
        {
            props[i].allocation.width = 0;
            continue;
        }
        
        gtk_widget_size_request (child->widget, &req);
        width = req.width;
        height = req.height;

        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            height = bar_height;
            
            if (child->expand)
            {
                n_expand++;
                max_expand = MAX (max_expand, width);
            }
            else
            {
                expand_width -= width;
            }
        }
        else
        {
            width = bar_height;
            
            if (child->expand)
            {
                n_expand++;
                max_expand = MAX (max_expand, height);
            }
            else
            {
                expand_width -= height;
            }
        }

        props[i].allocation.width = width;
        props[i].allocation.height = height;
        props[i].expand = child->expand;
    }

    if (n_expand > 0)
    {
        expand_width = MAX (expand_width / n_expand, 0);

        if (expand_width < max_expand)
            use_expand = FALSE;
    }
    
    x = y = border_width;
    
    /* second pass */
    for (l = priv->children, i = 0; l != NULL; l = l->next, i++)
    {
        XfceItembarChild *child = l->data;
        
        if (props[i].allocation.width == 0)
            continue;

        props[i].allocation.x = x + allocation->x;
        props[i].allocation.y = y + allocation->y;

        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            if (props[i].expand && use_expand)
                props[i].allocation.width = expand_width;

            if (direction == GTK_TEXT_DIR_RTL)
            {
                props[i].allocation.x = allocation->x + total_size - x 
                                        - props[i].allocation.width;
            }

            x += props[i].allocation.width;
        }
        else
        {
            if (props[i].expand && use_expand)
                props[i].allocation.height = expand_width;

            y += props[i].allocation.height;
        }
        
        gtk_widget_size_allocate (child->widget, 
                                  &(props[i].allocation));
    }

    g_free (props);
}

static void
xfce_itembar_realize (GtkWidget * widget)
{
    GdkWindowAttr attributes;
    int attributes_mask;
    int border_width;
    XfceItembarPrivate *priv;

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

    border_width = GTK_CONTAINER (widget)->border_width;

    attributes.x = widget->allocation.x + border_width;
    attributes.y = widget->allocation.y + border_width;
    attributes.width = widget->allocation.width - 2 * border_width;
    attributes.height = widget->allocation.height - 2 * border_width;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events (widget)
        | GDK_BUTTON_MOTION_MASK
        | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK
        | GDK_EXPOSURE_MASK 
        | GDK_ENTER_NOTIFY_MASK 
        | GDK_LEAVE_NOTIFY_MASK;
    attributes.wclass = GDK_INPUT_ONLY;
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    priv = XFCE_ITEMBAR_GET_PRIVATE (widget);

    widget->window = gtk_widget_get_parent_window (widget);
    g_object_ref (widget->window);

    priv->event_window = gdk_window_new (widget->window,
                                         &attributes, attributes_mask);

    gdk_window_set_user_data (priv->event_window, widget);

    widget->style = gtk_style_attach (widget->style, widget->window);
}

static void
xfce_itembar_unrealize (GtkWidget * widget)
{
    XfceItembarPrivate *priv;

    priv = XFCE_ITEMBAR_GET_PRIVATE (widget);

    if (priv->event_window != NULL)
    {
        gdk_window_set_user_data (priv->event_window, NULL);
        gdk_window_destroy (priv->event_window);
        priv->event_window = NULL;
    }

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (*GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
xfce_itembar_map (GtkWidget * widget)
{
    XfceItembarPrivate *priv;

    priv = XFCE_ITEMBAR_GET_PRIVATE (widget);

    gdk_window_show (priv->event_window);

    (*GTK_WIDGET_CLASS (parent_class)->map) (widget);
}

static void
xfce_itembar_unmap (GtkWidget * widget)
{
    XfceItembarPrivate *priv;

    priv = XFCE_ITEMBAR_GET_PRIVATE (widget);

    if (priv->event_window != NULL)
        gdk_window_hide (priv->event_window);

    (*GTK_WIDGET_CLASS (parent_class)->unmap) (widget);
}

static void
xfce_itembar_drag_leave (GtkWidget * widget,
			 GdkDragContext * context, guint time_)
{
    XfceItembar *toolbar = XFCE_ITEMBAR (widget);
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (toolbar);

    if (priv->drag_highlight)
    {
	gdk_window_set_user_data (priv->drag_highlight, NULL);
	gdk_window_destroy (priv->drag_highlight);
	priv->drag_highlight = NULL;
    }

    priv->drop_index = -1;
}

static gboolean
xfce_itembar_drag_motion (GtkWidget * widget,
			  GdkDragContext * context,
			  int x, int y, guint time_)
{
    XfceItembar *itembar = XFCE_ITEMBAR (widget);
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    XfceItembarChild *child;
    int new_index, new_pos;

    new_index = _find_drop_index (itembar, x, y);

    if (!priv->drag_highlight)
    {
	GdkWindowAttr attributes;
	guint attributes_mask;

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask =
	    GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK |
	    GDK_POINTER_MOTION_MASK;
	attributes.width = 1;
	attributes.height = 1;
	attributes_mask = GDK_WA_VISUAL | GDK_WA_COLORMAP;
	priv->drag_highlight = 
            gdk_window_new (gtk_widget_get_parent_window (widget),
		            &attributes, attributes_mask);
	gdk_window_set_user_data (priv->drag_highlight, widget);
	gdk_window_set_background (priv->drag_highlight,
				   &widget->style->fg[widget->state]);
    }

    if (priv->drop_index < 0 || priv->drop_index != new_index)
    {
	int border_width = GTK_CONTAINER (itembar)->border_width;

        child = g_list_nth_data (priv->children, new_index);
	
	priv->drop_index = new_index;
	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
	    if (child)
		new_pos = child->widget->allocation.x;
	    else
		new_pos = widget->allocation.x + widget->allocation.width
                          - border_width;
	    
	    gdk_window_move_resize (priv->drag_highlight, new_pos - 1,
				    widget->allocation.y + border_width, 2,
				    widget->allocation.height -
                                        border_width * 2);
	}
	else
	{
	    if (child)
		new_pos = child->widget->allocation.y;
	    else
		new_pos = widget->allocation.y + widget->allocation.height;
	    
	    gdk_window_move_resize (priv->drag_highlight,
				    widget->allocation.x + border_width,
				    new_pos - 1, widget->allocation.width
                                                 - border_width * 2, 2);
	}
    }

    gdk_window_show (priv->drag_highlight);

    gdk_drag_status (context, context->suggested_action, time_);

    return TRUE;
}

static void
xfce_itembar_forall (GtkContainer * container, gboolean include_internals,
                     GtkCallback callback, gpointer callback_data)
{
    GList *l;
    XfceItembarPrivate *priv = 
        XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (container));

    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;
        
        (*callback) (child->widget, callback_data);
    }
}

static GType
xfce_itembar_child_type (GtkContainer * container)
{
    return GTK_TYPE_WIDGET;
}

static void
xfce_itembar_add (GtkContainer * container, GtkWidget * child)
{
    xfce_itembar_insert (XFCE_ITEMBAR (container), child, -1);
}

static void
xfce_itembar_remove (GtkContainer * container, GtkWidget *child)
{
    XfceItembarPrivate *priv;
    GList *l;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (container));
    g_return_if_fail (child != NULL 
                      && child->parent == GTK_WIDGET (container));

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (container));
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *ic = l->data;

        if (ic->widget == child)
        {
            priv->children = g_list_remove_link (priv->children, l);

            gtk_widget_unparent (ic->widget);

            g_free (ic);
            g_list_free (l);

            break;
        }
    }

    gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
xfce_itembar_get_child_property (GtkContainer * container,
                                 GtkWidget * child, guint property_id,
                                 GValue * value, GParamSpec * pspec)
{
    gboolean expand = 0;

    switch (property_id)
    {
        case CHILD_PROP_EXPAND:
            expand = xfce_itembar_get_child_expand (XFCE_ITEMBAR (container),
                                                    child);
            g_value_set_boolean (value, expand);
            break;
        default:
            GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                          property_id, pspec);
            break;
    }
}

static void
xfce_itembar_set_child_property (GtkContainer * container,
                                 GtkWidget * child,
                                 guint property_id,
                                 const GValue * value, GParamSpec * pspec)
{
    switch (property_id)
    {
        case CHILD_PROP_EXPAND:
            xfce_itembar_set_child_expand (XFCE_ITEMBAR (container), child,
                                           g_value_get_boolean (value));
            break;
        default:
            GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container,
                                                          property_id, pspec);
            break;
    }
}

/* internal functions */
static int
_find_drop_index (XfceItembar * itembar, int x, int y)
{
    GList *l;
    GtkTextDirection direction;
    int distance, cursor, pos, i, index;
    XfceItembarChild *child;
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    int best_distance = G_MAXINT;
    
    direction = gtk_widget_get_direction (GTK_WIDGET (itembar));

    index = 0;
    
    child = priv->children->data;
    
    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        cursor =  x;
        x = GTK_WIDGET (itembar)->allocation.x;

        if (direction == GTK_TEXT_DIR_LTR)
        {
            pos = child->widget->allocation.x - x;
        }
        else
        {
            pos = child->widget->allocation.x 
                  + child->widget->allocation.width - x;
        }
    }
    else
    {
        cursor = y;
        y = GTK_WIDGET (itembar)->allocation.y;

        pos = child->widget->allocation.y - y;
    }
    
    best_distance = ABS (pos - cursor);

    /* distance to far end of each item */
    for (i = 1, l = priv->children; l != NULL; l = l->next, i++)
    {
	child = l->data;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
	    if (direction == GTK_TEXT_DIR_LTR)
		pos = child->widget->allocation.x 
                      + child->widget->allocation.width - x;
	    else
		pos = child->widget->allocation.x - x;
	}
	else
	{
	    pos = y + child->widget->allocation.y 
                  + child->widget->allocation.height - y;
	}

	distance = ABS (pos - cursor);

	if (distance <= best_distance)
	{
	    best_distance = distance;
            index = i;
	}
    }

    return index;
}


/* public interface */

GtkWidget *
xfce_itembar_new (GtkOrientation orientation)
{
    XfceItembar *itembar;

    itembar = g_object_new (XFCE_TYPE_ITEMBAR, "orientation", orientation, 
                            NULL);

    return GTK_WIDGET (itembar);
}

GtkOrientation 
xfce_itembar_get_orientation (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), DEFAULT_ORIENTATION);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    return priv->orientation;
}

void 
xfce_itembar_set_orientation (XfceItembar * itembar, GtkOrientation orientation)
{
    XfceItembarPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    if (orientation == priv->orientation)
        return;

    priv->orientation = orientation;

    g_signal_emit (itembar, itembar_signals[ORIENTATION_CHANGED], 0, 
                   orientation);

    g_object_notify (G_OBJECT (itembar), "orientation");

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}


void
xfce_itembar_insert (XfceItembar * itembar, GtkWidget * item, int position)
{
    XfceItembarPrivate *priv;
    XfceItembarChild *child;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));
    g_return_if_fail (item != NULL && GTK_WIDGET (item)->parent == NULL);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    
    child = g_new0 (XfceItembarChild, 1);
    child->widget = item;
    
    priv->children = g_list_insert (priv->children, child, position);

    gtk_widget_set_parent (GTK_WIDGET (item), GTK_WIDGET (itembar));

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}

void
xfce_itembar_append (XfceItembar * itembar, GtkWidget * item)
{
    xfce_itembar_insert (itembar, item, -1);
}

void
xfce_itembar_prepend (XfceItembar * itembar, GtkWidget * item)
{
    xfce_itembar_insert (itembar, item, 0);
}

void
xfce_itembar_reorder_child (XfceItembar * itembar, GtkWidget * item,
                            int position)
{
    XfceItembarPrivate *priv;
    GList *l;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));
    g_return_if_fail (item != NULL 
                      && GTK_WIDGET (item)->parent == GTK_WIDGET (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;

        if (item == child->widget)
        {
            priv->children = g_list_remove_link (priv->children, l);

            g_list_free (l);

            priv->children = g_list_insert (priv->children, child, position);

            break;
        }
    }

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}

gboolean 
xfce_itembar_get_child_expand (XfceItembar * itembar, GtkWidget * item)
{
    XfceItembarPrivate *priv;
    GList *l;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), FALSE);
    g_return_val_if_fail (item != NULL && 
                          GTK_WIDGET (item)->parent == GTK_WIDGET (itembar),
                          FALSE);

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;

        if (item == child->widget)
        {
            return child->expand;
        }
    }

    return FALSE;
}

void 
xfce_itembar_set_child_expand (XfceItembar * itembar, GtkWidget * item,
                               gboolean expand)
{
    XfceItembarPrivate *priv;
    GList *l;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));
    g_return_if_fail (item != NULL 
                      && GTK_WIDGET (item)->parent == GTK_WIDGET (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;

        if (item == child->widget)
        {
            child->expand = expand;
            break;
        }
    }

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}

int
xfce_itembar_get_n_items (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), 0);

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    return g_list_length (priv->children);
}

int
xfce_itembar_get_item_index (XfceItembar * itembar, GtkWidget * item)
{
    XfceItembarPrivate *priv;
    GList *l;
    int n;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), -1);
    g_return_val_if_fail (item != NULL 
                                && GTK_WIDGET (item)->parent 
                                        == GTK_WIDGET (itembar), -1);

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    for (n = 0, l = priv->children; l != NULL; l = l->next, ++n)
    {
        XfceItembarChild *child = l->data;

        if (item == child->widget)
        {
            return n;
        }
    }

    return -1;
}

/**
 * xfce_itembar_get_nth_item:
 * @itembar: a #XfceItembar
 * @n: A position on the itembar
 *
 * Returns the @n<!-- -->'s item on @itembar, or %NULL if the
 * itembar does not contain an @n<!-- -->'th item.
 * 
 * Return value: The @n<!-- -->'th #GtkWidget on @itembar, or %NULL if there
 * isn't an @n<!-- -->th item.
 **/
GtkWidget *
xfce_itembar_get_nth_item (XfceItembar * itembar, int n)
{
    XfceItembarPrivate *priv;
    XfceItembarChild *child;
    int n_items;

    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), NULL);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    n_items = g_list_length (priv->children);

    if (n < 0 || n >= n_items)
	return NULL;

    child = g_list_nth_data (priv->children, n);

    return child->widget;
}

/**
 * xfce_itembar_raise_event_window
 * @itembar: a #XfceItembar
 *
 * Raise the event window of @itembar. This causes all events, like
 * mouse clicks or key presses to be send to the itembar and not to 
 * any item.
 *
 * See also: xfce_itembar_lower_event_window
 **/
void
xfce_itembar_raise_event_window (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;

    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    priv->raised = TRUE;

    if (priv->event_window)
        gdk_window_raise (priv->event_window);
}

/**
 * xfce_itembar_lower_event_window
 * @itembar: a #XfceItembar
 *
 * Lower the event window of @itembar. This causes all events, like
 * mouse clicks or key presses to be send to the items, before reaching the
 * itembar.
 *
 * See also: xfce_itembar_raise_event_window
 **/
void
xfce_itembar_lower_event_window (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;

    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    priv->raised = FALSE;

    if (priv->event_window)
        gdk_window_lower (priv->event_window);
}

/**
 * xfce_itembar_event_window_is_raised
 * @itembar: a #XfceItembar
 *
 * Returns: %TRUE if event window is raised.
 **/
gboolean
xfce_itembar_event_window_is_raised (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;

    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), FALSE);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    return priv->raised;
}

/**
 * xfce_itembar_get_item_at_point
 * @itembar: a #XfceItembar
 * @x      : x coordinate relative to the itembar window
 * @y      : y coordinate relative to the itembar window
 *
 * Returns: a #GtkWidget or %NULL.
 **/
GtkWidget *
xfce_itembar_get_item_at_point (XfceItembar * itembar, int x, int y)
{
    XfceItembarPrivate *priv;
    GList *l;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), NULL);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    for (l = priv->children; l != NULL; l = l->next)
    {
        XfceItembarChild *child = l->data;
        GtkWidget *w = child->widget;
        GtkAllocation *a = &(w->allocation);

        if (x >= a->x && x < a->x + a->width 
            && y >= a->y && y < a->y + a->height)
        {
            return w;
        }
    }

    return NULL;
}

/**
 * xfce_itembar_get_drop_index:
 * @itembar: a #XfceItembar
 * @x: x coordinate of a point on the itembar
 * @y: y coordinate of a point on the itembar
 *
 * Returns the position corresponding to the indicated point on
 * @itembar. This is useful when dragging items to the itembar:
 * this function returns the position a new item should be
 * inserted.
 *
 * @x and @y are in @itembar coordinates.
 * 
 * Returns: The position corresponding to the point (@x, @y) on the 
 * itembar.
 **/
int
xfce_itembar_get_drop_index (XfceItembar * itembar, int x, int y)
{
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), FALSE);

    return _find_drop_index (itembar, x, y);
}


