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
#include "clock-fuzzy.h"



static void xfce_clock_fuzzy_class_init   (XfceClockFuzzyClass *klass);
static void xfce_clock_fuzzy_init         (XfceClockFuzzy      *clock);
static void xfce_clock_fuzzy_set_property (GObject               *object,
                                             guint                  prop_id,
                                             const GValue          *value,
                                             GParamSpec            *pspec);
static void xfce_clock_fuzzy_get_property (GObject               *object,
                                             guint                  prop_id,
                                             GValue                *value,
                                             GParamSpec            *pspec);


enum
{
  FUZZINESS_5_MINS = 0,
  FUZZINESS_15_MINS,
  FUZZINESS_DAY,
  FUZZINESS_WEEK,

  FUZZINESS_MIN = FUZZINESS_5_MINS,
  FUZZINESS_MAX = FUZZINESS_WEEK,
  FUZZINESS_DEFAULT = FUZZINESS_15_MINS
};

enum
{
 PROP_0,
 PROP_FUZZINESS
};

struct _XfceClockFuzzyClass
{
 GtkLabelClass __parent__;
};

struct _XfceClockFuzzy
{
  GtkLabel __parent__;

  /* fuzziness */
  guint fuzziness;
};



G_DEFINE_TYPE (XfceClockFuzzy, xfce_clock_fuzzy, GTK_TYPE_LABEL);



static void
xfce_clock_fuzzy_class_init (XfceClockFuzzyClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = xfce_clock_fuzzy_set_property;
  gobject_class->get_property = xfce_clock_fuzzy_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_FUZZINESS,
                                   g_param_spec_uint ("fuzziness", NULL, NULL,
                                                      FUZZINESS_MIN, FUZZINESS_MAX,
                                                      FUZZINESS_DEFAULT,
                                                      G_PARAM_READWRITE
                                                      | G_PARAM_STATIC_BLURB));
}



static void
xfce_clock_fuzzy_init (XfceClockFuzzy *clock)
{
  /* init */
  clock->fuzziness = FUZZINESS_DEFAULT;

  /* center text */
  gtk_label_set_justify (GTK_LABEL (clock), GTK_JUSTIFY_CENTER);
}



static void
xfce_clock_fuzzy_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  XfceClockFuzzy *clock = XFCE_CLOCK_FUZZY (object);

  switch (prop_id)
    {
      case PROP_FUZZINESS:
        clock->fuzziness = CLAMP (g_value_get_uint (value),
                                  FUZZINESS_MIN, FUZZINESS_MAX);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_clock_fuzzy_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  XfceClockFuzzy *clock = XFCE_CLOCK_FUZZY (object);

  switch (prop_id)
    {
      case PROP_FUZZINESS:
        g_value_set_uint (value, clock->fuzziness);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



GtkWidget *
xfce_clock_fuzzy_new (void)
{
  return g_object_new (XFCE_CLOCK_TYPE_FUZZY, NULL);
}



gboolean
xfce_clock_fuzzy_update (gpointer user_data)
{
  XfceClockFuzzy *clock = XFCE_CLOCK_FUZZY (user_data);
  struct tm       tm;
  const gchar    *strings_day[] = { N_("Night"),   N_("Early morning"),
                                    N_("Morning"), N_("Almost noon"),
                                    N_("Noon"),    N_("Afternoon"),
                                    N_("Evening"), N_("Late evening") };

  panel_return_val_if_fail (XFCE_CLOCK_IS_FUZZY (clock), FALSE);

  /* get the local time */
  clock_plugin_get_localtime (&tm);

  switch (clock->fuzziness)
    {
      case FUZZINESS_5_MINS:
      case FUZZINESS_15_MINS:
      case FUZZINESS_DAY:
      case FUZZINESS_WEEK:
        gtk_label_set_text (GTK_LABEL (clock), _(strings_day[tm.tm_hour / 3]));
        break;
    };

  return TRUE;
}



guint
xfce_clock_fuzzy_interval (XfceClockFuzzy *clock)
{
  panel_return_val_if_fail (XFCE_CLOCK_IS_FUZZY (clock), CLOCK_INTERVAL_MINUTE);

  return CLOCK_INTERVAL_MINUTE;
}
