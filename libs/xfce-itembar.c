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
    ICON_SIZE_CHANGED,
    TOOLBAR_STYLE_CHANGED,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_ORIENTATION,
    PROP_ICON_SIZE,
    PROP_TOOLBAR_STYLE,
};


typedef struct _XfceItembarPrivate XfceItembarPrivate;

struct _XfceItembarPrivate 
{
    GtkOrientation orientation;
    int icon_size;
    GtkToolbarStyle toolbar_style;

    GList *children;
    int homogeneous_size;

    GdkWindow *event_window;
    GdkWindow *drag_highlight;
    int drop_index;
};


/* init */
static void xfce_itembar_class_init      (XfceItembarClass * klass);

static void xfce_itembar_init            (XfceItembar * itembar);

/* GObject */
static void xfce_itembar_finalize        (GObject *object);

static void xfce_itembar_set_property    (GObject * object,
				          guint prop_id,
				          const GValue * value,
				          GParamSpec * pspec);

static void xfce_itembar_get_property    (GObject * object,
				          guint prop_id,
				          GValue * value, 
                                          GParamSpec * pspec);

/* widget */
static int xfce_itembar_expose           (GtkWidget * widget,
                                          GdkEventExpose * event);

static void xfce_itembar_size_request    (GtkWidget * widget,
                        	          GtkRequisition * requisition);

static void xfce_itembar_size_allocate   (GtkWidget * widget,
                        	          GtkAllocation * allocation);

static void xfce_itembar_realize         (GtkWidget * widget);

static void xfce_itembar_unrealize       (GtkWidget * widget);

static void xfce_itembar_map             (GtkWidget * widget);

static void xfce_itembar_unmap           (GtkWidget * widget);

static void xfce_itembar_drag_leave      (GtkWidget * widget,
				          GdkDragContext * context, 
                                          guint time_);

static gboolean xfce_itembar_drag_motion (GtkWidget * widget,
					  GdkDragContext * context,
					  int x, 
                                          int y, 
                                          guint time_);


/* container */
static void xfce_itembar_forall          (GtkContainer * container,
		                          gboolean include_internals,
		                          GtkCallback callback, 
                                          gpointer callback_data);

static GType xfce_itembar_child_type     (GtkContainer * container);

static void xfce_itembar_add             (GtkContainer * container,
                                          GtkWidget * child);

static void xfce_itembar_remove          (GtkContainer * container,
                                          GtkWidget * child);
    
/* internal functions */
static int _find_drop_index              (XfceItembar * itembar, 
                                          int x, 
                                          int y);

/* global vars */
static GtkBinClass *parent_class = NULL;

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

    gobject_class->finalize     = xfce_itembar_finalize;
    gobject_class->get_property = xfce_itembar_get_property;
    gobject_class->set_property = xfce_itembar_set_property;

    widget_class->expose_event  = xfce_itembar_expose;    
    widget_class->size_request  = xfce_itembar_size_request;    
    widget_class->size_allocate = xfce_itembar_size_allocate;    
    widget_class->realize       = xfce_itembar_realize;
    widget_class->unrealize     = xfce_itembar_unrealize;
    widget_class->map           = xfce_itembar_map;
    widget_class->unmap         = xfce_itembar_unmap;
    widget_class->drag_leave    = xfce_itembar_drag_leave;
    widget_class->drag_motion   = xfce_itembar_drag_motion;

    container_class->forall     = xfce_itembar_forall;
    container_class->child_type = xfce_itembar_child_type;
    container_class->add        = xfce_itembar_add;
    container_class->remove     = xfce_itembar_remove;

    
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

    /**
     * XfceItembar::icon_size
     *
     * The icon size for the #XfceItembar children.
     */
    pspec = g_param_spec_int  ("icon_size",
			       "Icon size",
			       "The icon size of the itembar",
                               MIN_ICON_SIZE,
                               MAX_ICON_SIZE,
			       DEFAULT_ICON_SIZE, 
                               G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_ICON_SIZE, pspec);
    
    /**
     * XfceItembar::toolbar_style
     *
     * The toolbar style for the #XfceItembar children.
     */
    pspec = g_param_spec_enum ("toolbar_style",
			       "Toolbar style",
			       "The toolbar style of the itembar",
			       GTK_TYPE_TOOLBAR_STYLE,
			       DEFAULT_TOOLBAR_STYLE, 
                               G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_TOOLBAR_STYLE, pspec);
    
    
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

    /**
    * XfceItembar::icon_size_changed:
    * @itembar: the object which emitted the signal
    * @size: the new icon size of the itembar
    *
    * Emitted when the icon size of the itembar changes.
    */
    itembar_signals[ICON_SIZE_CHANGED] =
        g_signal_new ("icon-size-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfceItembarClass, 
                                       icon_size_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__INT,
                      G_TYPE_NONE, 1, G_TYPE_INT);

    /**
    * XfceItembar::toolbar_style_changed:
    * @itembar: the object which emitted the signal
    * @style: the new #GtkToolbarStyle of the itembar
    *
    * Emitted when the toolbar style of the itembar changes.
    */
    itembar_signals[TOOLBAR_STYLE_CHANGED] =
        g_signal_new ("toolbar-style-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfceItembarClass, 
                                       toolbar_style_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, GTK_TYPE_TOOLBAR_STYLE);
}

