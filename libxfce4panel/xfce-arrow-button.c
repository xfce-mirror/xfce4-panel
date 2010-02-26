/* $Id$ */
/*
 * Copyright (C) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

#define ARROW_WIDTH   (8)
#define ARROW_PADDING (2)

enum
{
  ARROW_TYPE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ARROW_TYPE
};

static void     xfce_arrow_button_set_property  (GObject               *object,
                                                 guint                  prop_id,
                                                 const GValue          *value,
                                                 GParamSpec            *pspec);
static void     xfce_arrow_button_get_property  (GObject               *object,
                                                 guint                  prop_id,
                                                 GValue                *value,
                                                 GParamSpec            *pspec);
static gboolean xfce_arrow_button_expose_event  (GtkWidget             *widget,
                                                 GdkEventExpose        *event);
static void     xfce_arrow_button_size_request  (GtkWidget             *widget,
                                                 GtkRequisition        *requisition);
static void     xfce_arrow_button_add           (GtkContainer          *container,
                                                 GtkWidget             *child);
static GType    xfce_arrow_button_child_type    (GtkContainer          *container);



static guint arrow_button_signals[LAST_SIGNAL];



G_DEFINE_TYPE (XfceArrowButton, xfce_arrow_button, GTK_TYPE_TOGGLE_BUTTON)



static void
xfce_arrow_button_class_init (XfceArrowButtonClass * klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_arrow_button_get_property;
  gobject_class->set_property = xfce_arrow_button_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->expose_event = xfce_arrow_button_expose_event;
  gtkwidget_class->size_request = xfce_arrow_button_size_request;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = xfce_arrow_button_add;
  gtkcontainer_class->child_type = xfce_arrow_button_child_type;

  /**
   * XfceArrowButton::arrow-type-changed
   * @button: the object which emitted the signal
   * @type: the new #GtkArrowType of the button
   *
   * Emitted when the arrow direction of the menu button changes.
   * This value also determines the direction of the popup menu.
   **/
  arrow_button_signals[ARROW_TYPE_CHANGED] =
      g_signal_new (I_("arrow-type-changed"),
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (XfceArrowButtonClass, arrow_type_changed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__ENUM,
                    G_TYPE_NONE, 1, GTK_TYPE_ARROW_TYPE);

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
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
xfce_arrow_button_init (XfceArrowButton *button)
{
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_NO_WINDOW);

  button->arrow_type = GTK_ARROW_UP;
}



static void
xfce_arrow_button_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ARROW_TYPE:
        xfce_arrow_button_set_arrow_type (button, g_value_get_enum (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_arrow_button_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (object);

  switch (prop_id)
    {
      case PROP_ARROW_TYPE:
        g_value_set_enum (value, xfce_arrow_button_get_arrow_type (button));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static gboolean
xfce_arrow_button_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  gint x, y, w;

  if (G_LIKELY (GTK_WIDGET_DRAWABLE (widget)))
    {
      w = MIN (widget->allocation.height - 2 * widget->style->ythickness,
               widget->allocation.width  - 2 * widget->style->xthickness);
      w = MIN (w, ARROW_WIDTH);

      x = widget->allocation.x + (widget->allocation.width - w) / 2;
      y = widget->allocation.y + (widget->allocation.height - w) / 2;

      (*GTK_WIDGET_CLASS (xfce_arrow_button_parent_class)->expose_event) (widget, event);

      gtk_paint_arrow (widget->style, widget->window,
                       GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                       &(event->area), widget, "xfce_arrow_button",
                       XFCE_ARROW_BUTTON (widget)->arrow_type, FALSE,
                       x, y, w, w);
    }

  return TRUE;
}



static void
xfce_arrow_button_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  gint size;

  /* calculate the requested arrow size */
  size = ARROW_WIDTH + ARROW_PADDING + 2 * MAX (widget->style->xthickness, widget->style->ythickness);

  requisition->width = requisition->height = size;
}



static void
xfce_arrow_button_add (GtkContainer *container,
                       GtkWidget    *child)
{
  g_warning ("XfceArrowButton cannot contain any children");
}



static GType
xfce_arrow_button_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}



/**
 * xfce_arrow_button_new:
 * @type : #GtkArrowType for the arrow button
 *
 * Creates a new #XfceArrowButton widget.
 *
 * Returns: The newly created #XfceArrowButton widget.
 **/
PANEL_SYMBOL_EXPORT GtkWidget *
xfce_arrow_button_new (GtkArrowType arrow_type)
{
  return g_object_new (XFCE_TYPE_ARROW_BUTTON, "arrow-type", arrow_type, NULL);
}



/**
 * xfce_arrow_button_set_arrow_type:
 * @button : a #XfceArrowButton
 * @type   : a valid  #GtkArrowType
 *
 * Sets the arrow type for @button.
 **/
PANEL_SYMBOL_EXPORT void
xfce_arrow_button_set_arrow_type (XfceArrowButton *button,
                                  GtkArrowType     arrow_type)
{
  g_return_if_fail (XFCE_IS_ARROW_BUTTON (button));

  if (G_LIKELY (button->arrow_type != arrow_type))
    {
      /* store the new arrow type */
      button->arrow_type = arrow_type;

      /* emit signal */
      g_signal_emit (G_OBJECT (button), arrow_button_signals[ARROW_TYPE_CHANGED], 0, arrow_type);

      /* notify property change */
      g_object_notify (G_OBJECT (button), "arrow-type");

      /* redraw the arrow button */
      gtk_widget_queue_draw (GTK_WIDGET (button));
    }
}



/**
 * xfce_arrow_button_get_arrow_type:
 * @button : a #XfceArrowButton
 *
 * Returns the value of the ::arrow-type property.
 *
 * Returns: the #GtkArrowType of @button.
 **/
PANEL_SYMBOL_EXPORT GtkArrowType
xfce_arrow_button_get_arrow_type (XfceArrowButton *button)
{
  g_return_val_if_fail (XFCE_IS_ARROW_BUTTON (button), GTK_ARROW_UP);

  return button->arrow_type;
}
