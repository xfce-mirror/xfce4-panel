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

#ifndef _XFCE_ITEM_H
#define _XFCE_ITEM_H

#include <gtk/gtkenums.h>
#include <gtk/gtkbin.h>

#define XFCE_TYPE_ITEM            (xfce_item_get_type ())
#define XFCE_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_ITEM, XfceItem))
#define XFCE_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_ITEM, XfceItemClass))
#define XFCE_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_ITEM))
#define XFCE_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_ITEM))
#define XFCE_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_ITEM, XfceItemClass))


G_BEGIN_DECLS

typedef struct _XfceItem 	XfceItem;

typedef struct _XfceItemClass 	XfceItemClass;

struct _XfceItem
{
    GtkBin parent;
};

struct _XfceItemClass
{
    GtkBinClass parent_class;

    /* signals */
    void (*orientation_changed)   (XfceItem * item,
                                   GtkOrientation orientation);

    void (*icon_size_changed)     (XfceItem * item,
                                   int size);

    void (*toolbar_style_changed) (XfceItem * item,
                                   GtkToolbarStyle);

    /* Padding for future expansion */
    void (*_xfce_reserved1)       (void);
    void (*_xfce_reserved2)       (void);
    void (*_xfce_reserved3)       (void);
};


GType xfce_item_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_item_new (void);


GtkOrientation xfce_item_get_orientation    (XfceItem * item);

void xfce_item_set_orientation              (XfceItem * item,
                                             GtkOrientation orientation);


int xfce_item_get_icon_size                 (XfceItem * item);

void xfce_item_set_icon_size                (XfceItem * item,
                                             int size);


GtkToolbarStyle xfce_item_get_toolbar_style (XfceItem * item);

void xfce_item_set_toolbar_style            (XfceItem * item,
                                             GtkToolbarStyle style);


gboolean xfce_item_get_homogeneous          (XfceItem * item);

void xfce_item_set_homogeneous              (XfceItem * item,
                                             gboolean homogeneous);


gboolean xfce_item_get_expand               (XfceItem * item);

void xfce_item_set_expand                   (XfceItem * item,
                                             gboolean expand);


gboolean xfce_item_get_has_handle           (XfceItem * item);

void xfce_item_set_has_handle               (XfceItem * item,
                                             gboolean has_handle);


gboolean xfce_item_get_use_drag_window      (XfceItem * item);

void xfce_item_set_use_drag_window          (XfceItem * item,
                                             gboolean use_drag_window);


G_END_DECLS

#endif /* _XFCE_ITEM_H */
