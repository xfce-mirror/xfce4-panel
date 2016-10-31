/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
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

#include <gtk/gtk.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>
#include <common/panel-private.h>
#include <common/panel-debug.h>
#include <panel/panel-base-window.h>
#include <panel/panel-window.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-plugin-external-46.h>



static void     panel_base_window_get_property                (GObject              *object,
                                                               guint                 prop_id,
                                                               GValue               *value,
                                                               GParamSpec           *pspec);
static void     panel_base_window_set_property                (GObject              *object,
                                                               guint                 prop_id,
                                                               const GValue         *value,
                                                               GParamSpec           *pspec);
static void     panel_base_window_finalize                    (GObject              *object);
static void     panel_base_window_screen_changed              (GtkWidget            *widget,
                                                               GdkScreen            *previous_screen);
static gboolean panel_base_window_draw                        (GtkWidget            *widget,
                                                               cairo_t              *cr);
static gboolean panel_base_window_enter_notify_event          (GtkWidget            *widget,
                                                               GdkEventCrossing     *event);
static gboolean panel_base_window_leave_notify_event          (GtkWidget            *widget,
                                                               GdkEventCrossing     *event);
static void     panel_base_window_composited_changed          (GtkWidget            *widget);
static gboolean panel_base_window_active_timeout              (gpointer              user_data);
static void     panel_base_window_active_timeout_destroyed    (gpointer              user_data);
static void     panel_base_window_set_plugin_data             (PanelBaseWindow      *window,
                                                               GtkCallback           func);
static void     panel_base_window_set_plugin_background_alpha (GtkWidget            *widget,
                                                               gpointer              user_data);
static void     panel_base_window_set_plugin_background_color (GtkWidget            *widget,
                                                               gpointer              user_data);
static void     panel_base_window_set_plugin_background_image (GtkWidget            *widget,
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
  PROP_BACKGROUND_STYLE,
  PROP_BACKGROUND_COLOR,
  PROP_BACKGROUND_IMAGE
};

struct _PanelBaseWindowPrivate
{
  PanelBorders     borders;

  /* background image cache */
  cairo_pattern_t *bg_image_cache;

  /* transparency settings */
  gdouble          enter_opacity;
  gdouble          leave_opacity;

