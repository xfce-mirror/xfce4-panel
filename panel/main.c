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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <libxfce4util/libxfce4util.h>

#include <panel/panel-private.h>
#include <panel/panel-application.h>
#include <panel/panel-dbus-service.h>
#include <panel/panel-dbus-client.h>



static gboolean  opt_customize = FALSE;
static gboolean  opt_add = FALSE;
static gboolean  opt_save = FALSE;
static gboolean  opt_restart = FALSE;
static gboolean  opt_quit = FALSE;
static gboolean  opt_version = FALSE;
static gchar    *opt_client_id = NULL;



/* command line options */
static const GOptionEntry option_entries[] =
{
  { "customize", 'c', 0, G_OPTION_ARG_NONE, &opt_customize, N_("Show the 'Customize Panel' dialog"), NULL },
  { "add", 'a', 0, G_OPTION_ARG_NONE, &opt_add, N_("Show the 'Add New Items' dialog"), NULL },
  { "save", 's', 0, G_OPTION_ARG_NONE, &opt_save, N_("Save the panel configuration"), NULL },
  { "restart", 'r', 0, G_OPTION_ARG_NONE, &opt_restart, N_("Restart the running panel instance"), NULL },
  { "quit", 'q', 0, G_OPTION_ARG_NONE, &opt_quit, N_("Quit the running panel instance"), NULL },
  { "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, N_("Print version information and exit"), NULL },
  { "sm-client-id", '\0', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_client_id, NULL, NULL },
  { NULL }
};



gint
main (gint argc, gchar **argv)
{
  PanelApplication *application;
  GError           *error = NULL;
  GObject          *dbus_service;
  extern gboolean   dbus_quit_with_restart;
  gboolean          result;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

  /* initialize the gthread system */
  if (!g_thread_supported ())
    g_thread_init (NULL);

  /* initialize gtk+ */
  if (!gtk_init_with_args (&argc, &argv, "", (GOptionEntry *) option_entries, GETTEXT_PACKAGE, &error))
    {
      /* print an error message */
      if (error == NULL)
        {
          /* print warning */
          g_critical ("Failed to open display: %s", gdk_get_display_arg_name ());
        }
      else
        {
          /* print warning */
          g_critical (error->message);

          /* cleanup */
          g_error_free (error);
        }

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
  else if (opt_customize)
    {
      /* send a signal to the running instance to show the preferences dialog */
      result = panel_dbus_client_display_preferences_dialog (NULL, &error);

      goto dbus_return;
    }
  else if (opt_add)
    {
      /* send a signal to the running instance to show the add items dialog */
      result = panel_dbus_client_display_items_dialog (NULL, &error);

      goto dbus_return;
    }
  else if (opt_save)
    {
      /* send a save signal to the running instance */
      result = panel_dbus_client_save (&error);

      goto dbus_return;
    }
  else if (opt_restart || opt_quit)
    {
      /* send a terminate signal to the running instance */
      result = panel_dbus_client_terminate (opt_restart, &error);

      goto dbus_return;
    }

  /* create a new application */
  application = panel_application_get ();

  /* create dbus service */
  dbus_service = panel_dbus_service_new ();

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
      g_message (_("Restarting..."));

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
