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

  FUZZINESS_MIN = FUZZINESS_5_MINS,
  FUZZINESS_MAX = FUZZINESS_DAY,
  FUZZINESS_DEFAULT = FUZZINESS_5_MINS
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

const gchar *i18n_day_sectors[] =
{
  N_("Night"),   N_("Early morning"),
  N_("Morning"), N_("Almost noon"),
  N_("Noon"),    N_("Afternoon"),
  N_("Evening"), N_("Late evening")
};

const gchar *i18n_hour_sectors[] =
{
  N_("five past %s"),        N_("ten past %s"),
  N_("quarter past %s"),     N_("twenty past %s"),
  N_("twenty five past %s"), N_("half past %s"),
  N_("twenty five to %s"),   N_("twenty to %s"),
  N_("quarter to %s"),       N_("ten to %s"),
  N_("five to %s"),          N_("%s o'clock")
};

const gchar *i18n_hour_names[] =
{
  N_("one"),    N_("two"),
  N_("three"),  N_("four"),
  N_("five"),   N_("six"),
  N_("seven"),  N_("eight"),
  N_("nine"),   N_("ten"),
  N_("eleven"), N_("twelve")
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
  gint            hour_sector;
  gint            minutes, hour;
  gchar          *string;

  panel_return_val_if_fail (XFCE_CLOCK_IS_FUZZY (clock), FALSE);

  /* get the local time */
  clock_plugin_get_localtime (&tm);

  if (clock->fuzziness == FUZZINESS_5_MINS
      || clock->fuzziness == FUZZINESS_15_MINS)
    {
      /* set the time */
      minutes = tm.tm_min;
      hour = tm.tm_hour > 12 ? tm.tm_hour - 12 : tm.tm_hour;

      /* get the hour sector */
      if (clock->fuzziness == FUZZINESS_5_MINS)
        hour_sector = minutes >= 3 ? (minutes - 3) / 5 : 11;
      else
        hour_sector = minutes > 7 ? ((minutes - 7) / 15 + 1) * 3 - 1 : 11;

      /* advance 1 hour, if we're half past */
      if (hour_sector <= 6)
        hour--;

      /* set the time string */
      string = g_strdup_printf (_(i18n_hour_sectors[hour_sector]), _(i18n_hour_names[hour]));
      gtk_label_set_text (GTK_LABEL (clock), string);
      g_free (string);
    }
  else /* FUZZINESS_DAY */
    {
      gtk_label_set_text (GTK_LABEL (clock), _(i18n_day_sectors[tm.tm_hour / 3]));
    }

  return TRUE;
}



guint
xfce_clock_fuzzy_interval (XfceClockFuzzy *clock)
{
  panel_return_val_if_fail (XFCE_CLOCK_IS_FUZZY (clock), CLOCK_INTERVAL_MINUTE);

  return CLOCK_INTERVAL_MINUTE;
}
