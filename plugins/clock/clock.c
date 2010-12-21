/*
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>
#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>

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
static gboolean clock_plugin_button_press_event        (GtkWidget             *widget,
                                                        GdkEventButton        *event);
static void     clock_plugin_construct                 (XfcePanelPlugin       *panel_plugin);
static void     clock_plugin_free_data                 (XfcePanelPlugin       *panel_plugin);
static gboolean clock_plugin_size_changed              (XfcePanelPlugin       *panel_plugin,
                                                        gint                   size);
static void     clock_plugin_size_ratio_changed        (XfcePanelPlugin       *panel_plugin);
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
  PROP_TOOLTIP_FORMAT,
  PROP_COMMAND,
  PROP_ROTATE_VERTICALLY
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

  guint               show_frame : 1;
  gchar              *command;
  ClockPluginMode     mode;
  guint               rotate_vertically : 1;

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

static const gchar *tooltip_formats[] =
{
  DEFAULT_TOOLTIP_FORMAT,
  "%x",
  N_("Week %V"),
  NULL
};

static const gchar *digital_formats[] =
{
  DEFAULT_DIGITAL_FORMAT,
  "%T",
  "%r",
  "%I:%M %p",
  NULL
};

enum
{
  COLUMN_FORMAT,
  COLUMN_SEPARATOR,
  COLUMN_TEXT,
  N_COLUMNS
};



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
  widget_class->button_press_event = clock_plugin_button_press_event;

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
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_TOOLTIP_FORMAT,
                                   g_param_spec_string ("tooltip-format",
                                                        NULL, NULL,
                                                        DEFAULT_TOOLTIP_FORMAT,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ROTATE_VERTICALLY,
                                   g_param_spec_boolean ("rotate-vertically",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND,
                                   g_param_spec_string ("command",
                                                        NULL, NULL, NULL,
                                                        EXO_PARAM_READWRITE));
}



static void
clock_plugin_init (ClockPlugin *plugin)
{
  plugin->mode = CLOCK_PLUGIN_MODE_DEFAULT;
  plugin->clock = NULL;
  plugin->show_frame = TRUE;
  plugin->tooltip_format = g_strdup (DEFAULT_TOOLTIP_FORMAT);
  plugin->tooltip_timeout = NULL;
  plugin->command = NULL;
  plugin->rotate_vertically = FALSE;

  plugin->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->frame);
  gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show (plugin->frame);
}



static void
clock_plugin_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_MODE:
      g_value_set_uint (value, plugin->mode);
      break;

    case PROP_SHOW_FRAME:
      g_value_set_boolean (value, plugin->show_frame);
      break;

    case PROP_TOOLTIP_FORMAT:
      g_value_set_string (value, plugin->tooltip_format);
      break;

    case PROP_COMMAND:
      g_value_set_string (value, plugin->command);
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
clock_plugin_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (object);
  gboolean     show_frame;
  gboolean     rotate_vertically;

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
      show_frame = g_value_get_boolean (value);
      if (plugin->show_frame != show_frame)
        {
          plugin->show_frame = show_frame;
          gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame),
              show_frame ? GTK_SHADOW_ETCHED_IN : GTK_SHADOW_NONE);
        }
      break;

    case PROP_TOOLTIP_FORMAT:
      g_free (plugin->tooltip_format);
      plugin->tooltip_format = g_value_dup_string (value);
      break;

    case PROP_COMMAND:
      g_free (plugin->command);
      plugin->command = g_value_dup_string (value);
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



static gboolean
clock_plugin_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (widget);
  GError      *error = NULL;

  if (event->button == 1
      && event->type == GDK_2BUTTON_PRESS
      && !exo_str_is_empty (plugin->command))
    {
      /* launch command */
      if (!gdk_spawn_command_line_on_screen (gtk_widget_get_screen (widget),
                                             plugin->command, &error))
        {
          xfce_dialog_show_error (NULL, error, _("Failed to execute clock command"));
          g_error_free (error);
        }

      return TRUE;
    }

  return (*GTK_WIDGET_CLASS (clock_plugin_parent_class)->button_press_event) (widget, event);
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
    { "command", G_TYPE_STRING },
    { "rotate-vertically", G_TYPE_BOOLEAN },
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
  g_free (plugin->command);
}



static gboolean
clock_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint             size)
{
  ClockPlugin *plugin = XFCE_CLOCK_PLUGIN (panel_plugin);
  gdouble      ratio;
  gint         ratio_size;
  gint         border = 0;
  gint         offset;

  if (plugin->clock == NULL)
    return TRUE;

  /* set the frame border */
  if (plugin->show_frame && size > 26)
    border = 1;
  gtk_container_set_border_width (GTK_CONTAINER (plugin->frame), border);

  /* get the width:height ratio */
  g_object_get (G_OBJECT (plugin->clock), "size-ratio", &ratio, NULL);
  if (ratio > 0)
    {
      offset = MAX (plugin->frame->style->xthickness, plugin->frame->style->ythickness) + border;
      offset *= 2;
      ratio_size = size - offset;
    }
  else
    {
      ratio_size = -1;
    }

  /* set the clock size */
  if (xfce_panel_plugin_get_orientation (panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
    {
      if (ratio > 0)
        {
          ratio_size = ceil (ratio_size * ratio);
          ratio_size += offset;
        }

      gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), ratio_size, size);
    }
  else
    {
      if (ratio > 0)
        {
          ratio_size = ceil (ratio_size / ratio);
          ratio_size += offset;
        }

      gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, ratio_size);
    }

  return TRUE;
}



