/*  central.h
 *
 *  Copyright (C) 2002 Jasper Huijsmans <j.b.huijsmans@hetnet.nl>
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

struct _ScreenButton
{
    int index;
    char *name;

    int callback_id;

    GtkWidget *frame;
    GtkWidget *button;
    GtkWidget *label;
};

/* central panel */
void central_panel_init(void);
void add_central_panel(GtkBox * box);
void central_panel_cleanup(void);

/* global settings */
void central_panel_set_size(int size);
void central_panel_set_style(int style);
void central_panel_set_icon_theme(const char *theme);

void central_panel_set_current(int n);

void central_panel_set_num_screens(int n);

/* screen buttons */
ScreenButton *screen_button_new(int index);
void add_screen_button(ScreenButton * sb, GtkWidget * table);
void screen_button_free(ScreenButton * sb);

/* settings */
void screen_button_set_size(ScreenButton * sb, int size);
void screen_button_set_style(ScreenButton * sb, int style);

/* central panel configuration */
void central_panel_parse_xml(xmlNodePtr node);
void central_panel_write_xml(xmlNodePtr root);

#endif /* __XFCE_CENTRAL_H__ */
