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

#ifndef __SN_ITEM_H__
#define __SN_ITEM_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SN_TYPE_ITEM (sn_item_get_type ())
G_DECLARE_FINAL_TYPE (SnItem, sn_item, SN, ITEM, GObject)

void
sn_item_start (SnItem *item);

void
sn_item_invalidate (SnItem *item,
                    gboolean force_update);

const gchar *
sn_item_get_name (SnItem *item);

void
sn_item_get_icon (SnItem *item,
                  const gchar **theme_path,
                  const gchar **icon_name,
                  GdkPixbuf **icon_pixbuf,
                  const gchar **overlay_icon_name,
                  GdkPixbuf **overlay_icon_pixbuf);

void
sn_item_get_tooltip (SnItem *item,
                     const gchar **title,
                     const gchar **subtitle);

gboolean
sn_item_is_menu_only (SnItem *item);

GtkWidget *
sn_item_get_menu (SnItem *item);

void
sn_item_activate (SnItem *item,
                  gint x_root,
                  gint y_root);

void
sn_item_secondary_activate (SnItem *item,
                            gint x_root,
                            gint y_root);

void
sn_item_scroll (SnItem *item,
                gint delta_x,
                gint delta_y);

G_END_DECLS

#endif /* !__SN_ITEM_H__ */
