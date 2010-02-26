/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_DBUS_H__
#define __PANEL_DBUS_H__

typedef enum _DBusPropertyChanged DBusPropertyChanged;

/* panel dbus names */
#define PANEL_DBUS_SERVICE_INTERFACE "org.xfce.Panel"
#define PANEL_DBUS_SERVICE_PATH      "/org/xfce/Panel"
#define PANEL_DBUS_SERVICE_NAME      PANEL_DBUS_SERVICE_INTERFACE

enum _DBusPropertyChanged
{
  DBUS_PROPERTY_CHANGED_SIZE,
  DBUS_PROPERTY_CHANGED_ORIENTATION,
  DBUS_PROPERTY_CHANGED_SCREEN_POSITION,
  DBUS_PROPERTY_CHANGED_BACKGROUND_ALPHA,
  DBUS_PROPERTY_CHANGED_ACTIVE_PANEL,
  DBUS_PROPERTY_CHANGED_SENSITIVE,
  DBUS_PROPERTY_CHANGED_EMIT_SAVE,
  DBUS_PROPERTY_CHANGED_QUIT_WRAPPER,
};

#endif /* !__PANEL_DBUS_H__ */
