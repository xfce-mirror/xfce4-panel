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



#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "clock.h"
#include "clock-lcd.h"

#define RELATIVE_SPACE 0.10



/* prototypes */
static void      xfce_clock_lcd_class_init   (XfceClockLcdClass *klass);
static void      xfce_clock_lcd_init         (XfceClockLcd      *clock);
static void      xfce_clock_lcd_set_property (GObject           *object,
                                              guint              prop_id,
                                              const GValue      *value,
                                              GParamSpec        *pspec);
static void      xfce_clock_lcd_get_property (GObject           *object,
                                              guint              prop_id,
                                              GValue            *value,
                                              GParamSpec        *pspec);
static void      xfce_clock_lcd_size_request (GtkWidget         *widget,
                                              GtkRequisition    *requisition);
static gboolean  xfce_clock_lcd_expose_event (GtkWidget         *widget,
                                              GdkEventExpose    *event);
static gdouble   xfce_clock_lcd_get_ratio    (XfceClockLcd      *clock);
static gdouble   xfce_clock_lcd_draw_dots    (cairo_t           *cr,
                                              gdouble            size,
                                              gdouble            offset_x,
                                              gdouble            offset_y);
static gdouble   xfce_clock_lcd_draw_digit   (cairo_t           *cr,
                                              guint              number,
                                              gdouble            size,
                                              gdouble            offset_x,
                                              gdouble            offset_y);



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

    /* whether we show seconds */
    guint    show_seconds : 1;

    /* whether it's a 24h clock */
    guint    show_military : 1;

    /* whether we display am/pm */
    guint    show_meridiem : 1;

    /* whether to flash the separators */
    guint    flash_separators : 1;
};



XFCE_PANEL_DEFINE_TYPE (XfceClockLcd, xfce_clock_lcd, GTK_TYPE_IMAGE);



static void
xfce_clock_lcd_class_init (XfceClockLcdClass *klass)
{
    GObjectClass   *gobject_class;
    GtkWidgetClass *gtkwidget_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->set_property = xfce_clock_lcd_set_property;
    gobject_class->get_property = xfce_clock_lcd_get_property;

    gtkwidget_class = GTK_WIDGET_CLASS (klass);
    gtkwidget_class->size_request = xfce_clock_lcd_size_request;
    gtkwidget_class->expose_event = xfce_clock_lcd_expose_event;

    /**
     * Whether we display seconds
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_SECONDS,
                                     g_param_spec_boolean ("show-seconds", 
                                                           "show-seconds", 
                                                           "show-seconds",
                                                           FALSE,
                                                           G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS));

    /**
     * Whether we show a 24h clock
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_MILITARY,
                                     g_param_spec_boolean ("show-military", 
                                                           "show-military", 
                                                           "show-military",
                                                           FALSE,
                                                           G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS));

    /**
     * Whether we show am or pm
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_MERIDIEM,
                                     g_param_spec_boolean ("show-meridiem", 
                                                           "show-meridiem", 
                                                           "show-meridiem",
                                                           TRUE,
                                                           G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS));

    /**
     * Whether to flash the time separators
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_FLASH_SEPARATORS,
                                     g_param_spec_boolean ("flash-separators", 
                                                           "flash-separators", 
                                                           "flash-separators",
                                                           FALSE,
                                                           G_PARAM_READWRITE | PANEL_PARAM_STATIC_STRINGS));
}



static void
xfce_clock_lcd_init (XfceClockLcd *clock)
{
    /* init */
    clock->show_seconds = FALSE;
    clock->show_meridiem = FALSE;
    clock->show_military = TRUE;
    clock->flash_separators = FALSE;
}



