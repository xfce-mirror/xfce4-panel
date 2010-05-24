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

enum
{
  DEBUG_LEVEL_UNKNOWN = 0,
  DEBUG_LEVEL_NONE,
  DEBUG_LEVEL_ENABLED
};



gboolean panel_debug_enabled = FALSE;



void
panel_debug (const gchar *domain,
             const gchar *message,
             ...)
{
  static volatile gsize   level__volatile = DEBUG_LEVEL_UNKNOWN;
  gsize                   level;
  const gchar            *value;
  gchar                  *string;
  va_list                 args;

  panel_return_if_fail (domain != NULL);
  panel_return_if_fail (message != NULL);

  /* initialize the debugging domains */
  if (g_once_init_enter (&level__volatile))
    {
      value = g_getenv ("PANEL_DEBUG");
      if (G_UNLIKELY (value != NULL && *value == '1'))
        level = DEBUG_LEVEL_ENABLED;
      else
        level = DEBUG_LEVEL_NONE;

      g_once_init_leave (&level__volatile, level);
    }

  /* leave when debug is disabled */
  if (level__volatile == DEBUG_LEVEL_NONE)
    return;

  va_start (args, message);
  string = g_strdup_vprintf (message, args);
  va_end (args);

  g_printerr (PACKAGE_NAME "(%s): %s\n", domain, string);
  g_free (string);
}