static void
xfce_itembar_init (XfceItembar * itembar)
{
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (itembar), GTK_NO_WINDOW);

    priv->orientation      = DEFAULT_ORIENTATION;
    priv->icon_size        = DEFAULT_ICON_SIZE;
    priv->toolbar_style    = DEFAULT_TOOLBAR_STYLE;
    priv->children         = NULL;
    priv->homogeneous_size = 0;
    priv->event_window     = NULL;
    priv->drag_highlight   = NULL;
    priv->drop_index       = -1;
}

/* GObject */
static void
xfce_itembar_finalize (GObject * object)
{
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (object);

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
        case PROP_ICON_SIZE:
            xfce_itembar_set_icon_size (itembar, g_value_get_int (value));
            break;
        case PROP_TOOLBAR_STYLE:
            xfce_itembar_set_toolbar_style (itembar,
                                            g_value_get_enum (value));
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
        case PROP_ICON_SIZE:
            g_value_set_int (value, xfce_itembar_get_icon_size (itembar));
            break;
        case PROP_TOOLBAR_STYLE:
            g_value_set_enum (value,
                              xfce_itembar_get_toolbar_style (itembar));
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
    int max_width, max_height, n_homogeneous, other_size;

    requisition->width = 2 * GTK_CONTAINER (widget)->border_width;
    requisition->height = 2 * GTK_CONTAINER (widget)->border_width;
    
    max_width = max_height = n_homogeneous = other_size = 0;
    
    for (l = priv->children; l != NULL; l = l->next)
    {
        GtkWidget *child = l->data;
        GtkRequisition req;
        gboolean homogeneous = FALSE;

        if (!GTK_WIDGET_VISIBLE (child))
            continue;

        if (!xfce_item_get_expand (XFCE_ITEM (child)))
            homogeneous = xfce_item_get_homogeneous (XFCE_ITEM (child));

        gtk_widget_size_request (child, &req);
        
        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            max_height = MAX (max_height, req.height);

            if (homogeneous)
            {
                max_width = MAX (max_width, req.width);
                n_homogeneous++;
            }
            else
            {
                other_size += req.width;
            }
        }
        else
        {
            max_width = MAX (max_width, req.width);

            if (homogeneous)
            {
                max_height = MAX (max_height, req.height);
                n_homogeneous++;
            }
            else
            {
                other_size += req.height;
            }
        }
    }

    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        requisition->height += max_height;
        requisition->width += n_homogeneous * max_width + other_size;
        
        priv->homogeneous_size = max_width;
    }
    else
    {
        requisition->width += max_width;
        requisition->height += n_homogeneous * max_height + other_size;

        priv->homogeneous_size = max_height;
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
        gboolean homogeneous; 
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
        GtkWidget *item = l->data;
        gboolean homogeneous, expand;
        int width, height;
        GtkRequisition req;

        if (!GTK_WIDGET_VISIBLE (item))
        {
            props[i].allocation.width = 0;
            continue;
        }
        
        g_object_get (G_OBJECT (item), "homogeneous", &homogeneous,
                      "expand", &expand, NULL);
        
        gtk_widget_size_request (item, &req);
        width = req.width;
        height = req.height;

        if (expand)
        {
            n_expand++;
            homogeneous = FALSE;
        }
        
        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            height = bar_height;
            
            if (expand)
            {
                max_expand = MAX (max_expand, width);
            }
            else if (!homogeneous)
            {
                expand_width -= width;
            }
        }
        else
        {
            width = bar_height;
            
            if (expand)
            {
                max_expand = MAX (max_expand, height);
            }
            else if (!homogeneous)
            {
                expand_width -= height;
            }
        }

        if (homogeneous)
            expand_width -= priv->homogeneous_size;

        props[i].allocation.width = width;
        props[i].allocation.height = height;
        props[i].homogeneous = homogeneous;
        props[i].expand = expand;
    }

    if (n_expand > 0)
    {
        expand_width = MAX (expand_width / n_expand, 0);

        if (expand_width < max_expand)
            use_expand = FALSE;
    }
    
    x = border_width;
    y = border_width;
    
    /* second pass */
    for (l = priv->children, i = 0; l != NULL; l = l->next, i++)
    {
        if (props[i].allocation.width == 0)
            continue;

        props[i].allocation.x = x + allocation->x;
        props[i].allocation.y = y + allocation->y;

        if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
        {
            if (props[i].homogeneous)
                props[i].allocation.width = priv->homogeneous_size;
            else if (props[i].expand && use_expand)
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
            if (props[i].homogeneous)
                props[i].allocation.height = priv->homogeneous_size;
            else if (props[i].expand && use_expand)
                props[i].allocation.height = expand_width;

            y += props[i].allocation.height;
        }
        
        gtk_widget_size_allocate (GTK_WIDGET (l->data), 
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
    XfceItem *item;
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

        item = g_list_nth_data (priv->children, new_index);
	
	priv->drop_index = new_index;
	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
	    if (item)
		new_pos = GTK_WIDGET(item)->allocation.x;
	    else
		new_pos = widget->allocation.x + widget->allocation.width
                    - border_width;
	    
	    gdk_window_move_resize (priv->drag_highlight,
				    new_pos - 1,
				    widget->allocation.y + border_width,
				    2,
				    widget->allocation.height -
				    border_width * 2);
	}
	else
	{
	    if (item)
		new_pos = GTK_WIDGET(item)->allocation.y;
	    else
		new_pos = widget->allocation.y + widget->allocation.height;
	    
	    gdk_window_move_resize (priv->drag_highlight,
				    widget->allocation.x + border_width,
				    new_pos - 1,
				    widget->allocation.width -
				    border_width * 2, 2);
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
        (*callback) (l->data, callback_data);
    }
}

