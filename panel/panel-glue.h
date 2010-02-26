/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_GLUE_H__
#define __PANEL_GLUE_H__

#include <gtk/gtk.h>
#include <panel/panel-window.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

G_BEGIN_DECLS

XfceScreenPosition panel_glue_get_screen_position (PanelWindow *window);

void               panel_glue_set_provider_info   (XfcePanelPluginProvider *provider,
                                                   PanelWindow             *window);

G_END_DECLS

#endif /* !__PANEL_GLUE_H__ */
