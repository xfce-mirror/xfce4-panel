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
  PROP_TRUE_BINARY
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
}



static void
xfce_clock_binary_init (XfceClockBinary *binary)
{
  /* init */
  binary->show_seconds = FALSE;
  binary->true_binary = FALSE;
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



static gboolean
xfce_clock_binary_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  XfceClockBinary *binary = XFCE_CLOCK_BINARY (widget);
  gdouble          cw, ch, columns;
  gint             ticks, cells, decimal;
  gdouble          radius;
  gdouble          x, y;
  gint             i, j;
  gint             decimal_tb[] = {32, 16, 8, 4, 2, 1};
  gint             decimal_bcd[] = {80, 40, 20, 10, 8, 4, 2, 1};
  cairo_t         *cr;
  GdkColor         active, inactive;
  struct tm        tm;

  panel_return_val_if_fail (XFCE_CLOCK_IS_BINARY (binary), FALSE);

  /* number of columns and cells */
  columns = binary->show_seconds ? 3.0 : 2.0;
  cells = binary->true_binary ? 6 : 8;

  /* cell width and height */
  if (binary->true_binary)
    {
      cw = widget->allocation.width / 6.0;
      ch = widget->allocation.height / columns;
    }
  else /* bcd clock */
    {
      cw = widget->allocation.width / columns / 2.0;
      ch = widget->allocation.height / 4.0;
    }

  /* arc radius */
  radius = MIN (cw, ch) / 2.0 * 0.7;

  /* get colors */
  inactive = widget->style->fg[GTK_STATE_NORMAL];
  active = widget->style->bg[GTK_STATE_SELECTED];

  /* get the cairo context */
  cr = gdk_cairo_create (widget->window);

  if (G_LIKELY (cr != NULL))
    {
        /* clip the drawing region */
        gdk_cairo_rectangle (cr, &event->area);
        cairo_clip (cr);

        /* get the current time */
        clock_plugin_get_localtime (&tm);

        /* walk the three or two time parts */
        for (i = 0; i < columns; i++)
        {
          /* get the time of this column */
          if (i == 0)
            ticks = tm.tm_hour;
          else if (i == 1)
            ticks = tm.tm_min;
          else
            ticks = tm.tm_sec;

          /* walk the binary columns */
          for (j = 0; j < cells; j++)
            {
              if (binary->true_binary)
                {
                  /* skip the columns we don't draw */
                  if (i == 0 && j == 0)
                    continue;

                  /* decimal representation of this cell */
                  decimal = decimal_tb[j];

                  /* center of the arc */
                  x = cw * (j + 0.5) + widget->allocation.x;
                  y = ch * (i + 0.5) + widget->allocation.y;
                }
              else /* bcd clock */
                {
                  /* skip the columns we don't draw */
                  if ((j == 0) || (i == 0 && j == 1))
                    continue;

                  /* decimal representation of this cell */
                  decimal = decimal_bcd[j];

                  /* center of the arc */
                  x = cw * (i * 2 + (j < 4 ? 0 : 1) + 0.5) + widget->allocation.x;
                  y = ch * ((j >= 4 ? j - 4 : j) + 0.5) + widget->allocation.y;
                }

              /* if this binary values 'fits' in the time */
              if (ticks >= decimal)
                {
                  /* extract the decimal value from the time */
                  ticks -= decimal;

                  /* set the active color */
                  gdk_cairo_set_source_color (cr, &active);
                }
              else
                {
                  /* set the inactive color */
                  gdk_cairo_set_source_color (cr, &inactive);
                }

              /* draw the arc */
              cairo_move_to (cr, x, y);
              cairo_arc (cr, x, y, radius, 0, 2 * M_PI);
              cairo_close_path (cr);

              /* fill the arc */
              cairo_fill (cr);
            }
        }

      /* cleanup */
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
