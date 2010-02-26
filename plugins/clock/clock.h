/* $Id$ */
/*
 * Copyright (c) 2007-2008 Nick Schermer <nick@xfce.org>
 *
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

#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

/* clock intervals in ms */
#define CLOCK_INTERVAL_SECOND (1000)
#define CLOCK_INTERVAL_MINUTE (60 * CLOCK_INTERVAL_SECOND)
#define CLOCK_INTERVAL_HOUR   (60 * CLOCK_INTERVAL_MINUTE)

/* default values */
#define BUFFER_SIZE            256
#define DEFAULT_TOOLTIP_FORMAT "%A %d %B %Y"
#define DEFAULT_DIGITAL_FORMAT "%R"

/* allow timer grouping to save resources */
#if GLIB_CHECK_VERSION (2,14,0)
#define clock_timeout_add(interval, function, data) \
    g_timeout_add_seconds ((interval) / 1000, function, data)
#else
#define clock_timeout_add(interval, function, data) \
    g_timeout_add (interval, function, data)
#endif


typedef struct _XfceClockClass XfceClockClass;
typedef struct _XfceClock      XfceClock;
typedef enum   _XfceClockMode  XfceClockMode;

#define XFCE_TYPE_CLOCK            (xfce_clock_get_type ())
#define XFCE_CLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_CLOCK, XfceClock))
#define XFCE_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_CLOCK, XfceClockClass))
#define XFCE_IS_CLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_CLOCK))
#define XFCE_IS_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_CLOCK))
#define XFCE_CLOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_CLOCK, XfceClockClass))

enum _XfceClockMode
{
    XFCE_CLOCK_MODE_ANALOG = 0,
    XFCE_CLOCK_MODE_BINARY,
    XFCE_CLOCK_MODE_DIGITAL,
    XFCE_CLOCK_MODE_LCD
};

struct _XfceClockClass
{
  XfcePanelPluginClass __parent__;
};

struct _XfceClock
{
  XfcePanelPlugin __parent__;
  
  GtkWidget     *frame;
  GtkWidget     *widget;
  
  /* active clock */
  XfceClockMode  mode;
  
  /* widget timer update */
  GSourceFunc    update_func;
  guint          widget_interval;
  guint          widget_timer_id;
  
  /* tooltip timer update */
  guint          tooltip_interval;
  guint          tooltip_timer_id;
  
  /* settings */
  gchar         *tooltip_format;
  gchar         *digital_format;
  guint          show_frame : 1;
  guint          show_seconds : 1;
  guint          show_military : 1;
  guint          show_meridiem : 1;
  guint          true_binary : 1;
  guint          flash_separators : 1;
};



GType  xfce_clock_get_type                 (void) G_GNUC_CONST G_GNUC_INTERNAL;

void   xfce_clock_register_type            (XfcePanelModule *panel_module) G_GNUC_INTERNAL;

void   xfce_clock_util_get_localtime       (struct tm       *tm) G_GNUC_INTERNAL;

gchar *xfce_clock_util_strdup_strftime     (const gchar     *format,
                                            const struct tm *tm) G_GNUC_MALLOC G_GNUC_INTERNAL;

void   xfce_clock_tooltip_timer            (XfceClock       *clock) G_GNUC_INTERNAL;

void   xfce_clock_widget_timer             (XfceClock       *clock) G_GNUC_INTERNAL;

void   xfce_clock_widget_update_properties (XfceClock       *clock) G_GNUC_INTERNAL;

void   xfce_clock_widget_update_mode       (XfceClock       *clock) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__CLOCK_H__ */
