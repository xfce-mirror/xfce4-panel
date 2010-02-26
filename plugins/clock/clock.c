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

#include <gtk/gtk.h>
#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>

#include "clock.h"
#include "clock-analog.h"
#include "clock-binary.h"
#include "clock-digital.h"
#include "clock-fuzzy.h"
#include "clock-lcd.h"
#include "clock-dialog_glade.h"



static void     clock_plugin_class_init                (ClockPluginClass      *klass);
static void     clock_plugin_init                      (ClockPlugin           *separator);
static gboolean clock_plugin_leave_notify_event        (GtkWidget             *widget,
                                                        GdkEventCrossing      *event);
static gboolean clock_plugin_enter_notify_event        (GtkWidget             *widget,
                                                        GdkEventCrossing      *event);
static void     clock_plugin_construct                 (XfcePanelPlugin       *panel_plugin);
static void     clock_plugin_free_data                 (XfcePanelPlugin       *panel_plugin);
static gboolean clock_plugin_size_changed              (XfcePanelPlugin       *panel_plugin,
                                                        gint                   size);
static void     clock_plugin_orientation_changed       (XfcePanelPlugin       *panel_plugin,
                                                        GtkOrientation         orientation);
static void     clock_plugin_save                      (XfcePanelPlugin       *panel_plugin);
static void     clock_plugin_configure_plugin          (XfcePanelPlugin       *panel_plugin);
static void     clock_plugin_property_changed          (XfconfChannel         *channel,
                                                        const gchar           *property_name,
                                                        const GValue          *value,
                                                        ClockPlugin           *plugin);
static void     clock_plugin_notify                    (GObject               *object,
                                                        GParamSpec            *pspec,
                                                        ClockPlugin           *plugin);
static void     clock_plugin_set_child                 (ClockPlugin           *plugin);
static gboolean clock_plugin_sync_timeout              (gpointer               user_data);
static void     clock_plugin_sync                      (ClockPlugin           *plugin);
static guint    clock_plugin_ms_to_next_interval       (guint                  timeout_interval);
static gboolean clock_plugin_tooltip_timeout           (gpointer               user_data);
static gboolean clock_plugin_tooltip_sync_timeout      (gpointer               user_data);
static void     clock_plugin_tooltip_sync              (ClockPlugin           *plugin);



enum _ClockPluginMode
{
  /* modes */
  CLOCK_PLUGIN_MODE_ANALOG = 0,
  CLOCK_PLUGIN_MODE_BINARY,
  CLOCK_PLUGIN_MODE_DIGITAL,
  CLOCK_PLUGIN_MODE_FUZZY,
  CLOCK_PLUGIN_MODE_LCD,

  /* defines */
  CLOCK_PLUGIN_MODE_MIN = CLOCK_PLUGIN_MODE_ANALOG,
  CLOCK_PLUGIN_MODE_MAX = CLOCK_PLUGIN_MODE_LCD,
  CLOCK_PLUGIN_MODE_DEFAULT = CLOCK_PLUGIN_MODE_DIGITAL
};

struct _ClockPluginClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;
};

struct _ClockPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* xfconf channel */
  XfconfChannel   *channel;

  /* internal child widget */
  GtkWidget       *clock;
  GtkWidget       *frame;

  /* clock widget update function and interval */
  GSourceFunc      clock_func;
  guint            clock_interval;
  guint            clock_timeout_id;

  /* clock mode */
  ClockPluginMode  mode;

  /* tooltip */
  gchar           *tooltip_format;
  guint            tooltip_timeout_id;
  guint            tooltip_interval;
};



G_DEFINE_TYPE (ClockPlugin, clock_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_CLOCK_PLUGIN);



static void
clock_plugin_class_init (ClockPluginClass *klass)
{
  GtkWidgetClass       *widget_class;
  XfcePanelPluginClass *plugin_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->leave_notify_event = clock_plugin_leave_notify_event;
  widget_class->enter_notify_event = clock_plugin_enter_notify_event;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = clock_plugin_construct;
  plugin_class->free_data = clock_plugin_free_data;
  plugin_class->save = clock_plugin_save;
  plugin_class->size_changed = clock_plugin_size_changed;
  plugin_class->orientation_changed = clock_plugin_orientation_changed;
  plugin_class->configure_plugin = clock_plugin_configure_plugin;
}



