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

/* Port to gtk2 by Jasper Huijsmans, original code by Olivier Fourdan */

#ifndef __XFCE_CLOCK_H__
#define __XFCE_CLOCK_H__


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>


#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#define XFCE_CLOCK(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, xfce_clock_get_type (), XfceClock)
#define XFCE_CLOCK_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, xfce_clock_get_type (), XfceClockClass)
#define XFCE_IS_CLOCK(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, xfce_clock_get_type ())

#define UPDATE_DELAY_LENGTH        30000        /* Update clock every 30 secs */

    typedef struct _XfceClock XfceClock;
    typedef struct _XfceClockClass XfceClockClass;
    typedef enum
    {
        XFCE_CLOCK_ANALOG,
        XFCE_CLOCK_DIGITAL,
        XFCE_CLOCK_LEDS
    }
    XfceClockMode;

    typedef enum
    {
	DIGIT_TINY,
        DIGIT_SMALL,
        DIGIT_MEDIUM,
        DIGIT_LARGE,
        DIGIT_HUGE
    }
    Digit_Size;

    struct _XfceClock
    {

        GtkWidget widget;       /* parent */

        GtkStyle *parent_style;
        GdkBitmap *digits_bmap;

        /* Dimensions of clock components */
        gint radius;
        gint internal;
        gint pointer_width;

        gboolean relief;
        GtkShadowType shadow_type;

        /* ID of update timer, or 0 if none */
        guint32 timer;

        gfloat hrs_angle;
        gfloat min_angle;
        gfloat sec_angle;

        gint interval;          /* Number of seconds between updates. */
        XfceClockMode mode;
        gboolean military_time; /* true=24 hour clock, false = 12 hour clock. */
        gboolean display_am_pm;
        gboolean display_secs;
        Digit_Size leds_size;
    };

    struct _XfceClockClass
    {
        GtkWidgetClass parent_class;
    };


    GtkWidget *xfce_clock_new(void);
    GtkType xfce_clock_get_type(void);
    void xfce_clock_set_relief(XfceClock * clock, gboolean relief);
    void xfce_clock_set_shadow_type(XfceClock * clock, GtkShadowType type);
    void xfce_clock_show_ampm(XfceClock * clock, gboolean show);
    void xfce_clock_ampm_toggle(XfceClock * clock);
    gboolean xfce_clock_ampm_shown(XfceClock * clock);
    void xfce_clock_show_secs(XfceClock * clock, gboolean show);
    void xfce_clock_secs_toggle(XfceClock * clock);
    gboolean xfce_clock_secs_shown(XfceClock * clock);
    void xfce_clock_show_military(XfceClock * clock, gboolean show);
    void xfce_clock_military_toggle(XfceClock * clock);
    gboolean xfce_clock_military_shown(XfceClock * clock);
    void xfce_clock_set_interval(XfceClock * clock, guint interval);
    guint xfce_clock_get_interval(XfceClock * clock);
    void xfce_clock_suspend(XfceClock * clock);
    void xfce_clock_resume(XfceClock * clock);
    void xfce_clock_set_mode(XfceClock * clock, XfceClockMode mode);
    void xfce_clock_toggle_mode(XfceClock * clock);
    XfceClockMode xfce_clock_get_mode(XfceClock * clock);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */


#endif                          /* __XFCE_CLOCK_H__ */
/* example-end */
