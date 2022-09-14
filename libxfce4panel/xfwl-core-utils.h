/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFWL_CORE_UTILS_H__
#define __XFWL_CORE_UTILS_H__

#include <glib.h>
#include <wayland-util.h>

G_BEGIN_DECLS

/**
 * xfwl_registry_bind:
 * @interface: interface name as a token (not a string)
 *
 * A wrapper around wl_registry_bind() that allows to bind global objects available
 * from the Walyand compositor from their interface name.
 *
 * Returns: (allow-none) (transfer full): the new, client-created object bound to
 *          the server, or %NULL if @interface is not supported.
 **/
#define xfwl_registry_bind(interface) \
  xfwl_registry_bind_real (#interface, &interface##_interface)

gpointer     xfwl_registry_bind_real      (const gchar                    *interface_name,
                                           const struct wl_interface      *interface);

G_END_DECLS

#endif /* !__XFWL_CORE_UTILS_H__ */
