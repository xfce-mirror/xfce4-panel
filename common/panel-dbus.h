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
#define PANEL_DBUS_PATH             "/org/xfce/Panel"
#define PANEL_DBUS_PANEL_INTERFACE  "org.xfce.Panel"
#define PANEL_DBUS_PLUGIN_INTERFACE "org.xfce.PanelPlugin"

enum _DBusPropertyChanged
{
  /* provider iface */
  PROPERTY_CHANGED_PROVIDER_SIZE = 0,
  PROPERTY_CHANGED_PROVIDER_ORIENTATION,
  PROPERTY_CHANGED_PROVIDER_SCREEN_POSITION,
  PROPERTY_CHANGED_PROVIDER_EMIT_SAVE,
  PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_CONFIGURE,
  PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_ABOUT,

  /* wrapper plug */
  PROPERTY_CHANGED_WRAPPER_BACKGROUND_ALPHA,
  PROPERTY_CHANGED_WRAPPER_SET_SENSITIVE,
  PROPERTY_CHANGED_WRAPPER_QUIT
};

#endif /* !__PANEL_DBUS_H__ */
