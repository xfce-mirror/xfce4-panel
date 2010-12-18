/*
 * Copyright (c) 2002      Anders Carlsson <andersca@gnu.org>
 * Copyright (c) 2003-2006 Vincent Untz
 * Copyright (c) 2008      Red Hat, Inc.
 * Copyright (c) 2009-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
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
#include <common/panel-private.h>
#include "systray-socket.h"



struct _SystraySocketClass
{
  GtkSocketClass __parent__;
};

struct _SystraySocket
{
  GtkSocket __parent__;

  /* plug window */
  GdkNativeWindow window;

  guint           is_composited : 1;
  guint           parent_relative_bg : 1;
};



static void     systray_socket_realize       (GtkWidget      *widget);
static void     systray_socket_size_allocate (GtkWidget      *widget,
                                              GtkAllocation  *allocation);
static gboolean systray_socket_expose_event  (GtkWidget      *widget,
                                              GdkEventExpose *event);
static void     systray_socket_style_set     (GtkWidget      *widget,
                                              GtkStyle       *previous_style);



XFCE_PANEL_DEFINE_TYPE (SystraySocket, systray_socket, GTK_TYPE_SOCKET)



static void
systray_socket_class_init (SystraySocketClass *klass)
{
  GtkWidgetClass *gtkwidget_class;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = systray_socket_realize;
  gtkwidget_class->size_allocate = systray_socket_size_allocate;
  gtkwidget_class->expose_event = systray_socket_expose_event;
  gtkwidget_class->style_set = systray_socket_style_set;
}



static void
systray_socket_init (SystraySocket *socket)
{
}



static void
systray_socket_realize (GtkWidget *widget)
{
  SystraySocket *socket = XFCE_SYSTRAY_SOCKET (widget);
  GdkVisual     *visual;
  GdkColor       transparent = { 0, 0, 0, 0 };

  GTK_WIDGET_CLASS (systray_socket_parent_class)->realize (widget);

  visual = gtk_widget_get_visual (widget);
  if (visual->red_prec + visual->blue_prec + visual->green_prec < visual->depth
      && gdk_display_supports_composite (gtk_widget_get_display (widget)))
    {
      gdk_window_set_background (widget->window, &transparent);
      gdk_window_set_composited (widget->window, TRUE);

      socket->is_composited = TRUE;
      socket->parent_relative_bg = FALSE;
    }
  else if (visual == gdk_drawable_get_visual (
               GDK_DRAWABLE (gdk_window_get_parent (widget->window))))
    {
      gdk_window_set_back_pixmap (widget->window, NULL, TRUE);

      socket->is_composited = FALSE;
      socket->parent_relative_bg = TRUE;
    }
  else
    {
      socket->is_composited = FALSE;
      socket->parent_relative_bg = FALSE;
    }

  gtk_widget_set_app_paintable (widget,
      socket->parent_relative_bg || socket->is_composited);

  gtk_widget_set_double_buffered (widget, socket->parent_relative_bg);
}



static void
systray_socket_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
  SystraySocket *socket = XFCE_SYSTRAY_SOCKET (widget);
  gboolean       moved = allocation->x != widget->allocation.x
                         || allocation->y != widget->allocation.y;
  gboolean       resized = allocation->width != widget->allocation.width
                           ||allocation->height != widget->allocation.height;

  if ((moved || resized) && GTK_WIDGET_MAPPED (widget))
    {
      if (socket->is_composited)
        gdk_window_invalidate_rect (gdk_window_get_parent (widget->window),
                                    &widget->allocation, FALSE);
    }

  GTK_WIDGET_CLASS (systray_socket_parent_class)->size_allocate (widget, allocation);

  if ((moved || resized) && GTK_WIDGET_MAPPED (widget))
    {
      if (socket->is_composited)
        gdk_window_invalidate_rect (gdk_window_get_parent (widget->window),
                                    &widget->allocation, FALSE);
      else if (moved && socket->parent_relative_bg)
        systray_socket_force_redraw (socket);
    }
}


static gboolean
systray_socket_expose_event (GtkWidget      *widget,
                             GdkEventExpose *event)
{
  SystraySocket *socket = XFCE_SYSTRAY_SOCKET (widget);
  cairo_t       *cr;

  if (socket->is_composited)
    {
      /* clear to transparent */
      cr = gdk_cairo_create (widget->window);
      cairo_set_source_rgba (cr, 0, 0, 0, 0);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      gdk_cairo_region (cr, event->region);
      cairo_fill (cr);
      cairo_destroy (cr);
    }
  else if (socket->parent_relative_bg)
    {
      /* clear to parent-relative pixmap */
      gdk_window_clear_area (widget->window,
                             event->area.x,
           event->area.y,
                             event->area.width,
           event->area.height);
    }

  return FALSE;
}



