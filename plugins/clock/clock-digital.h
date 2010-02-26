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

#ifndef __CLOCK_DIGITAL_H__
#define __CLOCK_DIGITAL_H__

G_BEGIN_DECLS

typedef struct _XfceClockDigitalClass XfceClockDigitalClass;
typedef struct _XfceClockDigital      XfceClockDigital;

#define XFCE_CLOCK_TYPE_DIGITAL            (xfce_clock_digital_get_type ())
#define XFCE_CLOCK_DIGITAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_CLOCK_TYPE_DIGITAL, XfceClockDigital))
#define XFCE_CLOCK_DIGITAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_DIGITAL, XfceClockDigitalClass))
#define XFCE_CLOCK_IS_DIGITAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_CLOCK_TYPE_DIGITAL))
#define XFCE_CLOCK_IS_DIGITAL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_DIGITAL))
#define XFCE_CLOCK_DIGITAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_CLOCK_TYPE_DIGITAL, XfceClockDigitalClass))

GType      xfce_clock_digital_get_type      (void) G_GNUC_CONST;

void       xfce_clock_digital_register_type (GTypeModule      *type_module);

GtkWidget *xfce_clock_digital_new           (void) G_GNUC_MALLOC;

gboolean   xfce_clock_digital_update        (gpointer          user_data);

guint      xfce_clock_digital_interval      (XfceClockDigital *digital);

G_END_DECLS

#endif /* !__CLOCK_DIGITAL_H__ */
