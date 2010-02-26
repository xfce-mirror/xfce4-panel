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
#define SIGNAL_PREFIX                   '_'
#define SIGNAL_PREFIX_S                 "_"
#define SIGNAL_SET_SIZE                 SIGNAL_PREFIX_S "a"
#define SIGNAL_SET_ORIENTATION          SIGNAL_PREFIX_S "b"
#define SIGNAL_SET_SCREEN_POSITION      SIGNAL_PREFIX_S "c"
#define SIGNAL_SAVE                     SIGNAL_PREFIX_S "d"
#define SIGNAL_SHOW_CONFIGURE           SIGNAL_PREFIX_S "e"
#define SIGNAL_SHOW_ABOUT               SIGNAL_PREFIX_S "f"
#define SIGNAL_REMOVE                   SIGNAL_PREFIX_S "g"
#define SIGNAL_WRAPPER_SET_SENSITIVE    SIGNAL_PREFIX_S "h"
#define SIGNAL_WRAPPER_BACKGROUND_ALPHA SIGNAL_PREFIX_S "i"

/* special types for dbus communication */
#define PANEL_TYPE_DBUS_SET_MESSAGE dbus_g_type_get_struct ("GValueArray", \
                                                            G_TYPE_STRING, \
                                                            G_TYPE_VALUE, \
                                                            G_TYPE_UINT, \
                                                            G_TYPE_INVALID)
#define PANEL_TYPE_DBUS_SET_SIGNAL  dbus_g_type_get_collection ("GPtrArray", \
                                                                PANEL_TYPE_DBUS_SET_MESSAGE)

enum
{
  DBUS_SET_PROPERTY,
  DBUS_SET_VALUE,
  DBUS_SET_REPLY_ID
};

#endif /* !__PANEL_DBUS_H__ */
