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
#ifdef GDK_WINDOWING_WAYLAND

#include <gtk-layer-shell/gtk-layer-shell.h>

#include <common/panel-private.h>
#include <wrapper/wrapper-plug-wayland.h>
#include <wrapper/wrapper-plug.h>
#include <common/panel-dbus.h>
#include <wrapper/wrapper-external-exported.h>



static void       wrapper_plug_wayland_iface_init                   (WrapperPlugInterface             *iface);
static void       wrapper_plug_wayland_finalize                     (GObject                          *object);
static void       wrapper_plug_wayland_realize                      (GtkWidget                        *widget);
static void       wrapper_plug_wayland_size_allocate                (GtkWidget                        *widget,
                                                                     GtkAllocation                    *allocation);
static void       wrapper_plug_wayland_child_size_allocate          (GtkWidget                        *widget,
                                                                     GtkAllocation                    *allocation);
static void       wrapper_plug_wayland_proxy_provider_signal        (WrapperPlug                      *plug,
                                                                     XfcePanelPluginProviderSignal     provider_signal,
                                                                     XfcePanelPluginProvider          *provider);
static void       wrapper_plug_wayland_proxy_remote_event_result    (WrapperPlug                      *plug,
                                                                     guint                             handle,
                                                                     gboolean                          result);
static void       wrapper_plug_wayland_set_monitor                  (WrapperPlug                      *plug,
                                                                     gint                              monitor);
static void       wrapper_plug_wayland_set_geometry                 (WrapperPlug                      *plug,
                                                                     const GdkRectangle               *geometry);



struct _WrapperPlugWayland
{
  GtkWindow __parent__;

  GdkRectangle geometry;
  GtkRequisition child_size;
  GDBusProxy *proxy;
  GDBusConnection *connection;
  WrapperExternalExported *skeleton;
  guint watcher_id;
};



G_DEFINE_FINAL_TYPE_WITH_CODE (WrapperPlugWayland, wrapper_plug_wayland, GTK_TYPE_WINDOW,
                               G_IMPLEMENT_INTERFACE (WRAPPER_TYPE_PLUG,
                                                      wrapper_plug_wayland_iface_init))



static void
wrapper_plug_wayland_class_init (WrapperPlugWaylandClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_wayland_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = wrapper_plug_wayland_realize;
  gtkwidget_class->size_allocate = wrapper_plug_wayland_size_allocate;

  /* GtkWidget::size-allocate is flagged G_SIGNAL_RUN_FIRST so we need to overwrite it */
  g_signal_override_class_handler ("size-allocate", XFCE_TYPE_PANEL_PLUGIN,
                                   G_CALLBACK (wrapper_plug_wayland_child_size_allocate));
}



