/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_PRIVATE_H__
#define __PANEL_PRIVATE_H__

#include <gtk/gtk.h>
#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#define WINDOWING_IS_X11() GDK_IS_X11_DISPLAY (gdk_display_get_default ())
#else
#define WINDOWING_IS_X11() FALSE
#endif
#ifdef ENABLE_WAYLAND
#include <gdk/gdkwayland.h>
#define WINDOWING_IS_WAYLAND() GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ())
#else
#define WINDOWING_IS_WAYLAND() FALSE
#endif

/* support macros for debugging (improved macro for better position indication) */
#define panel_assert(expr) g_assert (expr)
#define panel_assert_not_reached() g_assert_not_reached ()
#define panel_return_if_fail(expr) \
  G_STMT_START \
  { \
    if (G_UNLIKELY (!(expr))) \
      { \
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
               "%s (%s): expression '%s' failed.", G_STRLOC, G_STRFUNC, \
               #expr); \
        return; \
      }; \
  } \
  G_STMT_END
#define panel_return_val_if_fail(expr, val) \
  G_STMT_START \
  { \
    if (G_UNLIKELY (!(expr))) \
      { \
        g_log (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, \
               "%s (%s): expression '%s' failed.", G_STRLOC, G_STRFUNC, \
               #expr); \
        return (val); \
      }; \
  } \
  G_STMT_END

/* handling flags */
#define PANEL_SET_FLAG(flags, flag) \
  G_STMT_START { ((flags) |= (flag)); } \
  G_STMT_END
#define PANEL_UNSET_FLAG(flags, flag) \
  G_STMT_START { ((flags) &= ~(flag)); } \
  G_STMT_END
#define PANEL_HAS_FLAG(flags, flag) (((flags) & (flag)) != 0)

/* relative path to the plugin directory */
#define PANEL_PLUGIN_RELATIVE_PATH "xfce4" G_DIR_SEPARATOR_S "panel"

/* relative plugin's rc filename (printf format) */
#define PANEL_PLUGIN_RC_RELATIVE_PATH PANEL_PLUGIN_RELATIVE_PATH G_DIR_SEPARATOR_S "%s-%d.rc"

/* xfconf properties for panels and plugins */
#define PANELS_PROPERTY_PREFIX "/panels"
#define PANELS_PROPERTY_BASE PANELS_PROPERTY_PREFIX "/panel-%d"
#define PLUGINS_PROPERTY_PREFIX "/plugins"
#define PLUGINS_PROPERTY_BASE PLUGINS_PROPERTY_PREFIX "/plugin-%d"
#define PLUGIN_IDS_PROPERTY_BASE PANELS_PROPERTY_BASE "/plugin-ids"

/* minimum time in seconds between automatic restarts of panel plugins
 * without asking the user what to do */
#define PANEL_PLUGIN_AUTO_RESTART (60)

#define OFFSCREEN (-9999)

/* integer swap functions */
#define SWAP_INTEGER(a, b) \
  G_STMT_START \
  { \
    gint swp = a; \
    a = b; \
    b = swp; \
  } \
  G_STMT_END
#define TRANSPOSE_AREA(area) \
  G_STMT_START \
  { \
    SWAP_INTEGER (area.width, area.height); \
    SWAP_INTEGER (area.x, area.y); \
  } \
  G_STMT_END

/* quick GList and GSList counting without traversing */
#define LIST_HAS_ONE_ENTRY(l) ((l) != NULL && (l)->next == NULL)
#define LIST_HAS_ONE_OR_NO_ENTRIES(l) ((l) == NULL || (l)->next == NULL)
#define LIST_HAS_TWO_OR_MORE_ENTRIES(l) ((l) != NULL && (l)->next != NULL)

/* group deprecations we don't want to replace */
static inline void
panel_image_menu_item_set_image (GtkWidget *image_menu_item,
                                 GtkWidget *image)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (image_menu_item), image);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline GtkWidget *
panel_image_menu_item_new (void)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gtk_image_menu_item_new ();
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline GtkWidget *
panel_image_menu_item_new_with_label (const gchar *label)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gtk_image_menu_item_new_with_label (label);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline GtkWidget *
panel_image_menu_item_new_with_mnemonic (const gchar *label)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gtk_image_menu_item_new_with_mnemonic (label);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline gint
panel_screen_get_number (GdkScreen *screen)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gdk_screen_get_number (screen);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline gint
panel_screen_get_width (GdkScreen *screen)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gdk_screen_get_width (screen);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline gint
panel_screen_get_height (GdkScreen *screen)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return gdk_screen_get_height (screen);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

#endif /* !__PANEL_PRIVATE_H__ */
