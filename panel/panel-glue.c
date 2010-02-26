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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-glue.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-window.h>



static void
panel_glue_popup_menu_quit (gpointer boolean)
{
  extern gboolean dbus_quit_with_restart;

  /* restart or quit */
  dbus_quit_with_restart = !!(GPOINTER_TO_UINT (boolean));

  /* quit main loop */
  gtk_main_quit ();
}



static void
panel_glue_popup_menu_deactivate (GtkMenu     *menu,
                                  PanelWindow *window)
{
  panel_return_if_fail (GTK_IS_MENU (menu));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* thaw autohide block */
  panel_window_thaw_autohide (window);

  /* destroy the menu */
  g_object_unref (G_OBJECT (menu));
}



void
panel_glue_popup_menu (PanelWindow *window)
{
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *image;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* freeze autohide */
  panel_window_freeze_autohide (window);

  /* create menu */
  menu = gtk_menu_new ();

  /* sink the menu and add unref on deactivate */
  g_object_ref_sink (G_OBJECT (menu));
  g_signal_connect (G_OBJECT (menu), "deactivate", G_CALLBACK (panel_glue_popup_menu_deactivate), window);

  /* label */
  item = gtk_image_menu_item_new_with_label (_("Xfce Panel"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_set_sensitive (item, FALSE);
  gtk_widget_show (item);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* add new items */
  item = gtk_image_menu_item_new_with_mnemonic (_("Add _New Items..."));
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (panel_item_dialog_show), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* customize panel */
  item = gtk_image_menu_item_new_with_mnemonic (_("Panel Pr_eferences..."));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_preferences_dialog_show), window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* quit item */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_glue_popup_menu_quit), GUINT_TO_POINTER (0));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* restart item */
  item = gtk_image_menu_item_new_with_mnemonic (_("_Restart"));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_glue_popup_menu_quit), GUINT_TO_POINTER (1));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* about item */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (panel_dialogs_show_about), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* popup the menu */
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  0, gtk_get_current_event_time ());
}



XfceScreenPosition
panel_glue_get_screen_position (PanelWindow *window)
{
  gboolean horizontal;

  panel_return_val_if_fail (PANEL_IS_WINDOW (window), XFCE_SCREEN_POSITION_NONE);

  /* whether the panel is horizontal */
  horizontal = !!(panel_window_get_orientation (window) == GTK_ORIENTATION_HORIZONTAL);

  /* return the screen position */
  switch (panel_window_get_snap_edge (window))
    {
      case PANEL_SNAP_EGDE_NONE:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_FLOATING_V;

      case PANEL_SNAP_EGDE_NW:
        return horizontal ? XFCE_SCREEN_POSITION_NW_H :
          XFCE_SCREEN_POSITION_NW_V;

      case PANEL_SNAP_EGDE_NE:
        return horizontal ? XFCE_SCREEN_POSITION_NE_H :
          XFCE_SCREEN_POSITION_NE_V;

      case PANEL_SNAP_EGDE_SW:
        return horizontal ? XFCE_SCREEN_POSITION_SW_H :
          XFCE_SCREEN_POSITION_SW_V;

      case PANEL_SNAP_EGDE_SE:
        return horizontal ? XFCE_SCREEN_POSITION_SE_H :
          XFCE_SCREEN_POSITION_SE_V;

      case PANEL_SNAP_EGDE_W:
      case PANEL_SNAP_EGDE_WC:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_W;

      case PANEL_SNAP_EGDE_E:
      case PANEL_SNAP_EGDE_EC:
        return horizontal ? XFCE_SCREEN_POSITION_FLOATING_H :
          XFCE_SCREEN_POSITION_E;

      case PANEL_SNAP_EGDE_S:
      case PANEL_SNAP_EGDE_SC:
        return horizontal ? XFCE_SCREEN_POSITION_S :
          XFCE_SCREEN_POSITION_FLOATING_V;

      case PANEL_SNAP_EGDE_N:
      case PANEL_SNAP_EGDE_NC:
        return horizontal ? XFCE_SCREEN_POSITION_N :
          XFCE_SCREEN_POSITION_FLOATING_V;
    }

  return XFCE_SCREEN_POSITION_NONE;
}



static void
panel_glue_set_orientation_foreach (GtkWidget *widget,
                                    gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* set the new plugin orientation */
  xfce_panel_plugin_provider_set_orientation (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                              GPOINTER_TO_INT (user_data));
}



static void
panel_glue_set_size_foreach (GtkWidget *widget,
                             gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* set the new plugin size */
  xfce_panel_plugin_provider_set_size (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                       GPOINTER_TO_INT (user_data));
}



static void
panel_glue_set_screen_position_foreach (GtkWidget *widget,
                                        gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* set the new plugin screen position */
  xfce_panel_plugin_provider_set_screen_position (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                                  GPOINTER_TO_INT (user_data));
}



void
panel_glue_set_orientation (PanelWindow    *window,
                            GtkOrientation  orienation)
{
  GtkWidget *itembar;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* set the size of the window */
  panel_window_set_orientation (window, orienation);

  /* get the itembar */
  itembar = gtk_bin_get_child (GTK_BIN (window));

  /* send the new orientation to all plugins */
  gtk_container_foreach (GTK_CONTAINER (itembar), panel_glue_set_orientation_foreach, GINT_TO_POINTER (orienation));

  /* screen position also changed */
  panel_glue_set_screen_position (window);
}



void
panel_glue_set_size (PanelWindow *window,
                     gint         size)
{
  GtkWidget *itembar;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* set the size of the window */
  panel_window_set_size (window, size);

  /* get the itembar */
  itembar = gtk_bin_get_child (GTK_BIN (window));

  /* send the new size to all plugins */
  gtk_container_foreach (GTK_CONTAINER (itembar), panel_glue_set_size_foreach, GINT_TO_POINTER (size));
}


void
panel_glue_set_screen_position (PanelWindow *window)
{
  XfceScreenPosition  screen_position;
  GtkWidget          *itembar;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the screen position */
  screen_position = panel_glue_get_screen_position (window);

  /* get the itembar */
  itembar = gtk_bin_get_child (GTK_BIN (window));

  /* send the new size to all plugins */
  gtk_container_foreach (GTK_CONTAINER (itembar), panel_glue_set_screen_position_foreach, GINT_TO_POINTER (screen_position));
}



void
panel_glue_set_provider_info (XfcePanelPluginProvider *provider)
{
  PanelWindow *window;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (PANEL_IS_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider))));

  /* get the plugins panel window */
  window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider)));

  /* set the background alpha if the plugin is external */
  if (PANEL_IS_PLUGIN_EXTERNAL (provider))
    panel_plugin_external_set_background_alpha (PANEL_PLUGIN_EXTERNAL (provider), panel_window_get_background_alpha (window));

  /* send plugin information */
  xfce_panel_plugin_provider_set_orientation (provider, panel_window_get_orientation (window));
  xfce_panel_plugin_provider_set_screen_position (provider, panel_glue_get_screen_position (window));
  xfce_panel_plugin_provider_set_size (provider, panel_window_get_size (window));
}
