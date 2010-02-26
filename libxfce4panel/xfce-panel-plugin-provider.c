/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#include <common/panel-private.h>

#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>
#include <libxfce4panel/libxfce4panel-alias.h>


enum
{
  PROVIDER_SIGNAL,
  LAST_SIGNAL
};



static void xfce_panel_plugin_provider_class_init (gpointer klass,
                                                   gpointer klass_data);



static guint provider_signals[LAST_SIGNAL];



GType
xfce_panel_plugin_provider_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    {
      type = g_type_register_static_simple (G_TYPE_INTERFACE,
                                            g_intern_static_string ("XfcePanelPluginProvider"),
                                            sizeof (XfcePanelPluginProviderIface),
                                            xfce_panel_plugin_provider_class_init,
                                            0, NULL, 0);
    }

  return type;
}



static void
xfce_panel_plugin_provider_class_init (gpointer klass,
                                       gpointer klass_data)
{
  provider_signals[PROVIDER_SIGNAL] =
    g_signal_new (g_intern_static_string ("provider-signal"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginProviderIface, provider_signal),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1, G_TYPE_UINT);
}



const gchar *
xfce_panel_plugin_provider_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_name) (provider);
}



gint
xfce_panel_plugin_provider_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_unique_id) (provider);
}



void
xfce_panel_plugin_provider_set_size (XfcePanelPluginProvider *provider,
                                     gint                     size)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_size) (provider, size);
}



void
xfce_panel_plugin_provider_set_orientation (XfcePanelPluginProvider *provider,
                                            GtkOrientation           orientation)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_orientation) (provider, orientation);
}



void
xfce_panel_plugin_provider_set_screen_position (XfcePanelPluginProvider *provider,
                                                XfceScreenPosition       screen_position)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_screen_position) (provider, screen_position);
}



void
xfce_panel_plugin_provider_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->save) (provider);
}



void
xfce_panel_plugin_provider_emit_signal (XfcePanelPluginProvider       *provider,
                                        XfcePanelPluginProviderSignal  provider_signal)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* emit the signal */
  g_signal_emit (G_OBJECT (provider), provider_signals[PROVIDER_SIGNAL], 0, provider_signal);
}



gboolean
xfce_panel_plugin_provider_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_configure) (provider);
}



void
xfce_panel_plugin_provider_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_configure) (provider);
}



gboolean
xfce_panel_plugin_provider_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_about) (provider);
}



void
xfce_panel_plugin_provider_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_about) (provider);
}



void
xfce_panel_plugin_provider_remove (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->remove) (provider);
}



#define __XFCE_PANEL_PLUGIN_PROVIDER_C__
#include <libxfce4panel/libxfce4panel-aliasdef.c>
