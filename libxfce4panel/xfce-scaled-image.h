/* $Id$ */
/*
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

#if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __XFCE_SCALED_IMAGE_H__
#define __XFCE_SCALED_IMAGE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XfceScaledImageClass XfceScaledImageClass;
typedef struct _XfceScaledImage      XfceScaledImage;

#define XFCE_TYPE_SCALED_IMAGE            (xfce_scaled_image_get_type ())
#define XFCE_SCALED_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SCALED_IMAGE, XfceScaledImage))
#define XFCE_SCALED_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SCALED_IMAGE, XfceScaledImageClass))
#define XFCE_IS_SCALED_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SCALED_IMAGE))
#define XFCE_IS_SCALED_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SCALED_IMAGE))
#define XFCE_SCALED_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SCALED_IMAGE, XfceScaledImageClass))

PANEL_SYMBOL_EXPORT
GType      xfce_scaled_image_get_type           (void) G_GNUC_CONST;

GtkWidget *xfce_scaled_image_new                (void) G_GNUC_MALLOC;

GtkWidget *xfce_scaled_image_new_from_pixbuf    (GdkPixbuf       *pixbuf) G_GNUC_MALLOC;

GtkWidget *xfce_scaled_image_new_from_icon_name (const gchar     *icon_name) G_GNUC_MALLOC;

GtkWidget *xfce_scaled_image_new_from_file      (const gchar     *filename) G_GNUC_MALLOC;

void       xfce_scaled_image_set_from_pixbuf    (XfceScaledImage *image,
                                                 GdkPixbuf       *pixbuf);

void       xfce_scaled_image_set_from_icon_name (XfceScaledImage *image,
                                                 const gchar     *icon_name);

void       xfce_scaled_image_set_from_file      (XfceScaledImage *image,
                                                 const gchar     *filename);

G_END_DECLS

#endif /* !__XFCE_SCALED_IMAGE_H__ */
