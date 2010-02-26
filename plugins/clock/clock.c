/* $Id: clock.c 26625 2008-02-18 07:59:29Z nick $ */
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
static void     xfce_clock_class_init                (XfceClockClass  *klass);
static void     xfce_clock_init                      (XfceClock       *clock);
static void     xfce_clock_finalize                  (GObject         *object);
static void     xfce_clock_size_changed              (XfcePanelPlugin *plugin,
                                                      gint             size);
static void     xfce_clock_configure_plugin          (XfcePanelPlugin *plugin);
static void     xfce_clock_load                      (XfceClock       *clock);
static guint    xfce_clock_util_interval_from_format (const gchar     *format);
static guint    xfce_clock_util_sync_interval        (guint            timeout_interval);
static gboolean xfce_clock_tooltip_timer_update      (gpointer         user_data);
static gboolean xfce_clock_tooltip_timer_sync        (gpointer         user_data);
static gboolean xfce_clock_widget_timer_sync         (gpointer         user_data);



#if !GTK_CHECK_VERSION (2,12,0)
static GtkTooltips *shared_tooltips = NULL;
#endif



XFCE_PANEL_DEFINE_TYPE (XfceClock, xfce_clock, XFCE_TYPE_PANEL_PLUGIN);



static void
xfce_clock_class_init (XfceClockClass *klass)
{
  GObjectClass         *gobject_class;
  XfcePanelPluginClass *plugin_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_clock_finalize;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->save = xfce_clock_save;
  plugin_class->size_changed = xfce_clock_size_changed;
  plugin_class->configure_plugin = xfce_clock_configure_plugin;

#if !GTK_CHECK_VERSION (2,12,0)
  /* allocate share tooltip */
  shared_tooltips = gtk_tooltips_new ();
#endif
}



static void
xfce_clock_init (XfceClock *clock)
{
  /* initialize the default values */
  clock->widget_timer_id = 0;
  clock->tooltip_timer_id = 0;
  clock->widget = NULL;
  clock->mode = XFCE_CLOCK_MODE_DIGITAL;
  clock->tooltip_format = NULL;
  clock->digital_format = NULL;

  /* read the user settings */
  xfce_clock_load (clock);

  /* build widgets */
  clock->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (clock), clock->frame);
  gtk_frame_set_shadow_type (GTK_FRAME (clock->frame), clock->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
  gtk_widget_show (clock->frame);

  /* set the clock mode (create clock widget) */
  xfce_clock_widget_update_mode (clock);

  /* set the clock settings */
  xfce_clock_widget_update_properties (clock);

  /* start the widget timer */
  xfce_clock_widget_timer (clock);

  /* start the tooltip timer */
  xfce_clock_tooltip_timer (clock);

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (clock));
}



static void
xfce_clock_finalize (GObject *object)
{
  XfceClock *clock = XFCE_CLOCK (object);

  /* stop running timeouts */
  if (G_LIKELY (clock->widget_timer_id != 0))
    g_source_remove (clock->widget_timer_id);

  if (G_LIKELY (clock->tooltip_timer_id != 0))
    g_source_remove (clock->tooltip_timer_id);

  /* cleanup */
  g_free (clock->tooltip_format);
  g_free (clock->digital_format);

  (*G_OBJECT_CLASS (xfce_clock_parent_class)->finalize) (object);
}



void
xfce_clock_save (XfcePanelPlugin *plugin)
{
  XfceClock *clock = XFCE_CLOCK (plugin);
  gchar     *filename;
  XfceRc    *rc;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (clock));

  /* config filename */
  filename = xfce_panel_plugin_save_location (plugin, TRUE);
  if (G_LIKELY (filename))
    {
      /* open rc file */
      rc = xfce_rc_simple_open (filename, FALSE);

      /* cleanup */
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
          xfce_rc_write_bool_entry (rc, "FlashSeparators", clock->flash_separators);

          /* close the rc file */
          xfce_rc_close (rc);
        }
    }
}



static void
xfce_clock_size_changed (XfcePanelPlugin *plugin,
                         gint             size)
{
  XfceClock      *clock = XFCE_CLOCK (plugin);
  GtkOrientation  orientation;

  /* set the frame border */
  gtk_container_set_border_width (GTK_CONTAINER (clock->frame), size > 26 ? 1 : 0);

  /* get the clock size */
  size -= size > 26 ? 6 : 4;

  /* get plugin orientation */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  /* set the clock size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (clock->widget, -1, size);
  else
    gtk_widget_set_size_request (clock->widget, size, -1);
}



static void
xfce_clock_configure_plugin (XfcePanelPlugin *plugin)
{
  /* show the dialog */
  xfce_clock_dialog_show (XFCE_CLOCK (plugin));
}



