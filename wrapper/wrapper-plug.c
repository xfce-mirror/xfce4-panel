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

#include "wrapper-plug.h"

#ifdef ENABLE_X11
#include "wrapper-plug-x11.h"
#endif

#ifdef HAVE_GTK_LAYER_SHELL
#include "wrapper-plug-wayland.h"

#include <gtk-layer-shell.h>
#endif

#include "common/panel-private.h"



G_DEFINE_INTERFACE (WrapperPlug, wrapper_plug, GTK_TYPE_WINDOW)



static void
wrapper_plug_default_init (WrapperPlugInterface *iface)
{
}



GtkWidget *
wrapper_plug_new (gulong socket_id,
                  gint unique_id,
                  GDBusProxy *proxy,
                  GError **error)
{
  panel_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  panel_return_val_if_fail (error != NULL && *error == NULL, NULL);

#ifdef ENABLE_X11
  if (WINDOWING_IS_X11 ())
    return wrapper_plug_x11_new (socket_id, proxy);
#endif
#ifdef HAVE_GTK_LAYER_SHELL
  if (gtk_layer_is_supported ())
    return wrapper_plug_wayland_new (unique_id, proxy, error);
#endif

  g_set_error (error, 0, 0, "Running plugins as external is not supported on this windowing environment");

  return NULL;
}



void
wrapper_plug_proxy_provider_signal (WrapperPlug *plug,
                                    XfcePanelPluginProviderSignal provider_signal,
                                    XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  WRAPPER_PLUG_GET_IFACE (plug)->proxy_provider_signal (plug, provider_signal, provider);
}



void
wrapper_plug_proxy_remote_event_result (WrapperPlug *plug,
                                        guint handle,
                                        gboolean result)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  WRAPPER_PLUG_GET_IFACE (plug)->proxy_remote_event_result (plug, handle, result);
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
wrapper_plug_proxy_method_call_sync (GDBusProxy *proxy,
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



static void
wrapper_plug_proxy_method_call_finish (GObject *source_object,
                                       GAsyncResult *res,
                                       gpointer data)
{
  GVariant *ret;
  GError *error = NULL;
  gchar *method = data;

  ret = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (ret != NULL)
    g_variant_unref (ret);
  else
    {
      g_warning ("%s call failed: %s", method, error->message);
      g_error_free (error);
    }

  g_free (method);
}



void
wrapper_plug_proxy_method_call (GDBusProxy *proxy,
                                const gchar *method,
                                GVariant *variant)
{
  g_dbus_proxy_call (proxy, method, variant, G_DBUS_CALL_FLAGS_NONE, -1, NULL,
                     wrapper_plug_proxy_method_call_finish, g_strdup (method));
}
