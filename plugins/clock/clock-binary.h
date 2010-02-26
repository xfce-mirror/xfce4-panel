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

#ifndef __CLOCK_BINARY_H__
#define __CLOCK_BINARY_H__

G_BEGIN_DECLS

typedef struct _XfceClockBinaryClass XfceClockBinaryClass;
typedef struct _XfceClockBinary      XfceClockBinary;

#define XFCE_CLOCK_TYPE_BINARY            (xfce_clock_binary_get_type ())
#define XFCE_CLOCK_BINARY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_CLOCK_TYPE_BINARY, XfceClockBinary))
#define XFCE_CLOCK_BINARY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_BINARY, XfceClockBinaryClass))
#define XFCE_CLOCK_IS_BINARY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_CLOCK_TYPE_BINARY))
#define XFCE_CLOCK_IS_BINARY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_CLOCK_TYPE_BINARY))
#define XFCE_CLOCK_BINARY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_CLOCK_TYPE_BINARY, XfceClockBinaryClass))

GType      xfce_clock_binary_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_clock_binary_new      (void) G_GNUC_MALLOC;

gboolean   xfce_clock_binary_update   (gpointer user_data);

G_END_DECLS

#endif /* !__CLOCK_BINARY_H__ */
