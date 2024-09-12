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

#include "panel-plugin-external-wrapper-exported.h"
#include "panel-plugin-external-wrapper-wayland.h"

#include "common/panel-dbus.h"
#include "common/panel-debug.h"
#include "common/panel-private.h"

#include <gtk-layer-shell.h>



static void
panel_plugin_external_wrapper_wayland_constructed (GObject *object);
static void
panel_plugin_external_wrapper_wayland_finalize (GObject *object);
static void
panel_plugin_external_wrapper_wayland_size_allocate (GtkWidget *widget,
                                                     GtkAllocation *allocation);
static gchar **
panel_plugin_external_wrapper_wayland_get_argv (PanelPluginExternal *external,
                                                gchar **arguments);
static gboolean
panel_plugin_external_wrapper_wayland_spawn (PanelPluginExternal *external,
                                             gchar **argv,
                                             GPid *pid,
                                             GError **error);
static void
panel_plugin_external_wrapper_wayland_set_background_color (PanelPluginExternal *external,
                                                            const GdkRGBA *color);
static void
panel_plugin_external_wrapper_wayland_set_background_image (PanelPluginExternal *external,
                                                            const gchar *image);
static void
panel_plugin_external_wrapper_wayland_set_geometry (PanelPluginExternal *external,
                                                    PanelWindow *window);
static void
panel_plugin_external_wrapper_wayland_proxy_set_requisition (GDBusProxy *proxy,
                                                             gchar *sender_name,
                                                             gchar *signal_name,
                                                             GVariant *parameters,
                                                             PanelPluginExternalWrapperWayland *wrapper);
static void
panel_plugin_external_wrapper_wayland_proxy_embedded (GDBusProxy *proxy,
                                                      gchar *sender_name,
                                                      gchar *signal_name,
                                                      GVariant *parameters,
                                                      PanelPluginExternalWrapperWayland *wrapper);
static void
panel_plugin_external_wrapper_wayland_proxy_pointer_enter (GDBusProxy *proxy,
                                                           gchar *sender_name,
                                                           gchar *signal_name,
                                                           GVariant *parameters,
                                                           PanelPluginExternalWrapperWayland *wrapper);
static void
panel_plugin_external_wrapper_wayland_proxy_pointer_leave (GDBusProxy *proxy,
                                                           gchar *sender_name,
                                                           gchar *signal_name,
                                                           GVariant *parameters,
                                                           PanelPluginExternalWrapperWayland *wrapper);
static gboolean
panel_plugin_external_wrapper_wayland_pointer_is_outside (PanelPluginExternal *external);



struct _PanelPluginExternalWrapperWayland
{
  PanelPluginExternalWrapper __parent__;

  GDBusProxy *proxy;
  GdkMonitor *monitor;
  GdkRectangle geometry;
};



G_DEFINE_FINAL_TYPE (PanelPluginExternalWrapperWayland, panel_plugin_external_wrapper_wayland, PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER)



static void
panel_plugin_external_wrapper_wayland_class_init (PanelPluginExternalWrapperWaylandClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  PanelPluginExternalClass *external_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = panel_plugin_external_wrapper_wayland_constructed;
  gobject_class->finalize = panel_plugin_external_wrapper_wayland_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_allocate = panel_plugin_external_wrapper_wayland_size_allocate;

  external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  external_class->get_argv = panel_plugin_external_wrapper_wayland_get_argv;
  external_class->spawn = panel_plugin_external_wrapper_wayland_spawn;
  external_class->set_background_color = panel_plugin_external_wrapper_wayland_set_background_color;
  external_class->set_background_image = panel_plugin_external_wrapper_wayland_set_background_image;
  external_class->set_geometry = panel_plugin_external_wrapper_wayland_set_geometry;
  external_class->pointer_is_outside = panel_plugin_external_wrapper_wayland_pointer_is_outside;
}



static void
panel_plugin_external_wrapper_wayland_init (PanelPluginExternalWrapperWayland *wrapper)
{
}



