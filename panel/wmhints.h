/*  wmhints.h
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

#ifndef __XFCE_WMHINTS_H__
#define __XFCE_WMHINTS_H__

void check_net_support(void);

void watch_root_properties(void);

void set_window_type_dock(GtkWidget * window, gboolean set);

void request_net_current_desktop(int n);
void request_net_number_of_desktops(int n);

int get_net_current_desktop(void);
int get_net_number_of_desktops(void);

#endif /* __XFCE_WMHINTS_H__ */
