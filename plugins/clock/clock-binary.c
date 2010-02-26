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
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#ifndef M_PI
#define M_PI 3.141592654
#endif

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "clock.h"
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
static void      xfce_clock_binary_size_request  (GtkWidget            *widget,
                                                  GtkRequisition       *requisition);
static gboolean  xfce_clock_binary_expose_event  (GtkWidget            *widget,
                                                  GdkEventExpose       *event);
static gboolean  xfce_clock_binary_update        (gpointer              user_data);



enum
{
  PROP_0,
  PROP_SHOW_SECONDS,
  PROP_TRUE_BINARY,
  PROP_SHOW_INACTIVE,
  PROP_SHOW_GRID
};

struct _XfceClockBinaryClass
{
  GtkImageClass __parent__;
};

struct _XfceClockBinary
{
  GtkImage  __parent__;

  ClockPluginTimeout *timeout;

  guint     show_seconds : 1;
  guint     true_binary : 1;
  guint     show_inactive : 1;
  guint     show_grid : 1;
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
  gtkwidget_class->size_request = xfce_clock_binary_size_request;
  gtkwidget_class->expose_event = xfce_clock_binary_expose_event;

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
  /* init */
  binary->show_seconds = FALSE;
  binary->true_binary = FALSE;
  binary->show_inactive = TRUE;
  binary->show_grid = FALSE;

  binary->timeout = clock_plugin_timeout_new (CLOCK_INTERVAL_MINUTE,
                                              xfce_clock_binary_update,
                                              binary);
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
    case PROP_SHOW_SECONDS:
      binary->show_seconds = g_value_get_boolean (value);
      break;

    case PROP_TRUE_BINARY:
      binary->true_binary = g_value_get_boolean (value);
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
  clock_plugin_timeout_set_interval (binary->timeout,
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_clock_binary_finalize (GObject *object)
{
  /* stop the timeout */
  clock_plugin_timeout_free (XFCE_CLOCK_BINARY (object)->timeout);

  (*G_OBJECT_CLASS (xfce_clock_binary_parent_class)->finalize) (object);
}



static void
xfce_clock_binary_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  gint             width, height;
  gdouble          ratio;
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);

  /* get the current widget size */
  gtk_widget_get_size_request (widget, &width, &height);

  /* ratio of the clock */
  if (binary->true_binary)
    ratio = binary->show_seconds ? 2.0 : 3.0;
  else
    ratio = binary->show_seconds ? 1.5 : 1.0;

  /* set requisition based on the plugin orientation */
  if (width == -1)
    {
      requisition->height = height;
      requisition->width = height * ratio;
    }
  else
    {
      requisition->height = width / ratio;
      requisition->width = width;
    }
}



