/*  xfce.c
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

/* xfce.c 
 * 
 * Contains 'main' function, quit and restart functions, some utility functions
 * and panel function.
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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Utility functions  
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int icon_size(int size)
{
    switch (size)
    {
        case TINY:
            return TINY_PANEL_ICONS;
        case SMALL:
            return SMALL_PANEL_ICONS;
        case LARGE:
            return LARGE_PANEL_ICONS;
        default:
            return MEDIUM_PANEL_ICONS;
    }
}

int popup_size(int size)
{
    switch (size)
    {
        case SMALL:
            return SMALL_POPUP_ICONS;
        case LARGE:
            return LARGE_POPUP_ICONS;
        default:
            return MEDIUM_POPUP_ICONS;
    }
}

int top_height(int size)
{
    switch (size)
    {
        case TINY:
            return 0;
        case SMALL:
            return SMALL_TOPHEIGHT;
        case LARGE:
            return LARGE_TOPHEIGHT;
        default:
            return MEDIUM_TOPHEIGHT;
    }
}

static GtkTooltips *tooltips = NULL;

void add_tooltip(GtkWidget * widget, char *tip)
{
    if(!tooltips)
        tooltips = gtk_tooltips_new();

    gtk_tooltips_set_tip(tooltips, widget, tip, NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Main program
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
void sighandler(int sig)
{
    switch (sig)
    {
        case SIGHUP:
            restart();
            break;
        default:
            quit();
    }
}

void quit(void)
{
    if(!confirm(_("Are you sure you want to log off ?"), GTK_STOCK_QUIT, NULL))
    {
        return;
    }

    gtk_widget_hide(toplevel);

    if(settings.exit_command)
        exec_cmd_silent(settings.exit_command, FALSE);

    gtk_main_quit();

    panel_cleanup();
}

void restart(void)
{
    gtk_main_quit();

    panel_cleanup();

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
    init_settings();
    panel_init();
    side_panel_init(LEFT);
    central_panel_init();
    side_panel_init(RIGHT);

    get_panel_config();

    create_xfce_panel();

    watch_root_properties();

    gtk_main();
}

int main(int argc, char **argv)
{
/*    pid_t pid = fork();

    if(pid < 0)
    {
        g_error("%s (%d): ** ERROR ** fork() failed\n", __FILE__, __LINE__);
    }
    else if(pid > 0)
    {
        _exit(0);
    }
*/
    gtk_init(&argc, &argv);

    xfce_init();

    xfce_run();

    return (0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Panel creation and destruction
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
GtkWidget *toplevel = NULL;
GtkWidget *central_frame = NULL;
GtkWidget *left_hbox = NULL;
GtkWidget *right_hbox = NULL;
GtkWidget *central_hbox = NULL;

static GtkWidget *create_panel_window(void)
{
    GtkWidget *w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWindow *window = GTK_WINDOW(w);
    GdkPixbuf *pb;

    gtk_window_set_title(window, _("XFce Main Panel"));
    gtk_window_set_decorated(window, FALSE);
    gtk_window_set_resizable(window, FALSE);
    gtk_window_stick(window);

/*    set_window_type_dock(w);*/

    gtk_container_set_border_width(GTK_CONTAINER(w), 0);

    pb = get_system_pixbuf(XFCE_ICON);
    gtk_window_set_icon(window, pb);
    g_object_unref(pb);

    g_signal_connect(w, "destroy-event", G_CALLBACK(panel_destroy_cb), NULL);
    g_signal_connect(w, "delete-event", G_CALLBACK(panel_delete_cb), NULL);

    return w;
}

/* Set up the basic framework for the panel:
 * - window
 * - hbox
 *   - hbox 	: left panel
 *   - frame
 *     - hbox 	: central panel
 *   - hbox 	: right panel
 */
void panel_init(void)
{
    GtkWidget *window;
    GtkWidget *frame1, *frame2;
    GtkWidget *hbox1, *hbox2, *hbox3, *hbox4;

    window = create_panel_window();

    toplevel = window;

    frame1 = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame1), GTK_SHADOW_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(frame1), 0);
    gtk_container_add(GTK_CONTAINER(window), frame1);
    gtk_widget_show(frame1);

    hbox1 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame1), hbox1);
    gtk_widget_show(hbox1);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(hbox1), hbox2);
    gtk_widget_show(hbox2);

    left_hbox = hbox2;

    frame2 = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(hbox1), frame2);
    gtk_widget_show(frame2);

    central_frame = frame2;

    hbox3 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame2), hbox3);
    gtk_widget_show(hbox3);

    central_hbox = hbox3;

    hbox4 = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(hbox1), hbox4);
    gtk_widget_show(hbox4);

    right_hbox = hbox4;
}

