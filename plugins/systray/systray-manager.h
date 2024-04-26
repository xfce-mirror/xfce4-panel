/*
 * Copyright (c) 2002      Anders Carlsson <andersca@gnu.org>
 * Copyright (c) 2003-2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2003-2004 Olivier Fourdan <fourdan@xfce.org>
 * Copyright (c) 2003-2006 Vincent Untz
 * Copyright (c) 2007-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __SYSTRAY_MANAGER_H__
#define __SYSTRAY_MANAGER_H__

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SYSTRAY_TYPE_MANAGER (systray_manager_get_type ())
G_DECLARE_FINAL_TYPE (SystrayManager, systray_manager, SYSTRAY, MANAGER, GObject)

void
systray_manager_register_type (XfcePanelTypeModule *type_module);

GQuark
systray_manager_error_quark (void);

SystrayManager *
systray_manager_new (void) G_GNUC_MALLOC;

gboolean
systray_manager_register (SystrayManager *manager,
                          GdkScreen *screen,
                          GError **error);

void
systray_manager_unregister (SystrayManager *manager);

void
systray_manager_set_colors (SystrayManager *manager,
                            GdkRGBA *fg,
                            GdkRGBA *error,
                            GdkRGBA *warning,
                            GdkRGBA *success);

void
systray_manager_set_orientation (SystrayManager *manager,
                                 GtkOrientation orientation);

G_END_DECLS

#endif /* !__SYSTRAY_MANAGER_H__ */
