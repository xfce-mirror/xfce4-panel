/*  item_dialog.h
 *
 *  Copyright (C) Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#ifndef __XFCE_ITEM_DIALOG_H__
#define __XFCE_ITEM_DIALOG_H__

#include <panel/item.h>

void edit_menu_item_dialog (Item * mi);

void add_menu_item_dialog (PanelPopup * pp);

/* options box for panel control dialog */
void panel_item_add_options (Control * control, GtkContainer * container,
			     GtkWidget * done);

extern GtkWidget *create_icon_option_menu (void);
extern GtkWidget *create_icon_option (GtkSizeGroup *);
extern GtkWidget *create_command_option (GtkSizeGroup *);
extern GtkWidget *create_caption_option (GtkSizeGroup *);
extern GtkWidget *create_tooltip_option (GtkSizeGroup *);
extern GtkWidget *create_position_option (void);
extern GtkWidget *create_item_options_box (void);
extern GtkWidget *create_icon_preview_frame (void);
extern GtkWidget *create_menu_item_dialog (Item *);

#endif /* __XFCE_ITEM_DIALOG_H__ */