static void
clock_plugin_init (ClockPlugin *plugin)
{
  /* init */
  plugin->mode = CLOCK_PLUGIN_MODE_DEFAULT;
  plugin->clock = NULL;
  plugin->tooltip_format = NULL;
  plugin->tooltip_timeout_id = 0;
  plugin->tooltip_interval = 0;
  plugin->clock_timeout_id = 0;

  /* initialize xfconf */
  xfconf_init (NULL);

  /* show configure */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* create frame widget */
  plugin->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->frame);
  gtk_widget_show (plugin->frame);
}



static gboolean
clock_plugin_leave_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  ClockPlugin   *plugin = XFCE_CLOCK_PLUGIN (widget);
  GtkAllocation *alloc = &widget->allocation;

  /* stop a running tooltip timeout when we leave the widget */
  if (plugin->tooltip_timeout_id != 0
      && (event->x <= 0 || event->x >= alloc->width
          || event->y <= 0 || event->y >= alloc->height))
    {
      g_source_remove (plugin->tooltip_timeout_id);
      plugin->tooltip_timeout_id = 0;
    }

  return FALSE;
}



static gboolean
clock_plugin_enter_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (widget);

  /* start the tooltip timeout if needed */
  if (plugin->tooltip_timeout_id == 0)
    clock_plugin_tooltip_sync (plugin);

  return FALSE;
}



static void
clock_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin   *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  gboolean       show_frame;
  guint          mode;

  /* set the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);
  g_signal_connect (G_OBJECT (plugin->channel), "property-changed",
                    G_CALLBACK (clock_plugin_property_changed), plugin);

  /* load properties */
  mode = xfconf_channel_get_uint (plugin->channel, "/mode", CLOCK_PLUGIN_MODE_DEFAULT);
  plugin->mode = CLAMP (mode, CLOCK_PLUGIN_MODE_MIN, CLOCK_PLUGIN_MODE_MAX);

  show_frame = xfconf_channel_get_bool (plugin->channel, "/show-frame", FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), show_frame ? GTK_SHADOW_IN :
                             GTK_SHADOW_NONE);

  plugin->tooltip_format = xfconf_channel_get_string (plugin->channel, "/tooltip-format",
                                                      DEFAULT_TOOLTIP_FORMAT);

  /* create the clock widget */
  clock_plugin_set_child (plugin);
}



static void
clock_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* stop clock timeout */
  if (plugin->clock_timeout_id != 0)
    g_source_remove (plugin->clock_timeout_id);

  /* stop tooltip timeout */
  if (plugin->tooltip_timeout_id != 0)
    g_source_remove (plugin->tooltip_timeout_id);

  /* release the xfonf channel */
  g_object_unref (G_OBJECT (plugin->channel));

  /* free the tooltip string */
  g_free (plugin->tooltip_format);

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static gboolean
clock_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint             size)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  gint         clock_size;

  /* set the frame border */
  gtk_container_set_border_width (GTK_CONTAINER (plugin->frame), size > 26 ? 1 : 0);

  /* get the clock size */
  clock_size = CLAMP (size - (size > 26 ? 6 : 4), 1, 128);

  /* set the clock size */
  if (xfce_panel_plugin_get_orientation (panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (plugin->clock, -1, clock_size);
  else
    gtk_widget_set_size_request (plugin->clock, clock_size, -1);

  return TRUE;
}



static void
clock_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                  GtkOrientation   orientation)
{
  /* do a size update */
  clock_plugin_size_changed (panel_plugin, xfce_panel_plugin_get_size (panel_plugin));
}



static void
clock_plugin_save (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin   *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  GtkShadowType  shadow_type;

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* shadow type */
  shadow_type = gtk_frame_get_shadow_type (GTK_FRAME (plugin->frame));

  /* save the properties */
  xfconf_channel_set_uint (plugin->channel, "/mode", plugin->mode);
  xfconf_channel_set_bool (plugin->channel, "/show-frame", !!(shadow_type == GTK_SHADOW_IN));
  xfconf_channel_set_string (plugin->channel, "/tooltip-format", plugin->tooltip_format ?
                             plugin->tooltip_format : "");
}



