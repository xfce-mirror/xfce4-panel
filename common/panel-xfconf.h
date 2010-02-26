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

#define PANEL_PROPERTIES_INIT(panel_plugin) do { \
  GError *__err = NULL; \
  if (G_LIKELY (xfconf_init (&__err))) \
    { \
      g_object_weak_ref (G_OBJECT (panel_plugin), \
          panel_properties_shutdown, NULL); \
    } \
  else \
    { \
      g_critical ("Failed to initialize Xfconf: %s", __err->message); \
      g_error_free (__err); \
    } } while (0)



typedef struct _PanelProperty PanelProperty;
struct _PanelProperty
{
  const gchar *property;
  GType        type;
};



XfconfChannel *panel_properties_get_channel       (void);

void           panel_properties_bind              (XfconfChannel       *channel,
                                                   GObject             *object,
                                                   const gchar         *property_base,
                                                   const PanelProperty *properties,
                                                   gboolean             save_properties);

void           panel_properties_unbind            (GObject             *object);

void           panel_properties_shared_hash_table (GHashTable          *hash_table);

void           panel_properties_shutdown          (gpointer             user_data,
                                                   GObject             *where_the_object_was);

#endif /* !__PANEL_XFCONF_H__ */
