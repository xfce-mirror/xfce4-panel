/*  xfce4
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Original code by Olivier Fourdan, port to gtk2 by Jasper Huijsmans
   and Xavier Maillard*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PANGO_ENABLE_BACKEND 1

#include <gtk/gtkmain.h>
#include <pango/pango.h>
#include "xfce_clock.h"
#include "icons/digits.xbm"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#define MY_CLOCK_DEFAULT_SIZE 100

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define digits_tiny_width   3
#define digits_tiny_height  6

#define digits_small_width    6
#define digits_small_height  10

#define digits_medium_width    8
#define digits_medium_height  14

#define digits_large_width  12
#define digits_large_height 20

#define digits_huge_width   18
#define digits_huge_height  30



/* Forward declarations */

static void xfce_clock_class_init(XfceClockClass * klass);
static void xfce_clock_init(XfceClock * clock);
static void xfce_clock_destroy(GtkObject * object);

static void xfce_clock_realize(GtkWidget * widget);
static void xfce_clock_size_request(GtkWidget * widget,
                                    GtkRequisition * requisition);
static void xfce_clock_size_allocate(GtkWidget * widget,
                                     GtkAllocation * allocation);
static gint xfce_clock_expose(GtkWidget * widget, GdkEventExpose * event);
static void xfce_clock_draw(GtkWidget * widget, GdkRectangle * area);
static gint xfce_clock_timer(XfceClock * clock);
static void xfce_clock_draw_internal(GtkWidget * widget);

static void draw_digits(XfceClock * clock, GdkGC * gc, gint x, gint y, gchar c);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GtkType xfce_clock_get_type(void)
{
    static GtkType clock_type = 0;

    if(!clock_type)
    {
        static const GTypeInfo clock_info = {
            sizeof(XfceClockClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) xfce_clock_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(XfceClock),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) xfce_clock_init
        };

        clock_type =
            g_type_register_static(GTK_TYPE_WIDGET, "XfceClock", &clock_info,
                                   0);
    }

    return clock_type;
}

static void xfce_clock_class_init(XfceClockClass * class)
{
    GtkObjectClass *object_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;

    parent_class = gtk_type_class(gtk_widget_get_type());

    object_class->destroy = xfce_clock_destroy;

    widget_class->realize = xfce_clock_realize;
    widget_class->expose_event = xfce_clock_expose;
    widget_class->size_request = xfce_clock_size_request;
    widget_class->size_allocate = xfce_clock_size_allocate;
}

static void xfce_clock_realize(GtkWidget * widget)
{
    XfceClock *clock;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    clock = XFCE_CLOCK(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window =
        gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);

    clock->digits_bmap =
        gdk_bitmap_create_from_data(widget->window, digits_bits, digits_width,
                                    digits_height);

    gdk_window_set_user_data(widget->window, widget);

    gtk_style_set_background(widget->parent->style, widget->window,
                             GTK_STATE_NORMAL);

    if(!(clock->timer))
        clock->timer =
            gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer,
                            (gpointer) clock);
}

static void xfce_clock_init(XfceClock * clock)
{
    time_t ticks;
    struct tm *tm;
    gint h, m, s;

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;
    clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
    clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
    clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;

    clock->timer = 0;
    clock->radius = 0;
    clock->pointer_width = 0;
    clock->relief = FALSE; /* TRUE */
    clock->shadow_type = GTK_SHADOW_NONE;
    clock->parent_style = NULL;

    clock->mode = XFCE_CLOCK_ANALOG;
    clock->military_time = 0;   /* use 12 hour mode by default */
    clock->display_am_pm = 1;   /* display am or pm by default. */
    clock->display_secs = 0;    /* do not display secs by default */
    clock->interval = 100;      /* 1/10 seconds update interval by default */
}

GtkWidget *xfce_clock_new(void)
{
    XfceClock *clock;

    clock = (XfceClock *) gtk_type_new(xfce_clock_get_type());

    return GTK_WIDGET(clock);
}

