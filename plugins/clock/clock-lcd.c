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

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "clock.h"
#include "clock-lcd.h"

#define RELATIVE_SPACE (0.10)
#define RELATIVE_DIGIT (0.50)
#define RELATIVE_DOTS  (3 * RELATIVE_SPACE)



static void      xfce_clock_lcd_set_property (GObject           *object,
                                              guint              prop_id,
                                              const GValue      *value,
                                              GParamSpec        *pspec);
static void      xfce_clock_lcd_get_property (GObject           *object,
                                              guint              prop_id,
                                              GValue            *value,
                                              GParamSpec        *pspec);
static void      xfce_clock_lcd_finalize     (GObject           *object);
static void      xfce_clock_lcd_size_request (GtkWidget         *widget,
                                              GtkRequisition    *requisition);
static gboolean  xfce_clock_lcd_expose_event (GtkWidget         *widget,
                                              GdkEventExpose    *event);
static gdouble   xfce_clock_lcd_get_ratio    (XfceClockLcd      *lcd);
static gdouble   xfce_clock_lcd_draw_dots    (cairo_t           *cr,
                                              gdouble            size,
                                              gdouble            offset_x,
                                              gdouble            offset_y);
static gdouble   xfce_clock_lcd_draw_digit   (cairo_t           *cr,
                                              guint              number,
                                              gdouble            size,
                                              gdouble            offset_x,
                                              gdouble            offset_y);
static gboolean  xfce_clock_lcd_update       (gpointer           user_data);


enum
{
  PROP_0,
  PROP_SHOW_SECONDS,
  PROP_SHOW_MILITARY,
  PROP_SHOW_MERIDIEM,
  PROP_FLASH_SEPARATORS,
};

struct _XfceClockLcdClass
{
  GtkImageClass __parent__;
};

struct _XfceClockLcd
{
  GtkImage __parent__;

  ClockPluginTimeout *timeout;

  guint               show_seconds : 1;
  guint               show_military : 1;
  guint               show_meridiem : 1;
  guint               flash_separators : 1;
};



XFCE_PANEL_DEFINE_TYPE (XfceClockLcd, xfce_clock_lcd, GTK_TYPE_IMAGE)



static void
xfce_clock_lcd_class_init (XfceClockLcdClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = xfce_clock_lcd_set_property;
  gobject_class->get_property = xfce_clock_lcd_get_property;
  gobject_class->finalize = xfce_clock_lcd_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request = xfce_clock_lcd_size_request;
  gtkwidget_class->expose_event = xfce_clock_lcd_expose_event;

  /**
   * Whether we display seconds
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_SECONDS,
                                   g_param_spec_boolean ("show-seconds", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_STRINGS));

  /**
   * Whether we show a 24h clock
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_MILITARY,
                                   g_param_spec_boolean ("show-military", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_STRINGS));

  /**
   * Whether we show am or pm
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_MERIDIEM,
                                   g_param_spec_boolean ("show-meridiem", NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_STRINGS));

  /**
   * Whether to flash the time separators
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FLASH_SEPARATORS,
                                   g_param_spec_boolean ("flash-separators", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE
                                                         | G_PARAM_STATIC_STRINGS));
}



static void
xfce_clock_lcd_init (XfceClockLcd *lcd)
{
  lcd->show_seconds = FALSE;
  lcd->show_meridiem = FALSE;
  lcd->show_military = TRUE;
  lcd->flash_separators = FALSE;
  lcd->timeout = clock_plugin_timeout_new (CLOCK_INTERVAL_MINUTE,
                                           xfce_clock_lcd_update,
                                           lcd);
}



static void
xfce_clock_lcd_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  XfceClockLcd *lcd = XFCE_CLOCK_LCD (object);
  gboolean      show_seconds;

  switch (prop_id)
    {
      case PROP_SHOW_SECONDS:
        lcd->show_seconds = g_value_get_boolean (value);
        break;

      case PROP_SHOW_MILITARY:
        lcd->show_military = g_value_get_boolean (value);
        break;

      case PROP_SHOW_MERIDIEM:
        lcd->show_meridiem = g_value_get_boolean (value);
        break;

      case PROP_FLASH_SEPARATORS:
        lcd->flash_separators = g_value_get_boolean (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }

  /* reschedule the timeout and resize */
  show_seconds = lcd->show_seconds || lcd->flash_separators;
  clock_plugin_timeout_set_interval (lcd->timeout,
      show_seconds ? CLOCK_INTERVAL_SECOND : CLOCK_INTERVAL_MINUTE);
  gtk_widget_queue_resize (GTK_WIDGET (lcd));
}



