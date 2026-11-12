/*
 * Copyright (C) 2026
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <glib/gi18n-lib.h>
#include "clock-jalali-calendar.h"

#include "common/panel-private.h"

/*
 * The calendar conversion routines are adapted from jalaali-js, which
 * implements the Borkowski algorithm.
 * Copyright (c) 2020 Behrang Norouzinia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

typedef struct
{
  GtkWidget *grid;
  GtkWidget *heading;
  gint year;
  gint month;
  gint day;
  gint today_year;
  gint today_month;
  gint today_day;
  gint selected_year;
  gint selected_month;
  gint selected_day;
  gboolean show_week_numbers;
} ClockJalaliCalendar;

static void clock_jalali_calendar_previous (GtkButton *button, gpointer user_data);
static void clock_jalali_calendar_next (GtkButton *button, gpointer user_data);
static void clock_jalali_calendar_day_clicked (GtkButton *button, gpointer user_data);

static const gchar *month_names[] = {
  N_ ("Farvardin"), N_ ("Ordibehesht"), N_ ("Khordad"),
  N_ ("Tir"), N_ ("Mordad"), N_ ("Shahrivar"),
  N_ ("Mehr"), N_ ("Aban"), N_ ("Azar"),
  N_ ("Dey"), N_ ("Bahman"), N_ ("Esfand")
};

static const gchar *weekday_names[] = {
  N_ ("Saturday"), N_ ("Sunday"), N_ ("Monday"), N_ ("Tuesday"),
  N_ ("Wednesday"), N_ ("Thursday"), N_ ("Friday")
};

static const gchar *month_names_fa[] = {
  "فروردین", "اردیبهشت", "خرداد", "تیر", "مرداد", "شهریور",
  "مهر", "آبان", "آذر", "دی", "بهمن", "اسفند"
};

static const gchar *weekday_names_fa[] = {
  "شنبه", "یکشنبه", "دوشنبه", "سه‌شنبه", "چهارشنبه", "پنجشنبه", "جمعه"
};

static gboolean
calendar_uses_persian_labels (void)
{
  const gchar * const *languages = g_get_language_names ();
  return languages[0] != NULL && g_str_has_prefix (languages[0], "fa");
}

static const gint jalali_breaks[] = {
  -61, 9, 38, 199, 426, 686, 756, 818, 1111, 1181,
  1210, 1635, 2060, 2097, 2192, 2262, 2324, 2394, 2456, 3178
};

static gint
gregorian_to_jdn (gint year,
                  gint month,
                  gint day)
{
  gint d = ((year + (month - 8) / 6 + 100100) * 1461) / 4
           + (153 * ((month + 9) % 12) + 2) / 5 + day - 34840408;
  d = d - (((year + 100100 + (month - 8) / 6) / 100) * 3) / 4 + 752;
  return d;
}

static void
jdn_to_gregorian (gint jdn,
                  gint *year,
                  gint *month,
                  gint *day)
{
  gint j = 4 * jdn + 139361631;
  j = j + (((4 * jdn + 183187720) / 146097) * 3 / 4) * 4 - 3908;
  gint i = (j % 1461) / 4 * 5 + 308;
  *day = (i % 153) / 5 + 1;
  *month = ((i / 153) % 12) + 1;
  *year = j / 1461 - 100100 + (8 - *month) / 6;
}

static void
jalali_calendar_calculate (gint year,
                           gint *gregorian_year,
                           gint *march_day,
                           gint *leap)
{
  gint leap_j = -14;
  gint jp = jalali_breaks[0];
  gint jm = 0;
  gint jump = 0;
  gint n, leap_g, adjusted;
  guint i;

  for (i = 1; i < G_N_ELEMENTS (jalali_breaks); i++)
    {
      jm = jalali_breaks[i];
      jump = jm - jp;
      if (year < jm)
        break;
      leap_j += (jump / 33) * 8 + (jump % 33) / 4;
      jp = jm;
    }

  n = year - jp;
  leap_j += (n / 33) * 8 + ((n % 33) + 3) / 4;
  if (jump % 33 == 4 && jump - n == 4)
    leap_j++;

  *gregorian_year = year + 621;
  leap_g = *gregorian_year / 4 - (((*gregorian_year / 100) + 1) * 3) / 4 - 150;
  *march_day = 20 + leap_j - leap_g;

  adjusted = n;
  if (jump - n < 6)
    adjusted = n - jump + ((jump + 4) / 33) * 33;
  *leap = (((adjusted + 1) % 33) - 1) % 4;
  if (*leap == -1)
    *leap = 4;
}

static gint
jalali_to_jdn (gint year,
               gint month,
               gint day)
{
  gint gregorian_year, march_day, leap;
  jalali_calendar_calculate (year, &gregorian_year, &march_day, &leap);
  (void) leap;
  return gregorian_to_jdn (gregorian_year, 3, march_day)
         + (month - 1) * 31 - (month / 7) * (month - 7) + day - 1;
}

static void
gregorian_to_jalali (gint gy,
                     gint gm,
                     gint gd,
                     gint *jy,
                     gint *jm,
                     gint *jd)
{
  gint gregorian_year, jalali_year, march_day, leap, jdn, first_day, offset;

  jdn = gregorian_to_jdn (gy, gm, gd);
  jalali_year = gy - 621;
  jalali_calendar_calculate (jalali_year, &gregorian_year, &march_day, &leap);
  first_day = gregorian_to_jdn (gregorian_year, 3, march_day);
  offset = jdn - first_day;
  if (offset >= 0)
    {
      if (offset <= 185)
        {
          *jy = jalali_year;
          *jm = 1 + offset / 31;
          *jd = 1 + offset % 31;
          return;
        }
      offset -= 186;
    }
  else
    {
      jalali_year--;
      offset += 179;
      if (leap == 1)
        offset++;
    }

  *jy = jalali_year;
  *jm = 7 + offset / 30;
  *jd = 1 + offset % 30;
}

static gchar *
calendar_number (gint number)
{
  gchar *ascii = g_strdup_printf ("%d", number);
  const gchar * const *languages = g_get_language_names ();
  GString *localized;
  const gchar *p;

  if (languages[0] == NULL || !calendar_uses_persian_labels ())
    return ascii;

  localized = g_string_new (NULL);
  for (p = ascii; *p != '\0'; p++)
    {
      if (*p >= '0' && *p <= '9')
        g_string_append_unichar (localized, 0x06f0 + (*p - '0'));
      else
        g_string_append_c (localized, *p);
    }
  g_free (ascii);
  return g_string_free (localized, FALSE);
}

static gint
month_length (gint year,
              gint month)
{
  if (month <= 6)
    return 31;
  if (month <= 11)
    return 30;
  {
    gint gy, march_day, leap;
    jalali_calendar_calculate (year, &gy, &march_day, &leap);
    (void) gy;
    (void) march_day;
    return leap == 0 ? 30 : 29;
  }
}

static gint
weekday_for_jalali_date (gint year,
                         gint month,
                         gint day)
{
  gint gy, gm, gd;
  GDateTime *date;
  gint weekday;

  jdn_to_gregorian (jalali_to_jdn (year, month, day), &gy, &gm, &gd);
  date = g_date_time_new_utc (gy, gm, gd, 0, 0, 0);
  weekday = date != NULL ? (g_date_time_get_day_of_week (date) + 1) % 7 : 0;
  if (date != NULL)
    g_date_time_unref (date);
  return weekday;
}

static void
calendar_clear (GtkWidget *grid)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (grid));
  GList *iter;
  for (iter = children; iter != NULL; iter = iter->next)
    gtk_widget_destroy (GTK_WIDGET (iter->data));
  g_list_free (children);
}

static void
calendar_render (ClockJalaliCalendar *calendar)
{
  gint first_weekday, days, day, row, col, rows, week_column;
  gint first_day_of_year, new_year_weekday;
  gchar *number, *title;
  GtkWidget *button, *label;
  gboolean rtl = gtk_widget_get_direction (calendar->grid) == GTK_TEXT_DIR_RTL;

  calendar_clear (calendar->grid);

  button = gtk_button_new_with_label (rtl ? "›" : "‹");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (button, _("Previous month"));
  g_object_set_data (G_OBJECT (button), "calendar-data", calendar);
  g_signal_connect (button, "clicked", G_CALLBACK (clock_jalali_calendar_previous), NULL);
  gtk_grid_attach (GTK_GRID (calendar->grid), button, 0, 0, 1, 1);

  number = calendar_number (calendar->year);
  title = g_strdup_printf ("%s %s",
                           calendar_uses_persian_labels ()
                             ? month_names_fa[calendar->month - 1]
                             : _(month_names[calendar->month - 1]),
                           number);
  g_free (number);
  calendar->heading = gtk_label_new (title);
  g_free (title);
  gtk_widget_set_hexpand (calendar->heading, TRUE);
  week_column = calendar->show_week_numbers ? 1 : 0;
  gtk_grid_attach (GTK_GRID (calendar->grid), calendar->heading, 1, 0,
                   calendar->show_week_numbers ? 6 : 5, 1);

  button = gtk_button_new_with_label (rtl ? "‹" : "›");
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (button, _("Next month"));
  g_object_set_data (G_OBJECT (button), "calendar-data", calendar);
  g_signal_connect (button, "clicked", G_CALLBACK (clock_jalali_calendar_next), NULL);
  gtk_grid_attach (GTK_GRID (calendar->grid), button,
                   calendar->show_week_numbers ? 7 : 6, 0, 1, 1);

  if (calendar->show_week_numbers)
    gtk_grid_attach (GTK_GRID (calendar->grid), gtk_label_new (""), 0, 1, 1, 1);

  for (col = 0; col < 7; col++)
    {
      label = gtk_label_new (calendar_uses_persian_labels ()
                               ? weekday_names_fa[col]
                               : _(weekday_names[col]));
      gtk_widget_set_margin_top (label, 4);
      gtk_widget_set_margin_bottom (label, 4);
      gtk_grid_attach (GTK_GRID (calendar->grid), label, week_column + col, 1, 1, 1);
    }

  first_weekday = weekday_for_jalali_date (calendar->year, calendar->month, 1);
  days = month_length (calendar->year, calendar->month);
  rows = (first_weekday + days + 6) / 7;
  new_year_weekday = weekday_for_jalali_date (calendar->year, 1, 1);
  first_day_of_year = calendar->month <= 6
                      ? (calendar->month - 1) * 31 + 1
                      : 186 + (calendar->month - 7) * 30 + 1;

  if (calendar->show_week_numbers)
    for (row = 0; row < rows; row++)
      {
        gint first_day = first_day_of_year + row * 7 - first_weekday;
        gint week = (MAX (1, first_day) - 1 + new_year_weekday) / 7 + 1;
        number = calendar_number (week);
        label = gtk_label_new (number);
        g_free (number);
        gtk_grid_attach (GTK_GRID (calendar->grid), label, 0, 2 + row, 1, 1);
      }

  for (day = 1; day <= days; day++)
    {
      gint cell = first_weekday + day - 1;
      row = 2 + cell / 7;
      col = cell % 7;
      number = calendar_number (day);
      button = gtk_toggle_button_new_with_label (number);
      g_free (number);
      gtk_widget_set_can_focus (button, FALSE);
      gtk_widget_set_size_request (button, 34, 30);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    calendar->year == calendar->selected_year
                                    && calendar->month == calendar->selected_month
                                    && day == calendar->selected_day);
      if (calendar->year == calendar->today_year
          && calendar->month == calendar->today_month
          && day == calendar->today_day)
        gtk_style_context_add_class (gtk_widget_get_style_context (button), "suggested-action");
      g_object_set_data (G_OBJECT (button), "calendar-data", calendar);
      g_object_set_data (G_OBJECT (button), "calendar-day", GINT_TO_POINTER (day));
      g_signal_connect (button, "clicked",
                        G_CALLBACK (clock_jalali_calendar_day_clicked), NULL);
      gtk_grid_attach (GTK_GRID (calendar->grid), button, week_column + col, row, 1, 1);
    }
  gtk_widget_show_all (calendar->grid);
}

static void
calendar_change_month (GtkWidget *button,
                       gint delta)
{
  ClockJalaliCalendar *calendar = g_object_get_data (G_OBJECT (button), "calendar-data");
  if (calendar == NULL)
    return;

  calendar->month += delta;
  if (calendar->month < 1)
    {
      calendar->month = 12;
      calendar->year--;
    }
  else if (calendar->month > 12)
    {
      calendar->month = 1;
      calendar->year++;
    }
  calendar_render (calendar);
}

static void
clock_jalali_calendar_previous (GtkButton *button,
                                gpointer user_data)
{
  calendar_change_month (GTK_WIDGET (button), -1);
}

static void
clock_jalali_calendar_next (GtkButton *button,
                            gpointer user_data)
{
  calendar_change_month (GTK_WIDGET (button), 1);
}

static void
clock_jalali_calendar_day_clicked (GtkButton *button,
                                  gpointer user_data)
{
  ClockJalaliCalendar *calendar = g_object_get_data (G_OBJECT (button), "calendar-data");
  if (calendar == NULL)
    return;
  calendar->selected_year = calendar->year;
  calendar->selected_month = calendar->month;
  calendar->selected_day = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "calendar-day"));
  calendar_render (calendar);
}

static ClockJalaliCalendar *
calendar_data (GtkWidget *widget)
{
  return g_object_get_data (G_OBJECT (widget), "clock-jalali-calendar");
}

GtkWidget *
clock_jalali_calendar_new (void)
{
  ClockJalaliCalendar *calendar = g_new0 (ClockJalaliCalendar, 1);
  GDateTime *today = g_date_time_new_now_local ();

  calendar->grid = gtk_grid_new ();
  if (calendar_uses_persian_labels ())
    gtk_widget_set_direction (calendar->grid, GTK_TEXT_DIR_RTL);
  gtk_grid_set_row_spacing (GTK_GRID (calendar->grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (calendar->grid), 2);
  gtk_container_set_border_width (GTK_CONTAINER (calendar->grid), 8);
  g_object_set_data_full (G_OBJECT (calendar->grid), "clock-jalali-calendar",
                          calendar, g_free);
  calendar->show_week_numbers = TRUE;

  gregorian_to_jalali (g_date_time_get_year (today), g_date_time_get_month (today),
                       g_date_time_get_day_of_month (today),
                       &calendar->year, &calendar->month, &calendar->day);
  calendar->today_year = calendar->year;
  calendar->today_month = calendar->month;
  calendar->today_day = calendar->day;
  calendar->selected_year = calendar->year;
  calendar->selected_month = calendar->month;
  calendar->selected_day = calendar->day;
  calendar_render (calendar);
  g_date_time_unref (today);
  return calendar->grid;
}

void
clock_jalali_calendar_set_date (GtkWidget *widget,
                                GDateTime *date)
{
  ClockJalaliCalendar *calendar = calendar_data (widget);
  if (calendar == NULL || date == NULL)
    return;

  gregorian_to_jalali (g_date_time_get_year (date), g_date_time_get_month (date),
                       g_date_time_get_day_of_month (date),
                       &calendar->year, &calendar->month, &calendar->day);
  calendar->today_year = calendar->year;
  calendar->today_month = calendar->month;
  calendar->today_day = calendar->day;
  calendar->selected_year = calendar->year;
  calendar->selected_month = calendar->month;
  calendar->selected_day = calendar->day;
  calendar_render (calendar);
}

void
clock_jalali_calendar_set_show_week_numbers (GtkWidget *widget,
                                             gboolean show_week_numbers)
{
  ClockJalaliCalendar *calendar = calendar_data (widget);
  if (calendar != NULL)
    {
      calendar->show_week_numbers = show_week_numbers;
      calendar_render (calendar);
    }
}
