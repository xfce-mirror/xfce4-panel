/*  side.h
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

#ifndef __XFCE_SIDE_H__
#define __XFCE_SIDE_H__

#include "global.h"

/* init */
void side_panel_init(int side, GtkBox * box);

void side_panel_pack(int side, GtkBox * box);

void side_panel_unpack(int side);

/* just points the proper array element to this panel control */
void side_panel_register_control(PanelControl * pc);

void side_panel_set_from_xml(int side, xmlNodePtr node);

/* exit */
void side_panel_write_xml(int side, xmlNodePtr root);

void side_panel_cleanup(int side);

/* settings */
void side_panel_set_size(int side, int size);

void side_panel_set_popup_size(int side, int size);

void side_panel_set_popup_position(int side, int position);

void side_panel_set_on_top(int side, gboolean on_top);

void side_panel_set_style(int side, int style);

void side_panel_set_theme(int side, const char *theme);

void side_panel_set_num_groups(int side, int n);

#endif /* __XFCE_SIDE_H__ */
