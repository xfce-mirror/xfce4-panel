/* $Id$ */
/*
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
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

#ifndef __CLOCK_LCD_H__
#define __CLOCK_LCD_H__

G_BEGIN_DECLS

typedef struct _XfceClockLcdClass XfceClockLcdClass;
typedef struct _XfceClockLcd      XfceClockLcd;

#define XFCE_CLOCK_TYPE_LCD            (xfce_clock_lcd_get_type ())
#define XFCE_CLOCK_LCD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_CLOCK_TYPE_LCD, XfceClockLcd))
#define XFCE_CLOCK_LCD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_LCD, XfceClockLcdClass))
#define XFCE_CLOCK_IS_LCD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_CLOCK_TYPE_LCD))
#define XFCE_CLOCK_IS_LCD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_LCD))
#define XFCE_CLOCK_LCD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_CLOCK_TYPE_LCD, XfceClockLcdClass))

GType      xfce_clock_lcd_get_type      (void) G_GNUC_CONST;

void       xfce_clock_lcd_register_type (GTypeModule  *type_module);

GtkWidget *xfce_clock_lcd_new           (void) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__CLOCK_LCD_H__ */
