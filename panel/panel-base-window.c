/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <exo/exo.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>
#include <common/panel-private.h>
#include <panel/panel-base-window.h>
#include <panel/panel-window.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-plugin-external-46.h>



#define PANEL_BASE_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                            PANEL_TYPE_BASE_WINDOW, \
                                            PanelBaseWindowPrivate))



static void     panel_base_window_get_property                (GObject              *object,
                                                               guint                 prop_id,
                                                               GValue               *value,
                                                               GParamSpec           *pspec);
static void     panel_base_window_set_property                (GObject              *object,
                                                               guint                 prop_id,
                                                               const GValue         *value,
                                                               GParamSpec           *pspec);
static void     panel_base_window_finalize                    (GObject              *object);
static gboolean panel_base_window_expose_event                (GtkWidget            *widget,
                                                               GdkEventExpose       *event);
static gboolean panel_base_window_enter_notify_event          (GtkWidget            *widget,
                                                               GdkEventCrossing     *event);
static gboolean panel_base_window_leave_notify_event          (GtkWidget            *widget,
                                                               GdkEventCrossing     *event);
static void     panel_base_window_composited_changed          (GtkWidget            *widget);
static void     panel_base_window_update_provider_info        (GtkWidget            *widget,
                                                               gpointer              user_data);
static gboolean panel_base_window_active_timeout              (gpointer              user_data);
static void     panel_base_window_active_timeout_destroyed    (gpointer              user_data);
static void     panel_base_window_set_plugin_background_alpha (GtkWidget            *widget,
                                                               gpointer              user_data);



enum
{
  PROP_0,
  PROP_ENTER_OPACITY,
  PROP_LEAVE_OPACITY,
  PROP_BACKGROUND_ALPHA,
  PROP_BORDERS,
  PROP_ACTIVE,
  PROP_COMPOSITED,
};

struct _PanelBaseWindowPrivate
{
  /* borders */
  PanelBorders borders;

  /* settings */
  gdouble      enter_opacity;
  gdouble      leave_opacity;

  /* active window timeout id */
  guint        active_timeout_id;
};



G_DEFINE_TYPE (PanelBaseWindow, panel_base_window, GTK_TYPE_WINDOW)



