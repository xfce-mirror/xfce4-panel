/*  xfce4
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
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

#include <X11/Xlib.h>
#include <libxfcegui4/netk-util.h>

#include "xfce.h"
#include "groups.h"
#include "handle.h"
#include "settings.h"

/* global settings */
Settings settings;
Position position;

/*  Panel dimensions 
 *  ----------------
 *  These sizes are exported to all other modules
*/
int minibutton_size[] = { 20, 24, 24, 32 };

int icon_size[] = { 24, 30, 45, 60 };

int border_width = 4;

int popup_icon_size[] = { 20, 24, 24, 32 };

int top_height[] = { 14, 16, 16, 18 };

int screen_button_width[] = { 35, 45, 80, 80 };

/*  Panel framework
 *  ---------------
*/
GtkWidget *toplevel = NULL;

static GtkWidget *main_frame;
/* static GtkWidget *main_box;	contains panel and taskbar (future) */

static GtkWidget *panel_box;	/* contains all panel components */

Handle *handles[2];
static GtkWidget *group_box;

/*  callbacks  */
static gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, 
				gpointer data)
{
    quit(FALSE);

    return TRUE;
}

/*  creation and destruction  */
static GtkWidget *create_panel_window(void)
{
    GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWindow *window = GTK_WINDOW(w);
    GdkPixbuf *pb;

    gtk_window_set_title(window, _("XFce Panel"));
    gtk_window_set_decorated(window, FALSE);
    gtk_window_set_resizable(window, FALSE);
    gtk_window_stick(window);

    pb = get_system_pixbuf(XFCE_ICON);
    gtk_window_set_icon(window, pb);
    g_object_unref(pb);

    g_signal_connect(w, "delete-event", G_CALLBACK(panel_delete_cb), NULL);

    return w;
}

static void create_panel_framework(void)
{
    gboolean vertical = (settings.orientation == VERTICAL);
    
    /* toplevel window */
    if (!toplevel)
    {
	toplevel = create_panel_window();
	g_object_add_weak_pointer(G_OBJECT(toplevel), (gpointer *)&toplevel);
    }
    
    /* main frame */
    main_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(main_frame), GTK_SHADOW_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(main_frame), 0);
    gtk_widget_show(main_frame);
    gtk_container_add(GTK_CONTAINER(toplevel), main_frame);

    /* create all widgets that depend on orientation */
    if (vertical)
    {
/*	main_box = gtk_hbox_new(FALSE, 0);*/

	panel_box = gtk_vbox_new(FALSE, 0);

	group_box = gtk_vbox_new(FALSE, 0);
    }
    else
    {
/*	main_box = gtk_vbox_new(FALSE, 0);*/

	panel_box = gtk_hbox_new(FALSE, 0);

	group_box = gtk_hbox_new(FALSE, 0);
    }
    
    /* show them */
/*    gtk_widget_show(main_box);*/
    gtk_widget_show(panel_box);
    gtk_widget_show(group_box);

    /* create handles */
    handles[LEFT] = handle_new(LEFT);
    handles[RIGHT] = handle_new(RIGHT);
    
    /* pack the widgets into the main frame */
    
/*    gtk_container_add(GTK_CONTAINER(main_frame), main_box);
    gtk_box_pack_start(GTK_BOX(main_box), panel_box, TRUE, TRUE, 0);
*/
    gtk_container_add(GTK_CONTAINER(main_frame), panel_box);
    
    handle_pack(handles[LEFT], GTK_BOX(panel_box));

    gtk_box_pack_start(GTK_BOX(panel_box), group_box, TRUE, TRUE, 0);

    handle_pack(handles[RIGHT], GTK_BOX(panel_box));
}

void panel_cleanup(void)
{
    groups_cleanup();
}

static void init_settings();

