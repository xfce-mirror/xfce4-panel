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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include "clock.h"
#include "clock-analog.h"
#include "clock-binary.h"
#include "clock-dialog.h"
#include "clock-digital.h"
#include "clock-lcd.h"


#define USE_DEBUG_TIME (0)



/** prototypes **/
#if USE_DEBUG_TIME
static void         xfce_clock_util_get_debug_localtime  (struct tm       *tm);
#endif
static guint        xfce_clock_util_interval_from_format (const gchar     *format);
static guint        xfce_clock_util_next_interval        (guint            timeout_interval);
static gboolean     xfce_clock_tooltip_update            (gpointer         user_data);
static gboolean     xfce_clock_tooltip_sync_timeout      (gpointer         user_data);
static gboolean     xfce_clock_widget_sync_timeout       (gpointer         user_data);
static ClockPlugin *xfce_clock_plugin_init               (XfcePanelPlugin *plugin);
static void         xfce_clock_plugin_read               (ClockPlugin     *clock);
static void         xfce_clock_plugin_write              (ClockPlugin     *clock);
static void         xfce_clock_plugin_free               (ClockPlugin     *clock);
static void         xfce_clock_plugin_construct          (XfcePanelPlugin *plugin);



/** register the plugin **/
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (xfce_clock_plugin_construct);



/** utilities **/
void
xfce_clock_util_get_localtime (struct tm *tm)
{
    time_t now = time (0);

#ifndef HAVE_LOCALTIME_R
    struct tm *tmbuf;

    tmbuf = localtime (&now);
    *tm = *tmbuf;
#else
    localtime_r (&now, tm);
#endif

#if USE_DEBUG_TIME
    xfce_clock_util_get_debug_localtime (tm);
#endif
}



#if USE_DEBUG_TIME
static void
xfce_clock_util_get_debug_localtime (struct tm *tm)
{
    static gint hour = 23;
    static gint min = 59;
    static gint sec = 45;
    
    /* add 1 seconds */
    sec++;
    
    /* update times */
    if (sec > 59)
    {
        sec = 0;
        min++;
    }
    if (min > 59)
    {
        min = 0;
        hour++;
    }
    if (hour > 23)
    {
        hour = 0;
    }
    
    /* set time structure */
    tm->tm_sec = sec;
    tm->tm_min = min;
    tm->tm_hour = hour;
}
#endif



static guint
xfce_clock_util_interval_from_format (const gchar *format)
{
    const gchar *p;
    guint        interval = CLOCK_INTERVAL_HOUR;

    g_return_val_if_fail (format != NULL, CLOCK_INTERVAL_HOUR);

    for (p = format; *p != '\0'; ++p)
    {
        if (p[0] == '%' && p[1] != '\0')
        {
            switch (*++p)
            {
                case 'c':
                case 'N':
                case 'r':
                case 's':
                case 'S':
                case 'T':
                case 'X':
                    return CLOCK_INTERVAL_SECOND;

                case 'M':
                case 'R':
                    interval = CLOCK_INTERVAL_MINUTE;
                    break;
            }
        }
    }

    return interval;
}



static guint
xfce_clock_util_next_interval (guint timeout_interval)
{
    struct tm tm;
    GTimeVal  timeval;
    guint     interval;

    /* get the precise time */
    g_get_current_time (&timeval);

    /* ms to next second */
    interval = 1000 - (timeval.tv_usec / 1000);

    /* get current time */
    xfce_clock_util_get_localtime (&tm);

    /* add the interval time to the next update */
    switch (timeout_interval)
    {
        case CLOCK_INTERVAL_HOUR:
            /* ms to next hour */
            interval += (60 - tm.tm_min) * CLOCK_INTERVAL_MINUTE;

            /* fall-through to add the minutes */

        case CLOCK_INTERVAL_MINUTE:
            /* ms to next minute */
            interval += (60 - tm.tm_sec) * CLOCK_INTERVAL_SECOND;
            break;

        default:
            break;
    }

    return interval;
}



