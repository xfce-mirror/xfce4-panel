/*  wmhints.c
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "xfce.h"
#include "wmhints.h"
#include "dialogs.h"
#include "panel.h"

gboolean atoms_created = FALSE;

Atom xa_NET_CURRENT_DESKTOP = 0;
Atom xa_NET_NUMBER_OF_DESKTOPS = 0;
Atom xa_NET_SUPPORTING_WM_CHECK = 0;

Display *dpy;
Window root;

/* create all atoms we may want to use */
static void create_atoms(void)
{
    dpy = GDK_DISPLAY();
    root = GDK_ROOT_WINDOW();

    xa_NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", FALSE);
    xa_NET_NUMBER_OF_DESKTOPS = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", FALSE);
    xa_NET_SUPPORTING_WM_CHECK = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", FALSE);

    atoms_created = TRUE;
}

/* The _NET_WM_SUPPORTING_WM contains the id of an invisible window
 * that has to contain the same property containing the same id */
void check_net_support(void)
{
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    long *data = 0;
    Window xid;
    int i;

    if(!atoms_created)
        create_atoms();

    /* try 3 times for a compliant window manager */
    for(i = 0; i < 3; i++)
    {
        gdk_error_trap_push();

        if(XGetWindowProperty
           (dpy, root, xa_NET_SUPPORTING_WM_CHECK, 0, 1, False, XA_CARDINAL,
            &ret_type, &fmt, &nitems, &bytes_after,
            (unsigned char **)&data) == Success && data)
        {
/*        xid = (Window) data[0];

        if(XGetWindowProperty
           (dpy, xid, xa_NET_SUPPORTING_WM_CHECK, 0, 1, False, XA_CARDINAL, &ret_type,
            &fmt, &nitems, &bytes_after, (unsigned char **)&data) == Success && data)
        {
            XFree(data);

            if(xid == data[0])
                return;
        }
*/
            XFree(data);

            gdk_flush();
            gdk_error_trap_pop();
            return;
        }
        else if (i < 2)
            g_usleep(2000000); /* wait 2 seconds */

		gdk_flush();
        gdk_error_trap_pop();
    }

    /* fall through */
    report_error(_("The xfce panel needs a window manager that supports the "
                   "extended window manager hints as defined on "
                   "http://www.freedesktop.org.\n"
                   "The panel was designed to run with xfwm4, the window"
                   "manager of the XFce project (http://www.xfce.org)"));
}

/* current desktop */
int get_net_current_desktop(void)
{
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    long *data = 0;
    int n;

    if(!atoms_created)
        create_atoms();

    if(XGetWindowProperty
       (dpy, root, xa_NET_CURRENT_DESKTOP, 0, 1, False, XA_CARDINAL, &ret_type, &fmt,
        &nitems, &bytes_after, (unsigned char **)&data) == Success && data)
    {
        n = (int)data[0];

        XFree(data);
    }
    else
        n = -1;

    return n;
}

/* change current desktop */
void request_net_current_desktop(int n)
{
    XClientMessageEvent sev;

    if(!atoms_created)
        create_atoms();

    sev.type = ClientMessage;
    sev.display = dpy;
    sev.format = 32;
    sev.window = root;
    sev.message_type = xa_NET_CURRENT_DESKTOP;
    sev.data.l[0] = n;

    gdk_error_trap_push();

    XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask,
               (XEvent *) & sev);

    gdk_flush();
    gdk_error_trap_pop();
}

/* number of desktops */
int get_net_number_of_desktops(void)
{
    Atom ret_type;
    int fmt;
    unsigned long nitems, bytes_after;
    long *data = 0;
    int n;

    if(!atoms_created)
        create_atoms();

    if(XGetWindowProperty
       (dpy, root, xa_NET_NUMBER_OF_DESKTOPS, 0, 1, False, XA_CARDINAL, &ret_type,
        &fmt, &nitems, &bytes_after, (unsigned char **)&data) == Success && data)
    {
        n = (int)data[0];

        XFree(data);
    }
    else
        n = -1;

    return n;
}

/* change number of desktops */
void request_net_number_of_desktops(int n)
{
    XClientMessageEvent sev;

    if(!atoms_created)
        create_atoms();

    sev.type = ClientMessage;
    sev.display = dpy;
    sev.format = 32;
    sev.window = root;
    sev.message_type = xa_NET_NUMBER_OF_DESKTOPS;
    sev.data.l[0] = n;

    gdk_error_trap_push();

    XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask,
               (XEvent *) & sev);

    gdk_flush();
    gdk_error_trap_pop();
}

/* next two functions are copied from rox pager */
static void root_property_notify(XPropertyEvent * event)
{
    if(event->atom == xa_NET_CURRENT_DESKTOP)
    {
        int n = get_net_current_desktop();

        if(n >= 0)
            panel_set_current(n);

        return;
    }

    if(event->atom == xa_NET_NUMBER_OF_DESKTOPS)
    {
        int n = get_net_number_of_desktops();

        if(n >= 0)
            panel_set_num_screens(n);

        return;
    }
}

static GdkFilterReturn root_filter(GdkXEvent * xevent, GdkEvent * event,
                                   gpointer data)
{
    XEvent *xev = (XEvent *) xevent;

    if(xev->type == PropertyNotify)
        root_property_notify((XPropertyEvent *) xev);

    return GDK_FILTER_CONTINUE;
}


/* watch for changes in any root property */
void watch_root_properties(void)
{
    GdkWindow *w = gdk_get_default_root_window();

    gdk_window_add_filter(w, root_filter, NULL);
    gdk_window_set_events(w, gdk_window_get_events(w) | GDK_PROPERTY_CHANGE_MASK);
}
