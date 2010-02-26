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

#ifndef __LIBXFCE4PANEL_DEPRECATED_H__
#define __LIBXFCE4PANEL_DEPRECATED_H__

//#ifndef XFCE_DISABLE_DEPRECATED


G_BEGIN_DECLS

#define panel_slice_alloc(block_size)            (g_slice_alloc ((block_size)))
#define panel_slice_alloc0(block_size)           (g_slice_alloc0 ((block_size)))
#define panel_slice_free1(block_size, mem_block) G_STMT_START{ g_slice_free1 ((block_size), (mem_block)); }G_STMT_END
#define panel_slice_new(type)                    (g_slice_new (type))
#define panel_slice_new0(type)                   (g_slice_new0 (type))
#define panel_slice_free(type, ptr)              G_STMT_START{ g_slice_free (type, (ptr)); }G_STMT_END

#define PANEL_PARAM_READABLE  (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)
#define PANEL_PARAM_WRITABLE  (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)
#define PANEL_PARAM_READWRITE (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

#define _panel_assert(expr)                  g_assert (expr)
#define _panel_assert_not_reached()          g_assert_not_reached ()
#define _panel_return_if_fail(expr)          g_return_if_fail (expr)
#define _panel_return_val_if_fail(expr, val) g_return_val_if_fail (expr, (val))

#define _panel_g_type_register_simple(type_parent,type_name_static,class_size,class_init,instance_size,instance_init) \
    g_type_register_static_simple(type_parent,type_name_static,class_size,class_init,instance_size,instance_init, 0)

G_END_DECLS

//#endif /* !XFCE_DISABLE_DEPRECATED */

#endif /* !__LIBXFCE4PANEL_DEPRECATED_H__ */
