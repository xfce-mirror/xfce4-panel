/*  item.c
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

#include "xfce.h"

#include "item.h"
#include "panel.h"
#include "dnd.h"
#include "callbacks.h"
#include "popup.h"
#include "icons.h"

PanelItem *panel_item_new(PanelGroup * pg)
{
    PanelItem *pi = g_new(PanelItem, 1);

    pi->parent = pg;

    pi->command = NULL;
    pi->tooltip = NULL;

    pi->id = UNKNOWN_ICON;
    pi->path = NULL;

    pi->button = NULL;
    pi->pb = NULL;
    pi->image = NULL;

    return pi;
}

PanelItem *panel_item_unknown_new(PanelGroup * pg)
{
    PanelItem *pi = panel_item_new(pg);

    pi->tooltip = g_strdup(_("Click Mouse 3 to change item"));

    return pi;
}

void create_panel_item(PanelItem * pi)
{
    GdkPixbuf *pb;

    pi->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(pi->button), GTK_RELIEF_NONE);

    if(pi->id == EXTERN_ICON && pi->path)
        pi->pb = gdk_pixbuf_new_from_file(pi->path, NULL);
    else
        pi->pb = get_pixbuf_from_id(pi->id);

    gtk_container_set_border_width(GTK_CONTAINER(pi->button), 2);
    pb = gdk_pixbuf_scale_simple(pi->pb,
                                 MEDIUM_PANEL_ICONS - 4,
                                 MEDIUM_PANEL_ICONS - 4, GDK_INTERP_BILINEAR);

    pi->image = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_container_add(GTK_CONTAINER(pi->button), pi->image);

    gtk_widget_show_all(pi->button);

    if(pi->command && strlen(pi->command))
    {
        g_signal_connect(pi->button, "clicked",
                         G_CALLBACK(item_click_cb), pi->command);

        if(pi->tooltip && strlen(pi->tooltip))
            add_tooltip(pi->button, pi->tooltip);
        else
            add_tooltip(pi->button, pi->command);

        dnd_set_drag_dest(pi->button);

        g_signal_connect(pi->button, "drag_data_received",
                         G_CALLBACK(item_drop_cb), pi->command);
    }
    else
        add_tooltip(pi->button, _("Click Mouse 3 to change item"));

    g_signal_connect(pi->button, "button-press-event",
                     G_CALLBACK(panel_group_press_cb), pi->parent);
}

void panel_item_pack(PanelItem * pi, GtkBox * box)
{
    gtk_box_pack_start(box, pi->button, FALSE, FALSE, 0);
}

void panel_item_unpack(PanelItem * pi, GtkContainer * container)
{
    gtk_container_remove(container, pi->button);
}

void panel_item_free(PanelItem * pi)
{
    g_free(pi->command);
    g_free(pi->path);
    g_free(pi->tooltip);

    g_free(pi);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_item_set_size(PanelItem * pi, int size)
{
    GdkPixbuf *pb;
    int width, height, bw;

    bw = size == SMALL ? 0 : 2;
    width = icon_size(size);
    height = icon_size(size);

    gtk_container_set_border_width(GTK_CONTAINER(pi->button), bw);
    gtk_widget_set_size_request(pi->button, width + 4, height + 4);

    pb = gdk_pixbuf_scale_simple(pi->pb,
                                 width - 2 * bw,
                                 height - 2 * bw, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(pi->image), pb);
    g_object_unref(pb);
}

void panel_item_set_style(PanelItem * pi, int style)
{
}

void panel_item_set_icon_theme(PanelItem * pi, const char *theme)
{
    if(pi->id == EXTERN_ICON)
        return;

    g_object_unref(pi->pb);

    pi->pb = get_themed_pixbuf_from_id(pi->id, theme);
    panel_item_set_size(pi, settings.size);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_item_parse_xml(xmlNodePtr node, PanelItem * pi)
{
    xmlNodePtr child;
    xmlChar *value;

    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Command"))
        {
            value = DATA(child);

            if(value)
                pi->command = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Tooltip"))
        {
            value = DATA(child);

            if(value)
                pi->tooltip = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Icon"))
        {
            value = xmlGetProp(child, (const xmlChar *)"id");

            if(value)
                pi->id = atoi(value);

            if(!value || pi->id < EXTERN_ICON || pi->id >= NUM_ICONS)
                pi->id = UNKNOWN_ICON;

            g_free(value);

            if(pi->id == EXTERN_ICON)
            {
                value = DATA(child);

                if(value)
                    pi->path = (char *)value;
                else
                    pi->id = UNKNOWN_ICON;
            }
        }
    }
}

void panel_item_write_xml(xmlNodePtr root, PanelItem *pi)
{
    xmlNodePtr node;
    xmlNodePtr child;
    char value[3];
    
    node = xmlNewTextChild(root, NULL, "Item", NULL);
    
    xmlNewTextChild(node, NULL, "Command", pi->command);
    
    if (pi->tooltip)
	xmlNewTextChild(node, NULL, "Tooltip", pi->tooltip);
    
    child = xmlNewTextChild(node, NULL, "Icon", pi->path);
    
    snprintf(value, 3, "%d", pi->id);
    xmlSetProp(child, "id", value);
}