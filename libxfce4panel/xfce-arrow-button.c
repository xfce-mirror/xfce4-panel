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

#include <common/panel-private.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/libxfce4panel-alias.h>

#define XFCE_ARROW_BUTTON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            XFCE_TYPE_ARROW_BUTTON, \
                                            XfceArrowButtonPrivate))

#define ARROW_WIDTH        (8)
#define MAX_BLINKING_COUNT (16)

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
static void     xfce_arrow_button_finalize      (GObject               *object);
static gboolean xfce_arrow_button_expose_event  (GtkWidget             *widget,
                                                 GdkEventExpose        *event);
static void     xfce_arrow_button_size_request  (GtkWidget             *widget,
                                                 GtkRequisition        *requisition);
static void     xfce_arrow_button_size_allocate (GtkWidget             *widget,
                                                 GtkAllocation         *allocation);



struct _XfceArrowButtonPrivate
{
  /* arrow type of the button */
  GtkArrowType   arrow_type;

  /* blinking timeout id */
  guint          blinking_timeout_id;

  /* counter to make the blinking stop when
   * MAX_BLINKING_COUNT is reached */
  guint          blinking_counter;

  /* button relief when the blinking starts */
  GtkReliefStyle last_relief;
};



static guint arrow_button_signals[LAST_SIGNAL];



G_DEFINE_TYPE (XfceArrowButton, xfce_arrow_button, GTK_TYPE_TOGGLE_BUTTON)



static void
xfce_arrow_button_class_init (XfceArrowButtonClass * klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  g_type_class_add_private (klass, sizeof (XfceArrowButtonPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_arrow_button_get_property;
  gobject_class->set_property = xfce_arrow_button_set_property;
  gobject_class->finalize = xfce_arrow_button_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->expose_event = xfce_arrow_button_expose_event;
  gtkwidget_class->size_request = xfce_arrow_button_size_request;
  gtkwidget_class->size_allocate = xfce_arrow_button_size_allocate;

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
                                                      G_PARAM_READWRITE
                                                      | G_PARAM_STATIC_STRINGS));
}



static void
xfce_arrow_button_init (XfceArrowButton *button)
{
  /* set private pointer */
  button->priv = XFCE_ARROW_BUTTON_GET_PRIVATE (button);

  /* initialize button values */
  button->priv->arrow_type = GTK_ARROW_UP;
  button->priv->blinking_timeout_id = 0;
  button->priv->blinking_counter = 0;
  button->priv->last_relief = GTK_RELIEF_NORMAL;

  /* set some widget properties */
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_NO_WINDOW);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
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



static void
xfce_arrow_button_finalize (GObject *object)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (object);

  if (button->priv->blinking_timeout_id != 0)
    g_source_remove (button->priv->blinking_timeout_id);

  (*G_OBJECT_CLASS (xfce_arrow_button_parent_class)->finalize) (object);
}



static gboolean
xfce_arrow_button_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (widget);
  GtkWidget       *child;
  gint             x, y, width;

  /* draw the button */
  (*GTK_WIDGET_CLASS (xfce_arrow_button_parent_class)->expose_event) (widget, event);

  if (button->priv->arrow_type != GTK_ARROW_NONE
      && GTK_WIDGET_DRAWABLE (widget))
    {
      child = gtk_bin_get_child (GTK_BIN (widget));
      if (child != NULL && GTK_WIDGET_VISIBLE (child))
        {
          if (button->priv->arrow_type == GTK_ARROW_UP
              || button->priv->arrow_type == GTK_ARROW_DOWN)
            {
              width = ARROW_WIDTH;
              x = widget->allocation.x + widget->style->xthickness;
              y = widget->allocation.y + (widget->allocation.height - width) / 2;
            }
          else
            {
              width = ARROW_WIDTH;
              x = widget->allocation.x + (widget->allocation.width - width) / 2;
              y = widget->allocation.y + widget->style->ythickness;
            }
        }
      else
        {
          width = MIN (widget->allocation.height - 2 * widget->style->ythickness,
                       widget->allocation.width  - 2 * widget->style->xthickness);
          width = CLAMP (width, 1, ARROW_WIDTH);

          x = widget->allocation.x + (widget->allocation.width - width) / 2;
          y = widget->allocation.y + (widget->allocation.height - width) / 2;
        }

      gtk_paint_arrow (widget->style, widget->window,
                       GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                       &(event->area), widget, "xfce_arrow_button",
                       button->priv->arrow_type, FALSE,
                       x, y, width, width);
    }

  return TRUE;
}



static void
xfce_arrow_button_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (widget);
  GtkWidget *child;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child != NULL && GTK_WIDGET_VISIBLE (child))
    {
      /* use gtk for the widget size */
      (*GTK_WIDGET_CLASS (xfce_arrow_button_parent_class)->size_request) (widget, requisition);

      /* reserve space for the arrow */
      switch (button->priv->arrow_type)
        {
          case GTK_ARROW_UP:
          case GTK_ARROW_DOWN:
            requisition->width += ARROW_WIDTH;
            break;

          case GTK_ARROW_LEFT:
          case GTK_ARROW_RIGHT:
            requisition->height += ARROW_WIDTH;
            break;

          default:
            break;
        }
    }
  else if (button->priv->arrow_type != GTK_ARROW_NONE)
    {
      requisition->height = ARROW_WIDTH + 2 * widget->style->xthickness;
      requisition->width = ARROW_WIDTH + 2 * widget->style->ythickness;
    }
}



