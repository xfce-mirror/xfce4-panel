/*  xfce.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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

/*  xfce.c 
 *  ------
 *  Contains 'main' function, quit and restart functions, some utility functions
 *  and panel functions.
*/

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "xfce.h"

#include "xfce_support.h"
#include "central.h"
#include "side.h"
#include "wmhints.h"
#include "icons.h"
#include "callbacks.h"
#include "settings.h"

/*  Panel dimensions 
*/
int minibutton_size[] = { 16, 24, 24, 24 };

int icon_size[] = { 24, 30, 45, 60 };

int border_width = 4;

int popup_icon_size[] = { 16, 20, 24, 32 };

int top_height[] = { 12, 14, 14, 16 };

int screen_button_width[] = { 30, 40, 80, 80 };

/*  Panel framework
 *  ---------------
*/
GtkWidget *toplevel;

static GtkWidget *main_frame;
static GtkWidget *main_box;	/* contains panel and taskbar (future) */

static GtkWidget *panel_box;	/* contains all panel components */

static GtkWidget *left_box;
static GtkWidget *right_box;
static GtkWidget *central_frame;
static GtkWidget *central_box;

gboolean central_frame_created = FALSE;

/*  callbacks  */
static gboolean panel_delete_cb(GtkWidget * window, GdkEvent * ev, 
				gpointer data)
{
    quit(FALSE);

    return TRUE;
}

static gboolean panel_destroy_cb(GtkWidget * frame, GdkEvent * ev, 
				      gpointer data)
{
    side_panel_cleanup(LEFT);
    side_panel_cleanup(RIGHT);

    if (central_frame_created)
	central_panel_cleanup();

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

    gtk_container_set_border_width(GTK_CONTAINER(w), 0);

    pb = get_system_pixbuf(XFCE_ICON);
    gtk_window_set_icon(window, pb);
    g_object_unref(pb);

    g_signal_connect(w, "destroy-event", G_CALLBACK(panel_destroy_cb), NULL);
    g_signal_connect(w, "delete-event", G_CALLBACK(panel_delete_cb), NULL);

    if(settings.on_top)
        set_window_type_dock(w, TRUE);

    return w;
}

static void create_panel_contents(void)
{
    gboolean vertical = settings.orientation == VERTICAL;
    
    main_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(main_frame), GTK_SHADOW_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(main_frame), 0);
    gtk_widget_show(main_frame);
    gtk_container_add(GTK_CONTAINER(toplevel), main_frame);

    /* create all widgets that depend on orientation */
    if (vertical)
    {
	main_box = gtk_hbox_new(FALSE, 0);

	panel_box = gtk_vbox_new(FALSE, 0);

	left_box = gtk_vbox_new(FALSE, 0);

	right_box = gtk_vbox_new(FALSE, 0);

	central_box = gtk_vbox_new(FALSE, 0);
    }
    else
    {
	main_box = gtk_vbox_new(FALSE, 0);

	panel_box = gtk_hbox_new(FALSE, 0);

	left_box = gtk_hbox_new(FALSE, 0);

	right_box = gtk_hbox_new(FALSE, 0);

	central_box = gtk_hbox_new(FALSE, 0);
    }
    
    /* show them */
    gtk_widget_show(main_box);
    gtk_widget_show(panel_box);
    gtk_widget_show(left_box);
    gtk_widget_show(right_box);
    gtk_widget_show(central_box);

    /* ref the ones we may need to move around */
    g_object_ref(panel_box);	/* may be swapped with taskbar container */
    g_object_ref(right_box); 	/* may have to be temporarily removed when 
				   adding central panel from the dialog */
    
    /* create the other widgets */
    
    if(settings.show_central)
    {
	central_frame = gtk_frame_new(NULL);
	
	if(settings.style == OLD_STYLE)
	    gtk_frame_set_shadow_type(GTK_FRAME(central_frame), GTK_SHADOW_OUT);
    
        gtk_widget_show(central_frame);
	g_object_ref(central_frame);

	central_frame_created = TRUE;
    }
    
    /* pack the widgets into the main frame */
    
    gtk_container_add(GTK_CONTAINER(main_frame), main_box);

    gtk_box_pack_start(GTK_BOX(main_box), panel_box, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(panel_box), left_box, TRUE, TRUE, 0);

    if (settings.show_central)
    {
	gtk_box_pack_start(GTK_BOX(panel_box), central_frame, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(central_frame), central_box);
    }

    gtk_box_pack_start(GTK_BOX(panel_box), right_box, TRUE, TRUE, 0);
}

