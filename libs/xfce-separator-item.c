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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include "xfce-separator-item.h"

#define XFCE_SEPARATOR_ITEM_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_SEPARATOR_ITEM, \
                                  XfceSeparatorItemPrivate))

#define DEFAULT_SHOW_LINE TRUE
#define DEFAULT_SEP_WIDTH 10
#define SEP_START         0.15
#define SEP_END           0.85

enum
{
    PROP_0,
    PROP_SHOW_LINE
};


typedef struct _XfceSeparatorItemPrivate XfceSeparatorItemPrivate;

struct _XfceSeparatorItemPrivate 
{
    guint show_line:1;
};


/* init */
static void xfce_separator_item_class_init   (XfceSeparatorItemClass * klass);

static void xfce_separator_item_init         (XfceSeparatorItem * item);

/* GObject */
static void xfce_separator_item_set_property  (GObject * object,
                                               guint prop_id,
                                               const GValue * value,
                                               GParamSpec * pspec);

static void xfce_separator_item_get_property  (GObject * object,
                                               guint prop_id,
                                               GValue * value, 
                                               GParamSpec * pspec);

/* widget */
static int xfce_separator_item_expose        (GtkWidget * widget,
                                              GdkEventExpose * event);

static void xfce_separator_item_size_request (GtkWidget * widget,
                        	              GtkRequisition * requisition);

/* container */
static GType xfce_separator_item_child_type  (GtkContainer * container);

static void xfce_separator_item_add          (GtkContainer * container,
                                              GtkWidget * child);

    
/* global vars */
static XfceItemClass *parent_class = NULL;


GType
xfce_separator_item_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GTypeInfo type_info = {
	    sizeof (XfceSeparatorItemClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) xfce_separator_item_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,
	    sizeof (XfceSeparatorItem),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) xfce_separator_item_init,
	};

	type = g_type_register_static (XFCE_TYPE_ITEM, "XfceSeparatorItem", 
                                       &type_info, 0);
    }

    return type;
}


static void
xfce_separator_item_class_init (XfceSeparatorItemClass * klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;
    GParamSpec *pspec;

    g_type_class_add_private (klass, sizeof (XfceSeparatorItemPrivate));

    parent_class = g_type_class_peek_parent (klass);

    gobject_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gobject_class->get_property = xfce_separator_item_get_property;
    gobject_class->set_property = xfce_separator_item_set_property;

    widget_class->expose_event  = xfce_separator_item_expose;    
    widget_class->size_request  = xfce_separator_item_size_request;    

    container_class->child_type = xfce_separator_item_child_type;
    container_class->add        = xfce_separator_item_add;

    /* properties */
    
    /**
     * XfceSeparatorItem::show_line
     *
     * Whether to show a separator line on the #XfceSeparatorItem.
     */
    pspec = g_param_spec_boolean ("show_line",
			          "Show line",
			          "Whether to show a separator line",
			          DEFAULT_SHOW_LINE, 
                                  G_PARAM_READWRITE);
    
    g_object_class_install_property (gobject_class, PROP_SHOW_LINE, pspec);
}

static void
xfce_separator_item_init (XfceSeparatorItem * item)
{
    XfceSeparatorItemPrivate *priv = XFCE_SEPARATOR_ITEM_GET_PRIVATE (item);
    
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (item), GTK_NO_WINDOW);

    priv->show_line = DEFAULT_SHOW_LINE;
}

static void
xfce_separator_item_set_property (GObject * object, guint prop_id, 
                                  const GValue * value, GParamSpec * pspec)
{
    XfceSeparatorItem *item = XFCE_SEPARATOR_ITEM (object);

    switch (prop_id)
    {
        case PROP_SHOW_LINE:
            xfce_separator_item_set_show_line (item, 
                                               g_value_get_boolean (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
xfce_separator_item_get_property (GObject * object, guint prop_id,
                                  GValue * value, GParamSpec * pspec)
{
    XfceSeparatorItem *item = XFCE_SEPARATOR_ITEM (object);

    switch (prop_id)
    {
        case PROP_SHOW_LINE:
            g_value_set_boolean (value,
                                 xfce_separator_item_get_show_line (item));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static int
xfce_separator_item_expose (GtkWidget * widget, GdkEventExpose *event)
{
    if (GTK_WIDGET_DRAWABLE (widget) 
        && xfce_separator_item_get_show_line (XFCE_SEPARATOR_ITEM (widget)))
    {
        GtkAllocation *allocation = &(widget->allocation);
        int start, end, position;

        if (GTK_ORIENTATION_HORIZONTAL 
            == xfce_item_get_orientation (XFCE_ITEM (widget)))
        {
            start = allocation->y + SEP_START * allocation->height;
            end = allocation->y + SEP_END * allocation->height;
            position = allocation->x + allocation->width / 2;
        
            gtk_paint_vline (widget->style, widget->window,
                             GTK_STATE_NORMAL,
                             &(event->area), widget, "xfce-separator-item",
                             start, end, position);
        }
        else
        {
            start = allocation->x + SEP_START * allocation->width;
            end = allocation->x + SEP_END * allocation->width;
            position = allocation->y + allocation->height / 2;
        
            gtk_paint_hline (widget->style, widget->window, 
                             GTK_STATE_NORMAL,
                             &(event->area), widget, "xfce-separator-item",
                             start, end, position);
        }

        return TRUE;
    }

    return FALSE;
}

static void
xfce_separator_item_size_request (GtkWidget * widget, 
                                  GtkRequisition * requisition)
{
    requisition->width = requisition->height = DEFAULT_SEP_WIDTH;
}

static GType
xfce_separator_item_child_type (GtkContainer * container)
{
    return G_TYPE_NONE;
}

static void
xfce_separator_item_add (GtkContainer * container, GtkWidget * child)
{
    g_warning ("It is not possible to add a widget to an XfceSeparatorItem.");
}

/* public interface */

GtkWidget *
xfce_separator_item_new (void)
{
    XfceSeparatorItem *separator_item;

    separator_item = g_object_new (XFCE_TYPE_SEPARATOR_ITEM, 
                                   "homogeneous", FALSE, 
                                   "use_drag_window", TRUE,
                                   NULL);

    return GTK_WIDGET (separator_item);
}

gboolean
xfce_separator_item_get_show_line (XfceSeparatorItem *item)
{
    XfceSeparatorItemPrivate *priv;

    g_return_val_if_fail (XFCE_IS_SEPARATOR_ITEM (item), DEFAULT_SHOW_LINE);
    
    priv = XFCE_SEPARATOR_ITEM_GET_PRIVATE (item);

    return priv->show_line;
}

void
xfce_separator_item_set_show_line (XfceSeparatorItem *item, gboolean show)
{
    XfceSeparatorItemPrivate *priv;

    g_return_if_fail (XFCE_IS_SEPARATOR_ITEM (item));
    
    priv = XFCE_SEPARATOR_ITEM_GET_PRIVATE (item);

    if (priv->show_line != show)
    {
        priv->show_line = show;

        g_object_notify (G_OBJECT (item), "show_line");

        gtk_widget_queue_draw (GTK_WIDGET (item));
    }
}


