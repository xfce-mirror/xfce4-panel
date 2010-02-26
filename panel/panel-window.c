/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#include <panel/panel-private.h>
#include <panel/panel-window.h>
#include <panel/panel-glue.h>
#include <panel/panel-application.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-dbus-service.h>

#define HANDLE_SIZE       (8)
#define HANDLE_SPACING    (2)
#define HANDLE_SIZE_TOTAL ((HANDLE_SIZE + HANDLE_SPACING) * 2)
#define SNAP_DISTANCE     (10)
#define POPUP_DELAY       (225)
#define POPDOWN_DELAY     (350)
#define HIDDEN_PANEL_SIZE (2)
#define OFFSCREEN         (-9999)



static void panel_window_class_init (PanelWindowClass *klass);
static void panel_window_init (PanelWindow *window);
static void panel_window_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void panel_window_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void panel_window_finalize (GObject *object);
static void panel_window_realize (GtkWidget *widget);
static gboolean panel_window_expose_event (GtkWidget *widget, GdkEventExpose *event);
static gboolean panel_window_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean panel_window_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean panel_window_button_release_event (GtkWidget *widget, GdkEventButton *event);
static gboolean panel_window_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event);
static gboolean panel_window_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event);
static void panel_window_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void panel_window_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void panel_window_screen_changed (GtkWidget *widget, GdkScreen *previous_screen);
static void panel_window_paint_handle (PanelWindow *window,  gboolean start, GtkStateType state, cairo_t *rc);
static void panel_window_paint_borders (PanelWindow *window, GtkStateType state, cairo_t *rc);
static void panel_window_calculate_position (PanelWindow *window, gint width, gint height, gint *x, gint *y);
static void panel_window_working_area (PanelWindow  *window, gint root_x, gint root_y, GdkRectangle *dest);
static gboolean panel_window_struts_are_possible (PanelWindow *window, gint x, gint y, gint width, gint height);
static void panel_window_struts_update (PanelWindow  *window, gint x, gint y, gint width, gint height);
static void panel_window_set_colormap (PanelWindow *window);
static void panel_window_get_position (PanelWindow *window, gint *root_x, gint *root_y);
static void panel_window_set_borders (PanelWindow *window);
static void panel_window_set_autohide (PanelWindow *window, gboolean     autohide);
static void panel_window_menu_quit (gpointer boolean);
static void panel_window_menu_deactivate (GtkMenu *menu, PanelWindow *window);
static void panel_window_menu_popup (PanelWindow *window);

static void panel_window_set_plugin_active_panel (GtkWidget *widget, gpointer user_data);
static void panel_window_set_plugin_background_alpha (GtkWidget *widget, gpointer user_data);
static void panel_window_set_plugin_size (GtkWidget *widget, gpointer user_data);
static void panel_window_set_plugin_orientation (GtkWidget *widget, gpointer user_data);

enum
{
  PROP_0,
  PROP_HORIZONTAL,
  PROP_SIZE,
  PROP_LENGTH,
  PROP_LOCKED,
  PROP_X_OFFSET,
  PROP_Y_OFFSET,
  PROP_ENTER_OPACITY,
  PROP_LEAVE_OPACITY,
  PROP_SNAP_EDGE,
  PROP_BACKGROUND_ALPHA,
  PROP_SPAN_MONITORS,
  PROP_AUTOHIDE
};

enum
{
  SNAP_NONE,
  SNAP_START,
  SNAP_CENTER,
  SNAP_END
};

typedef enum
{
  DISABLED,
  BLOCKED,
  VISIBLE,
  POPUP_QUEUED,
  HIDDEN,
  POPDOWN_QUEUED,
  POPDOWN_QUEUED_SLOW
}
AutohideStatus;

enum
{
  STRUT_LEFT = 0,
  STRUT_RIGHT,
  STRUT_TOP,
  STRUT_BOTTOM,
  STRUT_LEFT_START_Y,
  STRUT_LEFT_END_Y,
  STRUT_RIGHT_START_Y,
  STRUT_RIGHT_END_Y,
  STRUT_TOP_START_X,
  STRUT_TOP_END_X,
  STRUT_BOTTOM_START_X,
  STRUT_BOTTOM_END_X,
  N_STRUTS
};

struct _PanelWindowClass
{
  GtkWindowClass __parent__;
};

struct _PanelWindow
{
  GtkWindow __parent__;

  /* last allocated size, for recentering */
  GtkAllocation        prev_allocation;

  /* snapping edge of the window */
  PanelWindowSnapEdge  snap_edge;

  /* the borders we're going to draw */
  PanelWindowBorders   borders;

  /* whether we should apply struts for this screen position */
  gint                 struts_possible;

  /* the last used struts for this window */
  gulong               struts[N_STRUTS];

  /* the last calculated panel working area */
  GdkRectangle         working_area;

  /* whether we span monitors */
  guint                span_monitors : 1;

  /* whether the panel has a rgba colormap */
  guint                is_composited : 1;

  /* whether the panel is locked */
  guint                locked : 1;

  /* when this is the active panel */
  guint                is_active_panel : 1;

  /* panel orientation */
  guint                horizontal : 1;

  /* panel size (px) and length (%) */
  guint                size;
  gdouble              length;

  /* autohide */
  AutohideStatus       autohide_status;
  guint                autohide_timer;
  gint                 autohide_block;

  /* the window we use to show during autohide */
  GtkWidget           *autohide_window;

  /* background alpha */
  gdouble              background_alpha;

  /* panel enter/leave opacity */
  gdouble              enter_opacity;
  gdouble              leave_opacity;

  /* variables for dragging the panel */
  guint                drag_motion : 1;
  gint                 drag_start_x;
  gint                 drag_start_y;
};



static GdkAtom cardinal_atom = GDK_NONE;
static GdkAtom net_wm_strut_atom = GDK_NONE;
static GdkAtom net_wm_strut_partial_atom = GDK_NONE;



G_DEFINE_TYPE (PanelWindow, panel_window, GTK_TYPE_WINDOW);



static void
panel_window_class_init (PanelWindowClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = panel_window_get_property;
  gobject_class->set_property = panel_window_set_property;
  gobject_class->finalize = panel_window_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_window_realize;
  gtkwidget_class->expose_event = panel_window_expose_event;
  gtkwidget_class->motion_notify_event = panel_window_motion_notify;
  gtkwidget_class->button_press_event = panel_window_button_press_event;
  gtkwidget_class->button_release_event = panel_window_button_release_event;
  gtkwidget_class->enter_notify_event = panel_window_enter_notify_event;
  gtkwidget_class->leave_notify_event = panel_window_leave_notify_event;
  gtkwidget_class->size_request = panel_window_size_request;
  gtkwidget_class->size_allocate = panel_window_size_allocate;
  gtkwidget_class->screen_changed = panel_window_screen_changed;

  /**
   * PanelWindow::orientation:
   *
   * The orientation of the panel window. This is used to sync the
   * panel orientation with that of the itembar, using exo bindings.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_HORIZONTAL,
                                   g_param_spec_boolean ("horizontal", NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_uint ("size", NULL, NULL,
                                                      16, 128, 48,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LENGTH,
                                   g_param_spec_uint ("length", NULL, NULL,
                                                      1, 100, 25,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LOCKED,
                                   g_param_spec_boolean ("locked", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_X_OFFSET,
                                   g_param_spec_uint ("x-offset", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_Y_OFFSET,
                                   g_param_spec_uint ("y-offset", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ENTER_OPACITY,
                                   g_param_spec_uint ("enter-opacity", NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LEAVE_OPACITY,
                                   g_param_spec_uint ("leave-opacity", NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_ALPHA,
                                   g_param_spec_uint ("background-alpha", NULL, NULL,
                                                      0, 100, 100,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_AUTOHIDE,
                                   g_param_spec_boolean ("autohide", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SPAN_MONITORS,
                                   g_param_spec_boolean ("span-monitors", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SNAP_EDGE,
                                   g_param_spec_uint ("snap-edge", NULL, NULL,
                                                      PANEL_SNAP_EGDE_NONE, PANEL_SNAP_EGDE_S, PANEL_SNAP_EGDE_NONE,
                                                      EXO_PARAM_READWRITE));

  /* initialize the atoms */
  cardinal_atom = gdk_atom_intern_static_string ("CARDINAL");
  net_wm_strut_atom = gdk_atom_intern_static_string ("_NET_WM_STRUT");
  net_wm_strut_partial_atom = gdk_atom_intern_static_string ("_NET_WM_STRUT_PARTIAL");
}