static void
xfce_clock_binary_expose_event_true_binary (XfceClockBinary *binary,
                                            cairo_t         *cr,
                                            GtkAllocation   *alloc)
{
  GdkColor    *active, *inactive;
  struct tm    tm;
  gint         row, rows;
  static gint  binary_table[] = { 32, 16, 8, 4, 2, 1 };
  gint         col, cols = G_N_ELEMENTS (binary_table);
  gint         remain_h, remain_w;
  gint         offset_x, offset_y;
  gint         w, h, x;
  gint         ticks;
  gint         pad_x, pad_y;

  inactive = &(GTK_WIDGET (binary)->style->fg[GTK_STATE_NORMAL]);
  active = &(GTK_WIDGET (binary)->style->bg[GTK_STATE_SELECTED]);

  clock_plugin_get_localtime (&tm);

  gtk_misc_get_padding (GTK_MISC (binary), &pad_x, &pad_y);

  /* init sizes */
  remain_h = alloc->height - 1 - 2 * pad_y;
  offset_y = alloc->y + 1 + pad_y;

  rows = binary->show_seconds ? 3 : 2;
  for (row = 0; row < rows; row++)
    {
      /* get the time this row represents */
      if (row == 0)
        ticks = tm.tm_hour;
      else if (row == 1)
        ticks = tm.tm_min;
      else
        ticks = tm.tm_sec;

      /* reset sizes */
      remain_w = alloc->width - 1 - 2 * pad_x;
      offset_x = alloc->x + 1 + pad_x;
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
              gdk_cairo_set_source_color (cr, active);
              ticks -= binary_table[col];
            }
          else if (binary->show_inactive)
            {
              gdk_cairo_set_source_color (cr, inactive);
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
}



static void
xfce_clock_binary_expose_event_binary (XfceClockBinary *binary,
                                       cairo_t         *cr,
                                       GtkAllocation   *alloc)
{
  GdkColor    *active, *inactive;
  static gint  binary_table[] = { 80, 40, 20, 10, 8, 4, 2, 1 };
  struct tm    tm;
  gint         row, rows = G_N_ELEMENTS (binary_table) / 2;
  gint         col, cols;
  gint         digit;
  gint         remain_h, remain_w;
  gint         offset_x, offset_y;
  gint         w, h, y;
  gint         ticks;
  gint         pad_x, pad_y;

  inactive = &(GTK_WIDGET (binary)->style->fg[GTK_STATE_NORMAL]);
  active = &(GTK_WIDGET (binary)->style->bg[GTK_STATE_SELECTED]);

  clock_plugin_get_localtime (&tm);

  gtk_misc_get_padding (GTK_MISC (binary), &pad_x, &pad_y);

  remain_w = alloc->width - 1 - 2 * pad_x;
  offset_x = alloc->x + 1 + pad_x;

  cols = binary->show_seconds ? 6 : 4;
  for (col = 0; col < cols; col++)
    {
      /* get the time this row represents */
      if (col == 0)
        ticks = tm.tm_hour;
      else if (col == 2)
        ticks = tm.tm_min;
      else if (col == 4)
        ticks = tm.tm_sec;

      /* reset sizes */
      remain_h = alloc->height - 1 -  2 * pad_y;
      offset_y = alloc->y + 1 + pad_x;
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
              gdk_cairo_set_source_color (cr, active);
              ticks -= binary_table[digit];
            }
          else if (binary->show_inactive)
            {
              gdk_cairo_set_source_color (cr, inactive);
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
}



static gboolean
xfce_clock_binary_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);
  cairo_t         *cr;
  GdkColor        *color;
  gint             col, cols;
  gint             row, rows;
  GtkAllocation   *alloc;
  gdouble          remain_w, x;
  gdouble          remain_h, y;
  gint             w, h;
  gint             pad_x, pad_y;

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);
  panel_return_val_if_fail (GDK_IS_WINDOW (widget->window), FALSE);

  cr = gdk_cairo_create (widget->window);
  if (G_LIKELY (cr != NULL))
    {
      /* clip the drawing region */
      gdk_cairo_rectangle (cr, &event->area);
      cairo_clip (cr);

      if (binary->show_grid)
        {
          color = &(GTK_WIDGET (binary)->style->fg[GTK_STATE_NORMAL]);
          gdk_cairo_set_source_color (cr, color);
          cairo_set_line_width (cr, 1);

          cols = binary->true_binary ? 6 : (binary->show_seconds ? 6 : 4);
          rows = binary->true_binary ? (binary->show_seconds ? 2 : 3) : 4;

          alloc = &widget->allocation;

          gtk_misc_get_padding (GTK_MISC (widget), &pad_x, &pad_y);

          x = pad_x + alloc->x + 0.5;
          y = pad_y + alloc->y + 0.5;
          remain_w = alloc->width - 1 - 2 * pad_x;
          remain_h = alloc->height - 1 - 2 * pad_y;

          cairo_rectangle (cr, x, y, remain_w, remain_h);
          cairo_stroke (cr);

          for (col = 0; col < cols - 1; col++)
            {
              w = remain_w / (cols - col);
              x += w; remain_w -= w;
              cairo_move_to (cr, x, alloc->y);
              cairo_rel_line_to (cr, 0, alloc->height);
              cairo_stroke (cr);
            }

          for (row = 0; row < rows - 1; row++)
            {
              h = remain_h / (rows - row);
              y += h; remain_h -= h;
              cairo_move_to (cr, alloc->x, y);
              cairo_rel_line_to (cr, alloc->width, 0);
              cairo_stroke (cr);
            }
        }

      if (binary->true_binary)
        xfce_clock_binary_expose_event_true_binary (binary, cr, &widget->allocation);
      else
        xfce_clock_binary_expose_event_binary (binary, cr, &widget->allocation);

      cairo_destroy (cr);
    }

  return FALSE;
}



static gboolean
xfce_clock_binary_update (gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (user_data), FALSE);

  /* update if the widget if visible */
  if (G_LIKELY (GTK_WIDGET_VISIBLE (widget)))
    gtk_widget_queue_draw (widget);

  return TRUE;
}



GtkWidget *
xfce_clock_binary_new (void)
{
  return g_object_new (XFCE_CLOCK_TYPE_BINARY, NULL);
}
