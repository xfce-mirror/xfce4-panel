/*  callback.h
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

#ifndef __XFCE_CALLBACKS_H__
#define __XFCE_CALLBACKS_H__

#include "global.h"

void iconify_cb(void);
void close_cb(void);

void toggle_popup(GtkWidget * button, PanelPopup * pp);

void tearoff_popup(GtkWidget * button, PanelPopup * pp);

gboolean delete_popup(GtkWidget * window, GdkEvent * ev, PanelPopup * pp);

gboolean popup_key_pressed(GtkWidget * window, GdkEventKey * ev, 
			   PanelPopup * pp);

#endif /* __XFCE_CALLBACKS_H__ */