static void
panel_base_window_class_init (PanelBaseWindowClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  /* add private data */
  g_type_class_add_private (klass, sizeof (PanelBaseWindowPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = panel_base_window_get_property;
  gobject_class->set_property = panel_base_window_set_property;
  gobject_class->finalize = panel_base_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->expose_event = panel_base_window_expose_event;
  gtkwidget_class->enter_notify_event = panel_base_window_enter_notify_event;
  gtkwidget_class->leave_notify_event = panel_base_window_leave_notify_event;
  gtkwidget_class->composited_changed = panel_base_window_composited_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_ENTER_OPACITY,
                                   g_param_spec_uint ("enter-opacity",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LEAVE_OPACITY,
                                   g_param_spec_uint ("leave-opacity",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_ALPHA,
                                   g_param_spec_uint ("background-alpha",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BORDERS,
                                   g_param_spec_uint ("borders",
                                                      NULL, NULL,
                                                      0, G_MAXUINT,
                                                      PANEL_BORDER_NONE,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_COMPOSITED,
                                   g_param_spec_boolean ("composited",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READABLE));
}



static void
panel_base_window_init (PanelBaseWindow *window)
{
  window->priv = PANEL_BASE_WINDOW_GET_PRIVATE (window);
  window->is_composited = FALSE;
  window->background_alpha = 1.00;

  window->priv->enter_opacity = 1.00;
  window->priv->leave_opacity = 1.00;
  window->priv->borders = PANEL_BORDER_NONE;
  window->priv->active_timeout_id = 0;

  /* set colormap */
  panel_base_window_composited_changed (GTK_WIDGET (window));
}



static void
panel_base_window_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PanelBaseWindow        *window = PANEL_BASE_WINDOW (object);
  PanelBaseWindowPrivate *priv = window->priv;

  switch (prop_id)
    {
    case PROP_ENTER_OPACITY:
      g_value_set_uint (value, rint (priv->enter_opacity * 100.00));
      break;

    case PROP_LEAVE_OPACITY:
      g_value_set_uint (value, rint (priv->leave_opacity * 100.00));
      break;

    case PROP_BACKGROUND_ALPHA:
      g_value_set_uint (value, rint (window->background_alpha * 100.00));
      break;

    case PROP_BORDERS:
      g_value_set_uint (value, priv->borders);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, !!(priv->active_timeout_id != 0));
      break;

    case PROP_COMPOSITED:
      g_value_set_boolean (value, window->is_composited);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_base_window_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PanelBaseWindow        *window = PANEL_BASE_WINDOW (object);
  PanelBaseWindowPrivate *priv = window->priv;
  GtkWidget              *itembar;

  switch (prop_id)
    {
    case PROP_ENTER_OPACITY:
      /* set the new enter opacity */
      priv->enter_opacity = g_value_get_uint (value) / 100.00;
      break;

    case PROP_LEAVE_OPACITY:
      /* set the new leave opacity */
      priv->leave_opacity = g_value_get_uint (value) / 100.00;
      if (window->is_composited)
        gtk_window_set_opacity (GTK_WINDOW (object), priv->leave_opacity);
      break;

    case PROP_BACKGROUND_ALPHA:
      /* set the new background alpha */
      window->background_alpha = g_value_get_uint (value) / 100.00;
      if (window->is_composited)
        gtk_widget_queue_draw (GTK_WIDGET (object));

      /* send the new background alpha to the external plugins */
      itembar = gtk_bin_get_child (GTK_BIN (window));
      if (G_LIKELY (itembar != NULL))
        gtk_container_foreach (GTK_CONTAINER (itembar),
            panel_base_window_set_plugin_background_alpha, window);
      break;

    case PROP_BORDERS:
      /* set new window borders and redraw the widget */
      panel_base_window_set_borders (PANEL_BASE_WINDOW (object),
                                     g_value_get_uint (value));
      break;

    case PROP_ACTIVE:
      if (g_value_get_boolean (value))
        {
          if (priv->active_timeout_id == 0)
            {
              /* start timeout for the marching ants selection */
              priv->active_timeout_id = g_timeout_add_seconds_full (
                  G_PRIORITY_DEFAULT_IDLE, 1,
                  panel_base_window_active_timeout, object,
                  panel_base_window_active_timeout_destroyed);
            }
        }
      else if (priv->active_timeout_id != 0)
        {
          /* stop timeout */
          g_source_remove (priv->active_timeout_id);
        }

      /* queue a draw for first second */
      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_base_window_finalize (GObject *object)
{
  PanelBaseWindowPrivate *priv = PANEL_BASE_WINDOW (object)->priv;

  /* stop running marching ants timeout */
  if (priv->active_timeout_id != 0)
    g_source_remove (priv->active_timeout_id);

  (*G_OBJECT_CLASS (panel_base_window_parent_class)->finalize) (object);
}



static gboolean
panel_base_window_expose_event (GtkWidget      *widget,
                                GdkEventExpose *event)
{
  cairo_t                *cr;
  GdkColor               *color;
  PanelBaseWindow        *window = PANEL_BASE_WINDOW (widget);
  PanelBaseWindowPrivate *priv = window->priv;
  gdouble                 alpha;
  gdouble                 width = widget->allocation.width;
  gdouble                 height = widget->allocation.height;
  const gdouble           dashes[] = { 4.00, 4.00 };
  GTimeVal                timeval;
  gboolean                result;

  result = (*GTK_WIDGET_CLASS (panel_base_window_parent_class)->expose_event) (widget, event);

  if (!GTK_WIDGET_DRAWABLE (widget))
    return result;

  /* create cairo context and set some default properties */
  cr = gdk_cairo_create (widget->window);
  panel_return_val_if_fail (cr != NULL, result);
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width (cr, 1.00);

  /* clip the drawing area */
  gdk_cairo_rectangle (cr, &event->area);

  /* get background alpha */
  alpha = window->is_composited ? window->background_alpha : 1.00;

  /* only do something with the background when compositing is enabled */
  if (G_UNLIKELY (alpha < 1.00))
    {
      /* clip the drawing area, but preserve the rectangle */
      cairo_clip_preserve (cr);

      /* make the background transparent */
      color = &(widget->style->bg[GTK_STATE_NORMAL]);
      panel_util_set_source_rgba (cr, color, alpha);
      cairo_fill (cr);
    }
  else
    {
      /* clip the drawing area */
      cairo_clip (cr);
    }

  /* draw marching ants selection if the timeout is running */
  if (G_UNLIKELY (priv->active_timeout_id != 0))
    {
      /* red color, no alpha */
      cairo_set_source_rgb (cr, 1.00, 0.00, 0.00);

      /* set dash based on time (odd/even) */
      g_get_current_time (&timeval);
      cairo_set_dash (cr, dashes, G_N_ELEMENTS (dashes),
                      (timeval.tv_sec % 4) * 2);

      /* draw rectangle */
      cairo_rectangle (cr, 0.5, 0.5, width - 0.50, height - 0.50);
      cairo_stroke (cr);
    }
  else
    {
      if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_BOTTOM | PANEL_BORDER_RIGHT))
        {
          /* use dark color for buttom and right line */
          color = &(widget->style->dark[GTK_STATE_NORMAL]);
          panel_util_set_source_rgba (cr, color, alpha);

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_BOTTOM))
            {
              cairo_move_to (cr, 0.50, height - 0.50);
              cairo_rel_line_to (cr, width, 0.50);
            }

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_RIGHT))
            {
              cairo_move_to (cr, width - 0.50, 0.50);
              cairo_rel_line_to (cr, 0.50, height);
            }

          cairo_stroke (cr);
        }

      if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_TOP | PANEL_BORDER_LEFT))
        {
          /* use light color for top and left line */
          color = &(widget->style->light[GTK_STATE_NORMAL]);
          panel_util_set_source_rgba (cr, color, alpha);

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_LEFT))
            {
              cairo_move_to (cr, 0.50, 0.50);
              cairo_rel_line_to (cr, 0.50, height);
            }

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_TOP))
            {
              cairo_move_to (cr, 0.50, 0.50);
              cairo_rel_line_to (cr, width, 0.50);
            }

          cairo_stroke (cr);
        }
    }

  cairo_destroy (cr);

  return result;
}