void panel_init(void)
{
    toplevel = create_panel_window();
    create_panel_contents();
}

void panel_cleanup(void)
{
    if(GTK_IS_WIDGET(toplevel))
        gtk_widget_destroy(toplevel);
}

/*  Panel settings
 *  --------------
*/
void panel_set_orientation(int orientation)
{
    settings.orientation = orientation;

    settings.x = settings.y = -1;

    /* only keep side panels. We just rebuild the central panel
     * if necessary */
    side_panel_unpack(LEFT);
    side_panel_unpack(RIGHT);

    gtk_widget_destroy(main_frame);

    create_panel_contents();

    side_panel_pack(LEFT, GTK_BOX(left_box));
    side_panel_pack(RIGHT, GTK_BOX(right_box));
    
    if (settings.show_central)
	central_panel_init(GTK_BOX(central_box));

    panel_set_position();
}

void panel_set_on_top(gboolean on_top)
{
    settings.on_top = on_top;

    set_window_type_dock(toplevel, on_top);
    side_panel_set_on_top(LEFT, on_top);
    side_panel_set_on_top(RIGHT, on_top);
}

void panel_set_size(int size)
{
    settings.size = size;

    side_panel_set_size(LEFT, size);
    central_panel_set_size(size);
    side_panel_set_size(RIGHT, size);
}

void panel_set_popup_size(int size)
{
    settings.popup_size = size;

    side_panel_set_popup_size(LEFT, size);
    side_panel_set_popup_size(RIGHT, size);
}

void panel_set_popup_position(int position)
{
    settings.popup_position = position;

    side_panel_set_popup_position(LEFT, position);
    side_panel_set_popup_position(RIGHT, position);

    /* this is necessary to get the right proportions */
    panel_set_size(settings.size);
}

void panel_set_style(int style)
{
    settings.style = style;

    if(style == OLD_STYLE)
        gtk_frame_set_shadow_type(GTK_FRAME(central_frame), GTK_SHADOW_OUT);
    else
        gtk_frame_set_shadow_type(GTK_FRAME(central_frame),
                                  GTK_SHADOW_ETCHED_IN);

    side_panel_set_style(LEFT, style);
    central_panel_set_style(style);
    side_panel_set_style(RIGHT, style);
}

void panel_set_theme(const char *theme)
{
    char *tmp = settings.theme;

    settings.theme = g_strdup(theme);
    g_free(tmp);

    side_panel_set_theme(LEFT, theme);
    central_panel_set_theme(theme);
    side_panel_set_theme(RIGHT, theme);
}

void panel_set_num_left(int n)
{
    settings.num_left = n;
    side_panel_set_num_groups(LEFT, n);
}

void panel_set_num_right(int n)
{
    settings.num_right = n;
    side_panel_set_num_groups(RIGHT, n);
}

void panel_set_num_screens(int n)
{
    settings.num_screens = n;
    central_panel_set_num_screens(n);
}

void panel_set_show_central(gboolean show)
{
    settings.show_central = show;

    if(show)
        gtk_widget_show(central_frame);
    else
        gtk_widget_hide(central_frame);
}

void panel_set_show_desktop_buttons(gboolean show)
{
    settings.show_desktop_buttons = show;

    central_panel_set_show_desktop_buttons(show);
}

void panel_set_show_minibuttons(gboolean show)
{
    settings.show_minibuttons = show;

    central_panel_set_show_minibuttons(show);
}

void panel_set_current(int n)
{
    current_screen = n;
    central_panel_set_current(n);
}

/*  Global preferences
 *  ------------------
*/
Settings settings;

void init_settings(void)
{
    settings.x = -1;
    settings.y = -1;

    settings.size = SMALL;
    settings.popup_size = MEDIUM;
    settings.popup_position = TOP;
    settings.style = NEW_STYLE;
    settings.orientation = HORIZONTAL;
    settings.theme = NULL;
    settings.on_top = TRUE;

    settings.num_left = 5;
    settings.num_right = 5;
    settings.num_screens = 4;

    settings.show_central = TRUE;
    settings.show_desktop_buttons = TRUE;
    settings.show_minibuttons = TRUE;

    settings.lock_command = NULL;
    settings.exit_command = NULL;
}

