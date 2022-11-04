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
#include <wrapper/wrapper-plug-wayland.h>
#include <wrapper/wrapper-plug.h>



static void     wrapper_plug_wayland_iface_init               (WrapperPlugInterface     *iface);
static void     wrapper_plug_wayland_realize                  (GtkWidget                *widget);
static void     wrapper_plug_wayland_size_allocate            (GtkWidget                *widget,
                                                               GtkAllocation            *allocation);
static void     wrapper_plug_wayland_child_size_allocate      (GtkWidget                *widget,
                                                               GtkAllocation            *allocation);
static void     wrapper_plug_wayland_set_monitor              (WrapperPlug              *plug,
                                                               gint                      monitor);
static void     wrapper_plug_wayland_set_geometry             (WrapperPlug              *plug,
                                                               const GdkRectangle       *geometry);



struct _WrapperPlugWayland
{
  GtkWindow __parent__;

  GdkRectangle geometry;
  GtkRequisition child_size;
  GDBusProxy *proxy;
};



G_DEFINE_FINAL_TYPE_WITH_CODE (WrapperPlugWayland, wrapper_plug_wayland, GTK_TYPE_WINDOW,
                               G_IMPLEMENT_INTERFACE (WRAPPER_TYPE_PLUG,
                                                      wrapper_plug_wayland_iface_init))



static void
wrapper_plug_wayland_class_init (WrapperPlugWaylandClass *klass)
{
  GtkWidgetClass *gtkwidget_class;

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
wrapper_plug_wayland_realize (GtkWidget *widget)
{
  WrapperPlugWayland *plug = WRAPPER_PLUG_WAYLAND (widget);
  GtkWidget *child;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child != NULL)
    wrapper_plug_proxy_provider_signal (XFCE_PANEL_PLUGIN_PROVIDER (child),
                                        PROVIDER_SIGNAL_EMBEDDED, plug->proxy);

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
  GVariantBuilder builder;
  GVariant *variant;
  GtkRequisition size;

  /* set socket requisition */
  gtk_widget_get_preferred_size (widget, NULL, &size);
  plug = WRAPPER_PLUG_WAYLAND (gtk_widget_get_parent (widget));
  if (size.width != plug->child_size.width || size.height != plug->child_size.height)
    {
      plug->child_size = size;
      g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
      variant = g_variant_ref_sink (g_variant_new ("(iiii)", 0, 0, size.width, size.height));
      g_variant_builder_add (&builder, "(uv)", PROVIDER_PROP_TYPE_SET_GEOMETRY, variant);
      g_variant_unref (variant);
      wrapper_plug_proxy_method_call (plug->proxy, "Set", g_variant_builder_end (&builder));
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
  iface->set_monitor = wrapper_plug_wayland_set_monitor;
  iface->set_geometry = wrapper_plug_wayland_set_geometry;
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



GtkWidget *wrapper_plug_wayland_new (GDBusProxy *proxy)
{
  GtkWidget *plug;

  plug = g_object_new (WRAPPER_TYPE_PLUG_WAYLAND, NULL);
  WRAPPER_PLUG_WAYLAND (plug)->proxy = proxy;

  return plug;
}
