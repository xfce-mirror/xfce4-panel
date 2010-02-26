/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <gtk/gtk.h>
#include <common/panel-private.h>
#include <common/panel-dbus.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <wrapper/wrapper-plug.h>
#include <wrapper/wrapper-marshal.h>
#include <wrapper/wrapper-module.h>

#define DBUS_GLIB_CLIENT_WRAPPERS_org_xfce_Panel /* hack to exclude the panel client code */
#include <wrapper/wrapper-dbus-client-infos.h>



static gchar   *opt_display_name = NULL;
static gint     opt_unique_id = -1;
static gchar   *opt_comment = NULL;
static gchar   *opt_filename = NULL;
static gint     opt_socket_id = 0;
static gchar  **opt_arguments = NULL;
static GQuark   plug_quark = 0;



static GOptionEntry option_entries[] =
{
  { "name", 'n', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &wrapper_name, NULL, NULL },
  { "display-name", 'd', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_display_name, NULL, NULL },
  { "comment", 'c', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_comment, NULL, NULL },
  { "unique-id", 'i', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &opt_unique_id, NULL, NULL },
  { "filename", 'f', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_filename, NULL, NULL },
  { "socket-id", 's', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &opt_socket_id, NULL, NULL },
  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &opt_arguments, NULL, NULL },
  { NULL }
};



static void
dbus_gproxy_property_changed (DBusGProxy              *dbus_gproxy,
                              gint                     plugin_id,
                              DBusPropertyChanged      property,
                              const GValue            *value,
                              XfcePanelPluginProvider *provider)
{
  WrapperPlug *plug;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (value && G_TYPE_CHECK_VALUE (value));
  panel_return_if_fail (opt_unique_id == xfce_panel_plugin_provider_get_unique_id (provider));

  /* check if the signal is for this plugin */
  if (plugin_id != opt_unique_id)
    return;

  /* handle the changed property send by the panel to the wrapper */
  switch (property)
    {
      case PROPERTY_CHANGED_PROVIDER_SIZE:
        xfce_panel_plugin_provider_set_size (provider,
                                             g_value_get_int (value));
        break;

      case PROPERTY_CHANGED_PROVIDER_ORIENTATION:
        xfce_panel_plugin_provider_set_orientation (provider,
                                                    g_value_get_uint (value));
        break;

      case PROPERTY_CHANGED_PROVIDER_SCREEN_POSITION:
        xfce_panel_plugin_provider_set_screen_position (provider,
                                                        g_value_get_uint (value));
        break;

      case PROPERTY_CHANGED_PROVIDER_EMIT_SAVE:
        xfce_panel_plugin_provider_save (provider);
        break;

      case PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_CONFIGURE:
        xfce_panel_plugin_provider_show_configure (provider);
        break;

      case PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_ABOUT:
        xfce_panel_plugin_provider_show_about (provider);
        break;

      case PROPERTY_CHANGED_PROVIDER_REMOVE:
        xfce_panel_plugin_provider_remove (provider);
        break;

      case PROPERTY_CHANGED_WRAPPER_QUIT:
        gtk_main_quit ();
        break;

      case PROPERTY_CHANGED_WRAPPER_SET_SENSITIVE:
        gtk_widget_set_sensitive (GTK_WIDGET (provider),
                                  g_value_get_boolean (value));
        break;

      case PROPERTY_CHANGED_WRAPPER_BACKGROUND_ALPHA:
        plug = g_object_get_qdata (G_OBJECT (provider), plug_quark);
        wrapper_plug_set_background_alpha (plug, g_value_get_int (value) / 100.00);
        break;

      default:
        g_message ("External plugin \"%s-%d\" received unknown property \"%d\".",
                   wrapper_name, opt_unique_id, property);
        break;
    }
}



