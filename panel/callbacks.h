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

/* global */
gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, gpointer data);

gboolean panel_destroy_cb(GtkWidget * window, GdkEvent * ev, gpointer data);

gboolean main_frame_destroy_cb(GtkWidget * frame, GdkEvent * ev, gpointer data);

void iconify_cb(void);

/* central panel */
void screen_button_click(GtkWidget * b, ScreenButton * sb);

gboolean screen_button_pressed_cb(GtkButton * b, GdkEventButton * ev,
                                  ScreenButton * sb);

void mini_lock_cb(void);
void mini_info_cb(void);
void mini_palet_cb(void);

void mini_power_cb(GtkButton * b, GdkEventButton * ev, gpointer data);

/* side panel */
gboolean panel_control_press_cb(GtkButton * b, GdkEventButton * ev,
                                PanelControl * pc);

void toggle_popup(GtkWidget * button, PanelPopup * pp);

void tearoff_popup(GtkWidget * button, PanelPopup * pp);

gboolean delete_popup(GtkWidget * window, GdkEvent * ev, PanelPopup * pp);

/* panel items */
void panel_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                        gint x, gint y, GtkSelectionData * data,
                        guint info, guint time, PanelItem * pi);

void panel_item_click_cb(GtkButton * b, PanelItem * pi);

/* menu items */
void addtomenu_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                            gint x, gint y, GtkSelectionData * data,
                            guint info, guint time, PanelPopup * pp);

void addtomenu_item_click_cb(GtkButton * b, PanelPopup * pp);

gboolean menu_item_press(GtkButton * b, GdkEventButton * ev, MenuItem * mi);

void menu_item_drop_cb(GtkWidget * widget, GdkDragContext * context,
                       gint x, gint y, GtkSelectionData * data,
                       guint info, guint time, MenuItem * mi);

void menu_item_click_cb(GtkButton * b, MenuItem * mi);

#endif /* __XFCE_CALLBACKS_H__ */