static void
panel_plugin_external_wrapper_wayland_constructed (GObject *object)
{
  PanelPluginExternalWrapperWayland *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_WAYLAND (object);
  XfcePanelPluginWrapperExported *skeleton;
  GDBusConnection *connection;
  GError *error = NULL;
  gchar *path, *name;
  gint unique_id;

  /* first let ExternalWrapper do its part of the D-Bus job */
  G_OBJECT_CLASS (panel_plugin_external_wrapper_wayland_parent_class)->constructed (object);
  g_object_get (object, "skeleton", &skeleton, NULL);
  if (skeleton == NULL)
    return;

  connection = g_dbus_interface_skeleton_get_connection (G_DBUS_INTERFACE_SKELETON (skeleton));
  g_object_unref (skeleton);
  if (connection == NULL)
    return;

  g_object_get (object, "unique-id", &unique_id, NULL);
  name = g_strdup_printf (PANEL_DBUS_PLUGIN_NAME, unique_id);
  path = g_strdup_printf (PANEL_DBUS_PLUGIN_PATH, unique_id);
  wrapper->proxy = g_dbus_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE, NULL, name, path,
                                          PANEL_DBUS_EXTERNAL_INTERFACE, NULL, &error);
  if (wrapper->proxy != NULL)
    {
      /* PluginExternal gives WrapperPlug its geometry (position and size), which in turn
       * gives it its requisition (size only) */
      g_signal_connect (wrapper->proxy, "g-signal::SetRequisition",
                        G_CALLBACK (panel_plugin_external_wrapper_wayland_proxy_set_requisition), object);

      /* when the plug is ready to receive its geometry, in particular */
      g_signal_connect (wrapper->proxy, "g-signal::Embedded",
                        G_CALLBACK (panel_plugin_external_wrapper_wayland_proxy_embedded), object);

      /* forward pointer events from the plug to the panel */
      g_signal_connect (wrapper->proxy, "g-signal::PointerEnter",
                        G_CALLBACK (panel_plugin_external_wrapper_wayland_proxy_pointer_enter), object);
      g_signal_connect (wrapper->proxy, "g-signal::PointerLeave",
                        G_CALLBACK (panel_plugin_external_wrapper_wayland_proxy_pointer_leave), object);

      panel_debug (PANEL_DEBUG_EXTERNAL, "Created D-BUS proxy at path %s", path);
    }
  else
    {
      g_critical ("Failed to create D-BUS proxy at path %s: %s", path, error->message);
      g_error_free (error);
    }

  g_free (name);
  g_free (path);
}



static void
panel_plugin_external_wrapper_wayland_finalize (GObject *object)
{
  PanelPluginExternalWrapperWayland *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_WAYLAND (object);

  if (wrapper->proxy != NULL)
    g_object_unref (wrapper->proxy);

  G_OBJECT_CLASS (panel_plugin_external_wrapper_wayland_parent_class)->finalize (object);
}



static void
panel_plugin_external_wrapper_wayland_size_allocate (GtkWidget *widget,
                                                     GtkAllocation *allocation)
{
  PanelWindow *window = PANEL_WINDOW (gtk_widget_get_ancestor (widget, PANEL_TYPE_WINDOW));

  GTK_WIDGET_CLASS (panel_plugin_external_wrapper_wayland_parent_class)->size_allocate (widget, allocation);

  panel_plugin_external_wrapper_wayland_set_geometry (PANEL_PLUGIN_EXTERNAL (widget), window);
}



static gboolean
panel_plugin_external_wrapper_wayland_spawn (PanelPluginExternal *external,
                                             gchar **argv,
                                             GPid *pid,
                                             GError **error)
{
  return g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, pid, error);
}



static gchar **
panel_plugin_external_wrapper_wayland_get_argv (PanelPluginExternal *external,
                                                gchar **arguments)
{
  gchar **argv;

  argv = PANEL_PLUGIN_EXTERNAL_CLASS (panel_plugin_external_wrapper_wayland_parent_class)->get_argv (external, arguments);
  if (argv != NULL)
    argv[PLUGIN_ARGV_SOCKET_ID] = g_strdup_printf ("%lu", 0LU);

  return argv;
}



static void
panel_plugin_external_wrapper_wayland_set_background_color (PanelPluginExternal *external,
                                                            const GdkRGBA *color)
{
  /* X11 only */
}



static void
panel_plugin_external_wrapper_wayland_set_background_image (PanelPluginExternal *external,
                                                            const gchar *image)
{
  /* X11 only */
}



