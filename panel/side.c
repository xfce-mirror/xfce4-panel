/*  side.c
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

#include "side.h"

#include "xfce.h"
#include "popup.h"
#include "item.h"
#include "module.h"
#include "callbacks.h"
#include "move.h"

PanelGroup left_groups[NBGROUPS];
PanelGroup right_groups[NBGROUPS];

struct _MoveHandle
{
    int side;

    GtkWidget *container;
    GtkWidget *box;

    GtkWidget *button;
    GtkWidget *iconify_im;

    GtkWidget *frame;
    GtkWidget *eventbox;
    GtkWidget *handle_im;
};

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void side_panel_init(int side)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_init(pg, side, i);
    }
}

void add_side_panel(int side, GtkBox * hbox)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        add_panel_group(pg, hbox);
    }
}

void side_panel_cleanup(int side)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_cleanup(pg);
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void side_panel_set_size(int side, int size)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_set_size(pg, size);
    }
}

void side_panel_set_popup_size(int side, int size)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_set_popup_size(pg, size);
    }
}

void side_panel_set_style(int side, int style)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_set_style(pg, style);
    }
}

void side_panel_set_icon_theme(int side, const char *theme)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_set_icon_theme(pg, theme);
    }
}

void side_panel_set_num_groups(int side, int n)
{
    int i;
    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        if(i < n)
            gtk_widget_show(pg->container);
        else
            gtk_widget_hide(pg->container);
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void side_panel_parse_xml(xmlNodePtr node, int side)
{
    xmlNodePtr child;
    int i;

    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    child = node->children;

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        if(!child || !xmlStrEqual(child->name, (const xmlChar *)"Group"))
        {
            pg->type = ICON;
            pg->item = panel_item_unknown_new(pg);
        }
        else
            panel_group_parse_xml(child, pg);

        if(child)
            child = child->next;
    }
}

void side_panel_write_xml(xmlNodePtr root, int side)
{
    xmlNodePtr node;
    int i;

    PanelGroup *pg = (side == LEFT) ? left_groups : right_groups;

    if(side == LEFT)
        node = xmlNewTextChild(root, NULL, "Left", NULL);
    else
        node = xmlNewTextChild(root, NULL, "Right", NULL);

    for(i = 0; i < NBGROUPS; i++, pg++)
    {
        panel_group_write_xml(node, pg);
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* panel groups */

void panel_group_init(PanelGroup * pg, int side, int index)
{
    pg->side = side;
    pg->index = index;
    pg->size = MEDIUM;

    pg->container = NULL;
    pg->box = NULL;

    if(pg->index == 0)
        pg->handle = move_handle_new(side);
    else
        pg->popup = panel_popup_new();

    pg->type = ICON;
    pg->item = NULL;
    pg->module = NULL;
};

void add_panel_group(PanelGroup * pg, GtkBox * hbox)
{
    pg->container = gtk_alignment_new(0,0,1,1);
    gtk_widget_show(pg->container);

    pg->box = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(pg->box);
    gtk_container_add(GTK_CONTAINER(pg->container), pg->box);

    if(pg->index == 0)
        add_move_handle(pg->handle, pg->box);
    else
        add_panel_popup(pg->popup, GTK_BOX(pg->box));

    if(pg->type == ICON)
    {
        create_panel_item(pg->item);
        panel_item_pack(pg->item, GTK_BOX(pg->box));
    }
    else if(create_panel_module(pg->module))
    {
        panel_module_pack(pg->module, GTK_BOX(pg->box));
    }
    else
    {
        panel_module_free(pg->module);

        pg->type = ICON;
        pg->item = panel_item_unknown_new(pg);
        create_panel_item(pg->item);
        panel_item_pack(pg->item, GTK_BOX(pg->box));
    }

    if(pg->side == LEFT)
        gtk_box_pack_start(hbox, pg->container, TRUE, TRUE, 0);
    else
        gtk_box_pack_end(hbox, pg->container, TRUE, TRUE, 0);
}