static void xfce_clock_destroy(GtkObject * object)
{
    XfceClock *clock;

    g_return_if_fail(object != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(object));

    clock = XFCE_CLOCK(object);

    if(GDK_IS_DRAWABLE(clock->digits_bmap))
        gdk_bitmap_unref(clock->digits_bmap);

    if(clock->timer)
        gtk_timeout_remove(clock->timer);

    if(GTK_OBJECT_CLASS(parent_class)->destroy)
        (*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

void xfce_clock_show_ampm(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    clock->display_am_pm = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_ampm_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->display_am_pm)
        clock->display_am_pm = 0;
    else
        clock->display_am_pm = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_ampm_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->display_am_pm);
}

void xfce_clock_show_secs(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->display_secs = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_secs_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->display_secs)
        clock->display_secs = 0;
    else
        clock->display_secs = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_secs_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->display_secs);
}

void xfce_clock_show_military(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->military_time = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_military_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->military_time)
    {
        clock->military_time = 0;
        xfce_clock_show_ampm(clock, 0);
    }
    else
        clock->military_time = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_military_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->military_time);
}

void xfce_clock_set_interval(XfceClock * clock, guint interval)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->interval = interval;

    if(clock->timer)
    {
        gtk_timeout_remove(clock->timer);
        clock->timer =
            gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer,
                            (gpointer) clock);
    }
}

guint xfce_clock_get_interval(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, 0);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), 0);
    return (clock->interval);
}

void xfce_clock_suspend(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->timer)
    {
        gtk_timeout_remove(clock->timer);
        clock->timer = 0;
    }
}

void xfce_clock_resume(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->timer)
        return;
    if(!(clock->interval))
        return;

    clock->timer =
        gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer,
                        (gpointer) clock);
}

void xfce_clock_set_mode(XfceClock * clock, XfceClockMode mode)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->mode = mode;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_toggle_mode(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    switch (clock->mode)
    {
        case XFCE_CLOCK_ANALOG:
            clock->mode = XFCE_CLOCK_DIGITAL;
            break;
        case XFCE_CLOCK_DIGITAL:
            clock->mode = XFCE_CLOCK_LEDS;
            break;
        case XFCE_CLOCK_LEDS:
        default:
            clock->mode = XFCE_CLOCK_ANALOG;
    }
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

XfceClockMode xfce_clock_get_mode(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, 0);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), 0);
    return (clock->mode);
}

static void xfce_clock_size_request(GtkWidget * widget,
                                    GtkRequisition * requisition)
{
    requisition->width = MY_CLOCK_DEFAULT_SIZE;
    requisition->height = MY_CLOCK_DEFAULT_SIZE;
}

static void xfce_clock_size_allocate(GtkWidget * widget,
                                     GtkAllocation * allocation)
{
    XfceClock *clock;
    gint size;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(allocation != NULL);

    widget->allocation = *allocation;
    clock = XFCE_CLOCK(widget);

    if(GTK_WIDGET_REALIZED(widget))
    {

        gdk_window_move_resize(widget->window, allocation->x, allocation->y,
                               allocation->width, allocation->height);

    }
    size = MIN(allocation->width, allocation->height);
    clock->radius = size * 0.45;
    clock->internal = size * 0.49;
    clock->pointer_width = MAX(clock->radius / 5, 3);
}

static void draw_round_frame(GtkWidget * widget)
{
    XfceClock *clock;
    GtkStyle *style;
    gint x, y, w, h;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    style = gtk_widget_get_style(widget);

    w = h = 2 * clock->internal;
    x = widget->allocation.width / 2 - clock->internal;
    y = widget->allocation.height / 2 - clock->internal;

    gdk_draw_arc(widget->window, style->base_gc[GTK_STATE_NORMAL], TRUE, x, y,
                 w, h, 0, 360 * 64);
    gdk_draw_arc(widget->window, style->dark_gc[GTK_STATE_NORMAL], FALSE, x, y,
                 w, h, 30 * 64, 180 * 64);
    gdk_draw_arc(widget->window, style->black_gc, FALSE, x + 1, y + 1, w - 2,
                 h - 2, 30 * 64, 180 * 64);
    gdk_draw_arc(widget->window, style->light_gc[GTK_STATE_NORMAL], FALSE, x, y,
                 w, h, 210 * 64, 180 * 64);
}


