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
#include "xfwl-foreign-toplevel-manager.h"
#include "xfwl-core-utils.h"
#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"

/**
 * SECTION: xfwl-foreign-toplevel-manager
 * @title: XfwlForeignToplevelManager
 * @short_description: GObject wrapper around `struct zwlr_foreign_toplevel_manager_v1`
 * @include: libxfce4panel/libxfce4panel.h
 *
 * Wrapping a
 * [`struct zwlr_foreign_toplevel_manager_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1#zwlr_foreign_toplevel_manager_v1)
 * in a #GObject allows to allocate only one toplevel manager and to cache its resources
 * only once, offering at least the usual getters to access them.
 **/

static void         xfwl_foreign_toplevel_manager_get_property       (GObject                                 *object,
                                                                      guint                                    prop_id,
                                                                      GValue                                  *value,
                                                                      GParamSpec                              *pspec);
static void         xfwl_foreign_toplevel_manager_set_property       (GObject                                 *object,
                                                                      guint                                    prop_id,
                                                                      const GValue                            *value,
                                                                      GParamSpec                              *pspec);
static void         xfwl_foreign_toplevel_manager_constructed        (GObject                                 *object);
static void         xfwl_foreign_toplevel_manager_finalize           (GObject                                 *object);

static void         xfwl_foreign_toplevel_manager_toplevel           (void                                    *data,
                                                                      struct zwlr_foreign_toplevel_manager_v1 *wl_manager,
                                                                      struct zwlr_foreign_toplevel_handle_v1  *wl_toplevel);
static void         xfwl_foreign_toplevel_manager_finished           (void                                    *data,
                                                                      struct zwlr_foreign_toplevel_manager_v1 *wl_manager);
static void         xfwl_foreign_toplevel_manager_stop               (gpointer                                 data,
                                                                      GObject                                 *object,
                                                                      gboolean                                 is_last_ref);



enum
{
  PROP_0,
  PROP_WL_MANAGER,
  PROP_TOPLEVELS,
  PROP_ACTIVE,
  N_PROPERTIES
};

enum
{
  ADDED,
  REMOVED,
  N_SIGNALS
};

/**
 * XfwlForeignToplevelManager:
 *
 * An opaque structure wrapping a
 * [`struct zwlr_foreign_toplevel_manager_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1#zwlr_foreign_toplevel_manager_v1).
 **/
struct _XfwlForeignToplevelManager
{
  GObject __parent__;

  struct zwlr_foreign_toplevel_manager_v1 *wl_manager;
  GHashTable *toplevels;
  XfwlForeignToplevel *active;
};

static guint manager_signals[N_SIGNALS];
static GParamSpec *manager_props[N_PROPERTIES] = { NULL, };
static XfwlForeignToplevelManager *global_manager = NULL;

static const struct zwlr_foreign_toplevel_manager_v1_listener wl_manager_listener =
{
  xfwl_foreign_toplevel_manager_toplevel,
  xfwl_foreign_toplevel_manager_finished
};



G_DEFINE_TYPE (XfwlForeignToplevelManager, xfwl_foreign_toplevel_manager, G_TYPE_OBJECT)



