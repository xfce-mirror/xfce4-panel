/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-private.h>
#include <panel/panel-dbus-client.h>
#include <panel/panel-dbus-service.h>



static gboolean
panel_dbus_client_send_message (DBusMessage  *message,
                                GError      **error)
{
  DBusConnection *connection;
  DBusMessage    *result;
  DBusError       derror;

  /* initialize the dbus error */
  dbus_error_init (&derror);

  /* try to connect to the session bus */
  connection = dbus_bus_get (DBUS_BUS_SESSION, &derror);
  if (G_UNLIKELY (connection == NULL))
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);
      return FALSE;
    }

  /* send the message */
  result = dbus_connection_send_with_reply_and_block (connection, message, -1, &derror);

  /* check if no reply was received */
  if (result == NULL)
    {
      /* handle error */
      if (dbus_error_has_name (&derror, DBUS_ERROR_NAME_HAS_NO_OWNER))
        g_message (_("No running panel instance found."));
      else
        dbus_set_g_error (error, &derror);

      /* cleanup */
      dbus_error_free (&derror);

      return FALSE;
    }

  /* but maybe we received an error */
  if (G_UNLIKELY (dbus_message_get_type (result) == DBUS_MESSAGE_TYPE_ERROR))
    {
      dbus_set_error_from_message (&derror, result);
      dbus_set_g_error (error, &derror);
      dbus_message_unref (result);
      dbus_error_free (&derror);

      return FALSE;
    }

  /* it seems everything worked */
  dbus_message_unref (result);

  return TRUE;
}



static gboolean
panel_dbus_client_display_dialog (GdkScreen    *screen,
                                  const gchar  *methode_name,
                                  GError      **error)
{
  gchar       *display_name;
  DBusMessage *message;
  gboolean     result;

  /* fallback to default screen if no other is specified */
  if (G_LIKELY (screen == NULL))
    screen = gdk_screen_get_default ();

  /* determine the display name for the screen */
  display_name = gdk_screen_make_display_name (screen);

  /* generate the message */
  message = dbus_message_new_method_call (PANEL_DBUS_SERVICE_INTERFACE, PANEL_DBUS_SERVICE_PATH,
                                          PANEL_DBUS_SERVICE_INTERFACE, methode_name);
  dbus_message_set_auto_start (message, FALSE);
  dbus_message_append_args (message, DBUS_TYPE_STRING, &display_name, DBUS_TYPE_INVALID);

  /* send the message */
  result = panel_dbus_client_send_message (message, error);

  /* release the message */
  dbus_message_unref (message);

  /* cleanup */
  g_free (display_name);

  return result;
}



gboolean
panel_dbus_client_check_client_running (GError **error)
{
  DBusConnection *connection;
  DBusError       derror;
  gboolean        result;

  panel_return_val_if_fail (error == NULL || *error == NULL, TRUE);

  /* initialize the dbus error */
  dbus_error_init (&derror);

  /* try to connect to the session bus */
  connection = dbus_bus_get (DBUS_BUS_SESSION, &derror);
  if (G_UNLIKELY (connection == NULL))
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);

      return TRUE;
    }

  /* check if the name is already owned */
  result = dbus_bus_name_has_owner (connection, PANEL_DBUS_SERVICE_INTERFACE, &derror);

  /* handle the error is one is set */
  if (result == FALSE && dbus_error_is_set (&derror))
    {
      dbus_set_g_error (error, &derror);
      dbus_error_free (&derror);

      /* return on true on error */
      result = TRUE;
    }

  return result;
}



gboolean
panel_dbus_client_display_preferences_dialog (GdkScreen  *screen,
                                              GError    **error)
{
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return panel_dbus_client_display_dialog (screen, "DisplayPreferencesDialog", error);
}



gboolean
panel_dbus_client_display_items_dialog (GdkScreen  *screen,
                                        GError    **error)
{
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return panel_dbus_client_display_dialog (screen, "DisplayItemsDialog", error);
}



gboolean
panel_dbus_client_save (GError **error)
{
  DBusMessage *message;
  gboolean     result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* generate the message */
  message = dbus_message_new_method_call (PANEL_DBUS_SERVICE_INTERFACE, PANEL_DBUS_SERVICE_PATH,
                                          PANEL_DBUS_SERVICE_INTERFACE, "Save");
  dbus_message_set_auto_start (message, FALSE);

  /* send the message */
  result = panel_dbus_client_send_message (message, error);

  /* release the message */
  dbus_message_unref (message);

  return result;
}



gboolean
panel_dbus_client_add_new_item (const gchar  *plugin_name,
                                gchar       **arguments,
                                GError      **error)
{
  DBusMessage *message;
  gboolean     result;
  guint        length;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* arguments length */
  length = arguments ? g_strv_length (arguments) : 0;

  /* generate the message */
  message = dbus_message_new_method_call (PANEL_DBUS_SERVICE_INTERFACE, PANEL_DBUS_SERVICE_PATH,
                                          PANEL_DBUS_SERVICE_INTERFACE, "AddNewItem");
  dbus_message_set_auto_start (message, FALSE);
  dbus_message_append_args (message,
                            DBUS_TYPE_STRING, &plugin_name,
                            DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &arguments, length,
                            DBUS_TYPE_INVALID);

  /* send the message */
  result = panel_dbus_client_send_message (message, error);

  /* release the message */
  dbus_message_unref (message);

  return result;
}



gboolean
panel_dbus_client_terminate (gboolean   restart,
                             GError   **error)
{
  DBusMessage *message;
  gboolean     result;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* generate the message */
  message = dbus_message_new_method_call (PANEL_DBUS_SERVICE_INTERFACE, PANEL_DBUS_SERVICE_PATH,
                                          PANEL_DBUS_SERVICE_INTERFACE, "Terminate");
  dbus_message_set_auto_start (message, FALSE);
  dbus_message_append_args (message, DBUS_TYPE_BOOLEAN, &restart, DBUS_TYPE_INVALID);

  /* send the message */
  result = panel_dbus_client_send_message (message, error);

  /* release the message */
  dbus_message_unref (message);

  return result;
}