static void
wrapper_plug_wayland_init (WrapperPlugWayland *plug)
{
  GtkStyleContext *context;
  GtkCssProvider *provider;

  plug->geometry.width = 1;
  plug->geometry.height = 1;

  /* add panel css classes so they apply to external plugins as they do to internal ones */
  context = gtk_widget_get_style_context (GTK_WIDGET (plug));
  gtk_style_context_add_class (context, "panel");
  gtk_style_context_add_class (context, "xfce4-panel");

  /* make plug background transparent so it appears as the panel background */
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, "* { background: rgba(0,0,0,0); }", -1, NULL);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  /* set layer shell properties */
  gtk_layer_init_for_window (GTK_WINDOW (plug));
  gtk_layer_set_layer (GTK_WINDOW (plug), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_exclusive_zone (GTK_WINDOW (plug), -1);
  gtk_layer_set_anchor (GTK_WINDOW (plug), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
  gtk_layer_set_anchor (GTK_WINDOW (plug), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
}



static void
wrapper_plug_wayland_finalize (GObject *object)
{
  WrapperPlugWayland *plug = WRAPPER_PLUG_WAYLAND (object);

  if (plug->skeleton != NULL)
    g_object_unref (plug->skeleton);
  if (plug->watcher_id != 0)
    g_bus_unwatch_name (plug->watcher_id);

  G_OBJECT_CLASS (wrapper_plug_wayland_parent_class)->finalize (object);
}



static void
wrapper_plug_wayland_realize (GtkWidget *widget)
{
  WrapperPlugWayland *plug = WRAPPER_PLUG_WAYLAND (widget);
  const gchar *path = g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (plug->skeleton));

  g_dbus_connection_emit_signal (plug->connection, NULL, path, PANEL_DBUS_EXTERNAL_INTERFACE,
                                 "Embedded", NULL, NULL);

  GTK_WIDGET_CLASS (wrapper_plug_wayland_parent_class)->realize (widget);
}



static void
wrapper_plug_wayland_size_allocate (GtkWidget *widget,
                                    GtkAllocation *allocation)
{
  WrapperPlugWayland *plug = WRAPPER_PLUG_WAYLAND (widget);

  /* acts as an upper bound for allocation (see set_geometry() below) */
  allocation->width = plug->geometry.width;
  allocation->height = plug->geometry.height;

  GTK_WIDGET_CLASS (wrapper_plug_wayland_parent_class)->size_allocate (widget, allocation);
}



static void
wrapper_plug_wayland_child_size_allocate (GtkWidget *widget,
                                          GtkAllocation *allocation)
{
  WrapperPlugWayland *plug;
  GtkRequisition size;

  /* set socket requisition */
  gtk_widget_get_preferred_size (widget, NULL, &size);
  plug = WRAPPER_PLUG_WAYLAND (gtk_widget_get_parent (widget));
  if (size.width != plug->child_size.width || size.height != plug->child_size.height)
    {
      const gchar *path = g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (plug->skeleton));
      plug->child_size = size;
      g_dbus_connection_emit_signal (plug->connection, NULL, path, PANEL_DBUS_EXTERNAL_INTERFACE,
                                     "SetRequisition", g_variant_new ("((ii))", size.width, size.height), NULL);
    }

  /* avoid allocation warnings */
  if (allocation->width <= 1 && size.width > 1)
    allocation->width = size.width;
  if (allocation->height <= 1 && size.height > 1)
    allocation->height = size.height;

  g_signal_chain_from_overridden_handler (widget, allocation);
}



static void
wrapper_plug_wayland_iface_init (WrapperPlugInterface *iface)
{
  iface->proxy_provider_signal = wrapper_plug_wayland_proxy_provider_signal;
  iface->proxy_remote_event_result = wrapper_plug_wayland_proxy_remote_event_result;
  iface->set_monitor = wrapper_plug_wayland_set_monitor;
  iface->set_geometry = wrapper_plug_wayland_set_geometry;
}



static void
wrapper_plug_wayland_proxy_provider_signal (WrapperPlug *plug,
                                            XfcePanelPluginProviderSignal provider_signal,
                                            XfcePanelPluginProvider *provider)
{
  wrapper_plug_proxy_method_call (WRAPPER_PLUG_WAYLAND (plug)->proxy, "ProviderSignal",
                                  g_variant_new ("(u)", provider_signal));
}



static void
wrapper_plug_wayland_proxy_remote_event_result (WrapperPlug *plug,
                                                guint handle,
                                                gboolean result)
{
  wrapper_plug_proxy_method_call (WRAPPER_PLUG_WAYLAND (plug)->proxy, "RemoteEventResult",
                                  g_variant_new ("(ub)", handle, result));
}



static void
wrapper_plug_wayland_set_monitor (WrapperPlug *plug,
                                  gint monitor)
{
  gtk_layer_set_monitor (GTK_WINDOW (plug),
                         gdk_display_get_monitor (gdk_display_get_default (), monitor));
}