static void draw_ticks(GtkWidget * widget)
{
    XfceClock *clock;
    gint xc, yc;
    gint i;
    gdouble theta;
    gdouble s, c;
    GdkPoint points[5];

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    xc = widget->allocation.width / 2;
    yc = widget->allocation.height / 2;

    for(i = 0; i < 12; i++)
    {
        theta = (i * M_PI / 6.);
        s = sin(theta);
        c = cos(theta);

        points[0].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[0].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[1].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[1].y =
            yc + c * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[2].x =
            xc + s * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[2].y =
            yc + c * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[3].x =
            xc + s * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[3].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[4].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[4].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;

        if(clock->relief)
            gtk_draw_polygon(widget->style, widget->window, GTK_STATE_NORMAL,
                             GTK_SHADOW_OUT, points, 5, TRUE);
        else
            gdk_draw_polygon(widget->window,
                             widget->style->text_gc[GTK_STATE_NORMAL], TRUE,
                             points, 5);
    }

}

static void draw_sec_pointer(GtkWidget * widget)
{
    XfceClock *clock;
    GdkPoint points[5];
    gint xc, yc;
    gdouble s, c;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    xc = widget->allocation.width / 2;
    yc = widget->allocation.height / 2;

    s = sin(clock->sec_angle);
    c = cos(clock->sec_angle);


    points[0].x = xc + s * clock->pointer_width / 4;
    points[0].y = yc + c * clock->pointer_width / 4;
    points[1].x = xc + c * clock->radius;
    points[1].y = yc - s * clock->radius;
    points[2].x = xc - s * clock->pointer_width / 4;
    points[2].y = yc - c * clock->pointer_width / 4;
    points[3].x = xc - c * clock->pointer_width / 4;
    points[3].y = yc + s * clock->pointer_width / 4;
    points[4].x = xc + s * clock->pointer_width / 4;
    points[4].y = yc + c * clock->pointer_width / 4;

    if(clock->relief)
        gtk_draw_polygon(widget->style, widget->window, GTK_STATE_NORMAL,
                         GTK_SHADOW_OUT, points, 5, TRUE);
    else
        gdk_draw_polygon(widget->window,
                         widget->style->text_gc[GTK_STATE_NORMAL], TRUE, points,
                         5);

}

static void draw_min_pointer(GtkWidget * widget)
{
    XfceClock *clock;
    GdkPoint points[5];
    gint xc, yc;
    gdouble s, c;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    xc = widget->allocation.width / 2;
    yc = widget->allocation.height / 2;

    s = sin(clock->min_angle);
    c = cos(clock->min_angle);


    points[0].x = xc + s * clock->pointer_width / 2;
    points[0].y = yc + c * clock->pointer_width / 2;
    points[1].x = xc + c * clock->radius;
    points[1].y = yc - s * clock->radius;
    points[2].x = xc - s * clock->pointer_width / 2;
    points[2].y = yc - c * clock->pointer_width / 2;
    points[3].x = xc - c * clock->pointer_width / 2;
    points[3].y = yc + s * clock->pointer_width / 2;
    points[4].x = xc + s * clock->pointer_width / 2;
    points[4].y = yc + c * clock->pointer_width / 2;

    if(clock->relief)
        gtk_draw_polygon(widget->style, widget->window, GTK_STATE_NORMAL,
                         GTK_SHADOW_OUT, points, 5, TRUE);
    else
        gdk_draw_polygon(widget->window,
                         widget->style->text_gc[GTK_STATE_NORMAL], TRUE, points,
                         5);

}

