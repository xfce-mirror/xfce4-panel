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

#include <libxfcegui4/xfce_movehandler.h>
#include <libxfcegui4/xfce_decorbutton.h>

#include "xfce.h"
#include "handle.h"
#include "popup.h" /* to hide popups */
#include "groups.h"
#include "callbacks.h"
#include "controls_dialog.h"
#include "mcs_client.h"

/* popup menu */
static void edit_prefs(void)
{
    mcs_dialog(NULL);
}

static void settings_mgr(void)
{
    mcs_dialog("all");
}

static void add_new(void)
{
    Control *control;
    
    panel_add_control();
    panel_set_position();
    
    control = groups_get_control(settings.num_groups-1);

    controls_dialog(control);
}

static void lock_screen(void)
{
    exec_cmd("xflock", FALSE);
}

static void exit_panel(void)
{
    quit(FALSE);
}

extern void info_panel_dialog(void);

static void do_info(void)
{
   info_panel_dialog(); 
}

static void do_help(void)
{
    exec_cmd("xfhelp", FALSE);
}

static GtkItemFactoryEntry panel_items[] = {
  { N_("/XFce Panel"),        NULL, NULL,        0, "<Title>" },
  { "/sep",              NULL, NULL,        0, "<Separator>" },
  { N_("/Add _new item"), NULL, add_new,    0, "<Item>" },
  { "/sep",              NULL, NULL,        0, "<Separator>" },
  { N_("/_Preferences"), NULL, edit_prefs,  0, "<Item>" },
  { N_("/_Settings manager"), NULL, settings_mgr,  0, "<Item>" },
  { N_("/sep"),          NULL, NULL,        0, "<Separator>" },
  { N_("/_Lock screen"), NULL, lock_screen, 0, "<Item>" },
  { N_("/E_xit"),        NULL, exit_panel,  0, "<Item>" },
  { "/sep",              NULL, NULL,        0, "<Separator>" },
  { N_("/_About XFce"),  NULL, do_info,     0, "<Item>" },
  { N_("/_Help"),        NULL, do_help,     0, "<Item>" },
};

static GtkMenu *create_handle_menu(void)
{
    static GtkMenu *menu = NULL;
    GtkItemFactory *ifactory;

    if (!menu)
    {
	ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<popup>", NULL);

	gtk_item_factory_create_items(ifactory, G_N_ELEMENTS(panel_items), 
				      panel_items, NULL);

	menu = GTK_MENU(gtk_item_factory_get_widget(ifactory, "<popup>"));
    }

    return menu;
}

/* the handle */
struct _Handle
{
#if 0
    GtkWidget *base;
    GtkWidget *box;

    GtkWidget *button;
#endif
    GtkWidget *handler;
};

static void handle_arrange(Handle * mh)
{
#if 0
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
#endif
}

static gboolean handler_pressed_cb(GtkWidget *h, GdkEventButton *event, 
			       GtkMenu *menu)
{
    hide_current_popup_menu();

    if (event->button == 3 || 
	    (event->button == 1 && event->state & GDK_SHIFT_MASK ))
    {
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, event->time); 
	return TRUE;
    }
    
    /* let default handler run */
    return FALSE;
}

static gboolean handler_released_cb(GtkWidget *h, GdkEventButton *event, 
			            gpointer data)
{
    gtk_window_get_position(GTK_WINDOW(toplevel), &position.x, &position.y);
    /* let default handler run */
    return FALSE;
}

Handle *handle_new(int side)
{
/*    GtkWidget *im;*/
    Handle *mh = g_new(Handle, 1);
    GtkMenu *menu;

#if 0
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
	im = xfce_decorbutton_new(XFCE_DECORBUTTON_CLOSE);
	gtk_widget_show(im);
	gtk_container_add(GTK_CONTAINER(mh->button), im);
	
	add_tooltip(mh->button, _("Quit..."));
	
	g_signal_connect(mh->button, "clicked", G_CALLBACK(close_cb), NULL);
    }
    else
    {
	im = xfce_decorbutton_new(XFCE_DECORBUTTON_HIDE);
	gtk_widget_show(im);
	gtk_container_add(GTK_CONTAINER(mh->button), im);
	
	add_tooltip(mh->button, _("Iconify panel"));
	
	g_signal_connect(mh->button, "clicked", G_CALLBACK(iconify_cb), NULL);
    }

    gtk_widget_set_name(im, "xfce_popup_button");
#endif

    mh->handler = xfce_movehandler_new(toplevel);
    gtk_widget_show(mh->handler);
    
    gtk_widget_set_name(mh->handler, "xfce_panel");

    menu = create_handle_menu();
    
    g_signal_connect(mh->handler, "button-press-event", 
	    	     G_CALLBACK(handler_pressed_cb), menu);

    g_signal_connect(mh->handler, "button-release-event", 
	    	     G_CALLBACK(handler_released_cb), NULL);

    /* protect against destruction when removed from box 
    g_object_ref(mh->button);*/
    g_object_ref(mh->handler);

/*    mh->box = NULL;*/

    handle_set_size(mh, settings.size);
    handle_arrange(mh);

    return mh;
}

void handle_pack(Handle * mh, GtkBox *box)
{
    gtk_box_pack_start(box, mh->handler, FALSE, FALSE, 0);
}

void handle_unpack(Handle * mh, GtkContainer * container)
{
    gtk_container_remove(container, mh->handler);
}

void handle_set_size(Handle * mh, int size)
{
    int h = top_height[size];
    int w = icon_size[size] + 2*border_width;
    gboolean vertical = settings.orientation == VERTICAL;

    if(vertical)
        gtk_widget_set_size_request(mh->handler, w, h);
    else
        gtk_widget_set_size_request(mh->handler, h, w);

/*    gtk_widget_set_size_request(mh->button, h, h);*/
}

void handle_set_style(Handle * mh, int style)
{
#if 0
    if(style == OLD_STYLE)
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NORMAL);
    else
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
#endif
}

void handle_set_popup_position(Handle *mh)
{
    handle_arrange(mh);
}


