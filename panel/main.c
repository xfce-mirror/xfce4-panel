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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#include <panel/panel-private.h>
#include <panel/panel-application.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-dbus-client.h>

#define PANEL_CALLBACK_OPTION G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, callback_handler



static gint       opt_preferences = -1;
static gint       opt_add_items = -1;
static gboolean   opt_save = FALSE;
static gchar     *opt_add = NULL;
static gboolean   opt_restart = FALSE;
static gboolean   opt_quit = FALSE;
static gboolean   opt_version = FALSE;
static gchar     *opt_client_id = NULL;
static gchar    **opt_arguments = NULL;



static gboolean callback_handler (const gchar  *name,
                                  const gchar  *value,
                                  gpointer      user_data,
                                  GError      **error);
static void     signal_handler   (gint          signum);



/* command line options */
static GOptionEntry option_entries[] =
{
  { "preferences", 'p', PANEL_CALLBACK_OPTION, N_("Show the 'Panel Preferences' dialog"), N_("Panel Number") },
  { "add-items", 'a', PANEL_CALLBACK_OPTION, N_("Show the 'Add New Items' dialog"), N_("Panel Number") },
  { "save", 's', 0, G_OPTION_ARG_NONE, &opt_save, N_("Save the panel configuration"), NULL },
  { "add", '\0', 0, G_OPTION_ARG_STRING, &opt_add, N_("Add a new plugin to the panel"), N_("PLUGIN NAME") },
  { "restart", 'r', 0, G_OPTION_ARG_NONE, &opt_restart, N_("Restart the running panel instance"), NULL },
  { "quit", 'q', 0, G_OPTION_ARG_NONE, &opt_quit, N_("Quit the running panel instance"), NULL },
  { "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, N_("Print version information and exit"), NULL },
  { "sm-client-id", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_client_id, NULL, NULL },
  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &opt_arguments, NULL, NULL },
  { NULL }
};



static gboolean
callback_handler (const gchar  *name,
                  const gchar  *value,
                  gpointer      user_data,
                  GError      **error)
{
  /* this should actually never happen */
  if (G_UNLIKELY (name == NULL))
    return FALSE;

  switch (*(name + 1))
    {
      case 'p':
        /* set the option */
        opt_preferences = value ? MAX (0, atoi (value)) : 0;
        break;
      
      case 'a':
        /* set the option */
        opt_add_items = value ? MAX (0, atoi (value)) : 0;
        break;

      default:
        return FALSE;
    }
    
  return TRUE;
}



static void
signal_handler (gint signum)
{
  extern gboolean dbus_quit_with_restart;

  /* don't try to restart */
  dbus_quit_with_restart = FALSE;

  /* quit the main loop */
  gtk_main_quit ();
}



gint
main (gint argc, gchar **argv)
{
  PanelApplication *application;
  GError           *error = NULL;
  PanelDBusService *dbus_service;
  extern gboolean   dbus_quit_with_restart;
  gboolean          result;
  guint             i;
  const gint        signums[] = { SIGHUP, SIGINT, SIGQUIT, SIGTERM };

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifndef NDEBUG
  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* initialize the gthread system */
  if (g_thread_supported () == FALSE)
    g_thread_init (NULL);

  /* initialize gtk+ */
  if (!gtk_init_with_args (&argc, &argv, _("[ARGUMENTS...]"), option_entries, GETTEXT_PACKAGE, &error))
    {
      if (G_LIKELY (error))
        {
          /* print error */
          g_print ("%s: %s.\n", G_LOG_DOMAIN, error->message);
          g_print (_("Type '%s --help' for usage."), G_LOG_DOMAIN);
          g_print ("\n");

          /* cleanup */
          g_error_free (error);
          }
        else
          {
            g_error ("Unable to open display.");
          }

        return EXIT_FAILURE;

      /* leave */
      return EXIT_FAILURE;
    }

  /* handle option arguments */
  if (opt_version)
    {
      /* print version information */
      g_print ("%s %s (Xfce %s)\n\n", PACKAGE_NAME, PACKAGE_VERSION, xfce_version_string ());
      g_print ("%s\n", "Copyright (c) 2004-2008");
      g_print ("\t%s\n\n", _("The Xfce development team. All rights reserved."));
      g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
      g_print ("\n");

      return EXIT_SUCCESS;
    }
  else if (opt_preferences >= 0)
    {
      /* send a signal to the running instance to show the preferences dialog */
      result = panel_dbus_client_display_preferences_dialog (NULL, opt_preferences, &error);

      goto dbus_return;
    }
  else if (opt_add_items >= 0)
    {
      /* send a signal to the running instance to show the add items dialog */
      result = panel_dbus_client_display_items_dialog (NULL, opt_add_items, &error);

      goto dbus_return;
    }
  else if (opt_save)
    {
      /* send a save signal to the running instance */
      result = panel_dbus_client_save (&error);

      goto dbus_return;
    }
  else if (opt_add)
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
  else if (panel_dbus_client_check_client_running (&error))
    {
      /* quit without error if and instance is running */
      result = !!(error == NULL);

      /* print message */
      if (G_LIKELY (result == TRUE))
        g_message (_("There is already a running instance..."));

      goto dbus_return;
    }
  
  /* create dbus service */
  dbus_service = panel_dbus_service_get ();

  /* create a new application */
  application = panel_application_get ();

  /* setup signal handlers to properly quit the main loop */
  for (i = 0; i < G_N_ELEMENTS (signums); i++)
    signal (signums[i], signal_handler);

  /* enter the main loop */
  gtk_main ();
  
  /* release dbus service */
  g_object_unref (G_OBJECT (dbus_service));

  /* destroy all the opened dialogs */
  panel_application_destroy_dialogs (application);

  /* save the configuration */
  panel_application_save (application);

  /* release application reference */
  g_object_unref (G_OBJECT (application));

  /* whether we need to restart */
  if (dbus_quit_with_restart)
    {
      /* message */
      g_print ("%s\n\n", _("Restarting the Xfce Panel..."));

      /* spawn ourselfs again */
      g_spawn_command_line_async (argv[0], NULL);
    }

  return EXIT_SUCCESS;

  dbus_return:

  /* stop any running startup notification */
  gdk_notify_startup_complete ();

  if (G_UNLIKELY (error != NULL))
    {
      /* print warning */
      g_critical ("Failed to send D-BUS message: %s", error ? error->message : "No error message");

      /* cleanup */
      g_error_free (error);
    }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
