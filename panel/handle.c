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

#include "handle.h"
#include "icons.h"
#include "popup.h"
#include "callbacks.h"

#if 0
#include "icons/handle.xpm"
#include "icons/close.xpm"
#include "icons/iconify.xpm"
#endif

/* moving the panel */
static gboolean pressed = FALSE;

struct Offset
{
    int x, y;
};

static struct Offset offset;

static void
handle_pressed(GtkWidget * widget, GdkEventButton * event, gpointer * topwin)
{

    gint upositionx = 0;
    gint upositiony = 0;
    gint uwidth = 0;
    gint uheight = 0;
    gint xp, yp;

    /* Added by Jason Litowitz */
    hide_current_popup_menu();
    
    /* ignore double and triple click */
    if(event->type != GDK_BUTTON_PRESS)
        return;
    if(event->button != 1)
        return;

    gtk_grab_add(widget);
    gdk_pointer_grab(widget->window, TRUE,
                     GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK, NULL,
                     NULL, 0);

    gdk_window_get_origin(((GtkWidget *) topwin)->window, &upositionx,
                          &upositiony);
    gdk_window_get_size(((GtkWidget *) topwin)->window, &uwidth, &uheight);

    offset.x = (int)upositionx - event->x_root;
    offset.y = (int)upositiony - event->y_root;

    xp = 0;
    yp = 0;
    gdk_window_get_pointer(GDK_ROOT_PARENT(), &xp, &yp, NULL);
    xp += offset.x;
    yp += offset.y;

    pressed = TRUE;
}

static void
handle_released(GtkWidget * widget, GdkEventButton * event, gpointer * topwin)
{
    gint xp, yp;

    if(event->button != 1)
        return;

    if(!pressed)
        return;

    pressed = FALSE;

    xp = 0;
    yp = 0;
    gdk_window_get_pointer(GDK_ROOT_PARENT(), &xp, &yp, NULL);
    xp += offset.x;
    yp += offset.y;

    gtk_window_move(GTK_WINDOW(topwin), xp > 0 ? xp : 0, yp > 0 ? yp : 0);
    
    gtk_grab_remove(widget);
    gdk_pointer_ungrab(0);

    gtk_window_get_position(GTK_WINDOW(topwin), &settings.x, &settings.y);
    offset.x = offset.y = 0;
}

static void
handle_motion(GtkWidget * widget, GdkEventMotion * event, gpointer * topwin)
{
    gint xp, yp;

    if(!pressed)
        return;

    xp = 0;
    yp = 0;
    gdk_window_get_pointer(GDK_ROOT_PARENT(), &xp, &yp, NULL);
    xp += offset.x;
    yp += offset.y;

    gtk_window_move(GTK_WINDOW(topwin), xp > 0 ? xp : 0, yp > 0 ? yp : 0);
}

void attach_handle_callbacks(GtkWidget * widget)
{
    gtk_widget_set_events(widget,
                          GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK);

    g_signal_connect(widget, "button_press_event",
                     G_CALLBACK(handle_pressed), toplevel);
    g_signal_connect(widget, "button_release_event",
                     G_CALLBACK(handle_released), toplevel);
    g_signal_connect(widget, "motion_notify_event",
                     G_CALLBACK(handle_motion), toplevel);
}

/* end moving code */

enum
{HANDLE_ICONIFY, HANDLE_CLOSE, HANDLE_NOICON};

struct _Handle
{
    GtkWidget *base;
    GtkWidget *box;

    GtkWidget *button;

    GtkWidget *eventbox;
    GtkWidget *frame;
};

static void handle_arrange(Handle * mh)
{
    gboolean vertical = settings.orientation == VERTICAL;
    int position = settings.popup_position;

    if(mh->box)
    {
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->button);
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->eventbox);

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
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
	}
	else
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	}
    }
    else
    {
	if (position == BOTTOM)
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	}
	else
	{
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
	    gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
	}
    }
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

    mh->eventbox = gtk_event_box_new();
    add_tooltip(mh->eventbox, _("Move panel"));
    gtk_widget_show(mh->eventbox);

    gtk_widget_set_name(mh->eventbox, "xfce_panel");

    mh->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_NONE);
    gtk_widget_show(mh->frame);
    gtk_container_add(GTK_CONTAINER(mh->eventbox), mh->frame);

    pb = get_system_pixbuf(HANDLE_ICON);
    im = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(mh->frame), im);

    gtk_widget_set_name(im, "xfce_panel");

    /* protect against destruction when removed from box */
    g_object_ref(mh->button);
    g_object_ref(mh->eventbox);

    mh->box = NULL;

    handle_set_size(mh, settings.size);
    handle_arrange(mh);

    /* signals */
    attach_handle_callbacks(mh->eventbox);

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
        gtk_widget_set_size_request(mh->eventbox, w + border_width, h);
    else
        gtk_widget_set_size_request(mh->eventbox, h, w);

    gtk_widget_set_size_request(mh->button, h, h);
}

void handle_set_style(Handle * mh, int style)
{
    if(style == OLD_STYLE)
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NORMAL);
    else
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
}



