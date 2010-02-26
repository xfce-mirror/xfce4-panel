/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#include <dbus/dbus-glib.h>
#include <libxfce4util/libxfce4util.h>
#include <common/panel-private.h>
#include <common/panel-dbus.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-dbus-client.h>
#include <panel/panel-dbus-service.h>

#include <panel/panel-dbus-client-infos.h>



static DBusGProxy *
panel_dbus_client_get_proxy (GError **error)
{
  DBusGConnection *dbus_connection;
  DBusGProxy      *dbus_proxy;

  /* try to open the dbus connection */
  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, error);
  if (G_UNLIKELY (dbus_connection == NULL))
    return NULL;

  /* get the proxy */
  dbus_proxy = dbus_g_proxy_new_for_name_owner (dbus_connection,
                                                PANEL_DBUS_NAME,
                                                PANEL_DBUS_PATH,
                                                PANEL_DBUS_INTERFACE,
                                                error);

  return dbus_proxy;
}



gboolean
panel_dbus_client_check_instance_running (void)
{
  DBusGProxy *dbus_proxy;

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (NULL);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* cleanup */
  g_object_unref (G_OBJECT (dbus_proxy));

  return TRUE;
}



gboolean
panel_dbus_client_display_preferences_dialog (guint    active,
                                              GError **error)
{
  gboolean    result;
  DBusGProxy *dbus_proxy;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* call */
  result = _panel_dbus_client_display_preferences_dialog (dbus_proxy,
                                                          active, error);

  /* cleanup */
  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



gboolean
panel_dbus_client_display_items_dialog (guint    active,
                                        GError **error)
{
  gboolean    result;
  DBusGProxy *dbus_proxy;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* call */
  result = _panel_dbus_client_display_items_dialog (dbus_proxy, active,
                                                    error);

  /* cleanup */
  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



gboolean
panel_dbus_client_save (GError **error)
{
  DBusGProxy *dbus_proxy;
  gboolean    result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* call */
  result = _panel_dbus_client_save (dbus_proxy, error);
  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



gboolean
panel_dbus_client_add_new_item (const gchar  *plugin_name,
                                gchar       **arguments,
                                GError      **error)
{
  DBusGProxy *dbus_proxy;
  gboolean    result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* call */
  result = _panel_dbus_client_add_new_item (dbus_proxy, plugin_name,
                                            (const gchar **) arguments,
                                            error);
  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



gboolean
panel_dbus_client_terminate (gboolean   restart,
                             GError   **error)
{
  DBusGProxy *dbus_proxy;
  gboolean    result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* get the proxy */
  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  /* call */
  result = _panel_dbus_client_terminate (dbus_proxy, restart, error);
  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}
