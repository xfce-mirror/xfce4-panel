/*  central.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <j.b.huijsmans@hetnet.nl>
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

/* central.c
 *
 * Contains functions for the central part of the panel, which consists of the
 * desktop switcher and four mini-buttons.
 *
 */

#include "central.h"

#include "xfce.h"
#include "wmhints.h"
#include "callbacks.h"
#include "icons.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* structures */

struct _ScreenButton
{
    int index;
    char *name;

    int callback_id;

    GtkWidget *frame;
    GtkWidget *button;
    GtkWidget *label;
};

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* colors used for screen buttons in OLD_STYLE */

static char *default_screen_names[] = {
    N_("One"),
    N_("Two"),
    N_("Three"),
    N_("Four"),
    N_("Five"),
    N_("Six"),
    N_("Seven"),
    N_("Eight"),
    N_("Nine"),
    N_("Ten"),
    N_("Eleven"),
    N_("Twelve")
};

static char *screen_class[] = {
    "gxfce_color2",
    "gxfce_color5",
    "gxfce_color4",
    "gxfce_color6"
};

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Central panel  

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

GtkWidget *minibuttons[4];
GtkWidget *left_table;
GtkWidget *right_table;

GtkWidget *desktop_table;
ScreenButton *screen_buttons[NBSCREENS];

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/*  creation and destruction */

void central_panel_init()
{
    int i;

    for(i = 0; i < 4; i++)
        minibuttons[i] = NULL;

    left_table = NULL;
    right_table = NULL;
    desktop_table = NULL;

    for(i = 0; i < NBSCREENS; i++)
        screen_buttons[i] = screen_button_new(i);
}

/*---------------------------------------------------------------------------*/

static void create_minibuttons(void)
{
    GdkPixbuf *pb[4];
    GtkWidget *im;
    GtkWidget *button;
    int i;

    pb[0] = get_pixbuf_from_id(MINILOCK_ICON);
    pb[1] = get_pixbuf_from_id(MINIINFO_ICON);
    pb[2] = get_pixbuf_from_id(MINIPALET_ICON);
    pb[3] = get_pixbuf_from_id(MINIPOWER_ICON);

    for(i = 0; i < 4; i++)
    {
        button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_widget_show(button);

        im = gtk_image_new_from_pixbuf(pb[i]);
        gtk_widget_show(im);
        g_object_unref(pb[i]);
        gtk_container_add(GTK_CONTAINER(button), im);

        minibuttons[i] = button;
    }

    /* fixed tooltips, since the user can't change the icon anyway */
    add_tooltip(minibuttons[0], _("Lock the screen"));
    add_tooltip(minibuttons[1], _("Info..."));
    add_tooltip(minibuttons[2], _("Setup..."));
    add_tooltip(minibuttons[3], _("Mouse 1: Exit\nMouse 3: Restart"));

    /* signals */
    g_signal_connect_swapped(minibuttons[0], "clicked",
                             G_CALLBACK(mini_lock_cb), NULL);

    g_signal_connect_swapped(minibuttons[1], "clicked",
                             G_CALLBACK(mini_info_cb), NULL);

    g_signal_connect_swapped(minibuttons[2], "clicked",
                             G_CALLBACK(mini_palet_cb), NULL);

    /* make left _and_ right mouse buttons work for mini power icon */
    g_signal_connect(minibuttons[3], "button-press-event",
                     G_CALLBACK(gtk_button_pressed), NULL);
    g_signal_connect(minibuttons[3], "button-release-event",
                     G_CALLBACK(mini_power_cb), NULL);
}

static void add_mini_table(int side, GtkBox * hbox)
{
    GtkWidget *table;
    int first;

    table = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table);

    if(side == LEFT)
    {
        left_table = table;
        first = 0;
    }
    else
    {
        right_table = table;
        first = 2;
    }

    /* by default put buttons in 1st column in 2 rows */
    gtk_table_attach(GTK_TABLE(table), minibuttons[first],
                     0, 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 0, 0);
    gtk_table_attach(GTK_TABLE(table), minibuttons[first + 1],
                     0, 1, 1, 2, GTK_SHRINK, GTK_EXPAND, 0, 0);

    gtk_box_pack_start(hbox, table, TRUE, TRUE, 0);
}

static void add_desktop_table(GtkBox * hbox)
{
    GtkWidget *table;
    int i;

    table = gtk_table_new(2, NBSCREENS, FALSE);
    gtk_widget_show(table);

    desktop_table = table;

    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        add_screen_button(sb, table);
    }

    gtk_box_pack_start(hbox, table, TRUE, TRUE, 0);
}

