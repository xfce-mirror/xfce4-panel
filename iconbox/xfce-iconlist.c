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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "xfce-iconlist.h"

GType
xfce_iconlist_get_type (void)
{
    return G_TYPE_NONE;
}

GtkOrientation
xfce_iconlist_get_orientation (XfceIconlist * list)
{
    return GTK_ORIENTATION_HORIZONTAL;
}

void
xfce_iconlist_set_orientation (XfceIconlist * list,
                               GtkOrientation orientation)
{
}

int
xfce_iconlist_get_rows (XfceIconlist * list)
{
    return 1;
}

void
xfce_iconlist_set_rows (XfceIconlist * list, int rows)
{
}

int
xfce_iconlist_get_icon_size (XfceIconlist * list)
{
    return 32;
}

void
xfce_iconlist_set_icon_size (XfceIconlist * list, int size)
{
}
