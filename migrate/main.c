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

#include <migrate/migrate_46.h>



gint
main (gint argc, gchar **argv)
{
  gchar         *file;
  XfconfChannel *channel;
  GError        *error = NULL;

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
      g_critical ("%s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  channel = xfconf_channel_get (CHANNEL_NAME);

  file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, XFCE_46_CONFIG);
  if (file != NULL)
    {
      g_message ("found old config file: %s", file);
      if (!migrate_46 (file, channel, &error))
        {
          g_critical ("%s", error->message);
          g_error_free (error);
        }
    }

  g_free (file);

  xfconf_shutdown ();

  return EXIT_SUCCESS;
}
