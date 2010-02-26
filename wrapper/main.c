/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <wrapper/wrapper-plug.h>
#include <wrapper/wrapper-marshal.h>
#include <wrapper/wrapper-dbus-client-infos.h>



gchar          *opt_name = NULL;
static gchar   *opt_display_name = NULL;
static gchar   *opt_id = NULL;
static gchar   *opt_filename = NULL;
static gint     opt_socket_id = 0;
static gchar  **opt_arguments = NULL;
static GQuark   plug_quark = 0;



static GOptionEntry option_entries[] =
{
  { "name", 'n', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_name, NULL, NULL },
  { "display-name", 'd', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_display_name, NULL, NULL },
  { "id", 'i', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_id, NULL, NULL },
  { "filename", 'f', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_filename, NULL, NULL },
  { "socket-id", 's', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &opt_socket_id, NULL, NULL },
  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &opt_arguments, NULL, NULL },
  { NULL }
};



static void
dbus_proxy_provider_property_changed (DBusGProxy              *dbus_proxy,
                                      const gchar             *plugin_id,
                                      const gchar             *property,
                                      const GValue            *value,
                                      XfcePanelPluginProvider *provider)
{
  WrapperPlug *plug;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (value && G_TYPE_CHECK_VALUE (value));

  /* check if the signal is for this panel */
  if (!plugin_id || strcmp (plugin_id, xfce_panel_plugin_provider_get_id (provider)) != 0)
    return;

  /* handle the property */
  if (G_UNLIKELY (property == NULL || *property == '\0'))
    g_message ("External plugin '%s' received null property", plugin_id);
  else if (strcmp (property, "Size") == 0)
    xfce_panel_plugin_provider_set_size (provider, g_value_get_int (value));
  else if (strcmp (property, "Orientation") == 0)
    xfce_panel_plugin_provider_set_orientation (provider, g_value_get_uint (value));
  else if (strcmp (property, "ScreenPosition") == 0)
    xfce_panel_plugin_provider_set_screen_position (provider, g_value_get_uint (value));
  else if (strcmp (property, "Save") == 0)
    xfce_panel_plugin_provider_save (provider);
  else if (strcmp (property, "Quit") == 0)
    gtk_main_quit ();
  else if (strcmp (property, "Sensitive") == 0)
    gtk_widget_set_sensitive (GTK_WIDGET (provider), g_value_get_boolean (value));
  else
    {
      /* get the plug */
      plug = g_object_get_qdata (G_OBJECT (provider), plug_quark);

      if (strcmp (property, "BackgroundAlpha") == 0)
        wrapper_plug_set_background_alpha (plug, g_value_get_int (value) / 100.00);
      else if (strcmp (property, "ActivePanel") == 0)
        wrapper_plug_set_selected (plug, g_value_get_boolean (value));
      else
        g_message ("External plugin '%s' received unknown property '%s'", plugin_id, property);
    }
}



static void
dbus_proxy_provider_expand_changed (XfcePanelPluginProvider *provider,
                                    gboolean                 expand,
                                    DBusGProxy              *dbus_proxy)
{
  GValue  value = { 0, };
  GError *error = NULL;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* init the value */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, expand);

  /* call */
  if (!wrapper_dbus_client_set_property (dbus_proxy, xfce_panel_plugin_provider_get_id (provider),
                                         "Expand", &value, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }

  /* unset */
  g_value_unset (&value);
}



