/*  item.h
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

#ifndef __XFCE_ITEM_H__
#define __XFCE_ITEM_H__

#include "global.h"

/*  The panel item stuff is only public for use in item_dialog.c, so that
 *  the dialog can provide immediate apply and revert functionality.
 *
 *  The dialog is in a separate file because it is shared with menu items.
*/
struct _PanelItem
{
    char *command;
    gboolean in_terminal;
    char *tooltip;

    int icon_id;
    char *icon_path;            /* if id==EXTERN_ICON */

    GtkWidget *button;
};

void panel_item_apply_config(PanelItem * pi);

/*  panel control interface
*/
void create_panel_item(Control * control);
void panel_item_class_init(ControlClass *cc);

#endif /* __XFCE_ITEM_H__ */
