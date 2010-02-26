/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <panel/panel-dbus.h>
#include <panel/panel-application.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>



#define PANEL_DBUS_PATH      "/org/xfce/Panel"
#define PANEL_DBUS_INTERFACE "org.xfce.Panel"



static void      panel_dbus_service_class_init    (PanelDBusServiceClass  *klass);
static void      panel_dbus_service_init          (PanelDBusService       *service);
static void      panel_dbus_service_finalize      (GObject                *object);
static gboolean  panel_dbus_service_signal        (PanelDBusService       *service,
                                                   guint32                 signal_id,
                                                   GError                **error);

struct _PanelDBusServiceClass
{
  GObjectClass __parent__;
};

struct _PanelDBusService
{
  GObject __parent__;

  /* the dbus connection */
  DBusGConnection *connection;
};



/* shared boolean with main.c */
gboolean dbus_quit_with_restart = FALSE;



G_DEFINE_TYPE (PanelDBusService, panel_dbus_service, G_TYPE_OBJECT);



static void
panel_dbus_service_class_init (PanelDBusServiceClass *klass)
{
  extern const DBusGObjectInfo  dbus_glib_panel_dbus_service_object_info;
  GObjectClass                 *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_dbus_service_finalize;

  /* install the D-BUS info for our class */
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), &dbus_glib_panel_dbus_service_object_info);
}



static void
panel_dbus_service_init (PanelDBusService *service)
{
  GError *error = NULL;

  /* try to connect to the session bus */
  service->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (G_LIKELY (service->connection != NULL))
    {
      /* register the /org/xfce/Panel object */
      dbus_g_connection_register_g_object (service->connection, PANEL_DBUS_PATH, G_OBJECT (service));

      /* request the org.xfce.Mousepad name for Mousepad */
      dbus_bus_request_name (dbus_g_connection_get_connection (service->connection),
                             PANEL_DBUS_INTERFACE, DBUS_NAME_FLAG_REPLACE_EXISTING, NULL);
    }
  else
    {
      /* print warning */
      g_warning ("Failed to connect to the D-BUS session bus: %s", error->message);

      /* cleanup */
      g_error_free (error);
    }
}



static void
panel_dbus_service_finalize (GObject *object)
{
  PanelDBusService *service = PANEL_DBUS_SERVICE (object);

  /* release the connection */
  if (G_LIKELY (service->connection != NULL))
    dbus_g_connection_unref (service->connection);

  (*G_OBJECT_CLASS (panel_dbus_service_parent_class)->finalize) (object);
}



static gboolean
panel_dbus_service_signal (PanelDBusService  *service,
                           guint32            signal_id,
                           GError           **error)
{
  PanelApplication *application;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (signal_id == SIGNAL_CUSTOMIZE)
    {
      /* show the prefernces dialog */
      panel_preferences_dialog_show (NULL);
    }
  else if (signal_id == SIGNAL_ADD)
    {
      /* show the items dialog */
      panel_item_dialog_show ();
    }
  else if (signal_id == SIGNAL_SAVE)
    {
      /* save the configuration */
      application = panel_application_get ();
      panel_application_save (application);
      g_object_unref (G_OBJECT (application));
    }
  else if (signal_id == SIGNAL_RESTART || signal_id == SIGNAL_QUIT)
    {
      /* set restart boolean */
      dbus_quit_with_restart = !!(signal_id == SIGNAL_RESTART);

      /* quit */
      gtk_main_quit ();
    }

  return TRUE;
}



GObject *
panel_dbus_service_new (void)
{
  return g_object_new (PANEL_TYPE_DBUS_SERVICE, NULL);
}



static gboolean
panel_dbus_client_send (DBusMessage  *message,
                        GError      **error)
{
  DBusConnection *connection;
  DBusMessage    *result;
  DBusError       derror;

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



gboolean
panel_dbus_client_send_signal (guint32   signal_id,
                               GError  **error)
{
  DBusMessage *message;

  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* generate the message */
  message = dbus_message_new_method_call (PANEL_DBUS_INTERFACE, PANEL_DBUS_PATH, PANEL_DBUS_INTERFACE, "Signal");
  dbus_message_set_auto_start (message, FALSE);
  dbus_message_append_args (message, DBUS_TYPE_UINT32, &signal_id, DBUS_TYPE_INVALID);

  /* send the message */
  panel_dbus_client_send (message, error);

  /* unref the message */
  dbus_message_unref (message);

  /* we return false if an error was set */
  return (error != NULL);
}



/* include the dbus glue generated by dbus-binding-tool */
#include <panel/panel-dbus-infos.h>

