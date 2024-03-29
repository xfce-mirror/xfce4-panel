/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>
#include <libxfce4util/libxfce4util.h>
#include <common/panel-private.h>
#include <common/panel-utils.h>

#include "showdesktop.h"



#define DRAG_ACTIVATE_TIMEOUT (500)



static void     show_desktop_plugin_screen_changed          (GtkWidget              *widget,
                                                             GdkScreen              *previous_screen);
static void     show_desktop_plugin_construct               (XfcePanelPlugin        *panel_plugin);
static void     show_desktop_plugin_free_data               (XfcePanelPlugin        *panel_plugin);
static gboolean show_desktop_plugin_size_changed            (XfcePanelPlugin        *panel_plugin,
                                                             gint                    size);
static void     show_desktop_plugin_toggled                 (GtkToggleButton        *button,
                                                             ShowDesktopPlugin      *plugin);
static gboolean show_desktop_plugin_button_release_event    (GtkToggleButton        *button,
                                                             GdkEventButton         *event,
                                                             ShowDesktopPlugin      *plugin);
static void     show_desktop_plugin_show_desktop_changed    (XfwScreen              *xfw_screen,
                                                             GParamSpec             *pspec,
                                                             ShowDesktopPlugin      *plugin);
static void     show_desktop_plugin_drag_leave              (GtkWidget              *widget,
                                                             GdkDragContext         *context,
                                                             guint                   time,
                                                             ShowDesktopPlugin      *plugin);
static gboolean show_desktop_plugin_drag_motion             (GtkWidget              *widget,
                                                             GdkDragContext         *context,
                                                             gint                    x,
                                                             gint                    y,
                                                             guint                   time,
                                                             ShowDesktopPlugin      *plugin);



struct _ShowDesktopPlugin
{
  XfcePanelPlugin __parent__;

  /* the toggle button */
  GtkWidget  *button;
  GtkWidget  *icon;

  /* Dnd timeout */
  guint       drag_timeout;

  /* the xfw screen */
  XfwScreen *xfw_screen;
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ShowDesktopPlugin, show_desktop_plugin)



static void
show_desktop_plugin_class_init (ShowDesktopPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = show_desktop_plugin_construct;
  plugin_class->free_data = show_desktop_plugin_free_data;
  plugin_class->size_changed = show_desktop_plugin_size_changed;
}



static void
show_desktop_plugin_init (ShowDesktopPlugin *plugin)
{
  GtkWidget *button;

  plugin->xfw_screen = NULL;

  /* monitor screen changes */
  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (show_desktop_plugin_screen_changed), NULL);

  /* create the toggle button */
  button = plugin->button = xfce_panel_create_toggle_button ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (plugin), button);
  gtk_widget_set_name (button, "showdesktop-button");
  g_signal_connect (G_OBJECT (button), "toggled",
      G_CALLBACK (show_desktop_plugin_toggled), plugin);
  g_signal_connect (G_OBJECT (button), "button-release-event",
      G_CALLBACK (show_desktop_plugin_button_release_event), plugin);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), button);
  gtk_widget_show (button);

  /* allow toggle the button when drag something.*/
  gtk_drag_dest_set (GTK_WIDGET (plugin->button), 0, NULL, 0, 0);
  g_signal_connect (G_OBJECT (plugin->button), "drag_motion",
      G_CALLBACK (show_desktop_plugin_drag_motion), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "drag_leave",
      G_CALLBACK (show_desktop_plugin_drag_leave), plugin);

  plugin->icon = gtk_image_new_from_icon_name ("org.xfce.panel.showdesktop", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), plugin->icon);
  gtk_widget_show (plugin->icon);
}



static void
show_desktop_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  xfce_panel_plugin_set_small (panel_plugin, TRUE);
}



static void
show_desktop_plugin_screen_changed (GtkWidget *widget,
                                    GdkScreen *previous_screen)
{
  ShowDesktopPlugin *plugin = SHOW_DESKTOP_PLUGIN (widget);
  XfwScreen         *xfw_screen;

  panel_return_if_fail (SHOW_DESKTOP_IS_PLUGIN (widget));

  /* get the new xfw screen */
  xfw_screen = xfw_screen_get_default ();
  panel_return_if_fail (XFW_IS_SCREEN (xfw_screen));

  /* leave when the xfw screen did not change */
  if (plugin->xfw_screen == xfw_screen)
    {
      g_object_unref (xfw_screen);
      return;
    }

  /* disconnect signals from an existing xfw screen */
  if (plugin->xfw_screen != NULL)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->xfw_screen),
          show_desktop_plugin_show_desktop_changed, plugin);
      g_object_unref (plugin->xfw_screen);
    }

  /* set the new xfw screen */
  plugin->xfw_screen = xfw_screen;
  g_signal_connect (G_OBJECT (xfw_screen), "notify::show-desktop",
      G_CALLBACK (show_desktop_plugin_show_desktop_changed), plugin);

  /* toggle the button to the current state or update the tooltip */
  if (G_UNLIKELY (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->button)) !=
        xfw_screen_get_show_desktop (xfw_screen)))
    show_desktop_plugin_show_desktop_changed (xfw_screen, NULL, plugin);
  else
    show_desktop_plugin_toggled (GTK_TOGGLE_BUTTON (plugin->button), plugin);
}