static void
panel_plugin_external_wrapper_wayland_set_geometry (PanelPluginExternal *external,
                                                    PanelWindow *window)
{
  PanelPluginExternalWrapperWayland *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_WAYLAND (external);
  GtkAllocation alloc;
  GdkRectangle *geom = &wrapper->geometry;
  GdkDisplay *display;
  GdkWindow *gdkwindow;
  GdkMonitor *monitor = NULL;
  gint x, y;

  display = gdk_display_get_default ();
  gdkwindow = gtk_widget_get_window (GTK_WIDGET (external));
  if (gdkwindow != NULL)
    monitor = gdk_display_get_monitor_at_window (display, gdkwindow);

  if (monitor != wrapper->monitor)
    {
      GValue value = G_VALUE_INIT;
      gint n, n_monitors;

      wrapper->monitor = monitor;
      n_monitors = gdk_display_get_n_monitors (display);
      for (n = 0; n < n_monitors; n++)
        if (gdk_display_get_monitor (display, n) == monitor)
          break;

      g_value_init (&value, G_TYPE_INT);
      g_value_set_int (&value, n);
      panel_plugin_external_queue_add (external, PROVIDER_PROP_TYPE_SET_MONITOR, &value);
      g_value_unset (&value);
    }

  gtk_widget_get_allocation (GTK_WIDGET (external), &alloc);
  if (gtk_widget_get_visible (GTK_WIDGET (window)))
    {
      x = alloc.x + gtk_layer_get_margin (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT);
      y = alloc.y + gtk_layer_get_margin (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_TOP);
    }
  else
    {
      x = y = OFFSCREEN;
    }
  if (x != geom->x || y != geom->y || alloc.width != geom->width || alloc.height != geom->height)
    {
      GValue value = G_VALUE_INIT;

      geom->x = x;
      geom->y = y;
      geom->width = alloc.width;
      geom->height = alloc.height;
      g_value_init (&value, G_TYPE_VARIANT);
      g_value_set_variant (&value, g_variant_new ("(iiii)", x, y, alloc.width, alloc.height));
      panel_plugin_external_queue_add (external, PROVIDER_PROP_TYPE_SET_GEOMETRY, &value);
      g_value_unset (&value);
    }
}



static void
panel_plugin_external_wrapper_wayland_proxy_set_requisition (GDBusProxy *proxy,
                                                             gchar *sender_name,
                                                             gchar *signal_name,
                                                             GVariant *parameters,
                                                             PanelPluginExternalWrapperWayland *wrapper)
{
  gint width, height;

  g_variant_get (parameters, "((ii))", &width, &height);
  gtk_widget_set_size_request (GTK_WIDGET (wrapper), width, height);
}



static void
panel_plugin_external_wrapper_wayland_proxy_embedded (GDBusProxy *proxy,
                                                      gchar *sender_name,
                                                      gchar *signal_name,
                                                      GVariant *parameters,
                                                      PanelPluginExternalWrapperWayland *wrapper)
{
  /* reset geometry when child is respawned */
  wrapper->monitor = NULL;
  wrapper->geometry.x = 0;
  wrapper->geometry.y = 0;
  wrapper->geometry.width = 0;
  wrapper->geometry.height = 0;

  panel_plugin_external_set_embedded (PANEL_PLUGIN_EXTERNAL (wrapper), TRUE);
}



static void
panel_plugin_external_wrapper_wayland_proxy_pointer_enter (GDBusProxy *proxy,
                                                           gchar *sender_name,
                                                           gchar *signal_name,
                                                           GVariant *parameters,
                                                           PanelPluginExternalWrapperWayland *wrapper)
{
  GtkWidget *toplevel;
  GdkEventCrossing event = { 0 };

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (wrapper));
  event.type = GDK_ENTER_NOTIFY;
  event.window = gtk_widget_get_window (toplevel);
  event.time = GDK_CURRENT_TIME;
  event.mode = GDK_CROSSING_NORMAL;
  event.detail = GDK_NOTIFY_NONLINEAR;
  GTK_WIDGET_GET_CLASS (toplevel)->enter_notify_event (toplevel, &event);
}



static void
panel_plugin_external_wrapper_wayland_proxy_pointer_leave (GDBusProxy *proxy,
                                                           gchar *sender_name,
                                                           gchar *signal_name,
                                                           GVariant *parameters,
                                                           PanelPluginExternalWrapperWayland *wrapper)
{
  GtkWidget *toplevel;
  GdkEventCrossing event = { 0 };

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (wrapper));
  event.type = GDK_ENTER_NOTIFY;
  event.window = gtk_widget_get_window (toplevel);
  event.time = GDK_CURRENT_TIME;
  event.mode = GDK_CROSSING_NORMAL;
  event.detail = GDK_NOTIFY_NONLINEAR;
  GTK_WIDGET_GET_CLASS (toplevel)->leave_notify_event (toplevel, &event);
}



static gboolean
panel_plugin_external_wrapper_wayland_pointer_is_outside (PanelPluginExternal *external)
{
  PanelPluginExternalWrapperWayland *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_WAYLAND (external);
  GVariant *retv;
  GError *error = NULL;
  gboolean ret = FALSE;

  retv = g_dbus_proxy_call_sync (wrapper->proxy, "PointerIsOutside", NULL,
                                 G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (retv != NULL)
    {
      g_variant_get (retv, "(b)", &ret);
      g_variant_unref (retv);
    }
  else
    {
      g_warning ("PointerIsOutside call failed: %s", error->message);
      g_error_free (error);
    }

  return ret;
}