static void
panel_window_init (PanelWindow *window)
{
  GdkScreen *screen;

  /* set window properties */
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
  gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_gravity (GTK_WINDOW (window), GDK_GRAVITY_STATIC);

  /* init vars */
  window->is_composited = FALSE;
  window->drag_motion = FALSE;
  window->struts_possible = -1;
  window->size = 48;
  window->snap_edge = PANEL_SNAP_EGDE_NONE;
  window->borders = 0;
  window->span_monitors = FALSE;
  window->length = 0.25;
  window->horizontal = TRUE;
  window->background_alpha = 1.00;
  window->enter_opacity = 1.00;
  window->leave_opacity = 1.00;
  window->autohide_timer = 0;
  window->autohide_status = DISABLED;
  window->autohide_block = 0;
  window->autohide_window = NULL;
  window->is_active_panel = FALSE;

  /* set additional events we want to have */
  gtk_widget_add_events (GTK_WIDGET (window), GDK_BUTTON_PRESS_MASK);

  /* connect signal to monitor the compositor changes */
  g_signal_connect (G_OBJECT (window), "composited-changed", G_CALLBACK (panel_window_set_colormap), NULL);

  /* set the colormap */
  panel_window_set_colormap (window);

  /* get the window screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));

  /* connect screen update signals */
  g_signal_connect_swapped (G_OBJECT (screen), "size-changed", G_CALLBACK (panel_window_screen_changed), window);
#if GTK_CHECK_VERSION (2,14,0)
  g_signal_connect_swapped (G_OBJECT (screen), "monitors-changed", G_CALLBACK (panel_window_screen_changed), window);
#endif
}



static void
panel_window_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PanelWindow *window = PANEL_WINDOW (object);
  //gint         pos;

  switch (prop_id)
    {
      case PROP_HORIZONTAL:
        g_value_set_boolean (value, window->horizontal);
        break;

      case PROP_SIZE:
        g_value_set_uint (value, window->size);
        break;

      case PROP_LENGTH:
        g_value_set_uint (value,  rint (window->length * 100.00));
        break;

      case PROP_LOCKED:
        g_value_set_boolean (value, window->locked);
        break;

      case PROP_X_OFFSET:
        //panel_window_get_position (window, &pos, NULL);
        //g_value_set_uint (value, pos);
        g_value_set_uint (value, 0);
        break;

      case PROP_Y_OFFSET:
        //panel_window_get_position (window, NULL, &pos);
        //g_value_set_uint (value, pos);
        g_value_set_uint (value, 0);
        break;

      case PROP_ENTER_OPACITY:
        g_value_set_uint (value, rint (window->enter_opacity * 100.00));
        break;

      case PROP_LEAVE_OPACITY:
        g_value_set_uint (value, rint (window->leave_opacity * 100.00));
        break;

      case PROP_BACKGROUND_ALPHA:
        g_value_set_uint (value, rint (window->background_alpha * 100.00));
        break;

      case PROP_SNAP_EDGE:
        g_value_set_uint (value, window->snap_edge);
        break;

      case PROP_SPAN_MONITORS:
        g_value_set_boolean (value, window->span_monitors);
        break;

      case PROP_AUTOHIDE:
        g_value_set_boolean (value, !!(window->autohide_status != DISABLED));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
panel_window_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PanelWindow *window = PANEL_WINDOW (object);
  //gint         pos;

  switch (prop_id)
    {
      case PROP_HORIZONTAL:
        /* set whether the panel */
        window->horizontal = g_value_get_boolean (value);

        /* update all the panel plugins */
        gtk_container_foreach (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window))),
                               panel_window_set_plugin_orientation,
                               GUINT_TO_POINTER (window->horizontal ? GTK_ORIENTATION_HORIZONTAL:
                                                 GTK_ORIENTATION_VERTICAL));

        /* queue a resize */
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_SIZE:
        /* update the panel size */
        window->size = g_value_get_uint (value);

        /* update all the panel plugins */
        gtk_container_foreach (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window))),
                               panel_window_set_plugin_size,
                               GUINT_TO_POINTER (window->size));

        /* queue a resize */
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_LENGTH:
        /* set the new length */
        window->length = g_value_get_uint (value) / 100.00;

        /* update the border and resize */
        panel_window_set_borders (window);
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_LOCKED:
        /* set new lock value and resize */
        window->locked = g_value_get_boolean (value);
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_X_OFFSET:
        /* get window position */
        //gtk_window_get_position (GTK_WINDOW (window), NULL, &pos);
        //gtk_window_move (GTK_WINDOW (window), g_value_get_uint (value), pos);
        break;

      case PROP_Y_OFFSET:
        /* get window position */
        //gtk_window_get_position (GTK_WINDOW (window), &pos, NULL);
        //gtk_window_move (GTK_WINDOW (window), pos, g_value_get_uint (value));
        break;

      case PROP_ENTER_OPACITY:
        /* set the new enter opacity */
        window->enter_opacity = g_value_get_uint (value) / 100.00;
        break;

      case PROP_LEAVE_OPACITY:
        /* set the new leave opacity */
        window->leave_opacity = g_value_get_uint (value) / 100.00;

        /* set the autohide window opacity if created */
        if (window->autohide_window)
          gtk_window_set_opacity (GTK_WINDOW (window->autohide_window), window->leave_opacity);

        /* update the panel window opacity */
        gtk_window_set_opacity (GTK_WINDOW (window), window->leave_opacity);
        break;

      case PROP_BACKGROUND_ALPHA:
        /* set the new value and redraw the panel */
        window->background_alpha = g_value_get_uint (value) / 100.00;
        gtk_widget_queue_draw (GTK_WIDGET (window));

        /* update the external plugins */
        gtk_container_foreach (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window))),
                               panel_window_set_plugin_background_alpha,
                               GUINT_TO_POINTER (g_value_get_uint (value)));
        break;

      case PROP_SNAP_EDGE:
        /* set snap edge value */
        window->snap_edge = g_value_get_uint (value);

        /* update the window borders */
        panel_window_set_borders (window);

        /* queue a resize */
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_SPAN_MONITORS:
        /* store new value */
        window->span_monitors = g_value_get_boolean (value);

        /* update the working area */
        panel_window_working_area (window, -1, -1, &window->working_area);

        /* resize the panel */
        gtk_widget_queue_resize (GTK_WIDGET (window));
        break;

      case PROP_AUTOHIDE:
        panel_window_set_autohide (window, g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
panel_window_finalize (GObject *object)
{
  PanelWindow *window = PANEL_WINDOW (object);

  /* stop the autohide timeout */
  if (window->autohide_timer != 0)
    g_source_remove (window->autohide_timer);

  /* destroy autohide window */
  if (window->autohide_window != NULL)
    gtk_widget_destroy (window->autohide_window);

  (*G_OBJECT_CLASS (panel_window_parent_class)->finalize) (object);
}



static void
panel_window_realize (GtkWidget *widget)
{
  /* realize the window */
  (*GTK_WIDGET_CLASS (panel_window_parent_class)->realize) (widget);

  /* initialize the working area */
  panel_window_working_area (PANEL_WINDOW (widget), -1, -1, &PANEL_WINDOW (widget)->working_area);
}



static gboolean
panel_window_expose_event (GtkWidget      *widget,
                           GdkEventExpose *event)
{
  PanelWindow  *window = PANEL_WINDOW (widget);
  cairo_t      *cr;
  GdkColor     *color;
  GtkStateType  state = GTK_STATE_NORMAL;
  gdouble       alpha = window->is_composited ? window->background_alpha : 1.00;

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      /* create cairo context */
      cr = gdk_cairo_create (widget->window);

      /* clip the drawing area */
      gdk_cairo_rectangle (cr, &event->area);
      cairo_clip (cr);

      /* use another state when the panel is selected */
      if (G_UNLIKELY (window->is_active_panel))
        state = GTK_STATE_SELECTED;

      if (alpha < 1.00 || window->is_active_panel)
        {
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
        }

      /* paint handles */
      if (window->locked == FALSE)
        {
          panel_window_paint_handle (window, TRUE, state, cr);
          panel_window_paint_handle (window, FALSE, state, cr);
        }

      /* paint the panel borders */
      panel_window_paint_borders (window, state, cr);

      /* destroy cairo context */
      cairo_destroy (cr);
    }

  /* send expose event to child too */
  if (GTK_BIN (widget)->child)
    gtk_container_propagate_expose (GTK_CONTAINER (widget), GTK_BIN (widget)->child, event);

  return FALSE;
}



