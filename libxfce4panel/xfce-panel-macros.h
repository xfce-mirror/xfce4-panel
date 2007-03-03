/* $Id$
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

#ifndef __XFCE_PANEL_MACROS_H__
#define __XFCE_PANEL_MACROS_H__

G_BEGIN_DECLS

/* Support macro's for the slice allocator */
#if GLIB_CHECK_VERSION(2,10,0)
#define panel_slice_alloc(block_size)             (g_slice_alloc ((block_size)))
#define panel_slice_alloc0(block_size)            (g_slice_alloc0 ((block_size)))
#define panel_slice_free1(block_size, mem_block)  G_STMT_START{ g_slice_free1 ((block_size), (mem_block)); }G_STMT_END
#define panel_slice_new(type)                     (g_slice_new (type))
#define panel_slice_new0(type)                    (g_slice_new0 (type))
#define panel_slice_free(type, ptr)               G_STMT_START{ g_slice_free (type, (ptr)); }G_STMT_END
#else
#define panel_slice_alloc(block_size)             (g_malloc ((block_size)))
#define panel_slice_alloc0(block_size)            (g_malloc0 ((block_size)))
#define panel_slice_free1(block_size, mem_block)  G_STMT_START{ g_free ((mem_block)); }G_STMT_END
#define panel_slice_new(type)                     (g_new (type, 1))
#define panel_slice_new0(type)                    (g_new0 (type, 1))
#define panel_slice_free(type, ptr)               G_STMT_START{ g_free ((ptr)); }G_STMT_END
#endif

/* Canonical Strings */
#if GLIB_CHECK_VERSION(2,10,0)
#define I_(string) (g_intern_static_string ((string)))
#else
#define I_(string) (g_quark_to_string (g_quark_from_static_string ((string))))
#endif

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
