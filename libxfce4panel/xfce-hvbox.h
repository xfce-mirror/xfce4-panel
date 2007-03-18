/*  $Id$
 *
 *  OBox Copyright (c) 2002      Red Hat Inc. based on GtkHBox
 *  Copyright (c)      2006      Jani Monoses <jani@ubuntu.com>
 *  Copyright (c)      2006-2007 Jasper Huijsmans <jasper@xfce.org>
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

#ifndef __XFCE_HVBOX_H__
#define __XFCE_HVBOX_H__

#include <gtk/gtkbox.h>

G_BEGIN_DECLS

typedef struct _XfceHVBox      XfceHVBox;
typedef struct _XfceHVBoxClass XfceHVBoxClass;

#define XFCE_TYPE_HVBOX            (xfce_hvbox_get_type())
#define XFCE_HVBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, xfce_hvbox_get_type (), XfceHVBox))
#define XFCE_HVBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, xfce_hvbox_get_type (), XfceHVBoxClass))
#define XFCE_IS_HVBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, xfce_hvbox_get_type ()))
#define XFCE_IS_HVBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_HVBOX))
#define XFCE_HVBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_HVBOX, TrayOBoxClass))

struct _XfceHVBox
{
    GtkBox         hvbox;
    GtkOrientation orientation;
};

struct _XfceHVBoxClass
{
    GtkBoxClass parent_class;
};

GType       xfce_hvbox_get_type         (void) G_GNUC_CONST;

GtkWidget  *xfce_hvbox_new              (GtkOrientation   orientation,
                                         gboolean         homogeneous,
                                         gint             spacing) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void        xfce_hvbox_set_orientation  (XfceHVBox       *hvbox,
                                         GtkOrientation   orientation);

G_END_DECLS

#endif /* !__XFCE_HVBOX_H__ */
