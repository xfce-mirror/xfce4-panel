/*
 *  Copyright (c) 2017 Viktor Odintsev <ninetls@xfce.org>
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

#ifndef __SN_ICON_BOX_H__
#define __SN_ICON_BOX_H__

#include "sn-config.h"
#include "sn-item.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_SN_ICON_BOX (sn_icon_box_get_type ())
G_DECLARE_FINAL_TYPE (SnIconBox, sn_icon_box, XFCE, SN_ICON_BOX, GtkContainer)

GtkWidget *
sn_icon_box_new (SnItem *item,
                 SnConfig *config);

G_END_DECLS

#endif /* !__SN_ICON_BOX_H__ */