static void
clock_plugin_configure_plugin_visibility (GtkComboBox *combo,
                                          GtkBuilder  *builder)
{
  guint        i, visible = 0;
  GObject     *object;
  const gchar *names[] = { "show-seconds",     /* 1 */
                           "true-binary",      /* 2 */
                           "show-military",    /* 3 */
                           "flash-separators", /* 4 */
                           "show-meridiem",    /* 5 */
                           "format-box",       /* 6 */
                           "fuzziness-box"     /* 7 */};

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_BUILDER (builder));

  /* the visible items for each mode */
  switch (gtk_combo_box_get_active (combo))
    {
      case CLOCK_PLUGIN_MODE_ANALOG:
        visible = 1 << 1;
        break;

      case CLOCK_PLUGIN_MODE_BINARY:
        visible = 1 << 1 | 1 << 2;
        break;

      case CLOCK_PLUGIN_MODE_DIGITAL:
        visible = 1 << 6;
        break;

      case CLOCK_PLUGIN_MODE_FUZZY:
        visible = 1 << 7;
        break;

      case CLOCK_PLUGIN_MODE_LCD:
        visible = 1 << 1 | 1 << 3 | 1 << 4 | 1 << 5;
        break;
    }

  /* show or hide the widgets */
  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      object = gtk_builder_get_object (builder, names[i]);
      if (PANEL_HAS_FLAG (visible, 1 << (i + 1)))
        gtk_widget_show (GTK_WIDGET (object));
      else
        gtk_widget_hide (GTK_WIDGET (object));
    }
}



static void
clock_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  GtkBuilder  *builder;
  GObject     *dialog;
  GObject     *object;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));
  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* save before we opend the dialog, so all properties exist in xfonf */
  clock_plugin_save (panel_plugin);

  /* load the dialog from the glade file */
  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, clock_dialog_glade, clock_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) xfce_panel_plugin_unblock_menu, panel_plugin);

      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);

      object = gtk_builder_get_object (builder, "mode");
      g_signal_connect (G_OBJECT (object), "changed", G_CALLBACK (clock_plugin_configure_plugin_visibility), builder);

      #define create_binding(property, type, name) \
        object = gtk_builder_get_object (builder, property); \
        panel_return_if_fail (G_IS_OBJECT (object)); \
        xfconf_g_property_bind (plugin->channel, "/" property, type, object, name)

      /* create bindings */
      create_binding ("mode", G_TYPE_UINT, "active");
      create_binding ("show-frame", G_TYPE_BOOLEAN, "active");
      create_binding ("show-seconds", G_TYPE_BOOLEAN, "active");
      create_binding ("true-binary", G_TYPE_BOOLEAN, "active");
      create_binding ("show-military", G_TYPE_BOOLEAN, "active");
      create_binding ("flash-separators", G_TYPE_BOOLEAN, "active");
      create_binding ("show-meridiem", G_TYPE_BOOLEAN, "active");
      create_binding ("fuzziness", G_TYPE_UINT, "value");

      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}



static void
clock_plugin_property_changed (XfconfChannel *channel,
                               const gchar   *property_name,
                               const GValue  *value,
                               ClockPlugin   *plugin)
{
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));
  panel_return_if_fail (plugin->channel == channel);

  if (strcmp (property_name, "/mode") == 0)
    {
      /* set new clock mode */
      plugin->mode = CLAMP (g_value_get_uint (value),
                            CLOCK_PLUGIN_MODE_MIN,
                            CLOCK_PLUGIN_MODE_MAX);

      /* update the child widget */
      clock_plugin_set_child (plugin);
    }
  else if (strcmp (property_name, "/tooltip-format") == 0)
    {
      /* cleanup */
      g_free (plugin->tooltip_format);

      /* set new tooltip */
      plugin->tooltip_format = g_value_dup_string (value);

      /* update the tooltip if a timeout is running */
      if (plugin->tooltip_timeout_id != 0)
        clock_plugin_tooltip_sync (plugin);
    }
  else if (strcmp (property_name, "/show-frame") == 0)
    {
      /* update frame shadow */
      gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame),
                                 g_value_get_boolean (value) ? GTK_SHADOW_IN :
                                 GTK_SHADOW_NONE);
    }
}



static void
clock_plugin_notify (GObject     *object,
                     GParamSpec  *pspec,
                     ClockPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));
  panel_return_if_fail (G_IS_PARAM_SPEC (pspec));

  /* bit of an ugly hack, but it works */
  if (g_param_spec_get_blurb (pspec) == NULL)
    {
      gtk_widget_queue_resize (plugin->clock);
      clock_plugin_sync (plugin);
    }
}