static guint
panel_window_motion_notify_snap (gint  value,
                                 gint  length,
                                 gint  start,
                                 gint  end,
                                 gint *return_value)
{
  gint tmp;

  /* snap in the center */
  tmp = start + ((end - start) - length) / 2;
  if (value >= tmp - SNAP_DISTANCE && value <= tmp + SNAP_DISTANCE)
    {
      *return_value = tmp;
      return SNAP_CENTER;
    }

  /* snap on the start */
  if (value >= start && value <= start + SNAP_DISTANCE)
    {
      *return_value = start;
      return SNAP_START;
    }

  /* snap on the end */
  tmp = end - length;
  if (value >= tmp - SNAP_DISTANCE && value <= tmp)
    {
      *return_value = tmp;
      return SNAP_END;
    }

  /* set value as return value */
  *return_value = value;

  return SNAP_NONE;
}



static gboolean
panel_window_motion_notify (GtkWidget      *widget,
                            GdkEventMotion *event)
{
  PanelWindow         *window = PANEL_WINDOW (widget);
  gint                 clamp_x, clamp_y;
  GdkScreen           *screen;
  gint                 window_width, window_height;
  gint                 window_x, window_y;
  GdkRectangle         area;
  gint                 snap_x, snap_y;
  guint                snap_horizontal;
  guint                snap_vertical;
  PanelWindowSnapEdge  snap_edge = PANEL_SNAP_EGDE_NONE;

  if (window->drag_motion)
    {
      /* get the pointer position and current screen */
      gdk_display_get_pointer (gtk_widget_get_display (widget), &screen, &window_x, &window_y, NULL);

      /* make sure the window is on the correct screen */
      gtk_window_set_screen (GTK_WINDOW (widget), screen);

      /* get the maximum panel area on this coordinate */
      panel_window_working_area (window, window_x, window_y, &area);

      /* convert to corner offset */
      window_x -= window->drag_start_x;
      window_y -= window->drag_start_y;

      /* get allocated window size, but make sure it fits in the maximum area */
      window_width = MIN (widget->allocation.width, area.width);
      window_height = MIN (widget->allocation.height, area.height);

      /* keep the panel inside the maximum area */
      clamp_x = CLAMP (window_x, area.x, area.x + area.width - window_width);
      clamp_y = CLAMP (window_y, area.y, area.y + area.height - window_height);

      /* update the drag coordinates, so dragging feels responsive when the user hits a screen edge */
      window->drag_start_x += window_x - clamp_x;
      window->drag_start_y += window_y - clamp_y;

      /* try to find snapping edges */
      snap_horizontal = panel_window_motion_notify_snap (clamp_x, window_width, area.x, area.x + area.width, &snap_x);
      snap_vertical = panel_window_motion_notify_snap (clamp_y, window_height, area.y, area.y + area.height, &snap_y);

      /* detect the snap mode */
      if (snap_horizontal == SNAP_START)
        snap_edge = PANEL_SNAP_EGDE_W + snap_vertical;
      else if (snap_horizontal == SNAP_END)
        snap_edge = PANEL_SNAP_EGDE_E + snap_vertical;
      else if (snap_horizontal == SNAP_CENTER && snap_vertical == SNAP_START)
        snap_edge = PANEL_SNAP_EGDE_NC;
      else if (snap_horizontal == SNAP_CENTER && snap_vertical == SNAP_END)
        snap_edge = PANEL_SNAP_EGDE_SC;
      else if (snap_horizontal == SNAP_NONE && snap_vertical == SNAP_START)
        snap_edge = PANEL_SNAP_EGDE_N;
      else if (snap_horizontal == SNAP_NONE && snap_vertical == SNAP_END)
        snap_edge = PANEL_SNAP_EGDE_S;

      /* when snapping succeeded, set the snap coordinates for visual feedback */
      if (snap_edge != PANEL_SNAP_EGDE_NONE)
        {
          clamp_x = snap_x;
          clamp_y = snap_y;
        }

      /* move and resize the window */
      gdk_window_move_resize (widget->window, clamp_x, clamp_y, window_width, window_height);

      /* if the snap edge changed, update the border */
      if (window->snap_edge != snap_edge)
        {
          /* set the new value */
          window->snap_edge = snap_edge;

          /* notify the property */
          g_object_notify (G_OBJECT (window), "snap-edge");

          /* update the borders */
          panel_window_set_borders (window);
        }

      return TRUE;
    }

  return FALSE;
}



static gboolean
panel_window_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  PanelWindow *window = PANEL_WINDOW (widget);
  GdkCursor   *cursor;
  guint        modifiers;

  /* get the modifiers */
  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  if (event->button == 1
      && event->window == widget->window
      && window->locked == FALSE
      && modifiers == 0)
    {
      /* set initial start coordinates */
      window->drag_start_x = event->x;
      window->drag_start_y = event->y;

      /* create a moving cursor */
      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget), GDK_FLEUR);

      /* try to drab the pointer */
      window->drag_motion = (gdk_pointer_grab (widget->window, FALSE,
                                               GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                                               NULL, cursor, event->time) == GDK_GRAB_SUCCESS);

      /* release the cursor */
      gdk_cursor_unref (cursor);

      return TRUE;
    }
  else if (event->button == 3 || (event->button == 1 && modifiers == GDK_CONTROL_MASK))
    {
      /* popup the panel menu */
      panel_window_menu_popup (window);

      return TRUE;
    }

  return FALSE;
}