void panel_group_cleanup(PanelGroup * pg)
{
    if(pg->index == 0)
        move_handle_free(pg->handle);
    else
        panel_popup_free(pg->popup);

    if(pg->type == ICON)
        panel_item_free(pg->item);
    else
    {
        panel_module_stop(pg->module);
        panel_module_free(pg->module);
    }
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void panel_group_set_size(PanelGroup * pg, int size)
{
    GtkWidget *newbox;

    /* obtain a reference to the widget we will be reordering
     * to prevent them from being destroyed when removing them
     * from their container
     */
    if(size == TINY)
        newbox = gtk_hbox_new(FALSE, 0);
    else
        newbox = gtk_vbox_new(FALSE, 0);

    gtk_widget_show(newbox);

    /* remove widgets from box */
    if(pg->index == 0)
        gtk_container_remove(GTK_CONTAINER(pg->box), pg->handle->container);
    else
        gtk_container_remove(GTK_CONTAINER(pg->box), pg->popup->button);

    if(pg->type == ICON)
        gtk_container_remove(GTK_CONTAINER(pg->box), pg->item->button);
    else
        panel_module_unpack(pg->module, GTK_CONTAINER(pg->box));

    gtk_container_remove(GTK_CONTAINER(pg->container), pg->box);
    /*gtk_widget_destroy(pg->box); */

    pg->box = newbox;
    gtk_container_add(GTK_CONTAINER(pg->container), newbox);

    if(size == TINY)
    {
        if(pg->index == 0 && pg->side == LEFT)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->handle->container, TRUE, TRUE, 0);
            move_handle_set_size(pg->handle, size);
        }

        if(pg->type == ICON)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->item->button, TRUE, TRUE, 0);
            panel_item_set_size(pg->item, size);
        }
        else
        {
            panel_module_pack(pg->module, GTK_BOX(pg->box));
            panel_module_set_size(pg->module, size);
        }

        if(pg->index != 0)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->popup->button, TRUE, TRUE, 0);
            panel_popup_set_size(pg->popup, size);
        }
        else if(pg->side == RIGHT)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->handle->container, TRUE, TRUE, 0);
            move_handle_set_size(pg->handle, size);
        }
    }
    else
    {
        if(pg->index == 0)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->handle->container, TRUE, TRUE, 0);
            move_handle_set_size(pg->handle, size);
        }
        else
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->popup->button, TRUE, TRUE, 0);
            panel_popup_set_size(pg->popup, size);
        }

        if(pg->type == ICON)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->item->button, TRUE, TRUE, 0);
            panel_item_set_size(pg->item, size);
        }
        else
        {
            panel_module_pack(pg->module, GTK_BOX(pg->box));
            panel_module_set_size(pg->module, size);
        }
    }
}

void panel_group_set_popup_size(PanelGroup * pg, int size)
{
    if(pg->index != 0)
        panel_popup_set_popup_size(pg->popup, size);
}

void panel_group_set_style(PanelGroup * pg, int style)
{
    if(pg->index == 0)
        move_handle_set_style(pg->handle, style);
    else
        panel_popup_set_style(pg->popup, style);

    if(pg->type == ICON)
        panel_item_set_style(pg->item, style);
    else
        panel_module_set_style(pg->module, style);
}

void panel_group_set_icon_theme(PanelGroup * pg, const char *theme)
{
    if(pg->index != 0)
        panel_popup_set_icon_theme(pg->popup, theme);

    if(pg->type == ICON)
        panel_item_set_icon_theme(pg->item, theme);
    else
        panel_module_set_icon_theme(pg->module, theme);
}

void panel_group_parse_xml(xmlNodePtr node, PanelGroup * pg)
{
    xmlNodePtr child;

    for(child = node->children; child; child = child->next)
    {
        if(child && xmlStrEqual(child->name, (const xmlChar *)"Popup"))
        {
            panel_popup_parse_xml(child, pg->popup);
        }
        else if(child && xmlStrEqual(child->name, (const xmlChar *)"Item"))
        {
            pg->type = ICON;
            pg->item = panel_item_new(pg);

            panel_item_parse_xml(child, pg->item);
        }
        else if(child && xmlStrEqual(child->name, (const xmlChar *)"Module"))
        {
            pg->type = MODULE;
            pg->module = panel_module_new(pg);

            panel_module_parse_xml(child, pg->module);
        }
    }
}

