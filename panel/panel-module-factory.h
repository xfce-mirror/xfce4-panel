/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_MODULE_FACTORY_H__
#define __PANEL_MODULE_FACTORY_H__

#include "panel-module.h"

#include "libxfce4panel/libxfce4panel.h"
#include "libxfce4panel/xfce-panel-plugin-provider.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_MODULE_FACTORY (panel_module_factory_get_type ())
G_DECLARE_FINAL_TYPE (PanelModuleFactory, panel_module_factory, PANEL, MODULE_FACTORY, GObject)

#define LAUNCHER_PLUGIN_NAME "launcher"
#define SEPARATOR_PLUGIN_NAME "separator"

PanelModuleFactory *
panel_module_factory_get (void);

void
panel_module_factory_force_run_mode (PanelModuleRunMode mode);

gboolean
panel_module_factory_has_launcher (PanelModuleFactory *factory);

void
panel_module_factory_emit_unique_changed (PanelModule *module);

GList *
panel_module_factory_get_modules (PanelModuleFactory *factory);

gboolean
panel_module_factory_has_module (PanelModuleFactory *factory,
                                 const gchar *name);

GSList *
panel_module_factory_get_plugins (PanelModuleFactory *factory,
                                  const gchar *plugin_name);

GtkWidget *
panel_module_factory_new_plugin (PanelModuleFactory *factory,
                                 const gchar *name,
                                 GdkScreen *screen,
                                 gint unique_id,
                                 gchar **arguments,
                                 gint *return_unique_id) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__PANEL_MODULE_FACTORY_H__ */
