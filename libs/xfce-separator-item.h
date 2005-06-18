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

#ifndef _XFCE_SEPARATOR_ITEM_H
#define _XFCE_SEPARATOR_ITEM_H

#include "xfce-item.h"

#define XFCE_TYPE_SEPARATOR_ITEM            (xfce_separator_item_get_type ())
#define XFCE_SEPARATOR_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SEPARATOR_ITEM, XfceSeparatorItem))
#define XFCE_SEPARATOR_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SEPARATOR_ITEM, XfceSeparatorItemClass))
#define XFCE_IS_SEPARATOR_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SEPARATOR_ITEM))
#define XFCE_IS_SEPARATOR_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SEPARATOR_ITEM))
#define XFCE_SEPARATOR_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SEPARATOR_ITEM, XfceSeparatorItemClass))


G_BEGIN_DECLS

typedef struct _XfceSeparatorItem 	XfceSeparatorItem;

typedef struct _XfceSeparatorItemClass 	XfceSeparatorItemClass;

struct _XfceSeparatorItem
{
    XfceItem parent;
};

struct _XfceSeparatorItemClass
{
    XfceItemClass parent_class;
};


GType xfce_separator_item_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_separator_item_new (void);

gboolean xfce_separator_item_get_show_line (XfceSeparatorItem *item);

void xfce_separator_item_set_show_line     (XfceSeparatorItem *item,
                                            gboolean show);


G_END_DECLS

#endif /* _XFCE_SEPARATOR_ITEM_H */
