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

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <gtk/gtk.h>

#include "clock.h"
#include "clock-digital.h"



/* class functions */
static void xfce_clock_digital_class_init   (XfceClockDigitalClass *klass);
static void xfce_clock_digital_init         (XfceClockDigital      *clock);
static void xfce_clock_digital_finalize     (GObject               *object);
static void xfce_clock_digital_set_property (GObject               *object,
                                             guint                  prop_id,
                                             const GValue          *value,
                                             GParamSpec            *pspec);
static void xfce_clock_digital_get_property (GObject               *object,
                                             guint                  prop_id,
                                             GValue                *value,
                                             GParamSpec            *pspec);



enum
{
    PROP_0,
    PROP_DIGITAL_FORMAT
};

struct _XfceClockDigitalClass
{
    GtkLabelClass __parent__;
};

struct _XfceClockDigital
{
    GtkLabel  __parent__;

    /* current clock format */
    gchar    *format;
};



G_DEFINE_TYPE (XfceClockDigital, xfce_clock_digital, GTK_TYPE_LABEL);



static void
xfce_clock_digital_class_init (XfceClockDigitalClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = xfce_clock_digital_finalize;
    gobject_class->set_property = xfce_clock_digital_set_property;
    gobject_class->get_property = xfce_clock_digital_get_property;

    /**
     * Digital clock format
     **/
    g_object_class_install_property (gobject_class,
                                     PROP_DIGITAL_FORMAT,
                                     g_param_spec_string ("digital-format", "digital-format", "digital-format",
                                                          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_BLURB));
}



static void
xfce_clock_digital_init (XfceClockDigital *clock)
{
    /* init */
    clock->format = NULL;

    /* center text */
    gtk_label_set_justify (GTK_LABEL (clock), GTK_JUSTIFY_CENTER);
}



static void
xfce_clock_digital_finalize (GObject *object)
{
    XfceClockDigital *clock = XFCE_CLOCK_DIGITAL (object);

    /* cleanup */
    g_free (clock->format);

    (*G_OBJECT_CLASS (xfce_clock_digital_parent_class)->finalize) (object);
}



static void
xfce_clock_digital_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    XfceClockDigital *clock = XFCE_CLOCK_DIGITAL (object);

    switch (prop_id)
    {
        case PROP_DIGITAL_FORMAT:
            /* cleanup */
            g_free (clock->format);

            /* set new format */
            clock->format = g_value_dup_string (value);

            /* update the widget */
            xfce_clock_digital_update (clock);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



static void
xfce_clock_digital_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    XfceClockDigital *clock = XFCE_CLOCK_DIGITAL (object);

    switch (prop_id)
    {
        case PROP_DIGITAL_FORMAT:
            g_value_set_string (value,clock->format);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}



GtkWidget *
xfce_clock_digital_new (void)
{
    return g_object_new (XFCE_CLOCK_TYPE_DIGITAL, NULL);
}



gboolean
xfce_clock_digital_update (gpointer user_data)
{
    XfceClockDigital *clock = XFCE_CLOCK_DIGITAL (user_data);
    gchar            *string;
    struct tm         tm;

    g_return_val_if_fail (XFCE_CLOCK_IS_DIGITAL (clock), FALSE);

    /* get the local time */
    xfce_clock_util_get_localtime (&tm);

    /* get the string */
    string = xfce_clock_util_strdup_strftime (clock->format ? clock->format : DEFAULT_DIGITAL_FORMAT, &tm);

    /* set the new label */
    gtk_label_set_markup (GTK_LABEL (clock), string);

    /* cleanup */
    g_free (string);

    return TRUE;
}
