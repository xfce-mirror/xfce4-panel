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

#include <session-client.h>

#include "xfce.h"

#include "xfce_support.h"
#include "central.h"
#include "side.h"
#include "wmhints.h"
#include "icons.h"
#include "callbacks.h"
#include "settings.h"
#include "handle.h"

/*  Panel dimensions 
*/
int minibutton_size[] = { 16, 24, 24, 32 };

int icon_size[] = { 24, 30, 45, 60 };

int border_width = 4;

int popup_icon_size[] = { 20, 24, 24, 32 };

int top_height[] = { 14, 16, 16, 18 };

int screen_button_width[] = { 30, 40, 80, 80 };

/*  Panel framework
 *  ---------------
*/
GtkWidget *toplevel;

static SessionClient *client_session;
static gboolean session_managed = FALSE;

static GtkWidget *main_frame;
static GtkWidget *main_box;	/* contains panel and taskbar (future) */

static GtkWidget *panel_box;	/* contains all panel components */

Handle *handles[2];

static GtkWidget *group_box;

gboolean central_created = FALSE;

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
    toplevel = NULL;

    side_panel_cleanup(LEFT);
    side_panel_cleanup(RIGHT);

    if (central_created)
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

    if(settings.layer == ABOVE)
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

	group_box = gtk_vbox_new(FALSE, 0);
    }
    else
    {
	main_box = gtk_vbox_new(FALSE, 0);

	panel_box = gtk_hbox_new(FALSE, 0);

	group_box = gtk_hbox_new(FALSE, 0);
    }
    
    /* show them */
    gtk_widget_show(main_box);
    gtk_widget_show(panel_box);
    gtk_widget_show(group_box);

    /* create the other widgets */

    handles[LEFT] = handle_new(LEFT);
    handles[RIGHT] = handle_new(RIGHT);
    
    
    /* pack the widgets into the main frame */
    
    gtk_container_add(GTK_CONTAINER(main_frame), main_box);

    gtk_box_pack_start(GTK_BOX(main_box), panel_box, TRUE, TRUE, 0);

    handle_pack(handles[LEFT], GTK_BOX(panel_box));

    gtk_box_pack_start(GTK_BOX(panel_box), group_box, TRUE, TRUE, 0);

    handle_pack(handles[RIGHT], GTK_BOX(panel_box));
}

void panel_init(void)
{
    toplevel = create_panel_window();
    create_panel_contents();
}

void panel_cleanup(void)
{
    if(toplevel && GTK_IS_WIDGET(toplevel))
        gtk_widget_destroy(toplevel);
    toplevel = NULL;
}

/*  Panel settings
 *  --------------
*/
void panel_set_orientation(int orientation)
{
    settings.orientation = orientation;

    position.x = position.y = -1;

    /* only keep groups. We just rebuild the central panel
     * if necessary */
    side_panel_unpack(LEFT);
    side_panel_unpack(RIGHT);

    gtk_widget_destroy(main_frame);

    create_panel_contents();

    side_panel_pack(LEFT, GTK_BOX(group_box));

    if (settings.show_central)
    {
	central_panel_init(GTK_BOX(group_box));
	central_created = TRUE;
    }

    side_panel_pack(RIGHT, GTK_BOX(group_box));
    
    panel_set_position();
    panel_set_popup_position(settings.popup_position);
}

void panel_set_layer(int layer)
{
    gboolean on_top = FALSE;
    
    settings.layer = layer;

    if (layer == ABOVE)
	on_top = TRUE;
    
    set_window_type_dock(toplevel, on_top);

    side_panel_set_on_top(LEFT, on_top);
    side_panel_set_on_top(RIGHT, on_top);
}

void panel_set_size(int size)
{
    settings.size = size;

    side_panel_set_size(LEFT, size);

    if (central_created)
	central_panel_set_size(size);

    side_panel_set_size(RIGHT, size);

    handle_set_size(handles[LEFT], size);
    handle_set_size(handles[RIGHT], size);
}

void panel_set_popup_position(int position)
{
    settings.popup_position = position;

    side_panel_set_popup_position(LEFT, position);
    side_panel_set_popup_position(RIGHT, position);

    handle_set_popup_position(handles[LEFT]);
    handle_set_popup_position(handles[RIGHT]);

    /* this is necessary to get the right proportions */
    panel_set_size(settings.size);
}

void panel_set_style(int style)
{
    settings.style = style;

    if (central_created)
	central_panel_set_style(style);
    
    side_panel_set_style(LEFT, style);
    side_panel_set_style(RIGHT, style);

    handle_set_style(handles[LEFT], style);
    handle_set_style(handles[RIGHT], style);
}

void panel_set_theme(const char *theme)
{
    char *tmp = settings.theme;

    settings.theme = g_strdup(theme);
    g_free(tmp);

    if (central_created)
	central_panel_set_theme(theme);

    side_panel_set_theme(LEFT, theme);
    side_panel_set_theme(RIGHT, theme);
}

void panel_set_central_index(int n)
{
    settings.central_index = n;

    if (central_created)
	central_panel_move(GTK_BOX(group_box), n);
}

void panel_set_num_groups(int n)
{
    settings.num_groups = n;
    side_panel_set_num_groups(n);
}

void panel_set_num_screens(int n)
{
    settings.num_screens = n;
    
    if (central_created)
	central_panel_set_num_screens(n);
}

