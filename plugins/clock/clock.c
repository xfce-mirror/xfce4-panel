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
static void         xfce_clock_tooltip_update            (ClockPlugin     *plugin);
static gboolean     xfce_clock_tooltip_sync_timeout      (gpointer         user_data);
static gboolean     xfce_clock_widget_sync_timeout       (gpointer         user_data);
static ClockPlugin *xfce_clock_plugin_init               (XfcePanelPlugin *panel_plugin);
static void         xfce_clock_plugin_read               (ClockPlugin     *plugin);
static void         xfce_clock_plugin_write              (ClockPlugin     *plugin);
static void         xfce_clock_plugin_free               (ClockPlugin     *plugin);
static void         xfce_clock_plugin_construct          (XfcePanelPlugin *panel_plugin);



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

    if (G_UNLIKELY (format == NULL))
        return CLOCK_INTERVAL_MINUTE;

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
            }
        }
    }

    return CLOCK_INTERVAL_MINUTE;
}



static guint
xfce_clock_util_next_interval (guint timeout_interval)
{
    struct tm tm;

    /* add the interval time to the next update */
    if (timeout_interval == CLOCK_INTERVAL_MINUTE)
    {
        /* ms to next minute */
        xfce_clock_util_get_localtime (&tm);
        return 60 - tm.tm_sec;
    }

    return 0;
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
static void
xfce_clock_tooltip_update (ClockPlugin *plugin)
{
    gchar              *string;
    struct tm           tm;
#if !GTK_CHECK_VERSION (2,12,0)
    static GtkTooltips *tooltips = NULL;

    /* allocate the tooltip on-demand */
    if (G_UNLIKELY (tooltips == NULL))
        tooltips = gtk_tooltips_new ();
#endif

    if (G_UNLIKELY (plugin->tooltip_format == NULL))
        return;

    /* get the local time */
    xfce_clock_util_get_localtime (&tm);

    /* get the string */
    string = xfce_clock_util_strdup_strftime (plugin->tooltip_format, &tm);

    /* set the tooltip */
#if GTK_CHECK_VERSION (2,12,0)
    gtk_widget_set_tooltip_text (plugin->ebox, string);
    gtk_widget_trigger_tooltip_query (GTK_WIDGET (plugin->ebox));
#else
    gtk_tooltips_set_tip (tooltips, plugin->ebox, string, NULL);
#endif

    /* cleanup */
    g_free (string);
}



static gboolean
xfce_clock_tooltip_timeout (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;
    struct tm    tm;

    xfce_clock_tooltip_update (plugin);

    if (plugin->tooltip_interval == CLOCK_INTERVAL_MINUTE)
    {
        xfce_clock_util_get_localtime (&tm);
        plugin->tooltip_restart = tm.tm_sec != 0;
    }

    return !plugin->tooltip_restart;
}



static void
xfce_clock_tooltip_timeout_destroyed (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;

    plugin->tooltip_timeout_id = 0;

    if (plugin->tooltip_restart)
    {
        plugin->tooltip_restart = FALSE;
        xfce_clock_tooltip_sync (plugin);
    }
}



static gboolean
xfce_clock_tooltip_sync_timeout (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;

    /* start the tooltip update interval */
    plugin->tooltip_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, plugin->tooltip_interval * 1000,
                                                      xfce_clock_tooltip_timeout, plugin,
                                                      xfce_clock_tooltip_timeout_destroyed);

    /* manual update for this timeout */
    xfce_clock_tooltip_update (plugin);

    /* stop the sync timeout */
    return FALSE;
}



void
xfce_clock_tooltip_sync (ClockPlugin *plugin)
{
    guint interval;

    if (plugin->tooltip_timeout_id)
    {
        /* stop old and reset timeout */
        g_source_remove (plugin->tooltip_timeout_id);
        plugin->tooltip_timeout_id = 0;
    }

    /* detect the tooltip interval from the string */
    plugin->tooltip_interval = xfce_clock_util_interval_from_format (plugin->tooltip_format);

    /* get the interval to the next update */
    interval = xfce_clock_util_next_interval (plugin->tooltip_interval);

    if (interval > 0 && plugin->tooltip_interval != CLOCK_INTERVAL_SECOND)
    {
         /* start the sync timeout */
         plugin->tooltip_timeout_id = g_timeout_add (interval * 1000, xfce_clock_tooltip_sync_timeout, plugin);
     }
     else
     {
         /* start the real timeout */
         plugin->tooltip_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, plugin->tooltip_interval * 1000,
                                                          xfce_clock_tooltip_timeout, plugin,
                                                          xfce_clock_tooltip_timeout_destroyed);
     }

    /* update the tooltip */
    xfce_clock_tooltip_update (plugin);
}



