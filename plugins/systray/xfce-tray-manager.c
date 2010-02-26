/* $Id$ */
/*
 * Copyright (c) 2002      Anders Carlsson <andersca@gnu.org>
 * Copyright (c) 2003-2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2003-2004 Olivier Fourdan <fourdan@xfce.org>
 * Copyright (c) 2003-2006 Vincent Untz
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-tray-manager.h"
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
#include "xfce-tray-marshal.h"
#endif



#define XFCE_TRAY_MANAGER_REQUEST_DOCK   0
#define XFCE_TRAY_MANAGER_BEGIN_MESSAGE  1
#define XFCE_TRAY_MANAGER_CANCEL_MESSAGE 2

#define XFCE_TRAY_MANAGER_ORIENTATION_HORIZONTAL 0
#define XFCE_TRAY_MANAGER_ORIENTATION_VERTICAL   1



/* prototypes */
static void                 xfce_tray_manager_finalize                           (GObject              *object);
static void                 xfce_tray_manager_remove_socket                      (gpointer              key,
                                                                                  gpointer              value,
                                                                                  gpointer              user_data);
static GdkFilterReturn      xfce_tray_manager_window_filter                      (GdkXEvent            *xev,
                                                                                  GdkEvent             *event,
                                                                                  gpointer              user_data);
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
static GdkFilterReturn      xfce_tray_manager_handle_client_message_opcode       (GdkXEvent            *xevent,
                                                                                  GdkEvent             *event,
                                                                                  gpointer              user_data);
static GdkFilterReturn      xfce_tray_manager_handle_client_message_message_data (GdkXEvent            *xevent,
                                                                                  GdkEvent             *event,
                                                                                  gpointer              user_data);
static void                 xfce_tray_manager_handle_begin_message               (XfceTrayManager      *manager,
                                                                                  XClientMessageEvent  *xevent);
static void                 xfce_tray_manager_handle_cancel_message              (XfceTrayManager      *manager,
                                                                                  XClientMessageEvent  *xevent);
#endif
static void                 xfce_tray_manager_handle_dock_request                (XfceTrayManager      *manager,
                                                                                  XClientMessageEvent  *xevent);
static gboolean             xfce_tray_manager_handle_undock_request              (GtkSocket            *socket,
                                                                                  gpointer              user_data);
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
static void                 xfce_tray_message_free                               (XfceTrayMessage      *message);
static void                 xfce_tray_message_remove_from_list                   (XfceTrayManager      *manager,
                                                                                  XClientMessageEvent  *xevent);
#endif


enum
{
  TRAY_ICON_ADDED,
  TRAY_ICON_REMOVED,
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
  TRAY_MESSAGE_SENT,
  TRAY_MESSAGE_CANCELLED,
#endif
  TRAY_LOST_SELECTION,
  LAST_SIGNAL
};

struct _XfceTrayManagerClass
{
    GObjectClass __parent__;
};

struct _XfceTrayManager
{
    GObject __parent__;

    /* invisible window */
    GtkWidget      *invisible;

    /* list of client sockets */
    GHashTable     *sockets;

    /* orientation of the tray */
    GtkOrientation  orientation;

#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
    /* list of pending messages */
    GSList         *messages;
#endif

    /* _net_system_tray_opcode atom */
    Atom            opcode_atom;

    /* _net_system_tray_s%d atom */
    GdkAtom         selection_atom;
};

#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
struct _XfceTrayMessage
{
    /* message string */
    gchar          *string;

    /* message id */
    glong           id;

    /* x11 window */
    Window          window;

    /* numb3rs */
    glong           length;
    glong           remaining_length;
    glong           timeout;
};
#endif



static guint xfce_tray_manager_signals[LAST_SIGNAL];



G_DEFINE_TYPE (XfceTrayManager, xfce_tray_manager, G_TYPE_OBJECT);