static gboolean
panel_window_button_release_event (GtkWidget      *widget,
                                   GdkEventButton *event)
{
  PanelWindow *window = PANEL_WINDOW (widget);

  if (window->drag_motion)
    {
      /* unset the drag */
      window->drag_motion = FALSE;

      /* release the pointer */
      gdk_pointer_ungrab (event->time);

      /* update the borders */
      panel_window_set_borders (window);

      /* update working area, struts and reallocate */
      panel_window_screen_changed (widget, gtk_window_get_screen (GTK_WINDOW (widget)));

      /* update the plugins */
      /* TODO panel function for this */

      return TRUE;
    }

  return FALSE;
}



static gboolean
panel_window_autohide_timeout (gpointer user_data)
{
  PanelWindow *window = PANEL_WINDOW (user_data);

  panel_return_val_if_fail (window->autohide_status != DISABLED
                            && window->autohide_status != BLOCKED, FALSE);

  /* change status */
  if (window->autohide_status == POPDOWN_QUEUED || window->autohide_status == POPDOWN_QUEUED_SLOW)
    window->autohide_status = HIDDEN;
  else if (window->autohide_status == POPUP_QUEUED)
    window->autohide_status = VISIBLE;

  gtk_widget_queue_resize (GTK_WIDGET (window));

  return FALSE;
}



static void
panel_window_autohide_timeout_destroy (gpointer user_data)
{
  PANEL_WINDOW (user_data)->autohide_timer = 0;
}



static void
panel_window_autohide_queue (PanelWindow    *window,
                             AutohideStatus  status)
{
  guint delay;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* stop a running autohide timeout */
  if (window->autohide_timer != 0)
    g_source_remove (window->autohide_timer);

  /* set new autohide status */
  window->autohide_status = status;

  /* leave when the autohide is disabled */
  if (status == DISABLED || status == BLOCKED)
    {
      /* queue a resize to make sure everything is visible */
      gtk_widget_queue_resize (GTK_WIDGET (window));
    }
  else
    {
      /* get the delay */
      if (status == POPDOWN_QUEUED)
        delay = POPDOWN_DELAY;
      else if (status == POPDOWN_QUEUED_SLOW)
        delay = POPDOWN_DELAY * 3;
      else
        delay = POPUP_DELAY;

      /* schedule a new timeout */
      window->autohide_timer = g_timeout_add_full (G_PRIORITY_LOW, delay,
                                                   panel_window_autohide_timeout, window,
                                                   panel_window_autohide_timeout_destroy);
    }
}



static gboolean
panel_window_autohide_enter_notify_event (GtkWidget        *widget,
                                          GdkEventCrossing *event,
                                          PanelWindow      *window)
{
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  /* queue a hide timeout */
  panel_window_autohide_queue (window, POPUP_QUEUED);

  return TRUE;
}



static gboolean
panel_window_autohide_leave_notify_event (GtkWidget        *widget,
                                          GdkEventCrossing *event,
                                          PanelWindow      *window)
{
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  /* stop a running autohide timeout */
  if (window->autohide_timer != 0)
    g_source_remove (window->autohide_timer);

  /* update the status */
  if (window->autohide_status == POPUP_QUEUED)
    window->autohide_status = HIDDEN;

  return TRUE;
}



static GtkWidget *
panel_window_autohide_window (PanelWindow *window)
{
  GtkWidget *popup;

  /* create window */
  popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_gravity (GTK_WINDOW (popup), GDK_GRAVITY_STATIC);

  /* connect signals to monitor enter/leave events */
  g_signal_connect (G_OBJECT (popup), "enter-notify-event", G_CALLBACK (panel_window_autohide_enter_notify_event), window);
  g_signal_connect (G_OBJECT (popup), "leave-notify-event", G_CALLBACK (panel_window_autohide_leave_notify_event), window);

  /* put the window offscreen */
  gtk_window_move (GTK_WINDOW (popup), OFFSCREEN, OFFSCREEN);

  /* set window opacity */
  gtk_window_set_opacity (GTK_WINDOW (popup), window->leave_opacity);

  /* show the window */
  gtk_widget_show (popup);

  return popup;
}



static gboolean
panel_window_enter_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  PanelWindow *window = PANEL_WINDOW (widget);

  /* ignore event when entered from an inferior */
  if (event->detail == GDK_NOTIFY_INFERIOR)
    return FALSE;

  /* set the opacity (when they differ) */
  if (window->leave_opacity != window->enter_opacity)
    gtk_window_set_opacity (GTK_WINDOW (window), window->enter_opacity);

  /* stop a running autohide timeout */
  if (window->autohide_timer != 0)
    g_source_remove (window->autohide_timer);

  /* update autohide status */
  if (window->autohide_status == POPDOWN_QUEUED)
    window->autohide_status = VISIBLE;

  return FALSE;
}



static gboolean
panel_window_leave_notify_event (GtkWidget        *widget,
                                 GdkEventCrossing *event)
{
  PanelWindow *window = PANEL_WINDOW (widget);

  /* ignore event when left towards an inferior */
  if (event->detail == GDK_NOTIFY_INFERIOR)
    return FALSE;

  /* set the opacity (when they differ) */
  if (window->leave_opacity != window->enter_opacity)
    gtk_window_set_opacity (GTK_WINDOW (window), window->leave_opacity);

  /* stop a running autohide timeout */
  if (window->autohide_timer != 0)
    g_source_remove (window->autohide_timer);

  /* queue a new autohide time if needed */
  if (window->autohide_status > BLOCKED && window->snap_edge != PANEL_SNAP_EGDE_NONE)
    panel_window_autohide_queue (window, POPDOWN_QUEUED);

  return FALSE;
}



/**
 * panel_window_size_request:
 * @widget      : the panel window.
 * @requisition : the size we're requesting and receiving on
 *                allocation.
 *
 * In this function we request the panel size, this size must fit
 * on the screen and match all the settings. It's not nessecarily
 * that all the plugins fit (based on their requisition that is),
 * they have to respect the size we allocate, requesting is only
 * being kind to plugins (actually the itembar), and we're not ;).
 **/
static void
panel_window_size_request (GtkWidget      *widget,
                           GtkRequisition *requisition)
{
  PanelWindow    *window = PANEL_WINDOW (widget);
  GtkRequisition  child_requisition;
  gint            extra_width = 0, extra_height = 0;

  if (GTK_WIDGET_REALIZED (widget))
    {
      /* poke the itembar to request it's size */
      if (G_LIKELY (GTK_BIN (widget)->child))
        gtk_widget_size_request (GTK_BIN (widget)->child, &child_requisition);
      else
        child_requisition.width = child_requisition.height = 0;

      /* add the handle size */
      if (window->locked == FALSE)
        {
          if (window->horizontal)
            extra_width += HANDLE_SIZE_TOTAL;
          else
            extra_height += HANDLE_SIZE_TOTAL;
        }

      /* handle the borders */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_LEFT))
        extra_width++;

      /* handle the borders */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_RIGHT))
        extra_width++;

      /* handle the borders */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_TOP))
        extra_height++;

      /* handle the borders */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_BOTTOM))
        extra_height++;

      /* get the real allocated size */
      if (window->horizontal)
        {
          /* calculate the panel width (fits content, fits on the screen, and extands to user size) */
          requisition->width = CLAMP (child_requisition.width + extra_width,
                                      window->working_area.width * window->length,
                                      window->working_area.width);

          /* set height based on user setting */
          requisition->height = window->size + extra_height;
        }
      else
        {
          /* calculate the panel width (fits content, fits on the screen, and extands to user size) */
          requisition->height = CLAMP (child_requisition.height + extra_height,
                                       window->working_area.height * window->length,
                                       window->working_area.height);

          /* set width based on user setting */
          requisition->width = window->size + extra_width;
        }
    }
}



