/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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

#ifndef __XFCE_46_PARSER_H__
#define __XFCE_46_PARSER_H__

#include <gtk/gtk.h>
#include <xfconf/xfconf.h>

G_BEGIN_DECLS

#define XFCE_46_CONFIG "xfce4" G_DIR_SEPARATOR_S "panel" G_DIR_SEPARATOR_S "panels.xml"

gboolean migrate_46 (const gchar *filename, XfconfChannel *channel, GError **error);

G_END_DECLS

#endif /* !__XFCE_46_PARSER_H__ */

