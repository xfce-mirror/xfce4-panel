/*  xfce4
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

#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include <libxfce4mcs/mcs-client.h>
#include <libxfcegui4/libnetk.h>

#include "global.h"
#include "panel.h"
#include "xfce_support.h"
#include "mcs_client.h"
#include "../settings/xfce_settings.h"

#define CHANNEL "xfce"

static McsClient *client = NULL;

/* special case: position setting */
static void mcs_position_setting(int pos)
{
    int width, height;
    DesktopMargins margins;
    Screen *xscreen;
    static int x, y;

    if (pos == XFCE_POSITION_NONE || !toplevel)
	return;

    if (pos == XFCE_POSITION_SAVE)
    {
	gtk_window_get_position(GTK_WINDOW(toplevel), &x, &y);
	return;
    }

    if (pos == XFCE_POSITION_RESTORE)
    {
	position.x = x;
	position.y = y;
        gtk_window_move(GTK_WINDOW(toplevel), x, y);

	return;
    }
    
    xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
    netk_get_desktop_margins(xscreen, &margins);

    gtk_window_get_size(GTK_WINDOW(toplevel), &width, &height);
    
    switch (pos)
    {
	case XFCE_POSITION_BOTTOM:
	    position.x = gdk_screen_width() / 2 - width / 2;
	    position.y = gdk_screen_height() - height - margins.bottom;
	    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
	    break;
	case XFCE_POSITION_TOP:
	    position.y = margins.top;
	    position.x = gdk_screen_width() / 2 - width / 2;
	    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
	    break;
	case XFCE_POSITION_LEFT:
	    position.x = margins.left;
	    position.y = gdk_screen_height() / 2 - height / 2;
	    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
	    break;
	case XFCE_POSITION_RIGHT:
	    position.x = gdk_screen_width() - width - margins.right;
	    position.y = gdk_screen_height() / 2 - height / 2;
	    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
	    break;
    }
}

/* settings hash table */
static GHashTable *settings_hash = NULL;

/* IMPORTATNT: keep in sync with settings names list */
static gpointer settings_callbacks [] = {
    panel_set_orientation,
    panel_set_layer,
    panel_set_size,
    panel_set_popup_position,
    panel_set_style,
    panel_set_theme,
/*    panel_set_num_groups,*/
    mcs_position_setting
};

static void init_settings_hash(void)
{
    int i;
    
    settings_hash = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < XFCE_OPTIONS; i++)
    {
	g_hash_table_insert(settings_hash, 
		            xfce_settings_names[i], settings_callbacks[i]);
    }
}

/* callback functions */
static void update_setting(const char *name, McsSetting *setting)
{
    void (*set_int)(int n);
    void (*set_string)(char *str);

    if (setting->type == MCS_TYPE_INT)
    {
	set_int = g_hash_table_lookup(settings_hash, name);

	if (set_int)
	    set_int(setting->data.v_int);
    }
    else if (setting->type == MCS_TYPE_STRING)
    {
	set_string = g_hash_table_lookup(settings_hash, name);

	if (set_string)
	    set_string(setting->data.v_string);
    }
}

static void notify_cb(const char *name, const char *channel_name, McsAction action, McsSetting * setting, void *data)
{
    if (g_ascii_strcasecmp(CHANNEL, channel_name))
	return;
    
    switch (action)
    {
        case MCS_ACTION_NEW:
	    /* fall through */
        case MCS_ACTION_CHANGED:
	    update_setting(name, setting);
            break;
        case MCS_ACTION_DELETED:
	    /* We don't use this now. Perhaps revert to default? */
            break;
    }
}

GdkFilterReturn client_event_filter(GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
    if(mcs_client_process_event(client, (XEvent *) xevent))
        return GDK_FILTER_REMOVE;
    else
        return GDK_FILTER_CONTINUE;
}

static void watch_cb(Window window, Bool is_start, long mask, void *cb_data)
{
    GdkWindow *gdkwin;

    gdkwin = gdk_window_lookup(window);

    if(is_start)
        gdk_window_add_filter(gdkwin, client_event_filter, NULL);
    else
        gdk_window_remove_filter(gdkwin, client_event_filter, NULL);
}

/* initialize settings */
static void init_one_setting(const char *name)
{
    McsSetting *setting;

    if (MCS_SUCCESS == mcs_client_get_setting(client, name, CHANNEL, &setting))
    {
	update_setting(name, setting);
	mcs_setting_free(setting);
    }
}

void mcs_init_settings(void)
{
    if (!settings_hash)
	init_settings_hash();

    g_hash_table_foreach(settings_hash, (GHFunc) init_one_setting, NULL);
}

/* connecting and disconnecting */
void mcs_watch_xfce_channel(void)
{
    Display *dpy = GDK_DISPLAY();
    int screen = DefaultScreen(dpy);

    if (!settings_hash)
	init_settings_hash();
    
    if (!mcs_client_check_manager(dpy, screen, "xfce-mcs-manager"))
	g_critical(_("MCS settings manager not running!"));
    
    client = mcs_client_new(dpy, screen, notify_cb, watch_cb, NULL);
       
    if(!client)
    {
        g_critical(_("xfce4: could not connect to settings manager!" 
		     "Please check your installation."));

	show_error(_("The XFce panel could not connect to the settings \n"
		       "manager.\n"
		       "Please make sure it is installed on your system."));

	return;
    }
 
    mcs_client_add_channel(client, CHANNEL);
}

void mcs_stop_watch(void)
{
    if (client)
	mcs_client_destroy(client);

    client = NULL;
}

void mcs_dialog(const char *channel)
{
    if (!client)
	return;

    if (channel)
	mcs_client_show(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()), channel);
    else
	mcs_client_show(GDK_DISPLAY(), DefaultScreen(GDK_DISPLAY()), CHANNEL);
}