  /* active window timeout id */
  guint            active_timeout_id;
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
  gtkwidget_class->draw = panel_base_window_draw;
  gtkwidget_class->enter_notify_event = panel_base_window_enter_notify_event;
  gtkwidget_class->leave_notify_event = panel_base_window_leave_notify_event;
  gtkwidget_class->composited_changed = panel_base_window_composited_changed;
  gtkwidget_class->screen_changed = panel_base_window_screen_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_ENTER_OPACITY,
                                   g_param_spec_uint ("enter-opacity",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_LEAVE_OPACITY,
                                   g_param_spec_uint ("leave-opacity",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_ALPHA,
                                   g_param_spec_uint ("background-alpha",
                                                      NULL, NULL,
                                                      0, 100, 100,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_STYLE,
                                   g_param_spec_uint ("background-style",
                                                      NULL, NULL,
                                                      PANEL_BG_STYLE_NONE,
                                                      PANEL_BG_STYLE_IMAGE,
                                                      PANEL_BG_STYLE_NONE,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_COLOR,
                                   g_param_spec_boxed ("background-color",
                                                       NULL, NULL,
                                                       GDK_TYPE_COLOR,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_IMAGE,
                                   g_param_spec_string ("background-image",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BORDERS,
                                   g_param_spec_uint ("borders",
                                                      NULL, NULL,
                                                      0, G_MAXUINT,
                                                      PANEL_BORDER_NONE,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_COMPOSITED,
                                   g_param_spec_boolean ("composited",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}



static void
panel_base_window_init (PanelBaseWindow *window)
{
  GtkStyleContext *context;

  window->priv = G_TYPE_INSTANCE_GET_PRIVATE (window, PANEL_TYPE_BASE_WINDOW, PanelBaseWindowPrivate);

  window->is_composited = FALSE;
  window->background_alpha = 1.00;
  window->background_style = PANEL_BG_STYLE_NONE;
  window->background_image = NULL;
  window->background_color = NULL;

  window->priv->bg_image_cache = NULL;
  window->priv->enter_opacity = 1.00;
  window->priv->leave_opacity = 1.00;
  window->priv->borders = PANEL_BORDER_NONE;
  window->priv->active_timeout_id = 0;

  /* some wm require stick to show the window on all workspaces, on xfwm4
   * the type-hint already takes care of that */
  gtk_window_stick (GTK_WINDOW (window));

  /* set colormap */
  panel_base_window_screen_changed (GTK_WIDGET (window), NULL);

  /* set the panel class */
  context = gtk_widget_get_style_context (GTK_WIDGET (window));
  gtk_style_context_add_class (context, "panel");
  gtk_style_context_add_class (context, "xfce4-panel");
}



static void
panel_base_window_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PanelBaseWindow        *window = PANEL_BASE_WINDOW (object);
  PanelBaseWindowPrivate *priv = window->priv;
  GdkRGBA                *color;
  GdkRGBA                bg_color;
  GtkStyleContext        *ctx;

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

    case PROP_BACKGROUND_STYLE:
      g_value_set_uint (value, window->background_style);
      break;

    case PROP_BACKGROUND_COLOR:
      if (window->background_color != NULL)
        color = window->background_color;
      else
        {
          ctx = gtk_widget_get_style_context (GTK_WIDGET (window));
          gtk_style_context_get_background_color (ctx, GTK_STATE_NORMAL, &bg_color);
          color = &bg_color;
        }
      g_value_set_boxed (value, color);
      break;

    case PROP_BACKGROUND_IMAGE:
      g_value_set_string (value, window->background_image);
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
  PanelBgStyle            bg_style;

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
      panel_base_window_set_plugin_data (window,
          panel_base_window_set_plugin_background_alpha);
      break;

    case PROP_BACKGROUND_STYLE:
      bg_style = g_value_get_uint (value);
      if (window->background_style != bg_style)
        {
          window->background_style = bg_style;

          if (priv->bg_image_cache != NULL)
            {
              /* destroy old image cache */
              cairo_pattern_destroy (priv->bg_image_cache);
              priv->bg_image_cache = NULL;
            }

          /* send information to external plugins */
          if (window->background_style == PANEL_BG_STYLE_IMAGE
              && window->background_image != NULL)
            {
              panel_base_window_set_plugin_data (window,
                  panel_base_window_set_plugin_background_image);
            }
          else if (window->background_style == PANEL_BG_STYLE_NONE
                   || (window->background_style == PANEL_BG_STYLE_COLOR
                       && window->background_color != NULL))
            {
              panel_base_window_set_plugin_data (window,
                  panel_base_window_set_plugin_background_color);
            }

          /* resize to update border size too */
          gtk_widget_queue_resize (GTK_WIDGET (window));
        }
      break;

    case PROP_BACKGROUND_COLOR:
      if (window->background_color != NULL)
        gdk_color_free (window->background_color);
      window->background_color = g_value_dup_boxed (value);

      if (window->background_style == PANEL_BG_STYLE_COLOR)
        {
          panel_base_window_set_plugin_data (window,
              panel_base_window_set_plugin_background_color);
          gtk_widget_queue_draw (GTK_WIDGET (window));
        }
      break;

    case PROP_BACKGROUND_IMAGE:
      /* store new filename */
      g_free (window->background_image);
      window->background_image = g_value_dup_string (value);

      /* drop old cache */
      if (priv->bg_image_cache != NULL)
        {
          cairo_pattern_destroy (priv->bg_image_cache);
          priv->bg_image_cache = NULL;
        }

      if (window->background_style == PANEL_BG_STYLE_IMAGE)
        {
          panel_base_window_set_plugin_data (window,
              panel_base_window_set_plugin_background_image);
          gtk_widget_queue_draw (GTK_WIDGET (window));
        }
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
              priv->active_timeout_id = gdk_threads_add_timeout_seconds_full (
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
  PanelBaseWindow *window = PANEL_BASE_WINDOW (object);

  /* stop running marching ants timeout */
  if (window->priv->active_timeout_id != 0)
    g_source_remove (window->priv->active_timeout_id);

  /* release bg image data */
  g_free (window->background_image);
  if (window->priv->bg_image_cache != NULL)
    cairo_pattern_destroy (window->priv->bg_image_cache);
  if (window->background_color != NULL)
    gdk_color_free (window->background_color);

  (*G_OBJECT_CLASS (panel_base_window_parent_class)->finalize) (object);
}



static void
panel_base_window_screen_changed (GtkWidget *widget, GdkScreen *previous_screen)
{
  PanelBaseWindow *window = PANEL_BASE_WINDOW (widget);
  GdkVisual       *visual;
  GdkScreen       *screen;

  if (GTK_WIDGET_CLASS (panel_base_window_parent_class)->screen_changed != NULL)
    GTK_WIDGET_CLASS (panel_base_window_parent_class)->screen_changed (widget, previous_screen);

  /* set the rgba colormap if supported by the screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));
  visual = gdk_screen_get_rgba_visual (screen);
  if (visual != NULL)
    {
      gtk_widget_set_visual (widget, visual);
      window->is_composited = gtk_widget_is_composited (widget);
    }

   panel_debug (PANEL_DEBUG_BASE_WINDOW,
               "%p: rgba visual=%p, compositing=%s", window,
               visual, PANEL_DEBUG_BOOL (window->is_composited));
}



static gboolean
panel_base_window_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  const GdkRGBA          *color;
  GdkRGBA                 bg_rgba;
  GtkSymbolicColor       *literal;
  GtkSymbolicColor       *shade;
  PanelBaseWindow        *window = PANEL_BASE_WINDOW (widget);
  PanelBaseWindowPrivate *priv = window->priv;
  gdouble                 alpha;
  gdouble                 width = gtk_widget_get_allocated_width (widget);
  gdouble                 height = gtk_widget_get_allocated_height (widget);
  const gdouble           dashes[] = { 4.00, 4.00 };
  GTimeVal                timeval;
  GdkPixbuf              *pixbuf;
  GError                 *error = NULL;
  cairo_matrix_t          matrix = { 1, 0, 0, 1, 0, 0 }; /* identity matrix */
  GtkStyleContext        *ctx;

  if (!gtk_widget_is_drawable (widget))
    return FALSE;

  ctx = gtk_widget_get_style_context (widget);

  /* create cairo context and set some default properties */
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width (cr, 1.00);

  /* get background alpha */
  alpha = window->is_composited ? window->background_alpha : 1.00;

  if (window->background_style == PANEL_BG_STYLE_IMAGE)
    {
      if (G_LIKELY (priv->bg_image_cache != NULL))
        {
          if (G_UNLIKELY (priv->active_timeout_id != 0))
            cairo_matrix_init_translate (&matrix, -1, -1);

          cairo_set_source (cr, priv->bg_image_cache);
          cairo_pattern_set_matrix (priv->bg_image_cache, &matrix);
          cairo_paint (cr);
        }
      else if (window->background_image != NULL)
        {
          /* load the image in a pixbuf */
          pixbuf = gdk_pixbuf_new_from_file (window->background_image, &error);

          if (G_LIKELY (pixbuf != NULL))
            {
              gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
              g_object_unref (G_OBJECT (pixbuf));

              priv->bg_image_cache = cairo_get_source (cr);
              cairo_pattern_reference (priv->bg_image_cache);
              cairo_pattern_set_extend (priv->bg_image_cache, CAIRO_EXTEND_REPEAT);
              cairo_paint (cr);
            }
          else
            {
              /* print error message */
              g_warning ("Background image disabled, \"%s\" could not be loaded: %s",
                         window->background_image, error != NULL ? error->message : "No error");
              g_error_free (error);

              /* disable background image mode */
              window->background_style = PANEL_BG_STYLE_NONE;
            }
        }
    }
  else
    {
      /* get the background color */
      if (window->background_style == PANEL_BG_STYLE_COLOR
          && window->background_color != NULL)
        {
          color = window->background_color;
          panel_util_set_source_rgba (cr, color, alpha);
        }
      else
        {
          gtk_style_context_get_background_color (ctx, GTK_STATE_NORMAL, &bg_rgba);
          bg_rgba.alpha = alpha;
          gdk_cairo_set_source_rgba (cr, &bg_rgba);
        }
      cairo_paint (cr);
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
      cairo_rectangle (cr, 0.5, 0.5, width - 1, height - 1);
      cairo_stroke (cr);
    }
  else if (window->background_style == PANEL_BG_STYLE_NONE)
    {
      if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_BOTTOM | PANEL_BORDER_RIGHT))
        {
          /* use dark color for buttom and right line */
          gtk_style_context_get_background_color (ctx, GTK_STATE_NORMAL, &bg_rgba);
          literal = gtk_symbolic_color_new_literal (&bg_rgba);
          shade = gtk_symbolic_color_new_shade (literal, 0.7);
          gtk_symbolic_color_unref (literal);
          gtk_symbolic_color_resolve (shade, NULL, &bg_rgba);
          gtk_symbolic_color_unref (shade);
          bg_rgba.alpha = alpha;
          gdk_cairo_set_source_rgba (cr, &bg_rgba);

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_BOTTOM))
            {
              cairo_move_to (cr, 0.50, height - 1);
              cairo_rel_line_to (cr, width, 0.50);
            }

          if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_RIGHT))
            {
              cairo_move_to (cr, width - 1, 0.50);
              cairo_rel_line_to (cr, 0.50, height);
            }

          cairo_stroke (cr);
        }

      if (PANEL_HAS_FLAG (priv->borders, PANEL_BORDER_TOP | PANEL_BORDER_LEFT))
        {
          /* use light color for top and left line */
          gtk_style_context_get_background_color (ctx, GTK_STATE_NORMAL, &bg_rgba);
          literal = gtk_symbolic_color_new_literal (&bg_rgba);
          shade = gtk_symbolic_color_new_shade (literal, 1.3);
          gtk_symbolic_color_unref (literal);
          gtk_symbolic_color_resolve (shade, NULL, &bg_rgba);
          gtk_symbolic_color_unref (shade);
          bg_rgba.alpha = alpha;
          gdk_cairo_set_source_rgba (cr, &bg_rgba);

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

  return FALSE;
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

  gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_PRELIGHT, TRUE);

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
  gboolean         was_composited = window->is_composited;
  GdkWindow       *gdkwindow;
  GtkAllocation    allocation;

  /* set new compositing state */
  window->is_composited = gtk_widget_is_composited (widget);
  if (window->is_composited == was_composited)
    return;

  if (window->is_composited)
    gtk_window_set_opacity (GTK_WINDOW (widget), window->priv->leave_opacity);

  panel_debug (PANEL_DEBUG_BASE_WINDOW,
               "%p: compositing=%s", window,
               PANEL_DEBUG_BOOL (window->is_composited));

  /* clear cairo image cache */
  if (window->priv->bg_image_cache != NULL)
    {
      cairo_pattern_destroy (window->priv->bg_image_cache);
      window->priv->bg_image_cache = NULL;
    }

  if (window->is_composited != was_composited)
    g_object_notify (G_OBJECT (widget), "composited");

  /* make sure the entire window is redrawn */
  gdkwindow = gtk_widget_get_window (widget);
  if (gdkwindow != NULL)
    gdk_window_invalidate_rect (gdkwindow, NULL, TRUE);

  /* HACK: invalid the geometry, so the wm notices it */
  gtk_widget_get_allocation (widget, &allocation);
  gtk_window_move (GTK_WINDOW (window),
                   allocation.x,
                   allocation.y);
  gtk_widget_queue_resize (widget);
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
panel_base_window_set_plugin_data (PanelBaseWindow *window,
                                   GtkCallback      func)
{
  GtkWidget *itembar;

  itembar = gtk_bin_get_child (GTK_BIN (window));
  if (G_LIKELY (itembar != NULL))
    gtk_container_foreach (GTK_CONTAINER (itembar), func, window);
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
}



static void
panel_base_window_set_plugin_background_color (GtkWidget *widget,
                                               gpointer   user_data)
{
  PanelBaseWindow *window = PANEL_BASE_WINDOW (user_data);
  GdkRGBA         *color;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));
  panel_return_if_fail (PANEL_IS_BASE_WINDOW (user_data));

  /* send null if the style is not a bg color */
  color = window->background_style == PANEL_BG_STYLE_COLOR ? window->background_color : NULL;

  if (PANEL_IS_PLUGIN_EXTERNAL (widget))
    panel_plugin_external_set_background_color (PANEL_PLUGIN_EXTERNAL (widget), color);
}



static void
panel_base_window_set_plugin_background_image (GtkWidget *widget,
                                               gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));
  panel_return_if_fail (PANEL_IS_BASE_WINDOW (user_data));

  if (PANEL_IS_PLUGIN_EXTERNAL (widget))
    panel_plugin_external_set_background_image (PANEL_PLUGIN_EXTERNAL (widget),
        PANEL_BASE_WINDOW (user_data)->background_image);
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
  else if (window->background_style != PANEL_BG_STYLE_NONE)
    return PANEL_BORDER_NONE;

  return priv->borders;
}



void
panel_util_set_source_rgba (cairo_t        *cr,
                            const GdkRGBA  *color,
                            gdouble         alpha)
{
  panel_return_if_fail (alpha >= 0.00 && alpha <= 1.00);
  panel_return_if_fail (color != NULL);

  if (G_LIKELY (alpha == 1.00))
    cairo_set_source_rgb (cr, color->red,
                          color->green,
                          color->blue);
  else
    cairo_set_source_rgba (cr, color->red,
                           color->green,
                           color->blue, alpha);
}
