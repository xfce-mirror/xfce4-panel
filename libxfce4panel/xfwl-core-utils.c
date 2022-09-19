/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gdk/gdkwayland.h>
#include "xfwl-core-utils.h"

/**
 * SECTION: xfwl-core-utils
 * @title: Xfwl core utilities
 * @short_description: Utilities related to the core Wayland protocol
 * @include: libxfce4panel/libxfce4panel.h
 **/

static void       xfwl_registry_global             (void                    *data,
                                                    struct wl_registry      *registry,
                                                    uint32_t                 id,
                                                    const char              *interface,
                                                    uint32_t                 version);
static void       xfwl_registry_global_remove      (void                    *data,
                                                    struct wl_registry      *registry,
                                                    uint32_t                 id);



typedef struct
{
  uint32_t id;
  uint32_t version;
} WlBindingParam;

static struct wl_registry *wl_registry = NULL;
static GHashTable *wl_binding_params = NULL;

static const struct wl_registry_listener wl_registry_listener =
{
  xfwl_registry_global,
  xfwl_registry_global_remove
};



static void
xfwl_registry_global (void *data,
                      struct wl_registry *registry,
                      uint32_t id,
                      const char *interface,
                      uint32_t version)
{
  WlBindingParam *param;

  param = g_new (WlBindingParam, 1);
  param->id = id;
  param->version = version;

  g_hash_table_insert (wl_binding_params, g_strdup (interface), param);
}



static void
xfwl_registry_global_remove (void *data,
                             struct wl_registry *registry,
                             uint32_t id)
{
  WlBindingParam *param;
  GHashTableIter iter;
  gpointer value;

  g_hash_table_iter_init (&iter, wl_binding_params);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    {
      param = value;
      if (param->id == id)
        {
          g_hash_table_iter_remove (&iter);
          break;
        }
    }
}



static void
xfce_panel_wayland_finalize (void)
{
  wl_registry_destroy (wl_registry);
  g_hash_table_destroy (wl_binding_params);
  wl_registry = NULL;
  wl_binding_params = NULL;
}



static gboolean
xfce_panel_wayland_init (void)
{
  struct wl_display *wl_display;
  GdkDisplay *display;

  if (wl_registry != NULL)
    return TRUE;

  display = gdk_display_get_default ();
  if (! GDK_IS_WAYLAND_DISPLAY (display))
    return FALSE;

  wl_display = gdk_wayland_display_get_wl_display (display);
  wl_registry = wl_display_get_registry (wl_display);
  if (wl_registry == NULL)
    return FALSE;

  wl_binding_params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  wl_registry_add_listener (wl_registry, &wl_registry_listener, NULL);
  wl_display_roundtrip (wl_display);
  g_signal_connect (display, "closed", G_CALLBACK (xfce_panel_wayland_finalize), NULL);

  return TRUE;
}



gpointer
xfwl_registry_bind_real (const gchar *interface_name,
                         const struct wl_interface *interface)
{
  WlBindingParam *param;

  if (! xfce_panel_wayland_init ())
    return NULL;

  param = g_hash_table_lookup (wl_binding_params, interface_name);
  if (param != NULL)
    return wl_registry_bind (wl_registry, param->id, interface,
                             MIN ((uint32_t) interface->version, param->version));

  return NULL;
}
