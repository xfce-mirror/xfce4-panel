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

#include <config.h>
#include <my_gettext.h>

#include <X11/Xlib.h>
#include <libxfcegui4/libnetk.h>

#include "xfce.h"
#include "groups.h"
#include "controls.h"
#include "controls_dialog.h"
#include "popup.h"
#include "info.h"
#include "settings.h"
#include "mcs_client.h"

/* global settings */
Settings settings;
Position position;

GtkWidget *toplevel = NULL;

static GtkWidget *main_frame;
static GtkWidget *panel_box;	/* contains all panel components */
static GtkWidget *handles[2];
static GtkWidget *group_box;

/* lock settings update when panel is not yet (re)build */
static gboolean panel_created = FALSE;

/*  Panel dimensions 
 *  ----------------
 *  These sizes are exported to all other modules.
 *  Arrays are indexed by symbolic sizes TINY, SMALL, MEDIUM, LARGE
 *  (see global.h).
*/
int icon_size[] = { 24, 30, 45, 60 };

int border_width = 4;

int top_height[] = { 14, 16, 18, 20 };

int popup_icon_size[] = { 20, 24, 24, 32 };

/*  Move handle menu
 *  ----------------
*/
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
  { N_("/_Panel settings"), NULL, edit_prefs,  0, "<Item>" },
  { N_("/_Settings manager"), NULL, settings_mgr,  0, "<Item>" },
  { "/sep",              NULL, NULL,        0, "<Separator>" },
  { N_("/_About XFce"),  NULL, do_info,     0, "<Item>" },
  { N_("/_Help"),        NULL, do_help,     0, "<Item>" },
  { "/sep",          NULL, NULL,        0, "<Separator>" },
  { N_("/_Lock screen"), NULL, lock_screen, 0, "<Item>" },
  { N_("/E_xit"),        NULL, exit_panel,  0, "<Item>" },
};

static GtkMenu *create_handle_menu(void)
{
    static GtkMenu *menu = NULL;
    GtkItemFactory *ifactory;

    if (!menu)
    {
	ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<popup>", NULL);

	gtk_item_factory_set_translate_func(ifactory, 
					    (GtkTranslateFunc) gettext,
					    NULL, NULL);
	gtk_item_factory_create_items(ifactory, G_N_ELEMENTS(panel_items), 
				      panel_items, NULL);
	menu = GTK_MENU(gtk_item_factory_get_widget(ifactory, "<popup>"));
    }

    return menu;
}

/*  Move handles
 *  ------------
*/
static void handle_set_size(GtkWidget * mh, int size)
{
    int h = (size == TINY) ? top_height[TINY] : top_height[SMALL];
    int w = icon_size[size] + 2*border_width;
    gboolean vertical = settings.orientation == VERTICAL;

    if(vertical)
        gtk_widget_set_size_request(mh, w, h);
    else
        gtk_widget_set_size_request(mh, h, w);
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

GtkWidget *handle_new(void)
{
    GtkWidget *mh;
    GtkMenu *menu;

    mh = xfce_movehandler_new(toplevel);
    gtk_widget_show(mh);
    
    gtk_widget_set_name(mh, "xfce_panel");

    menu = create_handle_menu();
    
    g_signal_connect(mh, "button-press-event", 
	    	     G_CALLBACK(handler_pressed_cb), menu);

    g_signal_connect(mh, "button-release-event", 
	    	     G_CALLBACK(handler_released_cb), NULL);

    /* protect against destruction when removed from box */
    g_object_ref(mh);

    handle_set_size(mh, settings.size);

    return mh;
}

/*  Panel framework
 *  ---------------
*/
static gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, 
				gpointer data)
{
    quit(FALSE);

    return TRUE;
}

static GtkWidget *create_panel_window(void)
{
    GtkWidget *w;
    GtkWindow *window;
    GdkPixbuf *pb;

    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    window = GTK_WINDOW(w);
    
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
    
    /* this is necessary after a SIGHUP */
    gtk_window_stick(GTK_WINDOW(toplevel));

    /* main frame */
    main_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(main_frame), GTK_SHADOW_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(main_frame), 0);
    gtk_widget_show(main_frame);
    gtk_container_add(GTK_CONTAINER(toplevel), main_frame);

    /* create all widgets that depend on orientation */
    if (vertical)
    {
	panel_box = gtk_vbox_new(FALSE, 0);
	group_box = gtk_vbox_new(FALSE, 0);
    }
    else
    {
	panel_box = gtk_hbox_new(FALSE, 0);
	group_box = gtk_hbox_new(FALSE, 0);
    }
    
    /* show them */
    gtk_widget_show(panel_box);
    gtk_widget_show(group_box);

    /* create handles */
    handles[LEFT] = handle_new();
    handles[RIGHT] = handle_new();
    
    /* pack the widgets into the main frame */
    gtk_container_add(GTK_CONTAINER(main_frame), panel_box);
    gtk_box_pack_start(GTK_BOX(panel_box), handles[LEFT], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel_box), group_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(panel_box), handles[RIGHT], FALSE, FALSE, 0);
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

    /* necessary for initial settings to do the right thing */
    panel_created = FALSE;

    if (need_init)
    {
	/* fill in the 'settings' structure */
	init_settings();
	get_global_prefs();

	/* If there is a settings manager it takes precedence */
	mcs_watch_xfce_channel();
    
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

    panel_created = TRUE;

    /* panel may have moved slightly off the screen */
    position.x = x;
    position.y = y;
    panel_set_position();
    
    gtk_widget_show(toplevel);
    set_window_layer(toplevel, settings.layer);
    set_window_skip(toplevel);
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

    if (!panel_created)
	return;
    
    hide_current_popup_menu();

    /* save panel controls */
    groups_unpack();

    /* no need to recreate the window */
    gtk_widget_destroy(main_frame);
    create_panel_framework();

    groups_pack(GTK_BOX(group_box));
    groups_set_orientation(orientation);
    
    position.x = position.y = -1;
    panel_set_position();
}

