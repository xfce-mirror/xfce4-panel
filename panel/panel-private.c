/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <panel/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>



#if !GTK_CHECK_VERSION (2,12,0)
void
_widget_set_tooltip_text (GtkWidget   *widget,
                          const gchar *text)
{
  static GtkTooltips tooltips = NULL;

  panel_return_if_fail (GTK_IS_WIDGET (widget));

  /* allocate shared tooltip */
  if (G_UNLIKELY (tooltips == NULL))
    tooltips = gtk_tooltips_new ();

  /* set tip */
  gtk_tooltips_set_tip (tooltips, widget, text);
}



void
_window_set_opacity (GtkWindow *window,
                     gdouble    opacity)
{
  guint32 cardinal;

  panel_return_if_fail (GTK_IS_WINDOW (window));

  /* prevent warnings */
  gdk_error_trap_push ();

  if (opacity >= 1.00 || opacity < 0.00)
    {
      /* remove property */
      gdk_property_delete (GTK_WIDGET (window)->window,
                           gdk_atom_intern_static_string ("_NET_WM_WINDOW_OPACITY"));
    }
  else
    {
      /* opacity value for the window manager */
      cardinal = opacity * 0xffffffff;

      /* set window property */
      gdk_property_change (GTK_WIDGET (window)->window,
                           gdk_atom_intern_static_string ("_NET_WM_WINDOW_OPACITY"),
                           gdk_atom_intern_static_string ("CARDINAL"), 32,
                           GDK_PROP_MODE_REPLACE,
                           (guchar *) &cardinal, 1L);
    }

  /* unlock warnings */
  gdk_error_trap_pop ();

}
#endif