static gboolean
panel_base_window_enter_notify_event (GtkWidget        *widget,
                                      GdkEventCrossing *event)
{
  PanelBaseWindowPrivate *priv = PANEL_BASE_WINDOW (widget)->priv;

  /* switch to enter opacity when compositing is enabled
   * and the two values are different */
  if (event->detail != GDK_NOTIFY_INFERIOR
      && PANEL_BASE_WINDOW (widget)->is_composited
      && priv->leave_opacity != priv->enter_opacity)
    gtk_window_set_opacity (GTK_WINDOW (widget), priv->enter_opacity);

  return FALSE;
}



static gboolean
panel_base_window_leave_notify_event (GtkWidget        *widget,
                                      GdkEventCrossing *event)
{
  PanelBaseWindowPrivate *priv = PANEL_BASE_WINDOW (widget)->priv;

  /* switch to leave opacity when compositing is enabled
   * and the two values are different */
  if (event->detail != GDK_NOTIFY_INFERIOR
      && PANEL_BASE_WINDOW (widget)->is_composited
      && priv->leave_opacity != priv->enter_opacity)
    gtk_window_set_opacity (GTK_WINDOW (widget), priv->leave_opacity);

  return FALSE;
}



static void
panel_base_window_composited_changed (GtkWidget *widget)
{
  PanelBaseWindow *window = PANEL_BASE_WINDOW (widget);
  GdkColormap     *colormap = NULL;
  gboolean         was_composited = window->is_composited;
  gboolean         was_visible;
  GtkWidget       *itembar;
  GdkScreen       *screen;

  panel_return_if_fail (PANEL_IS_BASE_WINDOW (widget));

  /* check if the window is visible, if so, hide and unrealize it */
  was_visible = GTK_WIDGET_VISIBLE (widget);
  if (was_visible)
    {
      gtk_widget_hide (widget);
      gtk_widget_unrealize (widget);
    }

  /* get the widget screen */
  screen = gtk_window_get_screen (GTK_WINDOW (widget));
  panel_return_if_fail (GDK_IS_SCREEN (screen));

  /* get the rgba colormap if compositing is supported */
  if (gtk_widget_is_composited (widget))
    colormap = gdk_screen_get_rgba_colormap (screen);

  /* fallback to the old colormap */
  if (colormap == NULL)
    {
      window->is_composited = FALSE;
      colormap = gdk_screen_get_rgb_colormap (screen);
    }
  else
    {
      window->is_composited = TRUE;
      gtk_window_set_opacity (GTK_WINDOW (widget), window->priv->leave_opacity);
    }

  /* set the new colormap */
  panel_return_if_fail (GDK_IS_COLORMAP (colormap));
  gtk_widget_set_colormap (widget, colormap);

  if (was_visible)
    {
      /* we destroyed all external plugin during unrealize, so queue
       * new provider information for the panel window (not the
       * autohide window) */
      if (PANEL_IS_WINDOW (widget))
        {
          itembar = gtk_bin_get_child (GTK_BIN (window));
          panel_return_if_fail (GTK_IS_CONTAINER (itembar));
          if (G_LIKELY (itembar != NULL))
            gtk_container_foreach (GTK_CONTAINER (itembar),
                panel_base_window_update_provider_info, window);
        }

      /* restore the window */
      gtk_widget_realize (widget);
      gtk_widget_show (widget);
    }

  /* emit the property if it changed */
  if (window->is_composited != was_composited)
    g_object_notify (G_OBJECT (widget), "composited");
}



