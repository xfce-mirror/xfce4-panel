/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
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
#include "config.h"
#endif

#include "panel-plugin-external-wrapper-x11.h"

#include "common/panel-private.h"

#include <gtk/gtkx.h>



static void
panel_plugin_external_wrapper_x11_size_allocate (GtkWidget *widget,
                                                 GtkAllocation *allocation);
static gchar **
panel_plugin_external_wrapper_x11_get_argv (PanelPluginExternal *external,
                                            gchar **arguments);
static gboolean
panel_plugin_external_wrapper_x11_spawn (PanelPluginExternal *external,
                                         gchar **argv,
                                         GPid *pid,
                                         GError **error);
static void
panel_plugin_external_wrapper_x11_set_background_color (PanelPluginExternal *external,
                                                        const GdkRGBA *color);
static void
panel_plugin_external_wrapper_x11_set_background_image (PanelPluginExternal *external,
                                                        const gchar *image);
static void
panel_plugin_external_wrapper_x11_set_geometry (PanelPluginExternal *external,
                                                PanelWindow *window);
static gboolean
panel_plugin_external_wrapper_x11_pointer_is_outside (PanelPluginExternal *external);
static void
panel_plugin_external_wrapper_x11_socket_plug_added (GtkSocket *socket,
                                                     PanelPluginExternalWrapperX11 *wrapper);
static gboolean
panel_plugin_external_wrapper_x11_socket_plug_removed (GtkSocket *socket,
                                                       PanelPluginExternalWrapperX11 *wrapper);



struct _PanelPluginExternalWrapperX11
{
  PanelPluginExternalWrapper __parent__;

  GtkWidget *socket;
  GdkRectangle geometry;
};



G_DEFINE_FINAL_TYPE (PanelPluginExternalWrapperX11, panel_plugin_external_wrapper_x11, PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER)



static void
panel_plugin_external_wrapper_x11_class_init (PanelPluginExternalWrapperX11Class *klass)
{
  GtkWidgetClass *gtkwidget_class;
  PanelPluginExternalClass *external_class;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_allocate = panel_plugin_external_wrapper_x11_size_allocate;

  external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  external_class->get_argv = panel_plugin_external_wrapper_x11_get_argv;
  external_class->spawn = panel_plugin_external_wrapper_x11_spawn;
  external_class->set_background_color = panel_plugin_external_wrapper_x11_set_background_color;
  external_class->set_background_image = panel_plugin_external_wrapper_x11_set_background_image;
  external_class->set_geometry = panel_plugin_external_wrapper_x11_set_geometry;
  external_class->pointer_is_outside = panel_plugin_external_wrapper_x11_pointer_is_outside;
}



static void
panel_plugin_external_wrapper_x11_init (PanelPluginExternalWrapperX11 *wrapper)
{
  wrapper->socket = gtk_socket_new ();
  g_signal_connect (wrapper->socket, "plug-added",
                    G_CALLBACK (panel_plugin_external_wrapper_x11_socket_plug_added), wrapper);
  g_signal_connect (wrapper->socket, "plug-removed",
                    G_CALLBACK (panel_plugin_external_wrapper_x11_socket_plug_removed), wrapper);
  gtk_box_pack_start (GTK_BOX (wrapper), wrapper->socket, TRUE, FALSE, 0);
  gtk_widget_show (wrapper->socket);
}



static void
panel_plugin_external_wrapper_x11_size_allocate (GtkWidget *widget,
                                                 GtkAllocation *allocation)
{
  PanelBaseWindow *window = PANEL_BASE_WINDOW (gtk_widget_get_ancestor (widget, PANEL_TYPE_BASE_WINDOW));

  GTK_WIDGET_CLASS (panel_plugin_external_wrapper_x11_parent_class)->size_allocate (widget, allocation);

  if (panel_base_window_get_background_style (window) == PANEL_BG_STYLE_IMAGE
      && panel_base_window_get_background_image (window) != NULL)
    panel_plugin_external_wrapper_x11_set_geometry (PANEL_PLUGIN_EXTERNAL (widget), PANEL_WINDOW (window));
}



