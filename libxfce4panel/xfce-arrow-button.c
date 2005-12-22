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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-marshal.h"
#include "xfce-panel-enum-types.h"
#include "xfce-arrow-button.h"

#define ARROW_WIDTH          8
#define ARROW_PADDING        2
#define DEFAULT_ARROW_TYPE   GTK_ARROW_UP


#ifndef _
#define _(x) (x)
#endif

enum
{
    ARROW_TYPE_CHANGED,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_ARROW_TYPE,
};


static void xfce_arrow_button_class_init   (XfceArrowButtonClass * klass);

static void xfce_arrow_button_init         (XfceArrowButton * button);

static void xfce_arrow_button_set_property (GObject * object,
				            guint prop_id,
				            const GValue * value,
				            GParamSpec * pspec);

static void xfce_arrow_button_get_property (GObject * object,
				            guint prop_id,
				            GValue * value, 
                                            GParamSpec * pspec);

static int xfce_arrow_button_expose        (GtkWidget * widget,
                                            GdkEventExpose * event);

static void xfce_arrow_button_size_request (GtkWidget * widget,
					    GtkRequisition * requisition);

static void xfce_arrow_button_add          (GtkContainer * container, 
                                            GtkWidget *child);

static GType xfce_arrow_button_child_type  (GtkContainer * container);


    
/* global vars */
static GtkToggleButtonClass *parent_class = NULL;

static guint arrow_button_signals[LAST_SIGNAL] = { 0 };


GType
xfce_arrow_button_get_type (void)
{
    static GtkType type = 0;

    if (!type)
    {
	static const GTypeInfo type_info = {
	    sizeof (XfceArrowButtonClass),
	    (GBaseInitFunc) NULL,
	    (GBaseFinalizeFunc) NULL,
	    (GClassInitFunc) xfce_arrow_button_class_init,
	    (GClassFinalizeFunc) NULL,
	    NULL,
	    sizeof (XfceArrowButton),
	    0,			/* n_preallocs */
	    (GInstanceInitFunc) xfce_arrow_button_init,
	};

	type = g_type_register_static (GTK_TYPE_TOGGLE_BUTTON,
				       "XfceArrowButton", &type_info, 0);
    }

    return type;
}


static void
xfce_arrow_button_class_init (XfceArrowButtonClass * klass)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    parent_class = g_type_class_peek_parent (klass);

    gobject_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    gobject_class->get_property = xfce_arrow_button_get_property;
    gobject_class->set_property = xfce_arrow_button_set_property;

    widget_class->expose_event = xfce_arrow_button_expose;    
    widget_class->size_request = xfce_arrow_button_size_request;    
    
    container_class->add = xfce_arrow_button_add;
    container_class->child_type = xfce_arrow_button_child_type;

    /* signals */

    /**
    * XfceArrowButton::arrow-type-changed
    * @button: the object which emitted the signal
    * @type: the new #GtkArrowType of the button
    *
    * Emitted when the arrow direction of the menu button changes.
    * This value also determines the direction of the popup menu.
    **/
    arrow_button_signals[ARROW_TYPE_CHANGED] =
        g_signal_new ("arrow-type-changed",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (XfceArrowButtonClass, 
                                       arrow_type_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__ENUM,
                      G_TYPE_NONE, 1, GTK_TYPE_ARROW_TYPE);

    /* properties */

    /**
     * XfceArrowButton:arrow-type
     *
     * The arrow type of the button. This value also determines the direction
     * of the popup menu.
     **/
    g_object_class_install_property (gobject_class, 
            PROP_ARROW_TYPE, 
            g_param_spec_enum ("arrow-type",
			       "Arrow type",
			       "The arrow type of the menu button",
			       GTK_TYPE_ARROW_TYPE,
			       GTK_ARROW_UP, 
                               G_PARAM_READWRITE));
}

