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

#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "clock.h"
#include "clock-analog.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CLOCK_SCALE 0.1
#define TICKS_TO_RADIANS(x)   (M_PI - (M_PI / 30.0) * (x))
#define HOURS_TO_RADIANS(x,y) (M_PI - (M_PI / 6.0) * (((x) > 12 ? (x) - 12 : (x)) + (y) / 60.0))



/* prototypes */
static void      xfce_clock_analog_class_init    (XfceClockAnalogClass *klass);
static void      xfce_clock_analog_init          (XfceClockAnalog      *clock);
static void      xfce_clock_analog_finalize      (GObject              *object);
static void      xfce_clock_analog_set_property  (GObject              *object,
                                                  guint                 prop_id,
                                                  const GValue         *value,
                                                  GParamSpec           *pspec);
static void      xfce_clock_analog_get_property  (GObject              *object,
                                                  guint                 prop_id,
                                                  GValue               *value,
                                                  GParamSpec           *pspec);
static void      xfce_clock_analog_size_request  (GtkWidget            *widget,
			                                      GtkRequisition       *requisition);
static gboolean  xfce_clock_analog_expose_event  (GtkWidget            *widget,
                                                  GdkEventExpose       *event);
static void      xfce_clock_analog_draw_ticks    (cairo_t              *cr,
                                                  gdouble               xc,
                                                  gdouble               yc,
                                                  gdouble               radius);
static void      xfce_clock_analog_draw_pointer  (cairo_t              *cr,
                                                  gdouble               xc,
                                                  gdouble               yc,
                                                  gdouble               radius,
                                                  gdouble               angle,
                                                  gdouble               scale,
                                                  gboolean              line);



enum
{
    PROP_0,
    PROP_SHOW_SECONDS
};

struct _XfceClockAnalogClass
{
    GtkImageClass __parent__;
};

struct _XfceClockAnalog
{
    GtkImage  __parent__;

    /* draw seconds */
    guint     show_seconds : 1;
};



G_DEFINE_TYPE (XfceClockAnalog, xfce_clock_analog, GTK_TYPE_IMAGE);



static void
xfce_clock_analog_class_init (XfceClockAnalogClass *klass)
{
    GObjectClass   *gobject_class;
    GtkWidgetClass *gtkwidget_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = xfce_clock_analog_finalize;
    gobject_class->set_property = xfce_clock_analog_set_property;
    gobject_class->get_property = xfce_clock_analog_get_property;

    gtkwidget_class = GTK_WIDGET_CLASS (klass);
    gtkwidget_class->size_request = xfce_clock_analog_size_request;
    gtkwidget_class->expose_event = xfce_clock_analog_expose_event;

    /**
     * Whether we display seconds
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_SHOW_SECONDS,
                                     g_param_spec_boolean ("show-seconds", "show-seconds", "show-seconds",
                                                           FALSE, PANEL_PARAM_READWRITE));
}



static void
xfce_clock_analog_init (XfceClockAnalog *clock)
{
    /* init */
    clock->show_seconds = FALSE;
}



static void
xfce_clock_analog_finalize (GObject *object)
{
    (*G_OBJECT_CLASS (xfce_clock_analog_parent_class)->finalize) (object);
}



