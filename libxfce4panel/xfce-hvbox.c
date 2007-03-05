/*  $Id$
 *
 *  OBox Copyright (c) 2002      Red Hat Inc. based on GtkHBox
 *  Copyright      (c) 2006      Jani Monoses <jani@ubuntu.com>
 *  Copyright      (c) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
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

#include <gdk/gdk.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>

#include "xfce-panel-macros.h"
#include "xfce-hvbox.h"



/* prototypes */
static void  xfce_hvbox_class_init     (XfceHVBoxClass *klass);
static void  xfce_hvbox_init           (XfceHVBox      *hvbox);
static void  xfce_hvbox_size_request   (GtkWidget      *widget,
                                        GtkRequisition *requisition);
static void  xfce_hvbox_size_allocate  (GtkWidget      *widget,
                                        GtkAllocation  *allocation);



GtkType
xfce_hvbox_get_type (void)
{
  static GtkType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        type = _panel_g_type_register_simple (GTK_TYPE_BOX,
                                              "XfceHVBox",
                                              sizeof (XfceHVBoxClass),
                                              xfce_hvbox_class_init,
                                              sizeof (XfceHVBox),
                                              xfce_hvbox_init);
    }

    return type;
}



static void
xfce_hvbox_class_init (XfceHVBoxClass *klass)
{
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) klass;

    widget_class->size_request  = xfce_hvbox_size_request;
    widget_class->size_allocate = xfce_hvbox_size_allocate;
}



static void
xfce_hvbox_init (XfceHVBox *hvbox)
{
    /* empty */
}



static GtkWidgetClass *
get_class (XfceHVBox *hvbox)
{
    GtkWidgetClass *klass;

    switch (hvbox->orientation)
    {
        case GTK_ORIENTATION_HORIZONTAL:
            klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_HBOX));
            break;
        case GTK_ORIENTATION_VERTICAL:
            klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_VBOX));
            break;
        default:
            g_assert_not_reached ();
            klass = NULL;
            break;
    }

    return klass;
}



static void
xfce_hvbox_size_request (GtkWidget      *widget,
                         GtkRequisition *requisition)
{
    GtkWidgetClass *klass;
    XfceHVBox      *hvbox;

    hvbox = XFCE_HVBOX (widget);

    klass = get_class (hvbox);

    klass->size_request (widget, requisition);
}



static void
xfce_hvbox_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
    GtkWidgetClass *klass;
    XfceHVBox      *hvbox;

    hvbox = XFCE_HVBOX (widget);

    klass = get_class (hvbox);

    klass->size_allocate (widget, allocation);
}



/**
 * xfce_hvbox_new:
 * @orienation  : Orientation of the #XfceHVBox
 * @homogeneous : whether all children should be allocated the same size
 * @spacing     : spacing between #XfceHVBox children
 *
 * Creates a new #XfceHVBox container widget.
 *
 * Return value: the newly allocated #XfceHVBox container widget.
 **/
GtkWidget *
xfce_hvbox_new (GtkOrientation orientation,
                gboolean       homogeneous,
                gint           spacing)
{
    GtkWidget *box = GTK_WIDGET (g_object_new (xfce_hvbox_get_type (), NULL));

    XFCE_HVBOX (box)->orientation = orientation;
    GTK_BOX (box)->spacing        = spacing;
    GTK_BOX (box)->homogeneous    = homogeneous;

    return box;
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

    if (G_UNLIKELY (hvbox->orientation == orientation))
        return;

    hvbox->orientation = orientation;

    gtk_widget_queue_resize (GTK_WIDGET (hvbox));
}
