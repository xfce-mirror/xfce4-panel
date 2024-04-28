/*
 * Copyright (C) 2011 Nick Schermer <nick@xfce.org>
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

#ifndef __MIGRATE_CONFIG_H__
#define __MIGRATE_CONFIG_H__

#include <glib.h>
#include <xfconf/xfconf.h>

G_BEGIN_DECLS

gboolean
migrate_config (XfconfChannel *channel,
                gint configver,
                GError **error);

G_END_DECLS

#endif /* !__MIGRATE_CONFIG_H__ */
