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

#ifndef __PANEL_DBUS_SERVICE_H__
#define __PANEL_DBUS_SERVICE_H__

#include "panel-gdbus-exported-service.h"

#include <glib.h>

G_BEGIN_DECLS

#define PANEL_TYPE_DBUS_SERVICE (panel_dbus_service_get_type ())
G_DECLARE_FINAL_TYPE (PanelDBusService, panel_dbus_service, PANEL, DBUS_SERVICE, XfcePanelExportedServiceSkeleton)

PanelDBusService *
panel_dbus_service_get (void);

void
panel_dbus_service_exit_panel (gboolean restart);

gboolean
panel_dbus_service_get_restart (void);

G_END_DECLS

#endif /* !__PANEL_DBUS_SERVICE_H__ */