static void
show_desktop_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ShowDesktopPlugin *plugin = SHOW_DESKTOP_PLUGIN (panel_plugin);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
     show_desktop_plugin_screen_changed, NULL);

  /* disconnect handle */
  if (plugin->xfw_screen != NULL)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->xfw_screen),
          show_desktop_plugin_show_desktop_changed, plugin);
      g_object_unref (plugin->xfw_screen);
    }
}



static gboolean
show_desktop_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                  gint             size)
{
  ShowDesktopPlugin *plugin = SHOW_DESKTOP_PLUGIN (panel_plugin);
  gint  icon_size;

  panel_return_val_if_fail (SHOW_DESKTOP_IS_PLUGIN (panel_plugin), FALSE);

  /* keep the button squared */
  size /= xfce_panel_plugin_get_nrows (panel_plugin);
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, size);
  icon_size = xfce_panel_plugin_get_icon_size (panel_plugin);
  gtk_image_set_pixel_size (GTK_IMAGE (plugin->icon), icon_size);

  return TRUE;
}



static void
show_desktop_plugin_toggled (GtkToggleButton   *button,
                             ShowDesktopPlugin *plugin)
{
  gboolean     active;
  const gchar *text;

  panel_return_if_fail (SHOW_DESKTOP_IS_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (XFW_IS_SCREEN (plugin->xfw_screen));

  /* toggle the desktop */
  active = gtk_toggle_button_get_active (button);
  if (active != xfw_screen_get_show_desktop (plugin->xfw_screen))
    xfw_screen_set_show_desktop (plugin->xfw_screen, active);

  if (active)
    text = _("Restore the minimized windows");
  else
    text = _("Minimize all open windows and show the desktop");

  gtk_widget_set_tooltip_text (GTK_WIDGET (button), text);
  panel_utils_set_atk_info (GTK_WIDGET (button), _("Show Desktop"), text);
}



static gboolean
show_desktop_plugin_button_release_event (GtkToggleButton   *button,
                                          GdkEventButton    *event,
                                          ShowDesktopPlugin *plugin)
{
  XfwWorkspaceManager *manager;
  XfwWorkspace *active_ws;
  GList *windows, *li;
  XfwWindow *window;

  panel_return_val_if_fail (SHOW_DESKTOP_IS_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (XFW_IS_SCREEN (plugin->xfw_screen), FALSE);

  if (event->button == 2)
    {
      manager = xfw_screen_get_workspace_manager (plugin->xfw_screen);
      li = xfw_workspace_manager_list_workspace_groups (manager);
      if (li == NULL)
        return FALSE;

      active_ws = xfw_workspace_group_get_active_workspace (li->data);
      windows = xfw_screen_get_windows (plugin->xfw_screen);

      for (li = windows; li != NULL; li = li->next)
        {
          window = XFW_WINDOW (li->data);

          if (xfw_window_get_workspace (window) != active_ws)
            continue;

          /* toggle the shade state */
          if (xfw_window_is_shaded (window))
            xfw_window_set_shaded (window, FALSE, NULL);
          else
            xfw_window_set_shaded (window, TRUE, NULL);
        }
    }

  return FALSE;
}



static void
show_desktop_plugin_show_desktop_changed (XfwScreen         *xfw_screen,
                                          GParamSpec        *pspec,
                                          ShowDesktopPlugin *plugin)
{
  panel_return_if_fail (SHOW_DESKTOP_IS_PLUGIN (plugin));
  panel_return_if_fail (XFW_IS_SCREEN (xfw_screen));
  panel_return_if_fail (plugin->xfw_screen == xfw_screen);

  /* update button to user action */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button),
      xfw_screen_get_show_desktop (xfw_screen));
}



static gboolean
show_desktop_plugin_drag_timeout (gpointer data)
{
  ShowDesktopPlugin *plugin = (ShowDesktopPlugin *) data;

  plugin->drag_timeout = 0;

  /* activate button to toggle show desktop */
  g_signal_emit_by_name (G_OBJECT (plugin->button), "clicked", plugin);

  return FALSE;
}



static void
show_desktop_plugin_drag_leave (GtkWidget         *widget,
                                GdkDragContext    *context,
                                guint              time,
                                ShowDesktopPlugin *plugin)
{
  if (plugin->drag_timeout != 0)
    {
      g_source_remove (plugin->drag_timeout);
      plugin->drag_timeout = 0;
    }

  gtk_drag_unhighlight (GTK_WIDGET (widget));
}



static gboolean
show_desktop_plugin_drag_motion (GtkWidget         *widget,
                                 GdkDragContext    *context,
                                 gint               x,
                                 gint               y,
                                 guint              time,
                                 ShowDesktopPlugin *plugin)
{
  if (plugin->drag_timeout == 0)
    plugin->drag_timeout = g_timeout_add (DRAG_ACTIVATE_TIMEOUT,
                                          show_desktop_plugin_drag_timeout,
                                          plugin);

  gtk_drag_highlight (GTK_WIDGET (widget));

  gdk_drag_status (context, 0, time);

  return TRUE;
}
