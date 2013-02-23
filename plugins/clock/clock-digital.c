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

#include <gtk/gtk.h>

#include "clock.h"
#include "clock-time.h"
#include "clock-digital.h"



static void     xfce_clock_digital_set_property (GObject               *object,
                                                 guint                  prop_id,
                                                 const GValue          *value,
                                                 GParamSpec            *pspec);
static void     xfce_clock_digital_get_property (GObject               *object,
                                                 guint                  prop_id,
                                                 GValue                *value,
                                                 GParamSpec            *pspec);
static void     xfce_clock_digital_finalize     (GObject               *object);
static gboolean xfce_clock_digital_update       (XfceClockDigital      *digital,
                                                 ClockTime             *time);




enum
{
  PROP_0,
  PROP_DIGITAL_FORMAT,
  PROP_SIZE_RATIO,
  PROP_ORIENTATION
};

struct _XfceClockDigitalClass
{
 GtkLabelClass __parent__;
};

struct _XfceClockDigital
{
  GtkLabel __parent__;

  ClockTime          *time;
  ClockTimeTimeout   *timeout;

  gchar *format;
};



XFCE_PANEL_DEFINE_TYPE (XfceClockDigital, xfce_clock_digital, GTK_TYPE_LABEL)



static void
xfce_clock_digital_class_init (XfceClockDigitalClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_clock_digital_finalize;
  gobject_class->set_property = xfce_clock_digital_set_property;
  gobject_class->get_property = xfce_clock_digital_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE_RATIO,
                                   g_param_spec_double ("size-ratio", NULL, NULL,
                                                        -1, G_MAXDOUBLE, 0.0,
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
                                   PROP_DIGITAL_FORMAT,
                                   g_param_spec_string ("digital-format", NULL, NULL,
                                                        DEFAULT_DIGITAL_FORMAT,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS));
}



static void
xfce_clock_digital_init (XfceClockDigital *digital)
{
  digital->format = g_strdup (DEFAULT_DIGITAL_FORMAT);

  gtk_label_set_justify (GTK_LABEL (digital), GTK_JUSTIFY_CENTER);
}



static void
xfce_clock_digital_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  XfceClockDigital *digital = XFCE_CLOCK_DIGITAL (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      gtk_label_set_angle (GTK_LABEL (object),
          g_value_get_enum (value) == GTK_ORIENTATION_HORIZONTAL ?
          0 : 270);
      break;

    case PROP_DIGITAL_FORMAT:
      g_free (digital->format);
      digital->format = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  /* reschedule the timeout and redraw */
  clock_time_timeout_set_interval (digital->timeout,
      clock_time_interval_from_format (digital->format));
  xfce_clock_digital_update (digital, digital->time);
}



static void
xfce_clock_digital_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  XfceClockDigital *digital = XFCE_CLOCK_DIGITAL (object);

  switch (prop_id)
    {
    case PROP_DIGITAL_FORMAT:
      g_value_set_string (value, digital->format);
      break;

    case PROP_SIZE_RATIO:
      g_value_set_double (value, -1.0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_clock_digital_finalize (GObject *object)
{
  XfceClockDigital *digital = XFCE_CLOCK_DIGITAL (object);

  /* stop the timeout */
  clock_time_timeout_free (digital->timeout);

  g_free (digital->format);

  (*G_OBJECT_CLASS (xfce_clock_digital_parent_class)->finalize) (object);
}



static gboolean
xfce_clock_digital_update (XfceClockDigital *digital,
                           ClockTime        *time)
{
  gchar            *string;

  panel_return_val_if_fail (XFCE_CLOCK_IS_DIGITAL (digital), FALSE);
  panel_return_val_if_fail (XFCE_IS_CLOCK_TIME (time), FALSE);

  /* set time string */
  string = clock_time_strdup_strftime (digital->time, digital->format);
  gtk_label_set_markup (GTK_LABEL (digital), string);
  g_free (string);

  return TRUE;
}



GtkWidget *
xfce_clock_digital_new (ClockTime *time)
{
  XfceClockDigital *digital = g_object_new (XFCE_CLOCK_TYPE_DIGITAL, NULL);

  digital->time = time;
  digital->timeout = clock_time_timeout_new (clock_time_interval_from_format (digital->format),
                                             digital->time,
                                             G_CALLBACK (xfce_clock_digital_update), digital);

  return GTK_WIDGET (digital);
}