static void
clock_plugin_size_ratio_changed (XfcePanelPlugin *panel_plugin)
{
  clock_plugin_size_changed (panel_plugin, xfce_panel_plugin_get_size (panel_plugin));
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
  guint    i, active, mode;
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
    { "digital-box", "digital-format", "text" },
    { "fuzziness-box", "fuzziness", "value" },
    { "show-inactive", "show-inactive", "active" },
    { "show-grid", "show-grid", "active" },
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
      active = 1 << 1 | 1 << 2 | 1 << 8 | 1 << 9;
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
clock_plugin_configure_plugin_chooser_changed (GtkComboBox *combo,
                                               GtkEntry    *entry)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *format;

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
                                                 GtkTreeIter  *iter,
                                                 gpointer      user_data)
{
  gboolean separator;

  gtk_tree_model_get (model, iter, COLUMN_SEPARATOR, &separator, -1);

  return separator;
}



static void
clock_plugin_configure_plugin_chooser_fill (GtkComboBox *combo,
                                            GtkEntry    *entry,
                                            const gchar *formats[])
{
  guint         i;
  GtkListStore *store;
  gchar        *preview;
  struct tm     now;
  GtkTreeIter   iter;
  const gchar  *active_format;
  gboolean      has_active = FALSE;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));
  panel_return_if_fail (GTK_IS_ENTRY (entry));

  gtk_combo_box_set_row_separator_func (combo,
      clock_plugin_configure_plugin_chooser_separator, NULL, NULL);

  store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  gtk_combo_box_set_model (combo, GTK_TREE_MODEL (store));

  clock_plugin_get_localtime (&now);

  active_format = gtk_entry_get_text (entry);

  for (i = 0; formats[i] != NULL; i++)
    {
      preview = clock_plugin_strdup_strftime (_(formats[i]), &now);
      gtk_list_store_insert_with_values (store, &iter, i,
                                         COLUMN_FORMAT, _(formats[i]),
                                         COLUMN_TEXT, preview, -1);
      g_free (preview);

      if (has_active == FALSE
          && !exo_str_is_empty (active_format)
          && strcmp (active_format, formats[i]) == 0)
        {
          gtk_combo_box_set_active_iter (combo, &iter);
          gtk_widget_hide (GTK_WIDGET (entry));
          has_active = TRUE;
        }
    }

  gtk_list_store_insert_with_values (store, NULL, i++,
                                     COLUMN_SEPARATOR, TRUE, -1);

  gtk_list_store_insert_with_values (store, &iter, i++,
                                     COLUMN_TEXT, _("Custom Format"), -1);
  if (!has_active)
    {
      gtk_combo_box_set_active_iter (combo, &iter);
      gtk_widget_show (GTK_WIDGET (entry));
    }

  g_signal_connect (G_OBJECT (combo), "changed",
      G_CALLBACK (clock_plugin_configure_plugin_chooser_changed), entry);

  g_object_unref (G_OBJECT (store));
}



static void
clock_plugin_configure_plugin_free (gpointer user_data)
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
  GObject           *combo;

  panel_return_if_fail (XFCE_IS_CLOCK_PLUGIN (plugin));

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, clock_dialog_ui,
                                     clock_dialog_ui_length, &window);
  if (G_UNLIKELY (builder == NULL))
    return;

  dialog = g_slice_new0 (ClockPluginDialog);
  dialog->plugin = plugin;
  dialog->builder = builder;

  object = gtk_builder_get_object (builder, "mode");
  g_signal_connect_data (G_OBJECT (object), "changed",
      G_CALLBACK (clock_plugin_configure_plugin_mode_changed), dialog,
      (GClosureNotify) clock_plugin_configure_plugin_free, 0);
  exo_mutual_binding_new (G_OBJECT (plugin), "mode",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "show-frame");
  exo_mutual_binding_new (G_OBJECT (plugin), "show-frame",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "tooltip-format");
  exo_mutual_binding_new (G_OBJECT (plugin), "tooltip-format",
                          G_OBJECT (object), "text");
  combo = gtk_builder_get_object (builder, "tooltip-chooser");
  clock_plugin_configure_plugin_chooser_fill (GTK_COMBO_BOX (combo),
                                              GTK_ENTRY (object),
                                              tooltip_formats);

  object = gtk_builder_get_object (builder, "digital-format");
  combo = gtk_builder_get_object (builder, "digital-chooser");
  clock_plugin_configure_plugin_chooser_fill (GTK_COMBO_BOX (combo),
                                              GTK_ENTRY (object),
                                              digital_formats);

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
      { "show-inactive", G_TYPE_BOOLEAN },
      { "show-grid", G_TYPE_BOOLEAN },
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

  if (plugin->rotate_vertically)
    exo_binding_new (G_OBJECT (plugin), "orientation", G_OBJECT (plugin->clock), "orientation");

  /* watch width/height changes */
  g_signal_connect_swapped (G_OBJECT (plugin->clock), "notify::size-ratio",
      G_CALLBACK (clock_plugin_size_ratio_changed), plugin);

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
  gtk_widget_set_tooltip_markup (GTK_WIDGET (plugin), string);
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
  gchar  buffer[1024];

  /* leave when format is null */
  if (G_UNLIKELY (exo_str_is_empty (format)))
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

  if (G_UNLIKELY (exo_str_is_empty (format)))
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
