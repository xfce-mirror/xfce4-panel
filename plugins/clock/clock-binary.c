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
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "clock.h"
#include "clock-time.h"
#include "clock-binary.h"



static void      xfce_clock_binary_set_property  (GObject              *object,
                                                  guint                 prop_id,
                                                  const GValue         *value,
                                                  GParamSpec           *pspec);
static void      xfce_clock_binary_get_property  (GObject              *object,
                                                  guint                 prop_id,
                                                  GValue               *value,
                                                  GParamSpec           *pspec);
static void      xfce_clock_binary_finalize      (GObject              *object);
static gboolean  xfce_clock_binary_draw          (GtkWidget            *widget,
                                                  cairo_t              *cr);
static gboolean  xfce_clock_binary_update        (XfceClockBinary      *binary,
                                                  ClockTime            *time);




enum
{
  PROP_0,
  PROP_SHOW_SECONDS,
  PROP_TRUE_BINARY,
  PROP_SHOW_INACTIVE,
  PROP_SHOW_GRID,
  PROP_SIZE_RATIO,
  PROP_ORIENTATION
};

struct _XfceClockBinaryClass
{
  GtkImageClass __parent__;
};

struct _XfceClockBinary
{
  GtkImage  __parent__;

  ClockTimeTimeout *timeout;

  guint     show_seconds : 1;
  guint     true_binary : 1;
  guint     show_inactive : 1;
  guint     show_grid : 1;

  ClockTime *time;
};



XFCE_PANEL_DEFINE_TYPE (XfceClockBinary, xfce_clock_binary, GTK_TYPE_IMAGE)