void create_panel(void)
{
    gboolean need_init = TRUE;
    int x, y;

    if (need_init)
    {
	/* fill in the 'settings' structure */
	init_settings();
	get_global_prefs();

	need_init = FALSE;
    }

    /* panel framework */
    create_panel_framework();

    /* FIXME
     * somehow the position gets set differently in the code below
     * we just save it here and restore it before reading the config
     * file */
    x = position.x;
    y = position.y;

    groups_init(GTK_BOX(group_box));

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config();

    /* panel may have moved slightly off the screen */
    position.x = x;
    position.y = y;
    panel_set_position();
    
    gtk_widget_show(toplevel);
    set_window_layer(toplevel, settings.layer);
}

void panel_add_control(void)
{
    settings.num_groups++;
    
    groups_set_num_groups(settings.num_groups);
}

/*  Panel settings
 *  --------------
*/
void panel_set_orientation(int orientation)
{
    settings.orientation = orientation;

    /* save panel controls */
    groups_unpack();

    /* no need to recreate the window */
    gtk_widget_destroy(main_frame);
    create_panel_framework();

    groups_pack(GTK_BOX(group_box));
    groups_set_orientation(orientation);
    
    /* this seems more logical */
    switch (settings.popup_position)
    {
	case LEFT:
	    settings.popup_position = BOTTOM;
	    break;
	case RIGHT:
	    settings.popup_position = TOP;
	    break;
	case TOP:
	    settings.popup_position = RIGHT;
	    break;
	case BOTTOM:
	    settings.popup_position = LEFT;
	    break;
    }
    
    panel_set_popup_position(settings.popup_position);

    position.x = position.y = -1;
    panel_set_position();
}

void panel_set_layer(int layer)
{
    settings.layer = layer;

    set_window_layer(toplevel, layer);

/*    groups_set_layer(layer);*/
}

void panel_set_size(int size)
{
    settings.size = size;

    groups_set_size(size);
    handle_set_size(handles[LEFT], size);
    handle_set_size(handles[RIGHT], size);
}

void panel_set_popup_position(int position)
{
    settings.popup_position = position;

    groups_set_popup_position(position);
    handle_set_popup_position(handles[LEFT]);
    handle_set_popup_position(handles[RIGHT]);

    /* this is necessary to get the right proportions */
    panel_set_size(settings.size);
}

void panel_set_style(int style)
{
    settings.style = style;
    
    groups_set_style(style);
    handle_set_style(handles[LEFT], style);
    handle_set_style(handles[RIGHT], style);
}

void panel_set_theme(const char *theme)
{
    char *tmp = settings.theme;

    settings.theme = g_strdup(theme);
    g_free(tmp);

    groups_set_theme(theme);
}

/* FIXME: this should probably NOT be a global option.
 * Just add and remove controls based on the config file */
void panel_set_num_groups(int n)
{
    settings.num_groups = n;
    groups_set_num_groups(n);
}

void panel_set_settings(void)
{
    panel_set_size(settings.size);
    panel_set_popup_position(settings.popup_position);

    panel_set_style(settings.style);
    panel_set_theme(settings.theme);

    panel_set_num_groups(settings.num_groups);
}

void panel_set_position(void)
{
    GtkRequisition req;
    int w, h;
    w = gdk_screen_width();
    h = gdk_screen_height();

    gtk_widget_size_request(toplevel, &req);

    if(position.x == -1 || position.y == -1)
    {
	DesktopMargins margins;
	Screen *xscreen;
	
	xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
	netk_get_desktop_margins(xscreen, &margins);
    
	if (settings.orientation == VERTICAL)
	{
	    position.y = h / 2 - req.height / 2;
	    position.x = margins.left;
	}
	else
	{
	    position.x = w / 2 - req.width / 2;
	    position.y = h - req.height - margins.bottom;
	}
    }
    else
    {
        if(position.x < 0)
            position.x = 0;
        if(position.x > w - req.width)
            position.x = w - req.width;
        if(position.y < 0)
            position.y = 0;
        if(position.y > h - req.height)
            position.y = h - req.height;
    }

    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
}

