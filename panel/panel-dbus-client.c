/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <exo/exo.h>
#include <dbus/dbus-glib.h>
#include <libxfce4util/libxfce4util.h>
#include <common/panel-private.h>
#include <common/panel-dbus.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-dbus-client.h>
#include <panel/panel-dbus-service.h>

#include <panel/panel-dbus-client-infos.h>



enum
{
  PLUGIN_NAME,
  NAME,
  TYPE,
  VALUE,
  N_TOKENS
};



static DBusGProxy *
panel_dbus_client_get_proxy (GError **error)
{
  DBusGConnection *dbus_connection;

  /* return null if no connection is found */
  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, error);
  if (G_UNLIKELY (dbus_connection == NULL))
    return NULL;

  return dbus_g_proxy_new_for_name_owner (dbus_connection,
                                          PANEL_DBUS_NAME,
                                          PANEL_DBUS_PATH,
                                          PANEL_DBUS_INTERFACE,
                                          error);
}



gboolean
panel_dbus_client_display_preferences_dialog (guint         active,
                                              const gchar  *socket_id,
                                              GError      **error)
{
  gboolean    result;
  DBusGProxy *dbus_proxy;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_LIKELY (dbus_proxy == NULL))
    return FALSE;

  result = _panel_dbus_client_display_preferences_dialog (dbus_proxy,
                                                          active, socket_id,
                                                          error);

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

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  result = _panel_dbus_client_display_items_dialog (dbus_proxy, active,
                                                    error);

  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



gboolean
panel_dbus_client_save (GError **error)
{
  DBusGProxy *dbus_proxy;
  gboolean    result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

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

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  result = _panel_dbus_client_add_new_item (dbus_proxy, plugin_name,
                                            (const gchar **) arguments,
                                            error);

  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}



static GType
panel_dbus_client_gtype_from_string (const gchar *str)
{
  if (strcmp (str, "bool") == 0)
    return G_TYPE_BOOLEAN;
  else if (strcmp (str, "double") == 0)
    return G_TYPE_DOUBLE;
  else if (strcmp (str, "int") == 0)
    return G_TYPE_INT;
  else if (strcmp (str, "string") == 0)
    return G_TYPE_STRING;
  else if (strcmp (str, "uint") == 0)
    return G_TYPE_UINT;
  else
    return G_TYPE_NONE;
}



gboolean
panel_dbus_client_plugin_event (const gchar  *plugin_event,
                                gboolean     *return_succeed,
                                GError      **error)
{
  DBusGProxy  *dbus_proxy;
  gboolean     result = FALSE;
  gchar      **tokens;
  GType        type;
  GValue       value = { 0, };
  guint        n_tokens;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  tokens = g_strsplit (plugin_event, ":", -1);
  n_tokens = g_strv_length (tokens);

  if (!(n_tokens == 2 || n_tokens == N_TOKENS)
      || exo_str_is_empty (tokens[PLUGIN_NAME])
      || exo_str_is_empty (tokens[NAME]))
    {
      g_set_error_literal (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                           _("Invalid plugin event syntax specified. "
                             "Use PLUGIN-NAME:NAME[:TYPE:VALUE]."));
      goto out;
    }
  else if (n_tokens == 2)
    {
      /* set noop value, recognized by the dbus service as %NULL value */
      g_value_init (&value, G_TYPE_UCHAR);
      g_value_set_uchar (&value, '\0');
    }
  else if (n_tokens == N_TOKENS)
    {
      type = panel_dbus_client_gtype_from_string (tokens[TYPE]);
      if (G_LIKELY (type != G_TYPE_NONE))
        {
          g_value_init (&value, type);

          if (type == G_TYPE_BOOLEAN)
            g_value_set_boolean (&value, strcmp (tokens[VALUE], "true") == 0);
          else if (type == G_TYPE_DOUBLE)
            g_value_set_double (&value, g_ascii_strtod (tokens[VALUE], NULL));
          else if (type == G_TYPE_INT)
            g_value_set_int (&value, strtol (tokens[VALUE], NULL, 0));
          else if (type == G_TYPE_STRING)
            g_value_set_static_string (&value, tokens[VALUE]);
          else if (type == G_TYPE_UINT)
            g_value_set_uint (&value, strtol (tokens[VALUE], NULL, 0));
          else
            panel_assert_not_reached ();
        }
      else
        {
          g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                       _("Invalid hint type \"%s\". Valid types "
                         "are bool, double, int, string and uint."),
                       tokens[TYPE]);
          goto out;
        }
    }
  else
    {
      panel_assert_not_reached ();
      goto out;
    }

  /* send value over dbus */
  panel_return_val_if_fail (G_IS_VALUE (&value), FALSE);
  result = _panel_dbus_client_plugin_event (dbus_proxy,
                                            tokens[PLUGIN_NAME],
                                            tokens[NAME],
                                            &value,
                                            return_succeed,
                                            error);
  g_value_unset (&value);

out:
  g_strfreev (tokens);
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

  dbus_proxy = panel_dbus_client_get_proxy (error);
  if (G_UNLIKELY (dbus_proxy == NULL))
    return FALSE;

  result = _panel_dbus_client_terminate (dbus_proxy, restart, error);

  g_object_unref (G_OBJECT (dbus_proxy));

  return result;
}