static void draw_hrs_pointer(GtkWidget * widget)
{
    XfceClock *clock;
    GdkPoint points[5];
    gint xc, yc;
    gdouble s, c;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    xc = widget->allocation.width / 2;
    yc = widget->allocation.height / 2;

    s = sin(clock->hrs_angle);
    c = cos(clock->hrs_angle);


    points[0].x = xc + s * clock->pointer_width / 2;
    points[0].y = yc + c * clock->pointer_width / 2;
    points[1].x = xc + 3 * c * clock->radius / 5;
    points[1].y = yc - 3 * s * clock->radius / 5;
    points[2].x = xc - s * clock->pointer_width / 2;
    points[2].y = yc - c * clock->pointer_width / 2;
    points[3].x = xc - c * clock->pointer_width / 2;
    points[3].y = yc + s * clock->pointer_width / 2;
    points[4].x = xc + s * clock->pointer_width / 2;
    points[4].y = yc + c * clock->pointer_width / 2;


    if(clock->relief)
        gtk_draw_polygon(widget->style, widget->window, GTK_STATE_NORMAL,
                         GTK_SHADOW_OUT, points, 5, TRUE);
    else
        gdk_draw_polygon(widget->window,
                         widget->style->text_gc[GTK_STATE_NORMAL], TRUE, points,
                         5);
}


static void xfce_clock_draw_digital(GtkWidget * widget)
{
    XfceClock *clock;
    time_t ticks;
    struct tm *tm;
    gint h, m, s;
    gint x, y;
    gchar ampm[3] = "AM";
    gchar time_buf[24];
    gint width, height;         /* to measure out string. */
    PangoLayout *layout;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;

    if(h >= 12)
        ampm[0] = 'P';

    if(!(clock->military_time))
    {
        if(h > 12)
            h -= 12;
        if(h == 0)
            h = 12;
    }

    if(clock->military_time)
    {
        if(clock->display_secs)
            sprintf(time_buf, "%d:%02d:%02d", h, m, s);
        else
            sprintf(time_buf, "%d:%02d", h, m);
    }
    else
    {
        if(clock->display_am_pm)
        {
            if(clock->display_secs)
                sprintf(time_buf, "%d:%02d:%02d %s", h, m, s, ampm);
            else
                sprintf(time_buf, "%d:%02d %s", h, m, ampm);
        }
        else
        {
            if(clock->display_secs)
                sprintf(time_buf, "%d:%02d:%02d", h, m, s);
            else
                sprintf(time_buf, "%d:%02d", h, m);
        }
    }

    gtk_draw_box(widget->style, widget->window, widget->state,
                 clock->shadow_type, 0, 0, widget->allocation.width,
                 widget->allocation.height);

    layout = gtk_widget_create_pango_layout(widget, time_buf);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    pango_layout_get_pixel_size(layout, &width, &height);
    x = (widget->allocation.width - width) / 2;
    y = (widget->allocation.height - height) / 2;

    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_STATE_NORMAL], x,
                    y, layout);
}

static void xfce_clock_draw_analog(GtkWidget * widget)
{
    XfceClock *clock;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    draw_round_frame(widget);
    draw_ticks(widget);
    draw_hrs_pointer(widget);
    draw_min_pointer(widget);
    if(clock->display_secs)
        draw_sec_pointer(widget);
}