static void
xfce_clock_lcd_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  XfceClockLcd *lcd = XFCE_CLOCK_LCD (object);

  switch (prop_id)
    {
      case PROP_SHOW_SECONDS:
        g_value_set_boolean (value, lcd->show_seconds);
        break;

      case PROP_SHOW_MILITARY:
        g_value_set_boolean (value, lcd->show_military);
        break;

      case PROP_SHOW_MERIDIEM:
        g_value_set_boolean (value, lcd->show_meridiem);
        break;

      case PROP_FLASH_SEPARATORS:
        g_value_set_boolean (value, lcd->flash_separators);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_clock_lcd_finalize (GObject *object)
{
  /* stop the timeout */
  clock_plugin_timeout_free (XFCE_CLOCK_LCD (object)->timeout);

  (*G_OBJECT_CLASS (xfce_clock_lcd_parent_class)->finalize) (object);
}



static void
xfce_clock_lcd_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
  gint    width, height;
  gdouble ratio;

  /* get the current widget size */
  gtk_widget_get_size_request (widget, &width, &height);

  /* get the width:height ratio */
  ratio = xfce_clock_lcd_get_ratio (XFCE_CLOCK_LCD (widget));

  if (width == -1)
    {
      requisition->height = MAX (10, height - height % 10);
      requisition->width = requisition->height * ratio;
    }
  else
    {
      /* calc height */
      height = width / ratio;
      height = MAX (10, height - height % 10);

      requisition->height = height;
      requisition->width = height * ratio;
    }
}



static gboolean
xfce_clock_lcd_expose_event (GtkWidget      *widget,
                             GdkEventExpose *event)
{
  XfceClockLcd *lcd = XFCE_CLOCK_LCD (widget);
  cairo_t      *cr;
  gdouble       offset_x, offset_y;
  gint          ticks, i;
  gdouble       size;
  gdouble       ratio;
  struct tm     tm;

  panel_return_val_if_fail (XFCE_CLOCK_IS_LCD (lcd), FALSE);

  /* get the width:height ratio */
  ratio = xfce_clock_lcd_get_ratio (XFCE_CLOCK_LCD (widget));

  /* size of a digit should be a fraction of 10 */
  size = widget->allocation.height - widget->allocation.height % 10;

  /* make sure we also fit on small vertical panels */
  size = MIN (rint ((gdouble) widget->allocation.width / ratio), size);

  /* begin offsets */
  offset_x = rint ((widget->allocation.width - (size * ratio)) / 2.00);
  offset_y = rint ((widget->allocation.height - size) / 2.00);

  /* only allow positive values from the base point */
  offset_x = widget->allocation.x + MAX (0.00, offset_x);
  offset_y = widget->allocation.y + MAX (0.00, offset_y);

  /* get the cairo context */
  cr = gdk_cairo_create (widget->window);

  if (G_LIKELY (cr != NULL))
    {
      /* clip the drawing region */
      gdk_cairo_rectangle (cr, &event->area);
      cairo_clip (cr);

      /* get the local time */
      clock_plugin_get_localtime (&tm);

      /* draw the hours */
      ticks = tm.tm_hour;

      /* convert 24h clock to 12h clock */
      if (!lcd->show_military && ticks > 12)
        ticks -= 12;

      /* queue a resize when the number of hour digits changed,
       * because we might miss the exact second (due to slightly delayed
       * timeout) we queue a resize the first 3 seconds or anything in
       * the first minute */
      if ((ticks == 10 || ticks == 0) && tm.tm_min == 0
          && (!lcd->show_seconds || tm.tm_sec < 3))
        gtk_widget_queue_resize (widget);

      if (ticks >= 10)
        {
          /* draw the number and increase the offset */
          offset_x = xfce_clock_lcd_draw_digit (cr, ticks >= 20 ? 2 : 1, size, offset_x, offset_y);
        }

      /* draw the other number of the hour and increase the offset */
      offset_x = xfce_clock_lcd_draw_digit (cr, ticks % 10, size, offset_x, offset_y);

      for (i = 0; i < 2; i++)
        {
          /* get the time */
          if (i == 0)
            {
              /* get the minutes */
              ticks = tm.tm_min;
            }
          else
            {
              /* leave when we don't want seconds */
              if (!lcd->show_seconds)
                break;

              /* get the seconds */
              ticks = tm.tm_sec;
            }

          /* draw the dots */
          if (lcd->flash_separators && (tm.tm_sec % 2) == 1)
            offset_x += size * RELATIVE_SPACE * 2;
          else
            offset_x = xfce_clock_lcd_draw_dots (cr, size, offset_x, offset_y);

          /* draw the first digit */
          offset_x = xfce_clock_lcd_draw_digit (cr, (ticks - (ticks % 10)) / 10, size, offset_x, offset_y);

          /* draw the second digit */
          offset_x = xfce_clock_lcd_draw_digit (cr, ticks % 10, size, offset_x, offset_y);
        }

      if (lcd->show_meridiem)
        {
          /* am or pm? */
          ticks = tm.tm_hour >= 12 ? 11 : 10;

          /* draw the digit */
          offset_x = xfce_clock_lcd_draw_digit (cr, ticks, size, offset_x, offset_y);
        }

      /* cleanup */
      cairo_destroy (cr);
    }

  return FALSE;
}



