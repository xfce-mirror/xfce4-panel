/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __XFCE_PANEL_MACROS_H__
#define __XFCE_PANEL_MACROS_H__

#include <glib.h>

G_BEGIN_DECLS

/* support macros for debugging */
#ifndef NDEBUG
#define panel_assert(expr)                  g_assert (expr)
#define panel_assert_not_reached()          g_assert_not_reached ()
#define panel_return_if_fail(expr)          g_return_if_fail (expr)
#define panel_return_val_if_fail(expr, val) g_return_val_if_fail (expr, (val))
#else
#define panel_assert(expr)                  G_STMT_START{ (void)0; }G_STMT_END
#define panel_assert_not_reached()          G_STMT_START{ (void)0; }G_STMT_END
#define panel_return_if_fail(expr)          G_STMT_START{ (void)0; }G_STMT_END
#define panel_return_val_if_fail(expr, val) G_STMT_START{ (void)0; }G_STMT_END
#endif

/* canonical representation for static strings */
#define I_(string) (g_intern_static_string ((string)))

/* static parameter strings */
#if GLIB_CHECK_VERSION (2,13,0)
#define PANEL_PARAM_STATIC_STRINGS (G_PARAM_STATIC_STRINGS)
#else
#define PANEL_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

/* gdk atoms */
#if GTK_CHECK_VERSION (2,10,0)
#define panel_atom_intern(string) gdk_atom_intern_static_string ((string))
#else
#define panel_atom_intern(string) gdk_atom_intern ((string), FALSE)
#endif

/* cairo context source color */
#define panel_cairo_set_source_rgba (cr, color, alpha) \
  if (

/* make api compatible with 4.4 panel */
#ifndef XFCE_DISABLE_DEPRECATED

/* convenience functions (deprecated) */
#define xfce_create_panel_button()        xfce_panel_create_button()
#define xfce_create_panel_toggle_button() xfce_panel_create_toggle_button()
#define xfce_allow_panel_customization()  xfce_panel_allow_customization()

/* register definitions (deprecated) */
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL             XFCE_PANEL_PLUGIN_REGISTER
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK  XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL             XFCE_PANEL_PLUGIN_REGISTER
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK  XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK

/* parameter flags (deprecated) */
#define PANEL_PARAM_READABLE  G_PARAM_READABLE | PANEL_PARAM_STATIC_STRINGS
#define PANEL_PARAM_READWRITE G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS
#define PANEL_PARAM_WRITABLE  G_PARAM_WRITABLE | PANEL_PARAM_STATIC_STRINGS

/* slice allocator (deprecated) */
#define panel_slice_alloc(block_size)             (g_slice_alloc ((block_size)))
#define panel_slice_alloc0(block_size)            (g_slice_alloc0 ((block_size)))
#define panel_slice_free1(block_size, mem_block)  G_STMT_START{ g_slice_free1 ((block_size), (mem_block)); }G_STMT_END
#define panel_slice_new(type)                     (g_slice_new (type))
#define panel_slice_new0(type)                    (g_slice_new0 (type))
#define panel_slice_free(type, ptr)               G_STMT_START{ g_slice_free (type, (ptr)); }G_STMT_END

/* debug macros (deprecated) */
#define _panel_assert(expr)                  panel_assert (expr)
#define _panel_assert_not_reached()          panel_assert_not_reached ()
#define _panel_return_if_fail(expr)          panel_return_if_fail (expr)
#define _panel_return_val_if_fail(expr, val) panel_return_val_if_fail (expr, (val))

#endif /* !XFCE_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