static void xfce_clock_draw_leds(GtkWidget * widget)
{
    XfceClock *clock;
    time_t ticks;
    struct tm *tm;
    gint h, m, s;
    gint x, y;
    guint c_width = 0;
    guint c_height = 0;
    guint len, i;
    gchar ampm[2] = "a";
    gchar separator[2] = ":";
    gchar time_buf[12];

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;

    if(h >= 12)
        ampm[0] = 'p';

    if(s & 1)
        separator[0] = ':';
    else
        separator[0] = ' ';

    if(!(clock->military_time))
    {
        if(h > 12)
            h -= 12;
        if(h == 0)
            h = 12;
    }

    if(clock->military_time)
    {
        if(clock->display_secs)
            sprintf(time_buf, "%02d%s%02d%s%02d", h, separator, m, separator,
                    s);
        else
            sprintf(time_buf, "%02d%s%02d", h, separator, m);
    }
    else
    {
        if(clock->display_am_pm)
        {
	    if(clock->leds_size != DIGIT_TINY && clock->leds_size != DIGIT_SMALL)
	    {
            if(clock->display_secs)
                sprintf(time_buf, "%02d%s%02d%s%02d%s", h, separator, m,
                        separator, s, ampm);
            else
                sprintf(time_buf, "%02d%s%02d%s", h, separator, m, ampm);
	    }
	    else
		sprintf(time_buf, "%02d%s%02d%s", h,separator,m,ampm);
        }
        else
        {
            if(clock->display_secs)
                sprintf(time_buf, "%02d%s%02d%s%02d", h, separator, m,
                        separator, s);
            else
                sprintf(time_buf, "%02d%s%02d", h, separator, m);
        }
    }
    if(time_buf[0] == '0')
        time_buf[0] = ' ';

    len = strlen(time_buf);

    if(((len * digits_huge_width) <= widget->allocation.width) &&
       (digits_huge_height <= widget->allocation.height))
    {
        clock->leds_size = DIGIT_HUGE;
        c_width = digits_huge_width;
        c_height = digits_huge_height;
    }
    else if(((len * digits_large_width) <= widget->allocation.width) &&
            (digits_large_height <= widget->allocation.height))
    {
        clock->leds_size = DIGIT_LARGE;
        c_width = digits_large_width;
        c_height = digits_large_height;
    }
    else if(((len * digits_medium_width) <= widget->allocation.width) &&
            (digits_medium_height <= widget->allocation.height))
    {
        clock->leds_size = DIGIT_MEDIUM;
        c_width = digits_medium_width;
        c_height = digits_medium_height;
    }
    else  if(((len * digits_small_width) <= widget->allocation.width) &&
       (digits_small_height <= widget->allocation.height))
    {
        clock->leds_size = DIGIT_SMALL;
        c_width = digits_small_width;
        c_height = digits_small_height;
    }
    else
    {
	clock->leds_size = DIGIT_TINY;
	c_width = digits_tiny_height;
	c_height = digits_tiny_height;
    }
    /* Center in the widget */
    x = (widget->allocation.width - (c_width * len)) / 2;

    y = (widget->allocation.height - c_height) / 2;

    gtk_draw_box(widget->style, widget->window, widget->state,
                 clock->shadow_type, 0, 0, widget->allocation.width,
                 widget->allocation.height);
    for(i = 0; i < len; i++)
    {
        draw_digits(clock, widget->style->dark_gc[GTK_STATE_NORMAL],
                    x + i * c_width + 1, y + 1, time_buf[i]);
        draw_digits(clock, widget->style->text_gc[GTK_STATE_NORMAL],
                    x + i * c_width, y, time_buf[i]);
    }
}

static void xfce_clock_draw_internal(GtkWidget * widget)
{
    XfceClock *clock;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    if(GTK_WIDGET_DRAWABLE(widget))
    {
        switch (clock->mode)
        {
            case XFCE_CLOCK_ANALOG:
                xfce_clock_draw_analog(widget);
                break;
            case XFCE_CLOCK_LEDS:
                xfce_clock_draw_leds(widget);
                break;
            case XFCE_CLOCK_DIGITAL:
            default:
                xfce_clock_draw_digital(widget);
                break;
        }
    }
}

static gint xfce_clock_expose(GtkWidget * widget, GdkEventExpose * event)
{
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);
    g_return_val_if_fail(GTK_WIDGET_DRAWABLE(widget), FALSE);
    g_return_val_if_fail(!GTK_WIDGET_NO_WINDOW(widget), FALSE);

    if(event->count > 0)
        return FALSE;

    xfce_clock_draw(widget, NULL);

    return FALSE;
}

