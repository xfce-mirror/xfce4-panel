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

#ifdef HAVE_GTK_X11
#include <wrapper/wrapper-plug-x11.h>
#endif
#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <wrapper/wrapper-plug-wayland.h>
#endif

#include <common/panel-private.h>
#include <wrapper/wrapper-plug.h>



G_DEFINE_INTERFACE (WrapperPlug, wrapper_plug, GTK_TYPE_WINDOW)



static void
wrapper_plug_default_init (WrapperPlugInterface *iface)
{
}



GtkWidget *
wrapper_plug_new (gulong socket_id,
                  GDBusProxy *proxy)
{
#ifdef HAVE_GTK_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return wrapper_plug_x11_new (socket_id);
#endif
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    return wrapper_plug_wayland_new (proxy);
#endif

  g_critical ("Running plugins as external is not supported on this windowing environment");

  return NULL;
}



void
wrapper_plug_set_background_color (WrapperPlug *plug,
                                   const gchar *color)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  WRAPPER_PLUG_GET_IFACE (plug)->set_background_color (plug, color);
}



void
wrapper_plug_set_background_image (WrapperPlug *plug,
                                   const gchar *image)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  WRAPPER_PLUG_GET_IFACE (plug)->set_background_image (plug, image);
}



void
wrapper_plug_set_monitor (WrapperPlug *plug,
                          gint monitor)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  WRAPPER_PLUG_GET_IFACE (plug)->set_monitor (plug, monitor);
}



void
wrapper_plug_set_geometry (WrapperPlug *plug,
                           const GdkRectangle *geometry)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  WRAPPER_PLUG_GET_IFACE (plug)->set_geometry (plug, geometry);
}



void
wrapper_plug_proxy_method_call (GDBusProxy *proxy,
                                const gchar *method,
                                GVariant *variant)
{
  GVariant *ret;
  GError *error = NULL;

  ret = g_dbus_proxy_call_sync (proxy, method, variant, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (ret != NULL)
    g_variant_unref (ret);
  else
    {
      g_warning ("%s call failed: %s", method, error->message);
      g_error_free (error);
    }
}



void
wrapper_plug_proxy_provider_signal (XfcePanelPluginProvider *provider,
                                    XfcePanelPluginProviderSignal provider_signal,
                                    GDBusProxy *proxy)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  wrapper_plug_proxy_method_call (proxy, "ProviderSignal", g_variant_new ("(u)", provider_signal));
}