/** clock widget functions **/
static gboolean
xfce_clock_widget_timeout (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;
    gboolean     result;
    struct tm    tm;

    /* update the widget */
    result = (plugin->update) (plugin->widget);

    if (result && plugin->interval == CLOCK_INTERVAL_MINUTE)
    {
        xfce_clock_util_get_localtime (&tm);
        plugin->restart = tm.tm_sec != 0;
    }

    return result && !plugin->restart;
}



static void
xfce_clock_widget_timeout_destroyed (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;

    plugin->clock_timeout_id = 0;

    if (plugin->restart)
    {
        plugin->restart = FALSE;
        xfce_clock_widget_sync (plugin);
    }
}



static gboolean
xfce_clock_widget_sync_timeout (gpointer user_data)
{
    ClockPlugin *plugin = (ClockPlugin *) user_data;

    if (G_LIKELY (plugin->widget))
    {
        /* start the clock update timeout */
        plugin->clock_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, plugin->interval * 1000,
                                                       xfce_clock_widget_timeout, plugin,
                                                       xfce_clock_widget_timeout_destroyed);

        /* manual update for this interval */
        (plugin->update) (plugin->widget);
    }
    else
    {
        /* remove timer id */
        plugin->clock_timeout_id = 0;
    }

    /* stop the sync timeout */
    return FALSE;
}



void
xfce_clock_widget_sync (ClockPlugin *plugin)
{
    guint interval;

    if (plugin->clock_timeout_id)
    {
        /* stop old and reset timeout */
        g_source_remove (plugin->clock_timeout_id);
        plugin->clock_timeout_id = 0;
    }

    if (G_LIKELY (plugin->widget))
    {
        /* get the interval to the next update */
        interval = xfce_clock_util_next_interval (plugin->interval);

        if (interval > 0 && plugin->interval != CLOCK_INTERVAL_SECOND)
        {
            /* start the sync timeout */
            plugin->clock_timeout_id = g_timeout_add (interval * 1000, xfce_clock_widget_sync_timeout, plugin);
        }
        else
        {
            /* start the real timeout */
            plugin->clock_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, plugin->interval * 1000,
                                                           xfce_clock_widget_timeout, plugin,
                                                           xfce_clock_widget_timeout_destroyed);
        }


    }
}



void
xfce_clock_widget_update_settings (ClockPlugin *plugin)
{
    g_return_if_fail (plugin->widget != NULL);

    /* send the settings based on the clock mode */
    switch (plugin->mode)
    {
        case XFCE_CLOCK_ANALOG:
            /* set settings */
            g_object_set (G_OBJECT (plugin->widget),
                          "show-seconds", plugin->show_seconds, NULL);
            break;

        case XFCE_CLOCK_BINARY:
            /* set settings */
            g_object_set (G_OBJECT (plugin->widget),
                          "show-seconds", plugin->show_seconds,
                          "true-binary", plugin->true_binary, NULL);
            break;

        case XFCE_CLOCK_DIGITAL:
            /* set settings */
            g_object_set (G_OBJECT (plugin->widget),
                          "digital-format", plugin->digital_format, NULL);
            break;

        case XFCE_CLOCK_LCD:
            /* set settings */
            g_object_set (G_OBJECT (plugin->widget),
                          "show-seconds", plugin->show_seconds,
                          "show-military", plugin->show_military,
                          "show-meridiem", plugin->show_meridiem,
                          "flash-separators", plugin->flash_separators, NULL);
            break;
    }

    /* get update interval */
    if (plugin->mode == XFCE_CLOCK_DIGITAL)
    {
        /* get interval from string */
        plugin->interval = xfce_clock_util_interval_from_format (plugin->digital_format);
    }
    else
    {
        /* interval from setting */
        if (plugin->mode == XFCE_CLOCK_LCD)
            plugin->interval = (plugin->show_seconds || plugin->flash_separators) ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE;
        else
            plugin->interval = plugin->show_seconds ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE;
    }
}