static void
xfce_clock_analog_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
    XfceClockAnalog *clock = XFCE_CLOCK_ANALOG (object);

    switch (prop_id)
    {
        case PROP_SHOW_SECONDS:
            clock->show_seconds = g_value_get_boolean (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



static void
xfce_clock_analog_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
    XfceClockAnalog *clock = XFCE_CLOCK_ANALOG (object);

    switch (prop_id)
    {
        case PROP_SHOW_SECONDS:
            g_value_set_boolean (value, clock->show_seconds);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



static void
xfce_clock_analog_size_request  (GtkWidget      *widget,
			                     GtkRequisition *requisition)
{
    gint width, height;

    /* get the current widget size */
    gtk_widget_get_size_request (widget, &width, &height);

    /* square the widget */
    requisition->width = requisition->height = MAX (width, height);
}



static gboolean
xfce_clock_analog_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
    XfceClockAnalog *clock = XFCE_CLOCK_ANALOG (widget);
    gdouble          xc, yc;
    gdouble          angle, radius;
    cairo_t         *cr;
    struct tm        tm;

    g_return_val_if_fail (XFCE_CLOCK_IS_ANALOG (clock), FALSE);

    /* get center of the widget and the radius */
    xc = (widget->allocation.width / 2.0);
    yc = (widget->allocation.height / 2.0);
    radius = MIN (xc, yc);

    /* add the window offset */
    xc += widget->allocation.x;
    yc += widget->allocation.y;

    /* get the cairo context */
    cr = gdk_cairo_create (widget->window);

    if (G_LIKELY (cr != NULL))
    {
        /* clip the drawing area */
        gdk_cairo_rectangle (cr, &event->area);
        cairo_clip (cr);
        
        /* get the local time */
        xfce_clock_util_get_localtime (&tm);

        /* set the line properties */
        cairo_set_line_width (cr, 1);
        gdk_cairo_set_source_color (cr, &widget->style->fg[GTK_STATE_NORMAL]);

        /* draw the ticks */
        xfce_clock_analog_draw_ticks (cr, xc, yc, radius);

        if (clock->show_seconds)
        {
            /* second pointer */
            angle = TICKS_TO_RADIANS (tm.tm_sec);
            xfce_clock_analog_draw_pointer (cr, xc, yc, radius, angle, 0.7, TRUE);
        }

        /* minute pointer */
        angle = TICKS_TO_RADIANS (tm.tm_min);
        xfce_clock_analog_draw_pointer (cr, xc, yc, radius, angle, 0.8, FALSE);

        /* hour pointer */
        angle = HOURS_TO_RADIANS (tm.tm_hour, tm.tm_min);
        xfce_clock_analog_draw_pointer (cr, xc, yc, radius, angle, 0.5, FALSE);

        /* cleanup */
        cairo_destroy (cr);
    }

    return FALSE;
}



static void
xfce_clock_analog_draw_ticks (cairo_t *cr,
                              gdouble  xc,
                              gdouble  yc,
                              gdouble  radius)
{
    gint    i;
    gdouble x, y, angle;

    for (i = 0; i < 12; i++)
    {
        /* calculate */
        angle = HOURS_TO_RADIANS (i, 0);
        x = xc + sin (angle) * (radius * (1.0 - CLOCK_SCALE));
        y = yc + cos (angle) * (radius * (1.0 - CLOCK_SCALE));

        /* draw arc */
        cairo_move_to (cr, x, y);
        cairo_arc (cr, x, y, radius * CLOCK_SCALE, 0, 2 * M_PI);
        cairo_close_path (cr);
    }

    /* fill the arcs */
    cairo_fill (cr);
}



static void
xfce_clock_analog_draw_pointer (cairo_t *cr,
                                gdouble  xc,
                                gdouble  yc,
                                gdouble  radius,
                                gdouble  angle,
                                gdouble  scale,
                                gboolean line)
{
    gdouble xs, ys;
    gdouble xt, yt;

    /* calculate tip position */
    xt = xc + sin (angle) * radius * scale;
    yt = yc + cos (angle) * radius * scale;

    if (line)
    {
        /* draw the line */
        cairo_move_to (cr, xc, yc);
        cairo_line_to (cr, xt, yt);

        /* draw the line */
        cairo_stroke (cr);
    }
    else
    {
        /* calculate start position */
        xs = xc + sin (angle - 0.5 * M_PI) * radius * CLOCK_SCALE;
        ys = yc + cos (angle - 0.5 * M_PI) * radius * CLOCK_SCALE;

        /* draw the pointer */
        cairo_move_to (cr, xs, ys);
        cairo_arc (cr, xc, yc, radius * CLOCK_SCALE, -angle + M_PI, -angle);
        cairo_line_to (cr, xt, yt);
        cairo_close_path (cr);

        /* fill the pointer */
        cairo_fill (cr);
    }
}



GtkWidget *
xfce_clock_analog_new (void)
{
    return g_object_new (XFCE_CLOCK_TYPE_ANALOG, NULL);
}



gboolean
xfce_clock_analog_update (gpointer user_data)
{
    GtkWidget *widget = GTK_WIDGET (user_data);

    /* update if the widget if visible */
    if (G_LIKELY (GTK_WIDGET_VISIBLE (widget)))
        gtk_widget_queue_draw (widget);

    return TRUE;
}