static void
panel_base_window_update_provider_info (GtkWidget *widget,
                                        gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));
  panel_return_if_fail (PANEL_IS_WINDOW (user_data));

  if (PANEL_IS_PLUGIN_EXTERNAL (widget)
      || PANEL_IS_PLUGIN_EXTERNAL_46 (widget))
    panel_window_set_povider_info (user_data, widget);
}



static gboolean
panel_base_window_active_timeout (gpointer user_data)
{
  /* redraw to update marching ants */
  gtk_widget_queue_draw (GTK_WIDGET (user_data));

  return TRUE;
}



static void
panel_base_window_active_timeout_destroyed (gpointer user_data)
{
  PANEL_BASE_WINDOW (user_data)->priv->active_timeout_id = 0;
}



static void
panel_base_window_set_plugin_background_alpha (GtkWidget *widget,
                                               gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));
  panel_return_if_fail (PANEL_IS_BASE_WINDOW (user_data));

  /* if the plugin is external, send the new alpha value to the wrapper/socket  */
  if (PANEL_IS_PLUGIN_EXTERNAL (widget))
    panel_plugin_external_set_background_alpha (PANEL_PLUGIN_EXTERNAL (widget),
        PANEL_BASE_WINDOW (user_data)->background_alpha);
  else if (PANEL_IS_PLUGIN_EXTERNAL_46 (widget))
    panel_plugin_external_46_set_background_alpha (PANEL_PLUGIN_EXTERNAL_46 (widget),
        PANEL_BASE_WINDOW (user_data)->background_alpha);
}



void
panel_base_window_move_resize (PanelBaseWindow *window,
                               gint             x,
                               gint             y,
                               gint             width,
                               gint             height)
{
  panel_return_if_fail (PANEL_IS_BASE_WINDOW (window));

  if (width > 0 && height > 0)
    gtk_window_resize (GTK_WINDOW (window), width, height);

  gtk_window_move (GTK_WINDOW (window), x, y);
}



void
panel_base_window_set_borders (PanelBaseWindow *window,
                               PanelBorders     borders)
{
  PanelBaseWindowPrivate *priv = window->priv;

  panel_return_if_fail (PANEL_IS_BASE_WINDOW (window));

  if (priv->borders != borders)
    {
      priv->borders = borders;
      gtk_widget_queue_resize (GTK_WIDGET (window));
    }
}



PanelBorders
panel_base_window_get_borders (PanelBaseWindow *window)
{
  PanelBaseWindowPrivate *priv = window->priv;

  panel_return_val_if_fail (PANEL_IS_BASE_WINDOW (window), PANEL_BORDER_NONE);

  /* show all borders for the marching ants */
  if (priv->active_timeout_id != 0)
    return PANEL_BORDER_TOP | PANEL_BORDER_BOTTOM
           | PANEL_BORDER_LEFT | PANEL_BORDER_RIGHT;

  return priv->borders;
}



void
panel_util_set_source_rgba (cairo_t  *cr,
                            GdkColor *color,
                            gdouble   alpha)
{
  panel_return_if_fail (alpha >= 0.00 && alpha <= 1.00);

  if (G_LIKELY (alpha == 1.00))
    cairo_set_source_rgb (cr, color->red / 65535.00,
                          color->green / 65535.00,
                          color->blue / 65535.00);
  else
    cairo_set_source_rgba (cr, color->red / 65535.00,
                           color->green / 65535.00,
                           color->blue / 65535.00, alpha);
}
