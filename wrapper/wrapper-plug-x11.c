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

#include "wrapper-plug-x11.h"
#include "wrapper-plug.h"

#include "common/panel-private.h"



static void
wrapper_plug_x11_finalize (GObject *object);
static void
wrapper_plug_x11_iface_init (WrapperPlugInterface *iface);
static void
wrapper_plug_x11_child_size_allocate (GtkWidget *widget,
                                      GtkAllocation *allocation);
static void
wrapper_plug_x11_proxy_provider_signal (WrapperPlug *plug,
                                        XfcePanelPluginProviderSignal provider_signal,
                                        XfcePanelPluginProvider *provider);
static void
wrapper_plug_x11_proxy_remote_event_result (WrapperPlug *plug,
                                            guint handle,
                                            gboolean result);
static void
wrapper_plug_x11_set_background_color (WrapperPlug *plug,
                                       const gchar *color);
static void
wrapper_plug_x11_set_background_image (WrapperPlug *plug,
                                       const gchar *image);
static void
wrapper_plug_x11_set_geometry (WrapperPlug *plug,
                               const GdkRectangle *geometry);



struct _WrapperPlugX11
{
  GtkPlug __parent__;

  GDBusProxy *proxy;

  /* background information */
  GtkStyleProvider *style_provider;
  gchar *image;
};



G_DEFINE_FINAL_TYPE_WITH_CODE (WrapperPlugX11, wrapper_plug_x11, GTK_TYPE_PLUG,
                               G_IMPLEMENT_INTERFACE (WRAPPER_TYPE_PLUG,
                                                      wrapper_plug_x11_iface_init))



static void
wrapper_plug_x11_class_init (WrapperPlugX11Class *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_x11_finalize;

  /* GtkWidget::size-allocate is flagged G_SIGNAL_RUN_FIRST so we need to overwrite it */
  g_signal_override_class_handler ("size-allocate", XFCE_TYPE_PANEL_PLUGIN,
                                   G_CALLBACK (wrapper_plug_x11_child_size_allocate));
}



static void
wrapper_plug_x11_init (WrapperPlugX11 *plug)
{
  GdkVisual *visual = NULL;
  GdkScreen *screen;
  GtkStyleContext *context;

  gtk_widget_set_name (GTK_WIDGET (plug), "XfcePanelWindowWrapper");

  /* set the colormap */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (plug), visual);

  /* set the panel class */
  context = gtk_widget_get_style_context (GTK_WIDGET (plug));
  gtk_style_context_add_class (context, "panel");
  gtk_style_context_add_class (context, "xfce4-panel");

  gtk_drag_dest_unset (GTK_WIDGET (plug));

  /* add the style provider */
  plug->style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());

  gtk_style_context_add_provider (context, plug->style_provider,
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}



static void
wrapper_plug_x11_finalize (GObject *object)
{
  WrapperPlugX11 *plug = WRAPPER_PLUG_X11 (object);

  g_object_unref (plug->style_provider);
  g_free (plug->image);

  G_OBJECT_CLASS (wrapper_plug_x11_parent_class)->finalize (object);
}



static void
wrapper_plug_x11_child_size_allocate (GtkWidget *widget,
                                      GtkAllocation *allocation)
{
  GtkRequisition size;

  /* avoid allocation warnings */
  gtk_widget_get_preferred_size (widget, NULL, &size);
  if (allocation->width <= 1 && size.width > 1)
    allocation->width = size.width;
  if (allocation->height <= 1 && size.height > 1)
    allocation->height = size.height;

  g_signal_chain_from_overridden_handler (widget, allocation);
}



static void
wrapper_plug_x11_iface_init (WrapperPlugInterface *iface)
{
  iface->proxy_provider_signal = wrapper_plug_x11_proxy_provider_signal;
  iface->proxy_remote_event_result = wrapper_plug_x11_proxy_remote_event_result;
  iface->set_background_color = wrapper_plug_x11_set_background_color;
  iface->set_background_image = wrapper_plug_x11_set_background_image;
  iface->set_geometry = wrapper_plug_x11_set_geometry;
}