void add_central_panel(GtkBox * hbox)
{
    create_minibuttons();
    add_mini_table(LEFT, hbox);
    add_desktop_table(hbox);
    add_mini_table(RIGHT, hbox);
}

/*---------------------------------------------------------------------------*/

void central_panel_cleanup()
{
    int i;

    for(i = 0; i < NBSCREENS; i++)
        screen_button_free(screen_buttons[i]);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* global settings */

static void reorder_minitable(int side, int size)
{
    GtkWidget *table;
    int n;
    GList *child;
    GtkTableChild *tc;
    int hpos, vpos;

    if(side == LEFT)
    {
        table = left_table;
        n = 1;
    }
    else
    {
        table = right_table;
        n = 3;
    }

    /* when size == SMALL put second button in second column first row
     * otherwise put second button in first column second row */
    for(child = GTK_TABLE(table)->children;
        child && child->data; child = child->next)
    {
        tc = (GtkTableChild *) child->data;

        if(tc->widget == minibuttons[n])
            break;
    }

    if(size == SMALL)
    {
        hpos = 1;
        vpos = 0;
    }
    else
    {
        hpos = 0;
        vpos = 1;
    }

    tc->left_attach = hpos;
    tc->right_attach = hpos + 1;
    tc->top_attach = vpos;
    tc->bottom_attach = vpos + 1;

}

static void reorder_desktop_table(int size)
{
    GtkWidget *table;
    GList *child;
    GtkTableChild *tc;
    int hpos, vpos;

    table = desktop_table;

    for(child = GTK_TABLE(table)->children; child && child->data;
        child = child->next)
    {
        ScreenButton *sb;
        int i;

        tc = (GtkTableChild *) child->data;

        for(i = 0; i < NBSCREENS; i++)
        {
            sb = screen_buttons[i];

            if(tc->widget == sb->frame)
                break;
        }

        if((i % 2) == 0)
        {
            if(size == SMALL)
            {
                hpos = i;
                vpos = 0;
            }
            else
            {
                hpos = i / 2;
                vpos = 0;
            }
        }
        else
        {
            if(size == SMALL)
            {
                hpos = i;
                vpos = 0;
            }
            else
            {
                hpos = (i - 1) / 2;
                vpos = 1;
            }
        }

        tc->left_attach = hpos;
        tc->right_attach = hpos + 1;
        tc->top_attach = vpos;
        tc->bottom_attach = vpos + 1;
    }
}

void central_panel_set_size(int size)
{
    GtkRequisition req;
    int w, h, i;

    /* mini tables */
    reorder_minitable(LEFT, size);
    reorder_minitable(RIGHT, size);

    /* screen buttons */
    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        screen_button_set_size(sb, size);
    }

    /* desktop table */
    reorder_desktop_table(size);

    /* set all minibuttons to the same size */
    for(i = 0; i < 4; i++)
        gtk_widget_set_size_request(minibuttons[i], -1, -1);

    w = h = 0;

    for(i = 0; i < 4; i++)
    {
        gtk_widget_size_request(minibuttons[i], &req);

        if(req.width > w)
            w = req.width;
        if(req.height > h)
            h = req.height;
    }

    for(i = 0; i < 4; i++)
        gtk_widget_set_size_request(minibuttons[i], w, h);
}

/*---------------------------------------------------------------------------*/

void central_panel_set_style(int style)
{
    int i;

    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        screen_button_set_style(sb, style);
    }
}

void central_panel_set_icon_theme(const char *theme)
{
}

void central_panel_set_current(int n)
{
    int i;

    if(n < 0)
        return;

    if(n > NBSCREENS)
    {
        /* try to force our maximum number of desktops */
        request_net_number_of_desktops(NBSCREENS);
        request_net_current_desktop(NBSCREENS);
        return;
    }

    if(n > settings.num_screens)
        central_panel_set_num_screens(n);

    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        /* Prevent signal handler to run; we're being called
         * as a reaction to a change by another program
         */
        g_signal_handler_block(sb->button, sb->callback_id);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->button), i == n);
        g_signal_handler_unblock(sb->button, sb->callback_id);
    }
}

void central_panel_set_num_screens(int n)
{
    int i;

    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        if(i < n)
            gtk_widget_show(sb->frame);
        else
            gtk_widget_hide(sb->frame);
    }

    if(n == 1)
        gtk_widget_hide(desktop_table);
    else
        gtk_widget_show(desktop_table);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* configuration */

