/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
 *
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

#ifndef __LAUNCHER_H__
#define __LAUNCHER_H__

#include <gtk/gtk.h>
#include <xfconf/xfconf.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4menu/libxfce4menu.h>

G_BEGIN_DECLS

typedef struct _XfceLauncherPluginClass XfceLauncherPluginClass;
typedef struct _XfceLauncherPlugin      XfceLauncherPlugin;
typedef enum   _LauncherArrowType       LauncherArrowType;

#define XFCE_TYPE_LAUNCHER_PLUGIN            (launcher_plugin_get_type ())
#define XFCE_LAUNCHER_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_LAUNCHER_PLUGIN, XfceLauncherPlugin))
#define XFCE_LAUNCHER_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_LAUNCHER_PLUGIN, XfceLauncherPluginClass))
#define XFCE_IS_LAUNCHER_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_LAUNCHER_PLUGIN))
#define XFCE_IS_LAUNCHER_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_LAUNCHER_PLUGIN))
#define XFCE_LAUNCHER_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_LAUNCHER_PLUGIN, XfceLauncherPluginClass))

enum _LauncherArrowType
{
  LAUNCHER_ARROW_DEFAULT = 0,
  LAUNCHER_ARROW_NORTH,
  LAUNCHER_ARROW_WEST,
  LAUNCHER_ARROW_EAST,
  LAUNCHER_ARROW_SOUTH,
  LAUNCHER_ARROW_INTERNAL,

  LAUNCHER_ARROW_MAX = LAUNCHER_ARROW_INTERNAL
};

GType             launcher_plugin_get_type    (void) G_GNUC_CONST;

XfconfChannel    *launcher_plugin_get_channel (XfceLauncherPlugin *plugin);

GSList           *launcher_plugin_get_items   (XfceLauncherPlugin *plugin);

G_END_DECLS

#endif /* !__LAUNCHER_H__ */
