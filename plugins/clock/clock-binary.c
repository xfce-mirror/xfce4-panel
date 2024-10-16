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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clock-binary.h"
#include "clock.h"

#include "common/panel-private.h"

#include <cairo/cairo.h>

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#define MAX_HEIGHT (16)

static void
xfce_clock_binary_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec);
static void
xfce_clock_binary_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec);
static void
xfce_clock_binary_finalize (GObject *object);
static gboolean
xfce_clock_binary_draw (GtkWidget *widget,
                        cairo_t *cr);
static gboolean
xfce_clock_binary_update (XfceClockBinary *binary,
                          ClockTime *time);
static gint
xfce_clock_binary_get_cols (XfceClockBinary *binary);
static gint
xfce_clock_binary_get_rows (XfceClockBinary *binary);
static void
xfce_clock_binary_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint height,
                                                  gint *minimum_width,
                                                  gint *natural_width);
static void
xfce_clock_binary_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint width,
                                                  gint *minimum_height,
                                                  gint *natural_height);
static GtkSizeRequestMode
xfce_clock_binary_get_request_mode (GtkWidget *widget);



enum
{
  PROP_0,
  PROP_SHOW_SECONDS,
  PROP_MODE,
  PROP_SHOW_INACTIVE,
  PROP_SHOW_GRID,
  PROP_ORIENTATION,
  PROP_CONTAINER_ORIENTATION,
};

enum
{
  MODE_DECIMAL,
  MODE_SEXAGESIMAL,
  MODE_BINARY_TIME
};

struct _XfceClockBinary
{
  GtkImage __parent__;

  ClockTimeTimeout *timeout;

  GtkOrientation container_orientation;
  guint show_seconds : 1;
  guint mode;
  guint show_inactive : 1;
  guint show_grid : 1;

  ClockTime *time;
};



G_DEFINE_FINAL_TYPE (XfceClockBinary, xfce_clock_binary, GTK_TYPE_IMAGE)



