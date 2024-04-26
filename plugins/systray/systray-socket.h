/*
 * Copyright (c) 2002      Anders Carlsson <andersca@gnu.org>
 * Copyright (c) 2003-2006 Vincent Untz
 * Copyright (c) 2008      Red Hat, Inc.
 * Copyright (c) 2009-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __SYSTRAY_SOCKET_H__
#define __SYSTRAY_SOCKET_H__

#include <gtk/gtk.h>
#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define SYSTRAY_TYPE_SOCKET (systray_socket_get_type ())
G_DECLARE_FINAL_TYPE (SystraySocket, systray_socket, SYSTRAY, SOCKET, GtkSocket)

void
systray_socket_register_type (GTypeModule *type_module);

GtkWidget *
systray_socket_new (GdkScreen *screen,
                    Window window) G_GNUC_MALLOC;

void
systray_socket_force_redraw (SystraySocket *socket);

gboolean
systray_socket_is_composited (SystraySocket *socket);

const gchar *
systray_socket_get_name (SystraySocket *socket);

Window *
systray_socket_get_window (SystraySocket *socket);

gboolean
systray_socket_get_hidden (SystraySocket *socket);

void
systray_socket_set_hidden (SystraySocket *socket,
                           gboolean hidden);

G_END_DECLS

#endif /* !__SYSTRAY_SOCKET_H__ */
