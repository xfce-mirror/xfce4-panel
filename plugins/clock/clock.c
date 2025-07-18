/*
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
 * Copyright (C) 2012      Guido Berhoerster <gber@opensuse.org>
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

#include "clock-analog.h"
#include "clock-binary.h"
#include "clock-digital.h"
#include "clock-fuzzy.h"
#include "clock-lcd.h"
#include "clock-sleep-monitor.h"
#include "clock-time.h"
#include "clock.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"
#include "common/panel-xfconf.h"
#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#ifdef HAVE_MATH_H
#include <math.h>
#endif

/* TRANSLATORS: adjust this accordingly for your locale format */
#define DEFAULT_TOOLTIP_FORMAT NC_ ("Date", "%A %d %B %Y")

/* Please adjust the following command to match your distribution */
/* e.g. "time-admin" */
#define DEFAULT_TIME_CONFIG_TOOL "time-admin"

/* Use the posix directory for the names. If people want a time based on posix or
 * right time, they can prepend that manually in the entry */
#define ZONEINFO_DIR "/usr/share/zoneinfo/"



static void
clock_plugin_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec);
static void
clock_plugin_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec);
static gboolean
clock_plugin_leave_notify_event (GtkWidget *widget,
                                 GdkEventCrossing *event,
                                 ClockPlugin *plugin);
static gboolean
clock_plugin_enter_notify_event (GtkWidget *widget,
                                 GdkEventCrossing *event,
                                 ClockPlugin *plugin);
static gboolean
clock_plugin_button_press_event (GtkWidget *widget,
                                 GdkEventButton *event,
                                 ClockPlugin *plugin);
static void
clock_plugin_construct (XfcePanelPlugin *panel_plugin);
static void
clock_plugin_free_data (XfcePanelPlugin *panel_plugin);
static gboolean
clock_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint size);
static void
clock_plugin_mode_changed (XfcePanelPlugin *panel_plugin,
                           XfcePanelPluginMode mode);
static void
clock_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void
clock_plugin_set_mode (ClockPlugin *plugin);
static void
clock_plugin_popup_calendar (ClockPlugin *plugin);
static gboolean
clock_plugin_tooltip (gpointer user_data);
static void
clock_plugin_set_calendar_options (ClockPlugin *plugin);
static void
clock_plugin_get_preferred_width_for_height (GtkWidget *widget,
                                             gint height,
                                             gint *minimum_width,
                                             gint *natural_width);
static void
clock_plugin_get_preferred_height_for_width (GtkWidget *widget,
                                             gint width,
                                             gint *minimum_height,
                                             gint *natural_height);
static GtkSizeRequestMode
clock_plugin_get_request_mode (GtkWidget *widget);



enum
{
  PROP_0,
  PROP_MODE,
  PROP_TOOLTIP_FORMAT,
  PROP_COMMAND,
  PROP_SHOW_WEEK_NUMBERS,
  PROP_ROTATE_VERTICALLY,
  PROP_TIME_CONFIG_TOOL
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
} ClockPluginMode;

struct _ClockPlugin
{
  XfcePanelPlugin __parent__;

  GtkWidget *clock;
  GtkWidget *button;

  GtkWidget *calendar_window;
  GtkWidget *calendar;

  GObject *settings_dialog;
  gchar *command;
  guint show_week_numbers : 1;
  ClockPluginMode mode;
  guint rotate_vertically : 1;

  gchar *tooltip_format;
  ClockTimeTimeout *tooltip_timeout;

  gchar *time_config_tool;
  ClockTime *time;

  /* may be NULL if no sleep monitor could be instatiated */
  ClockSleepMonitor *sleep_monitor;
};

typedef struct
{
  ClockPlugin *plugin;
  GtkBuilder *builder;
  guint zonecompletion_idle;
} ClockPluginDialog;

static const gchar *tooltip_formats[] = {
  DEFAULT_TOOLTIP_FORMAT,
  "%x",
  N_ ("Week %V"),
  NULL
};

static const gchar *digital_time_formats[] = {
  DEFAULT_DIGITAL_TIME_FORMAT,
  "%T",
  "%_H:%M",
  "%H:%M",
  "%I:%M %p",
  "%H:%M:%S",
  "%l:%M:%S %P",
  NULL
};

static const gchar *digital_date_formats[] = {
  DEFAULT_DIGITAL_DATE_FORMAT,
  "%Y %B %d",
  "%m/%d/%Y",
  "%B %d, %Y",
  "%b %d, %Y",
  "%d/%m/%Y",
  "%d %B %Y",
  "%d %b %Y",
  "%A, %B %d, %Y",
  "%a, %b %d, %Y",
  "%A, %d %B %Y",
  "%a, %d %b %Y",
  NULL
};

enum
{
  COLUMN_FORMAT,
  COLUMN_SEPARATOR,
  COLUMN_TEXT,
  N_COLUMNS
};



