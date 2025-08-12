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

#include "separator.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"
#include "common/panel-xfconf.h"
#include "libxfce4panel/xfce-panel-plugin-provider.h"

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>


#define SEPARATOR_OFFSET (0.15)
#define MIN_SIZE (1)
#define DEFAULT_SIZE (8)
#define MAX_SIZE (G_MAXUINT)
#define DOTS_OFFSET (4)
#define DOTS_SIZE (3)
#define HANDLE_SIZE (4)



static void
separator_plugin_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec);
static void
separator_plugin_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec);
static gboolean
separator_plugin_draw (GtkWidget *widget,
                       cairo_t *cr);
static void
separator_plugin_construct (XfcePanelPlugin *panel_plugin);
static gboolean
separator_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                               gint size);
static void
separator_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void
separator_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                      GtkOrientation orientation);



typedef enum _SeparatorPluginStyle
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
} SeparatorPluginStyle;

struct _SeparatorPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* settings */
  GObject *settings_dialog;
  SeparatorPluginStyle style;
  guint size;
};

enum
{
  PROP_0,
  PROP_STYLE,
  PROP_EXPAND,
  PROP_SIZE,
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (SeparatorPlugin, separator_plugin)



static void
separator_plugin_class_init (SeparatorPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = separator_plugin_set_property;
  gobject_class->get_property = separator_plugin_get_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->draw = separator_plugin_draw;

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
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_EXPAND,
                                   g_param_spec_boolean ("expand",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_uint ("size",
                                                      NULL, NULL,
                                                      MIN_SIZE,
                                                      MAX_SIZE,
                                                      DEFAULT_SIZE,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
separator_plugin_init (SeparatorPlugin *plugin)
{
  plugin->style = SEPARATOR_PLUGIN_STYLE_DEFAULT;
  plugin->size = DEFAULT_SIZE;
}



static void
separator_plugin_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (object);
  gboolean expand;

  switch (prop_id)
    {
    case PROP_STYLE:
      g_value_set_uint (value, plugin->style);
      break;

    case PROP_EXPAND:
      expand = xfce_panel_plugin_get_expand (XFCE_PANEL_PLUGIN (plugin));
      g_value_set_boolean (value, expand);
      break;

    case PROP_SIZE:
      g_value_set_uint (value, plugin->size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
separator_plugin_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (object);

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
      separator_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
                                     xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
      break;

    case PROP_SIZE:
      plugin->size = g_value_get_uint (value);
      separator_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
                                     xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
separator_plugin_draw (GtkWidget *widget,
                       cairo_t *cr)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (widget);
  GtkAllocation alloc;
  gdouble x, y;
  guint dotcount, i;
  GtkStyleContext *ctx;
  GdkRGBA fg_rgba;

  gtk_widget_get_allocation (widget, &alloc);

  ctx = gtk_widget_get_style_context (widget);
  gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (widget), &fg_rgba);
  /* Tone down the foreground color a bit for the separators */
  fg_rgba.alpha = 0.5;
  gdk_cairo_set_source_rgba (cr, &fg_rgba);

  switch (plugin->style)
    {
    case SEPARATOR_PLUGIN_STYLE_TRANSPARENT:
    case SEPARATOR_PLUGIN_STYLE_WRAP:
      /* do nothing */
      break;

    case SEPARATOR_PLUGIN_STYLE_SEPARATOR:

      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_HORIZONTAL)
        {
          gtk_render_line (ctx, cr,
                           (gdouble) (alloc.width - 1.0) / 2.0,
                           (gdouble) alloc.height * SEPARATOR_OFFSET,
                           (gdouble) (alloc.width - 1.0) / 2.0,
                           (gdouble) alloc.height * (1.0 - SEPARATOR_OFFSET));
        }
      else
        {
          gtk_render_line (ctx, cr,
                           (gdouble) alloc.width * SEPARATOR_OFFSET,
                           (gdouble) (alloc.height - 1.0) / 2.0,
                           (gdouble) alloc.width * (1.0 - SEPARATOR_OFFSET),
                           (gdouble) (alloc.height - 1.0) / 2.0);
        }
      break;

    case SEPARATOR_PLUGIN_STYLE_HANDLE:
      x = (alloc.width - HANDLE_SIZE) / 2;
      y = (alloc.height - HANDLE_SIZE) / 2;
      cairo_set_line_width (cr, 1.5);
      /* draw the handle */
      for (i = 0; i < 3; i++)
        {
          if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_HORIZONTAL)
            {
              cairo_move_to (cr, x, y + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2));
              cairo_line_to (cr, x + HANDLE_SIZE, y + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2));
            }
          else
            {
              cairo_move_to (cr, x + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2), y);
              cairo_line_to (cr, x + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2), y + HANDLE_SIZE);
            }
          cairo_stroke (cr);
        }
      break;

    case SEPARATOR_PLUGIN_STYLE_DOTS:
      x = (alloc.width - DOTS_SIZE) / 2;
      y = (alloc.height - DOTS_SIZE) / 2;
      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_HORIZONTAL)
        {
          dotcount = MAX (alloc.height / (DOTS_SIZE + DOTS_OFFSET), 1);
          y = (alloc.height / (double) dotcount - DOTS_SIZE) / 2;
        }
      else
        {
          dotcount = MAX (alloc.width / (DOTS_SIZE + DOTS_OFFSET), 1);
          x = (alloc.width / (double) dotcount - DOTS_SIZE) / 2;
        }

      /* draw the dots */
      for (i = 0; i < dotcount; i++)
        {
          if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_HORIZONTAL)
            cairo_arc (cr, x, y + (i * (alloc.height / (double) dotcount)) + (DOTS_SIZE / 2),
                       DOTS_SIZE / 2, 0, 2 * 3.14);
          else
            cairo_arc (cr, x + (i * (alloc.width / (double) dotcount)) + (DOTS_SIZE / 2), y,
                       DOTS_SIZE / 2, 0, 2 * 3.14);
          cairo_fill (cr);
        }
      break;
    }

  return FALSE;
}



