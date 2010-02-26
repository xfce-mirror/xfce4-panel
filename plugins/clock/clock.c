/*
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
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
#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-builder.h>

#include "clock.h"
#include "clock-analog.h"
#include "clock-binary.h"
#include "clock-digital.h"
#include "clock-fuzzy.h"
#include "clock-lcd.h"
#include "clock-dialog_ui.h"

#define DEFAULT_TOOLTIP_FORMAT "%A %d %B %Y"



static void     clock_plugin_get_property              (GObject               *object,
                                                        guint                  prop_id,
                                                        GValue                *value,
                                                        GParamSpec            *pspec);
static void     clock_plugin_set_property              (GObject               *object,
                                                        guint                  prop_id,
                                                        const GValue          *value,
                                                        GParamSpec            *pspec);
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
static void     clock_plugin_configure_plugin          (XfcePanelPlugin       *panel_plugin);

static void     clock_plugin_set_mode                  (ClockPlugin           *plugin);
static gboolean clock_plugin_tooltip                   (gpointer               user_data);
static gboolean clock_plugin_timeout_running           (gpointer               user_data);
static void     clock_plugin_timeout_destroyed         (gpointer               user_data);
static gboolean clock_plugin_timeout_sync              (gpointer               user_data);



enum
{
  PROP_0,
  PROP_MODE,
  PROP_SHOW_FRAME,
  PROP_TOOLTIP_FORMAT
};

typedef enum
{
  CLOCK_PLUGIN_MODE_ANALOG = 0,
  CLOCK_PLUGIN_MODE_BINARY,
  CLOCK_PLUGIN_MODE_DIGITAL,
  CLOCK_PLUGIN_MODE_FUZZY,
  CLOCK_PLUGIN_MODE_LCD,

  /* defines */
  CLOCK_PLUGIN_MODE_MIN = CLOCK_PLUGIN_MODE_ANALOG,
  CLOCK_PLUGIN_MODE_MAX = CLOCK_PLUGIN_MODE_LCD,
  CLOCK_PLUGIN_MODE_DEFAULT = CLOCK_PLUGIN_MODE_DIGITAL
}
ClockPluginMode;

struct _ClockPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _ClockPlugin
{
  XfcePanelPlugin __parent__;

  GtkWidget          *clock;
  GtkWidget          *frame;

  ClockPluginMode     mode;

  gchar              *tooltip_format;
  ClockPluginTimeout *tooltip_timeout;
};

struct _ClockPluginTimeout
{
  guint       interval;
  GSourceFunc function;
  gpointer    data;
  guint       timeout_id;
  guint       restart : 1;
};

typedef struct
{
  ClockPlugin *plugin;
  GtkBuilder  *builder;
}
ClockPluginDialog;



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ClockPlugin, clock_plugin,
  xfce_clock_analog_register_type,
  xfce_clock_binary_register_type,
  xfce_clock_digital_register_type,
  xfce_clock_fuzzy_register_type,
  xfce_clock_lcd_register_type)



static void
clock_plugin_class_init (ClockPluginClass *klass)
{
  GObjectClass         *gobject_class;
  GtkWidgetClass       *widget_class;
  XfcePanelPluginClass *plugin_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = clock_plugin_set_property;
  gobject_class->get_property = clock_plugin_get_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->leave_notify_event = clock_plugin_leave_notify_event;
  widget_class->enter_notify_event = clock_plugin_enter_notify_event;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = clock_plugin_construct;
  plugin_class->free_data = clock_plugin_free_data;
  plugin_class->size_changed = clock_plugin_size_changed;
  plugin_class->orientation_changed = clock_plugin_orientation_changed;
  plugin_class->configure_plugin = clock_plugin_configure_plugin;

  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_uint ("mode",
                                                      NULL, NULL,
                                                      CLOCK_PLUGIN_MODE_MIN,
                                                      CLOCK_PLUGIN_MODE_MAX,
                                                      CLOCK_PLUGIN_MODE_DEFAULT,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_FRAME,
                                   g_param_spec_boolean ("show-frame",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_FORMAT,
                                   g_param_spec_string ("tooltip-format",
                                                        NULL, NULL,
                                                        DEFAULT_TOOLTIP_FORMAT,
                                                        EXO_PARAM_READWRITE));
}



static void
clock_plugin_init (ClockPlugin *plugin)
{
  plugin->mode = CLOCK_PLUGIN_MODE_DEFAULT;
  plugin->clock = NULL;
  plugin->tooltip_format = g_strdup (DEFAULT_TOOLTIP_FORMAT);
  plugin->tooltip_timeout = NULL;

  plugin->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->frame);
  gtk_widget_show (plugin->frame);
}



