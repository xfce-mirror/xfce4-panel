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

#ifndef __PANEL_MODULE_H__
#define __PANEL_MODULE_H__

#include "libxfce4panel/libxfce4panel.h"
#include "libxfce4panel/xfce-panel-plugin-provider.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_MODULE (panel_module_get_type ())
G_DECLARE_FINAL_TYPE (PanelModule, panel_module, PANEL, MODULE, GTypeModule)

typedef enum
{
  PANEL_MODULE_RUN_MODE_NONE, /* unset */
  PANEL_MODULE_RUN_MODE_INTERNAL, /* plugin library will be loaded in the panel */
  PANEL_MODULE_RUN_MODE_EXTERNAL /* plugin library will be loaded in the wrapper
                                  * with communication through PanelPluginExternal */
} PanelModuleRunMode;



PanelModule *
panel_module_new_from_desktop_file (const gchar *filename,
                                    const gchar *name,
                                    const gchar *lib_dir,
                                    PanelModuleRunMode forced_mode) G_GNUC_MALLOC;

GtkWidget *
panel_module_new_plugin (PanelModule *module,
                         GdkScreen *screen,
                         gint unique_id,
                         gchar **arguments) G_GNUC_MALLOC;

const gchar *
panel_module_get_name (PanelModule *module) G_GNUC_PURE;

const gchar *
panel_module_get_filename (PanelModule *module) G_GNUC_PURE;

const gchar *
panel_module_get_display_name (PanelModule *module) G_GNUC_PURE;

const gchar *
panel_module_get_comment (PanelModule *module) G_GNUC_PURE;

const gchar *
panel_module_get_icon_name (PanelModule *module) G_GNUC_PURE;

const gchar *
panel_module_get_api (PanelModule *module) G_GNUC_PURE;

PanelModule *
panel_module_get_from_plugin_provider (XfcePanelPluginProvider *provider);

gboolean
panel_module_is_valid (PanelModule *module);

gboolean
panel_module_is_unique (PanelModule *module) G_GNUC_PURE;

gboolean
panel_module_is_usable (PanelModule *module,
                        GdkScreen *screen);

G_END_DECLS

#endif /* !__PANEL_MODULE_H__ */
