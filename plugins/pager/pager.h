/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PAGER_H__
#define __PAGER_H__

#include "libxfce4panel/libxfce4panel.h"

G_BEGIN_DECLS

#define PAGER_TYPE_PLUGIN (pager_plugin_get_type ())
G_DECLARE_FINAL_TYPE (PagerPlugin, pager_plugin, PAGER, PLUGIN, XfcePanelPlugin)

void
pager_plugin_register_type (XfcePanelTypeModule *type_module);

G_END_DECLS

#endif /* !__PAGER_H__ */
