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

#ifndef __PANEL_PLUGIN_EXTERNAL_WRAPPER_H__
#define __PANEL_PLUGIN_EXTERNAL_WRAPPER_H__

#include "panel-module.h"
#include "panel-plugin-external.h"

G_BEGIN_DECLS

#define PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER (panel_plugin_external_wrapper_get_type ())
G_DECLARE_DERIVABLE_TYPE (PanelPluginExternalWrapper, panel_plugin_external_wrapper, PANEL, PLUGIN_EXTERNAL_WRAPPER, PanelPluginExternal)

struct _PanelPluginExternalWrapperClass
{
  PanelPluginExternalClass __parent__;
};

GtkWidget *
panel_plugin_external_wrapper_new (PanelModule *module,
                                   gint unique_id,
                                   gchar **arguments) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__PANEL_PLUGIN_EXTERNAL_WRAPPER_H__ */
