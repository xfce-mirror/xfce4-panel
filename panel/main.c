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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>
#include <panel/panel-application.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-dbus-client.h>



static gint       opt_preferences = -1;
static gint       opt_add_items = -1;
static gboolean   opt_save = FALSE;
static gchar     *opt_add = NULL;
static gboolean   opt_restart = FALSE;
static gboolean   opt_quit = FALSE;
static gboolean   opt_version = FALSE;
static gchar     *opt_plugin_event = NULL;
static gchar    **opt_arguments = NULL;



static gboolean callback_handler (const gchar  *name,
                                  const gchar  *value,
                                  gpointer      user_data,
                                  GError      **error);



/* command line options */
#define PANEL_CALLBACK_OPTION G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, callback_handler
static GOptionEntry option_entries[] =
{
  { "preferences", 'p', PANEL_CALLBACK_OPTION, N_("Show the 'Panel Preferences' dialog"), N_("PANEL-NUMBER") },
  { "add-items", 'a', PANEL_CALLBACK_OPTION, N_("Show the 'Add New Items' dialog"), N_("PANEL-NUMBER") },
  { "save", 's', 0, G_OPTION_ARG_NONE, &opt_save, N_("Save the panel configuration"), NULL },
  { "add", '\0', 0, G_OPTION_ARG_STRING, &opt_add, N_("Add a new plugin to the panel"), N_("PLUGIN-NAME") },
  { "restart", 'r', 0, G_OPTION_ARG_NONE, &opt_restart, N_("Restart the running panel instance"), NULL },
  { "quit", 'q', 0, G_OPTION_ARG_NONE, &opt_quit, N_("Quit the running panel instance"), NULL },
  { "version", 'V', 0, G_OPTION_ARG_NONE, &opt_version, N_("Print version information and exit"), NULL },
  { "plugin-event", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_plugin_event, NULL, NULL },
  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &opt_arguments, NULL, NULL },
  { NULL }
};



static gboolean
callback_handler (const gchar  *name,
                  const gchar  *value,
                  gpointer      user_data,
                  GError      **error)
{
  panel_return_val_if_fail (name != NULL, FALSE);

  if (strcmp (name, "--preferences") == 0
      || strcmp (name, "-p") == 0)
    {
      opt_preferences = value != NULL ? MAX (0, atoi (value)) : 0;
    }
  else if (strcmp (name, "--add-items") == 0
           || strcmp (name, "-a") == 0)
    {
      opt_add_items = value != NULL ? MAX (0, atoi (value)) : 0;
    }
  else
    {
      panel_assert_not_reached ();
      return FALSE;
    }

  return TRUE;
}



static void
signal_handler_quit (gint signum)
{
  panel_dbus_service_exit_panel (FALSE);
}