static void
clock_plugin_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClockPlugin   *plugin = XFCE_CLOCK_PLUGIN (object);
  GtkShadowType  shadow_type;

  switch (prop_id)
    {
      case PROP_MODE:
        g_value_set_uint (value, plugin->mode);
        break;

      case PROP_SHOW_FRAME:
        shadow_type = gtk_frame_get_shadow_type (GTK_FRAME (plugin->frame));
        g_value_set_boolean (value, shadow_type == GTK_SHADOW_IN);
        break;

      case PROP_TOOLTIP_FORMAT:
        g_value_set_string (value, plugin->tooltip_format);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
clock_plugin_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (object);

  switch (prop_id)
    {
      case PROP_MODE:
        if (plugin->mode != g_value_get_uint (value))
          {
            plugin->mode = g_value_get_uint (value);
            clock_plugin_set_mode (plugin);
          }
        break;

      case PROP_SHOW_FRAME:
        gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame),
            g_value_get_boolean (value) ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
        break;

      case PROP_TOOLTIP_FORMAT:
        g_free (plugin->tooltip_format);
        plugin->tooltip_format = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static gboolean
clock_plugin_leave_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (widget);

  /* stop a running tooltip timeout when we leave the widget */
  if (plugin->tooltip_timeout != NULL)
    {
      clock_plugin_timeout_free (plugin->tooltip_timeout);
      plugin->tooltip_timeout = NULL;
    }

  return FALSE;
}



static gboolean
clock_plugin_enter_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (widget);
  guint        interval;

  /* start the tooltip timeout if needed */
  if (plugin->tooltip_timeout == NULL)
    {
      interval = clock_plugin_interval_from_format (plugin->tooltip_format);
      plugin->tooltip_timeout = clock_plugin_timeout_new (interval, clock_plugin_tooltip, plugin);
    }

  return FALSE;
}



static void
clock_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin         *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "mode", G_TYPE_UINT },
    { "show-frame", G_TYPE_BOOLEAN },
    { "tooltip-format", G_TYPE_STRING },
    { NULL }
  };

  /* show configure */
  xfce_panel_plugin_menu_show_configure (panel_plugin);

  /* connect all properties */
  panel_properties_bind (NULL, G_OBJECT (panel_plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* make sure a mode is set */
  if (plugin->mode == CLOCK_PLUGIN_MODE_DEFAULT)
    clock_plugin_set_mode (plugin);
}



static void
clock_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);

  if (plugin->tooltip_timeout != NULL)
    clock_plugin_timeout_free (plugin->tooltip_timeout);

  g_free (plugin->tooltip_format);
}



static gboolean
clock_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint             size)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  gint         clock_size;

  if (plugin->clock == NULL)
    return TRUE;

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
clock_plugin_configure_plugin_mode_changed (GtkComboBox       *combo,
                                            ClockPluginDialog *dialog)
{
  guint     i, active, mode;
  GObject *object;
  struct {
    const gchar *widget;
    const gchar *binding;
    const gchar *property;
  } names[] = {
    { "show-seconds", "show-seconds", "active" },
    { "true-binary", "true-binary", "active" },
    { "show-military", "show-military", "active" },
    { "flash-separators", "flash-separators", "active" },
    { "show-meridiem", "show-meridiem", "active" },
    { "format-box", "digital-format", "active" },
    { "fuzziness-box", "fuzziness", "value" }
  };

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (dialog->plugin));

  /* the active items for each mode */
  mode = gtk_combo_box_get_active (combo);
  switch (mode)
    {
      case CLOCK_PLUGIN_MODE_ANALOG:
        active = 1 << 1;
        break;

      case CLOCK_PLUGIN_MODE_BINARY:
        active = 1 << 1 | 1 << 2;
        break;

      case CLOCK_PLUGIN_MODE_DIGITAL:
        active = 1 << 6;
        break;

      case CLOCK_PLUGIN_MODE_FUZZY:
        active = 1 << 7;
        break;

      case CLOCK_PLUGIN_MODE_LCD:
        active = 1 << 1 | 1 << 3 | 1 << 4 | 1 << 5;
        break;
    }

  /* show or hide the dialog widgets */
  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      object = gtk_builder_get_object (dialog->builder, names[i].widget);
      panel_return_if_fail (G_IS_OBJECT (object));
      if (PANEL_HAS_FLAG (active, 1 << (i + 1)))
        gtk_widget_show (GTK_WIDGET (object));
      else
        gtk_widget_hide (GTK_WIDGET (object));
    }

  /* make sure the new mode is set */
  if (dialog->plugin->mode != mode)
    g_object_set (G_OBJECT (dialog->plugin), "mode", mode, NULL);
  panel_return_if_fail (G_IS_OBJECT (dialog->plugin->clock));

  /* connect the exo bindings */
  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (PANEL_HAS_FLAG (active, 1 << (i + 1)))
        {
          object = gtk_builder_get_object (dialog->builder, names[i].binding);
          panel_return_if_fail (G_IS_OBJECT (object));
          exo_mutual_binding_new (G_OBJECT (dialog->plugin->clock), names[i].binding,
                                  G_OBJECT (object), names[i].property);
        }
    }
}



