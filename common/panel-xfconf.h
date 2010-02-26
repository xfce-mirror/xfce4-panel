/* $Id$ */
/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_XFCONF_H__
#define __PANEL_XFCONF_H__

#include <xfconf/xfconf.h>

typedef struct _PanelProperty PanelProperty;
struct _PanelProperty
{
  const gchar *property;
  GType        type;
};

void panel_properties_bind              (XfconfChannel       *channel,
                                         GObject             *object,
                                         const gchar         *property_base,
                                         const PanelProperty *properties);

void panel_properties_unbind            (GObject             *object);

void panel_properties_shared_hash_table (GHashTable          *hash_table);

#endif /* !__PANEL_XFCONF_H__ */