static void
xfce_arrow_button_init (XfceArrowButton * arrow_button)
{
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (arrow_button), GTK_NO_WINDOW);

    arrow_button->arrow_type = DEFAULT_ARROW_TYPE;
}

static void
xfce_arrow_button_set_property (GObject * object, guint prop_id,
			        const GValue * value, GParamSpec * pspec)
{
    XfceArrowButton *button = XFCE_ARROW_BUTTON (object);

    switch (prop_id)
    {
	case PROP_ARROW_TYPE:
	    xfce_arrow_button_set_arrow_type (button, 
                                              g_value_get_enum (value));
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static void
xfce_arrow_button_get_property (GObject * object,
			        guint prop_id, GValue * value, 
                                GParamSpec * pspec)
{
    XfceArrowButton *button = XFCE_ARROW_BUTTON (object);

    switch (prop_id)
    {
	case PROP_ARROW_TYPE:
	    g_value_set_enum (value, button->arrow_type);
	    break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	    break;
    }
}

static int
xfce_arrow_button_expose (GtkWidget * widget, GdkEventExpose *event)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        int x, y, w;

        w = MIN (widget->allocation.height - 2 * widget->style->ythickness, 
                 widget->allocation.width  - 2 * widget->style->xthickness);

        w = MIN (w, ARROW_WIDTH);
        
        x = widget->allocation.x + (widget->allocation.width - w) / 2;
        
        y = widget->allocation.y + (widget->allocation.height - w) / 2;
        
        GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

        gtk_paint_arrow (widget->style, widget->window,
                         GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                         &(event->area), widget, "xfce_arrow_button",
                         XFCE_ARROW_BUTTON (widget)->arrow_type, FALSE,
                         x, y, w, w);
    }

    return TRUE;
}

static void
xfce_arrow_button_size_request (GtkWidget * widget,
			       GtkRequisition * requisition)
{
    int size = ARROW_WIDTH + ARROW_PADDING + 
               2 * MAX (widget->style->xthickness, widget->style->ythickness);
    
    requisition->width = requisition->height = size;
}

static void xfce_arrow_button_add (GtkContainer * container, GtkWidget *child)
{
    g_warning ("XfceArrowButton cannot contain any children");
}

static GType xfce_arrow_button_child_type (GtkContainer * container)
{
    return GTK_TYPE_NONE;
}


/* public interface */

/**
 * xfce_arrow_button_new:
 * @type : #GtkArrowType for the arrow button
 *
 * Returns: newly created #XfceArrowButton widget.
 **/
GtkWidget *
xfce_arrow_button_new (GtkArrowType type)
{
    XfceArrowButton *button;

    button = g_object_new (XFCE_TYPE_ARROW_BUTTON, "arrow_type", type, NULL);

    return GTK_WIDGET (button);
}

/**
 * xfce_arrow_button_set_arrow_type:
 * @button : an #XfceArrowButton
 * @type   : new #GtkArrowType to set
 *
 * Sets the arrow type for @button.
 **/
void
xfce_arrow_button_set_arrow_type (XfceArrowButton * button,
                                  GtkArrowType type)
{
    g_return_if_fail (XFCE_IS_ARROW_BUTTON (button));
    
    button->arrow_type = type;

    g_signal_emit (button, arrow_button_signals[ARROW_TYPE_CHANGED],
		   0, type);

    g_object_notify (G_OBJECT (button), "arrow_type");

    gtk_widget_queue_draw (GTK_WIDGET (button));
}

/**
 * xfce_arrow_button_get_arrow_type:
 * @button : an #XfceArrowButton
 *
 * Returns: the #GtkArrowType of @button.
 **/
GtkArrowType
xfce_arrow_button_get_arrow_type (XfceArrowButton * button)
{
    g_return_val_if_fail (XFCE_IS_ARROW_BUTTON (button),
			  DEFAULT_ARROW_TYPE);

    return button->arrow_type;
}