static void
clock_plugin_configure_plugin_free (gpointer  user_data,
                                    GObject  *where_the_object_was)
{
  g_slice_free (ClockPluginDialog, user_data);
}



static void
clock_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin       *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  ClockPluginDialog *dialog;
  GtkBuilder        *builder;
  GObject           *window;
  GObject           *object;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));

  /* setup the dialog */
  builder = panel_builder_new (panel_plugin, clock_dialog_ui,
                               clock_dialog_ui_length, &window);
  if (G_UNLIKELY (builder == NULL))
    return;

  dialog = g_slice_new0 (ClockPluginDialog);
  dialog->plugin = plugin;
  dialog->builder = builder;
  g_object_weak_ref (G_OBJECT (builder), clock_plugin_configure_plugin_free, dialog);

  object = gtk_builder_get_object (builder, "mode");
  g_signal_connect (G_OBJECT (object), "changed",
      G_CALLBACK (clock_plugin_configure_plugin_mode_changed), dialog);
  exo_mutual_binding_new (G_OBJECT (plugin), "mode",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "show-frame");
  exo_mutual_binding_new (G_OBJECT (plugin), "show-frame",
                          G_OBJECT (object), "active");

  gtk_widget_show (GTK_WIDGET (window));
}



static void
clock_plugin_set_mode (ClockPlugin *plugin)
{
  const PanelProperty properties[][5] =
  {
    { /* analog */
      { "show-seconds", G_TYPE_BOOLEAN },
      { NULL },
    },
    { /* binary */
      { "show-seconds", G_TYPE_BOOLEAN },
      { "true-binary", G_TYPE_BOOLEAN },
      { NULL },
    },
    { /* digital */
      { "digital-format", G_TYPE_STRING },
      { NULL },
    },
    { /* fuzzy */
      { "fuzziness", G_TYPE_UINT },
      { NULL },
    },
    { /* lcd */
      { "show-seconds", G_TYPE_BOOLEAN },
      { "show-military", G_TYPE_BOOLEAN },
      { "show-meridiem", G_TYPE_BOOLEAN },
      { "flash-separators", G_TYPE_BOOLEAN },
      { NULL },
    }
  };

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));

  if (plugin->clock != NULL)
    gtk_widget_destroy (plugin->clock);

  /* create a new clock */
  if (plugin->mode == CLOCK_PLUGIN_MODE_ANALOG)
    plugin->clock = xfce_clock_analog_new ();
  else if (plugin->mode == CLOCK_PLUGIN_MODE_BINARY)
    plugin->clock = xfce_clock_binary_new ();
  else if (plugin->mode == CLOCK_PLUGIN_MODE_DIGITAL)
    plugin->clock = xfce_clock_digital_new ();
  else if (plugin->mode == CLOCK_PLUGIN_MODE_FUZZY)
    plugin->clock = xfce_clock_fuzzy_new ();
  else
    plugin->clock = xfce_clock_lcd_new ();

  panel_properties_bind (NULL, G_OBJECT (plugin->clock),
                         xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (plugin)),
                         properties[plugin->mode], FALSE);

  gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->clock);
  clock_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
      xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
  gtk_widget_show (plugin->clock);
}



