/*  iconbutton.h
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

#ifndef __XFCE_ICON_BUTTON_H__
#define __XFCE_ICON_BUTTON_H__

IconButton *icon_button_new(GdkPixbuf * pb);

GtkWidget *icon_button_get_button(IconButton * b);

void icon_button_set_size(IconButton * b, int size);

void icon_button_set_pixbuf(IconButton * b, GdkPixbuf * pb);

void icon_button_set_command(IconButton * b, const char *cmd,
                             gboolean use_terminal);

void icon_button_free(IconButton * b);

#endif /* __XFCE_ICON_BUTTON_H__ */