/**
 * panel_window_size_allocate:
 * @widget     : the panel window.
 * @allocation : the allocation.
 *
 * Here we position the window on the monitor/screen. The width and
 * height of the allocation cannot be changed, it's determend in
 * panel_window_size_request, so if it's wrong: fix things there.
 *
 * Because the panel window is a GtkWindow, the allocation x and y
 * are 0, which is fine for the child, but not for screen, so we
 * calculate the new screen position move and resize we window (at
 * once to avoid strange visual effects).
 *
 * The child allocation is basiclly the same, only a small change
 * to keep the handles free, when the panel is locked.
 **/
static void
panel_window_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  PanelWindow   *window = PANEL_WINDOW (widget);
  GtkAllocation  child_allocation;
  gint           root_x, root_y;
  gint           width = HIDDEN_PANEL_SIZE, height = HIDDEN_PANEL_SIZE;
  gint           hidden_root_x, hidden_root_y;

  if (GTK_WIDGET_REALIZED (widget))
    {
      /* set the widget allocation */
      widget->allocation = *allocation;

      /* get coordinates for the panel window */
      panel_window_calculate_position (window, allocation->width, allocation->height, &root_x, &root_y);

      /* handle hidden windows */
      if (window->autohide_status != DISABLED
          && window->autohide_window != NULL
          && window->snap_edge != PANEL_SNAP_EGDE_NONE)
        {
          if (window->autohide_status == VISIBLE
              || window->autohide_status == POPDOWN_QUEUED
              ||  window->autohide_status == POPDOWN_QUEUED_SLOW
              || window->autohide_status == BLOCKED)
            {
              /* put the hidden window offscreen */
              gtk_window_move (GTK_WINDOW (window->autohide_window), OFFSCREEN, OFFSCREEN);
            }
          else
            {
              /* init width and height */
              width = height = HIDDEN_PANEL_SIZE;

              /* get hidden panel size */
              switch (window->snap_edge)
                {
                  case PANEL_SNAP_EGDE_E:
                  case PANEL_SNAP_EGDE_EC:
                  case PANEL_SNAP_EGDE_W:
                  case PANEL_SNAP_EGDE_WC:
                    height = allocation->height;
                    break;

                  case PANEL_SNAP_EGDE_NC:
                  case PANEL_SNAP_EGDE_SC:
                  case PANEL_SNAP_EGDE_N:
                  case PANEL_SNAP_EGDE_S:
                    width = allocation->width;
                    break;

                  default:
                    if (window->horizontal)
                      width = allocation->width;
                    else
                      height = allocation->height;
                    break;
                }

              /* get coordinates for the hidden window */
              panel_window_calculate_position (window, width, height, &hidden_root_x, &hidden_root_y);

              /* position the hidden window */
              gtk_window_move (GTK_WINDOW (window->autohide_window), hidden_root_x, hidden_root_y);
              gtk_window_resize (GTK_WINDOW (window->autohide_window), width, height);

              /* put the panel window offscreen */
              root_x = root_y = OFFSCREEN;
            }
        }

      /* move and resize the panel window */
      gdk_window_move_resize (widget->window, root_x, root_y, allocation->width, allocation->height);

      if (GTK_BIN (widget)->child)
        {
          /* set the child allocation */
          child_allocation = *allocation;

          /* extract the border sizes from the allocation */
          if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_LEFT))
            {
              child_allocation.x++;
              child_allocation.width--;
            }

          if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_TOP))
            {
              child_allocation.y++;
              child_allocation.height--;
            }

          if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_RIGHT))
            child_allocation.width--;

          if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_BOTTOM))
            child_allocation.height--;

          /* keep free space for the handles if needed */
          if (window->locked == FALSE)
            {
              if (window->horizontal)
                {
                  child_allocation.width -= HANDLE_SIZE_TOTAL;
                  child_allocation.x += HANDLE_SIZE + HANDLE_SPACING;
                }
              else
                {
                  child_allocation.height -= HANDLE_SIZE_TOTAL;
                  child_allocation.y += HANDLE_SIZE + HANDLE_SPACING;
                }
            }

          /* keep things positive */
          child_allocation.width = MAX (0, child_allocation.width);
          child_allocation.height = MAX (0, child_allocation.height);

          /* allocate the child */
          gtk_widget_size_allocate (GTK_BIN (widget)->child, &child_allocation);
        }

      /* update struts if possible */
      if (window->struts_possible != 0)
        panel_window_struts_update (window, root_x, root_y, allocation->width, allocation->height);
    }

  /* update previous allocation */
  window->prev_allocation = *allocation;
}



static void
panel_window_screen_changed (GtkWidget *widget,
                             GdkScreen *previous_screen)
{
  GdkScreen   *screen;
  PanelWindow *window = PANEL_WINDOW (widget);

  panel_return_if_fail (PANEL_IS_WINDOW (widget));
  panel_return_if_fail (GDK_IS_SCREEN (previous_screen));

  /* get the new screen */
  screen = gtk_window_get_screen (GTK_WINDOW (widget));

  if (screen != previous_screen)
    {
      /* disconnect old screen changed handles */
      g_signal_handlers_disconnect_by_func (G_OBJECT (previous_screen), G_CALLBACK (gtk_widget_queue_resize), widget);

      /* connect new screen update signals */
      g_signal_connect_swapped (G_OBJECT (screen), "size-changed", G_CALLBACK (panel_window_screen_changed), widget);
#if GTK_CHECK_VERSION (2,14,0)
      g_signal_connect_swapped (G_OBJECT (screen), "monitors-changed", G_CALLBACK (panel_window_screen_changed), widget);
#endif
    }

  /* update the panel working area */
  panel_window_working_area (window, -1, -1, &window->working_area);

  /* check if struts are needed on the next resize */
  window->struts_possible = -1;

  /* queue a resize */
  gtk_widget_queue_resize (widget);
}



static void
panel_window_paint_handle (PanelWindow  *window,
                           gboolean      start,
                           GtkStateType  state,
                           cairo_t      *cr)
{
  GtkWidget     *widget = GTK_WIDGET (window);
  GtkAllocation *alloc  = &(widget->allocation);
  gint           x, y, width, height;
  gint           i, xx, yy;
  GdkColor      *color;
  gdouble        alpha;

  /* set the alpha (always show to handle for atleast 50%) */
  if (window->is_composited)
    alpha = 0.50 + window->background_alpha / 2.00;
  else
    alpha = 1.00;

  /* set initial numbers */
  x = alloc->x + 2;
  y = alloc->y + 2;

  if (window->horizontal)
    {
      width = HANDLE_SIZE / 3 * 3;
      height = alloc->height / 2;
      y += height / 2 - 1;
      x += (HANDLE_SIZE - width);

      /* draw handle on the right */
      if (!start)
        x += alloc->width - HANDLE_SIZE - 4;
    }
  else
    {
      height = HANDLE_SIZE / 3 * 3;
      width = alloc->width / 2;
      x += width / 2 - 1;
      y += (HANDLE_SIZE - height);

      /* draw handle on the bottom */
      if (!start)
        y += alloc->height - HANDLE_SIZE - 4;
    }

  /* draw handler */
  for (i = 2; i > 0; i--)
    {
      /* get the color for the job */
      if (i == 2)
        color = &(widget->style->light[state]);
      else
        color = &(widget->style->dark[state]);

      /* set source color */
      xfce_panel_cairo_set_source_rgba (cr, color, alpha);

      /* draw the dots */
      for (xx = 0; xx < width; xx += 3)
        for (yy = 0; yy < height; yy += 3)
          cairo_rectangle (cr, x + xx, y + yy, i, i);

      /* fill the rectangles */
      cairo_fill (cr);
    }
}



