/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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
#include "config.h"
#endif

#include "xfce-panel-macros.h"
#include "xfce-panel-plugin-provider.h"
#include "libxfce4panel-alias.h"

#include "common/panel-private.h"



enum
{
  PROVIDER_SIGNAL,
  LAST_SIGNAL
};

static guint provider_signals[LAST_SIGNAL];



G_DEFINE_INTERFACE (XfcePanelPluginProvider, xfce_panel_plugin_provider, G_TYPE_OBJECT)



static void
xfce_panel_plugin_provider_default_init (XfcePanelPluginProviderInterface *klass)
{
  provider_signals[PROVIDER_SIGNAL] = g_signal_new (g_intern_static_string ("provider-signal"),
                                                    G_TYPE_FROM_CLASS (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0, NULL, NULL,
                                                    g_cclosure_marshal_VOID__UINT,
                                                    G_TYPE_NONE, 1, G_TYPE_UINT);
}



const gchar *
xfce_panel_plugin_provider_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_name (provider);
}



gint
xfce_panel_plugin_provider_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_unique_id (provider);
}



void
xfce_panel_plugin_provider_set_size (XfcePanelPluginProvider *provider,
                                     gint size)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_size (provider, size);
}



void
xfce_panel_plugin_provider_set_icon_size (XfcePanelPluginProvider *provider,
                                          gint icon_size)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_icon_size (provider, icon_size);
}



void
xfce_panel_plugin_provider_set_dark_mode (XfcePanelPluginProvider *provider,
                                          gboolean dark_mode)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_dark_mode (provider, dark_mode);
}



void
xfce_panel_plugin_provider_set_mode (XfcePanelPluginProvider *provider,
                                     XfcePanelPluginMode mode)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_mode (provider, mode);
}



void
xfce_panel_plugin_provider_set_nrows (XfcePanelPluginProvider *provider,
                                      guint rows)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_nrows (provider, rows);
}



void
xfce_panel_plugin_provider_set_screen_position (XfcePanelPluginProvider *provider,
                                                XfceScreenPosition screen_position)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_screen_position (provider, screen_position);
}



void
xfce_panel_plugin_provider_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->save (provider);
}



void
xfce_panel_plugin_provider_emit_signal (XfcePanelPluginProvider *provider,
                                        XfcePanelPluginProviderSignal provider_signal)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* emit the signal only if the plugin is in a complete widget hierarchy (see
   * in particular panel_itembar_unref()) */
  if (GTK_IS_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider))))
    g_signal_emit (provider, provider_signals[PROVIDER_SIGNAL], 0, provider_signal);
}



gboolean
xfce_panel_plugin_provider_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_configure (provider);
}



void
xfce_panel_plugin_provider_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_configure (provider);
}



gboolean
xfce_panel_plugin_provider_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->get_show_about (provider);
}



void
xfce_panel_plugin_provider_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->show_about (provider);
}



void
xfce_panel_plugin_provider_removed (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->removed (provider);
}



gboolean
xfce_panel_plugin_provider_remote_event (XfcePanelPluginProvider *provider,
                                         const gchar *name,
                                         const GValue *value,
                                         guint *handle)
{
  const GValue *real_value = value;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), TRUE);
  panel_return_val_if_fail (name != NULL, TRUE);
  panel_return_val_if_fail (value == NULL || G_IS_VALUE (value), TRUE);

  if (XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->remote_event != NULL)
    {
      if (real_value != NULL
          && G_VALUE_HOLDS_UCHAR (real_value)
          && g_value_get_uchar (real_value) == '\0')
        real_value = NULL;

      return XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->remote_event (provider, name, real_value, handle);
    }

  return FALSE;
}



void
xfce_panel_plugin_provider_set_locked (XfcePanelPluginProvider *provider,
                                       gboolean locked)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->set_locked (provider, locked);
}



void
xfce_panel_plugin_provider_ask_remove (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE (provider)->ask_remove (provider);
}


#define __XFCE_PANEL_PLUGIN_PROVIDER_C__
#include "libxfce4panel-aliasdef.c"