static void
xfce_clock_lcd_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    XfceClockLcd *clock = XFCE_CLOCK_LCD (object);

    switch (prop_id)
    {
        case PROP_SHOW_SECONDS:
            clock->show_seconds = g_value_get_boolean (value);
            break;

        case PROP_SHOW_MILITARY:
            clock->show_military = g_value_get_boolean (value);
            break;

        case PROP_SHOW_MERIDIEM:
            clock->show_meridiem = g_value_get_boolean (value);
            break;

        case PROP_FLASH_SEPARATORS:
            clock->flash_separators = g_value_get_boolean (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



static void
xfce_clock_lcd_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    XfceClockLcd *clock = XFCE_CLOCK_LCD (object);

    switch (prop_id)
    {
        case PROP_SHOW_SECONDS:
            g_value_set_boolean (value, clock->show_seconds);
            break;

        case PROP_SHOW_MILITARY:
            g_value_set_boolean (value, clock->show_military);
            break;

        case PROP_SHOW_MERIDIEM:
            g_value_set_boolean (value, clock->show_meridiem);
            break;

        case PROP_FLASH_SEPARATORS:
            g_value_set_boolean (value, clock->flash_separators);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
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
    XfceClockLcd *clock = XFCE_CLOCK_LCD (widget);
    cairo_t      *cr;
    gdouble       offset_x, offset_y;
    gint          ticks, i;
    gdouble       size;
    gdouble       ratio;
    struct tm     tm;

    panel_return_val_if_fail (XFCE_IS_CLOCK_LCD (clock), FALSE);

    /* size of a digit should be a fraction of 10 */
    size = widget->allocation.height - widget->allocation.height % 10;

    /* get the width:height ratio */
    ratio = xfce_clock_lcd_get_ratio (XFCE_CLOCK_LCD (widget));

    /* begin offsets */
    offset_x = widget->allocation.x + (widget->allocation.width - (size * ratio)) / 2;
    offset_y = widget->allocation.y + (widget->allocation.height - size) / 2;

    /* get the cairo context */
    cr = gdk_cairo_create (widget->window);

    if (G_LIKELY (cr != NULL))
    {
        /* get the local time */
        xfce_clock_util_get_localtime (&tm);

        /* draw the hours */
        ticks = tm.tm_hour;

        /* convert 24h clock to 12h clock */
        if (!clock->show_military && ticks > 12)
            ticks -= 12;

        /* queue a resize when the number of hour digits changed */
        if (G_UNLIKELY ((ticks == 10 || ticks == 0) && tm.tm_min == 0 && tm.tm_sec == 0))
          gtk_widget_queue_resize (widget);

        if (ticks >= 10)
        {
            /* draw the number and increase the offset*/
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
                if (!clock->show_seconds)
                    break;

                /* get the seconds */
                ticks = tm.tm_sec;
            }

            /* draw the dots */
            if (clock->flash_separators && (tm.tm_sec % 2) == 1)
                offset_x += size * RELATIVE_SPACE * 2;
            else
                offset_x = xfce_clock_lcd_draw_dots (cr, size, offset_x, offset_y);

            /* draw the first digit */
            offset_x = xfce_clock_lcd_draw_digit (cr, (ticks - (ticks % 10)) / 10, size, offset_x, offset_y);

            /* draw the second digit */
            offset_x = xfce_clock_lcd_draw_digit (cr, ticks % 10, size, offset_x, offset_y);
        }

        if (clock->show_meridiem)
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
xfce_clock_lcd_get_ratio (XfceClockLcd *clock)
{
    gdouble   ratio;
    gint      ticks;
    struct tm tm;

    /* get the local time */
    xfce_clock_util_get_localtime (&tm);

    /* hour + minutes */
    ratio = (3 * 0.5 + 6 * RELATIVE_SPACE);

    ticks = tm.tm_hour;

    if (!clock->show_military && ticks > 12)
        ticks -= 12;

    if (ticks >= 10)
        ratio += (0.5 + RELATIVE_SPACE);

    if (clock->show_seconds)
        ratio += (2 * 0.5 + 4 * RELATIVE_SPACE);

    if (clock->show_meridiem)
        ratio += (0.5 + RELATIVE_SPACE);

    return ratio;
}



static gdouble
xfce_clock_lcd_draw_dots (cairo_t *cr,
                          gdouble  size,
                          gdouble  offset_x,
                          gdouble  offset_y)
{
    gint i;

    /* draw the dots */
    for (i = 1; i < 3; i++)
        cairo_rectangle (cr, offset_x, offset_y + size * 0.30 * i,
                         size * 0.10, size * 0.10);

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

    panel_return_val_if_fail (number >= 0 || number <= 11, offset_x);

    /* coordicates to draw for each segment */
    gdouble segments_x[][6] = { { 0.02, 0.48, 0.38, 0.12, -1.0, 0.00 },
                                { 0.40, 0.50, 0.50, 0.40, -1.0, 0.00 },
                                { 0.40, 0.50, 0.50, 0.40, -1.0, 0.00 },
                                { 0.12, 0.38, 0.48, 0.02, -1.0, 0.00 },
                                { 0.00, 0.10, 0.10, 0.00, -1.0, 0.00 },
                                { 0.00, 0.10, 0.10, 0.00, -1.0, 0.00 },
                                { 0.00, 0.10, 0.40, 0.50, 0.40, 0.10 } };
    gdouble segments_y[][6] = { { 0.00, 0.00, 0.10, 0.10, -1.0, 0.00 },
                                { 0.12, 0.02, 0.48, 0.43, -1.0, 0.00 },
                                { 0.57, 0.52, 0.98, 0.88, -1.0, 0.00 },
                                { 0.90, 0.90, 1.00, 1.00, -1.0, 0.00 },
                                { 0.52, 0.57, 0.88, 0.98, -1.0, 0.00 },
                                { 0.02, 0.12, 0.43, 0.48, -1.0, 0.00 },
                                { 0.50, 0.45, 0.45, 0.50, 0.55, 0.55 } };

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
            /* get x and y coordinates for this point */
            x = segments_x[segment][j] * size + offset_x;
            y = segments_y[segment][j] * size + offset_y;

            /* leave when there are no valid coordinates */
            if (x < 0 || y < 0)
                break;

            if (j == 0)
                cairo_move_to (cr, x, y);
            else
                cairo_line_to (cr, x, y);
        }

        /* close the line */
        cairo_close_path (cr);
    }

    /* fill the segments */
    cairo_fill (cr);

    return (offset_x + size * (0.50 + RELATIVE_SPACE));
}



GtkWidget *
xfce_clock_lcd_new (void)
{
    return g_object_new (XFCE_TYPE_CLOCK_LCD, NULL);
}



gboolean
xfce_clock_lcd_update (gpointer user_data)
{
    GtkWidget *widget = GTK_WIDGET (user_data);

    /* update if the widget if visible */
    if (G_LIKELY (GTK_WIDGET_VISIBLE (widget)))
        gtk_widget_queue_draw (widget);

    return TRUE;
}
