/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#include <glib.h>
#include <common/panel-debug.h>
#include <common/panel-private.h>



PanelDebugFlag panel_debug_flags = 0;



/* additional debug levels */
static const GDebugKey panel_debug_keys[] =
{
  { "gdb", PANEL_DEBUG_GDB }
};



void
panel_debug (const gchar *domain,
             const gchar *message,
             ...)
{
  static volatile gsize   level__volatile = 0;
  const gchar            *value;
  gchar                  *string, *path;
  va_list                 args;

  panel_return_if_fail (domain != NULL);
  panel_return_if_fail (message != NULL);

  /* initialize the debugging domains */
  if (g_once_init_enter (&level__volatile))
    {
      value = g_getenv ("PANEL_DEBUG");
      if (G_UNLIKELY (value != NULL))
        {
          panel_debug_flags = g_parse_debug_string (value, panel_debug_keys,
                                                    G_N_ELEMENTS (panel_debug_keys));

          /* always enable debug logging */
          PANEL_SET_FLAG (panel_debug_flags, PANEL_DEBUG_YES);

          /* TODO: only print this in the main application */
          if (PANEL_HAS_FLAG (panel_debug_flags, PANEL_DEBUG_GDB))
            {
              path = g_find_program_in_path ("gdb");
              if (G_LIKELY (path != NULL))
                {
                  g_printerr (PACKAGE_NAME "(debug): running plugins with %s; "
                              "log files stored in '%s'\n", path, g_get_tmp_dir ());
                  g_free (path);
                }
              else
                {
                  PANEL_UNSET_FLAG (panel_debug_flags, PANEL_DEBUG_GDB);

                  g_printerr (PACKAGE_NAME "(debug): gdb not found in PATH; mode disabled\n");
                }
            }
        }

      g_once_init_leave (&level__volatile, 1);
    }

  /* leave when debug is disabled */
  if (panel_debug_flags == 0)
    return;

  va_start (args, message);
  string = g_strdup_vprintf (message, args);
  va_end (args);

  g_printerr (PACKAGE_NAME "(%s): %s\n", domain, string);
  g_free (string);
}
