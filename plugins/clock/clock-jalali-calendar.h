/*
 * Copyright (C) 2026
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef __CLOCK_JALALI_CALENDAR_H__
#define __CLOCK_JALALI_CALENDAR_H__

#include <gtk/gtk.h>

GtkWidget *
clock_jalali_calendar_new (void);

void
clock_jalali_calendar_set_date (GtkWidget *calendar,
                                GDateTime *date);

void
clock_jalali_calendar_set_show_week_numbers (GtkWidget *calendar,
                                             gboolean show_week_numbers);

#endif /* !__CLOCK_JALALI_CALENDAR_H__ */