static void
panel_window_paint_borders (PanelWindow  *window,
                            GtkStateType  state,
                            cairo_t      *cr)
{
  GtkWidget     *widget = GTK_WIDGET (window);
  GtkAllocation *alloc = &(widget->allocation);
  GdkColor      *color;
  gdouble        alpha = window->is_composited ? window->background_alpha : 1.00;

  /* 1px line (1.5 results in a sharp 1px line) */
  cairo_set_line_width (cr, 1.5);

  /* possibly save some time */
  if (PANEL_HAS_FLAG (window->borders, (PANEL_BORDER_BOTTOM | PANEL_BORDER_RIGHT)))
    {
      /* dark color */
      color = &(widget->style->dark[state]);
      xfce_panel_cairo_set_source_rgba (cr, color, alpha);

      /* move the cursor the the bottom left */
      cairo_move_to (cr, alloc->x, alloc->y + alloc->height);

      /* bottom line */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_BOTTOM))
        cairo_rel_line_to (cr, alloc->width, 0);
      else
        cairo_rel_move_to (cr, alloc->width, 0);

      /* right line */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_RIGHT))
        cairo_rel_line_to (cr, 0, -alloc->height);
      else
        cairo_rel_move_to (cr, 0, -alloc->height);

      /* stroke this part */
      cairo_stroke (cr);
    }

  /* possibly save some time */
  if (PANEL_HAS_FLAG (window->borders, (PANEL_BORDER_TOP | PANEL_BORDER_LEFT)))
    {
      /* light color */
      color = &(widget->style->light[state]);
      xfce_panel_cairo_set_source_rgba (cr, color, alpha);

      /* move the cursor the the bottom left */
      cairo_move_to (cr, alloc->x, alloc->y + alloc->height);

      /* left line */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_LEFT))
        cairo_rel_line_to (cr, 0, -alloc->height);
      else
        cairo_rel_move_to (cr, 0, -alloc->height);

      /* top line */
      if (PANEL_HAS_FLAG (window->borders, PANEL_BORDER_TOP))
        cairo_rel_line_to (cr, alloc->width, 0);
      else
        cairo_rel_move_to (cr, alloc->width, 0);

      /* stroke the lines */
      cairo_stroke (cr);
    }
}



static void
panel_window_calculate_position (PanelWindow *window,
                                 gint         width,
                                 gint         height,
                                 gint        *x,
                                 gint        *y)
{
  gint          root_x, root_y;
  GdkRectangle *area = &window->working_area;

  /* get the panel window position */
  panel_window_get_position (window, &root_x, &root_y);

  /* x position of the window */
  switch (window->snap_edge)
    {
      /* left */
      case PANEL_SNAP_EGDE_W:
      case PANEL_SNAP_EGDE_NW:
      case PANEL_SNAP_EGDE_WC:
      case PANEL_SNAP_EGDE_SW:
        *x = area->x;
        break;

      /* right */
      case PANEL_SNAP_EGDE_E:
      case PANEL_SNAP_EGDE_NE:
      case PANEL_SNAP_EGDE_EC:
      case PANEL_SNAP_EGDE_SE:
        *x = area->x + area->width - width;
        break;

      /* center */
      case PANEL_SNAP_EGDE_NC:
      case PANEL_SNAP_EGDE_SC:
        *x = area->x + (area->width - width) / 2;
        break;

      /* other, recenter based on previous allocation */
      default:
        *x = root_x + (window->prev_allocation.width - width) / 2;
        *x = CLAMP (*x, area->x, area->x + area->width - width);
        break;
    }

  /* y position of the window */
  switch (window->snap_edge)
    {
      /* north */
      case PANEL_SNAP_EGDE_NE:
      case PANEL_SNAP_EGDE_NW:
      case PANEL_SNAP_EGDE_NC:
      case PANEL_SNAP_EGDE_N:
        *y = area->y;
        break;

      /* south */
      case PANEL_SNAP_EGDE_SE:
      case PANEL_SNAP_EGDE_SW:
      case PANEL_SNAP_EGDE_SC:
      case PANEL_SNAP_EGDE_S:
        *y = area->y + area->height - height;
        break;

      /* center */
      case PANEL_SNAP_EGDE_EC:
      case PANEL_SNAP_EGDE_WC:
        *y = area->y + (area->height - height) / 2;
        break;

      /* other, recenter based on previous allocation */
      default:
        *y = root_y + (window->prev_allocation.height - height) / 2;
        *y = CLAMP (*y, area->y, area->y + area->height - height);
        break;
    }
}



static void
panel_window_working_area (PanelWindow  *window,
                           gint          root_x,
                           gint          root_y,
                           GdkRectangle *dest)
{
  GdkScreen    *screen;
  gint          monitor_num;
  gint          n_monitors;
  GdkRectangle  geometry;
  gint          i;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (GDK_IS_WINDOW (GTK_WIDGET (window)->window));

  /* get valid coordinates if not set */
  if (root_x == -1 && root_y == -1)
    gtk_window_get_position (GTK_WINDOW (window), &root_x, &root_y);

  /* get panel screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));

  /* get the monitor number */
  monitor_num = gdk_screen_get_monitor_at_point (screen, root_x, root_y);

  /* get the root monitor geometry */
  gdk_screen_get_monitor_geometry (screen, monitor_num, dest);

  if (window->span_monitors)
    {
      /* get the number of monitors */
      n_monitors = gdk_screen_get_n_monitors (screen);

      /* only try to extend when there are more then 2 monitors */
      if (G_LIKELY (n_monitors > 1))
        {
          for (i = 0; i < n_monitors; i++)
            {
              /* skip the origional monitor */
              if (i == monitor_num)
                continue;

              /* get the monitor geometry */
              gdk_screen_get_monitor_geometry (screen, i, &geometry);

              g_message ("monitor %d, x=%d, y=%d, w=%d, h=%d", i, geometry.x, geometry.y, geometry.width, geometry.height);

              /* try to extend the dest geometry from the root coordinate's point of view */
              if (window->horizontal
                  && root_y >= geometry.y
                  && root_y <= geometry.y + geometry.height
                  && (dest->x + dest->width == geometry.x
                      || dest->x == geometry.x + geometry.width))
                {
                  /* extend the maximum area horizontally */
                  dest->x = MIN (dest->x, geometry.x);
                  dest->width += geometry.width;
                }
              else if (window->horizontal == FALSE
                       && root_x >= geometry.x
                       && root_x <= geometry.x + geometry.width
                       && (dest->y + dest->height == geometry.y
                           || dest->y == geometry.y + geometry.height))
                {
                  /* extend the maximum area vertically */
                  dest->y = MIN (dest->y, geometry.y);
                  dest->height += geometry.height;
                }
            }
        }
    }
}



