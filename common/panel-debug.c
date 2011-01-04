/*
 * Copyright (C) 2010-2011 Nick Schermer <nick@xfce.org>
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
  { "main", PANEL_DEBUG_MAIN },
  { "positioning", PANEL_DEBUG_POSITIONING },
  { "display-layout", PANEL_DEBUG_DISPLAY_LAYOUT },
  { "struts", PANEL_DEBUG_STRUTS },
  { "application", PANEL_DEBUG_APPLICATION },
  { "external", PANEL_DEBUG_EXTERNAL },
  { "external46", PANEL_DEBUG_EXTERNAL46 },
  { "tasklist", PANEL_DEBUG_TASKLIST },
  { "base-window", PANEL_DEBUG_BASE_WINDOW },
  { "applicationmenu", PANEL_DEBUG_APPLICATIONMENU },
  { "gdb", PANEL_DEBUG_GDB },
  { "valgrind", PANEL_DEBUG_VALGRIND }
};



static PanelDebugFlag
panel_debug_init (void)
{
  static volatile gsize  inited__volatile = 0;
  const gchar           *value;
  gchar                 *path;
  const gchar           *proxy_application;

  if (g_once_init_enter (&inited__volatile))
    {
      value = g_getenv ("PANEL_DEBUG");
      if (value != NULL && *value != '\0')
        {
          panel_debug_flags = g_parse_debug_string (value, panel_debug_keys,
                                                    G_N_ELEMENTS (panel_debug_keys));

          /* always enable (unfiltered) debugging messages */
          PANEL_SET_FLAG (panel_debug_flags, PANEL_DEBUG_YES);

          if (PANEL_HAS_FLAG (panel_debug_flags, PANEL_DEBUG_GDB))
            {
              proxy_application = "gdb";

              /* performs sanity checks on the released memory slices */
              g_setenv ("G_SLICE", "debug-blocks", TRUE);

              /* make sure we don't run gdb and valgrind at the same time */
              PANEL_UNSET_FLAG (panel_debug_flags, PANEL_DEBUG_VALGRIND);
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

      g_once_init_leave (&inited__volatile, 1);
    }

  return panel_debug_flags;
}



static void
panel_debug_print (PanelDebugFlag  domain,
                   const gchar    *message,
                   va_list         args)
{
  gchar       *string;
  const gchar *domain_name = NULL;
  guint        i;

  /* lookup domain name */
  for (i = 0; i < G_N_ELEMENTS (panel_debug_keys); i++)
    {
      if (panel_debug_keys[i].value == domain)
        {
          domain_name = panel_debug_keys[i].key;
          break;
        }
    }

  panel_assert (domain_name != NULL);

  string = g_strdup_vprintf (message, args);
  g_printerr (PACKAGE_NAME "(%s): %s\n", domain_name, string);
  g_free (string);
}



void
panel_debug (PanelDebugFlag  domain,
             const gchar    *message,
             ...)
{
  va_list args;

  panel_return_if_fail (domain > 0);
  panel_return_if_fail (message != NULL);

  /* leave when debug is disabled */
  if (panel_debug_init () == 0)
    return;

  va_start (args, message);
  panel_debug_print (domain, message, args);
  va_end (args);
}



void
panel_debug_filtered (PanelDebugFlag  domain,
                      const gchar    *message,
                      ...)
{
  va_list args;

  panel_return_if_fail (domain > 0);
  panel_return_if_fail (message != NULL);

  /* leave when the filter does not match */
  if (!PANEL_HAS_FLAG (panel_debug_init (), domain))
    return;

  va_start (args, message);
  panel_debug_print (domain, message, args);
  va_end (args);
}