void
xfce_clock_widget_set_mode (ClockPlugin *plugin)
{
    GtkWidget *widget;

    /* stop runing timeout */
    if (plugin->clock_timeout_id)
    {
        g_source_remove (plugin->clock_timeout_id);
        plugin->clock_timeout_id = 0;
    }

    /* destroy the old widget */
    if (plugin->widget)
    {
        gtk_widget_destroy (plugin->widget);
        plugin->widget = NULL;
    }

    switch (plugin->mode)
    {
        case XFCE_CLOCK_ANALOG:
            widget = xfce_clock_analog_new ();
            plugin->update = xfce_clock_analog_update;
            break;

        case XFCE_CLOCK_BINARY:
            widget = xfce_clock_binary_new ();
            plugin->update = xfce_clock_binary_update;
            break;

        case XFCE_CLOCK_DIGITAL:
            widget = xfce_clock_digital_new ();
            plugin->update = xfce_clock_digital_update;
            break;

        case XFCE_CLOCK_LCD:
            widget = xfce_clock_lcd_new ();
            plugin->update = xfce_clock_lcd_update;
            break;

        default:
            g_error ("Unknown clock type");
            return;
    }

    /* set the clock */
    plugin->widget = widget;

    /* add and show the clock */
    gtk_container_add (GTK_CONTAINER (plugin->frame), widget);
    gtk_widget_show (widget);
}



/** plugin functions **/
static ClockPlugin *
xfce_clock_plugin_init (XfcePanelPlugin *panel_plugin)
{
    ClockPlugin *plugin;

    /* create structure */
    plugin = panel_slice_new0 (ClockPlugin);

    /* set plugin */
    plugin->plugin = panel_plugin;

    /* initialize */
    plugin->clock_timeout_id = 0;
    plugin->tooltip_timeout_id = 0;
    plugin->widget = NULL;
    plugin->tooltip_format = NULL;
    plugin->digital_format = NULL;

    /* read the user settings */
    xfce_clock_plugin_read (plugin);

    /* build widgets */
    plugin->ebox = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->ebox);
    gtk_event_box_set_visible_window (GTK_EVENT_BOX (plugin->ebox), FALSE);
    gtk_widget_show (plugin->ebox);

    plugin->frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (plugin->ebox), plugin->frame);
    gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), plugin->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    gtk_widget_show (plugin->frame);

    /* set the clock */
    xfce_clock_widget_set_mode (plugin);

    /* set the clock settings */
    xfce_clock_widget_update_settings (plugin);

    /* start the timeout */
    xfce_clock_widget_sync (plugin);

    /* start the tooltip sync */
    xfce_clock_tooltip_sync (plugin);

    return plugin;
}



gboolean
xfce_clock_plugin_set_size (ClockPlugin *plugin,
                            guint        size)
{
    GtkOrientation orientation;

    /* set the frame border */
    gtk_container_set_border_width (GTK_CONTAINER (plugin->frame), size > 26 ? 1 : 0);

    /* get the clock size */
    size -= size > 26 ? 6 : 4;

    /* get plugin orientation */
    orientation = xfce_panel_plugin_get_orientation (plugin->plugin);

    /* set the clock size */
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (plugin->widget, -1, size);
    else
        gtk_widget_set_size_request (plugin->widget, size, -1);

    return TRUE;
}



static void
xfce_clock_plugin_set_orientation (ClockPlugin *plugin)
{
    /* do a size request */
    xfce_clock_plugin_set_size (plugin, xfce_panel_plugin_get_size (plugin->plugin));
}



