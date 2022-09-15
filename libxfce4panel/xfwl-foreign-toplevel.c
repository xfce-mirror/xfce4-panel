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
#include "xfwl-foreign-toplevel.h"
#include "protocols/wlr-foreign-toplevel-management-unstable-v1-client.h"
#include "libxfce4panel-enum-types.h"

/**
 * SECTION: xfwl-foreign-toplevel
 * @title: XfwlForeignToplevel
 * @short_description: GObject wrapper around `struct zwlr_foreign_toplevel_handle_v1`
 * @include: libxfce4panel/libxfce4panel.h
 *
 * See #XfwlForeignToplevelManager to know how to obtain a #XfwlForeignToplevel.
 **/

static void         xfwl_foreign_toplevel_get_property     (GObject                                *object,
                                                            guint                                   prop_id,
                                                            GValue                                 *value,
                                                            GParamSpec                             *pspec);
static void         xfwl_foreign_toplevel_set_property     (GObject                                *object,
                                                            guint                                   prop_id,
                                                            const GValue                           *value,
                                                            GParamSpec                             *pspec);
static void         xfwl_foreign_toplevel_constructed      (GObject                                *object);
static void         xfwl_foreign_toplevel_finalize         (GObject                                *object);

static void         xfwl_foreign_toplevel_title            (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            const char                             *title);
static void         xfwl_foreign_toplevel_app_id           (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            const char                             *app_id);
static void         xfwl_foreign_toplevel_output_enter     (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            struct wl_output                       *output);
static void         xfwl_foreign_toplevel_output_leave     (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            struct wl_output                       *output);
static void         xfwl_foreign_toplevel_state            (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            struct wl_array                        *states);
static void         xfwl_foreign_toplevel_done             (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void         xfwl_foreign_toplevel_closed           (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel);
static void         xfwl_foreign_toplevel_parent           (void                                   *data,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                                            struct zwlr_foreign_toplevel_handle_v1 *wl_parent);



enum
{
  PROP_0,
  PROP_WL_TOPLEVEL,
  PROP_TITLE,
  PROP_APP_ID,
  PROP_MONITORS,
  PROP_STATE,
  PROP_PARENT,
  N_PROPERTIES
};

enum
{
  CLOSED,
  N_SIGNALS
};

/**
 * XfwlForeignToplevel:
 *
 * An opaque structure wrapping a
 * [`struct zwlr_foreign_toplevel_handle_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1#zwlr_foreign_toplevel_handle_v1).
 **/
struct _XfwlForeignToplevel
{
  GObject __parent__;

  struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel;
  gchar *title, *app_id;
  GList *monitors;
  XfwlForeignToplevelState state;
  XfwlForeignToplevel *parent;
};

static guint toplevel_signals[N_SIGNALS];
static GParamSpec *toplevel_props[N_PROPERTIES] = { NULL, };

static const struct zwlr_foreign_toplevel_handle_v1_listener wl_toplevel_listener =
{
  xfwl_foreign_toplevel_title,
  xfwl_foreign_toplevel_app_id,
  xfwl_foreign_toplevel_output_enter,
  xfwl_foreign_toplevel_output_leave,
  xfwl_foreign_toplevel_state,
  xfwl_foreign_toplevel_done ,
  xfwl_foreign_toplevel_closed,
  xfwl_foreign_toplevel_parent,
};



G_DEFINE_TYPE (XfwlForeignToplevel, xfwl_foreign_toplevel, G_TYPE_OBJECT)



static void
xfwl_foreign_toplevel_class_init (XfwlForeignToplevelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = xfwl_foreign_toplevel_get_property;
  gobject_class->set_property = xfwl_foreign_toplevel_set_property;
  gobject_class->constructed = xfwl_foreign_toplevel_constructed;
  gobject_class->finalize = xfwl_foreign_toplevel_finalize;

  toplevel_props[PROP_WL_TOPLEVEL] =
    g_param_spec_pointer ("wl-toplevel", "Wayland toplevel", "The underlying Wayland object",
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

  toplevel_props[PROP_TITLE] =
    g_param_spec_string ("title", "Title", "The toplevel title", NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  toplevel_props[PROP_APP_ID] =
    g_param_spec_string ("app-id", "Application ID", "The toplevel application ID", NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  toplevel_props[PROP_MONITORS] =
    g_param_spec_pointer ("monitors", "Monitors",
                          "The list of monitors on which the toplevel appears",
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  toplevel_props[PROP_STATE] =
    g_param_spec_flags ("state", "State", "The toplevel state",
                        XFWL_TYPE_FOREIGN_TOPLEVEL_STATE, XFWL_FOREIGN_TOPLEVEL_STATE_NONE,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  toplevel_props[PROP_PARENT] =
    g_param_spec_object ("parent", "Parent", "The parent toplevel", XFWL_TYPE_FOREIGN_TOPLEVEL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, toplevel_props);

  /**
   * XfwlForeignToplevel::closed:
   * @toplevel: an #XfwlForeignToplevel
   *
   * This signal means the actual toplevel has been destroyed. It is guaranteed there
   * won't be any more signals from @toplevel, and any requests will be ignored. When
   * all handlers connected to this signal have been called, @toplevel is released.
   **/
  toplevel_signals[CLOSED] =
    g_signal_new (g_intern_static_string ("closed"), G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}



static void
xfwl_foreign_toplevel_init (XfwlForeignToplevel *toplevel)
{
}



static void
xfwl_foreign_toplevel_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  XfwlForeignToplevel *toplevel = XFWL_FOREIGN_TOPLEVEL (object);

  switch (prop_id)
    {
    case PROP_WL_TOPLEVEL:
      g_value_set_pointer (value, toplevel->wl_toplevel);
      break;

    case PROP_TITLE:
      g_value_set_string (value, toplevel->title);
      break;

    case PROP_APP_ID:
      g_value_set_string (value, toplevel->app_id);
      break;

    case PROP_MONITORS:
      g_value_set_pointer (value, toplevel->monitors);
      break;

    case PROP_STATE:
      g_value_set_flags (value, toplevel->state);
      break;

    case PROP_PARENT:
      g_value_set_object (value, toplevel->parent);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfwl_foreign_toplevel_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  XfwlForeignToplevel *toplevel = XFWL_FOREIGN_TOPLEVEL (object);

  switch (prop_id)
    {
    case PROP_WL_TOPLEVEL:
      toplevel->wl_toplevel = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfwl_foreign_toplevel_constructed (GObject *object)
{
  XfwlForeignToplevel *toplevel = XFWL_FOREIGN_TOPLEVEL (object);

  /* set properties */
  zwlr_foreign_toplevel_handle_v1_add_listener (toplevel->wl_toplevel, &wl_toplevel_listener, toplevel);
  wl_display_roundtrip (gdk_wayland_display_get_wl_display (gdk_display_get_default ()));

  G_OBJECT_CLASS (xfwl_foreign_toplevel_parent_class)->constructed (object);
}



static void
xfwl_foreign_toplevel_finalize (GObject *object)
{
  XfwlForeignToplevel *toplevel = XFWL_FOREIGN_TOPLEVEL (object);

  g_free (toplevel->title);
  g_free (toplevel->app_id);
  g_list_free (toplevel->monitors);
  if (toplevel->parent != NULL)
    g_object_unref (toplevel->parent);

  G_OBJECT_CLASS (xfwl_foreign_toplevel_parent_class)->finalize (object);
}



static void
xfwl_foreign_toplevel_title (void *data,
                             struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                             const char *title)
{
  XfwlForeignToplevel *toplevel = data;

  g_free (toplevel->title);
  toplevel->title = g_strdup (title);

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_TITLE]);
}



static void
xfwl_foreign_toplevel_app_id (void *data,
                              struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                              const char *app_id)
{
  XfwlForeignToplevel *toplevel = data;

  g_free (toplevel->app_id);
  toplevel->app_id = g_strdup (app_id);

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_APP_ID]);
}



static void
xfwl_foreign_toplevel_output_enter (void *data,
                                    struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                    struct wl_output *output)
{
  XfwlForeignToplevel *toplevel = data;
  GdkDisplay *display;
  GdkMonitor *monitor;
  gint n_monitors;

  display = gdk_display_get_default ();
  n_monitors = gdk_display_get_n_monitors (display);
  for (gint n = 0; n < n_monitors; n++)
    {
      monitor = gdk_display_get_monitor (display, n);
      if (output == gdk_wayland_monitor_get_wl_output (monitor))
        {
          toplevel->monitors = g_list_prepend (toplevel->monitors, monitor);
          break;
        }
    }

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_MONITORS]);
}



static void
xfwl_foreign_toplevel_output_leave (void *data,
                                    struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                                    struct wl_output *output)
{
  XfwlForeignToplevel *toplevel = data;
  GdkDisplay *display;
  GdkMonitor *monitor;
  gint n_monitors;

  display = gdk_display_get_default ();
  n_monitors = gdk_display_get_n_monitors (display);
  for (gint n = 0; n < n_monitors; n++)
    {
      monitor = gdk_display_get_monitor (display, n);
      if (output == gdk_wayland_monitor_get_wl_output (monitor))
        {
          toplevel->monitors = g_list_remove (toplevel->monitors, monitor);
          break;
        }
    }

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_MONITORS]);
}



static void
xfwl_foreign_toplevel_state (void *data,
                             struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                             struct wl_array *states)
{
  XfwlForeignToplevel *toplevel = data;
  enum zwlr_foreign_toplevel_handle_v1_state *state;

  toplevel->state = XFWL_FOREIGN_TOPLEVEL_STATE_NONE;
  wl_array_for_each (state, states)
    toplevel->state |= 1 << *state;

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_STATE]);
}