static void
xfce_arrow_button_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (widget);
  GtkWidget       *child;
  GtkAllocation    child_allocation;

  /* allocate the button */
  (*GTK_WIDGET_CLASS (xfce_arrow_button_parent_class)->size_allocate) (widget, allocation);

  if (button->priv->arrow_type != GTK_ARROW_NONE)
    {
      child = gtk_bin_get_child (GTK_BIN (widget));
      if (child != NULL && GTK_WIDGET_VISIBLE (child))
        {
          /* copy the child allocation */
          child_allocation = child->allocation;

          /* update the allocation to make space for the arrow */
          switch (button->priv->arrow_type)
            {
              case GTK_ARROW_LEFT:
              case GTK_ARROW_RIGHT:
                child_allocation.height -= ARROW_WIDTH;
                child_allocation.y += ARROW_WIDTH;
                break;

              default:
                child_allocation.width -= ARROW_WIDTH;
                child_allocation.x += ARROW_WIDTH;
                break;
            }

          /* set the child allocation again */
          gtk_widget_size_allocate (child, &child_allocation);
        }
    }
}



static gboolean
xfce_arrow_button_blinking_timeout (gpointer user_data)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (user_data);
  GtkStyle        *style;
  GtkRcStyle      *rc;

  rc = gtk_widget_get_modifier_style (GTK_WIDGET (button));
  if(PANEL_HAS_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG)
     || button->priv->blinking_timeout_id == 0)
    {
      gtk_button_set_relief (GTK_BUTTON (button), button->priv->last_relief);
      PANEL_UNSET_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG);
      gtk_widget_modify_style (GTK_WIDGET (button), rc);
    }
  else
    {
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NORMAL);
      PANEL_SET_FLAG (rc->color_flags[GTK_STATE_NORMAL], GTK_RC_BG);
      style = gtk_widget_get_style (GTK_WIDGET (button));
      rc->bg[GTK_STATE_NORMAL] = style->bg[GTK_STATE_SELECTED];
      gtk_widget_modify_style(GTK_WIDGET (button), rc);
    }

  return (button->priv->blinking_counter++ < MAX_BLINKING_COUNT);
}



static void
xfce_arrow_button_blinking_timeout_destroyed (gpointer user_data)
{
  XfceArrowButton *button = XFCE_ARROW_BUTTON (user_data);

  button->priv->blinking_timeout_id = 0;
  button->priv->blinking_counter = 0;
}



/**
 * xfce_arrow_button_new:
 * @type : #GtkArrowType for the arrow button
 *
 * Creates a new #XfceArrowButton widget.
 *
 * Returns: The newly created #XfceArrowButton widget.
 **/
GtkWidget *
xfce_arrow_button_new (GtkArrowType arrow_type)
{
  return g_object_new (XFCE_TYPE_ARROW_BUTTON,
                       "arrow-type", arrow_type,
                       NULL);
}



/**
 * xfce_arrow_button_get_arrow_type:
 * @button : a #XfceArrowButton
 *
 * Returns the value of the ::arrow-type property.
 *
 * Returns: the #GtkArrowType of @button.
 **/
GtkArrowType
xfce_arrow_button_get_arrow_type (XfceArrowButton *button)
{
  g_return_val_if_fail (XFCE_IS_ARROW_BUTTON (button), GTK_ARROW_UP);
  return button->priv->arrow_type;
}



/**
 * xfce_arrow_button_set_arrow_type:
 * @button : a #XfceArrowButton
 * @type   : a valid  #GtkArrowType
 *
 * Sets the arrow type for @button.
 **/
void
xfce_arrow_button_set_arrow_type (XfceArrowButton *button,
                                  GtkArrowType     arrow_type)
{
  g_return_if_fail (XFCE_IS_ARROW_BUTTON (button));

  if (G_LIKELY (button->priv->arrow_type != arrow_type))
    {
      /* store the new arrow type */
      button->priv->arrow_type = arrow_type;

      /* emit signal */
      g_signal_emit (G_OBJECT (button),
                     arrow_button_signals[ARROW_TYPE_CHANGED],
                     0, arrow_type);

      /* notify property change */
      g_object_notify (G_OBJECT (button), "arrow-type");

      /* redraw the arrow button */
      gtk_widget_queue_resize (GTK_WIDGET (button));
    }
}



/**
 * xfce_arrow_button_get_blinking:
 * @button : a #XfceArrowButton
 *
 * Whether the button is blinking.
 *
 * Returns: %TRUE when @button is blinking.
 **/
gboolean
xfce_arrow_button_get_blinking (XfceArrowButton *button)
{
  g_return_val_if_fail (XFCE_IS_ARROW_BUTTON (button), FALSE);
  return !!(button->priv->blinking_timeout_id != 0);
}



/**
 * xfce_arrow_button_set_blinking:
 * @button   : a #XfceArrowButton
 * @blinking : %TRUE when the button should start blinking, %FALSE to
 *             stop the blinking.
 *
 * Make the button blink.
 **/
void
xfce_arrow_button_set_blinking (XfceArrowButton *button,
                                gboolean         blinking)
{
  g_return_if_fail (XFCE_IS_ARROW_BUTTON (button));

  if (blinking)
    {
      /* store the relief of the button */
      button->priv->last_relief = gtk_button_get_relief (GTK_BUTTON (button));

      if (button->priv->blinking_timeout_id == 0)
        {
          /* start blinking timeout */
          button->priv->blinking_timeout_id =
              g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 500,
                                  xfce_arrow_button_blinking_timeout, button,
                                  xfce_arrow_button_blinking_timeout_destroyed);
        }
    }
  else if (button->priv->blinking_timeout_id != 0)
    {
      /* stop the blinking timeout */
      g_source_remove (button->priv->blinking_timeout_id);
    }

  /* start with a blinking or make sure the button is normal */
  xfce_arrow_button_blinking_timeout (button);
}



#define __XFCE_ARROW_BUTTON_C__
#include <libxfce4panel/libxfce4panel-aliasdef.c>
