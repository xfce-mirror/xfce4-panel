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

#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>



enum
{
  PROVIDER_SIGNAL,
  LAST_SIGNAL
};



static void xfce_panel_plugin_provider_base_init (gpointer klass);



static guint provider_signals[LAST_SIGNAL];



GType
xfce_panel_plugin_provider_get_type (void)
{
  static GType type = G_TYPE_INVALID;

  if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
      static const GTypeInfo info =
      {
        sizeof (XfcePanelPluginProviderIface),
        (GBaseInitFunc) xfce_panel_plugin_provider_base_init,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0,
        NULL,
        NULL
      };

      type = g_type_register_static (G_TYPE_INTERFACE, I_("XfcePanelPluginProvider"), &info, 0);
    }

  return type;
}



static void
xfce_panel_plugin_provider_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized))
    {
      provider_signals[PROVIDER_SIGNAL] =
        g_signal_new (I_("provider-signal"),
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__UINT,
                      G_TYPE_NONE, 1, G_TYPE_UINT);

      /* initialization finished */
      initialized = TRUE;
    }
}



const gchar *
xfce_panel_plugin_provider_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_name) (provider);
}



const gchar *
xfce_panel_plugin_provider_get_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return (*XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_id) (provider);
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
