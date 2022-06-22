/*
 * Copyright (C) 2022 Christian Henz <chrhenz@gmx.de>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gio/gio.h>

#include <libxfce4util/libxfce4util.h>

#include "clock-sleep-monitor.h"



/* Configuration:
 *
 * - Compile time checks for runtime detection mechanisms.
 *   Currently only D-BUS is used, which is implied.
 *
 * - Compile time checks for available systems.
 *   Currently none, but could check for libraries, target OS.
 */

#define SLEEP_MONITOR_USE_LOGIND 1



/* Base class for sleep monitor implemenations
 *
 * This is deliberately not using GType/GObject at the monent:
 *
 * - These are private types used only as an implementation
 *   detail. They don't need to be exposed in the type system. It is
 *   also unclear how they *should* be exposed in the context of the
 *   plugin system.
 * - No need (at the moment) for GObject features. Implementations are
 *   singletons with a single owner - no need to class/instance
 *   support, refcounting.
 * - There is only a single implementation at the monent, so keep it
 *   simple for now and reevaluate in the context of future additions.
 */

typedef struct _ClockSleepMonitorImpl ClockSleepMonitorImpl;

struct _ClockSleepMonitorImpl
{
  void (*destroy)(ClockSleepMonitorImpl*);
};

static void clock_sleep_monitor_impl_destroy(ClockSleepMonitorImpl* impl)
{
  impl->destroy(impl);
}



/* Public sleep monitor class
 *
 * This is a GObject-based class that wraps the implementation and
 * provides the signal `woke-up()` that is used to notify the timeouts
 * of wakeup events.
 */

static guint clock_sleep_monitor_woke_up_signal = 0;

struct _ClockSleepMonitor
{
  GObject parent_instance;
  ClockSleepMonitorImpl *impl;
};

struct _ClockSleepMonitorClass
{
  GObjectClass parent_class;
};

typedef struct _ClockSleepMonitorClass ClockSleepMonitorClass;

XFCE_PANEL_DEFINE_TYPE(ClockSleepMonitor, clock_sleep_monitor, G_TYPE_OBJECT)

static void clock_sleep_monitor_finalize(GObject *object);

static void clock_sleep_monitor_class_init(ClockSleepMonitorClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = clock_sleep_monitor_finalize;

  clock_sleep_monitor_woke_up_signal =
    g_signal_new(
      g_intern_static_string("woke-up"),
      G_TYPE_FROM_CLASS(gobject_class),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

static void clock_sleep_monitor_init(ClockSleepMonitor *monitor)
{
}

static void clock_sleep_monitor_finalize(GObject *object)
{
  ClockSleepMonitor *monitor = XFCE_CLOCK_SLEEP_MONITOR(object);

  g_return_if_fail(monitor != NULL);

  if (monitor->impl != NULL) {
    clock_sleep_monitor_impl_destroy(monitor->impl);
  }

  G_OBJECT_CLASS(clock_sleep_monitor_parent_class)->finalize(object);
}



/* Logind-based implementation */

#ifdef SLEEP_MONITOR_USE_LOGIND

struct _ClockSleepMonitorImplLogind
{
  ClockSleepMonitorImpl parent;
  GDBusProxy *logind_proxy;
  guint logind_signal_id;
};

typedef struct _ClockSleepMonitorImplLogind ClockSleepMonitorImplLogind;

static void clock_sleep_monitor_impl_logind_destroy(ClockSleepMonitorImpl *impl_base);

static void on_logind_signal(
    GDBusProxy *proxy,
    gchar *sender_name,
    gchar *signal_name,
    GVariant *parameters,
    ClockSleepMonitor *monitor)
{
  const gchar *format_string = "(b)";
  gboolean going_to_sleep;

  if (strcmp(signal_name, "PrepareForSleep") != 0) {
    return;
  }

  if (!g_variant_check_format_string(parameters, format_string, FALSE)) {
    g_error("unexpected format string: %s", g_variant_get_type_string(parameters));
    return;
  }

  g_variant_get(parameters, format_string, &going_to_sleep);

  if (!going_to_sleep) {
    g_signal_emit(G_OBJECT(monitor), clock_sleep_monitor_woke_up_signal, 0);
  }
}

static ClockSleepMonitorImpl* clock_sleep_monitor_impl_logind_create(ClockSleepMonitor *monitor)
{
  ClockSleepMonitorImplLogind *impl;

  g_info("trying to instantiate logind sleep monitor");

  impl = g_malloc0(sizeof(ClockSleepMonitorImplLogind));
  g_return_val_if_fail(impl != NULL, NULL);

  ((ClockSleepMonitorImpl*) impl)->destroy = clock_sleep_monitor_impl_logind_destroy;

  impl->logind_proxy = g_dbus_proxy_new_for_bus_sync(
      G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      "org.freedesktop.login1",
      "/org/freedesktop/login1",
      "org.freedesktop.login1.Manager",
      NULL,
      NULL);
  if (impl->logind_proxy == NULL) {
    g_info("could not get proxy for org.freedesktop.login1");
    clock_sleep_monitor_impl_logind_destroy((ClockSleepMonitorImpl*) impl);
    return NULL;
  }

  impl->logind_signal_id = g_signal_connect(impl->logind_proxy, "g-signal", G_CALLBACK(on_logind_signal), monitor);
  if (impl->logind_signal_id == 0) {
    g_error("could not connect to logind signal");
    clock_sleep_monitor_impl_logind_destroy((ClockSleepMonitorImpl*) impl);
    return NULL;
  }

  return (ClockSleepMonitorImpl*) impl;
}

static void clock_sleep_monitor_impl_logind_destroy(ClockSleepMonitorImpl *impl_base)
{
  ClockSleepMonitorImplLogind *impl = (ClockSleepMonitorImplLogind*) impl_base;

  if (impl->logind_proxy != NULL) {
    if (impl->logind_signal_id != 0) {
      g_signal_handler_disconnect(impl->logind_proxy, impl->logind_signal_id);
    }
    g_object_unref(G_OBJECT(impl->logind_proxy));
  }

  g_free(impl);
}

#endif /* defined SLEEP_MONITOR_USE_LOGIND */



/* Implementation factory registration
 *
 * Collect available implementations in a reasonable order.
 */

typedef ClockSleepMonitorImpl* (*SleepMonitorImplFactory)(ClockSleepMonitor*);

static SleepMonitorImplFactory sleep_monitor_impl_factories[] = {
  #ifdef SLEEP_MONITOR_USE_LOGIND
  clock_sleep_monitor_impl_logind_create,
  #endif
  NULL
};

ClockSleepMonitor *clock_sleep_monitor_create(void)
{
  SleepMonitorImplFactory *factory_ptr = &sleep_monitor_impl_factories[0];
  ClockSleepMonitorImpl *impl = NULL;
  ClockSleepMonitor *monitor;

  monitor = g_object_new(XFCE_TYPE_CLOCK_SLEEP_MONITOR, NULL);
  if (monitor == NULL) {
    g_error("could not create sleep monitor");
    return NULL;
  }

  for (; impl == NULL && *factory_ptr != NULL; factory_ptr++) {
    impl = (*factory_ptr)(monitor);
  }

  if (impl == NULL) {
    if (sleep_monitor_impl_factories[0] != NULL) {
      g_warning("could not instantiate a sleep monitor");
    }
    g_object_unref(monitor);
    return NULL;
  }

  monitor->impl = impl;

  return monitor;
}
