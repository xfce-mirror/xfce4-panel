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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <wrapper/wrapper-module.h>
#include <wrapper/wrapper-plug.h>



static gchar  *opt_name = NULL;
static gchar  *opt_display_name = NULL;
static gchar  *opt_id = NULL;
static gchar  *opt_filename = NULL;
static gint    opt_socket_id = 0;
static gchar **opt_arguments = NULL;



static const GOptionEntry option_entries[] =
{
  { "name", 'n', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_name, NULL, NULL },
  { "display-name", 'd', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_display_name, NULL, NULL },
  { "id", 'i', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_id, NULL, NULL },
  { "filename", 'f', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_filename, NULL, NULL },
  { "socket-id", 's', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_INT, &opt_socket_id, NULL, NULL },
  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &opt_arguments, NULL, NULL },
  { NULL }
};



gint
main (gint argc, gchar **argv)
{
  GError                  *error = NULL;
  XfcePanelPluginProvider *provider;
  GtkWidget               *plug;
  WrapperModule           *module;
  gboolean                 succeed = FALSE;

  /* set translation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* terminate the program on warnings and critical messages */
  g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

  /* initialize the gthread system */
  if (!g_thread_supported ())
    g_thread_init (NULL);

  /* initialize gtk */
  if (!gtk_init_with_args (&argc, &argv, _("[ARGUMENTS...]"), (GOptionEntry *) option_entries, GETTEXT_PACKAGE, &error))
    {
      /* print error */
      g_critical ("Failed to initialize GTK+: %s", error ? error->message : "Unable to open display");

      /* cleanup */
      if (G_LIKELY (error != NULL))
        g_error_free (error);

      /* leave */
      return EXIT_FAILURE;
    }

  /* check arguments */
  if (opt_filename == NULL || opt_socket_id == 0 || opt_name == NULL || opt_id == NULL || opt_display_name == NULL)
    {
      /* print error */
      g_critical ("One of the required arguments for the wrapper is missing");

      /* leave */
      return EXIT_FAILURE;
    }

  /* try to create a wrapper module */
  module = wrapper_module_new (opt_filename, opt_name);
  if (G_LIKELY (module != NULL))
    {
      /* try to create the panel plugin */
      provider = wrapper_module_create_plugin (module, opt_name, opt_id, opt_display_name, opt_arguments);
      if (G_LIKELY (provider != NULL))
        {
          /* create the plug */
          plug = wrapper_plug_new (opt_socket_id, provider);
          gtk_container_add (GTK_CONTAINER (plug), GTK_WIDGET (provider));
          gtk_widget_show (plug);

          /* show the plugin */
          gtk_widget_show (GTK_WIDGET (provider));

          /* everything worked fine */
          succeed = TRUE;

          /* enter the mainloop */
          gtk_main ();

          /* destroy the plug (and provider) */
          gtk_widget_destroy (plug);

          /* decrease the module use count */
          g_type_module_unuse (G_TYPE_MODULE (module));
        }

      /* g_object_unref for the module doesn't work,
       * so we don't do that... */
    }

  return succeed ? EXIT_SUCCESS : EXIT_FAILURE;
}
