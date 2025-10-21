/*
 * Copyright (C) 2025 Dmitry Petrachkov <dmitry-petrachkov@outlook.com>
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

#include "launcher-item-info.h"
#include "launcher.h"

#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>



gchar *
launcher_get_item_name (GarconMenuItem *item)
{
  const gchar *name = garcon_menu_item_get_name (item);

  if (xfce_str_is_empty (name))
    return g_strdup (_("Unnamed item"));

  return g_strdup (name);
}



gchar *
launcher_get_item_name_markup (GarconMenuItem *item)
{
  gchar *name = launcher_get_item_name (item);
  const gchar *comment = garcon_menu_item_get_comment (item);
  gchar *markup = NULL;

  if (!xfce_str_is_empty (comment))
    markup = g_markup_printf_escaped ("<b>%s</b>\n%s", name, comment);
  else
    markup = g_markup_printf_escaped ("<b>%s</b>", name);
  g_free (name);
  return markup;
}



GIcon *
launcher_get_item_icon (GarconMenuItem *item)
{
  return launcher_plugin_tooltip_icon (garcon_menu_item_get_icon_name (item));
}



gchar *
launcher_get_item_tooltip (GarconMenuItem *item)
{
  GFile *file = garcon_menu_item_get_file (item);
  gchar *tooltip = g_file_get_parse_name (file);

  g_clear_object (&file);
  return tooltip;
}
