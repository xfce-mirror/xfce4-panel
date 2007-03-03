/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "panel-app.h"
#include "panel-app-messages.h"

#ifndef _
#define _(x) x
#endif

#define PANEL_APP_ATOM "XFCE4_PANEL_APP"

/* client messages */

static gboolean
client_event_received (GtkWidget      *win,
                       GdkEventClient *ev)
{
    GdkAtom atom = gdk_atom_intern (PANEL_APP_ATOM, FALSE);

    if (ev->message_type == atom)
    {
        switch (ev->data.s[0])
        {
            case PANEL_APP_CUSTOMIZE:
                panel_app_customize ();
                break;
            case PANEL_APP_SAVE:
                panel_app_save ();
                break;
            case PANEL_APP_RESTART:
                panel_app_restart ();
                break;
            case PANEL_APP_QUIT:
                panel_app_quit ();
                break;
            case PANEL_APP_EXIT:
                panel_app_quit_noconfirm ();
                break;
            case PANEL_APP_ADD:
                panel_app_customize_items (NULL);
                break;
            default:
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

/* public API */

/**
 * panel_app_send
 * @message: %PanelAppMesssage
 *
 * Send a message to a running instance of xfce4-panel.
 **/
void
panel_app_send (PanelAppMessage message)
{
    Window win;
    GdkEventClient gev;
    GtkWidget *invisible;

    if (panel_app_init () != 1)
    {
        g_warning ("xfce4-panel is not running");
        return;
    }

    win = panel_app_get_ipc_window ();

    if (win)
    {
        invisible = gtk_invisible_new ();
        gtk_widget_realize (invisible);

        gev.type         = GDK_CLIENT_EVENT;
        gev.window       = invisible->window;
        gev.send_event   = TRUE;
        gev.message_type = gdk_atom_intern (PANEL_APP_ATOM, FALSE);
        gev.data_format  = 16;
        gev.data.s[0]    = message;
        gev.data.s[1]    = 0;

        gdk_event_send_client_message ((GdkEvent *) & gev,
                                       (GdkNativeWindow) win);
        gdk_flush ();

        gtk_widget_destroy (invisible);
    }
}

/**
 * panel_app_listen
 * @ipc_window: #GtkWidget that will receive messages.
 *
 * Set up listeners for messages to @ipc_window.
 **/
void
panel_app_listen (GtkWidget *ipc_window)
{
    g_signal_connect (G_OBJECT (ipc_window), "client-event",
                      G_CALLBACK (client_event_received), NULL);
}