static gboolean
clock_plugin_tooltip (gpointer user_data)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (user_data);
  gchar       *string;
  struct tm    tm;

  /* get the local time */
  clock_plugin_get_localtime (&tm);

  /* set the tooltip */
  string = clock_plugin_strdup_strftime (plugin->tooltip_format, &tm);
  gtk_widget_set_tooltip_text (GTK_WIDGET (plugin), string);
  g_free (string);

  /* make sure the tooltip is up2date */
  gtk_widget_trigger_tooltip_query (GTK_WIDGET (plugin));

  /* keep the timeout running */
  return TRUE;
}



static gboolean
clock_plugin_timeout_running (gpointer user_data)
{
  ClockPluginTimeout *timeout = user_data;
  gboolean            result;
  struct tm           tm;

  result = (timeout->function) (timeout->data);

  /* check if the timeout still runs in time if updating once a minute */
  if (result && timeout->interval == CLOCK_INTERVAL_MINUTE)
    {
      /* sync again when we don't run on time */
      clock_plugin_get_localtime (&tm);
      timeout->restart = tm.tm_sec != 0;
    }

  return result && !timeout->restart;
}



static void
clock_plugin_timeout_destroyed (gpointer user_data)
{
  ClockPluginTimeout *timeout = user_data;

  timeout->timeout_id = 0;

  if (G_UNLIKELY (timeout->restart))
    clock_plugin_timeout_set_interval (timeout, timeout->interval);
}



static gboolean
clock_plugin_timeout_sync (gpointer user_data)
{
  ClockPluginTimeout *timeout = user_data;

  /* run the user function */
  if ((timeout->function) (timeout->data))
    {
      /* start the real timeout */
      timeout->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, timeout->interval,
                                                        clock_plugin_timeout_running, timeout,
                                                        clock_plugin_timeout_destroyed);
    }
  else
    {
      timeout->timeout_id = 0;
    }

  /* stop the sync timeout */
  return FALSE;
}



ClockPluginTimeout *
clock_plugin_timeout_new (guint       interval,
                          GSourceFunc function,
                          gpointer    data)
{
  ClockPluginTimeout *timeout;

  panel_return_val_if_fail (interval > 0, NULL);
  panel_return_val_if_fail (function != NULL, NULL);

  timeout = g_slice_new0 (ClockPluginTimeout);
  timeout->interval = 0;
  timeout->function = function;
  timeout->data = data;
  timeout->timeout_id = 0;
  timeout->restart = FALSE;

  clock_plugin_timeout_set_interval (timeout, interval);

  return timeout;
}



void
clock_plugin_timeout_set_interval (ClockPluginTimeout *timeout,
                                   guint               interval)
{
  struct tm tm;
  guint     next_interval;
  gboolean  restart = timeout->restart;

  panel_return_if_fail (timeout != NULL);
  panel_return_if_fail (interval > 0);

  /* leave if nothing changed and we're not restarting */
  if (!restart && timeout->interval == interval)
    return;
  timeout->interval = interval;
  timeout->restart = FALSE;

  /* stop running timeout */
  if (G_LIKELY (timeout->timeout_id != 0))
    g_source_remove (timeout->timeout_id);
  timeout->timeout_id = 0;

  /* run function when not restarting, leave if it returns false */
  if (!restart && !(timeout->function) (timeout->data))
    return;

  /* get the seconds to the next internal */
  if (interval == CLOCK_INTERVAL_MINUTE)
    {
      clock_plugin_get_localtime (&tm);
      next_interval = 60 - tm.tm_sec;
    }
  else
    {
      next_interval = 0;
    }

  if (next_interval > 0)
    {
      /* start the sync timeout */
      timeout->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, next_interval,
                                                        clock_plugin_timeout_sync,
                                                        timeout, NULL);
    }
  else
    {
      /* directly start running the normal timeout */
      timeout->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, interval,
                                                        clock_plugin_timeout_running, timeout,
                                                        clock_plugin_timeout_destroyed);
    }
}



void
clock_plugin_timeout_free (ClockPluginTimeout *timeout)
{
  panel_return_if_fail (timeout != NULL);

  timeout->restart = FALSE;
  if (G_LIKELY (timeout->timeout_id != 0))
    g_source_remove (timeout->timeout_id);
  g_slice_free (ClockPluginTimeout, timeout);
}



void
clock_plugin_get_localtime (struct tm *tm)
{
  time_t now = time (NULL);

#ifndef HAVE_LOCALTIME_R
  struct tm *tmbuf;

  tmbuf = localtime (&now);
  *tm = *tmbuf;
#else
  localtime_r (&now, tm);
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

  if (G_UNLIKELY (!IS_STRING (format)))
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
