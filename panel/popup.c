/*  popup.c
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

#include "popup.h"

#include "xfce.h"
#include "xfce_support.h"
#include "callbacks.h"
#include "icons.h"
#include "settings.h"

/*  Menu items  
 *  ----------
*/
MenuItem *menu_item_new(PanelPopup * pp)
{
    MenuItem *mi = g_new(MenuItem, 1);

    mi->parent = pp;
    mi->pos = 0;

    mi->command = NULL;
    mi->in_terminal = FALSE;
    mi->caption = NULL;
    mi->tooltip = NULL;

    mi->icon_id = UNKNOWN_ICON;
    mi->icon_path = NULL;

    mi->button = NULL;
    mi->hbox = NULL;
    mi->frame = NULL;
    mi->label = NULL;

    mi->pixbuf = NULL;
    mi->image = NULL;

    return mi;
}

void menu_item_read_config(MenuItem * mi, xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar *value;

    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Caption"))
        {
            value = DATA(child);

            if(value)
                mi->caption = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Command"))
        {
            int n = -1;

            value = DATA(child);

            if(value)
                mi->command = (char *)value;

            value = xmlGetProp(child, "term");

            if(value)
                n = atoi(value);

            if(n == 1)
                mi->in_terminal = TRUE;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Tooltip"))
        {
            value = DATA(child);

            if(value)
                mi->tooltip = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Icon"))
        {
            value = xmlGetProp(child, (const xmlChar *)"id");

            if(value)
                mi->icon_id = atoi(value);

            if(!value || mi->icon_id < EXTERN_ICON || mi->icon_id >= NUM_ICONS)
                mi->icon_id = UNKNOWN_ICON;

            g_free(value);

            if(mi->icon_id == EXTERN_ICON)
            {
                value = DATA(child);

                if(value)
                    mi->icon_path = (char *)value;
                else
                    mi->icon_id = UNKNOWN_ICON;
            }
        }
    }
}

void menu_item_write_xml(MenuItem * mi, xmlNodePtr root)
{
    xmlNodePtr node;
    xmlNodePtr child;
    char value[3];

    node = xmlNewTextChild(root, NULL, "Item", NULL);

    xmlNewTextChild(node, NULL, "Caption", mi->caption);

    child = xmlNewChild(node, NULL, "Command", mi->command);

    snprintf(value, 2, "%d", mi->in_terminal);
    xmlSetProp(child, "term", value);

    if(mi->tooltip)
        xmlNewTextChild(node, NULL, "Tooltip", mi->tooltip);

    child = xmlNewTextChild(node, NULL, "Icon", mi->icon_path);

    snprintf(value, 3, "%d", mi->icon_id);
    xmlSetProp(child, "id", value);
}

void menu_item_free(MenuItem * mi)
{
    g_free(mi->command);
    g_free(mi->caption);
    g_free(mi->tooltip);
    g_free(mi->icon_path);

    g_free(mi);
}

void menu_item_set_popup_size(MenuItem * mi, int size)
{
    GdkPixbuf *pb;
    int s = popup_icon_size[size];

    pb = get_scaled_pixbuf(mi->pixbuf, s - border_width);
    gtk_image_set_from_pixbuf(GTK_IMAGE(mi->image), pb);
    g_object_unref(pb);

    gtk_widget_set_size_request(mi->frame, s, s);
    gtk_widget_set_size_request(mi->button, -1, s + border_width);
}

void menu_item_set_style(MenuItem * mi, int style)
{
    if(style == OLD_STYLE)
        gtk_frame_set_shadow_type(GTK_FRAME(mi->frame), GTK_SHADOW_IN);
    else
        gtk_frame_set_shadow_type(GTK_FRAME(mi->frame), GTK_SHADOW_NONE);
}

void menu_item_set_theme(MenuItem * mi, const char *theme)
{
    if(mi->icon_id == EXTERN_ICON)
        return;

    g_object_unref(mi->pixbuf);

    mi->pixbuf = get_pixbuf_by_id(mi->icon_id);
    menu_item_set_popup_size(mi, settings.popup_size);
}

void menu_item_apply_config(MenuItem * mi)
{
    GdkPixbuf *tmp;

    g_object_unref(mi->pixbuf);

    if(mi->icon_id == EXTERN_ICON)
        mi->pixbuf = get_pixbuf_from_file(mi->icon_path);
    else
        mi->pixbuf = get_pixbuf_by_id(mi->icon_id);

    menu_item_set_popup_size(mi, settings.popup_size);

    if(mi->tooltip)
        add_tooltip(mi->button, mi->tooltip);

    gtk_label_set_text(GTK_LABEL(mi->label), mi->caption);
}

void create_addtomenu_item(MenuItem * mi)
{
    mi->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mi->button), GTK_RELIEF_NONE);

    mi->hbox = gtk_hbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(mi->button), mi->hbox);

    mi->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(mi->frame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(mi->hbox), mi->frame, FALSE, FALSE, 0);

    mi->pixbuf = get_system_pixbuf(ADDICON_ICON);
    mi->image = gtk_image_new_from_pixbuf(mi->pixbuf);
    gtk_container_add(GTK_CONTAINER(mi->frame), mi->image);

    add_tooltip(mi->button, _("Add new item"));
    mi->label = gtk_label_new(_("Add icon..."));
    gtk_box_pack_start(GTK_BOX(mi->hbox), mi->label, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(mi->label), 0.1, 0.5);

    /* signals */
    dnd_set_drag_dest(mi->button);

    g_signal_connect(mi->button, "drag_data_received",
                     G_CALLBACK(addtomenu_item_drop_cb), mi->parent);

    g_signal_connect(mi->button, "clicked", G_CALLBACK(addtomenu_item_click_cb),
                     mi->parent);

    menu_item_set_popup_size(mi, settings.popup_size);
    menu_item_set_style(mi, settings.style);
}