XFCE_PANEL_DEFINE_PLUGIN (ClockPlugin, clock_plugin)



static void
clock_plugin_class_init (ClockPluginClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  XfcePanelPluginClass *plugin_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = clock_plugin_set_property;
  gobject_class->get_property = clock_plugin_get_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->get_preferred_width_for_height = clock_plugin_get_preferred_width_for_height;
  gtkwidget_class->get_preferred_height_for_width = clock_plugin_get_preferred_height_for_width;
  gtkwidget_class->get_request_mode = clock_plugin_get_request_mode;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = clock_plugin_construct;
  plugin_class->free_data = clock_plugin_free_data;
  plugin_class->size_changed = clock_plugin_size_changed;
  plugin_class->mode_changed = clock_plugin_mode_changed;
  plugin_class->configure_plugin = clock_plugin_configure_plugin;

  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_uint ("mode",
                                                      NULL, NULL,
                                                      CLOCK_PLUGIN_MODE_MIN,
                                                      CLOCK_PLUGIN_MODE_MAX,
                                                      CLOCK_PLUGIN_MODE_DEFAULT,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_FORMAT,
                                   g_param_spec_string ("tooltip-format",
                                                        NULL, NULL,
                                                        DEFAULT_TOOLTIP_FORMAT,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_ROTATE_VERTICALLY,
                                   g_param_spec_boolean ("rotate-vertically",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND,
                                   g_param_spec_string ("command",
                                                        NULL, NULL, NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_WEEK_NUMBERS,
                                   g_param_spec_boolean ("show-week-numbers",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_TIME_CONFIG_TOOL,
                                   g_param_spec_string ("time-config-tool",
                                                        NULL, NULL,
                                                        DEFAULT_TIME_CONFIG_TOOL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
clock_plugin_init (ClockPlugin *plugin)
{
  plugin->mode = CLOCK_PLUGIN_MODE_DEFAULT;
  plugin->clock = NULL;
  plugin->tooltip_format = g_strdup (DEFAULT_TOOLTIP_FORMAT);
  plugin->tooltip_timeout = NULL;
  plugin->command = g_strdup ("");
  plugin->show_week_numbers = TRUE;
  plugin->time_config_tool = g_strdup (DEFAULT_TIME_CONFIG_TOOL);
  plugin->rotate_vertically = TRUE;
  plugin->time = clock_time_new ();
  plugin->sleep_monitor = clock_sleep_monitor_create ();

  plugin->button = xfce_panel_create_toggle_button ();
  /* xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button); */
  gtk_container_add (GTK_CONTAINER (plugin), plugin->button);
  gtk_widget_set_name (plugin->button, "clock-button");
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  /* Have to handle all events in the button object rather than the plugin.
   * Otherwise, default handlers will block the events. */
  g_signal_connect (G_OBJECT (plugin->button), "button-press-event",
                    G_CALLBACK (clock_plugin_button_press_event), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "enter-notify-event",
                    G_CALLBACK (clock_plugin_enter_notify_event), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "leave-notify-event",
                    G_CALLBACK (clock_plugin_leave_notify_event), plugin);
  gtk_widget_show (plugin->button);
}



static void
clock_plugin_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_MODE:
      g_value_set_uint (value, plugin->mode);
      break;

    case PROP_TOOLTIP_FORMAT:
      g_value_set_string (value, plugin->tooltip_format);
      break;

    case PROP_COMMAND:
      g_value_set_string (value, plugin->command);
      break;

    case PROP_SHOW_WEEK_NUMBERS:
      g_value_set_boolean (value, plugin->show_week_numbers);
      break;

    case PROP_TIME_CONFIG_TOOL:
      g_value_set_string (value, plugin->time_config_tool);
      break;

    case PROP_ROTATE_VERTICALLY:
      g_value_set_boolean (value, plugin->rotate_vertically);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
clock_plugin_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (object);
  gboolean rotate_vertically;

  switch (prop_id)
    {
    case PROP_MODE:
      if (plugin->mode != g_value_get_uint (value))
        {
          plugin->mode = g_value_get_uint (value);
          clock_plugin_set_mode (plugin);
        }
      break;

    case PROP_TOOLTIP_FORMAT:
      g_free (plugin->tooltip_format);
      plugin->tooltip_format = g_value_dup_string (value);
      break;

    case PROP_COMMAND:
      g_free (plugin->command);
      plugin->command = g_value_dup_string (value);
      /*
       * ensure the calendar window is hidden since a non-empty command disables
       * toggling
       */
      if (plugin->calendar_window != NULL)
        gtk_widget_hide (plugin->calendar_window);
      break;

    case PROP_SHOW_WEEK_NUMBERS:
      plugin->show_week_numbers = g_value_get_boolean (value);
      if (plugin->calendar_window != NULL)
        clock_plugin_set_calendar_options (plugin);
      break;

    case PROP_TIME_CONFIG_TOOL:
      g_free (plugin->time_config_tool);
      plugin->time_config_tool = g_value_dup_string (value);
      break;

    case PROP_ROTATE_VERTICALLY:
      rotate_vertically = g_value_get_boolean (value);
      if (plugin->rotate_vertically != rotate_vertically)
        {
          plugin->rotate_vertically = rotate_vertically;
          clock_plugin_set_mode (plugin);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
clock_plugin_leave_notify_event (GtkWidget *widget,
                                 GdkEventCrossing *event,
                                 ClockPlugin *plugin)
{
  /* stop a running tooltip timeout when we leave the widget */
  if (plugin->tooltip_timeout != NULL)
    {
      clock_time_timeout_free (plugin->tooltip_timeout);
      plugin->tooltip_timeout = NULL;
    }

  return FALSE;
}



static gboolean
clock_plugin_enter_notify_event (GtkWidget *widget,
                                 GdkEventCrossing *event,
                                 ClockPlugin *plugin)
{
  guint interval;

  /* start the tooltip timeout if needed */
  /* don't bother with hooking up the sleep monitor here */
  if (plugin->tooltip_timeout == NULL)
    {
      interval = clock_time_interval_from_format (plugin->tooltip_format);
      plugin->tooltip_timeout = clock_time_timeout_new (interval, plugin->time, /*sleep_monitor*/ NULL,
                                                        G_CALLBACK (clock_plugin_tooltip), plugin);
    }

  return FALSE;
}



static gboolean
clock_plugin_button_press_event (GtkWidget *widget,
                                 GdkEventButton *event,
                                 ClockPlugin *plugin)
{
  GError *error = NULL;

  if (event->button == 1 || event->button == 2)
    {
      if (event->type == GDK_BUTTON_PRESS && xfce_str_is_empty (plugin->command))
        {
          /* toggle calendar window visibility */
          if (plugin->calendar_window == NULL
              || !gtk_widget_get_visible (GTK_WIDGET (plugin->calendar_window)))
            {
              clock_plugin_popup_calendar (plugin);
            }
          else
            {
              /* should not be reached if everything went well when showing the
               * the calendar window, it's a fallback in case the grab fails */
              gtk_widget_hide (plugin->calendar_window);
            }

          return TRUE;
        }
      else if (event->type == GDK_BUTTON_PRESS
               && !xfce_str_is_empty (plugin->command))
        {
          /* launch command */
          if (!xfce_spawn_command_line (gtk_widget_get_screen (widget),
                                        plugin->command, FALSE,
                                        FALSE, TRUE, &error))
            {
              xfce_dialog_show_error (NULL, error,
                                      _("Failed to execute clock command"));
              g_error_free (error);
            }

          return TRUE;
        }
      return TRUE;
    }

  /* bypass GTK_TOGGLE_BUTTON's handler and go directly to the plugin's one */
  return (*GTK_WIDGET_CLASS (clock_plugin_parent_class)->button_press_event) (GTK_WIDGET (plugin), event);
}



static void
clock_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (panel_plugin);
  const PanelProperty properties[] = {
    { "mode", G_TYPE_UINT },
    { "tooltip-format", G_TYPE_STRING },
    { "command", G_TYPE_STRING },
    { "show-week-numbers", G_TYPE_BOOLEAN },
    { "rotate-vertically", G_TYPE_BOOLEAN },
    { "time-config-tool", G_TYPE_STRING },
    { NULL }
  };

  const PanelProperty time_properties[] = {
    { "timezone", G_TYPE_STRING },
    { NULL }
  };

  /* show configure */
  xfce_panel_plugin_menu_show_configure (panel_plugin);

  /* connect all properties */
  panel_properties_bind (NULL, G_OBJECT (panel_plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  panel_properties_bind (NULL, G_OBJECT (plugin->time),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         time_properties, FALSE);

  /* make sure a mode is set */
  if (plugin->mode == CLOCK_PLUGIN_MODE_DEFAULT)
    clock_plugin_set_mode (plugin);
}



static void
clock_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (panel_plugin);

  if (plugin->tooltip_timeout != NULL)
    clock_time_timeout_free (plugin->tooltip_timeout);

  if (plugin->calendar_window != NULL)
    gtk_widget_destroy (plugin->calendar_window);

  g_object_unref (G_OBJECT (plugin->time));

  if (plugin->sleep_monitor != NULL)
    g_object_unref (G_OBJECT (plugin->sleep_monitor));

  g_free (plugin->tooltip_format);
  g_free (plugin->time_config_tool);
  g_free (plugin->command);
}



static gboolean
clock_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint size)
{
  return TRUE;
}



static void
clock_plugin_mode_changed (XfcePanelPlugin *panel_plugin,
                           XfcePanelPluginMode mode)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (panel_plugin);
  GtkOrientation orientation;
  GtkOrientation container_orientation;

  container_orientation = (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL) ? GTK_ORIENTATION_HORIZONTAL
                                                                      : GTK_ORIENTATION_VERTICAL;
  g_object_set (G_OBJECT (plugin->clock), "container-orientation", container_orientation, NULL);

  if (plugin->rotate_vertically)
    {
      orientation = (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_VERTICAL
                                                              : GTK_ORIENTATION_HORIZONTAL;
      g_object_set (G_OBJECT (plugin->clock), "orientation", orientation, NULL);
    }
}



static void
clock_plugin_get_preferred_height_for_width (GtkWidget *widget,
                                             gint width,
                                             gint *minimum_height,
                                             gint *natural_height)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (widget);

  gtk_widget_get_preferred_height_for_width (plugin->button, width, minimum_height, natural_height);
}



static void
clock_plugin_get_preferred_width_for_height (GtkWidget *widget,
                                             gint height,
                                             gint *minimum_width,
                                             gint *natural_width)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (widget);

  gtk_widget_get_preferred_width_for_height (plugin->button, height, minimum_width, natural_width);
}



static GtkSizeRequestMode
clock_plugin_get_request_mode (GtkWidget *widget)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN (widget);

  if (xfce_panel_plugin_get_mode (panel_plugin) == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
    return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
  else
    return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}



static void
clock_plugin_configure_plugin_mode_changed (GtkComboBox *combo,
                                            ClockPluginDialog *dialog)
{
  guint i, active, mode;
  GObject *object;
  struct
  {
    const gchar *widget;
    const gchar *binding;
    const gchar *property;
  } names[] = {
    { "show-seconds", "show-seconds", "active" },
    { "binary-box", "binary-mode", "active" },
    { "show-military", "show-military", "active" },
    { "flash-separators", "flash-separators", "active" },
    { "show-meridiem", "show-meridiem", "active" },
    { "digital-box", "digital-layout", "active" },
    { "digital-box", "digital-date-format", "text" },
    { "digital-box", "digital-time-format", "text" },
    { "digital-box", "digital-date-font", "font-name" },
    { "digital-box", "digital-time-font", "font-name" },
    { "fuzziness-box", "fuzziness", "value" },
    { "show-inactive", "show-inactive", "active" },
    { "show-grid", "show-grid", "active" },
  };

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (CLOCK_IS_PLUGIN (dialog->plugin));

  /* the active items for each mode */
  mode = gtk_combo_box_get_active (combo);
  switch (mode)
    {
    case CLOCK_PLUGIN_MODE_ANALOG:
      active = 1 << 1 | 1 << 3;
      break;

    case CLOCK_PLUGIN_MODE_BINARY:
      active = 1 << 1 | 1 << 2 | 1 << 12 | 1 << 13;
      break;

    case CLOCK_PLUGIN_MODE_DIGITAL:
      active = 1 << 6 | 1 << 7 | 1 << 8 | 1 << 9 | 1 << 10;
      break;

    case CLOCK_PLUGIN_MODE_FUZZY:
      active = 1 << 11;
      break;

    case CLOCK_PLUGIN_MODE_LCD:
      active = 1 << 1 | 1 << 3 | 1 << 4 | 1 << 5 | 1 << 12;
      break;

    default:
      panel_assert_not_reached ();
      active = 0;
      break;
    }

  /* show or hide the dialog widgets */
  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      object = gtk_builder_get_object (dialog->builder, names[i].widget);
      panel_return_if_fail (GTK_IS_WIDGET (object));
      if (PANEL_HAS_FLAG (active, 1 << (i + 1)))
        gtk_widget_show (GTK_WIDGET (object));
      else
        gtk_widget_hide (GTK_WIDGET (object));
    }

  /* make sure the new mode is set */
  if (dialog->plugin->mode != mode)
    g_object_set (G_OBJECT (dialog->plugin), "mode", mode, NULL);
  panel_return_if_fail (G_IS_OBJECT (dialog->plugin->clock));

  /* connect the bindings */
  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (PANEL_HAS_FLAG (active, 1 << (i + 1)))
        {
          object = gtk_builder_get_object (dialog->builder, names[i].binding);
          panel_return_if_fail (G_IS_OBJECT (object));
          g_object_bind_property (G_OBJECT (dialog->plugin->clock), names[i].binding,
                                  G_OBJECT (object), names[i].property,
                                  G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
        }
    }
}



static void
clock_plugin_digital_layout_changed (GtkComboBox *combo,
                                     ClockPluginDialog *dialog)
{
  guint mode;
  GObject *date_box, *time_box;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (CLOCK_IS_PLUGIN (dialog->plugin));

  date_box = gtk_builder_get_object (dialog->builder, "digital-date");
  time_box = gtk_builder_get_object (dialog->builder, "digital-time");

  /* the active items for each mode */
  mode = gtk_combo_box_get_active (combo);
  switch (mode)
    {
    case CLOCK_PLUGIN_DIGITAL_FORMAT_DATE:
      gtk_widget_show (GTK_WIDGET (date_box));
      gtk_widget_hide (GTK_WIDGET (time_box));
      break;

    case CLOCK_PLUGIN_DIGITAL_FORMAT_TIME:
      gtk_widget_hide (GTK_WIDGET (date_box));
      gtk_widget_show (GTK_WIDGET (time_box));
      break;

    case CLOCK_PLUGIN_DIGITAL_FORMAT_DATE_TIME:
      gtk_widget_show (GTK_WIDGET (date_box));
      gtk_widget_show (GTK_WIDGET (time_box));
      break;

    case CLOCK_PLUGIN_DIGITAL_FORMAT_TIME_DATE:
      gtk_widget_show (GTK_WIDGET (date_box));
      gtk_widget_show (GTK_WIDGET (time_box));
      break;

    default:
      panel_assert_not_reached ();
      break;
    }
}



static void
clock_plugin_configure_plugin_chooser_changed (GtkComboBox *combo,
                                               GtkEntry *entry)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *format;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_ENTRY (entry));

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      model = gtk_combo_box_get_model (combo);
      gtk_tree_model_get (model, &iter, COLUMN_FORMAT, &format, -1);

      if (format != NULL)
        {
          gtk_entry_set_text (entry, format);
          gtk_widget_hide (GTK_WIDGET (entry));
          g_free (format);
        }
      else
        {
          gtk_widget_show (GTK_WIDGET (entry));
        }
    }
}



static gboolean
clock_plugin_configure_plugin_chooser_separator (GtkTreeModel *model,
                                                 GtkTreeIter *iter,
                                                 gpointer user_data)
{
  gboolean separator;

  gtk_tree_model_get (model, iter, COLUMN_SEPARATOR, &separator, -1);

  return separator;
}



static void
clock_plugin_validate_format_specifier (GtkEntry *entry,
                                        const gchar *format,
                                        ClockPlugin *plugin)
{
  GtkStyleContext *context;
  gchar *clock_time;

  context = gtk_widget_get_style_context (GTK_WIDGET (entry));

  clock_time = clock_time_strdup_strftime (plugin->time, format);
  if (!clock_time)
    gtk_style_context_add_class (context, "error");
  else
    {
      gtk_style_context_remove_class (context, "error");
      g_free (clock_time);
    }
}



static void
clock_plugin_validate_entry_text (GtkEditable *entry,
                                  gpointer user_data)
{
  ClockPlugin *plugin = user_data;

  clock_plugin_validate_format_specifier (GTK_ENTRY (entry),
                                          gtk_entry_get_text (GTK_ENTRY (entry)),
                                          plugin);
}



static void
clock_plugin_validate_timezone (GtkEntry *entry,
                                const gchar *format,
                                ClockPlugin *plugin)
{
  GtkStyleContext *context;
  gchar *filename;

  context = gtk_widget_get_style_context (GTK_WIDGET (entry));

  if (strcmp (format, "") != 0)
    {
      filename = g_build_filename (ZONEINFO_DIR, format, NULL);

      if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        gtk_style_context_add_class (context, "error");
      else
        gtk_style_context_remove_class (context, "error");
    }
  else
    {
      gtk_style_context_remove_class (context, "error");
    }
}



static void
clock_plugin_validate_entry_tz (GtkEditable *entry,
                                gpointer user_data)
{
  ClockPlugin *plugin = user_data;

  clock_plugin_validate_timezone (GTK_ENTRY (entry),
                                  gtk_entry_get_text (GTK_ENTRY (entry)),
                                  plugin);
}



static gboolean
clock_plugin_tz_match_func (GtkEntryCompletion *completion,
                            const gchar *key,
                            GtkTreeIter *iter,
                            gpointer user_data)
{
  gchar *item = NULL;
  gchar *normalized_string;
  gchar *case_normalized_string;
  GtkTreeModel *model;

  gboolean ret = FALSE;

  model = gtk_entry_completion_get_model (completion);

  gtk_tree_model_get (model, iter, 0, &item, -1);

  if (item != NULL)
    {
      normalized_string = g_utf8_normalize (item, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

          if (strstr (case_normalized_string, key))
            ret = TRUE;

          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }
  g_free (item);

  return ret;
}



static void
clock_plugin_configure_plugin_chooser_fill (ClockPlugin *plugin,
                                            GtkComboBox *combo,
                                            GtkEntry *entry,
                                            const gchar *formats[])
{
  guint i;
  GtkListStore *store;
  gchar *preview;
  GtkTreeIter iter;
  const gchar *active_format;
  gboolean has_active = FALSE;

  panel_return_if_fail (CLOCK_IS_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_ENTRY (entry));

  gtk_combo_box_set_row_separator_func (combo, clock_plugin_configure_plugin_chooser_separator, NULL, NULL);

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

  active_format = gtk_entry_get_text (entry);

  for (i = 0; formats[i] != NULL; i++)
    {
      preview = clock_time_strdup_strftime (plugin->time, _(formats[i]));
      if (preview)
        {
          gtk_list_store_insert_with_values (store, &iter, i,
                                             COLUMN_FORMAT, _(formats[i]),
                                             COLUMN_TEXT, preview, -1);

          g_free (preview);

          if (!has_active
              && !xfce_str_is_empty (active_format)
              && strcmp (active_format, formats[i]) == 0)
            {
              gtk_combo_box_set_active_iter (combo, &iter);
              gtk_widget_hide (GTK_WIDGET (entry));
              has_active = TRUE;
            }
        }
      else
        g_warning ("Getting a time preview failed for format specifier %s, so "
                   "omitting it from the list of default formats.",
                   formats[i]);
    }

  gtk_list_store_insert_with_values (store, NULL, i++,
                                     COLUMN_SEPARATOR, TRUE, -1);

  gtk_list_store_insert_with_values (store, &iter, i++,
                                     COLUMN_TEXT, _("Custom Format"), -1);
  if (!has_active)
    {
      gtk_combo_box_set_active_iter (combo, &iter);
      gtk_widget_show (GTK_WIDGET (entry));
      clock_plugin_validate_format_specifier (entry,
                                              gtk_entry_get_text (entry),
                                              plugin);
    }

  g_signal_connect (G_OBJECT (combo), "changed",
                    G_CALLBACK (clock_plugin_configure_plugin_chooser_changed), entry);

  g_object_unref (G_OBJECT (store));
}



static void
clock_plugin_configure_plugin_free (gpointer user_data)
{
  ClockPluginDialog *dialog = user_data;

  if (dialog->zonecompletion_idle != 0)
    g_source_remove (dialog->zonecompletion_idle);

  g_slice_free (ClockPluginDialog, dialog);
}



static void
clock_plugin_configure_config_tool_changed (ClockPluginDialog *dialog)
{
  GObject *object;
  gchar *path;

  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (CLOCK_IS_PLUGIN (dialog->plugin));

  object = gtk_builder_get_object (dialog->builder, "run-time-config-tool");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  path = g_find_program_in_path (dialog->plugin->time_config_tool);
  gtk_widget_set_visible (GTK_WIDGET (object), path != NULL);
  g_free (path);
}



static void
clock_plugin_configure_run_config_tool (GtkWidget *button,
                                        ClockPlugin *plugin)
{
  GError *error = NULL;

  panel_return_if_fail (CLOCK_IS_PLUGIN (plugin));

  if (!xfce_spawn_command_line (gtk_widget_get_screen (button),
                                plugin->time_config_tool,
                                FALSE, FALSE, TRUE, &error))
    {
      xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\"."), plugin->time_config_tool);
      g_error_free (error);
    }
}



static void
clock_plugin_configure_zoneinfo_model_insert (GtkListStore *store,
                                              const gchar *parent)
{
  gchar *filename;
  GtkTreeIter iter;
  GDir *dir;
  const gchar *name;
  gsize dirlen = strlen (ZONEINFO_DIR);

  panel_return_if_fail (GTK_IS_LIST_STORE (store));

  if (g_str_has_suffix (parent, "posix") || g_str_has_suffix (parent, "right"))
    return;

  dir = g_dir_open (parent, 0, NULL);
  if (dir == NULL)
    return;

  for (;;)
    {
      name = g_dir_read_name (dir);
      if (name == NULL)
        break;

      filename = g_build_filename (parent, name, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_DIR))
        {
          if (!g_file_test (filename, G_FILE_TEST_IS_SYMLINK))
            clock_plugin_configure_zoneinfo_model_insert (store, filename);
        }
      else
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter, 0, filename + dirlen, -1);
        }

      g_free (filename);
    }

  g_dir_close (dir);
}



