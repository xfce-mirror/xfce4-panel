/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_DEBUG_H__
#define __PANEL_DEBUG_H__

#define PANEL_DEBUG_DOMAIN_MAIN        "main"
#define PANEL_DEBUG_DOMAIN_POSITIONING "positioning"
#define PANEL_DEBUG_DOMAIN_STRUTS      "struts"
#define PANEL_DEBUG_DOMAIN_APPLICATION "application"
#define PANEL_DEBUG_DOMAIN_EXTERNAL    "external"
#define PANEL_DEBUG_DOMAIN_EXTERNAL46  "external46"
#define PANEL_DEBUG_DOMAIN_TASKLIST    "tasklist"

#define PANEL_DEBUG_BOOL(bool) ((bool) ? "true" : "false")

extern gboolean panel_debug_enabled;

void panel_debug (const gchar *domain,
                  const gchar *message,
                  ...) G_GNUC_PRINTF (2, 3);

#endif /* !__PANEL_DEBUG_H__ */
