/*  xfce4
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __XFCE_ITEMS_H
#define __XFCE_ITEMS_H

#include <panel/global.h>

enum
{ PANELITEM, MENUITEM };

struct _Item
{
    char *command;
    gboolean in_terminal;
    gboolean use_sn;
    char *caption;
    char *tooltip;

    int icon_id;
    char *icon_path;

    int type;
    GtkWidget *button;

    /* for menu items */
    PanelPopup *parent;
    int pos;

    /* for panel launchers */
    gboolean with_popup;
};

/* special menu item */
void create_addtomenu_item (Item * mi);

/* menu items */
Item *menu_item_new (PanelPopup * pp);

void create_menu_item (Item * mi);

void menu_item_set_popup_size (Item * item, int size);

/*  panel control interface for panel items */
void create_panel_item (Control * control);

void panel_item_class_init (ControlClass * cc);

/* common functions */
void item_free (Item * item);

void item_set_theme (Item * item, const char *theme);

void item_apply_config (Item * item);

void item_read_config (Item * item, xmlNodePtr node);

void item_write_config (Item * item, xmlNodePtr node);

#endif /* __XFCE_ITEMS_H */
