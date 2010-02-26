/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>


/* design limit for the panel, to reduce the uncached pixbuf size */
#define MAX_PIXBUF_SIZE (128)



struct _XfceScaledImageClass
{
  GtkWidgetClass __parent__;
};

struct _XfceScaledImage
{
  GtkWidget __parent__;

  /* pixbuf set by the user */
  GdkPixbuf *pixbuf;

  /* icon name */
  gchar     *icon_name;

  /* internal cached pixbuf (resized) */
  GdkPixbuf *cache;

  /* cached width and height */
  gint       width;
  gint       height;
};



static void       xfce_scaled_image_finalize      (GObject              *object);
static void       xfce_scaled_image_size_request  (GtkWidget            *widget,
                                                   GtkRequisition       *requisition);
static gboolean   xfce_scaled_image_expose_event  (GtkWidget            *widget,
                                                   GdkEventExpose       *event);
static void       xfce_scaled_image_size_allocate (GtkWidget            *widget,
                                                   GtkAllocation        *allocation);
static GdkPixbuf *xfce_scaled_image_scale_pixbuf  (GdkPixbuf            *source,
                                                   gint                  dest_width,
                                                   gint                  dest_height);
static void       xfce_scaled_image_update_cache  (XfceScaledImage      *image);
static void       xfce_scaled_image_cleanup       (XfceScaledImage      *image);



G_DEFINE_TYPE (XfceScaledImage, xfce_scaled_image, GTK_TYPE_WIDGET);



static void
xfce_scaled_image_class_init (XfceScaledImageClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_scaled_image_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->expose_event = xfce_scaled_image_expose_event;
  gtkwidget_class->size_request = xfce_scaled_image_size_request;
  gtkwidget_class->size_allocate = xfce_scaled_image_size_allocate;
}



static void
xfce_scaled_image_init (XfceScaledImage *image)
{
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (image), GTK_NO_WINDOW);

  image->pixbuf = NULL;
  image->icon_name = NULL;
  image->cache = NULL;
  image->width = -1;
  image->height = -1;
}



static void
xfce_scaled_image_finalize (GObject *object)
{
  XfceScaledImage *image = XFCE_SCALED_IMAGE (object);

  /* cleanup */
  xfce_scaled_image_cleanup (image);

  (*G_OBJECT_CLASS (xfce_scaled_image_parent_class)->finalize) (object);
}



static gboolean
xfce_scaled_image_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  XfceScaledImage *image = XFCE_SCALED_IMAGE (widget);
  gint             source_width, source_height;
  gint             dest_x, dest_y;

  if (image->cache)
    {
      /* get the size of the cache pixbuf */
      source_width = gdk_pixbuf_get_width (image->cache);
      source_height = gdk_pixbuf_get_height (image->cache);

      /* position */
      dest_x = widget->allocation.x + (image->width - source_width) / 2;
      dest_y = widget->allocation.y + (image->height - source_height) / 2;

      /* draw the pixbuf */
      gdk_draw_pixbuf (widget->window,
                       widget->style->black_gc,
                       image->cache,
                       0, 0,
                       dest_x, dest_y,
                       source_width, source_height,
                       GDK_RGB_DITHER_NORMAL, 0, 0);
    }

  return FALSE;
}



static void
xfce_scaled_image_size_request (GtkWidget      *widget,
                                GtkRequisition *requisition)
{
  XfceScaledImage *image = XFCE_SCALED_IMAGE (widget);

  if (image->pixbuf)
    {
      widget->requisition.width = gdk_pixbuf_get_width (image->pixbuf);
      widget->requisition.height = gdk_pixbuf_get_height (image->pixbuf);
    }

  GTK_WIDGET_CLASS (xfce_scaled_image_parent_class)->size_request (widget, requisition);
}



static void
xfce_scaled_image_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  XfceScaledImage *image = XFCE_SCALED_IMAGE (widget);

  /* set the widget allocation */
  widget->allocation = *allocation;

  /* check if the available size changed */
  if ((image->pixbuf || image->icon_name)
      && allocation->width > 0
      && allocation->height > 0
      && (allocation->width != image->width
      || allocation->height != image->height))
    {
      /* store the new size */
      image->width = allocation->width;
      image->height = allocation->height;

      /* clear the cache if there is one */
      if (image->cache)
        {
          g_object_unref (G_OBJECT (image->cache));
          image->cache = NULL;
        }

      xfce_scaled_image_update_cache (image);
    }

  GTK_WIDGET_CLASS (xfce_scaled_image_parent_class)->size_allocate (widget, allocation);
}



static GdkPixbuf *
xfce_scaled_image_scale_pixbuf (GdkPixbuf *source,
                                gint       dest_width,
                                gint       dest_height)
{
  gdouble wratio;
  gdouble hratio;
  gint    source_width;
  gint    source_height;

  panel_return_val_if_fail (GDK_IS_PIXBUF (source), NULL);
  panel_return_val_if_fail (dest_width > 0, NULL);
  panel_return_val_if_fail (dest_height > 0, NULL);

  source_width = gdk_pixbuf_get_width (source);
  source_height = gdk_pixbuf_get_height (source);

  /* check if we need to scale */
  if (G_UNLIKELY (source_width <= dest_width && source_height <= dest_height))
    return g_object_ref (G_OBJECT (source));

  /* calculate the new dimensions */
  wratio = (gdouble) source_width  / (gdouble) dest_width;
  hratio = (gdouble) source_height / (gdouble) dest_height;

  if (hratio > wratio)
    dest_width  = rint (source_width / hratio);
  else
    dest_height = rint (source_height / wratio);

  return gdk_pixbuf_scale_simple (source, MAX (dest_width, 1), MAX (dest_height, 1), GDK_INTERP_BILINEAR);
}



