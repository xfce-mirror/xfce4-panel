/*  handle.c
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

#include <xfce_movehandler.h>

#include "handle.h"
#include "icons.h"
#include "popup.h" /* to hide popups */
#include "callbacks.h"

#if 0
#include "icons/handle.xpm"
#include "icons/close.xpm"
#include "icons/iconify.xpm"
#endif

enum
{HANDLE_ICONIFY, HANDLE_CLOSE, HANDLE_NOICON};

struct _Handle
{
    GtkWidget *base;
    GtkWidget *box;

    GtkWidget *button;

    GtkWidget *handler;
};

static void handle_arrange(Handle * mh)
{
    gboolean vertical = settings.orientation == VERTICAL;
    int position = settings.popup_position;

    if(mh->box)
    {
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->button);
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->handler);

        /* removing the box will destroy it */
        gtk_container_remove(GTK_CONTAINER(mh->base), mh->box);
    }

    /* create new box */
    if(vertical)
        mh->box = gtk_hbox_new(FALSE, 0);
    else
        mh->box = gtk_vbox_new(FALSE, 0);

    gtk_widget_show(mh->box);
    gtk_container_add(GTK_CONTAINER(mh->base), mh->box);

    if(vertical)
    {
	if (position == LEFT)
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->handler, TRUE, TRUE, 0);
	}
	else
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->handler, TRUE, TRUE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	}
    }
    else
    {
	if (position == BOTTOM)
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->handler, TRUE, TRUE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	}
	else
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->handler, TRUE, TRUE, 0);
	}
    }
}

static void handler_pressed_cb(GtkWidget *h, GdkEventButton *event, 
			       gpointer data)
{
    hide_current_popup_menu();
}

static gboolean handler_released_cb(GtkWidget *h, GdkEventButton *event, 
			        gpointer data)
{
    gtk_window_get_position(GTK_WINDOW(toplevel), &settings.x, &settings.y);
}

Handle *handle_new(int side)
{
    GdkPixbuf *pb;
    GtkWidget *im;
    Handle *mh = g_new(Handle, 1);

    mh->base = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(mh->base);

    /* protect against destruction when unpacking */
    g_object_ref(mh->base);

    mh->button = gtk_button_new();
    if(settings.style == NEW_STYLE)
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
    gtk_widget_show(mh->button);

    if (side == LEFT)
    {
	pb = get_system_pixbuf(CLOSE_ICON);
	im = gtk_image_new_from_pixbuf(pb);
	g_object_unref(pb);
	gtk_widget_show(im);
	gtk_container_add(GTK_CONTAINER(mh->button), im);

	add_tooltip(mh->button, _("Quit..."));
	
	g_signal_connect(mh->button, "clicked", G_CALLBACK(close_cb), NULL);
    }
    else
    {
	pb = get_system_pixbuf(ICONIFY_ICON);
	im = gtk_image_new_from_pixbuf(pb);
	g_object_unref(pb);
	gtk_widget_show(im);
	gtk_container_add(GTK_CONTAINER(mh->button), im);

	add_tooltip(mh->button, _("Iconify panel"));
	
	g_signal_connect(mh->button, "clicked", G_CALLBACK(iconify_cb), NULL);
    }

    gtk_widget_set_name(im, "xfce_popup_button");

    mh->handler = xfce_movehandler_new(toplevel);
    gtk_widget_show(mh->handler);
    
    gtk_widget_set_name(mh->handler, "xfce_panel");

    /* connect_after is necessary to let the default handler run 
     * probably could have returned FALSE from the handler to get
     * the same effect; never mind, this works */
    g_signal_connect_after(mh->handler, "button-press-event", 
	    	     G_CALLBACK(handler_pressed_cb), NULL);

    g_signal_connect_after(mh->handler, "button-release-event", 
	    	     G_CALLBACK(handler_released_cb), NULL);

    /* protect against destruction when removed from box */
    g_object_ref(mh->button);
    g_object_ref(mh->handler);

    mh->box = NULL;

    handle_set_size(mh, settings.size);
    handle_arrange(mh);

    return mh;
}

void handle_pack(Handle * mh, GtkBox *box)
{
    gtk_box_pack_start(box, mh->base, FALSE, FALSE, 0);
}

void handle_unpack(Handle * mh, GtkContainer * container)
{
    gtk_container_remove(container, mh->base);
}

void handle_set_size(Handle * mh, int size)
{
    int h = top_height[size];
    int w = icon_size[size] + border_width - h;
    gboolean vertical = settings.orientation == VERTICAL;

    if(vertical)
        gtk_widget_set_size_request(mh->handler, w + border_width, h);
    else
        gtk_widget_set_size_request(mh->handler, h, w);

    gtk_widget_set_size_request(mh->button, h, h);
}

void handle_set_style(Handle * mh, int style)
{
    if(style == OLD_STYLE)
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NORMAL);
    else
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
}

void handle_set_popup_position(Handle *mh)
{
    handle_arrange(mh);
}