static gdouble
xfce_clock_lcd_get_ratio (XfceClockLcd *lcd)
{
  gdouble   ratio;
  gint      ticks;
  struct tm tm;

  /* get the local time */
  clock_plugin_get_localtime (&tm);

  /* 8:88 */
  ratio = (3 * RELATIVE_DIGIT) + RELATIVE_DOTS + RELATIVE_SPACE;

  ticks = tm.tm_hour;

  if (!lcd->show_military && ticks > 12)
    ticks -= 12;

  if (ticks >= 10)
    ratio += RELATIVE_DIGIT + RELATIVE_SPACE;

  if (lcd->show_seconds)
    ratio += (2 * RELATIVE_DIGIT) + RELATIVE_SPACE + RELATIVE_DOTS;

  if (lcd->show_meridiem)
    ratio += RELATIVE_DIGIT + RELATIVE_SPACE;

  return ratio;
}



static gdouble
xfce_clock_lcd_draw_dots (cairo_t *cr,
                          gdouble  size,
                          gdouble  offset_x,
                          gdouble  offset_y)
{
  gint i;

  if (size >= 10)
    {
      /* draw the dots (with rounding) */
      for (i = 1; i < 3; i++)
        cairo_rectangle (cr, rint (offset_x), rint (offset_y + size * RELATIVE_DOTS * i),
                         rint (size * RELATIVE_SPACE), rint (size * RELATIVE_SPACE));
    }
  else
    {
      /* draw the dots */
      for (i = 1; i < 3; i++)
        cairo_rectangle (cr, offset_x, offset_y + size * RELATIVE_DOTS * i,
                         size * RELATIVE_SPACE, size * RELATIVE_SPACE);
    }

  /* fill the dots */
  cairo_fill (cr);

  return (offset_x + size * RELATIVE_SPACE * 2);
}



