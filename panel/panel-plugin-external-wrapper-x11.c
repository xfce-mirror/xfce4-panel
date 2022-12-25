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
#include <config.h>
#endif

#include <gtk/gtkx.h>

#include <common/panel-private.h>
#include <panel/panel-plugin-external-wrapper-x11.h>



static gchar      **panel_plugin_external_wrapper_x11_get_argv                (PanelPluginExternal              *external,
                                                                               gchar                           **arguments);
static void         panel_plugin_external_wrapper_x11_set_background_color    (PanelPluginExternal              *external,
                                                                               const GdkRGBA                    *color);
static void         panel_plugin_external_wrapper_x11_set_background_image    (PanelPluginExternal              *external,
                                                                               const gchar                      *image);
static void         panel_plugin_external_wrapper_x11_set_geometry            (PanelPluginExternal              *external,
                                                                               PanelWindow                      *window);
static gboolean     panel_plugin_external_wrapper_x11_pointer_is_outside      (PanelPluginExternal              *external);
static void         panel_plugin_external_wrapper_x11_socket_plug_added       (GtkSocket                        *socket,
                                                                               PanelPluginExternalWrapperX11    *wrapper);
static gboolean     panel_plugin_external_wrapper_x11_socket_plug_removed     (GtkSocket                        *socket,
                                                                               PanelPluginExternalWrapperX11    *wrapper);



struct _PanelPluginExternalWrapperX11
{
  PanelPluginExternalWrapper __parent__;

  GtkWidget *socket;
};



G_DEFINE_FINAL_TYPE (PanelPluginExternalWrapperX11, panel_plugin_external_wrapper_x11, PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER)



static void
panel_plugin_external_wrapper_x11_class_init (PanelPluginExternalWrapperX11Class *klass)
{
  PanelPluginExternalClass *external_class;

  external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  external_class->get_argv = panel_plugin_external_wrapper_x11_get_argv;
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
  /* Wayland only */
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
  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), TRUE);
}



static gboolean
panel_plugin_external_wrapper_x11_socket_plug_removed (GtkSocket *socket,
                                                       PanelPluginExternalWrapperX11 *wrapper)
{
  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), FALSE);

  return TRUE;
}