static void
xfce_clock_binary_class_init (XfceClockBinaryClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = xfce_clock_binary_set_property;
  gobject_class->get_property = xfce_clock_binary_get_property;
  gobject_class->finalize = xfce_clock_binary_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->draw = xfce_clock_binary_draw;

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE_RATIO,
                                   g_param_spec_double ("size-ratio", NULL, NULL,
                                                        -1, G_MAXDOUBLE, 1.0,
                                                        G_PARAM_READABLE
                                                        | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
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
                                   PROP_TRUE_BINARY,
                                   g_param_spec_boolean ("true-binary", NULL, NULL,
                                                         FALSE,
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
  binary->show_seconds = FALSE;
  binary->true_binary = FALSE;
  binary->show_inactive = TRUE;
  binary->show_grid = FALSE;

}



static void
xfce_clock_binary_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      break;

    case PROP_SHOW_SECONDS:
      binary->show_seconds = g_value_get_boolean (value);
      g_object_notify (object, "size-ratio");
      break;

    case PROP_TRUE_BINARY:
      binary->true_binary = g_value_get_boolean (value);
      g_object_notify (object, "size-ratio");
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
xfce_clock_binary_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (object);
  gdouble          ratio;

  switch (prop_id)
    {
    case PROP_SHOW_SECONDS:
      g_value_set_boolean (value, binary->show_seconds);
      break;

    case PROP_TRUE_BINARY:
      g_value_set_boolean (value, binary->true_binary);
      break;

    case PROP_SHOW_INACTIVE:
      g_value_set_boolean (value, binary->show_inactive);
      break;

    case PROP_SHOW_GRID:
      g_value_set_boolean (value, binary->show_grid);
      break;

    case PROP_SIZE_RATIO:
      if (binary->true_binary)
        ratio = binary->show_seconds ? 2.0 : 3.0;
      else
        ratio = binary->show_seconds ? 1.5 : 1.0;
      g_value_set_double (value, ratio);
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



static void
xfce_clock_binary_draw_true_binary (XfceClockBinary *binary,
                                    cairo_t         *cr,
                                    GtkAllocation   *alloc)
{
  GDateTime        *time;
  gint              row, rows;
  static gint       binary_table[] = { 32, 16, 8, 4, 2, 1 };
  gint              col, cols = G_N_ELEMENTS (binary_table);
  gint              remain_h, remain_w;
  gint              offset_x, offset_y;
  gint              w, h, x;
  gint              ticks;
  GtkStyleContext  *ctx;
  GdkRGBA           active_rgba, inactive_rgba;

  ctx = gtk_widget_get_style_context (GTK_WIDGET (gtk_widget_get_parent (GTK_WIDGET (binary))));

  if (G_UNLIKELY (gtk_widget_get_state_flags (GTK_WIDGET (binary)) & GTK_STATE_FLAG_INSENSITIVE))
    {
      gtk_style_context_get_color (ctx, GTK_STATE_FLAG_INSENSITIVE,
                                   &inactive_rgba);
      inactive_rgba.alpha = 0.2;
      gtk_style_context_get_color (ctx, GTK_STATE_FLAG_INSENSITIVE,
                                   &active_rgba);
      active_rgba.alpha = 1.0;
    }
  else
    {
      gtk_style_context_get_color (ctx, GTK_STATE_FLAG_NORMAL,
                                   &inactive_rgba);
      inactive_rgba.alpha = 0.2;
      gtk_style_context_get_color (ctx, GTK_STATE_FLAG_ACTIVE,
                                   &active_rgba);
      active_rgba.alpha = 1.0;
    }

  time = clock_time_get_time (binary->time);

  /* init sizes */
  remain_h = alloc->height;
  offset_y = alloc->y;

  rows = binary->show_seconds ? 3 : 2;
  for (row = 0; row < rows; row++)
    {
      /* get the time this row represents */
      if (row == 0)
        ticks = g_date_time_get_hour (time);
      else if (row == 1)
        ticks = g_date_time_get_minute (time);
      else
        ticks = g_date_time_get_second (time);

      /* reset sizes */
      remain_w = alloc->width;
      offset_x = alloc->x;
      h = remain_h / (rows - row);
      remain_h -= h;

      for (col = 0; col < cols; col++)
        {
          /* update sizes */
          w = remain_w / (cols - col);
          x = offset_x;
          remain_w -= w;
          offset_x += w;

          if (ticks >= binary_table[col])
            {
              gdk_cairo_set_source_rgba (cr, &active_rgba);
              ticks -= binary_table[col];
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
          cairo_rectangle (cr, x, offset_y, w - 1, h - 1);
          cairo_fill (cr);
        }

      /* advance offset */
      offset_y += h;
    }

  g_date_time_unref (time);
}



static void
xfce_clock_binary_draw_binary (XfceClockBinary *binary,
                                     cairo_t         *cr,
                                     GtkAllocation   *alloc)
{
  static gint       binary_table[] = { 80, 40, 20, 10, 8, 4, 2, 1 };
  GDateTime        *time;
  gint              row, rows = G_N_ELEMENTS (binary_table) / 2;
  gint              col, cols;
  gint              digit;
  gint              remain_h, remain_w;
  gint              offset_x, offset_y;
  gint              w, h, y;
  gint              ticks = 0;
  GtkStyleContext  *ctx;
  GdkRGBA           active_rgba, inactive_rgba;

  ctx = gtk_widget_get_style_context (GTK_WIDGET (binary));

  if (G_UNLIKELY (gtk_widget_get_state_flags (GTK_WIDGET (binary)) & GTK_STATE_INSENSITIVE))
    {
      gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (GTK_WIDGET (binary)),
                                   &inactive_rgba);
      inactive_rgba.alpha = 0.2;
      gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (GTK_WIDGET (binary)),
                                   &active_rgba);
      active_rgba.alpha = 1.0;
    }
  else
    {
      gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (GTK_WIDGET (binary)),
                                   &inactive_rgba);
      inactive_rgba.alpha = 0.2;
      gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (GTK_WIDGET (binary)),
                                   &active_rgba);
      active_rgba.alpha = 1.0;
    }

  time = clock_time_get_time (binary->time);

  remain_w = alloc->width;
  offset_x = alloc->x;

  /* make sure the cols are all equal */
  cols = binary->show_seconds ? 6 : 4;
  for (col = 0; col < cols; col++)
    {
      /* get the time this row represents */
      if (col == 0)
        ticks = g_date_time_get_hour (time);
      else if (col == 2)
        ticks = g_date_time_get_minute (time);
      else if (col == 4)
        ticks = g_date_time_get_second (time);

      /* reset sizes */
      remain_h = alloc->height;
      offset_y = alloc->y;
      w = remain_w / (cols - col);
      remain_w -= w;

      for (row = 0; row < rows; row++)
        {
          /* update sizes */
          h = remain_h / (rows - row);
          remain_h -= h;
          y = offset_y;
          offset_y += h;

          digit = row + (4 * (col % 2));
          if (ticks >= binary_table[digit])
            {
              gdk_cairo_set_source_rgba (cr, &active_rgba);
              ticks -= binary_table[digit];
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
          cairo_rectangle (cr, offset_x, y, w - 1, h - 1);
          cairo_fill (cr);
        }

      /* advance offset */
      offset_x += w;
    }

  g_date_time_unref (time);
}



static gboolean
xfce_clock_binary_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  XfceClockBinary  *binary = XFCE_CLOCK_BINARY (widget);
  gint              col, cols;
  gint              row, rows;
  GtkAllocation     alloc;
  gdouble           remain_w, x;
  gdouble           remain_h, y;
  gint              w, h;
  gint              pad_x, pad_y;
  gint              diff;
  GtkStyleContext  *ctx;
  GdkRGBA           grid_rgba;
  GtkBorder         padding;

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);
  //panel_return_val_if_fail (gtk_widget_get_has_window (widget), FALSE);
  panel_return_val_if_fail (cr != NULL, FALSE);

  ctx = gtk_widget_get_style_context (widget);
  gtk_style_context_get_padding (ctx, gtk_widget_get_state_flags (widget), &padding);
  pad_x = MAX (padding.left, padding.right);
  pad_y = MAX (padding.top, padding.bottom);

  gtk_widget_get_allocation (widget, &alloc);
  alloc.width -= 1 + 2 * pad_x;
  alloc.height -= 1 + 2 * pad_y;
  alloc.x = pad_x + 1;
  alloc.y = pad_y + 1;

  /* align columns and fix rounding */
  cols = binary->true_binary ? 6 : (binary->show_seconds ? 6 : 4);
  diff = alloc.width - (floor ((gdouble) alloc.width / cols) * cols);
  alloc.width -= diff;
  alloc.x += diff / 2;

  /* align rows and fix rounding */
  rows = binary->true_binary ? (binary->show_seconds ? 3 : 2) : 4;
  diff = alloc.height - (floor ((gdouble) alloc.height / rows) * rows);
  alloc.height -= diff;
  alloc.y += diff / 2;

  if (binary->show_grid)
    {
      gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (widget),
                                   &grid_rgba);
      grid_rgba.alpha = 0.4;
      gdk_cairo_set_source_rgba (cr, &grid_rgba);
      cairo_set_line_width (cr, 1);

      remain_w = alloc.width;
      remain_h = alloc.height;
      x = alloc.x - 0.5;
      y = alloc.y - 0.5;

      cairo_rectangle (cr, x, y, alloc.width, alloc.height);
      cairo_stroke (cr);

      for (col = 0; col < cols - 1; col++)
        {
          w = remain_w / (cols - col);
          x += w; remain_w -= w;
          cairo_move_to (cr, x, alloc.y);
          cairo_rel_line_to (cr, 0, alloc.height);
          cairo_stroke (cr);
        }

      for (row = 0; row < rows - 1; row++)
        {
          h = remain_h / (rows - row);
          y += h; remain_h -= h;
          cairo_move_to (cr, alloc.x, y);
          cairo_rel_line_to (cr, alloc.width, 0);
          cairo_stroke (cr);
        }
    }

  if (binary->true_binary)
    xfce_clock_binary_draw_true_binary (binary, cr, &alloc);
  else
    xfce_clock_binary_draw_binary (binary, cr, &alloc);

  return FALSE;
}



static gboolean
xfce_clock_binary_update (XfceClockBinary     *binary,
                          ClockTime           *time)
{
  GtkWidget *widget = GTK_WIDGET (binary);

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);

  /* update if the widget if visible */
  if (G_LIKELY (gtk_widget_get_visible (widget)))
    gtk_widget_queue_draw (widget);

  return TRUE;
}



GtkWidget *
xfce_clock_binary_new (ClockTime *time)
{
  XfceClockBinary *binary = g_object_new (XFCE_CLOCK_TYPE_BINARY, NULL);

  binary->time = time;
  binary->timeout = clock_time_timeout_new (CLOCK_INTERVAL_MINUTE,
                                            binary->time,
                                            G_CALLBACK (xfce_clock_binary_update), binary);

  return GTK_WIDGET (binary);
}

