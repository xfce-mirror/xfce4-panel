/* $Id */
/*
 * Copyright (C) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2008      Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <gtk/gtk.h>

#include <libxfce4panel/libxfce4panel.h>

/**
 * xfce_panel_create_button:
 *
 * Create regular #GtkButton with a few properties set to be useful in the
 * Xfce panel: Flat (%GTK_RELIEF_NONE), no focus on click and minimal padding.
 *
 * Returns: newly created #GtkButton.
 **/
PANEL_SYMBOL_EXPORT GtkWidget *
xfce_panel_create_button (void)
{
  GtkWidget *button = gtk_button_new ();

  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);

  gtk_widget_set_name (button, "xfce_panel_button");

  return button;
}



/**
 * xfce_panel_create_toggle_button:
 *
 * Create regular #GtkToggleButton with a few properties set to be useful in
 * Xfce panel: Flat (%GTK_RELIEF_NONE), no focus on click and minimal padding.
 *
 * Returns: newly created #GtkToggleButton.
 **/
PANEL_SYMBOL_EXPORT GtkWidget *
xfce_panel_create_toggle_button (void)
{
  GtkWidget *button = gtk_toggle_button_new ();

  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);

  gtk_widget_set_name (button, "xfce_panel_toggle_button");

  return button;
}



/**
 * xfce_panel_allow_customization:
 *
 * Check if the user is allowed to customize the panel. Uses the kiosk mode
 * implementation from libxfce4util.
 *
 * Returns: %TRUE if the user is allowed to customize the panel, %FALSE
 *          otherwise.
 **/
PANEL_SYMBOL_EXPORT gboolean
xfce_panel_allow_customization (void)
{
  static gboolean  allow_customization = FALSE;
  static gboolean  checked = FALSE;
  XfceKiosk       *kiosk;

  if (G_UNLIKELY (!checked))
    {
      kiosk = xfce_kiosk_new ("xfce4-panel");
      allow_customization = xfce_kiosk_query (kiosk, "CustomizePanel");
      xfce_kiosk_free (kiosk);
      checked = TRUE;
    }

  return allow_customization;
}



PANEL_SYMBOL_EXPORT void
xfce_panel_cairo_set_source_rgba (cairo_t  *cr,
                                  GdkColor *color,
                                  gdouble   alpha)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (alpha >= 0.00 && alpha <= 1.00);

  if (alpha >= 1.00)
    {
      /* set normal source color */
      cairo_set_source_rgb (cr,
                            color->red / 65535.00,
                            color->green / 65535.00,
                            color->blue / 65535.00);
    }
  else
    {
      /* set source color with alpha */
      cairo_set_source_rgba (cr,
                             color->red / 65535.00,
                             color->green / 65535.00,
                             color->blue / 65535.00,
                             alpha);
    }
}
