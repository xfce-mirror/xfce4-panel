/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __XFCE_PANEL_MACROS_H__
#define __XFCE_PANEL_MACROS_H__

#include <glib.h>

G_BEGIN_DECLS

/* xfconf channel for plugins */
#define XFCE_PANEL_PLUGIN_CHANNEL_NAME ("xfce4-panel")

/* macro for opening an XfconfChannel for plugins */
#define xfce_panel_plugin_xfconf_channel_new(plugin) \
  xfconf_channel_new_with_property_base (XFCE_PANEL_PLUGIN_CHANNEL_NAME, \
    xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (plugin)))

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