void panel_set_layer(int layer)
{
    settings.layer = layer;

    if (!panel_created)
	return;
    
    set_window_layer(toplevel, layer);
}

void panel_set_size(int size)
{
    settings.size = size;

    if (!panel_created)
	return;
    
    hide_current_popup_menu();

    groups_set_size(size);
    handle_set_size(handles[LEFT], size);
    handle_set_size(handles[RIGHT], size);
}

void panel_set_popup_position(int position)
{
    settings.popup_position = position;

    if (!panel_created)
	return;
    
    hide_current_popup_menu();

    groups_set_popup_position(position);

    /* this is necessary to get the right proportions */
    panel_set_size(settings.size);
}

void panel_set_theme(const char *theme)
{
    g_free(settings.theme);
    settings.theme = g_strdup(theme);

    if (!panel_created)
	return;
    
    groups_set_theme(theme);
}

/* FIXME: this should probably NOT be a global option.
 * Just add and remove controls based on the config file */
void panel_set_num_groups(int n)
{
    settings.num_groups = n;

    if (!panel_created)
	return;
    
    groups_set_num_groups(n);
}

void panel_set_settings(void)
{
    panel_set_size(settings.size);
    panel_set_popup_position(settings.popup_position);

    panel_set_theme(settings.theme);

    panel_set_num_groups(settings.num_groups);
}

void panel_center(int side)
{
    GtkRequisition req;
    int w, h;
    DesktopMargins margins;
    Screen *xscreen;
    
    w = gdk_screen_width();
    h = gdk_screen_height();

    xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
    netk_get_desktop_margins(xscreen, &margins);
    
    gtk_widget_size_request(toplevel, &req);

    switch (side)
    {
	case LEFT:
	    position.x = margins.left;
	    position.y = h / 2 - req.height / 2;
	    break;
	case RIGHT:
	    position.x = w - req.width - margins.right;
	    position.y = h / 2 - req.height / 2;
	    break;
	case TOP:
	    position.x = w / 2 - req.width / 2;
	    position.y = margins.top;
	    break;
	default:
	    position.x = w / 2 - req.width / 2;
	    position.y = h - req.height - margins.bottom;
    }

    if (position.x < 0)
	position.x = 0;

    if (position.y < 0)
	position.y = 0;
    
    panel_set_position();
}

void panel_set_position(void)
{
    if(position.x < 0 || position.y < 0)
    {
	if (settings.orientation == HORIZONTAL)
	    panel_center(BOTTOM);
	else
	    panel_center(LEFT);
    }
    else
    {
	GtkRequisition req;
	int w, h;
	
	w = gdk_screen_width();
	h = gdk_screen_height();
	
	gtk_widget_size_request(toplevel, &req);
	
	if (position.x + req.width > w && req.width <= w)
	    position.x = w - req.width;
	
	if (position.y + req.height > h && req.height <= h)
	    position.y = h - req.height;
	
	DBG("position: (%d, %d)\n", position.x, position.y);

	/* use gdk to prevent margins from interfering :) */
	gtk_window_present(GTK_WINDOW(toplevel));
	gdk_window_move(toplevel->window, position.x, position.y);
    }
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
	int w, h;
	
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

	    if (position.x < 0 || position.y < 0)
		break;

	    value = xmlGetProp(child, (const xmlChar *)"screenwidth");

	    if (value)
		w = atoi(value);
	    else
		break;

	    value = xmlGetProp(child, (const xmlChar *)"screenheight");

	    if (value)
		h = atoi(value);
	    else
		break;

	    /* this doesn't actually work completely, we need to
	     * save the panel width as well to do it right */
	    if (w != gdk_screen_width() || h != gdk_screen_height())
	    {
		position.x = (int)((double)(position.x * gdk_screen_width()) / 
				(double)w);
		position.y = (int)((double)(position.y * gdk_screen_height()) / 
				(double)h);
	    }
        }
    }

    /* check the values */
    if(settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
        settings.orientation = HORIZONTAL;
    if(settings.size < TINY || settings.size > LARGE)
        settings.size = SMALL;
    if(settings.num_groups < 1 || settings.num_groups > 2*NBGROUPS)
        settings.num_groups = 10;
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

    if(settings.theme)
        xmlSetProp(node, "icontheme", settings.theme);

    snprintf(value, 3, "%d", settings.num_groups);
    xmlSetProp(node, "groups", value);

    child = xmlNewTextChild(node, NULL, "Position", NULL);

    snprintf(value, 5, "%d", position.x);
    xmlSetProp(child, "x", value);

    snprintf(value, 5, "%d", position.y);
    xmlSetProp(child, "y", value);

    /* save screen width and hide, so we can use a relative position
     * the user logs in on different computers */
    snprintf(value, 5, "%d", gdk_screen_width());
    xmlSetProp(child, "screenwidth", value);

    snprintf(value, 5, "%d", gdk_screen_height());
    xmlSetProp(child, "screenheight", value);
}


