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

#ifndef _XFCE_PANEL_ITEM_CONTROL_H
#define _XFCE_PANEL_ITEM_CONTROL_H

#include <gmodule.h>
#include <panel/global.h>
#include <libxml/tree.h>

typedef struct _ItemControl ItemControl;

struct _ItemControl
{
    GtkWidget *base;		/* container to pack into panel */
    GtkWidget *box;

    Item *item;
    PanelPopup *popup;
};

G_MODULE_IMPORT void item_control_add_popup_from_xml (Control *control, 
                                                      xmlNodePtr node);

G_MODULE_IMPORT void item_control_set_popup_position (Control *control, 
                                                      int position);

G_MODULE_IMPORT void item_control_set_arrow_direction (Control *control, 
                                                       GtkArrowType type);

G_MODULE_IMPORT PanelPopup *item_control_get_popup (Control *control);

G_MODULE_IMPORT void item_control_show_popup (Control *control, int show);

G_MODULE_IMPORT void item_control_write_popup_xml (Control *control, 
                                                   xmlNodePtr node);

/*  panel control interface for panel items */
G_MODULE_IMPORT void create_item_control (Control * control);

G_MODULE_IMPORT void item_control_class_init (ControlClass * cc);


#endif /* _XFCE_PANEL_ITEM_CONTROL_H */