static void
xfwl_foreign_toplevel_done (void *data,
                            struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel)
{
}



static void
xfwl_foreign_toplevel_closed (void *data,
                              struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel)
{
  g_signal_emit (data, toplevel_signals[CLOSED], 0);
}



static void
xfwl_foreign_toplevel_parent (void *data,
                              struct zwlr_foreign_toplevel_handle_v1 *wl_toplevel,
                              struct zwlr_foreign_toplevel_handle_v1 *wl_parent)
{
  XfwlForeignToplevel *toplevel = data;

  if (toplevel->parent != NULL)
    {
      g_object_unref (toplevel->parent);
      toplevel->parent = NULL;
    }

  if (wl_parent != NULL)
    toplevel->parent = g_object_new (XFWL_TYPE_FOREIGN_TOPLEVEL, "wl-toplevel", wl_parent, NULL);

  g_object_notify_by_pspec (G_OBJECT (toplevel), toplevel_props[PROP_PARENT]);
}



/**
 * xfwl_foreign_toplevel_get_wl_toplevel:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: (transfer none): The underlying
 * [`struct zwlr_foreign_toplevel_handle_v1`](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1#zwlr_foreign_toplevel_handle_v1)
 * Wayland object.
 **/
struct zwlr_foreign_toplevel_handle_v1 *
xfwl_foreign_toplevel_get_wl_toplevel (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), NULL);

  return toplevel->wl_toplevel;
}