void create_menu_item(MenuItem * mi)
{
    GdkPixbuf *pb;

    mi->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mi->button), GTK_RELIEF_NONE);

    if(mi->icon_id == EXTERN_ICON && mi->icon_path)
        mi->pixbuf = get_pixbuf_from_file(mi->icon_path);
    else
        mi->pixbuf = get_pixbuf_by_id(mi->icon_id);

    mi->hbox = gtk_hbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(mi->button), mi->hbox);

    mi->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(mi->frame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(mi->hbox), mi->frame, FALSE, FALSE, 0);

    pb = get_scaled_pixbuf(mi->pixbuf, icon_size[settings.size]);

    mi->image = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_container_add(GTK_CONTAINER(mi->frame), mi->image);

    mi->label = gtk_label_new(mi->caption);
    gtk_box_pack_start(GTK_BOX(mi->hbox), mi->label, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(mi->label), 0.1, 0.5);

    gtk_widget_show_all(mi->button);

    if(mi->tooltip && strlen(mi->tooltip))
        add_tooltip(mi->button, mi->tooltip);
    else if(mi->command && strlen(mi->command))
        add_tooltip(mi->button, mi->command);
    else
        add_tooltip(mi->button, _("Click Mouse 3 to change item"));

    /* signals */
    g_signal_connect(mi->button, "button-press-event",
                     G_CALLBACK(menu_item_press), mi);

    g_signal_connect(mi->button, "clicked", G_CALLBACK(menu_item_click_cb), mi);

    dnd_set_drag_dest(mi->button);

    g_signal_connect(mi->button, "drag_data_received",
                     G_CALLBACK(menu_item_drop_cb), mi);

    menu_item_set_popup_size(mi, settings.popup_size);
    menu_item_set_style(mi, settings.style);
}

