/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <exo/exo.h>

#include "separator.h"
#include "separator-dialog_ui.h"


#define SEPARATOR_OFFSET (0.15)
#define SEPARATOR_SIZE   (8)
#define DOTS_SIZE        (6)



static void     separator_plugin_get_property              (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);
static void     separator_plugin_set_property              (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static gboolean separator_plugin_expose_event              (GtkWidget             *widget,
                                                            GdkEventExpose        *event);
static void     separator_plugin_construct                 (XfcePanelPlugin       *panel_plugin);
static gboolean separator_plugin_size_changed              (XfcePanelPlugin       *panel_plugin,
                                                            gint                   size);
static void     separator_plugin_configure_plugin          (XfcePanelPlugin       *panel_plugin);
static void     separator_plugin_orientation_changed       (XfcePanelPlugin       *panel_plugin,
                                                            GtkOrientation         orientation);



enum _SeparatorPluginStyle
{
  SEPARATOR_PLUGIN_STYLE_TRANSPARENT = 0,
  SEPARATOR_PLUGIN_STYLE_SEPARATOR,
  SEPARATOR_PLUGIN_STYLE_HANDLE,
  SEPARATOR_PLUGIN_STYLE_DOTS,
  SEPARATOR_PLUGIN_STYLE_WRAP, /* not used in 4.10, nrows property is now used */

  /* defines */
  SEPARATOR_PLUGIN_STYLE_MIN = SEPARATOR_PLUGIN_STYLE_TRANSPARENT,
  SEPARATOR_PLUGIN_STYLE_MAX = SEPARATOR_PLUGIN_STYLE_WRAP,
  SEPARATOR_PLUGIN_STYLE_DEFAULT = SEPARATOR_PLUGIN_STYLE_SEPARATOR
};

struct _SeparatorPluginClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;
};

struct _SeparatorPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* separator style */
  SeparatorPluginStyle  style;
};

enum
{
  PROP_0,
  PROP_STYLE,
  PROP_EXPAND
};



