/*  xfce.h
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

#ifndef __XFCE_H__
#define __XFCE_H__

#include "global.h"

/* main program */
void quit(gboolean force);
void restart(void);

void xfce_init(void);
void xfce_run(void);

/* panel functions */
void panel_init(void);
void panel_cleanup(void);
void panel_reorient(void);

/* apply panel settings */
void panel_set_size(int size);
void panel_set_popup_position(int position);
void panel_set_orientation(int orientation);
void panel_set_on_top(gboolean on_top);

void panel_set_style(int size);
void panel_set_theme(const char *theme);
void panel_set_on_top(gboolean on_top);

void panel_set_num_left(int n);
void panel_set_num_right(int n);
void panel_set_num_screens(int n);
void panel_set_show_central(gboolean show);
void panel_set_show_desktop_buttons(gboolean show);
void panel_set_show_minibuttons(gboolean show);

/* global prefs */
void init_settings(void);
void panel_set_settings(void);
void panel_set_position(void);

void panel_parse_xml(xmlNodePtr node);
void panel_write_xml(xmlNodePtr root);

#endif /* __XFCE_H__ */
