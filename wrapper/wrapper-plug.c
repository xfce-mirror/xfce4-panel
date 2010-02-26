/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

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



static void     wrapper_plug_class_init                (WrapperPlugClass        *klass);
static void     wrapper_plug_init                      (WrapperPlug             *plug);
static gboolean wrapper_plug_expose_event              (GtkWidget               *widget,
                                                        GdkEventExpose          *event);
static void     wrapper_plug_set_colormap              (WrapperPlug             *plug);



struct _WrapperPlugClass
{
  GtkPlugClass __parent__;
};

struct _WrapperPlug
{
  GtkPlug __parent__;

  /* background alpha */
  gdouble background_alpha;

  /* if the plugin background should be the selection color */
  guint   is_selected : 1;

  /* whether the wrapper has a rgba colormap */
  guint   is_composited : 1;
};



G_DEFINE_TYPE (WrapperPlug, wrapper_plug, GTK_TYPE_PLUG);



static void
wrapper_plug_class_init (WrapperPlugClass *klass)
{
  GtkWidgetClass *gtkwidget_class;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->expose_event = wrapper_plug_expose_event;
}



static void
wrapper_plug_init (WrapperPlug *plug)
{
  extern gchar *opt_name;

  /* init vars */
  plug->background_alpha = 1.00;
  plug->is_selected = FALSE;
  plug->is_composited = FALSE;

  /* allow painting, else compositing won't work */
  gtk_widget_set_app_paintable (GTK_WIDGET (plug), TRUE);

  /* connect signal to monitor the compositor changes */
  g_signal_connect (G_OBJECT (plug), "composited-changed", G_CALLBACK (wrapper_plug_set_colormap), NULL);

  /* set the colormap */
  /* HACK: the systray can't handle composited windows! */
  if (strcmp (opt_name, "systray") != 0)
    wrapper_plug_set_colormap (plug);
}



static gboolean
wrapper_plug_expose_event (GtkWidget      *widget,
                           GdkEventExpose *event)
{
  WrapperPlug   *plug = WRAPPER_PLUG (widget);
  cairo_t       *cr;
  GdkColor      *color;
  GtkStateType   state = GTK_STATE_NORMAL;
  gdouble        alpha = plug->is_composited ? plug->background_alpha : 1.00;

  if (GTK_WIDGET_DRAWABLE (widget) &&
      (alpha < 1.00 || plug->is_selected))
    {
      /* create the cairo context */
      cr = gdk_cairo_create (widget->window);

      /* change the state is this plugin is on an active panel */
      if (G_UNLIKELY (plug->is_selected))
        state = GTK_STATE_SELECTED;

      /* get the background gdk color */
      color = &(widget->style->bg[state]);

      /* set the cairo source color */
      xfce_panel_cairo_set_source_rgba (cr, color, alpha);

      /* create retangle */
      cairo_rectangle (cr, event->area.x, event->area.y,
                       event->area.width, event->area.height);

      /* draw on source */
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

      /* paint rectangle */
      cairo_fill (cr);

      /* destroy cairo context */
      cairo_destroy (cr);
    }

  return GTK_WIDGET_CLASS (wrapper_plug_parent_class)->expose_event (widget, event);
}



static void
wrapper_plug_set_colormap (WrapperPlug *plug)
{
  GdkColormap *colormap = NULL;
  GdkScreen   *screen;
  gboolean     restore;
  GtkWidget   *widget = GTK_WIDGET (plug);
  gint         root_x, root_y;

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* whether the widget was previously visible */
  restore = GTK_WIDGET_REALIZED (widget);

  /* unrealize the window if needed */
  if (restore)
    {
      /* store the window position */
      gtk_window_get_position (GTK_WINDOW (plug), &root_x, &root_y);

      /* hide the widget */
      gtk_widget_hide (widget);
      gtk_widget_unrealize (widget);
    }

  /* set bool */
  plug->is_composited = gtk_widget_is_composited (widget);

  /* get the screen */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));

  /* try to get the rgba colormap */
  if (plug->is_composited)
    colormap = gdk_screen_get_rgba_colormap (screen);

  /* get the default colormap */
  if (colormap == NULL)
    {
      colormap = gdk_screen_get_rgb_colormap (screen);
      plug->is_composited = FALSE;
    }

  /* set the colormap */
  if (colormap)
    gtk_widget_set_colormap (widget, colormap);

  /* restore the window */
  if (restore)
    {
      /* restore the position */
      gtk_window_move (GTK_WINDOW (plug), root_x, root_y);

      /* show the widget again */
      gtk_widget_realize (widget);
      gtk_widget_show (widget);
    }

  gtk_widget_queue_draw (widget);
}



WrapperPlug *
wrapper_plug_new (GdkNativeWindow socket_id)
{
  WrapperPlug *plug;

  /* create new object */
  plug = g_object_new (WRAPPER_TYPE_PLUG, NULL);

  /* contruct the plug */
  gtk_plug_construct (GTK_PLUG (plug), socket_id);

  return plug;
}



void
wrapper_plug_set_background_alpha (WrapperPlug *plug,
                                   gdouble      alpha)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));
  panel_return_if_fail (GTK_IS_WIDGET (plug));

  /* set the alpha */
  plug->background_alpha = CLAMP (alpha, 0.00, 1.00);

  /* redraw */
  if (plug->is_composited)
    gtk_widget_queue_draw (GTK_WIDGET (plug));
}



void
wrapper_plug_set_selected (WrapperPlug *plug,
                           gboolean     selected)
{
  panel_return_if_fail (WRAPPER_IS_PLUG (plug));
  panel_return_if_fail (GTK_IS_WIDGET (plug));

  /* set value */
  plug->is_selected = !!selected;

  /* redraw */
  gtk_widget_queue_draw (GTK_WIDGET (plug));
}