void panel_set_settings(void)
{
    int n;

    panel_set_size(settings.size);
    panel_set_popup_size(settings.popup_size);
    panel_set_popup_position(settings.popup_position);

    panel_set_style(settings.style);
    panel_set_theme(settings.theme);

    side_panel_set_num_groups(LEFT, settings.num_left);
    side_panel_set_num_groups(RIGHT, settings.num_right);

    central_panel_set_num_screens(settings.num_screens);

    panel_set_show_central(settings.show_central);
    central_panel_set_show_desktop_buttons(settings.show_desktop_buttons);
    panel_set_show_minibuttons(settings.show_minibuttons);

    request_net_number_of_desktops(settings.num_screens);

    n = get_net_current_desktop();

    /* force creation of _NET_CURRENT_DESKTOP hint */
    if(n == 0)
        central_panel_set_current(1);

    current_screen = n;
    central_panel_set_current(n);
/*    panel_set_orientation(settings.orientation);*/
}

void panel_set_position(void)
{
    GtkRequisition req;
    int w = 0, h = 0;

    if(!w)
    {
        w = gdk_screen_width();
        h = gdk_screen_height();
    }

    gtk_widget_size_request(toplevel, &req);

    if(settings.x == -1 || settings.y == -1)
    {
	if (settings.orientation == VERTICAL)
	{
	    settings.x = settings.y = 0;
	}
	else
	{
	    settings.x = w / 2 - req.width / 2;
	    settings.y = h - req.height;
	}
    }
    else
    {
        if(settings.x < 0)
            settings.x = 0;
        if(settings.x > w - req.width)
            settings.x = w - req.width;
        if(settings.y < 0)
            settings.y = 0;
        if(settings.y > h - req.height)
            settings.y = h - req.height;
    }

    gtk_window_move(GTK_WINDOW(toplevel), settings.x, settings.y);
}

void panel_parse_xml(xmlNodePtr node)
{
    xmlChar *value;
    xmlNodePtr child;
    int n;

    /* properties */
    value = xmlGetProp(node, (const xmlChar *)"size");

    if(value)
        settings.size = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"popupsize");

    if(value)
        settings.popup_size = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"popupposition");

    if(value)
        settings.popup_position = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"style");

    if(value)
        settings.style = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"orientation");

    if(value)
        settings.orientation = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"icontheme");

    if(value)
        settings.theme = g_strdup(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"ontop");

    if(value)
    {
        n = atoi(value);

        if(n == 1)
            settings.on_top = TRUE;
        else
            settings.on_top = FALSE;
    }

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"left");

    if(value)
        settings.num_left = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"right");

    if(value)
        settings.num_right = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"screens");

    if(value)
        settings.num_screens = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"showcentral");

    if(value)
    {
        n = atoi(value);

        if(n == 1)
            settings.show_central = TRUE;
        else
            settings.show_central = FALSE;
    }

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"desktopbuttons");

    if(value)
    {
        n = atoi(value);

        if(n == 1)
            settings.show_desktop_buttons = TRUE;
        else
            settings.show_desktop_buttons = FALSE;
    }

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"minibuttons");

    if(value)
    {
        n = atoi(value);

        if(n == 1)
            settings.show_minibuttons = TRUE;
        else
            settings.show_minibuttons = FALSE;
    }

    g_free(value);

    /* child nodes */
    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Position"))
        {
            value = xmlGetProp(child, (const xmlChar *)"x");

            if(value)
                settings.x = atoi(value);

            g_free(value);

            value = xmlGetProp(child, (const xmlChar *)"y");

            if(value)
                settings.y = atoi(value);

            g_free(value);
        }
        if(xmlStrEqual(child->name, (const xmlChar *)"Lock"))
        {
            value = DATA(child);

            if(value)
                settings.lock_command = (char *)value;
        }
        if(xmlStrEqual(child->name, (const xmlChar *)"Exit"))
        {
            value = DATA(child);

            if(value)
                settings.exit_command = (char *)value;
        }
    }

    /* check the values */
    if(settings.style < OLD_STYLE || settings.style > NEW_STYLE)
        settings.style = NEW_STYLE;
    if(settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
        settings.orientation = HORIZONTAL;
    if(settings.size < TINY || settings.size > LARGE)
        settings.size = SMALL;
    if(settings.popup_size < SMALL || settings.popup_size > LARGE)
        settings.popup_size = MEDIUM;
    if(settings.num_left < 1 || settings.num_left > NBGROUPS)
        settings.num_left = 5;
    if(settings.num_right < 1 || settings.num_right > NBGROUPS)
        settings.num_right = 5;
    if(settings.num_screens < 1 || settings.num_screens > NBSCREENS)
        settings.num_screens = 4;

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

    snprintf(value, 2, "%d", settings.size);
    xmlSetProp(node, "size", value);

    snprintf(value, 2, "%d", settings.popup_size);
    xmlSetProp(node, "popupsize", value);

    snprintf(value, 2, "%d", settings.popup_position);
    xmlSetProp(node, "popupposition", value);

    snprintf(value, 2, "%d", settings.style);
    xmlSetProp(node, "style", value);

    snprintf(value, 2, "%d", settings.orientation);
    xmlSetProp(node, "orientation", value);

    if(settings.theme)
        xmlSetProp(node, "icontheme", settings.theme);

    snprintf(value, 2, "%d", settings.on_top);
    xmlSetProp(node, "ontop", value);

    snprintf(value, 3, "%d", settings.num_left);
    xmlSetProp(node, "left", value);

    snprintf(value, 3, "%d", settings.num_right);
    xmlSetProp(node, "right", value);

    snprintf(value, 3, "%d", settings.num_screens);
    xmlSetProp(node, "screens", value);

    snprintf(value, 2, "%d", settings.show_central);
    xmlSetProp(node, "showcentral", value);

    snprintf(value, 2, "%d", settings.show_desktop_buttons);
    xmlSetProp(node, "desktopbuttons", value);

    snprintf(value, 2, "%d", settings.show_minibuttons);
    xmlSetProp(node, "minibuttons", value);

    child = xmlNewTextChild(node, NULL, "Position", NULL);

    snprintf(value, 5, "%d", settings.x);
    xmlSetProp(child, "x", value);

    snprintf(value, 5, "%d", settings.y);
    xmlSetProp(child, "y", value);

    if(settings.lock_command)
    {
        child = xmlNewTextChild(node, NULL, "Lock", settings.lock_command);
    }

    if(settings.exit_command)
    {
        child = xmlNewTextChild(node, NULL, "Exit", settings.exit_command);
    }
}

