/*  popup.h
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

#ifndef __XFCE_POPUP_H__
#define __XFCE_POPUP_H__

#include "global.h"

struct _PanelPopup
{
    /* button */
    GtkWidget *button;

    /* menu */
    gboolean detached;

    GtkWidget *window;
    GtkWidget *frame;
    GtkWidget *vbox;

    GtkSizeGroup *hgroup;

    MenuItem *addtomenu_item;
    GtkWidget *separator;
    GtkWidget *tearoff_button;

    GList *items;               /* type MenuItem */
};

struct _MenuItem
{
    char *command;
    gboolean in_terminal;
    char *caption;
    char *tooltip;

    int icon_id;
    char *icon_path;

    PanelPopup *parent;
    int pos;

    GtkWidget *button;
    GtkWidget *hbox;

    GdkPixbuf *pixbuf;
    GtkWidget *image;

    GtkWidget *label;
};

/* Panel popups */

PanelPopup *create_panel_popup(void);
void panel_popup_free(PanelPopup * pp);

void panel_popup_pack(PanelPopup * pp, GtkContainer * container);
void panel_popup_unpack(PanelPopup * pp, GtkContainer * container);

void panel_popup_add_item(PanelPopup * pp, MenuItem * mi);
void panel_popup_remove_item(PanelPopup * pp, MenuItem * mi);

void panel_popup_set_size(PanelPopup * pp, int size);
void panel_popup_set_popup_position(PanelPopup * pp, int position);
void panel_popup_set_on_top(PanelPopup * pp, gboolean on_top);
void panel_popup_set_style(PanelPopup * pp, int size);
void panel_popup_set_theme(PanelPopup * pp, const char *theme);

void hide_current_popup_menu(void);

void panel_popup_set_from_xml(PanelPopup * pp, xmlNodePtr node);
void panel_popup_write_xml(PanelPopup * pp, xmlNodePtr root);

/* Menu items */
MenuItem *menu_item_new(PanelPopup * pp);

void create_menu_item(MenuItem * mi);

void menu_item_free(MenuItem * mi);

void menu_item_apply_config(MenuItem * mi);

void panel_popup_add_item(PanelPopup * pp, MenuItem * mi);

void panel_popup_remove_item(PanelPopup * pp, MenuItem * mi);

#endif /* __XFCE_POPUP_H__ */