static void
xfce_clock_binary_class_init (XfceClockBinaryClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = xfce_clock_binary_set_property;
  gobject_class->get_property = xfce_clock_binary_get_property;
  gobject_class->finalize = xfce_clock_binary_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->draw = xfce_clock_binary_draw;
  gtkwidget_class->get_preferred_width_for_height = xfce_clock_binary_get_preferred_width_for_height;
  gtkwidget_class->get_preferred_height_for_width = xfce_clock_binary_get_preferred_height_for_width;
  gtkwidget_class->get_request_mode = xfce_clock_binary_get_request_mode;

  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      G_PARAM_WRITABLE
                                                        | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_CONTAINER_ORIENTATION,
                                   g_param_spec_enum ("container-orientation", NULL, NULL,
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      G_PARAM_WRITABLE
                                                        | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_SECONDS,
                                   g_param_spec_boolean ("show-seconds", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE
                                                           | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_uint ("binary-mode", NULL, NULL,
                                                      MODE_DECIMAL, MODE_BINARY_TIME, MODE_DECIMAL,
                                                      G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_INACTIVE,
                                   g_param_spec_boolean ("show-inactive", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE
                                                           | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_GRID,
                                   g_param_spec_boolean ("show-grid", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE
                                                           | G_PARAM_STATIC_STRINGS));
}



static void
xfce_clock_binary_init (XfceClockBinary *binary)
{
  binary->container_orientation = GTK_ORIENTATION_HORIZONTAL;
  binary->show_seconds = FALSE;
  binary->mode = MODE_DECIMAL;
  binary->show_inactive = TRUE;
  binary->show_grid = FALSE;
}



static void
xfce_clock_binary_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      break;

    case PROP_CONTAINER_ORIENTATION:
      binary->container_orientation = g_value_get_enum (value);
      break;

    case PROP_SHOW_SECONDS:
      binary->show_seconds = g_value_get_boolean (value);
      gtk_widget_queue_resize (GTK_WIDGET (binary));
      break;

    case PROP_MODE:
      binary->mode = g_value_get_uint (value);
      gtk_widget_queue_resize (GTK_WIDGET (binary));
      break;

    case PROP_SHOW_INACTIVE:
      binary->show_inactive = g_value_get_boolean (value);
      break;

    case PROP_SHOW_GRID:
      binary->show_grid = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  /* reschedule the timeout and resize */
  clock_time_timeout_set_interval (binary->timeout,
                                   binary->show_seconds ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE);
  gtk_widget_queue_resize (GTK_WIDGET (binary));
}



static void
xfce_clock_binary_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (object);

  switch (prop_id)
    {
    case PROP_SHOW_SECONDS:
      g_value_set_boolean (value, binary->show_seconds);
      break;

    case PROP_MODE:
      g_value_set_uint (value, binary->mode);
      break;

    case PROP_SHOW_INACTIVE:
      g_value_set_boolean (value, binary->show_inactive);
      break;

    case PROP_SHOW_GRID:
      g_value_set_boolean (value, binary->show_grid);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_clock_binary_finalize (GObject *object)
{
  /* stop the timeout */
  clock_time_timeout_free (XFCE_CLOCK_BINARY (object)->timeout);

  (*G_OBJECT_CLASS (xfce_clock_binary_parent_class)->finalize) (object);
}


static guint
xfce_clock_binary_algo_value (GDateTime *time,
                              gboolean seconds)
{
  guint n;

  n = g_date_time_get_hour (time) * 100 + g_date_time_get_minute (time);

  if (seconds)
    n = n * 100 + g_date_time_get_second (time);

  return n;
}

static void
xfce_clock_binary_draw_true_binary (gulong *table,
                                    GDateTime *time,
                                    gboolean seconds,
                                    gint rows,
                                    gint cols)
{
  gint row, ticks;
  guint n, p;

  n = xfce_clock_binary_algo_value (time, seconds);

  for (row = 0, p = 1; row < rows; row++, p *= 100)
    {
      ticks = n / p % 100;
      *table |= ticks << (row * cols);
    }
}



static void
xfce_clock_binary_draw_binary (gulong *table,
                               GDateTime *time,
                               gboolean seconds,
                               gint rows,
                               gint cols)
{
  gint row, col, ticks;
  guint n, p;

  n = xfce_clock_binary_algo_value (time, seconds);

  for (col = 0, p = 1; col < cols; col++, p *= 10)
    {
      ticks = n / p % 10;
      for (row = 0; row < rows; row++)
        {
          if (ticks & (1 << row))
            *table |= 1 << (row * cols + col);
        }
    }
}



static void
xfce_clock_binary_draw_binary_time (gulong *table,
                                    GDateTime *time,
                                    gboolean seconds)
{
  guint n;

  n = g_date_time_get_hour (time) * 60 * 60
      + g_date_time_get_minute (time) * 60
      + g_date_time_get_second (time);

  *table = (n * 512) / 675; // 2 ** 16 / (24 * 60 * 60)

  if (!seconds)
    *table >>= 8;
}



static gboolean
xfce_clock_binary_draw (GtkWidget *widget,
                        cairo_t *cr)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);
  gint col, cols;
  gint row, rows;
  GtkAllocation alloc;
  gdouble x;
  gdouble y;
  gint w, h;
  gint width, height;
  gint pad_x, pad_y;
  gint diff;
  GtkStyleContext *ctx;
  GtkStateFlags state_flags;
  GdkRGBA active_rgba, inactive_rgba, grid_rgba;
  GtkBorder padding;
  gulong table = 0;
  GDateTime *time;

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);
  panel_return_val_if_fail (cr != NULL, FALSE);

  ctx = gtk_widget_get_style_context (widget);
  state_flags = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_padding (ctx, state_flags, &padding);
  pad_x = MAX (padding.left, padding.right);
  pad_y = MAX (padding.top, padding.bottom);

  gtk_widget_get_allocation (widget, &alloc);
  alloc.width -= 1 + 2 * pad_x;
  alloc.height -= 1 + 2 * pad_y;

  cols = xfce_clock_binary_get_cols (binary);
  rows = xfce_clock_binary_get_rows (binary);

  height = MIN (alloc.height, MAX_HEIGHT * rows);
  width = MIN (alloc.width, MAX_HEIGHT * cols);
  alloc.x = pad_x + 1 + (alloc.width - width) / 2;
  alloc.y = pad_y + 1 + (alloc.height - height) / 2;
  alloc.width = width;
  alloc.height = height;

  /* align columns and fix rounding */
  diff = alloc.width - (floor ((gdouble) alloc.width / cols) * cols);
  alloc.width -= diff;
  alloc.x += diff / 2;

  /* align rows and fix rounding */
  diff = alloc.height - (floor ((gdouble) alloc.height / rows) * rows);
  alloc.height -= diff;
  alloc.y += diff / 2;

  w = alloc.width / cols;
  h = alloc.height / rows;

  gtk_style_context_get_color (ctx, state_flags, &active_rgba);
  grid_rgba = inactive_rgba = active_rgba;

  if (binary->show_grid)
    {
      grid_rgba.alpha = 0.4;
      gdk_cairo_set_source_rgba (cr, &grid_rgba);
      cairo_set_line_width (cr, 1);

      x = alloc.x - 0.5;
      y = alloc.y - 0.5;

      for (col = 0; col <= cols; col++)
        {
          cairo_move_to (cr, x + col * w, alloc.y - 1);
          cairo_rel_line_to (cr, 0, alloc.height + 1);
          cairo_stroke (cr);
        }

      for (row = 0; row <= rows; row++)
        {
          cairo_move_to (cr, alloc.x - 1, y + row * h);
          cairo_rel_line_to (cr, alloc.width + 1, 0);
          cairo_stroke (cr);
        }
    }

  time = clock_time_get_time (binary->time);

  switch (binary->mode)
    {
    case MODE_DECIMAL:
      xfce_clock_binary_draw_binary (&table, time, binary->show_seconds, rows, cols);
      break;
    case MODE_SEXAGESIMAL:
      xfce_clock_binary_draw_true_binary (&table, time, binary->show_seconds, rows, cols);
      break;
    case MODE_BINARY_TIME:
      xfce_clock_binary_draw_binary_time (&table, time, binary->show_seconds);
      break;
    }

  g_date_time_unref (time);

  inactive_rgba.alpha = 0.2;
  active_rgba.alpha = 1.0;

  for (col = 0; col < cols; col++)
    {
      for (row = 0; row < rows; row++)
        {
          if (table & (1 << (row * cols + col)))
            {
              gdk_cairo_set_source_rgba (cr, &active_rgba);
            }
          else if (binary->show_inactive)
            {
              gdk_cairo_set_source_rgba (cr, &inactive_rgba);
            }
          else
            {
              continue;
            }

          /* draw the dot */
          cairo_rectangle (cr,
                           alloc.x + (cols - 1 - col) * w,
                           alloc.y + (rows - 1 - row) * h,
                           w - 1, h - 1);

          cairo_fill (cr);
        }
    }

  return FALSE;
}



