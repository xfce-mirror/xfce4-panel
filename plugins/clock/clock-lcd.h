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

#ifndef __CLOCK_LCD_H__
#define __CLOCK_LCD_H__

#include "clock-time.h"

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_CLOCK_TYPE_LCD (xfce_clock_lcd_get_type ())
G_DECLARE_FINAL_TYPE (XfceClockLcd, xfce_clock_lcd, XFCE_CLOCK, LCD, GtkImage)

void       xfce_clock_lcd_register_type (XfcePanelTypeModule *type_module);

GtkWidget *xfce_clock_lcd_new           (ClockTime           *time,
                                         ClockSleepMonitor   *sleep_monitor) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__CLOCK_LCD_H__ */
