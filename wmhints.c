#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "xfce.h"
#include "wmhints.h"

Atom xa_NET_CURRENT_DESKTOP = 0;
Atom xa_NET_NUMBER_OF_DESKTOPS = 0;

Display *dpy;
Window root;

void
create_atoms (void)
{
  dpy = GDK_DISPLAY ();
  root = GDK_ROOT_WINDOW ();

  xa_NET_CURRENT_DESKTOP = XInternAtom (dpy, "_NET_CURRENT_DESKTOP", FALSE);
  xa_NET_NUMBER_OF_DESKTOPS =
    XInternAtom (dpy, "_NET_NUMBER_OF_DESKTOPS", FALSE);
}

void
net_wm_support_check (void)
{
}

void
net_current_desktop_set (int n)
{
  XClientMessageEvent sev;

  sev.type = ClientMessage;
  sev.display = dpy;
  sev.format = 32;
  sev.window = root;
  sev.message_type = xa_NET_CURRENT_DESKTOP;
  sev.data.l[0] = n;

  gdk_error_trap_push ();

  XSendEvent (dpy, root, False,
	      SubstructureNotifyMask | SubstructureRedirectMask,
	      (XEvent *) & sev);

  gdk_flush ();
  gdk_error_trap_pop ();
}

void
net_number_of_desktops_set (int n)
{
  XClientMessageEvent sev;

  sev.type = ClientMessage;
  sev.display = dpy;
  sev.format = 32;
  sev.window = root;
  sev.message_type = xa_NET_NUMBER_OF_DESKTOPS;
  sev.data.l[0] = n;

  gdk_error_trap_push ();

  XSendEvent (dpy, root, False,
	      SubstructureNotifyMask | SubstructureRedirectMask,
	      (XEvent *) & sev);

  gdk_flush ();
  gdk_error_trap_pop ();
}

void
net_desktop_name_set (int n, const char *name)
{
}

int 
net_current_desktop_get (void)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems, bytes_after;
  long *data = 0;
  int n;
  
  if (XGetWindowProperty (dpy, root, xa_NET_CURRENT_DESKTOP, 
			  0, 1, False, XA_CARDINAL, 
			  &ret_type, &fmt, &nitems, &bytes_after, 
			  (unsigned char **) &data)
      == Success && data)
    {
      n = (int) data[0];
      
      XFree (data);
    }
  else
    n = -1;
  
  return n;
}

int net_number_of_desktops_get (void)
{
  Atom ret_type;
  int fmt;
  unsigned long nitems, bytes_after;
  long *data = 0;
  int n;
  
  if (XGetWindowProperty (dpy, root, xa_NET_NUMBER_OF_DESKTOPS, 
			  0, 1, False, XA_CARDINAL, 
			  &ret_type, &fmt, &nitems, &bytes_after, 
			  (unsigned char **) &data)
      == Success && data)
    {
      n = (int) data[0];
      
      XFree (data);
    }
  else
    n = -1;
  
  return n;
}

char **net_desktop_name_get (void)
{
  return NULL;
}

static void
root_property_notify (XPropertyEvent * event)
{
  if (event->atom == xa_NET_CURRENT_DESKTOP)
    {
      int n = net_current_desktop_get ();

      if (n >= 0)
	current_desktop_changed (n);
      
      return;
    }
  
  if (event->atom == xa_NET_NUMBER_OF_DESKTOPS)
    {
      int n = net_number_of_desktops_get ();

      if (n >= 0)
	number_of_desktops_changed (n);
	
      return;
    }
}

static GdkFilterReturn
root_filter (GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
  XEvent *xev = (XEvent *) xevent;

  if (xev->type == PropertyNotify)
    root_property_notify ((XPropertyEvent *) xev);

  return GDK_FILTER_CONTINUE;
}



void
watch_root_properties (void)
{
  GdkWindow *w = gdk_get_default_root_window ();
  
  gdk_window_add_filter (w, root_filter, NULL);
  gdk_window_set_events (w, 
      gdk_window_get_events (w) | GDK_PROPERTY_CHANGE_MASK);
}