static gchar **
panel_plugin_external_wrapper_x11_get_argv (PanelPluginExternal *external,
                                            gchar **arguments)
{
  PanelPluginExternalWrapperX11 *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_X11 (external);
  gchar **argv;

  argv = PANEL_PLUGIN_EXTERNAL_CLASS (panel_plugin_external_wrapper_x11_parent_class)->get_argv (external, arguments);
  if (argv != NULL)
    argv[PLUGIN_ARGV_SOCKET_ID] = g_strdup_printf ("%lu", gtk_socket_get_id (GTK_SOCKET (wrapper->socket)));

  return argv;
}



static void
panel_plugin_external_wrapper_x11_spawn_child_setup (gpointer data)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (data);
  GdkDisplay *display;
  const gchar *name;

  /* this is what gdk_spawn_on_screen does */
  display = gtk_widget_get_display (GTK_WIDGET (external));
  name = gdk_display_get_name (display);
  g_setenv ("DISPLAY", name, TRUE);
}



static gboolean
panel_plugin_external_wrapper_x11_spawn (PanelPluginExternal *external,
                                         gchar **argv,
                                         GPid *pid,
                                         GError **error)
{
  return g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                        panel_plugin_external_wrapper_x11_spawn_child_setup,
                        external, pid, error);
}



static void
panel_plugin_external_wrapper_x11_set_background_color (PanelPluginExternal *external,
                                                        const GdkRGBA *color)
{
  if (G_LIKELY (color != NULL))
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, G_TYPE_STRING);
      g_value_take_string (&value, gdk_rgba_to_string (color));

      panel_plugin_external_queue_add (external,
                                       PROVIDER_PROP_TYPE_SET_BACKGROUND_COLOR,
                                       &value);

      g_value_unset (&value);
    }
  else
    {
      panel_plugin_external_queue_add_action (external,
                                              PROVIDER_PROP_TYPE_ACTION_BACKGROUND_UNSET);
    }
}



static void
panel_plugin_external_wrapper_x11_set_background_image (PanelPluginExternal *external,
                                                        const gchar *image)
{
  if (G_UNLIKELY (image != NULL))
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, G_TYPE_STRING);
      g_value_set_string (&value, image);

      panel_plugin_external_queue_add (external,
                                       PROVIDER_PROP_TYPE_SET_BACKGROUND_IMAGE,
                                       &value);

      g_value_unset (&value);
    }
  else
    {
      panel_plugin_external_queue_add_action (external,
                                              PROVIDER_PROP_TYPE_ACTION_BACKGROUND_UNSET);
    }
}



static void
panel_plugin_external_wrapper_x11_set_geometry (PanelPluginExternal *external,
                                                PanelWindow *window)
{
  PanelPluginExternalWrapperX11 *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_X11 (external);
  GtkAllocation alloc;
  GdkRectangle *geom = &wrapper->geometry;

  gtk_widget_get_allocation (GTK_WIDGET (external), &alloc);
  if (alloc.x != geom->x || alloc.y != geom->y || alloc.width != geom->width || alloc.height != geom->height)
    {
      PanelBorders borders = panel_base_window_get_borders (PANEL_BASE_WINDOW (window));
      GValue value = G_VALUE_INIT;

      *geom = alloc;

      /* ignore borders (marching ants): background image is drawn inside */
      if (PANEL_HAS_FLAG (borders, PANEL_BORDER_LEFT))
        alloc.x--;
      if (PANEL_HAS_FLAG (borders, PANEL_BORDER_TOP))
        alloc.y--;

      g_value_init (&value, G_TYPE_VARIANT);
      g_value_set_variant (&value, g_variant_new ("(iiii)", alloc.x, alloc.y, alloc.width, alloc.height));
      panel_plugin_external_queue_add (external, PROVIDER_PROP_TYPE_SET_GEOMETRY, &value);
      g_value_unset (&value);
    }
}



static gboolean
panel_plugin_external_wrapper_x11_pointer_is_outside (PanelPluginExternal *external)
{
  /* Wayland only */
  return FALSE;
}



static void
panel_plugin_external_wrapper_x11_socket_plug_added (GtkSocket *socket,
                                                     PanelPluginExternalWrapperX11 *wrapper)
{
  /* reset geometry when child is respawned */
  wrapper->geometry.x = 0;
  wrapper->geometry.y = 0;
  wrapper->geometry.width = 0;
  wrapper->geometry.height = 0;

  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), TRUE);
}



static gboolean
panel_plugin_external_wrapper_x11_socket_plug_removed (GtkSocket *socket,
                                                       PanelPluginExternalWrapperX11 *wrapper)
{
  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), FALSE);

  return TRUE;
}
