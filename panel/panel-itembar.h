/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_ITEMBAR_H__
#define __PANEL_ITEMBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_ITEMBAR (panel_itembar_get_type ())
G_DECLARE_FINAL_TYPE (PanelItembar, panel_itembar, PANEL, ITEMBAR, GtkContainer)

GtkWidget *
panel_itembar_new (void) G_GNUC_MALLOC;

void
panel_itembar_insert (PanelItembar *itembar,
                      GtkWidget *widget,
                      gint position);

void
panel_itembar_reorder_child (PanelItembar *itembar,
                             GtkWidget *widget,
                             gint position);

gint
panel_itembar_get_child_index (PanelItembar *itembar,
                               GtkWidget *widget);

guint
panel_itembar_get_n_children (PanelItembar *itembar);

guint
panel_itembar_get_drop_index (PanelItembar *itembar,
                              gint x,
                              gint y);

void
panel_itembar_set_drop_highlight_item (PanelItembar *itembar,
                                       gint idx);

G_END_DECLS

#endif /* !__PANEL_ITEMBAR_H__ */
