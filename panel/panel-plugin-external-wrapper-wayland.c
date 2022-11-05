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

#include <gtk-layer-shell/gtk-layer-shell.h>

#include <common/panel-private.h>
#include <panel/panel-plugin-external-wrapper-wayland.h>



static void         panel_plugin_external_wrapper_wayland_size_allocate           (GtkWidget                        *widget,
                                                                                   GtkAllocation                    *allocation);
static gchar      **panel_plugin_external_wrapper_wayland_get_argv                (PanelPluginExternal              *external,
                                                                                   gchar                           **arguments);
static void         panel_plugin_external_wrapper_wayland_set_background_color    (PanelPluginExternal              *external,
                                                                                   const GdkRGBA                    *color);
static void         panel_plugin_external_wrapper_wayland_set_background_image    (PanelPluginExternal              *external,
                                                                                   const gchar                      *image);
static void         panel_plugin_external_wrapper_wayland_set_geometry            (PanelPluginExternal              *external,
                                                                                   PanelWindow                      *window);



struct _PanelPluginExternalWrapperWayland
{
  PanelPluginExternalWrapper __parent__;

  GdkMonitor *monitor;
  GdkRectangle geometry;
};



G_DEFINE_FINAL_TYPE (PanelPluginExternalWrapperWayland, panel_plugin_external_wrapper_wayland, PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER)



static void
panel_plugin_external_wrapper_wayland_class_init (PanelPluginExternalWrapperWaylandClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  PanelPluginExternalClass *external_class;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_allocate = panel_plugin_external_wrapper_wayland_size_allocate;

  external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  external_class->get_argv = panel_plugin_external_wrapper_wayland_get_argv;
  external_class->set_background_color = panel_plugin_external_wrapper_wayland_set_background_color;
  external_class->set_background_image = panel_plugin_external_wrapper_wayland_set_background_image;
  external_class->set_geometry = panel_plugin_external_wrapper_wayland_set_geometry;
}



static void
panel_plugin_external_wrapper_wayland_init (PanelPluginExternalWrapperWayland *wrapper)
{
}



static void
panel_plugin_external_wrapper_wayland_size_allocate (GtkWidget *widget,
                                                     GtkAllocation *allocation)
{
  PanelWindow *window = PANEL_WINDOW (gtk_widget_get_ancestor (widget, PANEL_TYPE_WINDOW));

  GTK_WIDGET_CLASS (panel_plugin_external_wrapper_wayland_parent_class)->size_allocate (widget, allocation);

  panel_plugin_external_wrapper_wayland_set_geometry (PANEL_PLUGIN_EXTERNAL (widget), window);
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
}



static void
panel_plugin_external_wrapper_wayland_set_background_image (PanelPluginExternal *external,
                                                            const gchar *image)
{
}



static void
panel_plugin_external_wrapper_wayland_set_geometry (PanelPluginExternal *external,
                                                    PanelWindow *window)
{
  PanelPluginExternalWrapperWayland *wrapper = PANEL_PLUGIN_EXTERNAL_WRAPPER_WAYLAND (external);
  GtkAllocation alloc;
  GdkRectangle *geom = &wrapper->geometry;
  GdkDisplay *display;
  GdkMonitor *monitor;
  gint x, y;

  display = gdk_display_get_default ();
  monitor = gdk_display_get_monitor_at_window (display, gtk_widget_get_window (GTK_WIDGET (external)));
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
  x = alloc.x + gtk_layer_get_margin (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT);
  y = alloc.y + gtk_layer_get_margin (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_TOP);
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
