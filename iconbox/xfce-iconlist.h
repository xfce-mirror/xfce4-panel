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

#include <gtk/gtkenums.h>
#include <gtk/gtkwidget.h>

typedef struct _XfceIconlist XfceIconlist;

typedef struct _XfceIconlistClass XfceIconlistClass;

struct _IconlistClass
{
    GtkWidgetClass parent_class;
};

struct _Iconlist
{
    GtkWidget parent;
};

G_BEGIN_DECLS

GType xfce_iconlist_get_type (void) G_GNUC_CONST;

GtkOrientation xfce_iconlist_get_orientation (XfceIconlist * list);

void xfce_iconlist_set_orientation (XfceIconlist * list,
                                    GtkOrientation orientation);

int xfce_iconlist_get_rows (XfceIconlist * list);

void xfce_iconlist_set_rows (XfceIconlist * list, int rows);

int xfce_iconlist_get_icon_size (XfceIconlist * list);

void xfce_iconlist_set_icon_size (XfceIconlist * list, int size);

G_END_DECLS