void panel_group_write_xml(xmlNodePtr root, PanelGroup * pg)
{
    xmlNodePtr node;

    node = xmlNewTextChild(root, NULL, "Group", NULL);

    if(pg->index != 0)
        panel_popup_write_xml(node, pg->popup);

    if(pg->type == ICON)
        panel_item_write_xml(node, pg->item);
    else
        panel_module_write_xml(node, pg->module);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* move handles */

MoveHandle *move_handle_new(int side)
{
    MoveHandle *mh = g_new(MoveHandle, 1);

    mh->side = side;

    mh->container = NULL;
    mh->box = NULL;

    mh->button = NULL;
    mh->iconify_im = NULL;

    mh->frame = NULL;
    mh->eventbox = NULL;
    mh->handle_im = NULL;

    return mh;
}

void add_move_handle(MoveHandle * mh, GtkWidget * vbox)
{
    GdkPixbuf *pb;
    GtkWidget *im;

    mh->container = gtk_alignment_new(0,0,1,1);
    gtk_widget_show(mh->container);
    /* protect against destruction when unpacking */
    g_object_ref(mh->container);

    mh->box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(mh->box);
    gtk_container_add(GTK_CONTAINER(mh->container), mh->box);

    mh->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(mh->button, MEDIUM_TOPHEIGHT, MEDIUM_TOPHEIGHT);
    add_tooltip(mh->button, _("Iconify panel"));
    gtk_widget_show(mh->button);

    pb = get_system_pixbuf(ICONIFY_ICON);
    im = gtk_image_new_from_pixbuf(pb);
    gtk_widget_set_name(im, "gxfce_color1");
    g_object_unref(pb);
    gtk_container_add(GTK_CONTAINER(mh->button), im);
    gtk_widget_show(im);

    mh->iconify_im = im;

    mh->eventbox = gtk_event_box_new();
    add_tooltip(mh->eventbox, _("Move panel"));
    gtk_widget_show(mh->eventbox);

    mh->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(mh->eventbox), mh->frame);
    gtk_widget_show(mh->frame);

    pb = get_system_pixbuf(HANDLE_ICON);
    im = gtk_image_new_from_pixbuf(pb);
    gtk_widget_set_name(im, "gxfce_color1");
    g_object_unref(pb);
    gtk_container_add(GTK_CONTAINER(mh->frame), im);
    gtk_widget_show(im);

    if(mh->side == LEFT)
    {
        gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
    }
    else
    {
        gtk_box_pack_end(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
    }

    /* signals */
    g_signal_connect(mh->button, "clicked", G_CALLBACK(iconify_cb), NULL);
    attach_move_callbacks(mh->eventbox);

    gtk_box_pack_start(GTK_BOX(vbox), mh->container, TRUE, TRUE, 0);
}

void move_handle_free(MoveHandle * mh)
{
    g_free(mh);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void move_handle_set_size(MoveHandle * mh, int size)
{
    GtkWidget *newbox;
    int h = top_height(size);
    int w = icon_size(size);

    gtk_container_remove(GTK_CONTAINER(mh->box), mh->button);
    gtk_container_remove(GTK_CONTAINER(mh->box), mh->eventbox);
    
    gtk_container_remove(GTK_CONTAINER(mh->container), mh->box);
    
    if(size == TINY)
    {
	int h1 = 2 * (w + 4) / 3;
	int h2 = w + 4 - h1;
	
	newbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(newbox);
	
	gtk_box_pack_start(GTK_BOX(newbox), mh->button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(newbox), mh->eventbox, TRUE, TRUE, 0);
	
	gtk_widget_set_size_request(mh->eventbox, h2, h1);
	gtk_widget_set_size_request(mh->button, h2, h2);
    }
    else
    {
	newbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(newbox);
	
	if (mh->side == LEFT)
	{
	    gtk_box_pack_start(GTK_BOX(newbox), mh->button, FALSE, FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(newbox), mh->eventbox, TRUE, TRUE, 0);
	}
	else
	{
	    gtk_box_pack_end(GTK_BOX(newbox), mh->button, FALSE, FALSE, 0);
	    gtk_box_pack_end(GTK_BOX(newbox), mh->eventbox, TRUE, TRUE, 0);
	}
	
	gtk_widget_set_size_request(mh->eventbox, w + 8 - h, h);
	gtk_widget_set_size_request(mh->button, h, h);
    }
    
    mh->box = newbox;
    gtk_container_add(GTK_CONTAINER(mh->container), mh->box);
}

void move_handle_set_style(MoveHandle * mh, int style)
{
    if(style == OLD_STYLE)
    {
        gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_OUT);
        gtk_widget_set_name(mh->frame, "gxfce_color1");

        gtk_widget_set_name(mh->eventbox, "gxfce_color1");

        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NORMAL);
        gtk_widget_set_name(mh->button, "gxfce_color1");
    }
    else
    {
        gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_NONE);
        gtk_widget_set_name(mh->frame, "gxfce_color7");

        gtk_widget_set_name(mh->eventbox, "gxfce_color7");

        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
        gtk_widget_set_name(mh->button, "gxfce_color7");
    }
}