static void
wrapper_plug_x11_proxy_provider_signal (WrapperPlug *plug,
                                        XfcePanelPluginProviderSignal provider_signal,
                                        XfcePanelPluginProvider *provider)
{
  wrapper_plug_proxy_method_call_sync (WRAPPER_PLUG_X11 (plug)->proxy, "ProviderSignal",
                                       g_variant_new ("(u)", provider_signal));
}



static void
wrapper_plug_x11_proxy_remote_event_result (WrapperPlug *plug,
                                            guint handle,
                                            gboolean result)
{
  wrapper_plug_proxy_method_call_sync (WRAPPER_PLUG_X11 (plug)->proxy, "RemoteEventResult",
                                       g_variant_new ("(ub)", handle, result));
}



static void
wrapper_plug_x11_set_background_color (WrapperPlug *plug,
                                       const gchar *color)
{
  WrapperPlugX11 *xplug = WRAPPER_PLUG_X11 (plug);
  GdkRGBA rgba;
  gchar *css, *str;

  /* interpret NULL color as user requesting the system theme, so reset the css here */
  if (color == NULL)
    {
      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), "", -1, NULL);
      g_free (xplug->image);
      xplug->image = NULL;
      return;
    }

  if (gdk_rgba_parse (&rgba, color))
    {
      str = gdk_rgba_to_string (&rgba);
      css = g_strdup_printf ("* { background: %s; }", str);
      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), css, -1, NULL);
      g_free (css);
      g_free (str);
    }
}



static void
wrapper_plug_x11_set_background_image (WrapperPlug *plug,
                                       const gchar *image)
{
  WrapperPlugX11 *xplug = WRAPPER_PLUG_X11 (plug);
  GdkRectangle geom = { 0 };

  g_free (xplug->image);
  xplug->image = g_strdup (image);

  /*
   * Normally, socket geometry should always be retrieved in this way (or better still via
   * `gdk_window_get_position()`), but in practice this is only suitable for initialization.
   * This is because :
   * - The coordinates returned by this function may be outdated, and you'll need to mouse
   *   over the plugin to trigger a socket allocation that updates this data (reproducible,
   *   for example, by switching panel mode from vertical to deskbar).
   * - It's even worse with `gdk_window_get_position()`.
   * - The socket window scale factor is not updated when the UI scale factor is changed at
   *   runtime.
   * For all these reasons, sending geometry via D-Bus is simpler and more reliable.
   */
  gdk_window_get_geometry (gtk_plug_get_socket_window (GTK_PLUG (plug)), &geom.x, &geom.y, NULL, NULL);
  wrapper_plug_x11_set_geometry (plug, &geom);
}



static void
wrapper_plug_x11_set_geometry (WrapperPlug *plug,
                               const GdkRectangle *geometry)
{
  WrapperPlugX11 *xplug = WRAPPER_PLUG_X11 (plug);

  /* do not scale background image with the panel */
  gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (plug));
  gchar *css_url = g_strdup_printf ("url(\"%s\")", xplug->image);
  for (gint i = 1; i < scale_factor; i++)
    {
      gchar *temp = g_strdup_printf ("%s,url(\"%s\")", css_url, xplug->image);
      g_free (css_url);
      css_url = temp;
    }
  gchar *css = g_strdup_printf ("* { background: -gtk-scaled(%s) %dpx %dpx; }",
                                css_url, -geometry->x, -geometry->y);
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), css, -1, NULL);
  g_free (css);
  g_free (css_url);
}



GtkWidget *
wrapper_plug_x11_new (Window socket_id,
                      GDBusProxy *proxy)
{
  WrapperPlugX11 *plug;

  plug = g_object_new (WRAPPER_TYPE_PLUG_X11, NULL);
  gtk_plug_construct (GTK_PLUG (plug), socket_id);
  plug->proxy = proxy;

  return GTK_WIDGET (plug);
}