static void
clock_plugin_set_child (ClockPlugin *plugin)
{
  gint width = -1, height = -1;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));
  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* stop running timeout */
  if (plugin->clock_timeout_id != 0)
    {
      g_source_remove (plugin->clock_timeout_id);
      plugin->clock_timeout_id = 0;
    }

  /* destroy the child */
  if (plugin->clock)
    {
      gtk_widget_get_size_request (plugin->clock, &width, &height);
      gtk_widget_destroy (GTK_WIDGET (plugin->clock));
      plugin->clock = NULL;
    }

  /* create a new clock */
  switch (plugin->mode)
    {
      case CLOCK_PLUGIN_MODE_ANALOG:
        /* create widget */
        plugin->clock = xfce_clock_analog_new ();
        plugin->clock_func = xfce_clock_analog_update;

        /* connect binding */
        xfconf_g_property_bind (plugin->channel, "/show-seconds",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "show-seconds");
        break;

      case CLOCK_PLUGIN_MODE_BINARY:
        /* create widget */
        plugin->clock = xfce_clock_binary_new ();
        plugin->clock_func = xfce_clock_binary_update;

        /* connect bindings */
        xfconf_g_property_bind (plugin->channel, "/show-seconds",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "show-seconds");
        xfconf_g_property_bind (plugin->channel, "/true-binary",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "true-binary");
        break;

      case CLOCK_PLUGIN_MODE_DIGITAL:
        /* create widget */
        plugin->clock = xfce_clock_digital_new ();
        plugin->clock_func = xfce_clock_digital_update;

        /* connect binding */
        xfconf_g_property_bind (plugin->channel, "/digital-format",
                                G_TYPE_STRING, plugin->clock,
                                "digital-format");
        break;

      case CLOCK_PLUGIN_MODE_FUZZY:
        /* create widget */
        plugin->clock = xfce_clock_fuzzy_new ();
        plugin->clock_func = xfce_clock_fuzzy_update;

        /* connect binding */
        xfconf_g_property_bind (plugin->channel, "/fuzziness",
                                G_TYPE_UINT, plugin->clock,
                                "fuzziness");
        break;

      case CLOCK_PLUGIN_MODE_LCD:
        /* create widget */
        plugin->clock = xfce_clock_lcd_new ();
        plugin->clock_func = xfce_clock_lcd_update;

        /* connect bindings */
        xfconf_g_property_bind (plugin->channel, "/show-seconds",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "show-seconds");
        xfconf_g_property_bind (plugin->channel, "/show-military",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "show-military");
        xfconf_g_property_bind (plugin->channel, "/show-meridiem",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "show-meridiem");
        xfconf_g_property_bind (plugin->channel, "/flash-separators",
                                G_TYPE_BOOLEAN, plugin->clock,
                                "flash-separators");
        break;
    }

  /* add the widget to the plugin frame */
  gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->clock);
  gtk_widget_set_size_request (plugin->clock, width, height);
  gtk_widget_show (plugin->clock);

  /* start clock timeout */
  clock_plugin_sync (plugin);

  /* property notify */
  g_signal_connect (G_OBJECT (plugin->clock), "notify", G_CALLBACK (clock_plugin_notify), plugin);
}



static gboolean
clock_plugin_sync_timeout (gpointer user_data)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (user_data);

  panel_return_val_if_fail (GTK_IS_WIDGET (plugin->clock), FALSE);

  /* start the clock update timeout */
  plugin->clock_timeout_id = g_timeout_add (plugin->clock_interval,
                                            plugin->clock_func,
                                            plugin->clock);

  /* manual update for this interval */
  (plugin->clock_func) (plugin->clock);

  /* stop the sync timeout */
  return FALSE;
}



static void
clock_plugin_sync (ClockPlugin *plugin)
{
  guint interval;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));

  /* stop a running timeout */
  if (plugin->clock_timeout_id != 0)
    {
      g_source_remove (plugin->clock_timeout_id);
      plugin->clock_timeout_id = 0;
    }

  if (plugin->clock)
    {
      /* get the timeout interval */
      switch (plugin->mode)
        {
          case CLOCK_PLUGIN_MODE_ANALOG:
            plugin->clock_interval = xfce_clock_analog_interval (XFCE_CLOCK_ANALOG (plugin->clock));
            break;

          case CLOCK_PLUGIN_MODE_BINARY:
            plugin->clock_interval = xfce_clock_binary_interval (XFCE_CLOCK_BINARY (plugin->clock));
            break;

          case CLOCK_PLUGIN_MODE_DIGITAL:
            plugin->clock_interval = xfce_clock_digital_interval (XFCE_CLOCK_DIGITAL (plugin->clock));
            break;

          case CLOCK_PLUGIN_MODE_FUZZY:
            plugin->clock_interval = xfce_clock_fuzzy_interval (XFCE_CLOCK_FUZZY (plugin->clock));
            break;

          case CLOCK_PLUGIN_MODE_LCD:
            plugin->clock_interval = xfce_clock_lcd_interval (XFCE_CLOCK_LCD (plugin->clock));
            break;
        }

      if (plugin->clock_interval > 0)
        {
          /* get ths sync interval */
          interval = clock_plugin_ms_to_next_interval (plugin->clock_interval);

          /* start the sync timeout */
          plugin->clock_timeout_id = g_timeout_add (interval, clock_plugin_sync_timeout, plugin);
        }

      /* update the clock once */
      (plugin->clock_func) (plugin->clock);
    }
}



