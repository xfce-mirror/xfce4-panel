/* $Id: xfce-panel-macros.h 25077 2007-03-03 19:26:06Z nick $
 *
 * Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>



#if (!GLIB_CHECK_VERSION(2,12,0))
/*
 * We can remove this code when the panel depends on Glib 2.12
 */
GType
g_type_register_static_simple (GType              parent_type,
                               const gchar       *type_name,
                               guint              class_size,
                               GClassInitFunc     class_init,
                               guint              instance_size,
                               GInstanceInitFunc  instance_init,
                               GTypeFlags         flags)
{
  GTypeInfo info;

  info.class_size     = class_size;
  info.base_init      = NULL;
  info.base_finalize  = NULL;
  info.class_init     = class_init;
  info.class_finalize = NULL;
  info.class_data     = NULL;
  info.instance_size  = instance_size;
  info.n_preallocs    = 0;
  info.instance_init  = instance_init;
  info.value_table    = NULL;

  return g_type_register_static (parent_type, type_name, &info, flags);
}
#endif

