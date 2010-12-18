/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
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
  { "gdb", PANEL_DEBUG_GDB },
  { "valgrind", PANEL_DEBUG_VALGRIND }
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
  const gchar            *proxy_application;

  panel_return_if_fail (domain != NULL);
  panel_return_if_fail (message != NULL);

  /* initialize the debugging domains */
  if (g_once_init_enter (&level__volatile))
    {
      value = g_getenv ("PANEL_DEBUG");
      if (value != NULL && *value != '\0')
        {
          panel_debug_flags = g_parse_debug_string (value, panel_debug_keys,
                                                    G_N_ELEMENTS (panel_debug_keys));

          /* always enable debug logging */
          PANEL_SET_FLAG (panel_debug_flags, PANEL_DEBUG_YES);

          if (PANEL_HAS_FLAG (panel_debug_flags, PANEL_DEBUG_GDB))
            {
              proxy_application = "gdb";

              /* performs sanity checks on the released memory slices */
              g_setenv ("G_SLICE", "debug-blocks", TRUE);
            }
          else if (PANEL_HAS_FLAG (panel_debug_flags, PANEL_DEBUG_VALGRIND))
            {
              proxy_application = "valgrind";

              /* use g_malloc() and g_free() instead of slices */
              g_setenv ("G_SLICE", "always-malloc", TRUE);
              g_setenv ("G_DEBUG", "gc-friendly", TRUE);
            }
          else
            {
              proxy_application = NULL;
            }

          if (proxy_application != NULL)
            {
              path = g_find_program_in_path (proxy_application);
              if (G_LIKELY (path != NULL))
                {
                  /* TODO: only print those messages in the main application */
                  g_printerr (PACKAGE_NAME "(debug): running plugins with %s; "
                              "log files stored in %s\n", path, g_get_tmp_dir ());
                  g_free (path);
                }
              else
                {
                  PANEL_UNSET_FLAG (panel_debug_flags, PANEL_DEBUG_GDB | PANEL_DEBUG_VALGRIND);

                  g_printerr (PACKAGE_NAME "(debug): %s not found in PATH\n",
                              proxy_application);
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
