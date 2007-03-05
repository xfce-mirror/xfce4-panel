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
#include <glib-object.h>

#include "xfce-panel-macros.h"



GType
_panel_g_type_register_simple (GType        type_parent,
                               const gchar *type_name_static,
                               guint        class_size,
                               gpointer     class_init,
                               guint        instance_size,
                               gpointer     instance_init)
{
  /* generate the type info (on the stack) */
  GTypeInfo info =
  {
    class_size,
    NULL,
    NULL,
    class_init,
    NULL,
    NULL,
    instance_size,
    0,
    instance_init,
    NULL,
  };

  /* register the static type */
  return g_type_register_static (type_parent, I_(type_name_static), &info, 0);
}