gchar *
xfce_clock_util_strdup_strftime (const gchar *format,
                                 const struct tm *tm)
{
    gchar *converted, *result;
    gsize  length;
    gchar  buffer[BUFFER_SIZE];

    /* convert to locale, because that's what strftime uses */
    converted = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
    if (G_UNLIKELY (converted == NULL))
        return NULL;

    /* parse the time string */
    length = strftime (buffer, sizeof (buffer), converted, tm);
    if (G_UNLIKELY (length == 0))
        buffer[0] = '\0';

    /* convert the string back to utf-8 */
    result = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);

    /* cleanup */
    g_free (converted);

    return result;
}



/** tooltip functions **/
static gboolean
xfce_clock_tooltip_update (gpointer user_data)
{
    ClockPlugin        *clock = (ClockPlugin *) user_data;
    gchar              *string;
    static GtkTooltips *tooltips = NULL;
    struct tm           tm;

    g_return_val_if_fail (clock->tooltip_format != NULL, TRUE);

    /* allocate the tooltip on-demand */
    if (G_UNLIKELY (tooltips == NULL))
        tooltips = gtk_tooltips_new ();

    /* get the local time */
    xfce_clock_util_get_localtime (&tm);

    /* get the string */
    string = xfce_clock_util_strdup_strftime (clock->tooltip_format, &tm);

    /* set the tooltip */
    gtk_tooltips_set_tip (tooltips, clock->ebox, string, NULL);

    /* cleanup */
    g_free (string);

    return TRUE;
}



static gboolean
xfce_clock_tooltip_sync_timeout (gpointer user_data)
{
    ClockPlugin *clock = (ClockPlugin *) user_data;

    /* start the tooltip update interval */
    clock->clock_timeout_id = g_timeout_add (clock->tooltip_interval, xfce_clock_tooltip_update, clock);

    /* manual update for this timeout */
    xfce_clock_tooltip_update (clock);

    /* stop the sync timeout */
    return FALSE;
}



void
xfce_clock_tooltip_sync (ClockPlugin *clock)
{
    guint interval;

    if (clock->tooltip_timeout_id)
    {
        /* stop old and reset timeout */
        g_source_remove (clock->tooltip_timeout_id);
        clock->tooltip_timeout_id = 0;
    }

    /* detect the tooltip interval from the string */
    clock->tooltip_interval = xfce_clock_util_interval_from_format (clock->tooltip_format);

    /* get the interval to the next update */
    interval = xfce_clock_util_next_interval (clock->tooltip_interval);

    /* start the sync timeout */
    clock->tooltip_timeout_id = g_timeout_add (interval, xfce_clock_tooltip_sync_timeout, clock);

    /* update the tooltip */
    xfce_clock_tooltip_update (clock);
}



/** clock widget functions **/
static gboolean
xfce_clock_widget_sync_timeout (gpointer user_data)
{
    ClockPlugin *clock = (ClockPlugin *) user_data;

    if (G_LIKELY (clock->widget))
    {
        /* start the clock update timeout */
        clock->clock_timeout_id = g_timeout_add (clock->interval, clock->update, clock->widget);

        /* manual update for this interval */
        (clock->update) (clock->widget);
    }
    else
    {
        /* remove timer id */
        clock->clock_timeout_id = 0;
    }

    /* stop the sync timeout */
    return FALSE;
}



void
xfce_clock_widget_sync (ClockPlugin *clock)
{
    guint interval;

    if (clock->clock_timeout_id)
    {
        /* stop old and reset timeout */
        g_source_remove (clock->clock_timeout_id);
        clock->clock_timeout_id = 0;
    }

    if (G_LIKELY (clock->widget))
    {
        /* get the interval to the next update */
        interval = xfce_clock_util_next_interval (clock->interval);

        /* start the sync timeout */
        clock->clock_timeout_id = g_timeout_add (interval, xfce_clock_widget_sync_timeout, clock);
    }
}



