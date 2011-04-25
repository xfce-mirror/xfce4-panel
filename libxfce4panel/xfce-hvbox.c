/*
 * This file is partly based on OBox
 * Copyright (C) 2002      Red Hat Inc. based on GtkHBox
 *
 * Copyright (C) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2006      Jani Monoses <jani@ubuntu.com>
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
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

#include <gtk/gtk.h>

#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/libxfce4panel-alias.h>



/**
 * SECTION: xfce-hvbox
 * @title: XfceHVBox
 * @short_description: Container widget with configurable orientation
 * @include: libxfce4panel/libxfce4panel.h
 *
 * #XfceHVBox is a #GtkBox widget that allows the user to change
 * its orientation. It is in fact a combination of #GtkHBox and #GtkVBox.
 *
 * If your code depends on Gtk+ 2.16 or later, if it better to use
 * the normal #GtkBox widgets in combination with
 * gtk_orientable_set_orientation().
 *
 * See also: #GtkOrientable and #GtkBox.
 **/



#if !GTK_CHECK_VERSION (2, 16, 0)
enum
{
  PROP_0,
  PROP_ORIENTATION
};

static void      xfce_hvbox_get_property (GObject        *object,
                                          guint           prop_id,
                                          GValue         *value,
                                          GParamSpec     *pspec);
static void      xfce_hvbox_set_property (GObject        *object,
                                          guint           prop_id,
                                          const GValue   *value,
                                          GParamSpec     *pspec);
static gpointer xfce_hvbox_get_class     (XfceHVBox      *hvbox);
static void     xfce_hvbox_size_request  (GtkWidget      *widget,
                                          GtkRequisition *requisition);
static void     xfce_hvbox_size_allocate (GtkWidget      *widget,
                                          GtkAllocation  *allocation);
#endif



G_DEFINE_TYPE (XfceHVBox, xfce_hvbox, GTK_TYPE_BOX)



static void
xfce_hvbox_class_init (XfceHVBoxClass *klass)
{
#if !GTK_CHECK_VERSION (2, 16, 0)
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_hvbox_get_property;
  gobject_class->set_property = xfce_hvbox_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request  = xfce_hvbox_size_request;
  gtkwidget_class->size_allocate = xfce_hvbox_size_allocate;

  /**
   * XfceHVBox:orientation:
   *
   * The orientation of the #XfceHVBox. When compiled with Gtk+ 2.16,
   * this is the orientation property of the #GtkOrientable.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      "Orientation",
                                                      "Orientation of the box",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      G_PARAM_READWRITE
                                                      | G_PARAM_STATIC_STRINGS));
#endif
}



static void
xfce_hvbox_init (XfceHVBox *hvbox)
{
  /* initialize variables */
  hvbox->orientation = GTK_ORIENTATION_HORIZONTAL;
}



#if !GTK_CHECK_VERSION (2, 16, 0)
static void
xfce_hvbox_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  XfceHVBox *hvbox = XFCE_HVBOX (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, hvbox->orientation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_hvbox_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  XfceHVBox *hvbox = XFCE_HVBOX (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      xfce_hvbox_set_orientation (hvbox, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gpointer
xfce_hvbox_get_class (XfceHVBox *hvbox)
{
  GType    type;
  gpointer klass;

  if (hvbox->orientation == GTK_ORIENTATION_HORIZONTAL)
    type = GTK_TYPE_HBOX;
  else
    type = GTK_TYPE_VBOX;

  /* peek the class, this only works if the class already exists */
  klass = g_type_class_peek (type);

  /* return the type or create the class */
  return klass ? klass : g_type_class_ref (type);
}



static void
xfce_hvbox_size_request (GtkWidget      *widget,
                         GtkRequisition *requisition)
{
  gpointer klass;

  /* get the widget class */
  klass = xfce_hvbox_get_class (XFCE_HVBOX (widget));

  /* request the size */
  (*GTK_WIDGET_CLASS (klass)->size_request) (widget, requisition);
}



static void
xfce_hvbox_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  gpointer klass;

  /* get the widget class */
  klass = xfce_hvbox_get_class (XFCE_HVBOX (widget));

  /* allocate the size */
  (*GTK_WIDGET_CLASS (klass)->size_allocate) (widget, allocation);
}
#endif



/**
 * xfce_hvbox_new:
 * @orientation : Orientation of the #XfceHVBox
 * @homogeneous : whether all children should be allocated the same size
 * @spacing     : spacing between #XfceHVBox children
 *
 * Creates a new #XfceHVBox container widget.
 *
 * Returns: the newly allocated #XfceHVBox container widget.
 **/
GtkWidget *
xfce_hvbox_new (GtkOrientation orientation,
                gboolean       homogeneous,
                gint           spacing)
{
  XfceHVBox *box;

  /* create new object */
  box = g_object_new (XFCE_TYPE_HVBOX,
#if GTK_CHECK_VERSION (2, 16, 0)
                      "orientation", orientation,
#endif
                      "homogeneous", homogeneous,
                      "spacing", spacing, NULL);

  /* store the orientation */
  box->orientation = orientation;

  return GTK_WIDGET (box);
}



/**
 * xfce_hvbox_set_orientation:
 * @hvbox       : #XfceHVBox
 * @orientation : the new orientation of the #XfceHVBox
 *
 * Set the new orientation of the #XfceHVBox container widget.
 **/
void
xfce_hvbox_set_orientation (XfceHVBox      *hvbox,
                            GtkOrientation  orientation)
{
  g_return_if_fail (XFCE_IS_HVBOX (hvbox));

  if (G_LIKELY (hvbox->orientation != orientation))
    {
      /* store new orientation */
      hvbox->orientation = orientation;

#if GTK_CHECK_VERSION (2, 16, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE (hvbox), orientation);
#else
      gtk_widget_queue_resize (GTK_WIDGET (hvbox));
#endif
    }
}



/**
 * xfce_hvbox_get_orientation:
 * @hvbox       : #XfceHVBox
 *
 * Get the current orientation of the @hvbox.
 *
 * Returns: the current orientation of the #XfceHVBox.
 **/
GtkOrientation
xfce_hvbox_get_orientation (XfceHVBox *hvbox)
{
  g_return_val_if_fail (XFCE_IS_HVBOX (hvbox), GTK_ORIENTATION_HORIZONTAL);

#if GTK_CHECK_VERSION (2, 16, 0)
  return gtk_orientable_get_orientation (GTK_ORIENTABLE (hvbox));
#else
  return hvbox->orientation;
#endif
}



#define __XFCE_HVBOX_C__
#include <libxfce4panel/libxfce4panel-aliasdef.c>