static GType
xfce_itembar_child_type (GtkContainer * container)
{
    return XFCE_TYPE_ITEM;
}

static void
xfce_itembar_add (GtkContainer * container, GtkWidget * child)
{
    xfce_itembar_insert (XFCE_ITEMBAR (container), XFCE_ITEM (child), -1);
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
        GtkWidget *widget = l->data;

        if (widget == child)
        {
            priv->children = g_list_remove_link (priv->children, l);

            gtk_widget_unparent (widget);

            g_list_free (l);

            break;
        }
    }

    gtk_widget_queue_resize (GTK_WIDGET (container));
}


/* internal functions */
static int
_find_drop_index (XfceItembar * itembar, int x, int y)
{
    GList *l;
    GtkTextDirection direction;
    int distance, cursor, pos, i, index;
    XfceItembarPrivate *priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    int best_distance = G_MAXINT;
    
    direction = gtk_widget_get_direction (GTK_WIDGET (itembar));

    index = 0;
    
    if (GTK_ORIENTATION_HORIZONTAL == priv->orientation)
    {
        cursor =  x;
        x = GTK_WIDGET (itembar)->allocation.x;

        if (direction == GTK_TEXT_DIR_LTR)
        {
            pos = GTK_WIDGET (priv->children->data)->allocation.x 
                  - x;
        }
        else
        {
            pos = GTK_WIDGET (priv->children->data)->allocation.x 
                  + GTK_WIDGET (priv->children->data)->allocation.width
                  - x;
        }
    }
    else
    {
        cursor = y;
        y = GTK_WIDGET (itembar)->allocation.y;

        pos = GTK_WIDGET (priv->children->data)->allocation.y
              - y;
    }
    
    best_distance = ABS (pos - cursor);

    /* distance to far end of each item */
    for (i = 1, l = priv->children; l != NULL; l = l->next, i++)
    {
	GtkWidget *widget = l->data;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
	    if (direction == GTK_TEXT_DIR_LTR)
		pos = widget->allocation.x + widget->allocation.width - x;
	    else
		pos = widget->allocation.x - x;
	}
	else
	{
	    pos = y + widget->allocation.y + widget->allocation.height - y;
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

    gtk_container_foreach (GTK_CONTAINER (itembar), 
                           (GtkCallback)xfce_item_set_orientation,
                           GINT_TO_POINTER ((int)orientation));

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}


int 
xfce_itembar_get_icon_size (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), DEFAULT_ICON_SIZE);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    return priv->icon_size;
}