gint
main (gint argc, gchar **argv)
{
  GOptionContext   *context;
  PanelApplication *application;
  GError           *error = NULL;
  PanelDBusService *dbus_service;
  gboolean          result;
  guint             i;
  const gint        signums[] = { SIGHUP, SIGINT, SIGQUIT, SIGTERM };
  const gchar      *error_msg;
  XfceSMClient     *sm_client;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifdef G_ENABLE_DEBUG
  /* do NOT remove this line for now, If something doesn't work,
   * fix your code instead! */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* parse context options */
  context = g_option_context_new (_("[ARGUMENTS...]"));
  g_option_context_add_main_entries (context, option_entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_add_group (context, xfce_sm_client_get_option_group (argc, argv));
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("%s: %s.\n", PACKAGE_NAME, error->message);
      g_print (_("Type '%s --help' for usage."), G_LOG_DOMAIN);
      g_print ("\n");
      g_error_free (error);

      return EXIT_FAILURE;
    }
  g_option_context_free (context);

  /* initialize gtk */
  gtk_init (&argc, &argv);

  /* handle option arguments */
  if (opt_version)
    {
      /* print version information */
      g_print ("%s %s (Xfce %s)\n\n", PACKAGE_NAME, PACKAGE_VERSION, xfce_version_string ());
      g_print ("%s\n", "Copyright (c) 2004-2010");
      g_print ("\t%s\n\n", _("The Xfce development team. All rights reserved."));
      g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
      g_print ("\n");

      return EXIT_SUCCESS;
    }
  else if (opt_preferences >= 0)
    {
      /* send a signal to the running instance to show the preferences dialog */
      result = panel_dbus_client_display_preferences_dialog (opt_preferences, &error);
      goto dbus_return;
    }
  else if (opt_add_items >= 0)
    {
      /* send a signal to the running instance to show the add items dialog */
      result = panel_dbus_client_display_items_dialog (opt_add_items, &error);
      goto dbus_return;
    }
  else if (opt_save)
    {
      /* send a save signal to the running instance */
      result = panel_dbus_client_save (&error);
      goto dbus_return;
    }
  else if (opt_add != NULL)
    {
      /* stop any running startup notification */
      gdk_notify_startup_complete ();

      /* send a add new item signal to the running instance */
      result = panel_dbus_client_add_new_item (opt_add, opt_arguments, &error);
      goto dbus_return;
    }
  else if (opt_restart || opt_quit)
    {
      /* send a terminate signal to the running instance */
      result = panel_dbus_client_terminate (opt_restart, &error);
      goto dbus_return;
    }
  else if (opt_plugin_event != NULL)
    {
      /* send the plugin event to the running instance */
      result = panel_dbus_client_plugin_event (opt_plugin_event, &error);
      goto dbus_return;
    }
  else if (panel_dbus_client_check_instance_running ())
    {
      /* quit without error if and instance is running */
      result = TRUE;

      /* print message */
      g_print ("%s: %s\n\n", G_LOG_DOMAIN, _("There is already a running instance"));
      goto dbus_return;
    }

  /* start session management */
  sm_client = xfce_sm_client_get ();
  xfce_sm_client_set_restart_style (sm_client, XFCE_SM_CLIENT_RESTART_IMMEDIATELY);
  g_signal_connect (G_OBJECT (sm_client), "quit",
      G_CALLBACK (signal_handler_quit), NULL);
  if (!xfce_sm_client_connect (sm_client, &error))
    {
      g_warning ("Failed to connect to session manager: %s", error->message);
      g_error_free (error);
    }

  /* create dbus service */
  dbus_service = panel_dbus_service_get ();

  /* create a new application */
  application = panel_application_get ();

  /* setup signal handlers to properly quit the main loop */
  for (i = 0; i < G_N_ELEMENTS (signums); i++)
    signal (signums[i], signal_handler_quit);

  /* enter the main loop */
  gtk_main ();

  /* release dbus service */
  g_object_unref (G_OBJECT (dbus_service));

  /* destroy all the opened dialogs */
  panel_application_destroy_dialogs (application);

  /* save the configuration */
  panel_application_save (application, TRUE);

  /* release application reference */
  g_object_unref (G_OBJECT (application));

  /* release session reference */
  g_object_unref (G_OBJECT (sm_client));

  /* whether we need to restart */
  if (panel_dbus_service_get_restart ())
    {
      /* message */
      g_print ("%s: %s\n\n", G_LOG_DOMAIN, _("Restarting"));

      /* spawn ourselfs again */
      g_spawn_command_line_async (argv[0], NULL);
    }

  return EXIT_SUCCESS;

dbus_return:

  /* stop any running startup notification */
  gdk_notify_startup_complete ();

  if (G_UNLIKELY (error != NULL))
    {
      /* get suitable error message */
      if (opt_preferences >= 0)
        error_msg = _("Failed to show the preferences dialog");
      else if (opt_add_items >= 0)
        error_msg = _("Failed to show the add new items dialog");
      else if (opt_save)
        error_msg = _("Failed to save the panel configuration");
      else if (opt_add)
        error_msg = _("Failed to remote add a plugin to the panel");
      else if (opt_restart)
        error_msg = _("Failed to restart the panel");
      else if (opt_quit)
        error_msg = _("Failed to quit the panel");
      else
        error_msg = _("Failed to send D-Bus message");

      /* show understandable message for this common error */
      if (error->code == DBUS_GERROR_NAME_HAS_NO_OWNER)
        {
          g_clear_error (&error);
          g_set_error (&error, 0, 0, _("No running instance of %s found"), G_LOG_DOMAIN);
        }

      /* show error dialog */
      xfce_dialog_show_error (NULL, error, "%s", error_msg);
      g_error_free (error);
    }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
