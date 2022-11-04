/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
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

#include <common/panel-private.h>
#include <wrapper/wrapper-plug-x11.h>
#include <wrapper/wrapper-plug.h>



static void     wrapper_plug_x11_finalize                 (GObject                  *object);
static void     wrapper_plug_x11_iface_init               (WrapperPlugInterface     *iface);
static void     wrapper_plug_x11_set_background_color     (WrapperPlug              *plug,
                                                           const gchar              *color);
static void     wrapper_plug_x11_set_background_image     (WrapperPlug              *plug,
                                                           const gchar              *image);



struct _WrapperPlugX11
{
  GtkPlug __parent__;

  /* background information */
  GtkStyleProvider *style_provider;
};



G_DEFINE_FINAL_TYPE_WITH_CODE (WrapperPlugX11, wrapper_plug_x11, GTK_TYPE_PLUG,
                               G_IMPLEMENT_INTERFACE (WRAPPER_TYPE_PLUG,
                                                      wrapper_plug_x11_iface_init))



static void
wrapper_plug_x11_class_init (WrapperPlugX11Class *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_x11_finalize;
}



static void
wrapper_plug_x11_init (WrapperPlugX11 *plug)
{
  GdkVisual *visual = NULL;
  GdkScreen *screen;
  GtkStyleContext *context;

  gtk_widget_set_name (GTK_WIDGET (plug), "XfcePanelWindowWrapper");

  /* set the colormap */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL && gdk_screen_is_composited (screen))
    gtk_widget_set_visual (GTK_WIDGET (plug), visual);

  /* set the panel class */
  context = gtk_widget_get_style_context (GTK_WIDGET (plug));
  gtk_style_context_add_class (context, "panel");
  gtk_style_context_add_class (context, "xfce4-panel");

  gtk_drag_dest_unset (GTK_WIDGET (plug));

  /* add the style provider */
  plug->style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());

  gtk_style_context_add_provider (context, plug->style_provider,
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}



static void
wrapper_plug_x11_finalize (GObject *object)
{
  g_object_unref (WRAPPER_PLUG_X11 (object)->style_provider);

  G_OBJECT_CLASS (wrapper_plug_x11_parent_class)->finalize (object);
}



static void
wrapper_plug_x11_iface_init (WrapperPlugInterface *iface)
{
  iface->set_background_color = wrapper_plug_x11_set_background_color;
  iface->set_background_image = wrapper_plug_x11_set_background_image;
}



static void
wrapper_plug_x11_set_background_color (WrapperPlug *plug,
                                       const gchar *color)
{
  WrapperPlugX11 *xplug = WRAPPER_PLUG_X11 (plug);
  GdkRGBA rgba;
  gchar *css, *str;

  /* interpret NULL color as user requesting the system theme, so reset the css here */
  if (color == NULL)
    {
      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), "", -1, NULL);
      return;
    }

  if (gdk_rgba_parse (&rgba, color))
    {
      str = gdk_rgba_to_string (&rgba);
      css = g_strdup_printf ("* { background: %s; }", str);
      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), css, -1, NULL);
      g_free (css);
      g_free (str);
    }
}



static void
wrapper_plug_x11_set_background_image (WrapperPlug *plug,
                                       const gchar *image)
{
  WrapperPlugX11 *xplug = WRAPPER_PLUG_X11 (plug);
  gchar *css;

  css = g_strdup_printf ("* { background: url(\"%s\"); }", image);
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (xplug->style_provider), css, -1, NULL);
  g_free (css);
}



GtkWidget *
wrapper_plug_x11_new (Window socket_id)
{
  GtkWidget *plug;

  plug = g_object_new (WRAPPER_TYPE_PLUG_X11, NULL);
  gtk_plug_construct (GTK_PLUG (plug), socket_id);
  gtk_widget_show (plug);

  return plug;
}
