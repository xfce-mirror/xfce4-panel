/*  iconbutton.c
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

/*  Icon buttons
 *  ------------
 *  This structure was created because most panel items and plugins use a 
 *  button with an icon in it. The functions defined here make it really 
 *  simple to create panel buttons.
 *
 *  It knows about the panel size, so setting a new image will automatically
 *  take care of resizing.
 *
 *  To create an IconButton call icon_button_new with a pixbuf as argument. The
 *  button will obtain a reference to the pixbuf, so it's safe to destroy your
 *  own reference.
 *
 *  Since the GtkButton is the first item in the structure you can cast it 
 *  with GTK_BUTTON() or GTK_WIDGET().
*/

#include "global.h"
#include "iconbutton.h"
#include "icons.h"

struct _IconButton
{
    GdkPixbuf *pb;

    GtkWidget *image;
    GtkWidget *button;

    char *command;
    gboolean use_terminal;
    int callback_id;
};

IconButton *icon_button_new(GdkPixbuf * pb)
{
    IconButton *b = g_new(IconButton, 1);

    if(pb && GDK_IS_PIXBUF(pb))
    {
        b->pb = pb;
        g_object_ref(b->pb);
    }
    else
    {
        b->pb = get_pixbuf_by_id(UNKNOWN_ICON);
    }

    b->image = gtk_image_new();
    gtk_widget_show(b->image);

    b->button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(b->button), GTK_RELIEF_NONE);
    gtk_widget_show(b->button);

    icon_button_set_size(b, settings.size);

    gtk_container_add(GTK_CONTAINER(b->button), b->image);

    return b;
}

GtkWidget *icon_button_get_button(IconButton * b)
{
    return b->button;
}

void icon_button_set_size(IconButton * b, int size)
{
    int s = icon_size[size];
    int h = s + border_width;
    GdkPixbuf *tmp = get_scaled_pixbuf(b->pb, s);

    gtk_image_set_from_pixbuf(GTK_IMAGE(b->image), tmp);
    g_object_unref(tmp);

    gtk_widget_set_size_request(icon_button_get_button(b), h, h);
}

void icon_button_set_pixbuf(IconButton * b, GdkPixbuf * pb)
{
    if(pb && GDK_IS_PIXBUF(pb))
    {
        g_object_unref(b->pb);

        b->pb = pb;
        g_object_ref(b->pb);

        icon_button_set_size(b, settings.size);
    }
}

void icon_button_free(IconButton * b)
{
    g_object_unref(b->pb);

    if(GTK_IS_WIDGET(b->button))
        gtk_widget_destroy(b->button);
}