void central_panel_parse_xml(xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar *value;
    int i;

    child = node->children;

    for(i = 0; i < NBSCREENS; i++)
    {
        ScreenButton *sb = screen_buttons[i];

        if(!child || !xmlStrEqual(child->name, (const xmlChar *)"Screen"))
        {
            sb->name = g_strdup(default_screen_names[i]);
            continue;
        }

        value = DATA(child);

        if(value)
            sb->name = (char *)value;
        else
            sb->name = g_strdup(default_screen_names[i]);

        if(child)
            child = child->next;
    }
}

void central_panel_write_xml(xmlNodePtr root)
{
    xmlNodePtr node;
    int i;
    
    node = xmlNewTextChild(root, NULL, "Central", NULL);
    
    for (i = 0; i < NBSCREENS; i++)
    {
	ScreenButton *sb = screen_buttons[i];
	
	xmlNewTextChild(node, NULL, "Screen", sb->name);
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Screen buttons

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

ScreenButton *screen_button_new(int index)
{
    ScreenButton *sb = g_new(ScreenButton, 1);

    sb->index = index;
    sb->name = NULL;
    sb->callback_id = 0;

    sb->frame = NULL;
    sb->button = NULL;
    sb->label = NULL;

    return sb;
};

void add_screen_button(ScreenButton * sb, GtkWidget * table)
{
    int pos;

    sb->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(sb->frame), GTK_SHADOW_IN);
    gtk_widget_show(sb->frame);

    sb->button = gtk_toggle_button_new();
    gtk_button_set_relief(GTK_BUTTON(sb->button), GTK_RELIEF_HALF);
    gtk_widget_set_size_request(sb->button, SCREEN_BUTTON_WIDTH, -1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->button), sb->index == 0);
    gtk_container_add(GTK_CONTAINER(sb->frame), sb->button);
    gtk_widget_show(sb->button);

    sb->label = gtk_label_new(sb->name);
    gtk_misc_set_alignment(GTK_MISC(sb->label), 0.1, 0.5);
    gtk_container_add(GTK_CONTAINER(sb->button), sb->label);
    gtk_widget_show(sb->label);

    /* pack buttons alternating first and second row */
    if((sb->index % 2) == 0)
    {
        pos = sb->index;

        gtk_table_attach(GTK_TABLE(table), sb->frame, pos, pos + 1, 0, 1, GTK_EXPAND,
                         GTK_EXPAND, 0, 0);
    }
    else
    {
        pos = sb->index - 1;

        gtk_table_attach(GTK_TABLE(table), sb->frame, pos, pos + 1, 1, 2,
                         GTK_EXPAND, GTK_EXPAND, 0, 0);
    }

    /* signals */
    /* we need the callback id to be able to block the handler to 
       prevent a race condition when changing screens */
    sb->callback_id =
        g_signal_connect_swapped(sb->button, "clicked",
                                 G_CALLBACK(request_net_current_desktop),
                                 GINT_TO_POINTER(sb->index));

    g_signal_connect(sb->button, "button-press-event",
                     G_CALLBACK(screen_button_pressed_cb), sb);
}

void screen_button_destroy(ScreenButton * sb)
{
    gtk_container_remove(GTK_CONTAINER(desktop_table), sb->frame);
    gtk_widget_destroy(sb->frame);
    sb->frame = NULL;
}

void screen_button_free(ScreenButton * sb)
{
    g_free(sb->name);
    g_free(sb);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void screen_button_set_size(ScreenButton * sb, int size)
{
    int w, h;

    /* NOTE:
     * calculation of height is very arbitrary. I just put here what looks good
     * on my screen. Should probably be something a little more intelligent. */
    if(size == SMALL)
    {
        w = SCREEN_BUTTON_WIDTH / 2;
        h = SMALL_PANEL_ICONS;
    }
    else if(size == LARGE)
    {
        w = SCREEN_BUTTON_WIDTH;
        h = (LARGE_TOPHEIGHT + LARGE_PANEL_ICONS) / 2 - 6;
    }
    else
    {
        w = SCREEN_BUTTON_WIDTH;
        h = (MEDIUM_TOPHEIGHT + MEDIUM_PANEL_ICONS) / 2 - 5;
    }

    gtk_widget_set_size_request(sb->button, w, h);
}

void screen_button_set_style(ScreenButton * sb, int style)
{
    if(style == OLD_STYLE)
        gtk_widget_set_name(sb->button, screen_class[sb->index % 4]);
    else
        gtk_widget_set_name(sb->button, "gxfce_color4");
}
