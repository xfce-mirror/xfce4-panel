/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#include <gmodule.h>
#include <panel/global.h>
#include <libxml/tree.h>

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
G_MODULE_IMPORT void create_addtomenu_item (Item * mi);

/* menu items */
G_MODULE_IMPORT Item *menu_item_new (PanelPopup * pp);

G_MODULE_IMPORT void create_menu_item (Item * mi);

G_MODULE_IMPORT void menu_item_set_popup_size (Item * item, int size);

/* common functions */
G_MODULE_IMPORT void item_free (Item * item);

G_MODULE_IMPORT void item_set_theme (Item * item, const char *theme);

G_MODULE_IMPORT void item_apply_config (Item * item);

G_MODULE_IMPORT void item_read_config (Item * item, xmlNodePtr node);

G_MODULE_IMPORT void item_write_config (Item * item, xmlNodePtr node);

/* panel item */
Item *panel_item_new (void);

#endif /* __XFCE_ITEMS_H */
