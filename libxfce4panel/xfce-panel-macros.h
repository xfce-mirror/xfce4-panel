/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_PANEL_MACROS_H
#define _XFCE_PANEL_MACROS_H

/**
 * Support macro's for the slice allocator
 **/
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

#endif /* _XFCE_PANEL_MACROS_H */