static gboolean
panel_window_struts_are_possible (PanelWindow *window,
                                  gint         x,
                                  gint         y,
                                  gint         width,
                                  gint         height)
{
  GdkScreen    *screen;
  gint          n_monitors;
  gint          i;
  GdkRectangle  geometry;

  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  /* never set struts when we don't snap or when autohide is enabled */
  if (window->snap_edge == PANEL_SNAP_EGDE_NONE || window->autohide_status != DISABLED)
    return FALSE;

  /* always set struts on the following condition */
  if ((window->horizontal && y == 0) || (!window->horizontal && x == 0))
    return TRUE;

  /* get panel screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));

  /* get the number of monitors */
  n_monitors = gdk_screen_get_n_monitors (screen);

  if (G_LIKELY (n_monitors == 1))
    {
      /* don't set the struts when we're not at a screen edge */
      if ((window->horizontal && y + height != gdk_screen_get_height (screen))
          || (!window->horizontal && x + width != gdk_screen_get_width (screen)))
        return FALSE;
    }
  else
    {
      for (i = 0; i < n_monitors; i++)
        {
          /* get the monitor geometry */
          gdk_screen_get_monitor_geometry (screen, i, &geometry);

          if (window->horizontal
              && x >= geometry.x
              && x + width <= geometry.x + geometry.width
              && y + height < geometry.y + geometry.height)
            return FALSE;

          if (window->horizontal == FALSE
              && y >= geometry.y
              && y + height <= geometry.y + geometry.height
              && x + width < geometry.x + geometry.width)
            return FALSE;
        }
    }

  return TRUE;
}



static void
panel_window_struts_update (PanelWindow *window,
                            gint         x,
                            gint         y,
                            gint         width,
                            gint         height)
{
  gulong     struts[N_STRUTS] = { 0, };
  GdkScreen *screen;
  guint      i;
  gboolean   update_struts = FALSE;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (GDK_IS_WINDOW (GTK_WIDGET (window)->window));
  panel_return_if_fail (N_STRUTS == 12);
  panel_return_if_fail (cardinal_atom != GDK_NONE);

  if (G_UNLIKELY (window->struts_possible == -1))
    {
      /* check whether struts are possible, skip to apply if not */
      window->struts_possible = panel_window_struts_are_possible (window, x, y, width, height);

      /* struts are not possible, reset them only this time */
      if (window->struts_possible == 0)
        goto reset_only;
    }

  /* get the panel window screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));

  if (window->horizontal)
    {
      if (snap_edge_is_top (window->snap_edge))
        {
          /* the window is snapped on the top screen edge */
          struts[STRUT_TOP] = y + height;
          struts[STRUT_TOP_START_X] = x;
          struts[STRUT_TOP_END_X] = x + width;


        }
      else if (snap_edge_is_bottom (window->snap_edge))
        {
          /* the window is snapped on the bottom screen edge */
          struts[STRUT_BOTTOM] = gdk_screen_get_height (screen) - y;
          struts[STRUT_BOTTOM_START_X] = x;
          struts[STRUT_BOTTOM_END_X] = x + width;
        }
    }
  else /* vertical */
    {
      if (snap_edge_is_left (window->snap_edge))
        {
          /* the window is snapped on the left screen edge */
          struts[STRUT_LEFT] = x + width;
          struts[STRUT_LEFT_START_Y] = y;
          struts[STRUT_LEFT_END_Y] = y + height;
        }
      else if (snap_edge_is_right (window->snap_edge))
        {
          /* the window is snapped on the right screen edge */
          struts[STRUT_RIGHT] = gdk_screen_get_width (screen) - x;
          struts[STRUT_RIGHT_START_Y] = y;
          struts[STRUT_RIGHT_END_Y] = y + height;
        }
    }

  reset_only:

  for (i = 0; i < N_STRUTS; i++)
    {
      /* check if we need to update */
      if (struts[i] != window->struts[i])
        update_struts = TRUE;

      /* store new strut */
      window->struts[i] = struts[i];
    }

  if (update_struts)
    {
      /* don't crash on x errors */
      gdk_error_trap_push ();

      /* set the wm strut partial */
      gdk_property_change (GTK_WIDGET (window)->window, net_wm_strut_partial_atom,
                           cardinal_atom, 32, GDK_PROP_MODE_REPLACE, (guchar *) &struts, 12);

      /* set the wm strut */
      gdk_property_change (GTK_WIDGET (window)->window, net_wm_strut_atom,
                           cardinal_atom, 32, GDK_PROP_MODE_REPLACE, (guchar *) &struts, 4);

      /* release the trap push */
      gdk_error_trap_pop ();

#if 0
      gint         n = -1;
      const gchar *names1[] = { "left", "right", "top", "bottom" };
      const gchar *names2[] = { "y",    "y",     "x",    "x" };

      if (struts[STRUT_LEFT] != 0)
        n = STRUT_LEFT;
	    else if (struts[STRUT_RIGHT] != 0)
	      n = STRUT_RIGHT;
	    else if (struts[STRUT_TOP] != 0)
	      n = STRUT_TOP;
	    else if (struts[STRUT_BOTTOM] != 0)
	      n = STRUT_BOTTOM;

      if (n == -1)
        g_print ("Struts: All set to zero\n");
      else
        g_print ("Struts: %s = %ld, start_%s = %ld, end_%s = %ld\n", names1[n],
                 struts[n], names2[n], struts[4 + n * 2], names2[n], struts[5 + n * 2]);
#endif
    }
}



static void
panel_window_set_colormap (PanelWindow *window)
{
  GdkColormap *colormap = NULL;
  GdkScreen   *screen;
  gboolean     restore;
  GtkWidget   *widget = GTK_WIDGET (window);
  gint         root_x, root_y;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* whether the widget was previously visible */
  restore = GTK_WIDGET_REALIZED (widget);

  /* unrealize the window if needed */
  if (restore)
    {
      /* store the window position */
      gtk_window_get_position (GTK_WINDOW (window), &root_x, &root_y);

      /* reset the struts */
      if (window->struts_possible == 1)
        panel_window_struts_update (window, 0, 0, 0, 0);

      /* hide the widget */
      gtk_widget_hide (widget);
      gtk_widget_unrealize (widget);
    }

  /* set bool */
  window->is_composited = gtk_widget_is_composited (widget);

  /* get the screen */
  screen = gtk_window_get_screen (GTK_WINDOW (window));

  /* try to get the rgba colormap */
  if (window->is_composited)
    colormap = gdk_screen_get_rgba_colormap (screen);

  /* get the default colormap */
  if (colormap == NULL)
    {
      colormap = gdk_screen_get_rgb_colormap (screen);
      window->is_composited = FALSE;
    }

  /* set the colormap */
  if (colormap)
    gtk_widget_set_colormap (widget, colormap);

  /* restore the window */
  if (restore)
    {
      /* restore the position */
      gtk_window_move (GTK_WINDOW (window), root_x, root_y);

      /* show the widget again */
      gtk_widget_realize (widget);
      gtk_widget_show (widget);

      /* set the struts again */
      if (window->struts_possible == 1)
        panel_window_struts_update (window, root_x, root_y, widget->allocation.width, widget->allocation.height);
    }
}



static void
panel_window_get_position (PanelWindow *window,
                           gint        *root_x,
                           gint        *root_y)
{
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the window position of the visible window */
  if (G_UNLIKELY (window->autohide_window
      && (window->autohide_status == HIDDEN || window->autohide_status == POPUP_QUEUED)))
    gtk_window_get_position (GTK_WINDOW (window->autohide_window), root_x, root_y);
  else
    gtk_window_get_position (GTK_WINDOW (window), root_x, root_y);
}