static const gchar bits[3][6] =
{
  { 0x00, 0x0e, 0x02, 0x02, 0x00, 0x00 }, /* dark */
  { 0x00, 0x00, 0x10, 0x10, 0x1c, 0x00 }, /* light */
  { 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00 }  /* mid */
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (SeparatorPlugin, separator_plugin)



static void
separator_plugin_class_init (SeparatorPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;
  GtkWidgetClass       *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = separator_plugin_set_property;
  gobject_class->get_property = separator_plugin_get_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = separator_plugin_expose_event;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = separator_plugin_construct;
  plugin_class->size_changed = separator_plugin_size_changed;
  plugin_class->configure_plugin = separator_plugin_configure_plugin;
  plugin_class->orientation_changed = separator_plugin_orientation_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_STYLE,
                                   g_param_spec_uint ("style",
                                                      NULL, NULL,
                                                      SEPARATOR_PLUGIN_STYLE_MIN,
                                                      SEPARATOR_PLUGIN_STYLE_MAX,
                                                      SEPARATOR_PLUGIN_STYLE_DEFAULT,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_EXPAND,
                                   g_param_spec_boolean ("expand",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
}



static void
separator_plugin_init (SeparatorPlugin *plugin)
{
  plugin->style = SEPARATOR_PLUGIN_STYLE_DEFAULT;
}



static void
separator_plugin_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (object);
  gboolean         expand;

  switch (prop_id)
    {
    case PROP_STYLE:
      g_value_set_uint (value, plugin->style);
      break;

    case PROP_EXPAND:
      expand = xfce_panel_plugin_get_expand (XFCE_PANEL_PLUGIN (plugin));
      g_value_set_boolean (value, expand);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
separator_plugin_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_STYLE:
      plugin->style = g_value_get_uint (value);

      /* old property */
      if (plugin->style == SEPARATOR_PLUGIN_STYLE_WRAP)
        plugin->style = SEPARATOR_PLUGIN_STYLE_DEFAULT;

      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;

    case PROP_EXPAND:
      xfce_panel_plugin_set_expand (XFCE_PANEL_PLUGIN (plugin),
                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
separator_plugin_expose_event (GtkWidget      *widget,
                               GdkEventExpose *event)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (widget);
  GtkAllocation   *alloc = &(widget->allocation);
  GdkBitmap       *bmap;
  GdkGC           *gc;
  GtkStateType     state = GTK_WIDGET_STATE (widget);
  gint             x, y, w, h;
  gint             rows, cols;
  guint            i;

  switch (plugin->style)
    {
    case SEPARATOR_PLUGIN_STYLE_TRANSPARENT:
    case SEPARATOR_PLUGIN_STYLE_WRAP:
      /* do nothing */
      break;

    case SEPARATOR_PLUGIN_STYLE_SEPARATOR:
      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) ==
          GTK_ORIENTATION_HORIZONTAL)
        {
          gtk_paint_vline (widget->style,
                           widget->window,
                           state,
                           &(event->area),
                           widget, "separator",
                           alloc->y + alloc->height * SEPARATOR_OFFSET,
                           alloc->y + alloc->height * (1.00 - SEPARATOR_OFFSET),
                           alloc->x + alloc->width / 2 - 1);
        }
      else
        {
          gtk_paint_hline (widget->style,
                           widget->window,
                           state,
                           &(event->area),
                           widget, "separator",
                           alloc->x + alloc->width * SEPARATOR_OFFSET,
                           alloc->x + alloc->width * (1.00 - SEPARATOR_OFFSET),
                           alloc->y + alloc->height / 2 - 1);
        }
      break;

    case SEPARATOR_PLUGIN_STYLE_HANDLE:
      gtk_paint_handle (widget->style,
                        widget->window,
                        state,
                        GTK_SHADOW_NONE,
                        &(event->area),
                        widget, "handlebox",
                        alloc->x, alloc->y,
                        alloc->width,
                        alloc->height,
                        xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) ==
                            GTK_ORIENTATION_HORIZONTAL ? GTK_ORIENTATION_VERTICAL
                            : GTK_ORIENTATION_HORIZONTAL);
      break;

    case SEPARATOR_PLUGIN_STYLE_DOTS:
      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) ==
          GTK_ORIENTATION_HORIZONTAL)
        {
          rows = MAX (alloc->height / DOTS_SIZE, 1);
          w = DOTS_SIZE;
          h = rows * DOTS_SIZE;
        }
      else
        {
          cols = MAX (alloc->width / DOTS_SIZE, 1);
          h = DOTS_SIZE;
          w = cols * DOTS_SIZE;
        }

      x = alloc->x + (alloc->width - w) / 2;
      y = alloc->y + (alloc->height - h) / 2;

      for (i = 0; i < G_N_ELEMENTS (bits); i++)
        {
          /* pick color, but be same order as bits array */
          if (i == 0)
            gc = widget->style->dark_gc[state];
          else if (i == 1)
            gc = widget->style->light_gc[state];
          else
            gc = widget->style->mid_gc[state];

          /* clip to drawing area */
          gdk_gc_set_clip_rectangle (gc, &(event->area));

          /* set the stipple for the gc */
          bmap = gdk_bitmap_create_from_data (widget->window, bits[i],
                                              DOTS_SIZE, DOTS_SIZE);
          gdk_gc_set_stipple (gc, bmap);
          gdk_gc_set_fill (gc, GDK_STIPPLED);
          g_object_unref (G_OBJECT (bmap));

          /* draw the dots */
          gdk_gc_set_ts_origin (gc, x, y);
          gdk_draw_rectangle (widget->window, gc, TRUE, x, y, w, h);
          gdk_gc_set_fill (gc, GDK_SOLID);

          /* unset the clip */
          gdk_gc_set_clip_rectangle (gc, NULL);
        }
      break;
    }

  return FALSE;
}



static void
separator_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin     *plugin = XFCE_SEPARATOR_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "style", G_TYPE_UINT },
    { "expand", G_TYPE_BOOLEAN },
    { NULL }
  };

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* connect all properties */
  PANEL_UTILS_LINK_4UI
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* make sure the plugin is drawn */
  gtk_widget_queue_draw (GTK_WIDGET (panel_plugin));
}



static gboolean
separator_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                               gint             size)
{
  /* set the minimum separator size */
  if (xfce_panel_plugin_get_orientation (panel_plugin) ==
      GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                 SEPARATOR_SIZE, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                 size, SEPARATOR_SIZE);

  return TRUE;
}



static void
separator_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin *plugin = XFCE_SEPARATOR_PLUGIN (panel_plugin);
  GtkBuilder      *builder;
  GObject         *dialog;
  GObject         *style, *expand;

  panel_return_if_fail (XFCE_IS_SEPARATOR_PLUGIN (plugin));

  /* setup the dialog */
  builder = panel_utils_builder_new (panel_plugin, separator_dialog_ui,
                                     separator_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  style = gtk_builder_get_object (builder, "style");
  exo_mutual_binding_new (G_OBJECT (plugin), "style",
                          G_OBJECT (style), "active");

  expand = gtk_builder_get_object (builder, "expand");
  exo_mutual_binding_new (G_OBJECT (plugin), "expand",
                          G_OBJECT (expand), "active");

  gtk_widget_show (GTK_WIDGET (dialog));
}



static void
separator_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                      GtkOrientation   orientation)
{
  /* for a size change to set the widget size request properly */
  separator_plugin_size_changed (panel_plugin,
                                 xfce_panel_plugin_get_size (panel_plugin));
}