static void xfce_clock_draw(GtkWidget * widget, GdkRectangle * area)
{
    XfceClock *clock;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(GTK_WIDGET_DRAWABLE(widget));
    g_return_if_fail(!GTK_WIDGET_NO_WINDOW(widget));
    g_return_if_fail(GTK_IS_WIDGET(widget->parent));
    g_return_if_fail(GTK_WIDGET_DRAWABLE(widget->parent));

    clock = XFCE_CLOCK(widget);
    if(clock->parent_style != widget->parent->style)
    {
        if(clock->parent_style)
            gtk_style_unref(clock->parent_style);
        clock->parent_style = widget->parent->style;
        gtk_style_ref(clock->parent_style);
    }
    if(clock->mode == XFCE_CLOCK_ANALOG)
        gtk_draw_box(clock->parent_style, widget->window, widget->parent->state,
                     GTK_SHADOW_NONE, 0, 0, widget->allocation.width,
                     widget->allocation.height);
    xfce_clock_draw_internal(widget);
}

static gint xfce_clock_timer(XfceClock * clock)
{
    time_t ticks;
    struct tm *tm;
    static guint oh = 0, om = 0, os = 0;
    gint h, m, s;
    static gboolean sem = FALSE;

    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);

    if(sem)
        return TRUE;
    sem = TRUE;
    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;
    if(!
       (((!((clock->display_secs) || (clock->mode == XFCE_CLOCK_LEDS))) ||
         (s == os)) && (m == om) && (h == oh)))
    {
        os = s;
        om = m;
        oh = h;
        clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
        clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
        clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;
        xfce_clock_draw_internal(GTK_WIDGET(clock));
    }

    sem = FALSE;
    return TRUE;
}

void xfce_clock_set_relief(XfceClock * clock, gboolean relief)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    clock->relief = relief;

    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw_internal(GTK_WIDGET(clock));
}

void xfce_clock_set_shadow_type(XfceClock * clock, GtkShadowType type)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    clock->shadow_type = type;

    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw_internal(GTK_WIDGET(clock));
}


static void draw_digits(XfceClock * clock, GdkGC * gc, gint x, gint y, gchar c)
{
    gint tsx, tsy, tsw, tsh;
    guint tc = 0;

    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if((c >= '0') && (c <= '9'))
        tc = c - '0';
    else if((c == 'A') || (c == 'a'))
        tc = 10;
    else if((c == 'P') || (c == 'p'))
        tc = 11;
    else if(c == ':')
        tc = 12;
    else
        return;

    switch (clock->leds_size)
    {
        case DIGIT_HUGE:
            tsx = tc * digits_huge_width;
            tsy =
                digits_small_height + digits_medium_height +
                digits_large_height;
            tsw = digits_huge_width;
            tsh = digits_huge_height;
            break;
        case DIGIT_LARGE:
            tsx = tc * digits_large_width;
            tsy =  digits_small_height + digits_medium_height;
            tsw = digits_large_width;
            tsh = digits_large_height;
            break;
        case DIGIT_MEDIUM:
            tsx = tc * digits_medium_width;
            tsy = digits_small_height;
            tsw = digits_medium_width;
            tsh = digits_medium_height;
            break;
        case DIGIT_SMALL:
            tsx = tc * digits_small_width;
            tsy = 0;
            tsw = digits_small_width;
            tsh = digits_small_height;
            break;
    case DIGIT_TINY:
	tsx = tc * digits_tiny_width;
	tsy = 0;
	tsw = digits_tiny_width;
	tsh = digits_tiny_height;
	break;
    }

    gdk_gc_set_stipple(gc, clock->digits_bmap);
    gdk_gc_set_fill(gc, GDK_STIPPLED);
    gdk_gc_set_ts_origin(gc, digits_width - tsx + x, digits_height - tsy + y);

    gdk_draw_rectangle(GTK_WIDGET(clock)->window, gc, TRUE, x, y, tsw, tsh);

    gdk_gc_set_fill(gc, GDK_SOLID);

}