static gboolean
clock_plugin_configure_zoneinfo_model (gpointer data)
{
  ClockPluginDialog *dialog = data;
  GtkEntryCompletion *completion;
  GtkListStore *store;
  GObject *object;

  dialog->zonecompletion_idle = 0;

  object = gtk_builder_get_object (dialog->builder, "timezone-name");
  panel_return_val_if_fail (GTK_IS_ENTRY (object), FALSE);

  /* build timezone model */
  store = gtk_list_store_new (1, G_TYPE_STRING);
  clock_plugin_configure_zoneinfo_model_insert (store, ZONEINFO_DIR);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), 0, GTK_SORT_ASCENDING);

  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (store));
  gtk_entry_completion_set_match_func (completion, clock_plugin_tz_match_func, NULL, NULL);
  g_object_unref (G_OBJECT (store));

  gtk_entry_set_completion (GTK_ENTRY (object), completion);
  gtk_entry_completion_set_popup_single_match (completion, TRUE);
  gtk_entry_completion_set_text_column (completion, 0);

  g_object_unref (G_OBJECT (completion));

  return FALSE;
}



static void
clock_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (panel_plugin);
  ClockPluginDialog *dialog;
  GtkBuilder *builder;
  GObject *object;
  GObject *combo;

  panel_return_if_fail (CLOCK_IS_PLUGIN (plugin));

  /* setup the dialog */
  builder = panel_utils_builder_new (panel_plugin, "/org/xfce/panel/clock-dialog.glade", &plugin->settings_dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  dialog = g_slice_new0 (ClockPluginDialog);
  dialog->plugin = plugin;
  dialog->builder = builder;

  object = gtk_builder_get_object (builder, "run-time-config-tool");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  g_signal_connect_swapped (G_OBJECT (plugin), "notify::time-config-tool",
                            G_CALLBACK (clock_plugin_configure_config_tool_changed),
                            dialog);
  clock_plugin_configure_config_tool_changed (dialog);
  g_signal_connect (G_OBJECT (object), "clicked",
                    G_CALLBACK (clock_plugin_configure_run_config_tool), plugin);

  object = gtk_builder_get_object (builder, "timezone-name");
  panel_return_if_fail (GTK_IS_ENTRY (object));
  g_signal_connect (G_OBJECT (object), "changed",
                    G_CALLBACK (clock_plugin_validate_entry_tz), plugin);
  g_object_bind_property (G_OBJECT (plugin->time), "timezone",
                          G_OBJECT (object), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  /* idle add the zone completion */
  dialog->zonecompletion_idle = gdk_threads_add_idle (clock_plugin_configure_zoneinfo_model, dialog);

  object = gtk_builder_get_object (builder, "mode");
  g_signal_connect_data (G_OBJECT (object), "changed",
                         G_CALLBACK (clock_plugin_configure_plugin_mode_changed), dialog,
                         (GClosureNotify) (void (*) (void)) clock_plugin_configure_plugin_free, 0);
  g_object_bind_property (G_OBJECT (plugin), "mode",
                          G_OBJECT (object), "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  object = gtk_builder_get_object (builder, "tooltip-format");
  g_object_bind_property (G_OBJECT (plugin), "tooltip-format",
                          G_OBJECT (object), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  combo = gtk_builder_get_object (builder, "tooltip-chooser");
  clock_plugin_configure_plugin_chooser_fill (plugin,
                                              GTK_COMBO_BOX (combo),
                                              GTK_ENTRY (object),
                                              tooltip_formats);

  object = gtk_builder_get_object (builder, "command");
  g_object_bind_property (G_OBJECT (plugin), "command",
                          G_OBJECT (object), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  object = gtk_builder_get_object (builder, "show-week-numbers");
  g_object_bind_property (G_OBJECT (plugin), "show-week-numbers",
                          G_OBJECT (object), "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  object = gtk_builder_get_object (builder, "digital-layout");
  g_signal_connect (G_OBJECT (object), "changed",
                    G_CALLBACK (clock_plugin_digital_layout_changed), dialog);
  clock_plugin_digital_layout_changed (GTK_COMBO_BOX (object), dialog);

  object = gtk_builder_get_object (builder, "digital-time-format");
  g_signal_connect (G_OBJECT (object), "changed",
                    G_CALLBACK (clock_plugin_validate_entry_text), plugin);
  combo = gtk_builder_get_object (builder, "digital-time-chooser");
  clock_plugin_configure_plugin_chooser_fill (plugin,
                                              GTK_COMBO_BOX (combo),
                                              GTK_ENTRY (object),
                                              digital_time_formats);

  object = gtk_builder_get_object (builder, "digital-date-format");
  g_signal_connect (G_OBJECT (object), "changed",
                    G_CALLBACK (clock_plugin_validate_entry_text), plugin);
  combo = gtk_builder_get_object (builder, "digital-date-chooser");
  clock_plugin_configure_plugin_chooser_fill (plugin,
                                              GTK_COMBO_BOX (combo),
                                              GTK_ENTRY (object),
                                              digital_date_formats);

  gtk_widget_show (GTK_WIDGET (plugin->settings_dialog));
}



static void
clock_plugin_set_mode (ClockPlugin *plugin)
{
  const PanelProperty properties[][6] = {
    {
      /* analog */
      { "show-seconds", G_TYPE_BOOLEAN },
      { "show-military", G_TYPE_BOOLEAN },
      { NULL },
    },
    {
      /* binary */
      { "show-seconds", G_TYPE_BOOLEAN },
      { "binary-mode", G_TYPE_UINT },
      { "show-inactive", G_TYPE_BOOLEAN },
      { "show-grid", G_TYPE_BOOLEAN },
      { NULL },
    },
    {
      /* digital */
      { "digital-layout", G_TYPE_UINT },
      { "digital-time-format", G_TYPE_STRING },
      { "digital-date-format", G_TYPE_STRING },
      { "digital-time-font", G_TYPE_STRING },
      { "digital-date-font", G_TYPE_STRING },
      { NULL },
    },
    {
      /* fuzzy */
      { "fuzziness", G_TYPE_UINT },
      { NULL },
    },
    {
      /* lcd */
      { "show-seconds", G_TYPE_BOOLEAN },
      { "show-military", G_TYPE_BOOLEAN },
      { "show-meridiem", G_TYPE_BOOLEAN },
      { "flash-separators", G_TYPE_BOOLEAN },
      { "show-inactive", G_TYPE_BOOLEAN },
      { NULL },
    }
  };
  GtkOrientation orientation;
  GtkOrientation container_orientation;

  panel_return_if_fail (CLOCK_IS_PLUGIN (plugin));

  if (plugin->clock != NULL)
    gtk_widget_destroy (plugin->clock);

  /* create a new clock */
  if (plugin->mode == CLOCK_PLUGIN_MODE_ANALOG)
    plugin->clock = xfce_clock_analog_new (plugin->time, plugin->sleep_monitor);
  else if (plugin->mode == CLOCK_PLUGIN_MODE_BINARY)
    plugin->clock = xfce_clock_binary_new (plugin->time, plugin->sleep_monitor);
  else if (plugin->mode == CLOCK_PLUGIN_MODE_DIGITAL)
    plugin->clock = xfce_clock_digital_new (plugin->time, plugin->sleep_monitor);
  else if (plugin->mode == CLOCK_PLUGIN_MODE_FUZZY)
    plugin->clock = xfce_clock_fuzzy_new (plugin->time, plugin->sleep_monitor);
  else
    plugin->clock = xfce_clock_lcd_new (plugin->time, plugin->sleep_monitor);

  container_orientation = (xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (plugin))
                           == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
                            ? GTK_ORIENTATION_HORIZONTAL
                            : GTK_ORIENTATION_VERTICAL;
  g_object_set (G_OBJECT (plugin->clock), "container-orientation", container_orientation, NULL);

  if (plugin->rotate_vertically)
    {
      orientation = (xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (plugin))
                     == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
                      ? GTK_ORIENTATION_VERTICAL
                      : GTK_ORIENTATION_HORIZONTAL;
      g_object_set (G_OBJECT (plugin->clock), "orientation", orientation, NULL);
    }

  panel_properties_bind (NULL, G_OBJECT (plugin->clock),
                         xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (plugin)),
                         properties[plugin->mode], FALSE);

  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->clock);

  gtk_widget_show (plugin->clock);
}



static void
clock_plugin_calendar_hide (GtkWidget *calendar_window,
                            ClockPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), FALSE);
}



static void
clock_plugin_calendar_show (GtkWidget *calendar_window,
                            ClockPlugin *plugin)
{
  GDateTime *time;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  time = clock_time_get_time (plugin->time);
  gtk_calendar_select_month (GTK_CALENDAR (plugin->calendar), g_date_time_get_month (time) - 1,
                             g_date_time_get_year (time));
  gtk_calendar_select_day (GTK_CALENDAR (plugin->calendar), g_date_time_get_day_of_month (time));
  g_date_time_unref (time);
}



static void
clock_plugin_popup_calendar (ClockPlugin *plugin)
{
  if (plugin->calendar_window == NULL)
    {
      plugin->calendar_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      g_signal_connect (G_OBJECT (plugin->calendar_window), "show",
                        G_CALLBACK (clock_plugin_calendar_show), plugin);
      g_signal_connect (G_OBJECT (plugin->calendar_window), "hide",
                        G_CALLBACK (clock_plugin_calendar_hide), plugin);

      plugin->calendar = gtk_calendar_new ();
      clock_plugin_set_calendar_options (plugin);
      gtk_container_add (GTK_CONTAINER (plugin->calendar_window), plugin->calendar);
      gtk_widget_show (plugin->calendar);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
  xfce_panel_plugin_popup_window (XFCE_PANEL_PLUGIN (plugin),
                                  GTK_WINDOW (plugin->calendar_window), NULL);
}



static gboolean
clock_plugin_tooltip (gpointer user_data)
{
  ClockPlugin *plugin = CLOCK_PLUGIN (user_data);
  gchar *string;

  /* set the tooltip */
  string = clock_time_strdup_strftime (plugin->time, plugin->tooltip_format);
  gtk_widget_set_tooltip_markup (GTK_WIDGET (plugin), string);
  g_free (string);

  /* make sure the tooltip is up2date */
  gtk_widget_trigger_tooltip_query (GTK_WIDGET (plugin));

  /* keep the timeout running */
  return TRUE;
}



static void
clock_plugin_set_calendar_options (ClockPlugin *plugin)
{
  GtkCalendarDisplayOptions options = GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES;
  if (plugin->show_week_numbers)
    options |= GTK_CALENDAR_SHOW_WEEK_NUMBERS;

  gtk_calendar_set_display_options (GTK_CALENDAR (plugin->calendar), options);
}
