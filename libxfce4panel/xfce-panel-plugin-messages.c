/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include "xfce-panel-plugin-messages.h"

/* public API */

/**
 * xfce_panel_plugin_message_send
 * @from   : #GdkWindow belonging to sender
 * @xid    : #GdkNativeWindow id of receiver
 * @message: %PanelAppMesssage
 *
 * Send a message to or from a panel plugin.
 **/
void
xfce_panel_plugin_message_send (GdkWindow *from, GdkNativeWindow xid, 
                                XfcePanelPluginMessage message, int value)
{
    GdkEventClient gev;

    gev.type = GDK_CLIENT_EVENT;
    gev.window = from;
    gev.send_event = TRUE;
    gev.message_type = gdk_atom_intern (XFCE_PANEL_PLUGIN_ATOM, FALSE);
    gev.data_format = 16;
    gev.data.s[0] = message;
    gev.data.s[1] = value;
    gev.data.s[2] = 0;

    gdk_event_send_client_message ((GdkEvent *) & gev, xid);
    gdk_flush ();
}

