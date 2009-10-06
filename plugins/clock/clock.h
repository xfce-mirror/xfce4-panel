/* $Id$ */
/*
 * Copyright (c) 2007 Nick Schermer <nick@xfce.org>
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
#include <libxfce4panel/xfce-panel-plugin.h>

G_BEGIN_DECLS

#define CLOCK_INTERVAL_SECOND (1000)
#define CLOCK_INTERVAL_MINUTE (60 * 1000)

#define BUFFER_SIZE            256
#define DEFAULT_TOOLTIP_FORMAT "%A %d %B %Y"
#define DEFAULT_DIGITAL_FORMAT "%R"



typedef struct _ClockPlugin     ClockPlugin;
typedef enum   _ClockPluginMode ClockPluginMode;

enum _ClockPluginMode
{
    XFCE_CLOCK_ANALOG = 0,
    XFCE_CLOCK_BINARY,
    XFCE_CLOCK_DIGITAL,
    XFCE_CLOCK_LCD
};

struct _ClockPlugin
{
    /* plugin */
    XfcePanelPlugin *plugin;

    /* widgets */
    GtkWidget       *ebox;
    GtkWidget       *frame;
    GtkWidget       *widget;

    /* clock update function and timeout */
    GSourceFunc      update;
    guint            interval;

    /* tooltip interval */
    guint            tooltip_interval;

    /* clock type */
    ClockPluginMode  mode;

    /* timeouts */
    guint            clock_timeout_id;
    guint            tooltip_timeout_id;

    /* settings */
    gchar           *tooltip_format;
    gchar           *digital_format;
    guint            show_frame : 1;
    guint            show_seconds : 1;
    guint            show_military : 1;
    guint            show_meridiem : 1;
    guint            true_binary : 1;
    guint            flash_separators : 1;
};



void      xfce_clock_util_get_localtime     (struct tm       *tm)    G_GNUC_INTERNAL;

gchar    *xfce_clock_util_strdup_strftime   (const gchar     *format,
                                             const struct tm *tm)    G_GNUC_MALLOC G_GNUC_INTERNAL;

void      xfce_clock_tooltip_sync           (ClockPlugin     *clock) G_GNUC_INTERNAL;

void      xfce_clock_widget_sync            (ClockPlugin     *clock) G_GNUC_INTERNAL;

void      xfce_clock_widget_update_settings (ClockPlugin     *clock) G_GNUC_INTERNAL;

void      xfce_clock_widget_set_mode        (ClockPlugin     *clock) G_GNUC_INTERNAL;

gboolean  xfce_clock_plugin_set_size        (ClockPlugin     *clock,
                                             guint            size)  G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__CLOCK_H__ */