static void
dbus_gproxy_provider_signal (XfcePanelPluginProvider       *provider,
                             XfcePanelPluginProviderSignal  provider_signal,
                             DBusGProxy                    *dbus_gproxy)
{
  GError *error = NULL;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (opt_unique_id == xfce_panel_plugin_provider_get_unique_id (provider));

  /* send the provider signal to the panel */
  if (!wrapper_dbus_client_provider_signal (dbus_gproxy, opt_unique_id,
                                            provider_signal, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }
}



static DBusHandlerResult
dbus_gproxy_dbus_filter (DBusConnection *connection,
                         DBusMessage    *message,
                         gpointer        user_data)
{
  gchar *service, *old_owner, *new_owner;

  /* make sure this is a name-owner-changed signal */
  if (dbus_message_is_signal (message, DBUS_INTERFACE_DBUS, "NameOwnerChanged"))
    {
      /* get the information of the changed service */
      if (dbus_message_get_args (message, NULL,
                                 DBUS_TYPE_STRING, &service,
                                 DBUS_TYPE_STRING, &old_owner,
                                 DBUS_TYPE_STRING, &new_owner,
                                 DBUS_TYPE_INVALID))
        {
          /* check if the panel service lost the owner, if so, leave the mainloop */
          if (strcmp (service, "org.xfce.Panel") == 0 && !IS_STRING (new_owner))
            gtk_main_quit ();
        }
    }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}



gint
main (gint argc, gchar **argv)
{
  GOptionContext         *context;
  GOptionGroup           *option_group;
  GError                 *error = NULL;
#if defined(HAVE_SYS_PRCTL_H) && defined(PR_SET_NAME)
  gchar                   process_name[16];
#endif
  GModule                *library = NULL;
  gint                    retval = WRAPPER_EXIT_FAILURE;
  XfcePanelPluginPreInit  preinit_func;
  gboolean                result;
  DBusGConnection        *dbus_gconnection;
  DBusConnection         *dbus_connection;
  DBusGProxy             *dbus_gproxy = NULL;
  WrapperModule          *module = NULL;
  WrapperPlug            *plug;
  GtkWidget              *provider;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifndef NDEBUG
  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* parse the wrapper options */
  context = g_option_context_new ("[ARGUMENTS...]");
  g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
  option_group = gtk_get_option_group (FALSE);
  g_option_context_add_group (context, option_group);
  result = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (G_UNLIKELY (result == FALSE))
    goto leave;

  /* check if we have all required arguments */
  if (opt_socket_id == 0 || wrapper_name == NULL
      || opt_unique_id == -1 || opt_display_name == NULL
      || opt_filename == NULL)
    {
      g_set_error (&error, 0, 0, "One of the required arguments is missing");
      goto leave;
    }

#if defined(HAVE_SYS_PRCTL_H) && defined(PR_SET_NAME)
  /* change the process name to something that makes sence */
  g_snprintf (process_name, sizeof (process_name), "panel-%d-%s",
              opt_unique_id, wrapper_name);
  if (prctl (PR_SET_NAME, (gulong) process_name, 0, 0, 0) == -1)
    g_warning ("Failed to change the process name to \"%s\".", process_name);
#endif

  /* open the plugin module */
  library = g_module_open (opt_filename, G_MODULE_BIND_LOCAL);
  if (G_UNLIKELY (library == NULL))
    {
      g_set_error (&error, 0, 0, "Failed to open plugin module \"%s\": %s",
                   opt_filename, g_module_error ());
      goto leave;
    }

  /* check for a plugin preinit function */
  if (g_module_symbol (library, "xfce_panel_module_preinit", (gpointer) &preinit_func)
      && preinit_func != NULL
      && (*preinit_func) (argc, argv) == FALSE)
    {
      retval = WRAPPER_EXIT_PREINIT;
      goto leave;
    }

  /* initialize gtk */
  gtk_init (&argc, &argv);

  /* try to connect to dbus */
  dbus_gconnection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (G_UNLIKELY (dbus_gconnection == NULL))
    goto leave;

  /* get the dbus connection from the gconnection */
  dbus_connection = dbus_g_connection_get_connection (dbus_gconnection);

  /* hookup a filter to monitor panel segfaults */
  if (dbus_connection_add_filter (dbus_connection, dbus_gproxy_dbus_filter, NULL, NULL))
    dbus_bus_add_match (dbus_connection,
                        "type='signal',sender='" DBUS_SERVICE_DBUS
                        "',path='" DBUS_PATH_DBUS
                        "',interface='" DBUS_INTERFACE_DBUS
                        "',member='NameOwnerChanged'", NULL);

  /* get the dbus proxy for the plugin interface */
  dbus_gproxy = dbus_g_proxy_new_for_name (dbus_gconnection,
                                           PANEL_DBUS_PLUGIN_INTERFACE,
                                           PANEL_DBUS_PATH,
                                           PANEL_DBUS_PLUGIN_INTERFACE);
  if (G_UNLIKELY (dbus_gproxy == NULL))
    {
      /* set error and leave */
      g_set_error (&error, 0, 0,
                   "Failed to create the dbus proxy for interface \"%s\"",
                   PANEL_DBUS_PLUGIN_INTERFACE);
      goto leave;
    }

  /* setup signal for property changes */
  dbus_g_object_register_marshaller (wrapper_marshal_VOID__INT_INT_BOXED,
                                     G_TYPE_NONE, G_TYPE_INT, G_TYPE_INT,
                                     G_TYPE_VALUE, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (dbus_gproxy, "PropertyChanged", G_TYPE_INT,
                           G_TYPE_INT, G_TYPE_VALUE, G_TYPE_INVALID);

  /* create the type module */
  module = wrapper_module_new (library);

  /* create the plugin provider */
  provider = wrapper_module_new_provider (module,
                                          gdk_screen_get_default (),
                                          wrapper_name, opt_unique_id,
                                          opt_display_name, opt_comment,
                                          opt_arguments);

  if (G_LIKELY (provider != NULL))
    {
      /* create the wrapper plug */
      plug = wrapper_plug_new (opt_socket_id);
      gtk_container_add (GTK_CONTAINER (plug), GTK_WIDGET (provider));
      gtk_widget_show (GTK_WIDGET (plug));

      /* set plug data to provider */
      plug_quark = g_quark_from_static_string ("plug-quark");
      g_object_set_qdata (G_OBJECT (provider), plug_quark, plug);

      /* monitor provider signals */
      g_signal_connect (G_OBJECT (provider), "provider-signal",
          G_CALLBACK (dbus_gproxy_provider_signal), dbus_gproxy);

      /* connect dbus signal to set provider properties send from the panel */
      dbus_g_proxy_connect_signal (dbus_gproxy, "PropertyChanged",
          G_CALLBACK (dbus_gproxy_property_changed), g_object_ref (provider),
          (GClosureNotify) g_object_unref);

      /* show the plugin */
      gtk_widget_show (GTK_WIDGET (provider));

      /* everything when fine */
      retval = WRAPPER_EXIT_SUCCESS;

      /* enter the main loop */
      gtk_main ();

      /* disconnect signal */
      dbus_g_proxy_disconnect_signal (dbus_gproxy, "PropertyChanged",
          G_CALLBACK (dbus_gproxy_property_changed), provider);

      /* destroy the plug and provider */
      gtk_widget_destroy (GTK_WIDGET (plug));
    }
  else
    {
      retval = WRAPPER_EXIT_NO_PROVIDER;
    }

leave:
  /* release the proxy */
  if (G_LIKELY (dbus_gproxy != NULL))
    g_object_unref (G_OBJECT (dbus_gproxy));

  /* close the type module */
  if (G_LIKELY (module != NULL))
    g_object_unref (G_OBJECT (module));

  /* close plugin module */
  if (G_LIKELY (library != NULL))
    g_module_close (library);

  if (G_UNLIKELY (error != NULL))
    {
      /* print the critical error */
      g_critical ("Wrapper %s-%d: %s.", wrapper_name,
                  opt_unique_id, error->message);

      /* cleanup */
      g_error_free (error);
    }

  return retval;
}
