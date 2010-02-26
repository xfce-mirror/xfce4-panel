/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __XFCE_PANEL_MACROS_H__
#define __XFCE_PANEL_MACROS_H__

#include <glib.h>

G_BEGIN_DECLS

/* support macros for debugging (improved macro for better position indication) */
#ifndef NDEBUG
#define panel_assert(expr)                 g_assert (expr)
#define panel_assert_not_reached()         g_assert_not_reached ()
#define panel_return_if_fail(expr)         panel_return_val_if_fail(expr,{})
#define panel_return_val_if_fail(expr,val) G_STMT_START { \
  if (G_UNLIKELY (!(expr))) \
    { \
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
             "%s (%s): expression '%s' failed.", G_STRLOC, G_STRFUNC, \
             #expr); \
      return (val); \
    }; }G_STMT_END
#else
#define panel_assert(expr)                 G_STMT_START{ (void)0; }G_STMT_END
#define panel_assert_not_reached()         G_STMT_START{ (void)0; }G_STMT_END
#define panel_return_if_fail(expr)         G_STMT_START{ (void)0; }G_STMT_END
#define panel_return_val_if_fail(expr,val) G_STMT_START{ (void)0; }G_STMT_END
#endif

/* canonical representation for static strings */
#ifndef I_
#define I_(string) (g_intern_static_string ((string)))
#endif

/* this is defined in glib 2.13.0 */
#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

/* visibility support */
#ifdef HAVE_GNUC_VISIBILITY
#define PANEL_SYMBOL_EXPORT __attribute__ ((visibility("default")))
#else
#define PANEL_SYMBOL_EXPORT
#endif

/* make api compatible with 4.4 panel */
#ifndef XFCE_DISABLE_DEPRECATED

/* panel plugin functio for the id, probably not used by anyone */
#define xfce_panel_plugin_get_id(plugin) (g_strdup_printf ("%d", \
    xfce_panel_plugin_get_unique_id (XFCE_PANEL_PLUGIN (plugin))))

#define xfce_panel_plugin_set_panel_hidden(plugin,hidden) \
    xfce_panel_plugin_block_autohide (plugin, !hidden)

/* convenience functions (deprecated) */
#define xfce_create_panel_button        xfce_panel_create_button
#define xfce_create_panel_toggle_button xfce_panel_create_toggle_button
#define xfce_allow_panel_customization  xfce_panel_allow_customization

/* register definitions (deprecated) */
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL            XFCE_PANEL_PLUGIN_REGISTER
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL            XFCE_PANEL_PLUGIN_REGISTER
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK

/* parameter flags (deprecated) */
#define PANEL_PARAM_READABLE  G_PARAM_READABLE | PANEL_PARAM_STATIC_STRINGS
#define PANEL_PARAM_READWRITE G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS
#define PANEL_PARAM_WRITABLE  G_PARAM_WRITABLE | PANEL_PARAM_STATIC_STRINGS

/* slice allocator (deprecated) */
#define panel_slice_alloc(block_size)            (g_slice_alloc ((block_size)))
#define panel_slice_alloc0(block_size)           (g_slice_alloc0 ((block_size)))
#define panel_slice_free1(block_size, mem_block) G_STMT_START{ g_slice_free1 ((block_size), (mem_block)); }G_STMT_END
#define panel_slice_new(type)                    (g_slice_new (type))
#define panel_slice_new0(type)                   (g_slice_new0 (type))
#define panel_slice_free(type, ptr)              G_STMT_START{ g_slice_free (type, (ptr)); }G_STMT_END

/* debug macros (deprecated) */
#define _panel_assert(expr)                  panel_assert (expr)
#define _panel_assert_not_reached()          panel_assert_not_reached ()
#define _panel_return_if_fail(expr)          panel_return_if_fail (expr)
#define _panel_return_val_if_fail(expr, val) panel_return_val_if_fail (expr, (val))

#endif /* !XFCE_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
