/*  gxfce
 *  Copyright (C) 1999, 2002 Olivier Fourdan (fourdan@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "xfce.h"
#include "move.h"

GdkGC *DrawGC = NULL;
static gboolean xgrabbed = FALSE;
static gboolean pressed = FALSE;

/* Added by Jason Litowitz */
#include "popup.h" 

void
move_internal_grab (Display * disp)
{
  if (!xgrabbed)
  {
    XGrabServer (disp);
    xgrabbed = TRUE;
  }
}

void
move_internal_ungrab (Display * disp)
{
  if (xgrabbed)
  {
    XUngrabServer (disp);
    xgrabbed = FALSE;
  }
}

void
CreateDrawGC (GdkWindow * w)
{
  GdkGCValues gcv;
  GdkGCValuesMask gcm;
  GdkColormap *cmap;
  GdkColor col;

  cmap = gdk_colormap_get_system ();
  col.red = col.green = col.blue = 0xFFFF;
  if (!gdk_color_alloc (cmap, &col))
  {
    g_error (_("couldn't allocate color"));
  }

  gcm = GDK_GC_FUNCTION | GDK_GC_LINE_WIDTH | GDK_GC_FOREGROUND | GDK_GC_SUBWINDOW | GDK_GC_LINE_STYLE;
  gcv.function = GDK_XOR;
  gcv.line_width = 5;
  gcv.foreground = col;
  gcv.subwindow_mode = GDK_INCLUDE_INFERIORS;
  gcv.line_style = GDK_LINE_SOLID;
  DrawGC = gdk_gc_new_with_values (w, &gcv, gcm);
}

void
FreeDrawGC (void)
{
  gdk_gc_destroy (DrawGC);
}

void
MoveOutline (int x, int y, int width, int height)
{
  static int lastx = 0;
  static int lasty = 0;
  static int lastWidth = 0;
  static int lastHeight = 0;

  if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
    return;

  /* undraw the old one, if any */
  if (lastWidth || lastHeight)
  {
    gdk_draw_rectangle (GDK_ROOT_PARENT (), DrawGC, FALSE, lastx, lasty, lastWidth, lastHeight);
  }

  lastx = x;
  lasty = y;
  lastWidth = width;
  lastHeight = height;

  /* draw the new one, if any */
  if (lastWidth || lastHeight)
  {
    gdk_draw_rectangle (GDK_ROOT_PARENT (), DrawGC, FALSE, lastx, lasty, lastWidth, lastHeight);
  }
}

static void
move_pressed (GtkWidget * widget, GdkEventButton * event, gpointer * topwin)
{

  CursorOffset *p;
  gint upositionx = 0;
  gint upositiony = 0;
  gint uwidth = 0;
  gint uheight = 0;
  gint xp, yp;

  /* Added by Jason Litowitz */
  hide_current_popup_menu ();
  /* ignore double and triple click */
  if (event->type != GDK_BUTTON_PRESS)
    return;
  if (event->button != 1)
    return;

/*  move_internal_grab (GDK_DISPLAY ());*/

  gtk_grab_add (widget);
  gdk_pointer_grab (widget->window, TRUE, GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK, NULL, NULL, 0);

  p = g_object_get_data (G_OBJECT (widget), "user_data");
  gdk_window_get_origin (((GtkWidget *) topwin)->window, &upositionx, &upositiony);
  gdk_window_get_size (((GtkWidget *) topwin)->window, &uwidth, &uheight);

  p->x = (int) upositionx - event->x_root;
  p->y = (int) upositiony - event->y_root;

  xp = 0;
  yp = 0;
  gdk_window_get_pointer (GDK_ROOT_PARENT (), &xp, &yp, NULL);
  xp += p->x;
  yp += p->y;

/*  MoveOutline (xp, yp, uwidth, uheight);*/
  pressed = TRUE;
}

static void
move_released (GtkWidget * widget, GdkEventButton * event, gpointer * topwin)
{
  gint xp, yp;
  CursorOffset *p;

  if (event->button != 1)
    return;

  if (!pressed)
    return;

  pressed = FALSE;

  p = g_object_get_data (G_OBJECT (widget), "user_data");

  xp = 0;
  yp = 0;
  gdk_window_get_pointer (GDK_ROOT_PARENT (), &xp, &yp, NULL);
  xp += p->x;
  yp += p->y;

/*  MoveOutline (0, 0, 0, 0);
  XSync (GDK_DISPLAY (), True);
  move_internal_ungrab (GDK_DISPLAY ());
*/
  XMoveWindow (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (((GtkWidget *) topwin)->window), (xp > 0) ? xp : 0, (yp > 0) ? yp : 0);
  gtk_grab_remove (widget);
  gdk_pointer_ungrab (0);
}

static void
move_motion (GtkWidget * widget, GdkEventMotion * event, gpointer * topwin)
{
  gint xp, yp;
  CursorOffset *p;
/*  
  gint uwidth = 0;
  gint uheight = 0;
*/
  if (!pressed)
    return;

  p = g_object_get_data (G_OBJECT (widget), "user_data");

  xp = 0;
  yp = 0;
  gdk_window_get_pointer (GDK_ROOT_PARENT (), &xp, &yp, NULL);
  xp += p->x;
  yp += p->y;

/*  
  gdk_window_get_size (((GtkWidget *) topwin)->window, &uwidth, &uheight);
  MoveOutline (xp, yp, uwidth, uheight);
*/
  XMoveWindow (GDK_DISPLAY (), GDK_WINDOW_XWINDOW (((GtkWidget *) topwin)->window), xp, yp);
}

void
attach_move_callbacks (GtkWidget * widget, GtkWidget * toplevel)
{
  CursorOffset *icon_pos;
  
  icon_pos = g_new (CursorOffset, 1);
  g_object_set_data (G_OBJECT (widget), "user_data", icon_pos);

  gtk_widget_set_events (widget, GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK);

  g_signal_connect (widget, "button_press_event", 
		    G_CALLBACK (move_pressed), toplevel);
  g_signal_connect (widget, "button_release_event", 
		    G_CALLBACK (move_released), toplevel);
  g_signal_connect (widget, "motion_notify_event", 
		    G_CALLBACK (move_motion), toplevel);
}