static gdouble
xfce_clock_lcd_draw_digit (cairo_t *cr,
                           guint    number,
                           gdouble  size,
                           gdouble  offset_x,
                           gdouble  offset_y)
{
  gint    i, j;
  gint    segment;
  gdouble x, y;
  gdouble rel_x, rel_y;

  /* ##1##
   * 6   2
   * ##7##
   * 5   3
   * ##4##
   */

  /* coordicates to draw for each segment */
  gdouble segments_x[][6] = { { 0.02, 0.48,  0.38,  0.12,  -1.0,  0.00 },   /* 1x */
                              { 0.40, 0.505, 0.505, 0.40,  -1.0,  0.00 },   /* 2x */
                              { 0.40, 0.505, 0.505, 0.40,  -1.0,  0.00 },   /* 3x */
                              { 0.12, 0.38,  0.48,  0.02,  -1.0,  0.00 },   /* 4x */
                              { 0.00, 0.105, 0.105, 0.00,  -1.0,  0.00 },   /* 5x */
                              { 0.00, 0.105, 0.105, 0.00,  -1.0,  0.00 },   /* 6x */
                              { 0.00, 0.10,  0.40,  0.50,   0.40, 0.10 } }; /* 7x */
  gdouble segments_y[][6] = { { 0.00, 0.00,  0.105, 0.105, -1.0,  0.00 },   /* 1y */
                              { 0.12, 0.02,  0.48,  0.43,  -1.0,  0.00 },   /* 2y */
                              { 0.57, 0.52,  0.98,  0.88,  -1.0,  0.00 },   /* 3y */
                              { 0.90, 0.90,  1.00,  1.00,  -1.0,  0.00 },   /* 4y */
                              { 0.52, 0.57,  0.88,  0.98,  -1.0,  0.00 },   /* 5y */
                              { 0.02, 0.12,  0.43,  0.48,  -1.0,  0.00 },   /* 6y */
                              { 0.50, 0.445, 0.445, 0.50,   0.55, 0.55 } }; /* 7y */

  /* segment to draw for each number: 0, 1, ..., 9, A, P */
  gint    numbers[][8] = { { 0, 1, 2, 3, 4, 5, -1 },
                           { 1, 2, -1 },
                           { 0, 1, 6, 4, 3, -1 },
                           { 0, 1, 6, 2, 3, -1 },
                           { 5, 6, 1, 2, -1 },
                           { 0, 5, 6, 2, 3, -1 },
                           { 0, 5, 4, 3, 2, 6, -1 },
                           { 0, 1, 2, -1 },
                           { 0, 1, 2, 3, 4, 5, 6, -1 },
                           { 3, 2, 1, 0, 5, 6, -1 },
                           { 4, 5, 0, 1, 2, 6, -1 },
                           { 4, 5, 0, 1, 6, -1 } };

  panel_return_val_if_fail (number <= 11, offset_x);

  for (i = 0; i < 9; i++)
    {
      /* get the segment we're going to draw */
      segment = numbers[number][i];

      /* leave when there are no more segments left */
      if (segment == -1)
        break;

      /* walk through the coordinate points */
      for (j = 0; j < 6; j++)
        {
          /* get the relative sizes */
          rel_x = segments_x[segment][j];
          rel_y = segments_y[segment][j];

          /* leave when there are no valid coordinates */
          if (rel_x == -1.00 || rel_y == -1.00)
            break;

          /* get x and y coordinates for this point */
          x = rel_x * size + offset_x;
          y = rel_y * size + offset_y;

          /* when 0.01 * size is larger then 1, round the numbers */
          if (size >= 10)
            {
              x = rint (x);
              y = rint (y);
            }

          if (G_UNLIKELY (j == 0))
            cairo_move_to (cr, x, y);
          else
            cairo_line_to (cr, x, y);
        }

      /* close the line */
      cairo_close_path (cr);
    }

  /* fill the segments */
  cairo_fill (cr);

  return (offset_x + size * (RELATIVE_DIGIT + RELATIVE_SPACE));
}



static gboolean
xfce_clock_lcd_update (gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);

  panel_return_val_if_fail (XFCE_CLOCK_IS_LCD (user_data), FALSE);

  /* update if the widget if visible */
  if (G_LIKELY (GTK_WIDGET_VISIBLE (widget)))
    gtk_widget_queue_draw (widget);

  return TRUE;
}



GtkWidget *
xfce_clock_lcd_new (void)
{
  return g_object_new (XFCE_CLOCK_TYPE_LCD, NULL);
}
