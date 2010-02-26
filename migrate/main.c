/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <migrate/migrate-46.h>
#include <migrate/migrate-default.h>



gint
main (gint argc, gchar **argv)
{
  gchar     *file;
  GError    *error = NULL;
  GtkWidget *dialog;
  GtkWidget *button;
  gint       result;
  gint       retval = EXIT_SUCCESS;
  gboolean   default_config_exists;
  gint       default_response = GTK_RESPONSE_CANCEL;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

#ifndef NDEBUG
  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);
#endif

  /* initialize gtk */
  gtk_init (&argc, &argv);

  if (!xfconf_init (&error))
    {
      g_critical ("Failed to initialize Xfconf: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  /* lookup the old 4.6 config file */
  file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, XFCE_46_CONFIG);

  /* create question dialog */
  dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   _("Welcome to the first start of the Xfce Panel"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                   _("Click one of the options below for the "
                                    "initial configuration of the Xfce Panel"));
  gtk_window_set_title (GTK_WINDOW (dialog), "Xfce Panel");
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);
  gtk_window_stick (GTK_WINDOW (dialog));
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Migrate old config"), GTK_RESPONSE_OK);
  gtk_widget_set_tooltip_text (button, _("The panel will migrate your existing 4.6 panel configuration"));
  gtk_widget_set_sensitive (button, file != NULL);
  if (file != NULL)
    default_response = GTK_RESPONSE_OK;

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Use default config"), GTK_RESPONSE_YES);
  gtk_widget_set_tooltip_text (button, _("The default configuration will be loaded"));
  default_config_exists = g_file_test (DEFAULT_CONFIG, G_FILE_TEST_IS_REGULAR);
  gtk_widget_set_sensitive (button, default_config_exists);
  if (default_config_exists && file == NULL)
    default_response = GTK_RESPONSE_YES;

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Cancel"), GTK_RESPONSE_CANCEL);
  gtk_widget_set_tooltip_text (button, _("Xfce Panel will start with one empty panel"));

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), default_response);

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (result == GTK_RESPONSE_OK && file != NULL)
    {
      /* restore 4.6 config */
      if (!migrate_46 (file, &error))
        {
          xfce_dialog_show_error (NULL, error, _("Failed to migrate the old panel configuration"));
          g_error_free (error);
          retval = EXIT_FAILURE;
        }
    }
  else if (result == GTK_RESPONSE_YES && default_config_exists)
    {
      /* apply default config */
      if (!migrate_default (DEFAULT_CONFIG, &error))
        {
          xfce_dialog_show_error (NULL, error, _("Failed to load the default configuration"));
          g_error_free (error);
          retval = EXIT_FAILURE;
        }
    }

  g_free (file);

  xfconf_shutdown ();

  return retval;
}