void panel_set_show_central(gboolean show)
{
    settings.show_central = show;

    if (show)
    {
	if (!central_created)
	{
	    if (settings.central_index == -1)
		settings.central_index = settings.num_groups / 2;
	    
	    central_panel_init(GTK_BOX(group_box));
	    central_created = TRUE;
	    panel_set_central_index(settings.central_index);
	}

	central_panel_show();
    }
    else if (central_created)
    {
	central_panel_hide();
    }
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

/*  Global preferences
 *  ------------------
*/
Settings settings;
Position position;

void init_settings(void)
{
    position.x = -1;
    position.y = -1;

    settings.orientation = HORIZONTAL;
    settings.layer = ABOVE;
    settings.size = SMALL;
    settings.popup_position = TOP;
    settings.style = NEW_STYLE;
    settings.theme = NULL;

    settings.num_screens = 4;
    settings.num_groups = 8;
    settings.central_index = -1;

    settings.show_central = TRUE;
    settings.show_desktop_buttons = TRUE;
    settings.show_minibuttons = TRUE;

    settings.lock_command = NULL;
    settings.exit_command = NULL;
}

void panel_set_settings(void)
{
    panel_set_size(settings.size);
    panel_set_popup_position(settings.popup_position);

    panel_set_style(settings.style);
    panel_set_theme(settings.theme);

    side_panel_set_num_groups(settings.num_groups);

    panel_set_num_screens(settings.num_screens);

    panel_set_show_central(settings.show_central);
    panel_set_central_index(settings.central_index);

    if (central_created)
    {
	central_panel_set_show_desktop_buttons(settings.show_desktop_buttons);
	central_panel_set_show_minibuttons(settings.show_minibuttons);
    }
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

    if(position.x == -1 || position.y == -1)
    {
	if (settings.orientation == VERTICAL)
	{
	    position.x = position.y = 0;
	}
	else
	{
	    position.x = w / 2 - req.width / 2;
	    position.y = h - req.height;
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

    /* old setting, use it as fallback; superceded by "layer" */
    value = xmlGetProp(node, (const xmlChar *)"ontop");

    if(value)
    {
        n = atoi(value);

        if(n == 1)
            settings.layer = ABOVE;
        else
            settings.layer = NORMAL;

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

    value = xmlGetProp(node, (const xmlChar *)"left");

    if(value)
        settings.central_index = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"right");

    if(value)
    {
        settings.num_groups = atoi(value);

	if (settings.central_index > -1)
	    settings.num_groups += settings.central_index;
    }

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
                position.x = atoi(value);

            g_free(value);

            value = xmlGetProp(child, (const xmlChar *)"y");

            if(value)
                position.y = atoi(value);

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
    if(settings.num_groups < 1 || settings.num_groups > 2*NBGROUPS)
        settings.num_groups = 10;
    if(settings.num_screens < 1 || settings.num_screens > NBSCREENS)
        settings.num_screens = 4;

    if (settings.show_central && settings.central_index == -1)
	settings.central_index = settings.num_groups / 2;

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

    snprintf(value, 2, "%d", settings.popup_position);
    xmlSetProp(node, "popupposition", value);

    snprintf(value, 2, "%d", settings.style);
    xmlSetProp(node, "style", value);

    snprintf(value, 2, "%d", settings.orientation);
    xmlSetProp(node, "orientation", value);

    if(settings.theme)
        xmlSetProp(node, "icontheme", settings.theme);

    snprintf(value, 2, "%d", settings.layer);
    xmlSetProp(node, "layer", value);

    snprintf(value, 3, "%d", settings.central_index);
    xmlSetProp(node, "left", value);

    if (settings.central_index == -1)
	snprintf(value, 3, "%d", settings.num_groups);
    else
	snprintf(value, 3, "%d", settings.num_groups - settings.central_index);
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

    snprintf(value, 5, "%d", position.x);
    xmlSetProp(child, "x", value);

    snprintf(value, 5, "%d", position.y);
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
	    /* doesn't work when run from xterm and xterm closes
	       find something else!
	      restart();
	      break;
	    */
	      default:
            quit(TRUE);
    }
}

void quit(gboolean force)
{
    if(!force)
    {
	if (session_managed)
	{
	    logout_session(client_session);
	    return;
	}
	else if (!confirm(_("Are you sure you want to Exit ?"), GTK_STOCK_QUIT, NULL))
	{
	    return;
	}
    }
    
    if (!disable_user_config)
	write_panel_config();

    gtk_widget_hide(toplevel);

    if(settings.exit_command)
        exec_cmd_silent(settings.exit_command, FALSE);

/*    gtk_widget_destroy(toplevel);*/

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
    
    side_panel_init(LEFT, GTK_BOX(group_box));
    
    if (settings.show_central)
    {
	central_panel_init(GTK_BOX(group_box));
	central_created = TRUE;
    }

    side_panel_init(RIGHT, GTK_BOX(group_box));

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

static void save_yourself(gpointer data, int save_style, gboolean shutdown, int interact_style, gboolean fast)
{
    if (!disable_user_config)
	write_panel_config();
}

static void die (gpointer client_data)
{
    quit(TRUE);
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

    client_session = client_session_new(argc, argv, NULL /* data */ , SESSION_RESTART_IF_RUNNING, 40);

    client_session->save_yourself = save_yourself;
    client_session->die = die;

    if(!(session_managed = session_init(client_session)))
        g_message("xfce4: Cannot connect to session manager");

    xfce_init();

    xfce_run();

    return 0;
}