static void
separator_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (panel_plugin);
  const PanelProperty properties[] = {
    { "style", G_TYPE_UINT },
    { "expand", G_TYPE_BOOLEAN },
    { "size", G_TYPE_UINT },
    { NULL }
  };

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* connect all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* make sure the plugin is drawn */
  gtk_widget_queue_draw (GTK_WIDGET (panel_plugin));
}



static gboolean
separator_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                               gint size)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (panel_plugin);

  if (xfce_panel_plugin_get_expand (panel_plugin))
    {
      /* set the minimum separator size */
      if (xfce_panel_plugin_get_orientation (panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                     MIN_SIZE, size);
      else
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                     size, MIN_SIZE);
    }
  else
    {
      if (xfce_panel_plugin_get_orientation (panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                     plugin->size, size);
      else
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                                     size, plugin->size);
    }

  return TRUE;
}



static void
separator_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SeparatorPlugin *plugin = SEPARATOR_PLUGIN (panel_plugin);
  GtkBuilder *builder;
  GObject *style, *expand, *size, *size_scale;

  panel_return_if_fail (SEPARATOR_IS_PLUGIN (plugin));

  /* setup the dialog */
  builder = panel_utils_builder_new (panel_plugin, "/org/xfce/panel/separator-dialog.glade", &plugin->settings_dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  style = gtk_builder_get_object (builder, "style");
  g_object_bind_property (G_OBJECT (plugin), "style",
                          G_OBJECT (style), "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  expand = gtk_builder_get_object (builder, "expand");
  g_object_bind_property (G_OBJECT (plugin), "expand",
                          G_OBJECT (expand), "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  size = gtk_builder_get_object (builder, "size");

  gtk_adjustment_set_lower (GTK_ADJUSTMENT (size), MIN_SIZE);
  gtk_adjustment_set_upper (GTK_ADJUSTMENT (size), MAX_SIZE);

  g_object_bind_property (G_OBJECT (plugin), "size",
                          G_OBJECT (size), "value",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  size_scale = gtk_builder_get_object (builder, "size-scale");
  g_object_bind_property (G_OBJECT (expand), "active",
                          G_OBJECT (size_scale), "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  gtk_widget_show (GTK_WIDGET (plugin->settings_dialog));
}



static void
separator_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                      GtkOrientation orientation)
{
  /* for a size change to set the widget size request properly */
  separator_plugin_size_changed (panel_plugin,
                                 xfce_panel_plugin_get_size (panel_plugin));
}
