/*  central.h
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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

#ifndef __XFCE_CENTRAL_H__
#define __XFCE_CENTRAL_H__

#include "global.h"

/* central panel */
void central_panel_init(GtkBox *box);

void central_panel_set_from_xml(xmlNodePtr node);

void central_panel_write_xml(xmlNodePtr root);

void central_panel_cleanup(void);

/* global settings */
void central_panel_set_size(int size);

void central_panel_set_style(int style);

void central_panel_set_theme(const char *theme);

void central_panel_set_current(int n);

void central_panel_set_num_screens(int n);

void central_panel_move(GtkBox * box, int n);

void central_panel_show(void);

void central_panel_hide(void);

void central_panel_set_show_desktop_buttons(gboolean show);

void central_panel_set_show_minibuttons(gboolean show);

/* Screen buttons */
char *screen_button_get_name(ScreenButton * sb);

void screen_button_set_name(ScreenButton * sb, const char *name);

int screen_button_get_index(ScreenButton * sb);

#endif /* __XFCE_CENTRAL_H__ */