/*  Popup menus 
 *  -----------
*/
static void set_popup_window_properties(GtkWidget * win)
{
    GtkWindow *window = GTK_WINDOW(win);
    GdkPixbuf *pb;

    gtk_window_set_decorated(window, FALSE);
    gtk_window_set_resizable(window, FALSE);
    gtk_window_stick(window);
    gtk_window_set_title(window, _("Menu"));
    gtk_window_set_transient_for(window, GTK_WINDOW(toplevel));
    gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_MENU);

    pb = get_system_pixbuf(MENU_ICON);
    gtk_window_set_icon(window, pb);
    g_object_unref(pb);

    if(settings.on_top)
        set_window_type_dock(window, settings.on_top);

    /* don't care about decorations when calculating position */
    gtk_window_set_gravity(window, GDK_GRAVITY_STATIC);
}

PanelPopup *create_panel_popup(void)
{
    PanelPopup *pp = g_new(PanelPopup, 1);
    GtkWidget *sep;

    pp->up = NULL;
    pp->down = NULL;

    /* the button */
    pp->button = gtk_toggle_button_new();
    gtk_button_set_relief(GTK_BUTTON(pp->button), GTK_RELIEF_NONE);

    pp->image = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(pp->button), pp->image);

    gtk_widget_show_all(pp->button);

    /* the menu */
    pp->hgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    pp->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    set_popup_window_properties(pp->window);

    pp->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(pp->frame), GTK_SHADOW_OUT);
    gtk_container_add(GTK_CONTAINER(pp->window), pp->frame);

    pp->vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(pp->frame), pp->vbox);

    pp->addtomenu_item = menu_item_new(pp);
    create_addtomenu_item(pp->addtomenu_item);
    gtk_size_group_add_widget(pp->hgroup, pp->addtomenu_item->image);
    gtk_box_pack_start(GTK_BOX(pp->vbox), pp->addtomenu_item->button, TRUE,
                       TRUE, 0);

    pp->separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(pp->vbox), pp->separator, FALSE, FALSE, 0);

    /* we don't know the items until we read the config file */
    pp->items = NULL;

    pp->tearoff_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(pp->tearoff_button), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(pp->vbox), pp->tearoff_button, FALSE, TRUE, 0);
    sep = gtk_hseparator_new();
    gtk_container_add(GTK_CONTAINER(pp->tearoff_button), sep);

    /* signals */
    g_signal_connect(pp->button, "clicked", G_CALLBACK(toggle_popup), pp);


    g_signal_connect(pp->tearoff_button, "clicked", G_CALLBACK(tearoff_popup),
                     pp);

    g_signal_connect(pp->window, "delete-event", G_CALLBACK(delete_popup), pp);

    /* apparently this is necessary to make the popup show correctly */
    pp->detached = FALSE;

    gtk_widget_show_all(pp->frame);

    panel_popup_set_size(pp, settings.size);
    panel_popup_set_style(pp, settings.style);

    return pp;
}

void panel_popup_pack(PanelPopup * pp, GtkContainer * container)
{
    gtk_container_add(container, pp->button);
}

void panel_popup_unpack(PanelPopup * pp, GtkContainer * container)
{
    gtk_container_remove(container, pp->button);
}

void panel_popup_add_item(PanelPopup * pp, MenuItem * mi)
{
    GList *li;
    int i;

    gtk_size_group_add_widget(pp->hgroup, mi->image);
    gtk_box_pack_start(GTK_BOX(pp->vbox), mi->button, TRUE, TRUE, 0);
    gtk_box_reorder_child(GTK_BOX(pp->vbox), mi->button, mi->pos + 2);

    pp->items = g_list_insert(pp->items, mi, mi->pos);

    for(i = 0, li = pp->items; li && li->data; i++, li = li->next)
    {
        MenuItem *item = (MenuItem *) li->data;

        mi->pos = i;
    }
}

void panel_popup_remove_item(PanelPopup * pp, MenuItem * mi)
{
    GList *li;
    int i;

    gtk_container_remove(GTK_CONTAINER(pp->vbox), mi->button);

    pp->items = g_list_remove(pp->items, mi);

    menu_item_free(mi);

    for(i = 0, li = pp->items; li && li->data; i++, li = li->next)
    {
        MenuItem *item = (MenuItem *) li->data;

        mi->pos = i;
    }
}