static void
xfce_scaled_image_update_cache (XfceScaledImage *image)
{
  GdkPixbuf *pixbuf = NULL;
  GdkScreen *screen;

  panel_return_if_fail (image->cache == NULL);
  panel_return_if_fail (image->pixbuf != NULL || image->icon_name != NULL);

  if (image->pixbuf)
    {
      /* use the pixbuf set by the user */
      pixbuf = g_object_ref (G_OBJECT (image->pixbuf));
    }
  else if (image->icon_name)
    {
      /* get the screen */
      screen = gtk_widget_get_screen (GTK_WIDGET (image));

      /* get a pixbuf from the icon name */
      pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_for_screen (screen),
                                         image->icon_name,
                                         MIN (image->width, image->height),
                                         0, NULL);
    }

  if (G_LIKELY (pixbuf))
    {
      /* create a cache icon that fits in the available size */
      image->cache = xfce_scaled_image_scale_pixbuf (pixbuf, image->width, image->height);

      /* release the pixbuf */
      g_object_unref (G_OBJECT (pixbuf));
    }
}



static void
xfce_scaled_image_cleanup (XfceScaledImage *image)
{
  /* release the pixbuf reference */
  if (G_LIKELY (image->pixbuf))
    g_object_unref (G_OBJECT (image->pixbuf));

  /* release the cached pixbuf */
  if (G_LIKELY (image->cache))
    g_object_unref (G_OBJECT (image->cache));

  /* free the icon name */
  g_free (image->icon_name);

  /* reset varaibles */
  image->pixbuf = NULL;
  image->cache = NULL;
  image->icon_name = NULL;
  image->width = -1;
  image->height = -1;
}



PANEL_SYMBOL_EXPORT GtkWidget *
xfce_scaled_image_new (void)
{
  return g_object_new (XFCE_TYPE_SCALED_IMAGE, NULL);
}



PANEL_SYMBOL_EXPORT GtkWidget *
xfce_scaled_image_new_from_pixbuf (GdkPixbuf *pixbuf)
{
  GtkWidget *image;

  g_return_val_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf), NULL);

  /* create a scaled image */
  image = xfce_scaled_image_new ();

  /* set the pixbuf */
  xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (image), pixbuf);

  return image;
}



PANEL_SYMBOL_EXPORT GtkWidget *
xfce_scaled_image_new_from_icon_name (const gchar *icon_name)
{
  GtkWidget *image;

  /* create a scaled image */
  image = xfce_scaled_image_new ();

  /* set the icon name */
  xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (image), icon_name);

  return image;
}



PANEL_SYMBOL_EXPORT GtkWidget *
xfce_scaled_image_new_from_file (const gchar *filename)
{
  GtkWidget *image;

  /* create a scaled image */
  image = xfce_scaled_image_new ();

  /* set the filename */
  xfce_scaled_image_set_from_file (XFCE_SCALED_IMAGE (image), filename);

  return image;
}



PANEL_SYMBOL_EXPORT void
xfce_scaled_image_set_from_pixbuf (XfceScaledImage *image,
                                   GdkPixbuf       *pixbuf)
{
  gint source_width;
  gint source_height;

  g_return_if_fail (XFCE_IS_SCALED_IMAGE (image));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  /* cleanup */
  xfce_scaled_image_cleanup (image);

  /* get the new pixbuf sizes */
  source_width = gdk_pixbuf_get_width (pixbuf);
  source_height = gdk_pixbuf_get_height (pixbuf);

  /* set the new pixbuf, scale it to the maximum size if needed */
  if (G_LIKELY (source_width <= MAX_PIXBUF_SIZE && source_height <= MAX_PIXBUF_SIZE))
    image->pixbuf = g_object_ref (G_OBJECT (pixbuf));
  else
    image->pixbuf = xfce_scaled_image_scale_pixbuf (pixbuf, MAX_PIXBUF_SIZE, MAX_PIXBUF_SIZE);

  /* queue a resize */
  gtk_widget_queue_resize (GTK_WIDGET (image));
}



PANEL_SYMBOL_EXPORT void
xfce_scaled_image_set_from_icon_name (XfceScaledImage *image,
                                      const gchar     *icon_name)
{
  g_return_if_fail (XFCE_IS_SCALED_IMAGE (image));
  panel_return_if_fail (icon_name == NULL || !g_path_is_absolute (icon_name));

  /* cleanup */
  xfce_scaled_image_cleanup (image);

  /* set the new icon name */
  if (G_LIKELY (IS_STRING (icon_name)))
    image->icon_name = g_strdup (icon_name);

  /* queue a resize */
  gtk_widget_queue_resize (GTK_WIDGET (image));
}



PANEL_SYMBOL_EXPORT void
xfce_scaled_image_set_from_file (XfceScaledImage *image,
                                 const gchar     *filename)
{
  GError    *error = NULL;
  GdkPixbuf *pixbuf;

  g_return_if_fail (XFCE_IS_SCALED_IMAGE (image));
  panel_return_if_fail (filename == NULL || g_path_is_absolute (filename));

  /* cleanup */
  xfce_scaled_image_cleanup (image);

  if (G_LIKELY (IS_STRING (filename)))
    {
      /* try to load the image from the file */
      pixbuf = gdk_pixbuf_new_from_file (filename, &error);

      if (G_LIKELY (pixbuf))
        {
          /* set the new pixbuf */
          xfce_scaled_image_set_from_pixbuf (image, pixbuf);

          /* release the pixbuf */
          g_object_unref (G_OBJECT (pixbuf));
        }
      else
        {
          /* print a warning what went wrong */
          g_critical ("Failed to loading image from filename: %s", error->message);

          /* cleanup */
          g_error_free (error);
        }
    }
}