static void
xfce_clock_plugin_read (ClockPlugin *plugin)
{
    gchar       *filename;
    const gchar *value;
    XfceRc      *rc;

    /* config filename */
    filename = xfce_panel_plugin_lookup_rc_file (plugin->plugin);

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
                plugin->digital_format = g_strdup (value);

            value = xfce_rc_read_entry (rc, "TooltipFormat", DEFAULT_TOOLTIP_FORMAT);
            if (G_LIKELY (value != NULL && *value != '\0'))
                plugin->tooltip_format = g_strdup (value);

            /* read clock type */
            plugin->mode = xfce_rc_read_int_entry (rc, "ClockType", XFCE_CLOCK_DIGITAL);

            /* read boolean settings */
            plugin->show_frame       = xfce_rc_read_bool_entry (rc, "ShowFrame", TRUE);
            plugin->show_seconds     = xfce_rc_read_bool_entry (rc, "ShowSeconds", FALSE);
            plugin->show_military    = xfce_rc_read_bool_entry (rc, "ShowMilitary", TRUE);
            plugin->show_meridiem    = xfce_rc_read_bool_entry (rc, "ShowMeridiem", FALSE);
            plugin->true_binary      = xfce_rc_read_bool_entry (rc, "TrueBinary", FALSE);
            plugin->flash_separators = xfce_rc_read_bool_entry (rc, "FlashSeparators", FALSE);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_clock_plugin_write (ClockPlugin *plugin)
{
    gchar  *filename;
    XfceRc *rc;

    /* config filename */
    filename = xfce_panel_plugin_save_location (plugin->plugin, TRUE);

    if (G_LIKELY (filename))
    {
        /* open rc file and cleanup */
        rc = xfce_rc_simple_open (filename, FALSE);
        g_free (filename);

        if (G_LIKELY (rc))
        {
            /* write settings */
            if (G_LIKELY (plugin->digital_format && *plugin->digital_format != '\0'))
                xfce_rc_write_entry (rc, "DigitalFormat", plugin->digital_format);

            if (G_LIKELY (plugin->tooltip_format && *plugin->tooltip_format != '\0'))
                xfce_rc_write_entry (rc, "TooltipFormat", plugin->tooltip_format);

            xfce_rc_write_int_entry (rc, "ClockType", plugin->mode);
            xfce_rc_write_bool_entry (rc, "ShowFrame", plugin->show_frame);
            xfce_rc_write_bool_entry (rc, "ShowSeconds", plugin->show_seconds);
            xfce_rc_write_bool_entry (rc, "ShowMilitary", plugin->show_military);
            xfce_rc_write_bool_entry (rc, "ShowMeridiem", plugin->show_meridiem);
            xfce_rc_write_bool_entry (rc, "TrueBinary", plugin->true_binary);
            xfce_rc_write_bool_entry (rc, "FlashSeparators", plugin->flash_separators);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_clock_plugin_free (ClockPlugin *plugin)
{
    GtkWidget *dialog;

    /* stop timeouts */
    if (G_LIKELY (plugin->clock_timeout_id))
        g_source_remove (plugin->clock_timeout_id);

    if (G_LIKELY (plugin->tooltip_timeout_id))
        g_source_remove (plugin->tooltip_timeout_id);

    /* destroy the configure dialog if it's still open */
    dialog = g_object_get_data (G_OBJECT (plugin->plugin), I_("configure-dialog"));
    if (G_UNLIKELY (dialog != NULL))
        gtk_widget_destroy (dialog);

    /* cleanup */
    g_free (plugin->tooltip_format);
    g_free (plugin->digital_format);

    /* free structure */
    panel_slice_free (ClockPlugin, plugin);
}



static void
xfce_clock_plugin_construct (XfcePanelPlugin *panel_plugin)
{
    ClockPlugin *plugin = xfce_clock_plugin_init (panel_plugin);

    /* plugin settings */
    xfce_panel_plugin_add_action_widget (panel_plugin, plugin->ebox);
    xfce_panel_plugin_menu_show_configure (panel_plugin);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "size-changed", G_CALLBACK (xfce_clock_plugin_set_size), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "save", G_CALLBACK (xfce_clock_plugin_write), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "free-data", G_CALLBACK (xfce_clock_plugin_free), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "configure-plugin", G_CALLBACK (xfce_clock_dialog_show), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "orientation-changed", G_CALLBACK (xfce_clock_plugin_set_orientation), plugin);
}