/**
 * xfwl_foreign_toplevel_get_title:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: (transfer none) (nullable): The toplevel title or %NULL if there is none.
 **/
const gchar *
xfwl_foreign_toplevel_get_title (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), NULL);

  return toplevel->title;
}



/**
 * xfwl_foreign_toplevel_get_app_id:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: (transfer none) (nullable): The toplevel application ID or %NULL if there
 * is none.
 **/
const gchar *
xfwl_foreign_toplevel_get_app_id (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), NULL);

  return toplevel->app_id;
}



/**
 * xfwl_foreign_toplevel_get_monitors:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: (element-type GdkMonitor) (transfer none): The list of monitors on which
 * the toplevel appears.
 **/
GList *
xfwl_foreign_toplevel_get_monitors (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), NULL);

  return toplevel->monitors;
}



/**
 * xfwl_foreign_toplevel_get_state:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: The toplevel state.
 **/
XfwlForeignToplevelState
xfwl_foreign_toplevel_get_state (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), XFWL_FOREIGN_TOPLEVEL_STATE_NONE);

  return toplevel->state;
}



/**
 * xfwl_foreign_toplevel_get_parent:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Returns: (transfer none) (nullable): The parent toplevel or %NULL if there is none.
 **/
XfwlForeignToplevel *
xfwl_foreign_toplevel_get_parent (XfwlForeignToplevel *toplevel)
{
  g_return_val_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel), NULL);

  return toplevel->parent;
}



