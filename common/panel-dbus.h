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

#ifndef __PANEL_DBUS_H__
#define __PANEL_DBUS_H__

/* panel dbus names */
#define PANEL_DBUS_NAME              "org.xfce.Panel"
#define PANEL_DBUS_PATH              "/org/xfce/Panel"
#define PANEL_DBUS_INTERFACE         PANEL_DBUS_NAME
#define PANEL_DBUS_WRAPPER_PATH      PANEL_DBUS_PATH "/Wrapper/%d"
#define PANEL_DBUS_WRAPPER_INTERFACE PANEL_DBUS_INTERFACE ".Wrapper"

/* internal signals send over dbus */
#define SIGNAL_SET_SIZE                 "a"
#define SIGNAL_SET_ORIENTATION          "b"
#define SIGNAL_SET_SCREEN_POSITION      "c"
#define SIGNAL_SET_LOCKED               "d"
#define SIGNAL_SAVE                     "e"
#define SIGNAL_SHOW_CONFIGURE           "f"
#define SIGNAL_SHOW_ABOUT               "g"
#define SIGNAL_REMOVED                  "h"
#define SIGNAL_WRAPPER_SET_SENSITIVE    "i"
#define SIGNAL_WRAPPER_BACKGROUND_ALPHA "j"
#define SIGNAL_WRAPPER_QUIT             "k"
#define SIGNAL_WRAPPER_BACKGROUND_COLOR "l"
#define SIGNAL_WRAPPER_BACKGROUND_IMAGE "m"
#define SIGNAL_WRAPPER_BACKGROUND_UNSET "n"

/* special types for dbus communication */
#define PANEL_TYPE_DBUS_SET_MESSAGE \
  dbus_g_type_get_struct ("GValueArray", \
                          G_TYPE_STRING, \
                          G_TYPE_VALUE, \
                          G_TYPE_INVALID)

#define PANEL_TYPE_DBUS_SET_SIGNAL \
  dbus_g_type_get_collection ("GPtrArray", \
                              PANEL_TYPE_DBUS_SET_MESSAGE)

enum
{
  DBUS_SET_PROPERTY,
  DBUS_SET_VALUE
};

#endif /* !__PANEL_DBUS_H__ */
