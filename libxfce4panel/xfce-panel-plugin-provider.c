/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>



enum
{
  PROVIDER_SIGNAL,
  LAST_SIGNAL
};



static void xfce_panel_plugin_provider_class_init (gpointer klass);



static guint provider_signals[LAST_SIGNAL];



GType
xfce_panel_plugin_provider_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE, I_("XfcePanelPluginProvider"),
                                            sizeof (XfcePanelPluginProviderIface),
                                            (GClassInitFunc) xfce_panel_plugin_provider_class_init,
                                            0, NULL, 0);
    }

  return type;
}



static void
xfce_panel_plugin_provider_class_init (gpointer klass)
{
  provider_signals[PROVIDER_SIGNAL] =
    g_signal_new (I_("provider-signal"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginProviderIface, provider_signal),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1, G_TYPE_UINT);
}



PANEL_SYMBOL_EXPORT const gchar *
xfce_panel_plugin_provider_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_name) (provider);
}



PANEL_SYMBOL_EXPORT gint
xfce_panel_plugin_provider_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_unique_id) (provider);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_set_size (XfcePanelPluginProvider *provider,
                                     gint                     size)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_size) (provider, size);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_set_orientation (XfcePanelPluginProvider *provider,
                                            GtkOrientation           orientation)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_orientation) (provider, orientation);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_set_screen_position (XfcePanelPluginProvider *provider,
                                                XfceScreenPosition       screen_position)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_screen_position) (provider, screen_position);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->save) (provider);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_emit_signal (XfcePanelPluginProvider       *provider,
                                        XfcePanelPluginProviderSignal  signal)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* emit the signal */
  g_signal_emit (G_OBJECT (provider), provider_signals[PROVIDER_SIGNAL], 0, signal);
}



PANEL_SYMBOL_EXPORT gboolean
xfce_panel_plugin_provider_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_configure) (provider);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_configure) (provider);
}



PANEL_SYMBOL_EXPORT gboolean
xfce_panel_plugin_provider_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_about) (provider);
}



PANEL_SYMBOL_EXPORT void
xfce_panel_plugin_provider_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_about) (provider);
}