/**
 * xfwl_foreign_toplevel_maximize:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests that the toplevel be maximized. If the maximized state actually changes,
 * this will be indicated by #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_maximize (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_set_maximized (toplevel->wl_toplevel);
}



/**
 * xfwl_foreign_toplevel_unmaximize:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests that the toplevel be unmaximized. If the maximized state actually changes,
 * this will be indicated by #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_unmaximize (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_unset_maximized (toplevel->wl_toplevel);
}



/**
 * xfwl_foreign_toplevel_minimize:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests that the toplevel be minimized. If the minimized state actually changes,
 * this will be indicated by #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_minimize (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_set_minimized (toplevel->wl_toplevel);
}



/**
 * xfwl_foreign_toplevel_unminimize:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests that the toplevel be unminimized. If the minimized state actually changes,
 * this will be indicated by #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_unminimize (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_unset_minimized (toplevel->wl_toplevel);
}



/**
 * xfwl_foreign_toplevel_activate:
 * @toplevel: an #XfwlForeignToplevel
 * @seat: the seat on which @toplevel should be activated
 *
 * Requests that the toplevel be activated on the given seat. There is no guarantee
 * the toplevel will actually be activated. If so, this will be indicated by
 * #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_activate (XfwlForeignToplevel *toplevel,
                                GdkSeat *seat)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));
  g_return_if_fail (GDK_IS_SEAT (seat));

  zwlr_foreign_toplevel_handle_v1_activate (toplevel->wl_toplevel,
                                            gdk_wayland_seat_get_wl_seat (seat));
}



/**
 * xfwl_foreign_toplevel_close:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests the toplevel to close itself. There is no guarantee the toplevel will
 * actually be destroyed. If so, this will be indicated by #XfwlForeignToplevel::closed.
 **/
void
xfwl_foreign_toplevel_close (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_close (toplevel->wl_toplevel);
}



/**
 * xfwl_foreign_toplevel_set_rectangle:
 * @toplevel: an #XfwlForeignToplevel
 * @window: the window to which the coordinates in @rectangle are relative
 * @rectangle: the place where the calling app represents @toplevel, e.g. a window
 * button in a taskbar
 *
 * @rectangle corresponds to the place where the calling app represents @toplevel,
 * e.g. a window button in a taskbar. It can be used by the compositor as a hint for
 * some operations, e.g minimizing. It is however not required to set this, in which
 * case the compositor is free to decide some default value.
 *
 * If more than one rectangle is specified, only the last one is considered. Setting
 * `rectangle->width = rectangle->height = 0` removes the already-set rectangle.
 **/
void
xfwl_foreign_toplevel_set_rectangle (XfwlForeignToplevel *toplevel,
                                     GdkWindow *window,
                                     const GdkRectangle *rectangle)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));
  g_return_if_fail (GDK_IS_WINDOW (window));

  zwlr_foreign_toplevel_handle_v1_set_rectangle (toplevel->wl_toplevel,
                                                 gdk_wayland_window_get_wl_surface (window),
                                                 rectangle->x, rectangle->y,
                                                 rectangle->width, rectangle->height);
}



/**
 * xfwl_foreign_toplevel_fullscreen:
 * @toplevel: an #XfwlForeignToplevel
 * @monitor: (nullable): the monitor on which @toplevel should be fullscreened or %NULL
 * to let the compositor decide
 *
 * Requests that the toplevel be fullscreened on the given monitor. If the fullscreen
 * state and/or the monitors the toplevel is visible on actually change, this will be
 * indicated by the #XfwlForeignToplevel:state and #XfwlForeignToplevel:monitors.
 **/
void
xfwl_foreign_toplevel_fullscreen (XfwlForeignToplevel *toplevel,
                                  GdkMonitor *monitor)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));
  g_return_if_fail (monitor == NULL || GDK_IS_MONITOR (monitor));

  zwlr_foreign_toplevel_handle_v1_set_fullscreen (toplevel->wl_toplevel,
    monitor == NULL ? NULL : gdk_wayland_monitor_get_wl_output (monitor));
}



/**
 * xfwl_foreign_toplevel_unfullscreen:
 * @toplevel: an #XfwlForeignToplevel
 *
 * Requests that the toplevel be unfullscreened. If the fullscreen state actually changes,
 * this will be indicated by #XfwlForeignToplevel:state.
 **/
void
xfwl_foreign_toplevel_unfullscreen (XfwlForeignToplevel *toplevel)
{
  g_return_if_fail (XFWL_IS_FOREIGN_TOPLEVEL (toplevel));

  zwlr_foreign_toplevel_handle_v1_unset_fullscreen (toplevel->wl_toplevel);
}
