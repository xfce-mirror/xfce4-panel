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



/* Base class
 *
 * Provides the signal `woke-up()` that is used to notify the timeouts
 * of wakeup events.
 */

static guint clock_sleep_monitor_woke_up_signal = 0;

struct _ClockSleepMonitor
{
  GObject parent_instance;
};

struct _ClockSleepMonitorClass
{
  GObjectClass parent_class;
};

typedef struct _ClockSleepMonitorClass ClockSleepMonitorClass;

G_DEFINE_TYPE(ClockSleepMonitor, clock_sleep_monitor, G_TYPE_OBJECT)

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
  G_OBJECT_CLASS(clock_sleep_monitor_parent_class)->finalize(object);
}



/* Logind-based implementation */

#ifdef SLEEP_MONITOR_USE_LOGIND

struct _ClockSleepMonitorLogind
{
  ClockSleepMonitor parent_instance;
  GDBusProxy *logind_proxy;
  guint logind_signal_id;
};

struct _ClockSleepMonitorLogindClass
{
  ClockSleepMonitorClass parent_class;
};

typedef struct _ClockSleepMonitorLogind ClockSleepMonitorLogind;
typedef struct _ClockSleepMonitorLogindClass ClockSleepMonitorLogindClass;

GType clock_sleep_monitor_logind_get_type(void) G_GNUC_CONST;

#define XFCE_TYPE_CLOCK_SLEEP_MONITOR_LOGIND (clock_sleep_monitor_logind_get_type())
#define XFCE_CLOCK_SLEEP_MONITOR_LOGIND(object) (G_TYPE_CHECK_INSTANCE_CAST((object), XFCE_TYPE_CLOCK_SLEEP_MONITOR_LOGIND, ClockSleepMonitorLogind))
#define XFCE_IS_CLOCK_SLEEP_MONITOR_LOGIND(object) (G_TYPE_CHECK_INSTANCE_TYPE((object), XFCE_TYPE_CLOCK_SLEEP_MONITOR_LOGIND))

G_DEFINE_TYPE(ClockSleepMonitorLogind, clock_sleep_monitor_logind, XFCE_TYPE_CLOCK_SLEEP_MONITOR)

static void clock_sleep_monitor_logind_finalize(GObject *object);

static void clock_sleep_monitor_logind_class_init(ClockSleepMonitorLogindClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = clock_sleep_monitor_logind_finalize;
}

static void clock_sleep_monitor_logind_init(ClockSleepMonitorLogind *monitor)
{
}

static void clock_sleep_monitor_logind_finalize(GObject *object)
{
  ClockSleepMonitorLogind *monitor = XFCE_CLOCK_SLEEP_MONITOR_LOGIND(object);
  g_return_if_fail(monitor != NULL);

  if (monitor->logind_proxy != NULL) {
    if (monitor->logind_signal_id != 0) {
      g_signal_handler_disconnect(monitor->logind_proxy, monitor->logind_signal_id);
    }
    g_object_unref(G_OBJECT(monitor->logind_proxy));
  }

  G_OBJECT_CLASS(clock_sleep_monitor_logind_parent_class)->finalize(object);
}

static void on_logind_signal(
    GDBusProxy *proxy,
    gchar *sender_name,
    gchar *signal_name,
    GVariant *parameters,
    ClockSleepMonitorLogind *monitor)
{
  const gchar *format_string = "(b)";
  gboolean going_to_sleep;

  g_return_if_fail(XFCE_IS_CLOCK_SLEEP_MONITOR_LOGIND(monitor));

  if (strcmp(signal_name, "PrepareForSleep") != 0) {
    return;
  }

  if (!g_variant_check_format_string(parameters, format_string, FALSE)) {
    g_critical("unexpected format string: %s", g_variant_get_type_string(parameters));
    return;
  }

  g_variant_get(parameters, format_string, &going_to_sleep);

  if (!going_to_sleep) {
    g_signal_emit(G_OBJECT(monitor), clock_sleep_monitor_woke_up_signal, 0);
  }
}

static ClockSleepMonitor* clock_sleep_monitor_logind_create(void)
{
  ClockSleepMonitorLogind* monitor;

  g_message("trying to instantiate logind sleep monitor");

  monitor = g_object_new(XFCE_TYPE_CLOCK_SLEEP_MONITOR_LOGIND, NULL);
  g_return_val_if_fail(monitor != NULL, NULL);

  monitor->logind_proxy = g_dbus_proxy_new_for_bus_sync(
      G_BUS_TYPE_SYSTEM,
      G_DBUS_PROXY_FLAGS_NONE,
      NULL,
      "org.freedesktop.login1",
      "/org/freedesktop/login1",
      "org.freedesktop.login1.Manager",
      NULL,
      NULL);
  if (monitor->logind_proxy == NULL) {
    g_message("could not get proxy for org.freedesktop.login1");
    g_object_unref(G_OBJECT(monitor));
    return NULL;
  }

  monitor->logind_signal_id = g_signal_connect(monitor->logind_proxy, "g-signal", G_CALLBACK(on_logind_signal), monitor);
  if (monitor->logind_signal_id == 0) {
    g_critical("could not connect to logind signal");
    g_object_unref(G_OBJECT(monitor));
    return NULL;
  }

  return XFCE_CLOCK_SLEEP_MONITOR(monitor);
}

#endif /* defined SLEEP_MONITOR_USE_LOGIND */



/* Factory registration
 *
 * Collect available implementations in a reasonable order.
 */

typedef ClockSleepMonitor* (*SleepMonitorFactory)(void);

static SleepMonitorFactory sleep_monitor_factories[] = {
  #ifdef SLEEP_MONITOR_USE_LOGIND
  clock_sleep_monitor_logind_create,
  #endif
  NULL
};

ClockSleepMonitor *clock_sleep_monitor_create(void)
{
  SleepMonitorFactory *factory_ptr = &sleep_monitor_factories[0];
  ClockSleepMonitor *monitor = NULL;

  for (; monitor == NULL && *factory_ptr != NULL; factory_ptr++) {
    monitor = (*factory_ptr)();
  }

  if (monitor == NULL && sleep_monitor_factories[0] != NULL) {
    g_warning("could not instantiate a sleep monitor");
  }

  return monitor;
}