/*  Main program
 *  ------------
*/
void sighandler(int sig)
{
    switch (sig)
    {
        case SIGHUP:
            restart();
            break;
        default:
            quit(FALSE);
    }
}

void quit(gboolean force)
{
    if(!force && !confirm(_("Are you sure you want to log off ?"), 
			  GTK_STOCK_QUIT, NULL))
    {
        return;
    }
    
    if (!disable_user_config)
	write_panel_config();

    gtk_widget_hide(toplevel);

    if(settings.exit_command)
        exec_cmd_silent(settings.exit_command, FALSE);

    gtk_widget_destroy(toplevel);

    gtk_main_quit();
}

void restart(void)
{
    gtk_widget_destroy(main_frame);

    gtk_main_quit();

    xfce_run();
}

void xfce_init(void)
{
    check_net_support();

    create_builtin_pixbufs();

    signal(SIGHUP, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);
}

void xfce_run(void)
{
    gboolean need_init = TRUE;

    if (need_init)
    {
	/* fill in the 'settings' structure */
	init_settings();
	get_global_prefs();

	need_init = FALSE;
    }

    /* panel framework */
    panel_init();
    side_panel_init(LEFT, GTK_BOX(left_box));
    central_panel_init(GTK_BOX(central_box));
    side_panel_init(RIGHT, GTK_BOX(right_box));

    /* give early visual feedback 
     * the init functions have already created the basic panel */
    panel_set_position();
    gtk_widget_show(toplevel);

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config();

    /* panel may have moved slightly off the screen */
    panel_set_position();

    watch_root_properties();

    request_net_number_of_desktops(settings.num_screens);

    gtk_main();
}

int main(int argc, char **argv)
{
    if(argc == 2 && (strequal(argv[1], "-v") || strequal(argv[1], "--version")))
    {
        g_print(_("xfce4, version %s\n\n"
                  "Part of the XFce Desktop Environment\n"
                  "http://www.xfce.org\n"), VERSION);
    }

    gtk_init(&argc, &argv);

    xfce_init();

    xfce_run();

    return 0;
}