static void
xfce_clock_load (XfceClock *clock)
{
  gchar       *filename;
  const gchar *value;
  XfceRc      *rc;
  gboolean     succeed = FALSE;

  panel_return_if_fail (XFCE_IS_CLOCK (clock));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (clock));

  /* config filename */
  filename = xfce_panel_plugin_lookup_rc_file (XFCE_PANEL_PLUGIN (clock));

  if (G_LIKELY (filename))
    {
      /* open rc file (readonly) */
      rc = xfce_rc_simple_open (filename, TRUE);

      /* cleanup */
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
          clock->mode = xfce_rc_read_int_entry (rc, "ClockType", XFCE_CLOCK_MODE_DIGITAL);

          /* read boolean settings */
          clock->show_frame = xfce_rc_read_bool_entry (rc, "ShowFrame", TRUE);
          clock->show_seconds = xfce_rc_read_bool_entry (rc, "ShowSeconds", FALSE);
          clock->show_military = xfce_rc_read_bool_entry (rc, "ShowMilitary", TRUE);
          clock->show_meridiem = xfce_rc_read_bool_entry (rc, "ShowMeridiem", FALSE);
          clock->true_binary = xfce_rc_read_bool_entry (rc, "TrueBinary", FALSE);
          clock->flash_separators = xfce_rc_read_bool_entry (rc, "FlashSeparators", FALSE);

          /* close the rc file */
          xfce_rc_close (rc);

          /* loading succeeded */
          succeed = TRUE;
        }
    }

  if (succeed == FALSE)
    {
      /* fallback to defaults */
      clock->digital_format = g_strdup (DEFAULT_DIGITAL_FORMAT);
      clock->tooltip_format = g_strdup (DEFAULT_TOOLTIP_FORMAT);
    }
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