static void
systray_socket_style_set (GtkWidget *widget,
                          GtkStyle  *previous_style)
{
}



GtkWidget *
systray_socket_new (GdkScreen       *screen,
                    GdkNativeWindow  window)
{
  SystraySocket     *socket;
  GdkDisplay        *display;
  XWindowAttributes  attr;
  gint               result;
  GdkVisual         *visual;
  GdkColormap       *colormap;
  gboolean           release_colormap = FALSE;

  panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  /* get the window attributes */
  display = gdk_screen_get_display (screen);
  gdk_error_trap_push ();
  result = XGetWindowAttributes (GDK_DISPLAY_XDISPLAY (display),
                                 window, &attr);

  /* leave on an error or is the window does not exist */
  if (gdk_error_trap_pop () != 0 || result == 0)
    return NULL;

  /* get the windows visual */
  visual = gdk_x11_screen_lookup_visual (screen, attr.visual->visualid);
  if (G_UNLIKELY (visual == NULL))
    return NULL;

  /* get the correct colormap */
  if (visual == gdk_screen_get_rgb_visual (screen))
    colormap = gdk_screen_get_rgb_colormap (screen);
  else if (visual == gdk_screen_get_rgba_visual (screen))
    colormap = gdk_screen_get_rgba_colormap (screen);
  else if (visual == gdk_screen_get_system_visual (screen))
    colormap = gdk_screen_get_system_colormap (screen);
  else
    {
      /* create custom colormap */
      colormap = gdk_colormap_new (visual, FALSE);
      release_colormap = TRUE;
    }

  /* create a new socket */
  socket = g_object_new (XFCE_TYPE_SYSTRAY_SOCKET, NULL);
  gtk_widget_set_colormap (GTK_WIDGET (socket), colormap);
  socket->window = window;

  /* release the custom colormap */
  if (release_colormap)
    g_object_unref (G_OBJECT (colormap));

  return GTK_WIDGET (socket);
}



void
systray_socket_force_redraw (SystraySocket *socket)
{
  GtkWidget  *widget = GTK_WIDGET (socket);
  XEvent      xev;
  GdkDisplay *display;

  panel_return_if_fail (XFCE_IS_SYSTRAY_SOCKET (socket));

  if (GTK_WIDGET_MAPPED (socket) && socket->parent_relative_bg)
    {
      display = gtk_widget_get_display (widget);

      xev.xexpose.type = Expose;
      xev.xexpose.window = GDK_WINDOW_XWINDOW (GTK_SOCKET (socket)->plug_window);
      xev.xexpose.x = 0;
      xev.xexpose.y = 0;
      xev.xexpose.width = widget->allocation.width;
      xev.xexpose.height = widget->allocation.height;
      xev.xexpose.count = 0;

      gdk_error_trap_push ();
      XSendEvent (GDK_DISPLAY_XDISPLAY (display),
                  xev.xexpose.window,
                  False, ExposureMask,
                  &xev);
      /* We have to sync to reliably catch errors from the XSendEvent(),
       * since that is asynchronous.
       */
      XSync (GDK_DISPLAY_XDISPLAY (display), False);
      gdk_error_trap_pop ();
    }
}



gboolean
systray_socket_is_composited (SystraySocket *socket)
{
  panel_return_val_if_fail (XFCE_IS_SYSTRAY_SOCKET (socket), FALSE);

  return socket->is_composited;
}



gchar *
systray_socket_get_title  (SystraySocket *socket)
{
  gchar      *name = NULL;
  GdkDisplay *display;
  Atom        utf8_string, type;
  gint        result;
  gint        format;
  gulong      nitems;
  gulong      bytes_after;
  gchar      *val;

  panel_return_val_if_fail (XFCE_IS_SYSTRAY_SOCKET (socket), NULL);

  display = gtk_widget_get_display (GTK_WIDGET (socket));

  utf8_string = gdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING");

  gdk_error_trap_push ();

  result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
                               socket->window,
                               gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_NAME"),
                               0, G_MAXLONG, False,
                               utf8_string,
                               &type, &format, &nitems,
                               &bytes_after, (guchar **) &val);

  if (gdk_error_trap_pop () != 0 || result != Success || val == NULL)
    return NULL;

  if (type != utf8_string || format != 8 || nitems == 0)
    {
      XFree (val);
      return NULL;
    }

  if (!g_utf8_validate (val, nitems, NULL))
    {
      XFree (val);
      return NULL;
    }

  name = g_utf8_strdown (val, nitems);

  XFree (val);

  return name;
}



GdkNativeWindow *
systray_socket_get_window (SystraySocket *socket)
{
  panel_return_val_if_fail (XFCE_IS_SYSTRAY_SOCKET (socket), NULL);

  return &socket->window;
}
