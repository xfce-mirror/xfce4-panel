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
#define panel_return_if_fail(expr)         G_STMT_START { \
  if (G_UNLIKELY (!(expr))) \
    { \
      g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
             "%s (%s): expression '%s' failed.", G_STRLOC, G_STRFUNC, \
             #expr); \
      return; \
    }; }G_STMT_END
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

/* xfconf channel for plugins */
#define XFCE_PANEL_PLUGIN_CHANNEL_NAME ("xfce4-panel")

/* macro for opening an XfconfChannel for plugin */
#define xfce_panel_plugin_xfconf_channel_new(plugin) \
  xfconf_channel_new_with_property_base (XFCE_PANEL_PLUGIN_CHANNEL_NAME, \
    xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (plugin)));

/* this is defined in glib 2.13.0 */
#ifndef G_PARAM_STATIC_STRINGS
#define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
