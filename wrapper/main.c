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

#include <wrapper/wrapper-dbus-client-infos.h>



static gchar     *opt_display_name = NULL;
static gint       opt_unique_id = -1;
static gchar     *opt_comment = NULL;
static gchar     *opt_filename = NULL;
static gint       opt_socket_id = 0;
static gchar    **opt_arguments = NULL;
static GQuark     plug_quark = 0;
static gboolean   gproxy_destroyed = FALSE;



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
wrapper_gproxy_set (DBusGProxy              *dbus_gproxy,
                    const GPtrArray         *array,
                    XfcePanelPluginProvider *provider)
{
  WrapperPlug *plug;
  guint        i;
  GValue      *value;
  gchar       *property;
  guint        reply_id;
  GValue       msg = { 0, };

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&msg, PANEL_TYPE_DBUS_SET_MESSAGE);

  for (i = 0; i < array->len; i++)
    {
      g_value_set_static_boxed (&msg, g_ptr_array_index (array, i));
      if (!dbus_g_type_struct_get (&msg,
                                   DBUS_SET_PROPERTY, &property,
                                   DBUS_SET_VALUE, &value,
                                   DBUS_SET_REPLY_ID, &reply_id,
                                   G_MAXUINT))
        {
          panel_assert_not_reached ();
          continue;
        }

      if (G_LIKELY (*property == SIGNAL_PREFIX))
        {
          if (strcmp (property, SIGNAL_SET_SIZE) == 0)
            xfce_panel_plugin_provider_set_size (provider, g_value_get_int (value));
          else if (strcmp (property, SIGNAL_SET_ORIENTATION) == 0)
            xfce_panel_plugin_provider_set_orientation (provider, g_value_get_uint (value));
          else if (strcmp (property, SIGNAL_SET_SCREEN_POSITION) == 0)
            xfce_panel_plugin_provider_set_screen_position (provider, g_value_get_uint (value));
          else if (strcmp (property, SIGNAL_WRAPPER_BACKGROUND_ALPHA) == 0)
            {
              plug = g_object_get_qdata (G_OBJECT (provider), plug_quark);
              wrapper_plug_set_background_alpha (plug, g_value_get_double (value));
            }
          else if (strcmp (property, SIGNAL_SAVE) == 0)
            xfce_panel_plugin_provider_save (provider);
          else if (strcmp (property, SIGNAL_SHOW_CONFIGURE) == 0)
            xfce_panel_plugin_provider_show_configure (provider);
          else if (strcmp (property, SIGNAL_SHOW_ABOUT) == 0)
            xfce_panel_plugin_provider_show_about (provider);
          else if (strcmp (property, SIGNAL_REMOVE) == 0)
            xfce_panel_plugin_provider_remove (provider);
          else if (strcmp (property, SIGNAL_WRAPPER_SET_SENSITIVE) == 0)
            gtk_widget_set_sensitive (GTK_WIDGET (provider), g_value_get_boolean (value));
          else
            panel_assert_not_reached ();
        }
      else
        {
          xfce_panel_plugin_provider_remote_event (provider, property, value);
        }

      g_free (property);
      g_value_unset (value);
      g_free (value);
    }
}



static void
wrapper_gproxy_provider_signal_callback (DBusGProxy *proxy,
                                         GError     *error,
                                         gpointer    user_data)
{
  if (G_UNLIKELY (error != NULL))
    {
      g_warning ("Failed to send provider signal %d: %s",
                 GPOINTER_TO_UINT (user_data), error->message);
      g_error_free (error);
    }
}



static void
wrapper_gproxy_provider_signal (XfcePanelPluginProvider       *provider,
                                XfcePanelPluginProviderSignal  provider_signal,
                                DBusGProxy                    *dbus_gproxy)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  panel_return_if_fail (opt_unique_id == xfce_panel_plugin_provider_get_unique_id (provider));

  /* send the provider signal to the panel */
  wrapper_dbus_provider_signal_async (dbus_gproxy, provider_signal,
                                      wrapper_gproxy_provider_signal_callback,
                                      GUINT_TO_POINTER (provider_signal));
}



static void
wrapper_gproxy_destroyed (DBusGProxy  *dbus_gproxy,
                          GError     **error)
{
  panel_return_if_fail (error == NULL || *error == NULL);

  /* we lost communication with the panel, silently close the wrapper */
  gproxy_destroyed = TRUE;

  gtk_main_quit ();
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
  DBusGProxy             *dbus_gproxy = NULL;
  WrapperModule          *module = NULL;
  WrapperPlug            *plug;
  GtkWidget              *provider;
  gchar                  *path;
  guint                   gproxy_destroy_id = 0;

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

  gtk_init (&argc, &argv);

  /* connect the dbus proxy */
  dbus_gconnection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (G_UNLIKELY (dbus_gconnection == NULL))
    goto leave;

  path = g_strdup_printf (PANEL_DBUS_WRAPPER_PATH, opt_unique_id);
  dbus_gproxy = dbus_g_proxy_new_for_name_owner (dbus_gconnection,
                                                 PANEL_DBUS_NAME,
                                                 path,
                                                 PANEL_DBUS_WRAPPER_INTERFACE,
                                                 &error);
  g_free (path);
  if (G_UNLIKELY (dbus_gproxy == NULL))
    goto leave;

  /* quit when the proxy is destroyed (panel segfault for example) */
  gproxy_destroy_id = g_signal_connect (G_OBJECT (dbus_gproxy), "destroy",
      G_CALLBACK (wrapper_gproxy_destroyed), &error);

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
      g_object_add_weak_pointer (G_OBJECT (plug), (gpointer *) &plug);
      gtk_widget_show (GTK_WIDGET (plug));

      /* set plug data to provider */
      plug_quark = g_quark_from_static_string ("plug-quark");
      g_object_set_qdata (G_OBJECT (provider), plug_quark, plug);

      /* monitor provider signals */
      g_signal_connect (G_OBJECT (provider), "provider-signal",
          G_CALLBACK (wrapper_gproxy_provider_signal), dbus_gproxy);

      /* connect dbus signal to set provider properties send from the panel */
      dbus_g_proxy_add_signal (dbus_gproxy, "Set", PANEL_TYPE_DBUS_SET_SIGNAL, G_TYPE_INVALID);
      dbus_g_proxy_connect_signal (dbus_gproxy, "Set",
          G_CALLBACK (wrapper_gproxy_set), g_object_ref (provider),
          (GClosureNotify) g_object_unref);

      /* show the plugin */
      gtk_widget_show (GTK_WIDGET (provider));

      /* everything when fine */
      retval = WRAPPER_EXIT_SUCCESS;

      /* enter the main loop */
      gtk_main ();

      /* disconnect signals */
      if (!gproxy_destroyed)
        dbus_g_proxy_disconnect_signal (dbus_gproxy, "Set",
            G_CALLBACK (wrapper_gproxy_set), provider);

      /* destroy the plug and provider */
      if (plug != NULL)
        gtk_widget_destroy (GTK_WIDGET (plug));
    }
  else
    {
      retval = WRAPPER_EXIT_NO_PROVIDER;
    }

leave:
  /* release the proxy */
  if (G_LIKELY (dbus_gproxy != NULL))
    {
      if (G_LIKELY (gproxy_destroy_id != 0 && !gproxy_destroyed))
        g_signal_handler_disconnect (G_OBJECT (dbus_gproxy), gproxy_destroy_id);

      g_object_unref (G_OBJECT (dbus_gproxy));
    }

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
