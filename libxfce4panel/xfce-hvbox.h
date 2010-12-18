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

/* #if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif */

#ifndef __XFCE_HVBOX_H__
#define __XFCE_HVBOX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XfceHVBoxClass XfceHVBoxClass;
typedef struct _XfceHVBox      XfceHVBox;

#define XFCE_TYPE_HVBOX            (xfce_hvbox_get_type())
#define XFCE_HVBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST (obj, XFCE_TYPE_HVBOX, XfceHVBox))
#define XFCE_HVBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST (klass, XFCE_TYPE_HVBOX, XfceHVBoxClass))
#define XFCE_IS_HVBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, XFCE_TYPE_HVBOX))
#define XFCE_IS_HVBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_HVBOX))
#define XFCE_HVBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_HVBOX, XfceHVBoxClass))

struct _XfceHVBoxClass
{
  /*< private >*/
  GtkBoxClass __parent__;
};

/**
 * XfceHVBox:
 *
 * This struct contain private data only and should be accessed by
 * the functions below.
 **/
struct _XfceHVBox
{
  /*< private >*/
  GtkBox __parent__;

  GtkOrientation orientation;
};

GType           xfce_hvbox_get_type         (void) G_GNUC_CONST;

GtkWidget      *xfce_hvbox_new              (GtkOrientation  orientation,
                                             gboolean        homogeneous,
                                             gint            spacing) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void            xfce_hvbox_set_orientation  (XfceHVBox      *hvbox,
                                             GtkOrientation  orientation);
GtkOrientation  xfce_hvbox_get_orientation  (XfceHVBox      *hvbox);

G_END_DECLS

#endif /* !__XFCE_HVBOX_H__ */