void create_xfce_panel(void)
{
    add_side_panel(LEFT, GTK_BOX(left_hbox));
    add_central_panel(GTK_BOX(central_hbox));
    add_side_panel(RIGHT, GTK_BOX(right_hbox));

    panel_set_settings();
    panel_set_position();

    gtk_widget_show(toplevel);
}

void panel_cleanup(void)
{
    if(!disable_user_config)
        write_panel_config();

    side_panel_cleanup(LEFT);
    central_panel_cleanup();
    side_panel_cleanup(RIGHT);

    gtk_widget_destroy(toplevel);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Panel settings
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
Settings settings;

void init_settings(void)
{
    settings.x = -1;
    settings.y = -1;
    settings.size = MEDIUM;
    settings.popup_size = MEDIUM;
    settings.style = NEW_STYLE;
    settings.icon_theme = NULL;
    settings.num_left = 5;
    settings.num_right = 5;
    settings.num_screens = 4;
    settings.current = 0;
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


    if(settings.size == TINY)
        settings.style == NEW_STYLE;

    panel_set_style(settings.style);
    panel_set_icon_theme(settings.icon_theme);

    side_panel_set_num_groups(LEFT, settings.num_left);
    side_panel_set_num_groups(RIGHT, settings.num_right);

    n = get_net_number_of_desktops();

    if(n > settings.num_screens)
        settings.num_screens = n;

    central_panel_set_num_screens(settings.num_screens);

    panel_set_show_central(settings.show_central);
    central_panel_set_show_desktop_buttons(settings.show_desktop_buttons);
    panel_set_show_minibuttons(settings.show_minibuttons);

    if(n < settings.num_screens)
        request_net_number_of_desktops(settings.num_screens);

    n = get_net_current_desktop();

    if(n > 0)
    {
        settings.current = n;
        central_panel_set_current(n);
    }
}

void panel_set_position(void)
{
    GtkRequisition req;
    int w = gdk_screen_width();
    int h = gdk_screen_height();

    gtk_widget_size_request(toplevel, &req);

    if(settings.x == -1 || settings.y == -1)
    {
        settings.x = w / 2 - req.width / 2;
        settings.y = h - req.height;
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

void panel_set_style(int style)
{
    settings.style = style;

    if(style == OLD_STYLE)
        gtk_frame_set_shadow_type(GTK_FRAME(central_frame), GTK_SHADOW_OUT);
    else
        gtk_frame_set_shadow_type(GTK_FRAME(central_frame), GTK_SHADOW_ETCHED_IN);

    side_panel_set_style(LEFT, style);
    central_panel_set_style(style);
    side_panel_set_style(RIGHT, style);
}

void panel_set_icon_theme(const char *theme)
{
    char *tmp = settings.icon_theme;

    settings.icon_theme = g_strdup(theme);
    g_free(tmp);

    side_panel_set_icon_theme(LEFT, theme);
    central_panel_set_icon_theme(theme);
    side_panel_set_icon_theme(RIGHT, theme);
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

void panel_set_current(int n)
{
    settings.current = n;
    central_panel_set_current(n);
}

void panel_set_num_screens(int n)
{
    settings.num_screens = n;
    central_panel_set_num_screens(n);
}

void panel_set_show_central(gboolean show)
{
    settings.show_central = show;

    if (show)
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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Panel configuration
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
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

    value = xmlGetProp(node, (const xmlChar *)"style");

    if(value)
        settings.style = atoi(value);

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"icontheme");

    if(value)
        settings.icon_theme = g_strdup(value);

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

	if (n == 1)
	    settings.show_central = TRUE;
	else
	    settings.show_central = FALSE;
    }

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"desktopbuttons");

    if(value)
    {
	n = atoi(value);

	if (n == 1)
	    settings.show_desktop_buttons = TRUE;
	else
	    settings.show_desktop_buttons = FALSE;
    }

    g_free(value);

    value = xmlGetProp(node, (const xmlChar *)"minibuttons");

    if(value)
    {
	n = atoi(value);

	if (n == 1)
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
    if(settings.size < TINY || settings.size > LARGE)
        settings.size = MEDIUM;
    if(settings.popup_size < SMALL || settings.popup_size > LARGE)
        settings.popup_size = MEDIUM;
    if(settings.num_left < 1 || settings.num_left > NBGROUPS)
        settings.num_left = 5;
    if(settings.num_right < 1 || settings.num_right > NBGROUPS)
        settings.num_right = 5;
    if(settings.num_screens < 1 || settings.num_screens > NBSCREENS)
        settings.num_screens = 4;
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

    snprintf(value, 2, "%d", settings.style);
    xmlSetProp(node, "style", value);

    if(settings.icon_theme)
        xmlSetProp(node, "icontheme", settings.icon_theme);

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