static guint
clock_plugin_ms_to_next_interval (guint timeout_interval)
{
  struct tm tm;
  GTimeVal  timeval;
  guint     interval;

  /* ms to the next second */
  g_get_current_time (&timeval);
  interval = 1000 - (timeval.tv_usec / 1000);

  /* leave when we use a per second interval */
  if (timeout_interval == CLOCK_INTERVAL_SECOND)
    return interval;

  /* get the current time */
  clock_plugin_get_localtime (&tm);

  /* add the interval time to the next update */
  switch (timeout_interval)
    {
      /* ms to next hour */
      case CLOCK_INTERVAL_HOUR:
        interval += (60 - tm.tm_min) * CLOCK_INTERVAL_MINUTE;

        /* fall-through to add the minutes */

      /* ms to next minute */
      case CLOCK_INTERVAL_MINUTE:
        interval += (60 - tm.tm_sec) * CLOCK_INTERVAL_SECOND;
        break;
    }

  return interval;
}



static gboolean
clock_plugin_tooltip_timeout (gpointer user_data)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (user_data);
  gchar       *string;
  struct tm    tm;

  /* get the local time */
  clock_plugin_get_localtime (&tm);

  /* get the string */
  string = clock_plugin_strdup_strftime (plugin->tooltip_format, &tm);

  /* set the tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (plugin), string);

  /* make sure the tooltip is up2date */
  gtk_widget_trigger_tooltip_query (GTK_WIDGET (plugin));

  /* cleanup */
  g_free (string);

  /* keep the timeout running */
  return TRUE;
}



static gboolean
clock_plugin_tooltip_sync_timeout (gpointer user_data)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (user_data);

  /* start the real timeout interval, since we're synced now */
  plugin->tooltip_timeout_id = g_timeout_add (plugin->tooltip_interval, clock_plugin_tooltip_timeout, plugin);

  /* manual update for this timeout (if really needed) */
  if (plugin->tooltip_interval > CLOCK_INTERVAL_SECOND)
    clock_plugin_tooltip_timeout (clock);

  /* stop the sync timeout */
  return FALSE;
}



static void
clock_plugin_tooltip_sync (ClockPlugin *plugin)
{
  guint interval;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));

  /* stop a running timeout */
  if (plugin->tooltip_timeout_id != 0)
    {
      g_source_remove (plugin->tooltip_timeout_id);
      plugin->tooltip_timeout_id = 0;
    }

  /* get the timeout from the string */
  plugin->tooltip_interval = clock_plugin_interval_from_format (plugin->tooltip_format);

  /* only start the timeout if needed */
  if (plugin->tooltip_interval > 0)
    {
      /* get the interval to sync with the next interval */
      interval = clock_plugin_ms_to_next_interval (plugin->tooltip_interval);

      /* start the sync timeout */
      plugin->tooltip_timeout_id = g_timeout_add (interval, clock_plugin_tooltip_sync_timeout, plugin);
    }

  /* update the tooltip */
  clock_plugin_tooltip_timeout (plugin);
}



void
clock_plugin_get_localtime (struct tm *tm)
{
  time_t now = time (0);

#ifndef HAVE_LOCALTIME_R
  struct tm *tmbuf;

  tmbuf = localtime (&now);
  *tm = *tmbuf;
#else
  localtime_r (&now, tm);
#endif

#ifdef USE_DEBUG_TIME
  xfce_clock_util_get_debug_localtime (tm);
#endif
}



gchar *
clock_plugin_strdup_strftime (const gchar     *format,
                              const struct tm *tm)
{
  gchar *converted, *result;
  gsize  length;
  gchar  buffer[512];

  /* leave when format is null */
  if (G_UNLIKELY (!IS_STRING (format)))
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



guint
clock_plugin_interval_from_format (const gchar *format)
{
  const gchar *p;
  guint        interval = CLOCK_INTERVAL_HOUR;

  if (G_UNLIKELY (!IS_STRING (format)))
      return 0;

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