static guint
xfce_clock_util_interval_from_format (const gchar *format)
{
  const gchar *p;
  guint        interval = CLOCK_INTERVAL_HOUR;

  /* no format, update once an hour */
  if (G_UNLIKELY (format == NULL))
      return CLOCK_INTERVAL_HOUR;

  /* walk the format to find the smallest update time */
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
xfce_clock_util_sync_interval (guint timeout_interval)
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

  /* return null */
  if (G_UNLIKELY (format == NULL))
      return NULL;

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



static gboolean
xfce_clock_tooltip_timer_update (gpointer user_data)
{
  XfceClock *clock = XFCE_CLOCK (user_data);
  gchar     *string;
  struct tm  tm;

  /* stop running this timeout */
  if (clock->tooltip_format == NULL)
    return FALSE;

  /* get the local time */
  xfce_clock_util_get_localtime (&tm);

  /* get the string */
  string = xfce_clock_util_strdup_strftime (clock->tooltip_format, &tm);

#if GTK_CHECK_VERSION (2,12,0)
  /* set the tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (clock), string);
#else
  /* set the tooltip */
  if (G_LIKELY (shared_tooltips))
    gtk_tooltips_set_tip (shared_tooltips, GTK_WIDGET (clock), string, NULL);
#endif

  /* cleanup */
  g_free (string);

  return TRUE;
}



static gboolean
xfce_clock_tooltip_timer_sync (gpointer user_data)
{
  XfceClock *clock = XFCE_CLOCK (user_data);

  /* start the tooltip update interval */
  clock->tooltip_timer_id = clock_timeout_add (clock->tooltip_interval, xfce_clock_tooltip_timer_update, clock);

  /* manual update for this interval */
  xfce_clock_tooltip_timer_update (XFCE_CLOCK (user_data));

  /* stop the sync timeout */
  return FALSE;
}



void
xfce_clock_tooltip_timer (XfceClock *clock)
{
  guint interval;

  panel_return_if_fail (XFCE_IS_CLOCK (clock));

  /* stop a running timeout */
  if (clock->tooltip_timer_id != 0)
    {
      g_source_remove (clock->tooltip_timer_id);
      clock->tooltip_timer_id = 0;
    }

  /* start a new timeout if there is a format string */
  if (clock->tooltip_format != NULL)
    {
      /* detect the tooltip interval from the string */
      clock->tooltip_interval = xfce_clock_util_interval_from_format (clock->tooltip_format);

      /* get the interval to the next update */
      interval = xfce_clock_util_sync_interval (clock->tooltip_interval);

      /* start the sync timeout (loosy timer) */
      clock->tooltip_timer_id = clock_timeout_add (interval, xfce_clock_tooltip_timer_sync, clock);

      /* update the tooltip (if reasonable) */
      if (interval >= CLOCK_INTERVAL_SECOND)
        xfce_clock_tooltip_timer_update (clock);
    }
  else
    {
#if GTK_CHECK_VERSION (2,12,0)
      /* unset the tooltip */
      gtk_widget_set_tooltip_text (GTK_WIDGET (clock), NULL);
#else
      /* unset the tooltip */
      if (G_LIKELY (shared_tooltips))
        gtk_tooltips_set_tip (shared_tooltips, GTK_WIDGET (clock), NULL, NULL);
#endif
    }
}



static gboolean
xfce_clock_widget_timer_sync (gpointer user_data)
{
  XfceClock *clock = XFCE_CLOCK (user_data);

  if (G_LIKELY (clock->widget
                && clock->update_func != NULL
                && clock->widget_interval > 0))
    {
      /* start the clock update timeout */
      clock->widget_timer_id = clock_timeout_add (clock->widget_interval, clock->update_func, clock->widget);

      /* manual update for this interval */
      (*clock->update_func) (GTK_WIDGET (clock->widget));
    }
  else
    {
      /* remove timer id */
      clock->widget_timer_id = 0;
    }

  /* stop the sync timeout */
  return FALSE;
}



void
xfce_clock_widget_timer (XfceClock *clock)
{
  guint interval;

  panel_return_if_fail (XFCE_IS_CLOCK (clock));

  /* stop a running timeout */
  if (clock->widget_timer_id != 0)
    {
      g_source_remove (clock->widget_timer_id);
      clock->widget_timer_id = 0;
    }

  if (G_LIKELY (clock->widget))
    {
      /* get the interval to the next update */
      interval = xfce_clock_util_sync_interval (clock->widget_interval);

      /* start the sync timeout (use precision timer) */
      clock->widget_timer_id = g_timeout_add (interval, xfce_clock_widget_timer_sync, clock);

      /* manual update if reasonable */
      if (interval >= CLOCK_INTERVAL_SECOND)
        (*clock->update_func) (GTK_WIDGET (clock->widget));
    }
}



void
xfce_clock_widget_update_properties (XfceClock *clock)
{
  panel_return_if_fail (XFCE_IS_CLOCK (clock));
  panel_return_if_fail (clock->widget != NULL);

  /* leave when there is no widget */
  if (clock->widget == NULL)
    return;

  /* send the settings based on the clock mode */
  switch (clock->mode)
    {
      case XFCE_CLOCK_MODE_ANALOG:
        /* set settings */
        g_object_set (G_OBJECT (clock->widget),
                      "show-seconds", clock->show_seconds, NULL);
        break;

      case XFCE_CLOCK_MODE_BINARY:
        /* set settings */
        g_object_set (G_OBJECT (clock->widget),
                      "show-seconds", clock->show_seconds,
                      "true-binary", clock->true_binary, NULL);
        break;

      case XFCE_CLOCK_MODE_DIGITAL:
        /* set settings */
        g_object_set (G_OBJECT (clock->widget),
                      "digital-format", clock->digital_format, NULL);
        break;

      case XFCE_CLOCK_MODE_LCD:
        /* set settings */
        g_object_set (G_OBJECT (clock->widget),
                      "show-seconds", clock->show_seconds,
                      "show-military", clock->show_military,
                      "show-meridiem", clock->show_meridiem,
                      "flash-separators", clock->flash_separators, NULL);
        break;
    }

  /* get update interval */
  if (clock->mode == XFCE_CLOCK_MODE_DIGITAL)
    {
      /* get interval from string */
      clock->widget_interval = xfce_clock_util_interval_from_format (clock->digital_format);
    }
  else
    {
      /* interval from setting */
      if (clock->mode == XFCE_CLOCK_MODE_LCD)
        clock->widget_interval = (clock->show_seconds || clock->flash_separators) ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE;
      else
        clock->widget_interval = clock->show_seconds ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE;
    }
}



void
xfce_clock_widget_update_mode (XfceClock *clock)
{
  GtkWidget *widget = NULL;

  panel_return_if_fail (XFCE_IS_CLOCK (clock));

  /* stop runing timeout */
  if (clock->widget_timer_id)
    {
      g_source_remove (clock->widget_timer_id);
      clock->widget_timer_id = 0;
    }

  /* destroy the old widget */
  if (clock->widget)
    {
      gtk_widget_destroy (clock->widget);
      clock->widget = NULL;
    }

  /* create a new widget and set the update function */
  switch (clock->mode)
    {
      case XFCE_CLOCK_MODE_ANALOG:
        widget = xfce_clock_analog_new ();
        clock->update_func = xfce_clock_analog_update;
        break;

      case XFCE_CLOCK_MODE_BINARY:
        widget = xfce_clock_binary_new ();
        clock->update_func = xfce_clock_binary_update;
        break;

      case XFCE_CLOCK_MODE_DIGITAL:
        widget = xfce_clock_digital_new ();
        clock->update_func = xfce_clock_digital_update;
        break;

      case XFCE_CLOCK_MODE_LCD:
        widget = xfce_clock_lcd_new ();
        clock->update_func = xfce_clock_lcd_update;
        break;

      default:
        panel_assert_not_reached ();
        return;
    }

  /* set the new widget */
  if (G_LIKELY (widget))
    {
      clock->widget = widget;
      gtk_container_add (GTK_CONTAINER (clock->frame), widget);
      gtk_widget_show (widget);
    }
}



G_MODULE_EXPORT void
xfce_panel_plugin_register_types (XfcePanelModule *panel_module)
{
  panel_return_if_fail (G_IS_TYPE_MODULE (panel_module));

  /* register the types */
  xfce_clock_register_type (panel_module);
  xfce_clock_analog_register_type (panel_module);
  xfce_clock_binary_register_type (panel_module);
  xfce_clock_digital_register_type (panel_module);
  xfce_clock_lcd_register_type (panel_module);

  //g_message ("Clock types registered");
}


XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_CLOCK)
