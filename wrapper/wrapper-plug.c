/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <wrapper/wrapper-plug.h>
#include <common/panel-private.h>



static void     wrapper_plug_finalize         (GObject        *object);
static gboolean wrapper_plug_draw             (GtkWidget      *widget,
                                               cairo_t        *cr);
static void     wrapper_plug_background_reset (WrapperPlug    *plug);



struct _WrapperPlugClass
{
  GtkPlugClass __parent__;
};

struct _WrapperPlug
{
  GtkPlug __parent__;

  /* background information */
  GdkRGBA         *background_rgba;
  gchar           *background_image;
  cairo_pattern_t *background_image_cache;
};



/* shared internal plugin name */
gchar *wrapper_name = NULL;



G_DEFINE_TYPE (WrapperPlug, wrapper_plug, GTK_TYPE_PLUG)



static void
wrapper_plug_class_init (WrapperPlugClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->draw = wrapper_plug_draw;
}



static void
wrapper_plug_init (WrapperPlug *plug)
{
  GdkVisual       *visual = NULL;
  GdkScreen       *screen;
  GtkStyleContext *context;

  plug->background_rgba = NULL;
  plug->background_image = NULL;
  plug->background_image_cache = NULL;

  gtk_widget_set_name (GTK_WIDGET (plug), "XfcePanelWindowWrapper");

  /* allow painting, else compositing won't work */
  gtk_widget_set_app_paintable (GTK_WIDGET (plug), TRUE);

  /* set the colormap */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (plug), visual);

  /* set the panel class */
  context = gtk_widget_get_style_context (GTK_WIDGET (plug));
  gtk_style_context_add_class (context, "panel");
  gtk_style_context_add_class (context, "xfce4-panel");

  gtk_drag_dest_unset (GTK_WIDGET (plug));
}



static void
wrapper_plug_finalize (GObject *object)
{
  wrapper_plug_background_reset (WRAPPER_PLUG (object));

  G_OBJECT_CLASS (wrapper_plug_parent_class)->finalize (object);
}



static gboolean
wrapper_plug_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  WrapperPlug     *plug = WRAPPER_PLUG (widget);
  GtkStyleContext *context;
  const GdkRGBA   *color;
  GdkRGBA         *rgba;
  GdkPixbuf       *pixbuf;
  GError          *error = NULL;

  cairo_save (cr);

  /* The "draw" signal is in widget coordinates, transform back to window */
  gtk_cairo_transform_to_window (cr,
                                 GTK_WIDGET (plug),
                                 gtk_widget_get_window (gtk_widget_get_toplevel (GTK_WIDGET (plug))));

  if (G_UNLIKELY (plug->background_image != NULL))
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

      if (G_LIKELY (plug->background_image_cache != NULL))
        {
          cairo_set_source (cr, plug->background_image_cache);
          cairo_paint (cr);
        }
      else
        {
          /* load the image in a pixbuf */
          pixbuf = gdk_pixbuf_new_from_file (plug->background_image, &error);

          if (G_LIKELY (pixbuf != NULL))
            {
              gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
              g_object_unref (G_OBJECT (pixbuf));

              plug->background_image_cache = cairo_get_source (cr);
              cairo_pattern_reference (plug->background_image_cache);
              cairo_pattern_set_extend (plug->background_image_cache, CAIRO_EXTEND_REPEAT);
              cairo_paint (cr);
            }
          else
            {
              /* print error message */
              g_warning ("Background image disabled, \"%s\" could not be loaded: %s",
                         plug->background_image, error != NULL ? error->message : "No error");
              g_error_free (error);

              /* disable background image */
              wrapper_plug_background_reset (plug);
            }
        }
    }
  else
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

      /* get the background gdk rgba */
      if (plug->background_rgba != NULL)
        {
          color = plug->background_rgba;
          cairo_set_source_rgba (cr, color->red, color->green,
                                 color->blue, color->alpha);
        }
      else
        {
          context = gtk_widget_get_style_context (widget);
          gtk_style_context_get (context, GTK_STATE_FLAG_NORMAL,
                                 GTK_STYLE_PROPERTY_BACKGROUND_COLOR,
                                 &rgba, NULL);
          gdk_cairo_set_source_rgba (cr, rgba);
          gdk_rgba_free (rgba);
        }

      /* draw the background color */
      cairo_paint (cr);
    }

  cairo_restore(cr);

  return GTK_WIDGET_CLASS (wrapper_plug_parent_class)->draw (widget, cr);
}



static void
wrapper_plug_background_reset (WrapperPlug *plug)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  if (plug->background_rgba != NULL)
    gdk_rgba_free (plug->background_rgba);
  plug->background_rgba = NULL;

  if (plug->background_image_cache != NULL)
    cairo_pattern_destroy (plug->background_image_cache);
  plug->background_image_cache = NULL;

  g_free (plug->background_image);
  plug->background_image = NULL;
}



WrapperPlug *
wrapper_plug_new (Window socket_id)
{
  WrapperPlug *plug;

  /* create new object */
  plug = g_object_new (WRAPPER_TYPE_PLUG, NULL);

  /* contruct the plug */
  gtk_plug_construct (GTK_PLUG (plug), socket_id);

  return plug;
}



void
wrapper_plug_set_opacity (WrapperPlug *plug,
                          gdouble      opacity)
{

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  if (gtk_widget_get_opacity (GTK_WIDGET (plug)) != opacity)
    gtk_widget_set_opacity (GTK_WIDGET (plug), opacity);
}



void
wrapper_plug_set_background_color (WrapperPlug *plug,
                                   const gchar *color_string)
{
  GdkRGBA                 color;

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  wrapper_plug_background_reset (plug);

  if (color_string != NULL
      && gdk_rgba_parse (&color, color_string))
    plug->background_rgba = gdk_rgba_copy (&color);

  gtk_widget_queue_draw (GTK_WIDGET (plug));
}



void
wrapper_plug_set_background_image (WrapperPlug *plug,
                                   const gchar *image)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  wrapper_plug_background_reset (plug);

  plug->background_image = g_strdup (image);

  gtk_widget_queue_draw (GTK_WIDGET (plug));
}