static void
dbus_proxy_provider_move_item (XfcePanelPluginProvider *provider,
                               DBusGProxy              *dbus_proxy)
{
  GError *error = NULL;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* call */
  if (!wrapper_dbus_client_set_property (dbus_proxy, xfce_panel_plugin_provider_get_id (provider),
                                         "MoveItem", NULL, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }
}



static void
dbus_proxy_provider_add_new_items (XfcePanelPluginProvider *provider,
                                   DBusGProxy              *dbus_proxy)
{
  gchar  *name;
  GError *error = NULL;
  guint   active_panel = 0;
  GValue  value = { 0, };
  
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* try to get the panel number of this plugin */
  if (wrapper_dbus_client_get_property (dbus_proxy, xfce_panel_plugin_provider_get_id (provider),
                                        "PanelNumber", &value, NULL))
    {
      /* set the panel number */
      active_panel = g_value_get_uint (&value);

      /* unset */
      g_value_unset (&value);
    }

  /* create a screen name */
  name = gdk_screen_make_display_name (gtk_widget_get_screen (GTK_WIDGET (provider)));

  /* call */
  if (!wrapper_dbus_client_display_items_dialog (dbus_proxy, name, 0, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }

  /* cleanup */
  g_free (name);
}



static void
dbus_proxy_provider_panel_preferences (XfcePanelPluginProvider *provider,
                                       DBusGProxy              *dbus_proxy)
{
  gchar  *name;
  GError *error = NULL;
  guint   active_panel = 0;
  GValue  value = { 0, };
  
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* try to get the panel number of this plugin */
  if (wrapper_dbus_client_get_property (dbus_proxy, xfce_panel_plugin_provider_get_id (provider),
                                        "PanelNumber", &value, NULL))
    {
      /* set the panel number */
      active_panel = g_value_get_uint (&value);

      /* unset */
      g_value_unset (&value);
    }

  /* create a screen name */
  name = gdk_screen_make_display_name (gtk_widget_get_screen (GTK_WIDGET (provider)));

  /* call */
  if (!wrapper_dbus_client_display_preferences_dialog (dbus_proxy, name, 0, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }

  /* cleanup */
  g_free (name);
}



static void
dbus_proxy_provider_remove (XfcePanelPluginProvider *provider,
                            DBusGProxy              *dbus_proxy)
{
  GError *error = NULL;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* call */
  if (!wrapper_dbus_client_set_property (dbus_proxy,
                                         xfce_panel_plugin_provider_get_id (provider),
                                         "Remove", NULL, &error))
    {
      g_critical ("DBus error: %s", error->message);
      g_error_free (error);
    }
}



gint
main (gint argc, gchar **argv)
{
  GError                  *error = NULL;
  XfcePanelPluginProvider *provider;
  GModule                 *module;
  PluginConstructFunc      construct_func;
  DBusGConnection         *dbus_connection;
  DBusGProxy              *dbus_proxy;
  WrapperPlug             *plug;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifndef NDEBUG
  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* initialize the gthread system */
  if (g_thread_supported () == FALSE)
    g_thread_init (NULL);

  /* initialize gtk */
  if (!gtk_init_with_args (&argc, &argv, _("[ARGUMENTS...]"), option_entries, GETTEXT_PACKAGE, &error))
    {
      /* print error */
      g_critical ("Failed to initialize GTK+: %s", error ? error->message : "Unable to open display");

      /* cleanup */
      if (G_LIKELY (error != NULL))
        g_error_free (error);

      /* leave */
      return EXIT_FAILURE;
    }

  /* check if the module exists */
  if (opt_filename == NULL || *opt_filename == '\0'
      || g_file_test (opt_filename, G_FILE_TEST_EXISTS) == FALSE)
    {
      /* print error */
      g_critical ("Unable to find plugin module '%s'.", opt_filename);

      /* leave */
      return EXIT_FAILURE;
    }

  /* check if all the other arguments are defined */
  if (opt_socket_id == 0
      || opt_name == NULL || *opt_name == '\0'
      || opt_id == NULL || *opt_id == '\0'
      || opt_display_name == NULL || *opt_display_name == '\0')
    {
      /* print error */
      g_critical ("One of the required arguments is missing.");

      /* leave */
      return EXIT_FAILURE;
    }

  /* try to connect to dbus */
  dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (G_UNLIKELY (dbus_connection == NULL))
    {
      /* print error */
      g_critical ("Failed to connect to dbus: %s", error->message);

      /* cleanup */
      g_error_free (error);

      /* leave */
      return EXIT_FAILURE;
    }

  /* get the dbus proxy */
  dbus_proxy = dbus_g_proxy_new_for_name (dbus_connection, "org.xfce.Panel", "/org/xfce/Panel", "org.xfce.Panel");
  if (G_UNLIKELY (dbus_proxy == NULL))
    {
      /* print error */
      g_critical ("Failed to create the dbus proxy: %s", error->message);

      /* cleanup */
      g_object_unref (G_OBJECT (dbus_connection));
      g_error_free (error);

      /* leave */
      return EXIT_FAILURE;
    }

  /* setup signal for property changes */
  dbus_g_object_register_marshaller (wrapper_marshal_VOID__STRING_STRING_BOXED, G_TYPE_NONE,
                                     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (dbus_proxy, "PropertyChanged", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);

  /* load the module and link the function */
  module = g_module_open (opt_filename, 0);
  if (G_LIKELY (module))
    {
      /* get the contruct symbol */
      if (!g_module_symbol (module, "xfce_panel_plugin_construct", (gpointer) &construct_func))
        {
          /* print error */
          g_critical ("Plugin '%s' lacks a required symbol: %s.", opt_name, g_module_error ());

          /* close the module */
          g_module_close (module);

          /* leave */
          return EXIT_FAILURE;
        }
    }
  else
    {
      /* print error */
      g_critical ("Unable to load the plugin module '%s': %s.", opt_name, g_module_error ());

      /* leave */
      return EXIT_FAILURE;
    }

  /* contruct the panel plugin */
  provider = (*construct_func) (opt_name, opt_id, opt_display_name, opt_arguments, gdk_screen_get_default ());
  if (G_LIKELY (provider))
    {
      /* create quark */
      plug_quark = g_quark_from_static_string ("plug-quark");

      /* create the wrapper plug (a gtk plug with transparency capabilities) */
      plug = wrapper_plug_new (opt_socket_id);
      g_object_set_qdata (G_OBJECT (provider), plug_quark, plug);

      /* connect provider signals */
      g_signal_connect (G_OBJECT (provider), "expand-changed", G_CALLBACK (dbus_proxy_provider_expand_changed), dbus_proxy);
      g_signal_connect (G_OBJECT (provider), "move-item", G_CALLBACK (dbus_proxy_provider_move_item), dbus_proxy);
      g_signal_connect (G_OBJECT (provider), "add-new-items", G_CALLBACK (dbus_proxy_provider_add_new_items), dbus_proxy);
      g_signal_connect (G_OBJECT (provider), "panel-preferences", G_CALLBACK (dbus_proxy_provider_panel_preferences), dbus_proxy);
      g_signal_connect (G_OBJECT (provider), "destroy", G_CALLBACK (dbus_proxy_provider_remove), dbus_proxy);

      /* connect dbus property change signal */
      dbus_g_proxy_connect_signal (dbus_proxy, "PropertyChanged", G_CALLBACK (dbus_proxy_provider_property_changed), provider, NULL);

      /* show the plugin */
      gtk_container_add (GTK_CONTAINER (plug), GTK_WIDGET (provider));
      gtk_widget_show (GTK_WIDGET (plug));
      gtk_widget_show (GTK_WIDGET (provider));

      /* enter the main loop */
      gtk_main ();

      /* destroy the plug and provider */
      gtk_widget_destroy (GTK_WIDGET (plug));
    }
  else
    {
      /* print error */
      g_critical ("Failed to contruct the plugin '%s'.", opt_name);
    }

  /* close the module */
  g_module_close (module);

  /* release dbus */
  g_object_unref (G_OBJECT (dbus_proxy));
  g_object_unref (G_OBJECT (dbus_connection));

  return EXIT_SUCCESS;
}
