/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

typedef struct _Iconbox Iconbox;

void iconbox_set_justification (Iconbox *ib, GtkJustification justification);

void iconbox_set_side (Iconbox *ib, GtkSideType side);

void iconbox_set_icon_size (Iconbox *ib, int size);

void iconbox_set_show_only_hidden (Iconbox *ib, gboolean only_hidden);

McsClient *iconbox_get_mcs_client (Iconbox *ib);