/*  Global preferences
 *  ------------------
*/
static void init_settings(void)
{
    position.x = -1;
    position.y = -1;

    settings.orientation = HORIZONTAL;
    settings.layer = ABOVE;

    settings.size = SMALL;
    settings.popup_position = RIGHT;
    settings.style = NEW_STYLE;

    settings.theme = NULL;

    settings.num_groups = 9;
}

void panel_parse_xml(xmlNodePtr node)
{
    xmlChar *value;
    xmlNodePtr child;
    int n;

    /* properties */
    value = xmlGetProp(node, (const xmlChar *)"orientation");

    if(value)
    {
        settings.orientation = atoi(value);
	g_free(value);
    }

    value = xmlGetProp(node, (const xmlChar *)"layer");

    if(value)
    {
        n = atoi(value);

        if(n >= ABOVE && n <= BELOW)
            settings.layer = n;

	g_free(value);
    }

    value = xmlGetProp(node, (const xmlChar *)"size");

    if(value)
    {
        settings.size = atoi(value);
	g_free(value);
    }

    value = xmlGetProp(node, (const xmlChar *)"popupposition");

    if(value)
    {
        settings.popup_position = atoi(value);
	g_free(value);
    }

    value = xmlGetProp(node, (const xmlChar *)"style");

    if(value)
        settings.style = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"icontheme");

    if(value)
        settings.theme = g_strdup(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"groups");

    if (value)
    {
	settings.num_groups = atoi(value);
	g_free(value);
    }
    else
    {
	value = xmlGetProp(node, (const xmlChar *)"left");

	if(value)
	{
	    settings.num_groups = atoi(value);
	    g_free(value);

	    value = xmlGetProp(node, (const xmlChar *)"right");

	    if(value)
	    {
		settings.num_groups += atoi(value);
		g_free(value);
	    }
	}
    }

    /* child nodes */
    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Position"))
        {
            value = xmlGetProp(child, (const xmlChar *)"x");

            if(value)
                position.x = atoi(value);

            g_free(value);

            value = xmlGetProp(child, (const xmlChar *)"y");

            if(value)
                position.y = atoi(value);

            g_free(value);
        }
    }

    /* check the values */
    if(settings.style < OLD_STYLE || settings.style > NEW_STYLE)
        settings.style = NEW_STYLE;
    if(settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
        settings.orientation = HORIZONTAL;
    if(settings.size < TINY || settings.size > LARGE)
        settings.size = SMALL;
    if(settings.num_groups < 1 || settings.num_groups > 2*NBGROUPS)
        settings.num_groups = 10;

    /* some things just look awful with old style */
    if(settings.orientation == HORIZONTAL)
    {
	if (settings.popup_position == LEFT || 
	    settings.popup_position == RIGHT)
	{
	    settings.style = NEW_STYLE;
	}
    }
    else
    {
	if (settings.popup_position == TOP || 
	    settings.popup_position == BOTTOM)
	{
	    settings.style = NEW_STYLE;
	}
    }
}

void panel_write_xml(xmlNodePtr root)
{
    xmlNodePtr node;
    xmlNodePtr child;
    char value[MAXSTRLEN + 1];

    node = xmlNewTextChild(root, NULL, "Panel", NULL);

    snprintf(value, 2, "%d", settings.orientation);
    xmlSetProp(node, "orientation", value);

    snprintf(value, 2, "%d", settings.layer);
    xmlSetProp(node, "layer", value);

    snprintf(value, 2, "%d", settings.size);
    xmlSetProp(node, "size", value);

    snprintf(value, 2, "%d", settings.popup_position);
    xmlSetProp(node, "popupposition", value);

    snprintf(value, 2, "%d", settings.style);
    xmlSetProp(node, "style", value);

    if(settings.theme)
        xmlSetProp(node, "icontheme", settings.theme);

    snprintf(value, 3, "%d", settings.num_groups);
    xmlSetProp(node, "groups", value);

    child = xmlNewTextChild(node, NULL, "Position", NULL);

    snprintf(value, 5, "%d", position.x);
    xmlSetProp(child, "x", value);

    snprintf(value, 5, "%d", position.y);
    xmlSetProp(child, "y", value);
}


