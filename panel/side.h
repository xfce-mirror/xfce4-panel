/*  side.h
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

#ifndef __XFCE_SIDE_H__
#define __XFCE_SIDE_H__

#include "global.h"

struct _PanelGroup
{
    int side;
    int index;
    int size;

    GtkWidget *container;       /* main container */

    GtkWidget *box;             /* hbox or vbox depending on size */

    MoveHandle *handle;
    PanelPopup *popup;

    int type;
    PanelItem *item;
    PanelModule *module;
};

/* side panel */
void side_panel_init(int side);
void add_side_panel(int side, GtkBox * hbox);
void side_panel_cleanup(int side);

void side_panel_set_size(int side, int size);
void side_panel_set_popup_size(int side, int size);
void side_panel_set_style(int side, int style);
void side_panel_set_icon_theme(int side, const char *theme);
void side_panel_set_num_groups(int side, int n);

void side_panel_parse_xml(xmlNodePtr node, int side);
void side_panel_write_xml(xmlNodePtr root, int side);

/*****************************************************************************/
/* panel groups */

void panel_group_init(PanelGroup * pg, int side, int index);
void add_panel_group(PanelGroup * pg, GtkBox * hbox);
void panel_group_cleanup(PanelGroup * pg);

void panel_group_set_size(PanelGroup * pg, int size);
void panel_group_set_popup_size(PanelGroup * pg, int size);
void panel_group_set_style(PanelGroup * pg, int style);
void panel_group_set_icon_theme(PanelGroup * pg, const char *theme);

void panel_group_parse_xml(xmlNodePtr node, PanelGroup * pg);
void panel_group_write_xml(xmlNodePtr root, PanelGroup * pg);

/*****************************************************************************/
/* move handle */

MoveHandle *move_handle_new(int side);
void add_move_handle(MoveHandle * mh, GtkWidget * vbox);
void move_handle_free(MoveHandle * mh);

void move_handle_set_size(MoveHandle * mh, int size);
void move_handle_set_style(MoveHandle * mh, int style);

#endif /* __XFCE_SIDE_H__ */