static void
xfce_tray_manager_class_init (XfceTrayManagerClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = xfce_tray_manager_finalize;

    xfce_tray_manager_signals[TRAY_ICON_ADDED] =
        g_signal_new (I_("tray-icon-added"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      GTK_TYPE_SOCKET);

    xfce_tray_manager_signals[TRAY_ICON_REMOVED] =
        g_signal_new (I_("tray-icon-removed"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      GTK_TYPE_SOCKET);

#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
    xfce_tray_manager_signals[TRAY_MESSAGE_SENT] =
        g_signal_new (I_("tray-message-sent"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      _xfce_tray_marshal_VOID__OBJECT_STRING_LONG_LONG,
                      G_TYPE_NONE, 4,
                      GTK_TYPE_SOCKET,
                      G_TYPE_STRING,
                      G_TYPE_LONG,
                      G_TYPE_LONG);

    xfce_tray_manager_signals[TRAY_MESSAGE_CANCELLED] =
        g_signal_new (I_("tray-message-cancelled"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      _xfce_tray_marshal_VOID__OBJECT_LONG,
                      G_TYPE_NONE, 2,
                      GTK_TYPE_SOCKET,
                      G_TYPE_LONG);
#endif

    xfce_tray_manager_signals[TRAY_LOST_SELECTION] =
        g_signal_new (I_("tray-lost-selection"),
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}



static void
xfce_tray_manager_init (XfceTrayManager *manager)
{
  /* initialize */
  manager->invisible = NULL;
  manager->orientation = GTK_ORIENTATION_HORIZONTAL;
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
  manager->messages = NULL;
#endif

  /* create new sockets table */
  manager->sockets = g_hash_table_new (NULL, NULL);
}



GQuark
xfce_tray_manager_error_quark (void)
{
    static GQuark q = 0;

    if (q == 0)
    {
        q = g_quark_from_static_string ("xfce-tray-manager-error-quark");
    }

    return q;
}



static void
xfce_tray_manager_finalize (GObject *object)
{
    XfceTrayManager *manager = XFCE_TRAY_MANAGER (object);

    /* destroy the hash table */
    g_hash_table_destroy (manager->sockets);

#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
    if (manager->messages)
    {
        /* cleanup all pending messages */
        g_slist_foreach (manager->messages, (GFunc) xfce_tray_message_free, NULL);

        /* free the list */
        g_slist_free (manager->messages);
    }
#endif

    G_OBJECT_CLASS (xfce_tray_manager_parent_class)->finalize (object);
}



XfceTrayManager *
xfce_tray_manager_new (void)
{
    return g_object_new (XFCE_TYPE_TRAY_MANAGER, NULL);
}



gboolean
xfce_tray_manager_check_running (GdkScreen *screen)
{
    gchar      *selection_name;
    GdkDisplay *display;
    Atom        selection_atom;

    panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

    /* get the display */
    display = gdk_screen_get_display (screen);

    /* create the selection atom name */
    selection_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d", gdk_screen_get_number (screen));

    /* get the atom */
    selection_atom = gdk_x11_get_xatom_by_name_for_display (display, selection_name);

    /* cleanup */
    g_free (selection_name);

    /* return result */
    return (XGetSelectionOwner (GDK_DISPLAY_XDISPLAY (display), selection_atom) != None);
}



gboolean
xfce_tray_manager_register (XfceTrayManager  *manager,
                            GdkScreen        *screen,
                            GError          **error)
{
    GdkDisplay          *display;
    gchar               *selection_name;
    gboolean             succeed;
    gint                 screen_number;
    GtkWidget           *invisible;
    guint32              timestamp;
    GdkAtom              opcode_atom;
    XClientMessageEvent  xevent;
    Window               root_window;

    panel_return_val_if_fail (XFCE_IS_TRAY_MANAGER (manager), FALSE);
    panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
    panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    /* create invisible window */
    invisible = gtk_invisible_new_for_screen (screen);
    gtk_widget_realize (invisible);

    /* let the invisible window monitor property and configuration changes */
    gtk_widget_add_events (invisible, GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK);

    /* get the screen number */
    screen_number = gdk_screen_get_number (screen);

    /* create the selection atom name */
    selection_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d", screen_number);

    /* get the selection atom */
    manager->selection_atom = gdk_atom_intern (selection_name, FALSE);

    /* cleanup */
    g_free (selection_name);

    /* get the display */
    display = gdk_screen_get_display (screen);

    /* get the current x server time stamp */
    timestamp = gdk_x11_get_server_time (invisible->window);

    /* try to become the selection owner of this display */
    succeed = gdk_selection_owner_set_for_display (display, invisible->window, manager->selection_atom, timestamp, TRUE);

    if (G_LIKELY (succeed))
    {
        /* get the root window */
        root_window = RootWindowOfScreen (GDK_SCREEN_XSCREEN (screen));

        /* send a message to x11 that we're going to handle this display */
        xevent.type         = ClientMessage;
        xevent.window       = root_window;
        xevent.message_type = gdk_x11_get_xatom_by_name_for_display (display, "MANAGER");
        xevent.format       = 32;
        xevent.data.l[0]    = timestamp;
        xevent.data.l[1]    = gdk_x11_atom_to_xatom_for_display (display, manager->selection_atom);
        xevent.data.l[2]    = GDK_WINDOW_XWINDOW (invisible->window);
        xevent.data.l[3]    = 0;
        xevent.data.l[4]    = 0;

        /* send the message */
        XSendEvent (GDK_DISPLAY_XDISPLAY (display), root_window, False, StructureNotifyMask, (XEvent *)&xevent);

        /* set the invisible window and take a reference */
        manager->invisible = g_object_ref (G_OBJECT (invisible));

        /* system_tray_request_dock and selectionclear */
        gdk_window_add_filter (invisible->window, xfce_tray_manager_window_filter, manager);

        /* get the opcode atom (for both gdk and x11) */
        opcode_atom = gdk_atom_intern ("_NET_SYSTEM_TRAY_OPCODE", FALSE);
        manager->opcode_atom = gdk_x11_atom_to_xatom_for_display (display, opcode_atom);

#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
        /* system_tray_begin_message and system_tray_cancel_message */
        gdk_display_add_client_message_filter (display,
                                               opcode_atom,
                                               xfce_tray_manager_handle_client_message_opcode,
                                               manager);

        /* _net_system_tray_message_data */
        gdk_display_add_client_message_filter (display,
                                               gdk_atom_intern ("_NET_SYSTEM_TRAY_MESSAGE_DATA", FALSE),
                                               xfce_tray_manager_handle_client_message_message_data,
                                               manager);
#endif
    }
    else
    {
        /* desktroy the invisible window */
        gtk_widget_destroy (invisible);

        /* set an error */
        g_set_error (error, XFCE_TRAY_MANAGER_ERROR, XFCE_TRAY_MANAGER_ERROR_SELECTION_FAILED,
                     _("Failed to acquire manager selection for screen %d"), screen_number);
    }

    return succeed;
}



static void
xfce_tray_manager_remove_socket (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
    XfceTrayManager *manager = XFCE_TRAY_MANAGER (user_data);
    GtkSocket       *socket = GTK_SOCKET (value);

    panel_return_if_fail (XFCE_IS_TRAY_MANAGER (manager));
    panel_return_if_fail (GTK_IS_SOCKET (socket));

    /* properly undock from the tray */
    g_signal_emit (manager, xfce_tray_manager_signals[TRAY_ICON_REMOVED], 0, socket);
}



void
xfce_tray_manager_unregister (XfceTrayManager *manager)
{
    GdkDisplay *display;
    GtkWidget  *invisible = manager->invisible;

    /* leave when there is no invisible window */
    if (invisible == NULL)
        return;

    panel_return_if_fail (GTK_IS_INVISIBLE (invisible));
    panel_return_if_fail (GTK_WIDGET_REALIZED (invisible));
    panel_return_if_fail (GDK_IS_WINDOW (invisible->window));

    /* get the display of the invisible window */
    display = gtk_widget_get_display (invisible);

    /* remove our handling of the selection if we're the owner */
    if (gdk_selection_owner_get_for_display (display, manager->selection_atom) == invisible->window)
    {
        /* reset the selection owner */
        gdk_selection_owner_set_for_display (display,
                                             NULL,
                                             manager->selection_atom,
                                             gdk_x11_get_server_time (invisible->window),
                                             TRUE);
    }

    /* remove window filter */
    gdk_window_remove_filter (invisible->window, xfce_tray_manager_window_filter, manager);

    /* remove all sockets from the hash table */
    g_hash_table_foreach (manager->sockets, xfce_tray_manager_remove_socket, manager);

    /* destroy and unref the invisible window */
    manager->invisible = NULL;
    gtk_widget_destroy (invisible);
    g_object_unref (G_OBJECT (invisible));
}



static GdkFilterReturn
xfce_tray_manager_window_filter (GdkXEvent *xev,
                                 GdkEvent  *event,
                                 gpointer   user_data)
{
    XEvent          *xevent = (XEvent *)xev;
    XfceTrayManager *manager = XFCE_TRAY_MANAGER (user_data);

    if (xevent->type == ClientMessage)
    {
        if (xevent->xclient.message_type == manager->opcode_atom
            && xevent->xclient.data.l[1] == XFCE_TRAY_MANAGER_REQUEST_DOCK)
        {
            /* dock a tray icon */
            xfce_tray_manager_handle_dock_request (manager, (XClientMessageEvent *) xevent);

            return GDK_FILTER_REMOVE;
        }
    }
    else if (xevent->type == SelectionClear)
    {
        /* emit the signal */
        g_signal_emit (manager, xfce_tray_manager_signals[TRAY_LOST_SELECTION], 0);

        /* unregister the manager */
        xfce_tray_manager_unregister (manager);
    }

    return GDK_FILTER_CONTINUE;
}



#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
static GdkFilterReturn
xfce_tray_manager_handle_client_message_opcode (GdkXEvent *xevent,
                                                GdkEvent  *event,
                                                gpointer   user_data)
{
    XClientMessageEvent *xev;
    XfceTrayManager     *manager = XFCE_TRAY_MANAGER (user_data);

    panel_return_val_if_fail (XFCE_IS_TRAY_MANAGER (manager), GDK_FILTER_REMOVE);

    /* cast to x11 event */
    xev = (XClientMessageEvent *) xevent;

    switch (xev->data.l[1])
    {
        case XFCE_TRAY_MANAGER_REQUEST_DOCK:
            /* handled in xfce_tray_manager_window_filter () */
            break;

        case XFCE_TRAY_MANAGER_BEGIN_MESSAGE:
            xfce_tray_manager_handle_begin_message (manager, xev);
            return GDK_FILTER_REMOVE;

        case XFCE_TRAY_MANAGER_CANCEL_MESSAGE:
            xfce_tray_manager_handle_cancel_message (manager, xev);
            return GDK_FILTER_REMOVE;

        default:
            break;
    }

    return GDK_FILTER_CONTINUE;
}



static GdkFilterReturn
xfce_tray_manager_handle_client_message_message_data (GdkXEvent *xevent,
                                                      GdkEvent  *event,
                                                      gpointer   user_data)
{
    XClientMessageEvent *xev;
    XfceTrayManager     *manager = XFCE_TRAY_MANAGER (user_data);
    GSList              *li;
    XfceTrayMessage     *message;
    glong                length;
    GtkSocket           *socket;

    panel_return_val_if_fail (XFCE_IS_TRAY_MANAGER (manager), GDK_FILTER_REMOVE);

    /* try to find the pending message in the list */
    for (li = manager->messages; li != NULL; li = li->next)
    {
        message = li->data;

        if (xev->window == message->window)
        {
            /* copy the data of this message */
            length = MIN (message->remaining_length, 20);
            memcpy ((message->string + message->length - message->remaining_length), &xev->data, length);
            message->remaining_length -= length;

            /* check if we have the complete message */
            if (message->remaining_length == 0)
            {
                /* try to get the socket from the known tray icons */
                socket = g_hash_table_lookup (manager->sockets, GUINT_TO_POINTER (message->window));

                if (G_LIKELY (socket))
                {
                    /* known socket, send the signal */
                    g_signal_emit (manager, xfce_tray_manager_signals[TRAY_MESSAGE_SENT], 0,
                                   socket, message->string, message->id, message->timeout);
                }

                /* delete the message from the list */
                manager->messages = g_slist_delete_link (manager->messages, li);

                /* free the message */
                xfce_tray_message_free (message);
            }

            /* stop searching */
            break;
        }
    }

    return GDK_FILTER_REMOVE;
}



static void
xfce_tray_manager_handle_begin_message (XfceTrayManager     *manager,
                                        XClientMessageEvent *xevent)
{
    GtkSocket       *socket;
    XfceTrayMessage *message;
    glong            length, timeout, id;

    /* try to find the window in the list of known tray icons */
    socket = g_hash_table_lookup (manager->sockets, GUINT_TO_POINTER (xevent->window));

    /* unkown tray icon: ignore the message */
    if (G_UNLIKELY (socket == NULL))
       return;

    /* remove the same message from the list */
    xfce_tray_message_remove_from_list (manager, xevent);

    /* get some message information */
    timeout = xevent->data.l[2];
    length  = xevent->data.l[3];
    id      = xevent->data.l[4];

    if (length == 0)
    {
        /* directly emit empty messages */
        g_signal_emit (manager, xfce_tray_manager_signals[TRAY_MESSAGE_SENT], 0,
                       socket, "", id, timeout);
    }
    else
    {
        /* create new structure */
        message = g_slice_new0 (XfceTrayMessage);

        /* set message data */
        message->window           = xevent->window;
        message->timeout          = timeout;
        message->length           = length;
        message->id               = id;
        message->remaining_length = length;
        message->string           = g_malloc (length + 1);
        message->string[length]   = '\0';

        /* add this message to the list of pending messages */
        manager->messages = g_slist_prepend (manager->messages, message);
    }
}



static void
xfce_tray_manager_handle_cancel_message (XfceTrayManager     *manager,
                                         XClientMessageEvent *xevent)
{
    GtkSocket *socket;

    /* remove the same message from the list */
    xfce_tray_message_remove_from_list (manager, xevent);

    /* try to find the window in the list of known tray icons */
    socket = g_hash_table_lookup (manager->sockets, GUINT_TO_POINTER (xevent->window));

    if (G_LIKELY (socket))
    {
        /* emit the cancelled signal */
        g_signal_emit (manager, xfce_tray_manager_signals[TRAY_MESSAGE_CANCELLED], 0,
                       socket, xevent->data.l[2]);
    }
}
#endif



static void
xfce_tray_manager_handle_dock_request (XfceTrayManager     *manager,
                                       XClientMessageEvent *xevent)
{
    GtkWidget *socket;
    Window    *xwindow;

    /* check if we already have this notification */
    if (g_hash_table_lookup (manager->sockets, GUINT_TO_POINTER (xevent->data.l[2])))
        return;

    /* create a new socket */
    socket = gtk_socket_new ();

    /* allow applications to draw on this widget */
    gtk_widget_set_app_paintable (socket, TRUE);

    /* allocate and set the xwindow */
    xwindow = g_new (Window, 1);
    *xwindow = xevent->data.l[2];

    /* connect the xwindow data to the socket */
    g_object_set_data_full (G_OBJECT (socket), I_("xfce-tray-manager-xwindow"), xwindow, g_free);

    /* add the icon to the tray */
    g_signal_emit (manager, xfce_tray_manager_signals[TRAY_ICON_ADDED], 0, socket);

    /* check if the widget has been attached. if the widget has no
       toplevel window, we cannot set the socket id. */
    if (G_LIKELY (GTK_IS_WINDOW (gtk_widget_get_toplevel (socket))))
    {
        /* signal to monitor if the client is removed from the socket */
        g_signal_connect (G_OBJECT (socket), "plug-removed", G_CALLBACK (xfce_tray_manager_handle_undock_request), manager);

        /* register the xembed client window id for this socket */
        gtk_socket_add_id (GTK_SOCKET (socket), *xwindow);

        /* add the socket to the list of known sockets */
        g_hash_table_insert (manager->sockets, GUINT_TO_POINTER (*xwindow), socket);
    }
    else
    {
        /* warning */
        g_warning ("No parent window set, destroying socket");

        /* not attached successfully, destroy it */
        gtk_widget_destroy (socket);
    }
}



static gboolean
xfce_tray_manager_handle_undock_request (GtkSocket *socket,
                                         gpointer   user_data)
{
    XfceTrayManager *manager = XFCE_TRAY_MANAGER (user_data);
    Window          *xwindow;

    panel_return_val_if_fail (XFCE_IS_TRAY_MANAGER (manager), FALSE);

    /* emit signal that the socket will be removed */
    g_signal_emit (manager, xfce_tray_manager_signals[TRAY_ICON_REMOVED], 0, socket);

    /* get the xwindow */
    xwindow = g_object_get_data (G_OBJECT (socket), I_("xfce-tray-manager-xwindow"));

    /* remove the socket from the list */
    g_hash_table_remove (manager->sockets, GUINT_TO_POINTER (*xwindow));

    /* unset object data */
    g_object_set_data (G_OBJECT (socket), I_("xfce-tray-manager-xwindow"), NULL);

    /* destroy the socket */
    return FALSE;
}



GtkOrientation
xfce_tray_manager_get_orientation (XfceTrayManager *manager)
{
    panel_return_val_if_fail (XFCE_IS_TRAY_MANAGER (manager), GTK_ORIENTATION_HORIZONTAL);

    return manager->orientation;
}



void
xfce_tray_manager_set_orientation (XfceTrayManager *manager,
                                   GtkOrientation   orientation)
{
    GdkDisplay *display;
    Atom        orientation_atom;
    gulong      data[1];

    panel_return_if_fail (XFCE_IS_TRAY_MANAGER (manager));
    panel_return_if_fail (GTK_IS_INVISIBLE (manager->invisible));

    if (G_LIKELY (manager->orientation != orientation))
    {
        /* set the new orientation */
        manager->orientation = orientation;

        /* get invisible display */
        display = gtk_widget_get_display (manager->invisible);

        /* get the xatom for the orientation property */
        orientation_atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_XFCE_TRAY_MANAGER_ORIENTATION");

        /* set the data we're going to send to x */
        data[0] = (manager->orientation == GTK_ORIENTATION_HORIZONTAL ?
                   XFCE_TRAY_MANAGER_ORIENTATION_HORIZONTAL : XFCE_TRAY_MANAGER_ORIENTATION_VERTICAL);

        /* change the x property */
        XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
                         GDK_WINDOW_XWINDOW (manager->invisible->window),
                         orientation_atom,
                         XA_CARDINAL, 32,
                         PropModeReplace,
                         (guchar *) &data, 1);
    }
}



gchar *
xfce_tray_manager_get_application_name (GtkWidget *socket)
{
    gchar         *name = NULL;
    GdkDisplay    *display;
    gint           succeed;
    XTextProperty  xprop;
    Window        *xwindow;

    /* get the xwindow */
    xwindow = g_object_get_data (G_OBJECT (socket), I_("xfce-tray-manager-xwindow"));

    if (G_LIKELY (xwindow))
    {
        /* get the display of the socket */
        display = gtk_widget_get_display (socket);

        /* avoid exiting the application on X errors */
        gdk_error_trap_push ();

        /* try to get the wm name (this is more relaiable with qt applications) */
        succeed = XGetWMName (GDK_DISPLAY_XDISPLAY (display), *xwindow, &xprop);

        /* check if everything went fine */
        if (G_LIKELY (gdk_error_trap_pop () == 0 && succeed >= Success))
        {
            /* check the xprop content */
            if (G_LIKELY (xprop.value && xprop.nitems > 0))
            {
                /* if the string is utf-8 valid, set the name */
                if (G_LIKELY (g_utf8_validate ((const gchar *) xprop.value, xprop.nitems, NULL)))
                {
                    /* create the application name (lower case) */
                    name = g_utf8_strdown ((const gchar *) xprop.value, xprop.nitems);
                }

                /* cleanup */
                XFree (xprop.value);
            }
        }
    }

    return name;
}



/**
 * tray messages
 **/
#if XFCE_TRAY_MANAGER_ENABLE_MESSAGES
static void
xfce_tray_message_free (XfceTrayMessage *message)
{
    /* cleanup */
    g_free (message->string);

    /* remove slice */
    g_slice_free (XfceTrayMessage, message);
}



static void
xfce_tray_message_remove_from_list (XfceTrayManager     *manager,
                                    XClientMessageEvent *xevent)
{
    GSList          *li;
    XfceTrayMessage *message;

    /* seach for the same message in the list of pending messages */
    for (li = manager->messages; li != NULL; li = li->next)
    {
        message = li->data;

        /* check if this is the same message */
        if (xevent->window == message->window && xevent->data.l[4] == message->id)
        {
            /* delete the message from the list */
            manager->messages = g_slist_delete_link (manager->messages, li);

            /* free the message */
            xfce_tray_message_free (message);

            break;
        }
    }
}
#endif