void
xfce_clock_widget_update_settings (ClockPlugin *clock)
{
    g_return_if_fail (clock->widget != NULL);

    /* send the settings based on the clock mode */
    switch (clock->mode)
    {
        case XFCE_CLOCK_ANALOG:
            /* set settings */
            g_object_set (G_OBJECT (clock->widget),
                          "show-seconds", clock->show_seconds, NULL);
            break;

        case XFCE_CLOCK_BINARY:
            /* set settings */
            g_object_set (G_OBJECT (clock->widget),
                          "show-seconds", clock->show_seconds,
                          "true-binary", clock->true_binary, NULL);
            break;

        case XFCE_CLOCK_DIGITAL:
            /* set settings */
            g_object_set (G_OBJECT (clock->widget),
                          "digital-format", clock->digital_format, NULL);
            break;

        case XFCE_CLOCK_LCD:
            /* set settings */
            g_object_set (G_OBJECT (clock->widget),
                          "show-seconds", clock->show_seconds,
                          "show-military", clock->show_military,
                          "show-meridiem", clock->show_meridiem, NULL);
            break;
    }

    /* get update interval */
    if (clock->mode == XFCE_CLOCK_DIGITAL)
    {
        /* get interval from string */
        clock->interval = xfce_clock_util_interval_from_format (clock->digital_format);
    }
    else
    {
        /* interval from setting */
        clock->interval = clock->show_seconds ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE;
    }
}



void
xfce_clock_widget_set_mode (ClockPlugin *clock)
{
    GtkWidget *widget;

    /* stop runing timeout */
    if (clock->clock_timeout_id)
    {
        g_source_remove (clock->clock_timeout_id);
        clock->clock_timeout_id = 0;
    }

    /* destroy the old widget */
    if (clock->widget)
    {
        gtk_widget_destroy (clock->widget);
        clock->widget = NULL;
    }

    switch (clock->mode)
    {
        case XFCE_CLOCK_ANALOG:
            widget = xfce_clock_analog_new ();
            clock->update = xfce_clock_analog_update;
            break;

        case XFCE_CLOCK_BINARY:
            widget = xfce_clock_binary_new ();
            clock->update = xfce_clock_binary_update;
            break;

        case XFCE_CLOCK_DIGITAL:
            widget = xfce_clock_digital_new ();
            clock->update = xfce_clock_digital_update;
            break;

        case XFCE_CLOCK_LCD:
            widget = xfce_clock_lcd_new ();
            clock->update = xfce_clock_lcd_update;
            break;

        default:
            g_error ("Unknown clock type");
            return;
    }

    /* set the clock */
    clock->widget = widget;

    /* add and show the clock */
    gtk_container_add (GTK_CONTAINER (clock->frame), widget);
    gtk_widget_show (widget);
}



/** plugin functions **/
static ClockPlugin *
xfce_clock_plugin_init (XfcePanelPlugin *plugin)
{
    ClockPlugin *clock;

    /* create structure */
    clock = panel_slice_new0 (ClockPlugin);

    /* set plugin */
    clock->plugin = plugin;

    /* initialize */
    clock->clock_timeout_id = 0;
    clock->tooltip_timeout_id = 0;
    clock->widget = NULL;
    clock->tooltip_format = NULL;
    clock->digital_format = NULL;

    /* read the user settings */
    xfce_clock_plugin_read (clock);

    /* build widgets */
    clock->ebox = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (plugin), clock->ebox);
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (clock->ebox), FALSE);
    gtk_widget_show (clock->ebox);

    clock->frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (clock->ebox), clock->frame);
    gtk_frame_set_shadow_type (GTK_FRAME (clock->frame), clock->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    gtk_widget_show (clock->frame);

    /* set the clock */
    xfce_clock_widget_set_mode (clock);

    /* set the clock settings */
    xfce_clock_widget_update_settings (clock);

    /* start the timeout */
    xfce_clock_widget_sync (clock);

    /* start the tooltip sync */
    xfce_clock_tooltip_sync (clock);

    return clock;
}



gboolean
xfce_clock_plugin_set_size (ClockPlugin *clock,
                            guint        size)
{
    GtkOrientation orientation;

    /* set the frame border */
    gtk_container_set_border_width (GTK_CONTAINER (clock->frame), size > 26 ? 1 : 0);

    /* get the clock size */
    size -= size > 26 ? 6 : 4;

    /* get plugin orientation */
    orientation = xfce_panel_plugin_get_orientation (clock->plugin);

    /* set the clock size */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (clock->widget, -1, size);
    else
        gtk_widget_set_size_request (clock->widget, size, -1);

    return TRUE;
}