static void
xfwl_foreign_toplevel_manager_class_init (XfwlForeignToplevelManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xfwl_foreign_toplevel_manager_get_property;
  gobject_class->set_property = xfwl_foreign_toplevel_manager_set_property;
  gobject_class->constructed = xfwl_foreign_toplevel_manager_constructed;
  gobject_class->finalize = xfwl_foreign_toplevel_manager_finalize;

  manager_props[PROP_WL_MANAGER] =
    g_param_spec_pointer ("wl-manager", "Wayland manager", "The underlying Wayland object",
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  manager_props[PROP_TOPLEVELS] =
    g_param_spec_pointer ("toplevels", "Toplevels", "The toplevel list",
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  manager_props[PROP_ACTIVE] =
    g_param_spec_object ("active", "Active", "The active toplevel", XFWL_TYPE_FOREIGN_TOPLEVEL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, manager_props);

  /**
   * XfwlForeignToplevelManager::added:
   * @manager: an #XfwlForeignToplevelManager
   * @toplevel: the added toplevel
   *
   * The signal is emitted after the toplevel has been added to the list.
   **/
  manager_signals[ADDED] =
    g_signal_new (g_intern_static_string ("added"), G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, XFWL_TYPE_FOREIGN_TOPLEVEL);

  /**
   * XfwlForeignToplevelManager::removed:
   * @manager: an #XfwlForeignToplevelManager
   * @toplevel: the removed toplevel
   *
   * The signal is emitted before the toplevel is removed from the list.
   **/
  manager_signals[REMOVED] =
    g_signal_new (g_intern_static_string ("removed"), G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, XFWL_TYPE_FOREIGN_TOPLEVEL);
}



static void
xfwl_foreign_toplevel_manager_init (XfwlForeignToplevelManager *manager)
{
}



static void
xfwl_foreign_toplevel_manager_get_property (GObject *object,
                                            guint prop_id,
                                            GValue *value,
                                            GParamSpec *pspec)
{
  XfwlForeignToplevelManager *manager = XFWL_FOREIGN_TOPLEVEL_MANAGER (object);

  switch (prop_id)
    {
    case PROP_WL_MANAGER:
      g_value_set_pointer (value, manager->wl_manager);
      break;

    case PROP_TOPLEVELS:
      g_value_set_pointer (value, g_hash_table_get_values (manager->toplevels));
      break;

    case PROP_ACTIVE:
      g_value_set_object (value, manager->active);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfwl_foreign_toplevel_manager_set_property (GObject *object,
                                            guint prop_id,
                                            const GValue *value,
                                            GParamSpec *pspec)
{
  XfwlForeignToplevelManager *manager = XFWL_FOREIGN_TOPLEVEL_MANAGER (object);

  switch (prop_id)
    {
    case PROP_WL_MANAGER:
      manager->wl_manager = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfwl_foreign_toplevel_manager_constructed (GObject *object)
{
  XfwlForeignToplevelManager *manager = XFWL_FOREIGN_TOPLEVEL_MANAGER (object);

  manager->toplevels =
    g_hash_table_new_full (g_direct_hash, g_direct_equal,
                           (GDestroyNotify) zwlr_foreign_toplevel_handle_v1_destroy,
                           g_object_unref);

  /* to correctly manage the asynchronous release of the manager */
  g_object_add_toggle_ref (object, xfwl_foreign_toplevel_manager_stop, manager->wl_manager);

  /* fill the hash table */
  zwlr_foreign_toplevel_manager_v1_add_listener (manager->wl_manager, &wl_manager_listener, manager);
  wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));

  G_OBJECT_CLASS (xfwl_foreign_toplevel_manager_parent_class)->constructed (object);
}



static void
xfwl_foreign_toplevel_manager_finalize (GObject *object)
{
  XfwlForeignToplevelManager *manager = XFWL_FOREIGN_TOPLEVEL_MANAGER (object);

  g_hash_table_destroy (manager->toplevels);
  zwlr_foreign_toplevel_manager_v1_destroy (manager->wl_manager);

  G_OBJECT_CLASS (xfwl_foreign_toplevel_manager_parent_class)->finalize (object);
}



static gboolean
xfwl_foreign_toplevel_manager_toplevel_closed_idle (gpointer data)
{
  if (global_manager == NULL)
    return FALSE;

  g_hash_table_remove (global_manager->toplevels, xfwl_foreign_toplevel_get_wl_toplevel (data));
  g_object_notify_by_pspec (G_OBJECT (global_manager), manager_props[PROP_TOPLEVELS]);

  return FALSE;
}



static void
xfwl_foreign_toplevel_manager_toplevel_closed (XfwlForeignToplevel *toplevel,
                                               XfwlForeignToplevelManager *manager)
{
  if (toplevel == manager->active)
    {
      manager->active = NULL;
      g_object_notify_by_pspec (G_OBJECT (manager), manager_props[PROP_ACTIVE]);
    }

  g_signal_emit (manager, manager_signals[REMOVED], 0, toplevel);

  /* let other handlers connected to this signal be called before releasing toplevel */
  g_idle_add (xfwl_foreign_toplevel_manager_toplevel_closed_idle, toplevel);
}



static void
xfwl_foreign_toplevel_manager_toplevel_state (XfwlForeignToplevel *toplevel,
                                              GParamSpec *pspec,
                                              XfwlForeignToplevelManager *manager)
{
  if (toplevel != manager->active
      && xfwl_foreign_toplevel_get_state (toplevel) & XFWL_FOREIGN_TOPLEVEL_STATE_ACTIVATED)
    {
      manager->active = toplevel;
      g_object_notify_by_pspec (G_OBJECT (manager), manager_props[PROP_ACTIVE]);
    }
}



static void
xfwl_foreign_toplevel_manager_toplevel (void *data,
                                        struct zwlr_foreign_toplevel_manager_v1 *wl_manager,
                                        struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel)
{
  XfwlForeignToplevelManager *manager = data;
  XfwlForeignToplevel *toplevel;

  toplevel = g_object_new (XFWL_TYPE_FOREIGN_TOPLEVEL, "wl-toplevel", wl_toplevel, NULL);
  g_signal_connect (toplevel, "closed",
                    G_CALLBACK (xfwl_foreign_toplevel_manager_toplevel_closed), manager);
  xfwl_foreign_toplevel_manager_toplevel_state (toplevel, NULL, manager);
  g_signal_connect (toplevel, "notify::state",
                    G_CALLBACK (xfwl_foreign_toplevel_manager_toplevel_state), manager);

  g_hash_table_insert (manager->toplevels, wl_toplevel, toplevel);
  g_object_notify_by_pspec (G_OBJECT (manager), manager_props[PROP_TOPLEVELS]);
  g_signal_emit (manager, manager_signals[ADDED], 0, toplevel);
}



static void
xfwl_foreign_toplevel_manager_finished (void *data,
                                        struct zwlr_foreign_toplevel_manager_v1 *wl_manager)
{
  /* triggers finalize() */
  g_object_unref (data);
}



static void
xfwl_foreign_toplevel_manager_stop (gpointer data,
                                    GObject *object,
                                    gboolean is_last_ref)
{
  if (! is_last_ref)
    return;

  /* remove this callback now to be sure not to pass here again when object is disposed */
  g_object_ref (object);
  g_object_remove_toggle_ref (object, xfwl_foreign_toplevel_manager_stop, data);

  /* this will trigger finished() later */
  zwlr_foreign_toplevel_manager_v1_stop (data);
  global_manager = NULL;
}



/**
 * xfwl_foreign_toplevel_manager_get:
 *
 * Returns: (transfer full) (nullable): A new toplevel manager, or a reference to
 * the existing one, if the
 * [`wlr_foreign_toplevel_management_unstable_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1)
 * protocol is supported. %NULL otherwise. To be released with g_object_unref() when
 * no longer used.
 **/
XfwlForeignToplevelManager *
xfwl_foreign_toplevel_manager_get (void)
{
  struct zwlr_foreign_toplevel_manager_v1 *wl_manager;

  if (global_manager != NULL)
    return g_object_ref (global_manager);

  wl_manager = xfwl_registry_bind (zwlr_foreign_toplevel_manager_v1);
  if (wl_manager == NULL)
    return NULL;

  global_manager = g_object_new (XFWL_TYPE_FOREIGN_TOPLEVEL_MANAGER,
                                 "wl-manager", wl_manager, NULL);

  return global_manager;
}



/**
 * xfwl_foreign_toplevel_manager_get_wl_manager:
 * @manager: an #XfwlForeignToplevelManager
 *
 * Returns: (transfer none): The underlying
 * [`struct zwlr_foreign_toplevel_manager_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1#zwlr_foreign_toplevel_manager_v1)
 * Wayland object.
 **/
struct zwlr_foreign_toplevel_manager_v1 *
xfwl_foreign_toplevel_manager_get_wl_manager (XfwlForeignToplevelManager *manager)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL_MANAGER (manager), NULL);

  return manager->wl_manager;
}



/**
 * xfwl_foreign_toplevel_manager_get_toplevels:
 * @manager: an #XfwlForeignToplevelManager
 *
 * Returns: (element-type XfwlForeignToplevel) (transfer container): The toplevel list,
 * to be freed with g_list_free() when no longer used.
 **/
GList *
xfwl_foreign_toplevel_manager_get_toplevels (XfwlForeignToplevelManager *manager)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL_MANAGER (manager), NULL);

  return g_hash_table_get_values (manager->toplevels);
}



/**
 * xfwl_foreign_toplevel_manager_get_active:
 * @manager: an #XfwlForeignToplevelManager
 *
 * Returns: (transfer none) (nullable): The active toplevel or %NULL if none is active.
 **/
XfwlForeignToplevel *
xfwl_foreign_toplevel_manager_get_active (XfwlForeignToplevelManager *manager)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL_MANAGER (manager), NULL);

  return manager->active;
}
