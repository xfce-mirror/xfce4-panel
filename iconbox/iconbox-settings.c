/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libxfce4mcs/mcs-client.h>

#include "iconbox.h"

static GdkFilterReturn
client_event_filter (GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    McsClient *client = iconbox_get_mcs_client (ib);

    if (mcs_client_process_event (client, (XEvent *) xevent))
        return GDK_FILTER_REMOVE;
    else
        return GDK_FILTER_CONTINUE;
}

static void
watch_cb (Window window, Bool is_start, long mask, void *cb_data)
{
    GdkWindow *gdkwin;

    gdkwin = gdk_window_lookup (window);

    if (is_start)
    {
        if (!gdkwin)
        {
            gdkwin = gdk_window_foreign_new (window);
        }
        else
        {
            g_object_ref (gdkwin);
        }
        gdk_window_add_filter (gdkwin, client_event_filter, cb_data);
    }
    else
    {
        g_assert (gdkwin);
        gdk_window_remove_filter (gdkwin, client_event_filter, cb_data);
        g_object_unref (gdkwin);
    }
}

static void
notify_cb (const char *name, const char *channel_name, McsAction action,
           McsSetting * setting, void *data)
{
    Iconbox *ib = (Iconbox *) data;

    g_return_if_fail (data != NULL);
    
    if (action == MCS_ACTION_DELETED)
        return;
 
    if (setting->type == MCS_TYPE_INT)
    {
        if (!strcmp (name, "Iconbox/Justification"))
        {
            if (setting->data.v_int <= GTK_JUSTIFY_LEFT)
                iconbox_set_justification (ib, GTK_JUSTIFY_LEFT);
            else if (setting->data.v_int == GTK_JUSTIFY_RIGHT)
                iconbox_set_justification (ib, GTK_JUSTIFY_RIGHT);
            else if (setting->data.v_int == GTK_JUSTIFY_CENTER)
                iconbox_set_justification (ib, GTK_JUSTIFY_CENTER);
            else
                iconbox_set_justification (ib, GTK_JUSTIFY_FILL);
        }
        else if (!strcmp (name, "Iconbox/Side"))
        {
            if (setting->data.v_int <= GTK_SIDE_TOP)
                iconbox_set_side (ib, GTK_SIDE_TOP);
            else if (setting->data.v_int == GTK_SIDE_BOTTOM)
                iconbox_set_side (ib, GTK_SIDE_BOTTOM);
            else if (setting->data.v_int == GTK_SIDE_LEFT)
                iconbox_set_side (ib, GTK_SIDE_LEFT);
            else
                iconbox_set_side (ib, GTK_SIDE_RIGHT);
        }
        else if (!strcmp (name, "Iconbox/Size"))
        {
            iconbox_set_icon_size (ib, setting->data.v_int);
        }
        else if (!strcmp (name, "Iconbox/OnlyHidden"))
        {
            iconbox_set_show_only_hidden (ib, (setting->data.v_int == 1));
        }
        else if (!strcmp (name, "Iconbox/Transparency"))
        {
            iconbox_set_transparency (ib, setting->data.v_int);
        }
    }
}

McsClient *
iconbox_connect_mcs_client (GdkScreen *screen, Iconbox *ib)
{
    McsClient *client;
    Display *dpy;
    int scr;
    
    scr = gdk_screen_get_number (screen);
    dpy = gdk_x11_display_get_xdisplay (gdk_screen_get_display (screen));
    
    client = mcs_client_new (dpy, scr, notify_cb, watch_cb, ib);

    if (client)
    {
        mcs_client_add_channel (client, "iconbox");
    }
    else
    {
        g_warning ("Can't connect to settings manager");
    }

    return client;
}

void 
iconbox_disconnect_mcs_client (McsClient *client)
{
	mcs_client_destroy (client);

	client = NULL;
}

void
mcs_open_dialog (GdkScreen *screen, const char *channel)
{
    if (channel)
    {
        int scr = gdk_screen_get_number (screen);
        Display *dpy = 
            gdk_x11_display_get_xdisplay (gdk_screen_get_display (screen));
        
	mcs_client_show (dpy, scr, channel);
    }
}