static void
wrapper_plug_wayland_set_geometry (WrapperPlug *plug,
                                   const GdkRectangle *geometry)
{
  WRAPPER_PLUG_WAYLAND (plug)->geometry = *geometry;

  /* acts as a lower bound for allocation (see size_allocate() above) */
  gtk_widget_set_size_request (GTK_WIDGET (plug), geometry->width, geometry->height);

  gtk_layer_set_margin (GTK_WINDOW (plug), GTK_LAYER_SHELL_EDGE_LEFT, geometry->x);
  gtk_layer_set_margin (GTK_WINDOW (plug), GTK_LAYER_SHELL_EDGE_TOP, geometry->y);
}



static void
wrapper_plug_wayland_name_vanished (GDBusConnection *connection,
                                    const gchar *name,
                                    gpointer plug)
{
  g_critical ("Name %s lost on D-Bus", name);
  g_object_set_data (plug, "exit-code", GINT_TO_POINTER (PLUGIN_EXIT_NAME_LOST));
  gtk_main_quit ();
}



static void
wrapper_plug_wayland_name_lost (GDBusConnection *connection,
                                const gchar *name,
                                gpointer loop)
{
  g_main_loop_quit (loop);
}



static void
wrapper_plug_wayland_name_acquired (GDBusConnection *connection,
                                    const gchar *name,
                                    gpointer loop)
{
  g_object_set_data (G_OBJECT (connection), "name-acquired", GINT_TO_POINTER (TRUE));
  g_main_loop_quit (loop);
}



static void
wrapper_plug_wayland_own_name (GTask *task,
                               gpointer connection,
                               gpointer name,
                               GCancellable *cancellable)
{
  GMainContext *context;
  GMainLoop *loop;

  context = g_main_context_new ();
  g_main_context_push_thread_default (context);
  loop = g_main_loop_new (context, FALSE);
  g_bus_own_name_on_connection (connection, name,
                                G_BUS_NAME_OWNER_FLAGS_NONE,
                                wrapper_plug_wayland_name_acquired,
                                wrapper_plug_wayland_name_lost,
                                loop, NULL);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
  g_main_context_unref (context);
}



GtkWidget *
wrapper_plug_wayland_new (gint unique_id,
                          GDBusProxy *proxy,
                          GError **error)
{
  WrapperPlugWayland *plug;
  WrapperExternalExported *skeleton;
  GDBusConnection *connection;
  GTask *task;
  gchar *path, *name;

  skeleton = wrapper_external_exported_skeleton_new ();
  connection = g_dbus_proxy_get_connection (proxy);
  path = g_strdup_printf (PANEL_DBUS_PLUGIN_PATH, unique_id);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton), connection, path, error);
  g_free (path);
  if (*error != NULL)
    return NULL;

  task = g_task_new (connection, NULL, NULL, NULL);
  name = g_strdup_printf (PANEL_DBUS_PLUGIN_NAME, unique_id);
  g_task_set_task_data (task, name, NULL);
  g_task_run_in_thread_sync (task, wrapper_plug_wayland_own_name);
  g_object_unref (task);
  if (! g_object_get_data (G_OBJECT (connection), "name-acquired"))
    {
      g_set_error (error, 0, 0, "Failed to acquire name %s on D-Bus", name);
      g_free (name);
      return NULL;
    }

  plug = g_object_new (WRAPPER_TYPE_PLUG_WAYLAND, NULL);
  g_signal_connect (skeleton, "handle-pointer-is-outside",
                    G_CALLBACK (wrapper_plug_wayland_handle_pointer_is_outside), plug);
  plug->watcher_id = g_bus_watch_name_on_connection (connection, name,
                                                     G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                     NULL, wrapper_plug_wayland_name_vanished,
                                                     plug, NULL);
  plug->proxy = proxy;
  plug->connection = connection;
  plug->skeleton = skeleton;

  g_free (name);

  return GTK_WIDGET (plug);
}

#endif /* ! GDK_WINDOWING_WAYLAND */
