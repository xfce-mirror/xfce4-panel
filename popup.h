/*  popup.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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
    GdkPixbuf *up;
    GdkPixbuf *down;
    GtkWidget *image;

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

    GtkWidget *frame;
    GdkPixbuf *pixbuf;
    GtkWidget *image;

    GtkWidget *label;
};

/*****************************************************************************/

PanelPopup *panel_popup_new(void);
void add_panel_popup(PanelPopup * pp, GtkBox * box);
void panel_popup_free(PanelPopup * pp);

void panel_popup_set_size(PanelPopup * pp, int size);
void panel_popup_set_popup_size(PanelPopup * pp, int size);
void panel_popup_set_style(PanelPopup * pp, int size);
void panel_popup_set_icon_theme(PanelPopup * pp, const char *theme);

void hide_current_popup_menu(void);

void panel_popup_parse_xml(xmlNodePtr node, PanelPopup * pp);
void panel_popup_write_xml(xmlNodePtr root, PanelPopup *pp);

/*****************************************************************************/

MenuItem *menu_item_new(PanelPopup * pp);

void create_addtomenu_item(MenuItem * mi);
void create_menu_item(MenuItem * mi);
void menu_item_free(MenuItem * mi);

void menu_item_set_popup_size(MenuItem * mi, int size);
void menu_item_set_style(MenuItem * mi, int style);
void menu_item_set_icon_theme(MenuItem * mi, const char *theme);

void menu_item_parse_xml(xmlNodePtr node, MenuItem * mi);
void menu_item_write_xml(xmlNodePtr root, MenuItem *mi);

#endif /* __XFCE_POPUP_H__ */