static gint
xfce_clock_binary_get_cols (XfceClockBinary *binary)
{
  gint cols = 1;
  switch (binary->mode)
    {
    case MODE_DECIMAL:
      cols = binary->show_seconds ? 6 : 4;
      break;
    case MODE_SEXAGESIMAL:
      cols = 6;
      break;
    case MODE_BINARY_TIME:
      cols = 4;
      break;
    }
  return cols;
}



static gint
xfce_clock_binary_get_rows (XfceClockBinary *binary)
{
  gint rows = 1;
  switch (binary->mode)
    {
    case MODE_DECIMAL:
      rows = 4;
      break;
    case MODE_SEXAGESIMAL:
      rows = binary->show_seconds ? 3 : 2;
      break;
    case MODE_BINARY_TIME:
      rows = binary->show_seconds ? 4 : 2;
      break;
    }
  return rows;
}



static void
xfce_clock_binary_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint height,
                                                  gint *minimum_width,
                                                  gint *natural_width)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);

  gint cols = xfce_clock_binary_get_cols (binary);
  gint rows = xfce_clock_binary_get_rows (binary);
  gint cell = MIN ((gdouble) height / rows, MAX_HEIGHT);
  gint width = ceil ((gdouble) cell * cols);

  if (minimum_width != NULL)
    *minimum_width = width;

  if (natural_width != NULL)
    *natural_width = width;
}



static void
xfce_clock_binary_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint width,
                                                  gint *minimum_height,
                                                  gint *natural_height)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);

  gint cols = xfce_clock_binary_get_cols (binary);
  gint rows = xfce_clock_binary_get_rows (binary);
  gint cell = MIN ((gdouble) width / cols, MAX_HEIGHT);
  gint height = ceil ((gdouble) cell * rows);

  if (minimum_height != NULL)
    *minimum_height = height;

  if (natural_height != NULL)
    *natural_height = height;
}



static GtkSizeRequestMode
xfce_clock_binary_get_request_mode (GtkWidget *widget)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);

  if (binary->container_orientation == GTK_ORIENTATION_HORIZONTAL)
    return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
  else
    return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}



static gboolean
xfce_clock_binary_update (XfceClockBinary *binary,
                          ClockTime *time)
{
  GtkWidget *widget = GTK_WIDGET (binary);

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);

  /* update if the widget if visible */
  if (G_LIKELY (gtk_widget_get_visible (widget)))
    gtk_widget_queue_draw (widget);

  return TRUE;
}



GtkWidget *
xfce_clock_binary_new (ClockTime *time,
                       ClockSleepMonitor *sleep_monitor)
{
  XfceClockBinary *binary = g_object_new (XFCE_CLOCK_TYPE_BINARY, NULL);

  binary->time = time;
  binary->timeout = clock_time_timeout_new (CLOCK_INTERVAL_MINUTE,
                                            binary->time,
                                            sleep_monitor,
                                            G_CALLBACK (xfce_clock_binary_update), binary);

  return GTK_WIDGET (binary);
}
