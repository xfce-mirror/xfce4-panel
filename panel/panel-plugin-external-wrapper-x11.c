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

#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11

#include <gtk/gtkx.h>

#include <common/panel-private.h>
#include <panel/panel-plugin-external-wrapper-x11.h>



static void         panel_plugin_external_wrapper_x11_finalize                (GObject                          *object);
static gchar      **panel_plugin_external_wrapper_x11_get_argv                (PanelPluginExternal              *external,
                                                                               gchar                           **arguments);
static void         panel_plugin_external_wrapper_x11_set_background_color    (PanelPluginExternal              *external,
                                                                               const GdkRGBA                    *color);
static void         panel_plugin_external_wrapper_x11_set_background_image    (PanelPluginExternal              *external,
                                                                               const gchar                      *image);
static void         panel_plugin_external_wrapper_x11_set_geometry            (PanelPluginExternal              *external,
                                                                               PanelWindow                      *window);
static gboolean     panel_plugin_external_wrapper_x11_pointer_is_outside      (PanelPluginExternal              *external);
static void         panel_plugin_external_wrapper_x11_socket_size_allocate    (GtkWidget                        *widget,
                                                                               GtkAllocation                    *allocation);
static void         panel_plugin_external_wrapper_x11_socket_plug_added       (GtkSocket                        *socket,
                                                                               PanelPluginExternalWrapperX11    *wrapper);
static gboolean     panel_plugin_external_wrapper_x11_socket_plug_removed     (GtkSocket                        *socket,
                                                                               PanelPluginExternalWrapperX11    *wrapper);



struct _PanelPluginExternalWrapperX11
{
  PanelPluginExternalWrapper __parent__;

  GtkWidget *socket;

  /* silence allocation warnings */
  guint resize_timeout_id;
};

gulong global_resize_timeout_id = 0;



G_DEFINE_FINAL_TYPE (PanelPluginExternalWrapperX11, panel_plugin_external_wrapper_x11, PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER)



static void
panel_plugin_external_wrapper_x11_class_init (PanelPluginExternalWrapperX11Class *klass)
{
  GObjectClass *gobject_class;
  PanelPluginExternalClass *external_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_wrapper_x11_finalize;

  external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  external_class->get_argv = panel_plugin_external_wrapper_x11_get_argv;
  external_class->set_background_color = panel_plugin_external_wrapper_x11_set_background_color;
  external_class->set_background_image = panel_plugin_external_wrapper_x11_set_background_image;
  external_class->set_geometry = panel_plugin_external_wrapper_x11_set_geometry;
  external_class->pointer_is_outside = panel_plugin_external_wrapper_x11_pointer_is_outside;

  /* GtkWidget::size-allocate is flagged G_SIGNAL_RUN_FIRST so we need to overwrite it */
  g_signal_override_class_handler ("size-allocate", GTK_TYPE_SOCKET,
                                   G_CALLBACK (panel_plugin_external_wrapper_x11_socket_size_allocate));
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
panel_plugin_external_wrapper_x11_finalize (GObject *object)
{
  PanelPluginExternalWrapperX11 *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_X11 (object);

  if (wrapper->resize_timeout_id != 0)
    {
      g_source_remove (wrapper->resize_timeout_id);
      global_resize_timeout_id -= wrapper->resize_timeout_id;
    }

  G_OBJECT_CLASS (panel_plugin_external_wrapper_x11_parent_class)->finalize (object);
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
panel_plugin_external_wrapper_x11_queue_resize (GtkWidget *widget,
                                                gpointer data)
{
  if (PANEL_IS_PLUGIN_EXTERNAL (data))
    gtk_widget_queue_resize (data);
}



static gboolean
panel_plugin_external_wrapper_x11_queue_resize_timeout (gpointer data)
{
  PanelPluginExternalWrapperX11 *wrapper = data;

  if (! gdk_window_is_visible (gtk_socket_get_plug_window (GTK_SOCKET (wrapper->socket))))
    return TRUE;

  global_resize_timeout_id -= wrapper->resize_timeout_id;
  wrapper->resize_timeout_id = 0;
  if (global_resize_timeout_id == 0)
    gtk_container_foreach (GTK_CONTAINER (gtk_widget_get_parent (data)),
                           panel_plugin_external_wrapper_x11_queue_resize, data);

  return FALSE;
}



static void
panel_plugin_external_wrapper_x11_socket_size_allocate (GtkWidget *widget,
                                                        GtkAllocation *allocation)
{
  PanelPluginExternalWrapperX11 *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_X11 (gtk_widget_get_parent (widget));

  if (global_resize_timeout_id != 0)
    {
      if (wrapper->resize_timeout_id != 0)
        {
          global_resize_timeout_id -= wrapper->resize_timeout_id;
          g_source_remove (wrapper->resize_timeout_id);
          wrapper->resize_timeout_id =
            g_timeout_add_seconds (1, panel_plugin_external_wrapper_x11_queue_resize_timeout, wrapper);
          global_resize_timeout_id += wrapper->resize_timeout_id;
        }

      allocation->width = MAX (allocation->width, 16);
      allocation->height = MAX (allocation->height, 16);
    }

  g_signal_chain_from_overridden_handler (widget, allocation);
}



static void
panel_plugin_external_wrapper_x11_socket_plug_added (GtkSocket *socket,
                                                     PanelPluginExternalWrapperX11 *wrapper)
{
  wrapper->resize_timeout_id =
    g_timeout_add_seconds (1, panel_plugin_external_wrapper_x11_queue_resize_timeout, wrapper);
  global_resize_timeout_id += wrapper->resize_timeout_id;

  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), TRUE);
}



static gboolean
panel_plugin_external_wrapper_x11_socket_plug_removed (GtkSocket *socket,
                                                       PanelPluginExternalWrapperX11 *wrapper)
{
  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), FALSE);

  return TRUE;
}

#endif /* ! GDK_WINDOWING_X11 */
