/*  item.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

struct _PanelItem
{
    PanelGroup *parent;

    char *command;
    char *tooltip;

    int id;
    char *path;                 /* if id==EXTERN_ICON */

    GtkWidget *button;
    GdkPixbuf *pb;
    GtkWidget *image;
};

/*****************************************************************************/

PanelItem *panel_item_new(PanelGroup * pg);
PanelItem *panel_item_unknown_new(PanelGroup * pg);
void create_panel_item(PanelItem * pi);
void panel_item_pack(PanelItem * pi, GtkBox * box);
void panel_item_unpack(PanelItem * pi, GtkContainer * container);
void panel_item_free(PanelItem * pi);

void panel_item_set_size(PanelItem * pi, int size);
void panel_item_set_style(PanelItem * pi, int style);
void panel_item_set_icon_theme(PanelItem * pi, const char *theme);

void panel_item_parse_xml(xmlNodePtr node, PanelItem * pi);
void panel_item_write_xml(xmlNodePtr root, PanelItem *pi);

#endif /* __XFCE_ITEM_H__ */