static void
xfce_clock_plugin_read (ClockPlugin *clock)
{
    gchar       *filename;
    const gchar *value;
    XfceRc      *rc;

    /* config filename */
    filename = xfce_panel_plugin_lookup_rc_file (clock->plugin);

    if (G_LIKELY (filename))
    {
        /* open rc file (readonly) and cleanup */
        rc = xfce_rc_simple_open (filename, TRUE);
        g_free (filename);

        if (G_LIKELY (rc))
        {
            /* read strings */
            value = xfce_rc_read_entry (rc, "DigitalFormat", DEFAULT_DIGITAL_FORMAT);
            if (G_LIKELY (value != NULL && *value != '\0'))
                clock->digital_format = g_strdup (value);

            value = xfce_rc_read_entry (rc, "TooltipFormat", DEFAULT_TOOLTIP_FORMAT);
            if (G_LIKELY (value != NULL && *value != '\0'))
                clock->tooltip_format = g_strdup (value);

            /* read clock type */
            clock->mode = xfce_rc_read_int_entry (rc, "ClockType", XFCE_CLOCK_DIGITAL);

            /* read boolean settings */
            clock->show_frame    = xfce_rc_read_bool_entry (rc, "ShowFrame", TRUE);
            clock->show_seconds  = xfce_rc_read_bool_entry (rc, "ShowSeconds", FALSE);
            clock->show_military = xfce_rc_read_bool_entry (rc, "ShowMilitary", TRUE);
            clock->show_meridiem = xfce_rc_read_bool_entry (rc, "ShowMeridiem", FALSE);
            clock->true_binary   = xfce_rc_read_bool_entry (rc, "TrueBinary", FALSE);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_clock_plugin_write (ClockPlugin *clock)
{
    gchar  *filename;
    XfceRc *rc;

    /* config filename */
    filename = xfce_panel_plugin_save_location (clock->plugin, TRUE);

    if (G_LIKELY (filename))
    {
        /* open rc file and cleanup */
        rc = xfce_rc_simple_open (filename, FALSE);
        g_free (filename);

        if (G_LIKELY (rc))
        {
            /* write settings */
            if (G_LIKELY (clock->digital_format && *clock->digital_format != '\0'))
                xfce_rc_write_entry (rc, "DigitalFormat", clock->digital_format);

            if (G_LIKELY (clock->tooltip_format && *clock->tooltip_format != '\0'))
                xfce_rc_write_entry (rc, "TooltipFormat", clock->tooltip_format);

            xfce_rc_write_int_entry (rc, "ClockType", clock->mode);
            xfce_rc_write_bool_entry (rc, "ShowFrame", clock->show_frame);
            xfce_rc_write_bool_entry (rc, "ShowSeconds", clock->show_seconds);
            xfce_rc_write_bool_entry (rc, "ShowMilitary", clock->show_military);
            xfce_rc_write_bool_entry (rc, "ShowMeridiem", clock->show_meridiem);
            xfce_rc_write_bool_entry (rc, "TrueBinary", clock->true_binary);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_clock_plugin_free (ClockPlugin *clock)
{
    GtkWidget *dialog;

    /* stop timeouts */
    if (G_LIKELY (clock->clock_timeout_id))
        g_source_remove (clock->clock_timeout_id);

    if (G_LIKELY (clock->tooltip_timeout_id))
        g_source_remove (clock->tooltip_timeout_id);

    /* destroy the configure dialog if it's still open */
    dialog = g_object_get_data (G_OBJECT (clock->plugin), I_("configure-dialog"));
    if (G_UNLIKELY (dialog != NULL))
        gtk_widget_destroy (dialog);

    /* cleanup */
    g_free (clock->tooltip_format);
    g_free (clock->digital_format);

    /* free structure */
    panel_slice_free (ClockPlugin, clock);
}



static void
xfce_clock_plugin_construct (XfcePanelPlugin *plugin)
{
    ClockPlugin *clock = xfce_clock_plugin_init (plugin);

    /* plugin settings */
    xfce_panel_plugin_add_action_widget (plugin, clock->ebox);
    xfce_panel_plugin_menu_show_configure (plugin);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (plugin), "size-changed", G_CALLBACK (xfce_clock_plugin_set_size), clock);
    g_signal_connect_swapped (G_OBJECT (plugin), "save", G_CALLBACK (xfce_clock_plugin_write), clock);
    g_signal_connect_swapped (G_OBJECT (plugin), "free-data", G_CALLBACK (xfce_clock_plugin_free), clock);
    g_signal_connect_swapped (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (xfce_clock_dialog_show), clock);
}

