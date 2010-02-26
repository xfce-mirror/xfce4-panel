/*
 * Copyright (C) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/libxfce4panel-alias.h>



/**
 * SECTION: convenience
 * @title: Convenience Functions
 * @short_description: Special purpose widgets and utilities
 * @include: libxfce4panel/libxfce4panel.h
 *
 * This section describes a number of functions that were created
 * to help developers of Xfce Panel plugins.
 **/

/**
 * xfce_panel_create_button:
 *
 * Create regular #GtkButton with a few properties set to be useful in the
 * Xfce panel: Flat (%GTK_RELIEF_NONE), no focus on click and minimal padding.
 *
 * Returns: newly created #GtkButton.
 **/
GtkWidget *
xfce_panel_create_button (void)
{
  GtkWidget *button = gtk_button_new ();

  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  gtk_widget_set_name (button, "xfce-panel-button");

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
GtkWidget *
xfce_panel_create_toggle_button (void)
{
  GtkWidget *button = gtk_toggle_button_new ();

  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  gtk_widget_set_name (button, "xfce-panel-toggle-button");

  return button;
}



#define __XFCE_PANEL_CONVENIENCE_C__
#include <libxfce4panel/libxfce4panel-aliasdef.c>