void 
xfce_itembar_set_icon_size (XfceItembar * itembar, int size)
{
    XfceItembarPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    if (size == priv->icon_size)
        return;

    priv->icon_size = size;

    g_signal_emit (itembar, itembar_signals[ICON_SIZE_CHANGED], 0, size);

    g_object_notify (G_OBJECT (itembar), "icon_size");

    gtk_container_foreach (GTK_CONTAINER (itembar), 
                           (GtkCallback)xfce_item_set_icon_size,
                           GINT_TO_POINTER (size));

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}


GtkToolbarStyle 
xfce_itembar_get_toolbar_style (XfceItembar * itembar)
{
    XfceItembarPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), DEFAULT_TOOLBAR_STYLE);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    return priv->toolbar_style;
}

void 
xfce_itembar_set_toolbar_style (XfceItembar * itembar, GtkToolbarStyle style)
{
    XfceItembarPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    if (style == priv->toolbar_style)
        return;

    priv->toolbar_style = style;

    g_object_notify (G_OBJECT (itembar), "toolbar_style");

    g_signal_emit (itembar, itembar_signals[TOOLBAR_STYLE_CHANGED], 0, style);

    gtk_container_foreach (GTK_CONTAINER (itembar), 
                           (GtkCallback)xfce_item_set_toolbar_style,
                           GINT_TO_POINTER ((int)style));
}


void
xfce_itembar_insert (XfceItembar * itembar, XfceItem * item, int position)
{
    XfceItembarPrivate *priv;
    
    g_return_if_fail (XFCE_IS_ITEMBAR (itembar));
    g_return_if_fail (item != NULL && GTK_WIDGET (item)->parent == NULL);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);
    
    priv->children = g_list_insert (priv->children, item, position);

    gtk_widget_set_parent (GTK_WIDGET (item), GTK_WIDGET (itembar));

    gtk_widget_queue_resize (GTK_WIDGET (itembar));
}

void
xfce_itembar_append (XfceItembar * itembar, XfceItem * item)
{
    xfce_itembar_insert (itembar, item, -1);
}

void
xfce_itembar_prepend (XfceItembar * itembar, XfceItem * item)
{
    xfce_itembar_insert (itembar, item, 0);
}

void
xfce_itembar_reorder_child (XfceItembar * itembar, XfceItem * item,
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
        XfceItem *child = l->data;

        if (item == child)
        {
            priv->children = g_list_remove_link (priv->children, l);

            g_list_free (l);

            priv->children = g_list_insert (priv->children, child, position);

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
xfce_itembar_get_item_index (XfceItembar * itembar, XfceItem * item)
{
    XfceItembarPrivate *priv;
    
    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), -1);

    priv = XFCE_ITEMBAR_GET_PRIVATE (XFCE_ITEMBAR (itembar));
    
    return g_list_index (priv->children, item);
}

/**
 * xfce_itembar_get_nth_item:
 * @itembar: a #XfceItembar
 * @n: A position on the itembar
 *
 * Returns the @n<!-- -->'s item on @itembar, or %NULL if the
 * itembar does not contain an @n<!-- -->'th item.
 * 
 * Return value: The @n<!-- -->'th #XfceItem on @itembar, or %NULL if there
 * isn't an @n<!-- -->th item.
 **/
XfceItem *
xfce_itembar_get_nth_item (XfceItembar * itembar, int n)
{
    XfceItembarPrivate *priv;
    XfceItem *item;
    int n_items;

    g_return_val_if_fail (XFCE_IS_ITEMBAR (itembar), NULL);

    priv = XFCE_ITEMBAR_GET_PRIVATE (itembar);

    n_items = g_list_length (priv->children);

    if (n < 0 || n >= n_items)
	return NULL;

    item = g_list_nth_data (priv->children, n);

    return item;
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


