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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-private.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-application.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-marshal.h>



static void      panel_dbus_service_class_init    (PanelDBusServiceClass  *klass);
static void      panel_dbus_service_init          (PanelDBusService       *service);
static void      panel_dbus_service_finalize      (GObject                *object);



enum
{
  PROPERTY_CHANGED,
  LAST_SIGNAL
};

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

static guint dbus_service_signals[LAST_SIGNAL];



G_DEFINE_TYPE (PanelDBusService, panel_dbus_service, G_TYPE_OBJECT);



static void
panel_dbus_service_class_init (PanelDBusServiceClass *klass)
{
  extern const DBusGObjectInfo  dbus_glib_panel_dbus_service_object_info;
  GObjectClass                 *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_dbus_service_finalize;

  /**
   * Emited when a plugin property changes
   **/
  /* TODO implement this */
  dbus_service_signals[PROPERTY_CHANGED] =
    g_signal_new (I_("property-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  panel_marshal_VOID__STRING_STRING_BOXED,
                  G_TYPE_NONE, 3,
                  G_TYPE_STRING,
                  G_TYPE_STRING,
                  G_TYPE_VALUE);

  /* install the d-bus info for our class */
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), &dbus_glib_panel_dbus_service_object_info);
}


/* TODO wh can do better with error handling below */
static void
panel_dbus_service_init (PanelDBusService *service)
{
  GError *error = NULL;

  /* try to connect to the session bus */
  service->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (G_LIKELY (service->connection != NULL))
    {
      /* request the org.xfce.Panel name */
      dbus_bus_request_name (dbus_g_connection_get_connection (service->connection),
                             PANEL_DBUS_SERVICE_INTERFACE, 0, NULL);

      /* register the /org/xfce/Panel object */
      dbus_g_connection_register_g_object (service->connection, PANEL_DBUS_SERVICE_PATH, G_OBJECT (service));
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
panel_dbus_service_display_preferences_dialog (PanelDBusService  *service,
                                               guint              active,
                                               GError           **error)
{
  PanelApplication *application;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* show the preferences dialog */
  application = panel_application_get ();
  panel_preferences_dialog_show (panel_application_get_window (application, active));
  g_object_unref (G_OBJECT (application));

  return TRUE;
}



static gboolean
panel_dbus_service_display_items_dialog (PanelDBusService  *service,
                                         guint              active,
                                         GError           **error)
{
  PanelApplication *application;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* show the items dialog */
  application = panel_application_get ();
  panel_item_dialog_show (panel_application_get_window (application, active));
  g_object_unref (G_OBJECT (application));

  return TRUE;
}



static gboolean
panel_dbus_service_save (PanelDBusService  *service,
                         GError           **error)
{
  PanelApplication *application;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* save the configuration */
  application = panel_application_get ();
  panel_application_save (application, TRUE);
  g_object_unref (G_OBJECT (application));

  return TRUE;
}



static gboolean
panel_dbus_service_add_new_item (PanelDBusService  *service,
                                 const gchar       *plugin_name,
                                 gchar            **arguments,
                                 GError           **error)
{
  PanelApplication *application;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  panel_return_val_if_fail (plugin_name != NULL, FALSE);

  application = panel_application_get ();

  /* save the configuration */
  if (arguments && *arguments != NULL)
    panel_application_add_new_item (application, plugin_name, arguments);
  else
    panel_application_add_new_item (application, plugin_name, NULL);

  g_object_unref (G_OBJECT (application));

  return TRUE;
}



static gboolean
panel_dbus_service_terminate (PanelDBusService  *service,
                              gboolean           restart,
                              GError           **error)
{
  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* set restart boolean */
  dbus_quit_with_restart = restart;

  /* quit */
  gtk_main_quit ();

  return TRUE;
}



static gboolean
panel_dbus_service_get_property (PanelDBusService *service,
                                 const gchar      *plugin_id,
                                 const gchar      *property,
                                 GValue           *value,
                                 GError           *error)
{
  PanelModuleFactory      *factory;
  PanelApplication        *application;
  XfcePanelPluginProvider *provider;
  GtkWidget               *window;
  gboolean                 succeed = FALSE;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (plugin_id != NULL, FALSE);
  panel_return_val_if_fail (property != NULL, FALSE);

  /* get the module factory */
  factory = panel_module_factory_get ();

  /* get the plugin */
  provider = panel_module_factory_get_plugin (factory, plugin_id);
  if (G_LIKELY (provider))
    {
      if (strcmp ("PanelNumber", property) == 0)
        {
          /* get the plugin's panel window */
          window = gtk_widget_get_toplevel (GTK_WIDGET (provider));

          /* initialize the value */
          g_value_init (value, G_TYPE_UINT);

          /* get the panel number from the application */
          application = panel_application_get ();
          g_value_set_uint (value, panel_application_get_window_index (application, PANEL_WINDOW (window)));
          g_object_unref (G_OBJECT (application));

          succeed = TRUE;
        }
    }

  /* release the factory */
  g_object_unref (G_OBJECT (factory));

  /* check if we return a good value */
  panel_return_val_if_fail (!succeed || G_TYPE_CHECK_VALUE (value), FALSE);

  return succeed;
}



static gboolean
panel_dbus_service_set_property (PanelDBusService *service,
                                 const gchar      *plugin_id,
                                 const gchar      *property,
                                 const GValue     *value,
                                 GError           *error)
{
  PanelModuleFactory      *factory;
  XfcePanelPluginProvider *provider;

  panel_return_val_if_fail (PANEL_IS_DBUS_SERVICE (service), FALSE);
  panel_return_val_if_fail (plugin_id != NULL, FALSE);
  panel_return_val_if_fail (property != NULL, FALSE);
  panel_return_val_if_fail (value && G_TYPE_CHECK_VALUE (value), FALSE);

  /* check if this is a plugin property change */
  if (strcmp (property, "ProviderSignal") == 0)
    {
      /* get the module factory */
      factory = panel_module_factory_get ();

      /* get the plugin from the factory */
      provider = panel_module_factory_get_plugin (factory, plugin_id);

      /* emit the signal for the local plugin provider */
      if (G_LIKELY (provider))
        xfce_panel_plugin_provider_send_signal (XFCE_PANEL_PLUGIN_PROVIDER (provider), g_value_get_uint (value));

      /* release the factory */
      g_object_unref (G_OBJECT (factory));
    }

  return TRUE;
}



PanelDBusService *
panel_dbus_service_get (void)
{
  static PanelDBusService *service = NULL;

  if (G_LIKELY (service))
    {
      g_object_ref (G_OBJECT (service));
    }
  else
    {
      service = g_object_new (PANEL_TYPE_DBUS_SERVICE, NULL);
      g_object_add_weak_pointer (G_OBJECT (service), (gpointer) &service);
    }

  return service;
}



void
panel_dbus_service_set_plugin_property (const gchar  *plugin_id,
                                        const gchar  *property,
                                        const GValue *value)
{
  PanelDBusService *service;

  panel_return_if_fail (plugin_id && *plugin_id != '\0');
  panel_return_if_fail (property && *property != '\0');
  panel_return_if_fail (value && G_TYPE_CHECK_VALUE (value));

  /* get the dbus service */
  service = panel_dbus_service_get ();

  /* emit the signal */
  g_signal_emit (G_OBJECT (service), dbus_service_signals[PROPERTY_CHANGED],
                 0, plugin_id, property, value);

  /* release */
  g_object_unref (G_OBJECT (service));
}



/* include the dbus glue generated by dbus-binding-tool */
#include <panel/panel-dbus-service-infos.h>