static void
panel_window_set_borders (PanelWindow *window)
{
  PanelWindowBorders borders = 0;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  if (window->horizontal)
    {
      /* only attempt to show the side borders if we're not filling the area */
      if (window->length < 1.00)
        {
          /* show the left border if we don't snap to the left */
          if (snap_edge_is_left (window->snap_edge) == FALSE)
            PANEL_SET_FLAG (borders, PANEL_BORDER_LEFT);

          /* show the right border if we don't snap to the right */
          if (snap_edge_is_right (window->snap_edge) == FALSE)
            PANEL_SET_FLAG (borders, PANEL_BORDER_RIGHT);
        }

      /* show the top border if not snapped to the top */
      if (snap_edge_is_top (window->snap_edge) == FALSE)
        PANEL_SET_FLAG (borders, PANEL_BORDER_TOP);

      /* show the bottom border if not snapped to the bottom */
      if (snap_edge_is_bottom (window->snap_edge) == FALSE)
        PANEL_SET_FLAG (borders, PANEL_BORDER_BOTTOM);
    }
  else
    {
      /* only attempt to show the top borders if we're not filling the area */
      if (window->length < 1.00)
        {
          /* show the top border if we don't snap to the top */
          if (snap_edge_is_top (window->snap_edge) == FALSE)
            PANEL_SET_FLAG (borders, PANEL_BORDER_TOP);

          /* show the bottom border if we don't snap to the bottom */
          if (snap_edge_is_bottom (window->snap_edge) == FALSE)
            PANEL_SET_FLAG (borders, PANEL_BORDER_BOTTOM);
        }

      /* show the left border if not snapped to the left */
      if (snap_edge_is_left (window->snap_edge) == FALSE)
        PANEL_SET_FLAG (borders, PANEL_BORDER_LEFT);

      /* show the right border if not snapped to the right */
      if (snap_edge_is_right (window->snap_edge) == FALSE)
        PANEL_SET_FLAG (borders, PANEL_BORDER_RIGHT);
    }

  /* set the new value and queue a resize if needed */
  if (window->borders != borders)
    {
      /* set the new value */
      window->borders = borders;

      /* queue a resize */
      gtk_widget_queue_resize (GTK_WIDGET (window));
    }
}



static void
panel_window_set_autohide (PanelWindow *window,
                           gboolean     autohide)
{
  AutohideStatus status;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  if (G_LIKELY ((window->autohide_status != DISABLED) != autohide))
    {
      /* determ whether struts are possible on the next resize */
      window->struts_possible = -1;

      if (autohide)
        {
          /* create popup window if needed */
          if (window->autohide_window == NULL)
            window->autohide_window = panel_window_autohide_window (window);

          /* get the correct status */
          status = window->autohide_block == 0 ? POPDOWN_QUEUED_SLOW : BLOCKED;

          /* queue a popdown */
          panel_window_autohide_queue (window, status);
        }
      else
        {
          /* disable autohiding */
          panel_window_autohide_queue (window, DISABLED);

          /* destroy the autohide window */
          if (window->autohide_window)
            {
              gtk_widget_destroy (window->autohide_window);
              window->autohide_window = NULL;
            }
        }
    }
}



static void
panel_window_menu_quit (gpointer boolean)
{
  extern gboolean dbus_quit_with_restart;

  /* restart or quit */
  dbus_quit_with_restart = !!(GPOINTER_TO_UINT (boolean));

  /* quit main loop */
  gtk_main_quit ();
}



static void
panel_window_menu_deactivate (GtkMenu     *menu,
                              PanelWindow *window)
{
  panel_return_if_fail (GTK_IS_MENU (menu));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* thaw autohide block */
  panel_window_thaw_autohide (window);

  /* destroy the menu */
  g_object_unref (G_OBJECT (menu));
}



static void
panel_window_menu_popup (PanelWindow *window)
{
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *image;

  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* freeze autohide */
  panel_window_freeze_autohide (window);

  /* create menu */
  menu = gtk_menu_new ();

  /* sink the menu and add unref on deactivate */
  g_object_ref_sink (G_OBJECT (menu));
  g_signal_connect (G_OBJECT (menu), "deactivate", G_CALLBACK (panel_window_menu_deactivate), window);

  /* label */
  item = gtk_image_menu_item_new_with_label (_("Xfce Panel"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_set_sensitive (item, FALSE);
  gtk_widget_show (item);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* add new items */
  item = gtk_image_menu_item_new_with_mnemonic (_("Add _New Items..."));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_item_dialog_show), window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* customize panel */
  item = gtk_image_menu_item_new_with_mnemonic (_("Panel Pr_eferences..."));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_preferences_dialog_show), window);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* quit item */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_window_menu_quit), GUINT_TO_POINTER (0));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* restart item */
  item = gtk_image_menu_item_new_with_mnemonic (_("_Restart"));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (panel_window_menu_quit), GUINT_TO_POINTER (1));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show (image);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* about item */
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (panel_dialogs_show_about), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* popup the menu */
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  0, gtk_get_current_event_time ());
}



static void
panel_window_set_plugin_active_panel (GtkWidget *widget,
                                      gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  if (PANEL_IS_PLUGIN_EXTERNAL (widget))
    panel_plugin_external_set_active_panel (PANEL_PLUGIN_EXTERNAL (widget),
                                            !!GPOINTER_TO_UINT (user_data));
}



static void
panel_window_set_plugin_background_alpha (GtkWidget *widget,
                                          gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* we only have to send the alpha to external plugins */
  if (PANEL_IS_PLUGIN_EXTERNAL (widget))
    panel_plugin_external_set_background_alpha (PANEL_PLUGIN_EXTERNAL (widget),
                                                GPOINTER_TO_UINT (user_data));
}



static void
panel_window_set_plugin_size (GtkWidget *widget,
                              gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* set the new plugin size */
  xfce_panel_plugin_provider_set_size (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                       GPOINTER_TO_INT (user_data));
}



static void
panel_window_set_plugin_orientation (GtkWidget *widget,
                                     gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* set the new plugin orientation */
  xfce_panel_plugin_provider_set_orientation (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                              GPOINTER_TO_INT (user_data));
}



gboolean
panel_window_is_composited (PanelWindow *window)
{
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  return window->is_composited;
}



void
panel_window_set_active_panel (PanelWindow *window,
                               gboolean     active)
{
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  if (G_UNLIKELY (window->is_active_panel != active))
    {
      /* set new value */
      window->is_active_panel = !!active;

      /* queue a redraw */
      gtk_widget_queue_draw (GTK_WIDGET (window));

      /* poke all the plugins */
      gtk_container_foreach (GTK_CONTAINER (gtk_bin_get_child (GTK_BIN (window))),
                             panel_window_set_plugin_active_panel,
                             GUINT_TO_POINTER (active));
    }
}



void
panel_window_freeze_autohide (PanelWindow *window)
{
  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (window->autohide_block >= 0);

  /* increase autohide block counter */
  window->autohide_block++;

  /* block autohide */
  if (window->autohide_block == 1 && window->autohide_status != DISABLED)
    panel_window_autohide_queue (window, BLOCKED);
}



void
panel_window_thaw_autohide (PanelWindow *window)
{
  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (window->autohide_block > 0);

  /* decrease autohide block counter */
  window->autohide_block--;

  /* queue an autohide when needed */
  if (window->autohide_block == 0 && window->autohide_status != DISABLED)
    panel_window_autohide_queue (window, POPDOWN_QUEUED);
}
