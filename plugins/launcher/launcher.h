/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

#include "libxfce4panel/libxfce4panel.h"

#include <garcon/garcon.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_PLUGIN (launcher_plugin_get_type ())
G_DECLARE_FINAL_TYPE (LauncherPlugin, launcher_plugin, LAUNCHER, PLUGIN, XfcePanelPlugin)

typedef enum _LauncherArrowType
{
  LAUNCHER_ARROW_DEFAULT = 0,
  LAUNCHER_ARROW_NORTH,
  LAUNCHER_ARROW_WEST,
  LAUNCHER_ARROW_EAST,
  LAUNCHER_ARROW_SOUTH,
  LAUNCHER_ARROW_INTERNAL
} LauncherArrowType;

void
launcher_plugin_register_type (XfcePanelTypeModule *type_module);

GSList *
launcher_plugin_get_items (LauncherPlugin *plugin);

gchar *
launcher_plugin_unique_filename (LauncherPlugin *plugin);

GHashTable *
launcher_plugin_garcon_menu_pool (void);

gboolean
launcher_plugin_item_is_editable (LauncherPlugin *plugin,
                                  GarconMenuItem *item,
                                  gboolean *can_delete);

GIcon *
launcher_plugin_tooltip_icon (const gchar *icon_name);

G_END_DECLS

#endif /* !__LAUNCHER_H__ */
