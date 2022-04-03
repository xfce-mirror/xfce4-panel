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



struct _WrapperPlugClass
{
  GtkPlugClass __parent__;
};

struct _WrapperPlug
{
  GtkPlug __parent__;

  /* stored styles */
  GtkStyleContext  *style_context;
  GtkStyleProvider *style_provider;
};



/* shared internal plugin name */
gchar *wrapper_name = NULL;



G_DEFINE_TYPE (WrapperPlug, wrapper_plug, GTK_TYPE_PLUG)



static void
wrapper_plug_class_init (WrapperPlugClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = wrapper_plug_finalize;
}



static void
wrapper_plug_init (WrapperPlug *plug)
{
  GdkVisual *visual = NULL;
  GdkScreen *screen;

  plug->style_context = gtk_widget_get_style_context (GTK_WIDGET (plug));
  plug->style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new());

  gtk_widget_set_name (GTK_WIDGET (plug), "XfcePanelWindowWrapper");

  /* set the colormap */
  screen = gtk_window_get_screen (GTK_WINDOW (plug));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    gtk_widget_set_visual (GTK_WIDGET (plug), visual);

  /* set the panel class */
  gtk_style_context_add_class (plug->style_context, "panel");
  gtk_style_context_add_class (plug->style_context, "xfce4-panel");

  gtk_drag_dest_unset (GTK_WIDGET (plug));

  /* add the style provider */
  gtk_style_context_add_provider (plug->style_context, plug->style_provider,
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}



static void
wrapper_plug_finalize (GObject *object)
{
  g_object_unref (WRAPPER_PLUG (object)->style_provider);

  G_OBJECT_CLASS (wrapper_plug_parent_class)->finalize (object);
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
  GdkRGBA  color;
  gchar   *css;

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  /* interpret NULL color as user requesting the system theme, so reset the css here */
  if (color_string == NULL)
    {
      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (plug->style_provider), "", -1, NULL);
      return;
    }

  if (gdk_rgba_parse (&color, color_string))
    {
      css = g_strdup_printf ("* { background: %s; }", gdk_rgba_to_string (&color));

      gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (plug->style_provider), css, -1, NULL);

      g_free (css);
    }
}



void
wrapper_plug_set_background_image (WrapperPlug *plug,
                                   const gchar *image)
{
  gchar *css;

  panel_return_if_fail (WRAPPER_IS_PLUG (plug));

  css = g_strdup_printf ("* { background: url('%s'); }", image);

  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (plug->style_provider), css, -1, NULL);

  g_free (css);
}
