/*  xfce.h
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

#ifndef __XFCE_H__
#define __XFCE_H__

#include "global.h"

/* helper functions */
int icon_size(int size);
int popup_size(int size);
int top_height(int size);

void add_tooltip(GtkWidget * widget, char *tip);

/* main program */
void quit(void);
void restart(void);

void xfce_init(void);
void xfce_run(void);

/* panel functions */
void panel_init(void);
void create_xfce_panel(void);
void panel_cleanup(void);

/* panel settings */
void init_settings(void);
void panel_set_settings(void);
void panel_set_position(void);

void panel_set_size(int size);
void panel_set_popup_size(int size);
void panel_set_style(int size);
void panel_set_icon_theme(const char *theme);

void panel_set_num_left(int n);
void panel_set_num_right(int n);

void panel_set_current(int n);
void panel_set_num_screens(int n);
void panel_set_show_central(gboolean show);
void panel_set_show_desktop_buttons(gboolean show);
void panel_set_show_minibuttons(gboolean show);

/* panel configuration */
void panel_parse_xml(xmlNodePtr node);
void panel_write_xml(xmlNodePtr root);

#endif /* __XFCE_H__ */