void panel_popup_set_from_xml(PanelPopup * pp, xmlNodePtr node)
{
    xmlNodePtr child;
    int i = 0;

    for(child = node->children; child; child = child->next)
    {
        MenuItem *mi;

        if(!xmlStrEqual(child->name, (const xmlChar *)"Item"))
            continue;

        mi = menu_item_new(pp);
        menu_item_read_config(mi, child);
        create_menu_item(mi);

        mi->pos = i;

        panel_popup_add_item(pp, mi);

        i++;
    }
}

void panel_popup_write_xml(PanelPopup * pp, xmlNodePtr root)
{
    xmlNodePtr node;
    GList *li;

    node = xmlNewTextChild(root, NULL, "Popup", NULL);

    for(li = pp->items; li; li = li->next)
    {
        MenuItem *mi = (MenuItem *) li->data;

        menu_item_write_xml(mi, node);
    }
}

void panel_popup_free(PanelPopup * pp)
{
    /* only items contain non-gtk elements to be freed */
    GList *li;
    for(li = pp->items; li && li->data; li = li->next)
    {
        MenuItem *mi = (MenuItem *) li->data;
        menu_item_free(mi);
    }

    g_free(pp);
}

void panel_popup_set_size(PanelPopup * pp, int size)
{
    int pos = settings.popup_position;
    GdkPixbuf *pb;
    int w, h, s;

    w = icon_size[size] + border_width;
    h = top_height[size];

    if(pp->up)
        g_object_unref(pp->up);
    pb = get_system_pixbuf(UP_ICON);
    pp->up = get_scaled_pixbuf(pb, h - border_width);
    g_object_unref(pb);

    if(pp->down)
        g_object_unref(pp->down);
    pb = get_system_pixbuf(DOWN_ICON);
    pp->down = get_scaled_pixbuf(pb, h - border_width);
    g_object_unref(pb);

    if(!pp->detached)
        gtk_image_set_from_pixbuf(GTK_IMAGE(pp->image), pp->up);
    else
        gtk_image_set_from_pixbuf(GTK_IMAGE(pp->image), pp->down);

    if(pos == LEFT || pos == RIGHT)
        gtk_widget_set_size_request(pp->button, h, w);
    else
        gtk_widget_set_size_request(pp->button, w, h);
}

void panel_popup_set_style(PanelPopup * pp, int style)
{
    GList *li;
    if(style == OLD_STYLE)
    {
        gtk_button_set_relief(GTK_BUTTON(pp->button), GTK_RELIEF_NORMAL);
        gtk_widget_set_name(pp->button, "gxfce_color1");
    }
    else
    {
        gtk_button_set_relief(GTK_BUTTON(pp->button), GTK_RELIEF_NONE);
        gtk_widget_set_name(pp->button, "gxfce_popup_button");
    }

    menu_item_set_style(pp->addtomenu_item, style);
    
    for(li = pp->items; li && li->data; li = li->next)
    {
        MenuItem *mi = (MenuItem *) li->data;
        menu_item_set_style(mi, style);
    }
}

void panel_popup_set_popup_position(PanelPopup * pp, int position)
{
    settings.popup_position = position;
    panel_popup_set_size(pp, settings.size);
}

void panel_popup_set_on_top(PanelPopup * pp, gboolean on_top)
{
    set_window_type_dock(pp->window, on_top);
}

void panel_popup_set_theme(PanelPopup * pp, const char *theme)
{
    GList *li;

    for(li = pp->items; li && li->data; li = li->next)
    {
        MenuItem *mi = (MenuItem *) li->data;
        menu_item_set_theme(mi, theme);
    }
}

void panel_popup_set_popup_size(PanelPopup * pp, int size)
{
    GList *li;

    menu_item_set_popup_size(pp->addtomenu_item, size);
    
    for(li = pp->items; li && li->data; li = li->next)
    {
        MenuItem *mi = (MenuItem *) li->data;
        menu_item_set_popup_size(mi, size);
    }
}
