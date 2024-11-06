/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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
#include "config.h"
#endif

#include "tasklist-widget.h"

#include "common/panel-debug.h"
#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4windowing/libxfce4windowing.h>
#include <libxfce4windowingui/libxfce4windowingui.h>

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <libxfce4windowing/xfw-x11.h>
// Wayland does not supply a window id.  The window pointer should work for our purposes.
#define tasklist_window_get_wid(window) \
  (xfw_windowing_get () == XFW_WINDOWING_X11 ? xfw_window_x11_get_xid (window) : ((gulong) window))
#else
#define tasklist_window_get_wid(window) ((gulong) window)
#endif

#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell.h>
#define tasklist_get_monitor(tasklist) \
  (gtk_layer_is_supported () ? gtk_layer_get_monitor (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (tasklist)))) \
                             : gdk_display_get_monitor_at_window (tasklist->display, gtk_widget_get_window (GTK_WIDGET (tasklist))))
#else
#define tasklist_get_monitor(tasklist) \
  gdk_display_get_monitor_at_window (tasklist->display, gtk_widget_get_window (GTK_WIDGET (tasklist)))
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif



#define MIN_MAX_BUTTON_SIZE (-1)
#define MAX_MAX_BUTTON_SIZE (G_MAXINT)
#define DEFAULT_MAX_BUTTON_SIZE (32)
#define MIN_MAX_BUTTON_LENGTH (-1)
#define MAX_MAX_BUTTON_LENGTH (G_MAXINT)
#define DEFAULT_MAX_BUTTON_LENGTH (200)
#define MIN_MIN_BUTTON_LENGTH (0)
#define MAX_MIN_BUTTON_LENGTH (G_MAXINT)
#define DEFAULT_MIN_BUTTON_LENGTH (DEFAULT_MAX_BUTTON_LENGTH)
#define MIN_MINIMIZED_ICON_LUCENCY (0)
#define MAX_MINIMIZED_ICON_LUCENCY (100)
#define DEFAULT_MINIMIZED_ICON_LUCENCY (50)
#define DEFAULT_ELLIPSIZE_MODE (PANGO_ELLIPSIZE_END)
#define MIN_MENU_MAX_WIDTH_CHARS (-1)
#define MAX_MENU_MAX_WIDTH_CHARS (G_MAXINT)
#define DEFAULT_MENU_MAX_WIDTH_CHARS (24)
#define ARROW_BUTTON_SIZE (20)
#define WIREFRAME_SIZE (5) /* same as xfwm4 */
#define DRAG_ACTIVATE_TIMEOUT (500)


/* locking helpers for tasklist->locked */
#define xfce_taskbar_lock(tasklist) \
  G_STMT_START { XFCE_TASKLIST (tasklist)->locked++; } \
  G_STMT_END
#define xfce_taskbar_unlock(tasklist) \
  G_STMT_START \
  { \
    if (XFCE_TASKLIST (tasklist)->locked > 0) \
      XFCE_TASKLIST (tasklist)->locked--; \
    else \
      panel_assert_not_reached (); \
  } \
  G_STMT_END
#define xfce_taskbar_is_locked(tasklist) (XFCE_TASKLIST (tasklist)->locked > 0)

#define xfce_tasklist_get_panel_plugin(tasklist) XFCE_PANEL_PLUGIN (gtk_widget_get_ancestor (GTK_WIDGET (tasklist), XFCE_TYPE_PANEL_PLUGIN))
#define xfce_tasklist_horizontal(tasklist) ((tasklist)->mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
#define xfce_tasklist_vertical(tasklist) ((tasklist)->mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
#define xfce_tasklist_deskbar(tasklist) ((tasklist)->mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
#define xfce_tasklist_filter_monitors(tasklist) (!(tasklist)->all_monitors && (tasklist)->n_monitors > 1)

static inline const gchar *
xfce_tasklist_app_get_name (XfwApplication *app)
{
  const gchar *name = xfw_application_get_name (app);
  if (xfce_str_is_empty (name))
    name = xfw_application_get_class_id (app);
  return name;
}



typedef enum _XfceTasklistSortOrder
{
  XFCE_TASKLIST_SORT_ORDER_TIMESTAMP, /* sort by unique_id */
  XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP, /* sort by group and then by timestamp */
  XFCE_TASKLIST_SORT_ORDER_TITLE, /* sort by window title */
  XFCE_TASKLIST_SORT_ORDER_GROUP_TITLE, /* sort by group and then by title */
  XFCE_TASKLIST_SORT_ORDER_DND, /* append and support dnd */

  XFCE_TASKLIST_SORT_ORDER_MIN = XFCE_TASKLIST_SORT_ORDER_TIMESTAMP,
  XFCE_TASKLIST_SORT_ORDER_MAX = XFCE_TASKLIST_SORT_ORDER_DND,
  XFCE_TASKLIST_SORT_ORDER_DEFAULT = XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP
} XfceTasklistSortOrder;

typedef enum _XfceTasklistMClick
{
  XFCE_TASKLIST_MIDDLE_CLICK_NOTHING, /* do nothing */
  XFCE_TASKLIST_MIDDLE_CLICK_CLOSE_WINDOW, /* close the window */
  XFCE_TASKLIST_MIDDLE_CLICK_MINIMIZE_WINDOW, /* minimize, never minimize with button 1 */
  XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE, /* launches a new instance of the window */

  XFCE_TASKLIST_MIDDLE_CLICK_MIN = XFCE_TASKLIST_MIDDLE_CLICK_NOTHING,
  XFCE_TASKLIST_MIDDLE_CLICK_MAX = XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE,
  XFCE_TASKLIST_MIDDLE_CLICK_DEFAULT = XFCE_TASKLIST_MIDDLE_CLICK_NOTHING
} XfceTasklistMClick;

enum
{
  PROP_0,
  PROP_GROUPING,
  PROP_INCLUDE_ALL_WORKSPACES,
  PROP_INCLUDE_ALL_MONITORS,
  PROP_FLAT_BUTTONS,
  PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE,
  PROP_SHOW_LABELS,
  PROP_SHOW_ONLY_MINIMIZED,
  PROP_SHOW_WIREFRAMES,
  PROP_SHOW_HANDLE,
  PROP_SHOW_TOOLTIPS,
  PROP_SORT_ORDER,
  PROP_WINDOW_SCROLLING,
  PROP_WRAP_WINDOWS,
  PROP_INCLUDE_ALL_BLINKING,
  PROP_MIDDLE_CLICK,
  PROP_LABEL_DECORATIONS
};

struct _XfceTasklist
{
  GtkContainer __parent__;

  /* lock counter */
  gint locked;

  /* the screen of this tasklist */
  XfwScreen *screen;
  XfwWorkspaceGroup *workspace_group;
  GdkDisplay *display;

  /* window children in the tasklist */
  GList *windows;

  /* windows we monitor, but that are excluded from the tasklist */
  GSList *skipped_windows;

  /* arrow button of the overflow menu */
  GtkWidget *arrow_button;

  /* applications of all the windows in the taskbar */
  GHashTable *apps;

  /* normal or iconbox style */
  guint show_labels : 1;

  /* size of the panel pluin */
  gint size;

  /* mode (orientation) of the tasklist */
  XfcePanelPluginMode mode;

  /* relief of the tasklist buttons */
  GtkReliefStyle button_relief;

  /* whether we show windows from all workspaces or
   * only the active workspace */
  guint all_workspaces : 1;

  /* whether we switch to another workspace when we try to
   * unminimize a window on another workspace */
  guint switch_workspace : 1;

  /* whether we only show monimized windows in the
   * tasklist */
  guint only_minimized : 1;

  /* number of rows of window buttons */
  gint nrows;

  /* switch window with the mouse wheel */
  guint window_scrolling : 1;
  guint wrap_windows : 1;

  /* whether we show blinking windows from all workspaces
   * or only the active workspace */
  guint all_blinking : 1;

  /* action to preform when middle clicking */
  XfceTasklistMClick middle_click;

  /* whether decorate labels when window is not visible */
  guint label_decorations : 1;

  /* whether we only show windows that are in the geometry of
   * the monitor the tasklist is on */
  guint all_monitors : 1;
  guint n_monitors;

  /* whether we show wireframes when hovering a button in
   * the tasklist */
  guint show_wireframes : 1;

  /* icon geometries update timeout */
  guint update_icon_geometries_id;

  /* idle monitor geometry update */
  guint update_monitor_geometry_id;

  /* button grouping */
  guint grouping : 1;

  /* sorting order of the buttons */
  XfceTasklistSortOrder sort_order;

  /* dummy properties */
  guint show_handle : 1;
  guint show_tooltips : 1;

#ifdef ENABLE_X11
  /* wireframe window */
  Window wireframe_window;
#endif

  /* gtk style properties */
  gint max_button_length;
  gint min_button_length;
  gint max_button_size;
  PangoEllipsizeMode ellipsize_mode;
  gint minimized_icon_lucency;
  gint menu_max_width_chars;

  gint n_windows;
};

typedef enum
{
  CHILD_TYPE_WINDOW,
  CHILD_TYPE_GROUP,
  CHILD_TYPE_OVERFLOW_MENU,
  CHILD_TYPE_GROUP_MENU
} XfceTasklistChildType;

typedef struct _XfceTasklistChild
{
  /* type of this button */
  XfceTasklistChildType type;

  /* pointer to the tasklist */
  XfceTasklist *tasklist;

  /* button widgets */
  GtkWidget *button;
  GtkWidget *box;
  GtkWidget *icon;
  GtkWidget *label;

  /* we use a surface for icon rendering so keep original pixbuf around */
  GdkPixbuf *pixbuf;

  /* drag motion window activate */
  guint motion_timeout_id;
  guint motion_timestamp;

  /* unique id for sorting by insert time,
   * simply increased for each new button */
  guint unique_id;

  /* last time this window was focused */
  gint64 last_focused;

  /* list of windows in case of a group button */
  GSList *windows;
  gint n_windows;

  /* xfw information */
  XfwWindow *window;
  XfwApplication *app;
} XfceTasklistChild;

static const GtkTargetEntry source_targets[] = {
  { "application/x-wnck-window-id", 0, 0 }
};



static void
xfce_tasklist_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec);
static void
xfce_tasklist_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec);
static void
xfce_tasklist_finalize (GObject *object);
static void
xfce_tasklist_get_preferred_length (GtkWidget *widget,
                                    gint *minimum_length,
                                    gint *natural_length);
static void
xfce_tasklist_get_preferred_width (GtkWidget *widget,
                                   gint *minimum_width,
                                   gint *natural_width);
static void
xfce_tasklist_get_preferred_height (GtkWidget *widget,
                                    gint *minimum_height,
                                    gint *natural_height);
static void
xfce_tasklist_size_allocate (GtkWidget *widget,
                             GtkAllocation *allocation);
static void
xfce_tasklist_style_updated (GtkWidget *widget);
static void
xfce_tasklist_realize (GtkWidget *widget);
static void
xfce_tasklist_unrealize (GtkWidget *widget);
static gboolean
xfce_tasklist_scroll_event (GtkWidget *widget,
                            GdkEventScroll *event);
static void
xfce_tasklist_remove (GtkContainer *container,
                      GtkWidget *widget);
static void
xfce_tasklist_forall (GtkContainer *container,
                      gboolean include_internals,
                      GtkCallback callback,
                      gpointer callback_data);
static GType
xfce_tasklist_child_type (GtkContainer *container);
static void
xfce_tasklist_arrow_button_toggled (GtkWidget *button,
                                    XfceTasklist *tasklist);
static void
xfce_tasklist_connect_screen (XfceTasklist *tasklist);
static void
xfce_tasklist_disconnect_screen (XfceTasklist *tasklist);
static gboolean
xfce_tasklist_configure_event (GtkWidget *widget,
                               GdkEvent *event,
                               XfceTasklist *tasklist);
static void
xfce_tasklist_active_window_changed (XfwScreen *screen,
                                     XfwWindow *previous_window,
                                     XfceTasklist *tasklist);
static void
xfce_tasklist_active_workspace_changed (XfwWorkspaceGroup *group,
                                        XfwWorkspace *previous_workspace,
                                        XfceTasklist *tasklist);
static void
xfce_tasklist_window_added (XfwScreen *screen,
                            XfwWindow *window,
                            XfceTasklist *tasklist);
static void
xfce_tasklist_window_removed (XfwScreen *screen,
                              XfwWindow *window,
                              XfceTasklist *tasklist);
static void
xfce_tasklist_viewports_changed (XfwWorkspaceGroup *group,
                                 XfceTasklist *tasklist);
static void
xfce_tasklist_button_state_changed (XfwWindow *window,
                                    XfwWindowState changed_state,
                                    XfwWindowState new_state,
                                    XfceTasklistChild *child);
static void
xfce_tasklist_skipped_windows_state_changed (XfwWindow *window,
                                             XfwWindowState changed_state,
                                             XfwWindowState new_state,
                                             XfceTasklist *tasklist);
static void
xfce_tasklist_sort (XfceTasklist *tasklist,
                    gboolean sort_groups);
static void
xfce_tasklist_group_button_sort (XfceTasklistChild *group_child);
static gboolean
xfce_tasklist_update_icon_geometries (gpointer data);
static void
xfce_tasklist_update_icon_geometries_destroyed (gpointer data);

/* wireframe */
#ifdef ENABLE_X11
static void
xfce_tasklist_wireframe_hide (XfceTasklist *tasklist);
static void
xfce_tasklist_wireframe_destroy (XfceTasklist *tasklist);
static void
xfce_tasklist_wireframe_update (XfceTasklist *tasklist,
                                XfceTasklistChild *child);
#endif

/* tasklist buttons */
static inline gboolean
xfce_tasklist_button_visible (XfceTasklistChild *child,
                              XfwWorkspace *active_ws);
static gint
xfce_tasklist_button_compare (gconstpointer child_a,
                              gconstpointer child_b,
                              gpointer user_data);
static GtkWidget *
xfce_tasklist_button_proxy_menu_item (XfceTasklistChild *child,
                                      gboolean allow_wireframe);
static gboolean
xfce_tasklist_button_activate (XfceTasklistChild *child,
                               guint32 timestamp);
static XfceTasklistChild *
xfce_tasklist_button_new (XfwWindow *window,
                          XfceTasklist *tasklist);

/* tasklist group buttons */
static void
xfce_tasklist_group_button_menu_close (GtkWidget *menuitem,
                                       XfceTasklistChild *child,
                                       guint32 time);
static gboolean
xfce_tasklist_group_button_button_draw (GtkWidget *widget,
                                        cairo_t *cr,
                                        XfceTasklistChild *group_child);
static void
xfce_tasklist_group_button_remove (XfceTasklistChild *group_child);
static void
xfce_tasklist_group_button_add_window (XfceTasklistChild *group_child,
                                       XfceTasklistChild *window_child);
static void
xfce_tasklist_group_button_icon_changed (XfwApplication *app,
                                         XfceTasklistChild *group_child);
static XfceTasklistChild *
xfce_tasklist_group_button_new (XfwApplication *app,
                                XfceTasklist *tasklist);

/* potential public functions */
static void
xfce_tasklist_set_include_all_workspaces (XfceTasklist *tasklist,
                                          gboolean all_workspaces);
static void
xfce_tasklist_set_include_all_monitors (XfceTasklist *tasklist,
                                        gboolean all_monitors);
static void
xfce_tasklist_set_button_relief (XfceTasklist *tasklist,
                                 GtkReliefStyle button_relief);
static void
xfce_tasklist_set_show_labels (XfceTasklist *tasklist,
                               gboolean show_labels);
static void
xfce_tasklist_set_show_only_minimized (XfceTasklist *tasklist,
                                       gboolean only_minimized);
static void
xfce_tasklist_set_show_wireframes (XfceTasklist *tasklist,
                                   gboolean show_wireframes);
static void
xfce_tasklist_set_label_decorations (XfceTasklist *tasklist,
                                     gboolean label_decorations);
static void
xfce_tasklist_set_grouping (XfceTasklist *tasklist,
                            gboolean grouping);


G_DEFINE_FINAL_TYPE (XfceTasklist, xfce_tasklist, GTK_TYPE_CONTAINER)



static void
xfce_tasklist_class_init (XfceTasklistClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_tasklist_get_property;
  gobject_class->set_property = xfce_tasklist_set_property;
  gobject_class->finalize = xfce_tasklist_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->get_preferred_width = xfce_tasklist_get_preferred_width;
  gtkwidget_class->get_preferred_height = xfce_tasklist_get_preferred_height;
  gtkwidget_class->size_allocate = xfce_tasklist_size_allocate;
  gtkwidget_class->style_updated = xfce_tasklist_style_updated;
  gtkwidget_class->realize = xfce_tasklist_realize;
  gtkwidget_class->unrealize = xfce_tasklist_unrealize;
  gtkwidget_class->scroll_event = xfce_tasklist_scroll_event;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = NULL;
  gtkcontainer_class->remove = xfce_tasklist_remove;
  gtkcontainer_class->forall = xfce_tasklist_forall;
  gtkcontainer_class->child_type = xfce_tasklist_child_type;

  g_object_class_install_property (gobject_class,
                                   PROP_GROUPING,
                                   g_param_spec_boolean ("grouping",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_INCLUDE_ALL_WORKSPACES,
                                   g_param_spec_boolean ("include-all-workspaces",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_INCLUDE_ALL_MONITORS,
                                   g_param_spec_boolean ("include-all-monitors",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_FLAT_BUTTONS,
                                   g_param_spec_boolean ("flat-buttons",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE,
                                   g_param_spec_boolean ("switch-workspace-on-unminimize",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_LABELS,
                                   g_param_spec_boolean ("show-labels",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_ONLY_MINIMIZED,
                                   g_param_spec_boolean ("show-only-minimized",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_WIREFRAMES,
                                   g_param_spec_boolean ("show-wireframes",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_HANDLE,
                                   g_param_spec_boolean ("show-handle",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_TOOLTIPS,
                                   g_param_spec_boolean ("show-tooltips",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SORT_ORDER,
                                   g_param_spec_uint ("sort-order",
                                                      NULL, NULL,
                                                      XFCE_TASKLIST_SORT_ORDER_MIN,
                                                      XFCE_TASKLIST_SORT_ORDER_MAX,
                                                      XFCE_TASKLIST_SORT_ORDER_DEFAULT,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_WINDOW_SCROLLING,
                                   g_param_spec_boolean ("window-scrolling",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_WRAP_WINDOWS,
                                   g_param_spec_boolean ("wrap-windows",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_INCLUDE_ALL_BLINKING,
                                   g_param_spec_boolean ("include-all-blinking",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_MIDDLE_CLICK,
                                   g_param_spec_uint ("middle-click",
                                                      NULL, NULL,
                                                      XFCE_TASKLIST_MIDDLE_CLICK_MIN,
                                                      XFCE_TASKLIST_MIDDLE_CLICK_MAX,
                                                      XFCE_TASKLIST_MIDDLE_CLICK_DEFAULT,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_LABEL_DECORATIONS,
                                   g_param_spec_boolean ("label-decorations",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("max-button-length",
                                                             NULL,
                                                             "The maximum length of a window button",
                                                             MIN_MAX_BUTTON_LENGTH, MAX_MAX_BUTTON_LENGTH,
                                                             DEFAULT_MAX_BUTTON_LENGTH,
                                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("min-button-length",
                                                             NULL,
                                                             "The minimum length of a window button",
                                                             MIN_MIN_BUTTON_LENGTH, MAX_MIN_BUTTON_LENGTH,
                                                             DEFAULT_MIN_BUTTON_LENGTH,
                                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("max-button-size",
                                                             NULL,
                                                             "The maximum size of a window button",
                                                             MIN_MAX_BUTTON_SIZE, MAX_MAX_BUTTON_SIZE,
                                                             DEFAULT_MAX_BUTTON_SIZE,
                                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_enum ("ellipsize-mode",
                                                              NULL,
                                                              "The ellipsize mode used for the button label",
                                                              PANGO_TYPE_ELLIPSIZE_MODE,
                                                              DEFAULT_ELLIPSIZE_MODE,
                                                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("minimized-icon-lucency",
                                                             NULL,
                                                             "Lucent percentage of minimized icons",
                                                             MIN_MINIMIZED_ICON_LUCENCY, MAX_MINIMIZED_ICON_LUCENCY,
                                                             DEFAULT_MINIMIZED_ICON_LUCENCY,
                                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("menu-max-width-chars",
                                                             NULL,
                                                             "Maximum chars in the overflow menu labels",
                                                             MIN_MENU_MAX_WIDTH_CHARS, MAX_MENU_MAX_WIDTH_CHARS,
                                                             DEFAULT_MENU_MAX_WIDTH_CHARS,
                                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}



static void
xfce_tasklist_init (XfceTasklist *tasklist)
{
  GtkStyleContext *context;

  gtk_widget_set_has_window (GTK_WIDGET (tasklist), FALSE);

  tasklist->locked = 0;
  tasklist->screen = NULL;
  tasklist->windows = NULL;
  tasklist->skipped_windows = NULL;
  tasklist->mode = XFCE_PANEL_PLUGIN_MODE_HORIZONTAL;
  tasklist->nrows = 1;
  tasklist->all_workspaces = FALSE;
  tasklist->button_relief = GTK_RELIEF_NORMAL;
  tasklist->switch_workspace = TRUE;
  tasklist->only_minimized = FALSE;
  tasklist->show_labels = TRUE;
  tasklist->show_wireframes = FALSE;
  tasklist->show_handle = TRUE;
  tasklist->show_tooltips = TRUE;
  tasklist->all_monitors = TRUE;
  tasklist->n_monitors = 0;
  tasklist->window_scrolling = TRUE;
  tasklist->wrap_windows = FALSE;
  tasklist->all_blinking = TRUE;
  tasklist->middle_click = XFCE_TASKLIST_MIDDLE_CLICK_DEFAULT;
  tasklist->label_decorations = FALSE;
#ifdef ENABLE_X11
  tasklist->wireframe_window = 0;
#endif
  tasklist->update_icon_geometries_id = 0;
  tasklist->update_monitor_geometry_id = 0;
  tasklist->max_button_length = DEFAULT_MAX_BUTTON_LENGTH;
  tasklist->min_button_length = DEFAULT_MIN_BUTTON_LENGTH;
  tasklist->max_button_size = DEFAULT_MAX_BUTTON_SIZE;
  tasklist->minimized_icon_lucency = DEFAULT_MINIMIZED_ICON_LUCENCY;
  tasklist->ellipsize_mode = DEFAULT_ELLIPSIZE_MODE;
  tasklist->grouping = FALSE;
  tasklist->sort_order = XFCE_TASKLIST_SORT_ORDER_DEFAULT;
  tasklist->menu_max_width_chars = DEFAULT_MENU_MAX_WIDTH_CHARS;

  /* add style class for the tasklist widget */
  context = gtk_widget_get_style_context (GTK_WIDGET (tasklist));
  gtk_style_context_add_class (context, "tasklist");

  /* widgets for the overflow menu */
  /* TODO support drag-motion and drag-leave */
  tasklist->arrow_button = xfce_arrow_button_new (GTK_ARROW_DOWN);
  gtk_widget_set_parent (tasklist->arrow_button, GTK_WIDGET (tasklist));
  gtk_widget_set_name (tasklist->arrow_button, "panel-tasklist-arrow");
  gtk_button_set_relief (GTK_BUTTON (tasklist->arrow_button), tasklist->button_relief);
  g_signal_connect (G_OBJECT (tasklist->arrow_button), "toggled",
                    G_CALLBACK (xfce_tasklist_arrow_button_toggled), tasklist);
  gtk_widget_show (tasklist->arrow_button);
}



static void
xfce_tasklist_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);

  switch (prop_id)
    {
    case PROP_GROUPING:
      g_value_set_boolean (value, tasklist->grouping);
      break;

    case PROP_INCLUDE_ALL_WORKSPACES:
      g_value_set_boolean (value, tasklist->all_workspaces);
      break;

    case PROP_INCLUDE_ALL_MONITORS:
      g_value_set_boolean (value, tasklist->all_monitors);
      break;

    case PROP_FLAT_BUTTONS:
      g_value_set_boolean (value, !!(tasklist->button_relief == GTK_RELIEF_NONE));
      break;

    case PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE:
      g_value_set_boolean (value, tasklist->switch_workspace);
      break;

    case PROP_SHOW_LABELS:
      g_value_set_boolean (value, tasklist->show_labels);
      break;

    case PROP_SHOW_ONLY_MINIMIZED:
      g_value_set_boolean (value, tasklist->only_minimized);
      break;

    case PROP_SHOW_WIREFRAMES:
      g_value_set_boolean (value, tasklist->show_wireframes);
      break;

    case PROP_SHOW_HANDLE:
      g_value_set_boolean (value, tasklist->show_handle);
      break;

    case PROP_SHOW_TOOLTIPS:
      g_value_set_boolean (value, tasklist->show_tooltips);
      break;

    case PROP_SORT_ORDER:
      g_value_set_uint (value, tasklist->sort_order);
      break;

    case PROP_WINDOW_SCROLLING:
      g_value_set_boolean (value, tasklist->window_scrolling);
      break;

    case PROP_WRAP_WINDOWS:
      g_value_set_boolean (value, tasklist->wrap_windows);
      break;

    case PROP_INCLUDE_ALL_BLINKING:
      g_value_set_boolean (value, tasklist->all_blinking);
      break;

    case PROP_MIDDLE_CLICK:
      g_value_set_uint (value, tasklist->middle_click);
      break;

    case PROP_LABEL_DECORATIONS:
      g_value_set_boolean (value, tasklist->label_decorations);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_tasklist_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);
  XfceTasklistSortOrder sort_order;

  switch (prop_id)
    {
    case PROP_GROUPING:
      xfce_tasklist_set_grouping (tasklist, g_value_get_boolean (value));
      break;

    case PROP_INCLUDE_ALL_WORKSPACES:
      xfce_tasklist_set_include_all_workspaces (tasklist, g_value_get_boolean (value));
      break;

    case PROP_INCLUDE_ALL_MONITORS:
      xfce_tasklist_set_include_all_monitors (tasklist, g_value_get_boolean (value));
      break;

    case PROP_FLAT_BUTTONS:
      xfce_tasklist_set_button_relief (tasklist,
                                       g_value_get_boolean (value) ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);
      break;

    case PROP_SHOW_LABELS:
      xfce_tasklist_set_show_labels (tasklist, g_value_get_boolean (value));
      break;

    case PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE:
      tasklist->switch_workspace = g_value_get_boolean (value);
      break;

    case PROP_SHOW_ONLY_MINIMIZED:
      xfce_tasklist_set_show_only_minimized (tasklist, g_value_get_boolean (value));
      break;

    case PROP_SHOW_WIREFRAMES:
      xfce_tasklist_set_show_wireframes (tasklist, WINDOWING_IS_X11 () && g_value_get_boolean (value));
      break;

    case PROP_SHOW_HANDLE:
      tasklist->show_handle = g_value_get_boolean (value);
      break;

    case PROP_SHOW_TOOLTIPS:
      tasklist->show_tooltips = g_value_get_boolean (value);
      break;

    case PROP_SORT_ORDER:
      sort_order = g_value_get_uint (value);
      if (tasklist->sort_order != sort_order)
        {
          tasklist->sort_order = sort_order;
          xfce_tasklist_sort (tasklist, TRUE);
        }
      break;

    case PROP_WINDOW_SCROLLING:
      tasklist->window_scrolling = g_value_get_boolean (value);
      break;

    case PROP_WRAP_WINDOWS:
      tasklist->wrap_windows = g_value_get_boolean (value);
      break;

    case PROP_INCLUDE_ALL_BLINKING:
      tasklist->all_blinking = g_value_get_boolean (value);
      break;

    case PROP_MIDDLE_CLICK:
      tasklist->middle_click = g_value_get_uint (value);
      break;

    case PROP_LABEL_DECORATIONS:
      xfce_tasklist_set_label_decorations (tasklist, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_tasklist_finalize (GObject *object)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);

  /* data that should already be freed when disconnecting the screen */
  panel_return_if_fail (tasklist->windows == NULL);
  panel_return_if_fail (tasklist->skipped_windows == NULL);
  panel_return_if_fail (tasklist->screen == NULL);

  /* stop pending timeouts */
  if (tasklist->update_icon_geometries_id != 0)
    g_source_remove (tasklist->update_icon_geometries_id);
  if (tasklist->update_monitor_geometry_id != 0)
    g_source_remove (tasklist->update_monitor_geometry_id);

#ifdef ENABLE_X11
  /* destroy the wireframe window */
  xfce_tasklist_wireframe_destroy (tasklist);
#endif

  (*G_OBJECT_CLASS (xfce_tasklist_parent_class)->finalize) (object);
}



static void
xfce_tasklist_get_preferred_width (GtkWidget *widget,
                                   gint *minimum_width,
                                   gint *natural_width)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  if (xfce_tasklist_horizontal (tasklist))
    {
      xfce_tasklist_get_preferred_length (widget, minimum_width, natural_width);
    }
  else
    {
      if (minimum_width != NULL)
        *minimum_width = tasklist->size;

      if (natural_width != NULL)
        *natural_width = tasklist->size;
    }
}



static void
xfce_tasklist_get_preferred_height (GtkWidget *widget,
                                    gint *minimum_height,
                                    gint *natural_height)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  if (!xfce_tasklist_horizontal (tasklist))
    {
      xfce_tasklist_get_preferred_length (widget, minimum_height, natural_height);
    }
  else
    {
      if (minimum_height != NULL)
        *minimum_height = tasklist->size;

      if (natural_height != NULL)
        *natural_height = tasklist->size;
    }
}



static void
xfce_tasklist_get_preferred_length (GtkWidget *widget,
                                    gint *minimum_length,
                                    gint *natural_length)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);
  gint rows, cols;
  gint n_windows;
  GtkRequisition child_req;
  gint length = 0;
  GList *li;
  XfceTasklistChild *child;
  gint child_size = tasklist->size / tasklist->nrows;
  gint child_length = 0;

  for (li = tasklist->windows, n_windows = 0; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button))
        {
          gtk_widget_get_preferred_size (child->button, NULL, &child_req);
          child_length = MAX (child_length, xfce_tasklist_horizontal (tasklist) ? child_req.width : child_req.height);
          if (child->type == CHILD_TYPE_GROUP_MENU)
            continue;

          n_windows++;
        }
    }

  tasklist->n_windows = n_windows;

  if (n_windows != 0)
    {
      rows = MAX (tasklist->nrows, 1);
      if (tasklist->show_labels)
        {
          rows = MAX (rows, tasklist->size / tasklist->max_button_size);
          child_size = MIN (child_size, tasklist->max_button_size);
          child_length = CLAMP (child_length, tasklist->min_button_length, tasklist->max_button_length);
        }

      cols = n_windows / rows;
      if (cols * rows < n_windows)
        cols++;

      if (tasklist->show_labels)
        {
          if (xfce_tasklist_deskbar (tasklist))
            length = child_size * n_windows;
          else
            length = cols * child_length;
        }
      else
        length = (tasklist->size / rows) * cols;
    }

  /* set the requested sizes */
  if (natural_length != NULL)
    *natural_length = length;

  if (minimum_length != NULL)
    *minimum_length = (n_windows == 0) ? 0 : ARROW_BUTTON_SIZE;
}



static gint
xfce_tasklist_size_sort_window (gconstpointer a,
                                gconstpointer b)
{
  const XfceTasklistChild *child_a = a;
  const XfceTasklistChild *child_b = b;
  glong diff;

  diff = child_a->last_focused - child_b->last_focused;
  return CLAMP (diff, -1, 1);
}



static void
xfce_tasklist_size_layout (XfceTasklist *tasklist,
                           GtkAllocation *alloc,
                           gint *n_rows,
                           gint *n_cols,
                           gint *arrow_position)
{
  gint rows;
  gint min_button_length;
  gint cols;
  GSList *windows_scored = NULL, *lp;
  GList *li;
  XfceTasklistChild *child;
  gint max_button_length;
  gint n_buttons;
  gint n_buttons_target;

  /* if we're in deskbar mode, there are no columns */
  if (xfce_tasklist_deskbar (tasklist) && tasklist->show_labels)
    rows = 1;
  else if (tasklist->show_labels)
    rows = MAX (tasklist->nrows, tasklist->size / tasklist->max_button_size);
  else
    rows = tasklist->nrows;

  if (rows < 1)
    rows = 1;

  cols = tasklist->n_windows / rows;
  if (cols * rows < tasklist->n_windows)
    cols++;

  if (xfce_tasklist_deskbar (tasklist) && tasklist->show_labels)
    min_button_length = MIN (alloc->height / tasklist->nrows, tasklist->max_button_size);
  else if (!tasklist->show_labels)
    min_button_length = alloc->height / tasklist->nrows;
  else
    min_button_length = MIN (tasklist->min_button_length, tasklist->max_button_length / 4);

  *arrow_position = -1; /* not visible */

  /* unset overflow items, we decide about that again
   * later */
  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (child->type == CHILD_TYPE_OVERFLOW_MENU)
        child->type = CHILD_TYPE_WINDOW;
    }

  if (min_button_length * cols <= alloc->width)
    {
      /* all the windows seem to fit */
      *n_rows = rows;
      *n_cols = cols;
    }
  else
    {
      /* we need to group something, first create a list with the
       * windows most suitable for grouping at the beginning that are
       * (should be) currently visible */
      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;
          if (child->type == CHILD_TYPE_WINDOW && gtk_widget_get_visible (child->button))
            {
              windows_scored = g_slist_insert_sorted (windows_scored, child,
                                                      xfce_tasklist_size_sort_window);
            }
        }

      if (xfce_tasklist_deskbar (tasklist) || !tasklist->show_labels)
        max_button_length = min_button_length;
      else
        max_button_length = tasklist->max_button_length;

      n_buttons = tasklist->n_windows;
      /* Matches the existing behavior (with a bug fix) */
      /* n_buttons_target = MIN ((alloc->width - ARROW_BUTTON_SIZE) / min_button_length * rows,          *
       *                         (((alloc->width - ARROW_BUTTON_SIZE) / max_button_length) + 1) * rows); */

      /* Perhaps a better behavior (tries to display more buttons on the panel, */
      /* yet still within the specified limits) */
      n_buttons_target = (alloc->width - ARROW_BUTTON_SIZE) / min_button_length * rows;

      /* we now push the windows with the lowest score in the
       * overflow menu */
      if (n_buttons > n_buttons_target)
        {
          panel_debug (PANEL_DEBUG_TASKLIST,
                       "Putting %d windows in overflow menu",
                       n_buttons - n_buttons_target);

          for (lp = windows_scored;
               n_buttons > n_buttons_target && lp != NULL;
               lp = lp->next, n_buttons--)
            {
              child = lp->data;
              child->type = CHILD_TYPE_OVERFLOW_MENU;
            }

          /* Try to position the arrow widget at the end of the allocation area  *
           * if that's impossible (because buttons cannot be expanded enough)    *
           * position it just after the buttons.                                 */
          *arrow_position = MIN (alloc->width - ARROW_BUTTON_SIZE,
                                 n_buttons_target * max_button_length / rows);
        }

      g_slist_free (windows_scored);

      cols = n_buttons / rows;
      if (cols * rows < n_buttons)
        cols++;

      *n_rows = rows;
      *n_cols = cols;
    }
}



static void
xfce_tasklist_size_allocate (GtkWidget *widget,
                             GtkAllocation *allocation)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);
  gint rows, cols;
  gint row;
  GtkAllocation area = *allocation;
  GList *li;
  XfceTasklistChild *child;
  gint i;
  GtkAllocation child_alloc;
  gboolean direction_rtl = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;
  gint w, x, y, h;
  gint area_x, area_width;
  gint arrow_position;
  GtkRequisition child_req;

  panel_return_if_fail (gtk_widget_get_visible (tasklist->arrow_button));

  /* set widget allocation */
  gtk_widget_set_allocation (widget, allocation);

  /* swap integers with vertical orientation */
  if (!xfce_tasklist_horizontal (tasklist))
    TRANSPOSE_AREA (area);
  panel_return_if_fail (area.height == tasklist->size);

  /* TODO if we compare the allocation with the requisition we can
   * do a fast path to the child allocation, i think */

  /* useless but hides compiler warning */
  w = x = y = rows = cols = 0;

  xfce_tasklist_size_layout (tasklist, &area, &rows, &cols, &arrow_position);

  /* allocate the arrow button for the overflow menu */
  child_alloc.width = ARROW_BUTTON_SIZE;
  child_alloc.height = area.height;

  if (arrow_position != -1)
    {
      child_alloc.x = area.x;
      child_alloc.y = area.y;

      if (!direction_rtl)
        child_alloc.x += arrow_position;
      else
        child_alloc.x += (area.width - arrow_position);

      area.width = arrow_position;

      /* position the arrow in the correct position */
      if (!xfce_tasklist_horizontal (tasklist))
        TRANSPOSE_AREA (child_alloc);
    }
  else
    {
      child_alloc.x = child_alloc.y = OFFSCREEN;
    }

  gtk_widget_size_allocate (tasklist->arrow_button, &child_alloc);

  area_x = area.x;
  area_width = area.width;
  h = area.height / rows;

  /* allocate all the children */
  for (li = tasklist->windows, i = 0; li != NULL; li = li->next)
    {
      child = li->data;

      /* skip hidden buttons */
      if (!gtk_widget_get_visible (child->button))
        continue;

      if (G_LIKELY (child->type == CHILD_TYPE_WINDOW || child->type == CHILD_TYPE_GROUP))
        {
          row = (i % rows);
          if (row == 0)
            {
              x = area_x;
              y = area.y;

              if (xfce_tasklist_deskbar (tasklist) && tasklist->show_labels)
                {
                  /* fixed width is OK because area.width==w*cols */
                  w = MIN (area.height / tasklist->nrows, tasklist->max_button_size);
                }
              else if (tasklist->show_labels)
                {
                  /* TODO, this is a work-around, something else goes wrong
                   * with counting the windows... */
                  if (cols < 1)
                    cols = 1;
                  w = area_width / cols--;
                  if (w > tasklist->max_button_length)
                    w = tasklist->max_button_length;
                }
              else /* buttons without labels */
                {
                  w = h;
                }

              area_width -= w;
              area_x += w;
            }

          child_alloc.y = y;
          child_alloc.x = x;
          child_alloc.width = MAX (w, 1); /* TODO this is a workaround */
          child_alloc.height = h;

          y += h;

          if (direction_rtl)
            child_alloc.x = area.x + area.width - (child_alloc.x - area.x) - child_alloc.width;

          /* allocate the child */
          if (!xfce_tasklist_horizontal (tasklist))
            TRANSPOSE_AREA (child_alloc);

          /* increase the position counter */
          i++;
        }
      else
        {
          gtk_widget_get_preferred_size (child->button, NULL, &child_req);

          /* move the button offscreen */
          child_alloc.y = child_alloc.x = OFFSCREEN;
          child_alloc.width = child_req.width;
          child_alloc.height = child_req.height;
        }

      gtk_widget_size_allocate (child->button, &child_alloc);
    }

  /* update icon geometries */
  if (tasklist->update_icon_geometries_id == 0)
    tasklist->update_icon_geometries_id = g_idle_add_full (G_PRIORITY_LOW, xfce_tasklist_update_icon_geometries,
                                                           tasklist, xfce_tasklist_update_icon_geometries_destroyed);
}



static void
xfce_tasklist_style_updated (GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);
  gint max_button_length, min_button_length, max_button_size, minimized_icon_lucency, menu_max_width_chars;

  /* let gtk update the widget style */
  (*GTK_WIDGET_CLASS (xfce_tasklist_parent_class)->style_updated) (widget);

  /* read the style properties */
  gtk_widget_style_get (GTK_WIDGET (tasklist),
                        "max-button-length", &max_button_length,
                        "min-button-length", &min_button_length,
                        "ellipsize-mode", &tasklist->ellipsize_mode,
                        "max-button-size", &max_button_size,
                        "minimized-icon-lucency", &minimized_icon_lucency,
                        "menu-max-width-chars", &menu_max_width_chars,
                        NULL);

  /* GTK doesn't do this by itself unfortunately, unlike GObject */
  max_button_length = CLAMP (max_button_length, MIN_MAX_BUTTON_LENGTH, MAX_MAX_BUTTON_LENGTH);
  min_button_length = CLAMP (min_button_length, MIN_MIN_BUTTON_LENGTH, MAX_MIN_BUTTON_LENGTH);
  tasklist->max_button_size = CLAMP (max_button_size, MIN_MAX_BUTTON_SIZE, MAX_MAX_BUTTON_SIZE);
  tasklist->minimized_icon_lucency = CLAMP (minimized_icon_lucency, MIN_MINIMIZED_ICON_LUCENCY, MAX_MINIMIZED_ICON_LUCENCY);
  tasklist->menu_max_width_chars = CLAMP (menu_max_width_chars, MIN_MENU_MAX_WIDTH_CHARS, MAX_MENU_MAX_WIDTH_CHARS);

  if (max_button_length == -1)
    max_button_length = MAX_MAX_BUTTON_LENGTH;

  /* prevent abuse of the min/max button length */
  tasklist->max_button_length = MAX (min_button_length, max_button_length);
  tasklist->min_button_length = MIN (min_button_length, max_button_length);

  if (tasklist->max_button_size == -1)
    tasklist->max_button_size = MAX_MAX_BUTTON_SIZE;

  gtk_widget_queue_resize (widget);
}



static void
xfce_tasklist_realize (GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  (*GTK_WIDGET_CLASS (xfce_tasklist_parent_class)->realize) (widget);

  /* we now have a screen */
  xfce_tasklist_connect_screen (tasklist);
}



static void
xfce_tasklist_unrealize (GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  /* we're going to loose the screen */
  xfce_tasklist_disconnect_screen (tasklist);

  (*GTK_WIDGET_CLASS (xfce_tasklist_parent_class)->unrealize) (widget);
}



static gboolean
xfce_tasklist_scroll_event (GtkWidget *widget,
                            GdkEventScroll *event)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);
  XfceTasklistChild *child = NULL;
  GList *li, *lnew = NULL;
  GdkScrollDirection scrolling_direction;
  gboolean wrap_windows = tasklist->wrap_windows;

  if (!tasklist->window_scrolling)
    return TRUE;

  /* get the current active button */
  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      if (gtk_widget_get_visible (child->button)
          && child->window != NULL && xfw_window_is_active (child->window))
        break;
    }

  if (G_UNLIKELY (li == NULL))
    return TRUE;

  if (event->direction != GDK_SCROLL_SMOOTH)
    scrolling_direction = event->direction;
  else if (event->delta_y < 0)
    scrolling_direction = GDK_SCROLL_UP;
  else if (event->delta_y > 0)
    scrolling_direction = GDK_SCROLL_DOWN;
  else if (event->delta_x < 0)
    scrolling_direction = GDK_SCROLL_LEFT;
  else if (event->delta_x > 0)
    scrolling_direction = GDK_SCROLL_RIGHT;
  else
    {
      panel_debug_filtered (PANEL_DEBUG_TASKLIST, "scrolling event with no delta happened");
      return TRUE;
    }

  switch (scrolling_direction)
    {
    case GDK_SCROLL_UP:
      /* find previous button on the tasklist */
      for (lnew = g_list_previous (li);; lnew = lnew->prev)
        {
          if (lnew == NULL)
            {
              /* wrap only once if the first button is reached */
              if (wrap_windows)
                {
                  lnew = g_list_last (li);
                  wrap_windows = FALSE;
                  if (lnew == NULL)
                    break;
                }
              else
                break;
            }

          child = lnew->data;
          if (child->window != NULL
              && gtk_widget_get_visible (child->button))
            break;
        }

      break;

    case GDK_SCROLL_DOWN:
      /* find the next button on the tasklist */
      for (lnew = g_list_next (li);; lnew = lnew->next)
        {
          if (lnew == NULL)
            {
              /* wrap only once if the last button is reached */
              if (wrap_windows)
                {
                  lnew = g_list_first (li);
                  wrap_windows = FALSE;
                  if (lnew == NULL)
                    break;
                }
              else
                break;
            }

          child = lnew->data;
          if (child->window != NULL
              && gtk_widget_get_visible (child->button))
            break;
        }

      break;

    case GDK_SCROLL_LEFT:
      /* TODO */
      break;

    case GDK_SCROLL_RIGHT:
      /* TODO */
      break;

    default:
      panel_debug_filtered (PANEL_DEBUG_TASKLIST, "unknown scrolling event type");
      break;
    }

  if (lnew != NULL)
    xfce_tasklist_button_activate (lnew->data, event->time);

  return TRUE;
}



static gboolean
xfce_tasklist_free_child (gpointer data)
{
  g_slice_free (XfceTasklistChild, data);
  return FALSE;
}



static void
xfce_tasklist_remove (GtkContainer *container,
                      GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (container);
  gboolean was_visible;
  XfceTasklistChild *child;
  GList *li;

  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->button == widget)
        {
          tasklist->windows = g_list_delete_link (tasklist->windows, li);

          was_visible = gtk_widget_get_visible (widget);

          gtk_widget_unparent (child->button);

          if (child->motion_timeout_id != 0)
            g_source_remove (child->motion_timeout_id);

          if (child->pixbuf != NULL)
            g_object_unref (child->pixbuf);

          /* allow time for signal handlers connected to the destroy/dispose signals of
           * child members to run, they could refer to these members via child, e.g.
           * child->button as above to test for equality */
          g_idle_add (xfce_tasklist_free_child, child);

          /* queue a resize if needed */
          if (G_LIKELY (was_visible))
            gtk_widget_queue_resize (GTK_WIDGET (container));

          break;
        }
    }
}



static void
xfce_tasklist_forall (GtkContainer *container,
                      gboolean include_internals,
                      GtkCallback callback,
                      gpointer callback_data)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (container);
  GList *children = tasklist->windows;
  XfceTasklistChild *child;

  if (include_internals)
    (*callback) (tasklist->arrow_button, callback_data);

  while (children != NULL)
    {
      child = children->data;
      children = children->next;

      (*callback) (child->button, callback_data);
    }
}



static GType
xfce_tasklist_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}



static void
xfce_tasklist_arrow_button_menu_destroy (GtkWidget *menu,
                                         XfceTasklist *tasklist)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (tasklist->arrow_button));
  panel_return_if_fail (GTK_IS_WIDGET (menu));

  panel_utils_destroy_later (menu);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tasklist->arrow_button), FALSE);

#ifdef ENABLE_X11
  /* make sure the wireframe is hidden */
  xfce_tasklist_wireframe_hide (tasklist);
#endif
}



static void
xfce_tasklist_arrow_button_toggled (GtkWidget *button,
                                    XfceTasklist *tasklist)
{
  GList *li;
  XfceTasklistChild *child;
  GtkWidget *mi;
  GtkWidget *menu;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (tasklist->arrow_button == button);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      menu = gtk_menu_new ();
      g_signal_connect (G_OBJECT (menu), "deactivate",
                        G_CALLBACK (xfce_tasklist_arrow_button_menu_destroy), tasklist);

      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;

          if (child->type != CHILD_TYPE_OVERFLOW_MENU)
            continue;

          mi = xfce_tasklist_button_proxy_menu_item (child, TRUE);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
          gtk_widget_show (mi);
        }

      gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);
      xfce_panel_plugin_popup_menu (xfce_tasklist_get_panel_plugin (tasklist),
                                    GTK_MENU (menu), button, NULL);
    }
}



static void
xfce_tasklist_connect_screen (XfceTasklist *tasklist)
{
  GList *windows, *li;
  XfwWorkspaceManager *manager;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->screen == NULL);
  panel_return_if_fail (tasklist->display == NULL);

  if (tasklist->grouping)
    tasklist->apps = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                            NULL, (GDestroyNotify) xfce_tasklist_group_button_remove);

  /* set the display and screen */
  tasklist->display = gtk_widget_get_display (GTK_WIDGET (tasklist));
  tasklist->screen = xfw_screen_get_default ();
  manager = xfw_screen_get_workspace_manager (tasklist->screen);
  tasklist->workspace_group = xfw_workspace_manager_list_workspace_groups (manager)->data;

  /* add all existing windows on this screen */
  windows = xfw_screen_get_windows (tasklist->screen);
  for (li = windows; li != NULL; li = li->next)
    xfce_tasklist_window_added (tasklist->screen, li->data, tasklist);

  /* monitor window movement */
  g_signal_connect (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (tasklist))),
                    "configure-event",
                    G_CALLBACK (xfce_tasklist_configure_event), tasklist);

  /* monitor screen changes */
  xfce_tasklist_active_window_changed (tasklist->screen, NULL, tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "active-window-changed",
                    G_CALLBACK (xfce_tasklist_active_window_changed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->workspace_group), "active-workspace-changed",
                    G_CALLBACK (xfce_tasklist_active_workspace_changed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "window-opened",
                    G_CALLBACK (xfce_tasklist_window_added), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "window-closed",
                    G_CALLBACK (xfce_tasklist_window_removed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->workspace_group), "viewports-changed",
                    G_CALLBACK (xfce_tasklist_viewports_changed), tasklist);

  /* update the viewport if not all monitors are shown */
  if (!tasklist->all_monitors)
    {
      /* update the monitor geometry */
      xfce_tasklist_update_monitor_geometry (tasklist);
    }
}



static void
xfce_tasklist_disconnect_screen (XfceTasklist *tasklist)
{
  GSList *li, *lnext;
  GList *wi, *wnext;
  XfceTasklistChild *child;
  guint n;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (XFW_IS_SCREEN (tasklist->screen));

  /* disconnect configure-event signal */
  g_signal_handlers_disconnect_by_func (
    G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (tasklist))),
    G_CALLBACK (xfce_tasklist_configure_event), tasklist);

  /* disconnect monitor signals */
  n = g_signal_handlers_disconnect_matched (G_OBJECT (tasklist->screen),
                                            G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, tasklist);
  panel_return_if_fail (n == 3);
  n = g_signal_handlers_disconnect_matched (G_OBJECT (tasklist->workspace_group),
                                            G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, tasklist);
  panel_return_if_fail (n == 2);

  /* delete all known apps (and their buttons) */
  if (tasklist->apps != NULL)
    {
      g_hash_table_destroy (tasklist->apps);
      tasklist->apps = NULL;
    }

  /* disconnect from all skipped windows */
  for (li = tasklist->skipped_windows; li != NULL; li = lnext)
    {
      lnext = li->next;
      panel_return_if_fail (xfw_window_is_skip_tasklist (XFW_WINDOW (li->data)));
      xfce_tasklist_window_removed (tasklist->screen, li->data, tasklist);
    }

  /* remove all the windows */
  for (wi = tasklist->windows; wi != NULL; wi = wnext)
    {
      wnext = wi->next;
      child = wi->data;

      /* do a fake window remove */
      panel_return_if_fail (child->type != CHILD_TYPE_GROUP);
      panel_return_if_fail (XFW_IS_WINDOW (child->window));
      xfce_tasklist_window_removed (tasklist->screen, child->window, tasklist);
    }

  panel_assert (tasklist->windows == NULL);
  panel_assert (tasklist->skipped_windows == NULL);

  g_clear_object (&tasklist->screen);
  tasklist->display = NULL;
}



static gboolean
xfce_tasklist_configure_event (GtkWidget *widget,
                               GdkEvent *event,
                               XfceTasklist *tasklist)
{
  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), FALSE);

  if (!tasklist->all_monitors)
    {
      /* update the monitor geometry */
      xfce_tasklist_update_monitor_geometry (tasklist);
    }
  return FALSE;
}



static void
xfce_tasklist_active_window_changed (XfwScreen *screen,
                                     XfwWindow *previous_window,
                                     XfceTasklist *tasklist)
{
  XfwWindow *active_window;
  XfwApplication *app = NULL;
  GList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (XFW_IS_SCREEN (screen));
  panel_return_if_fail (previous_window == NULL || XFW_IS_WINDOW (previous_window));
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->screen == screen);

  /* get the new active window */
  active_window = xfw_screen_get_active_window (screen);

  /* lock the taskbar */
  xfce_taskbar_lock (tasklist);

  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      /* update timestamp for window */
      if (child->window == active_window)
        {
          child->last_focused = g_get_real_time ();
          /* the active window is in a group, so find the group button */
          if (child->type == CHILD_TYPE_GROUP_MENU)
            {
              app = child->app;
            }
        }

      /* set the toggle button state */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child->button),
                                    child->window == active_window && active_window != NULL);
    }
  /* set the toggle button state for the group button */
  if (app)
    {
      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;
          if (child->type == CHILD_TYPE_GROUP
              && child->app == app)
            {
              /* update the button's state and icon, the latter makes sure it is rendered correctly
                 if all previous group windows were minimized */
              xfce_tasklist_group_button_icon_changed (child->app, child);
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child->button), TRUE);
            }
        }
    }

  /* release the lock */
  xfce_taskbar_unlock (tasklist);
}



static void
xfce_tasklist_active_workspace_changed (XfwWorkspaceGroup *group,
                                        XfwWorkspace *previous_workspace,
                                        XfceTasklist *tasklist)
{
  GList *windows, *li;
  XfwWorkspace *active_ws;
  XfceTasklistChild *child;

  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (previous_workspace == NULL || XFW_IS_WORKSPACE (previous_workspace));
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->workspace_group == group);

  /* leave when we are locked or show all workspaces. the null
   * check for @previous_workspace is used to update the tasklist
   * on setting changes */
  if (xfce_taskbar_is_locked (tasklist)
      || (previous_workspace != NULL
          && tasklist->all_workspaces))
    return;

  /* walk all the children and update their visibility: make a copy of the window list
   * here because changing the buttons visibility can change the group buttons visibility,
   * which in turn can change the list order */
  active_ws = xfw_workspace_group_get_active_workspace (group);
  windows = g_list_copy (tasklist->windows);
  for (li = windows; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->type != CHILD_TYPE_GROUP)
        {
          if (xfce_tasklist_button_visible (child, active_ws))
            gtk_widget_show (child->button);
          else
            gtk_widget_hide (child->button);
        }
    }
  g_list_free (windows);
}



static void
xfce_tasklist_window_added (XfwScreen *screen,
                            XfwWindow *window,
                            XfceTasklist *tasklist)
{
  XfceTasklistChild *child;

  panel_return_if_fail (XFW_IS_SCREEN (screen));
  panel_return_if_fail (XFW_IS_WINDOW (window));
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->screen == screen);
  panel_return_if_fail (xfw_window_get_screen (window) == screen);

  /* ignore this window, but watch it for state changes */
  if (xfw_window_is_skip_tasklist (window))
    {
      tasklist->skipped_windows = g_slist_prepend (tasklist->skipped_windows, window);
      g_signal_connect (G_OBJECT (window), "state-changed",
                        G_CALLBACK (xfce_tasklist_skipped_windows_state_changed), tasklist);

      return;
    }

  /* create new window button */
  child = xfce_tasklist_button_new (window, tasklist);

  /* initial visibility of the function */
  if (xfce_tasklist_button_visible (child, xfw_workspace_group_get_active_workspace (tasklist->workspace_group)))
    gtk_widget_show (child->button);

  if (tasklist->grouping)
    {
      XfceTasklistChild *group_child = g_hash_table_lookup (tasklist->apps, child->app);
      if (group_child == NULL)
        {
          /* create group button for this window and add it */
          group_child = xfce_tasklist_group_button_new (child->app, tasklist);
          g_hash_table_insert (tasklist->apps, child->app, group_child);
        }

      /* add window to the group button */
      xfce_tasklist_group_button_add_window (group_child, child);
    }

  /* set urgency blinking if needed */
  if (xfw_window_is_urgent (window))
    xfce_tasklist_button_state_changed (window, XFW_WINDOW_STATE_URGENT, XFW_WINDOW_STATE_URGENT, child);

  gtk_widget_queue_resize (GTK_WIDGET (tasklist));
}



static void
xfce_tasklist_window_removed (XfwScreen *screen,
                              XfwWindow *window,
                              XfceTasklist *tasklist)
{
  GList *li;
  GSList *lp;
  XfceTasklistChild *child;
  guint n;

  panel_return_if_fail (XFW_IS_SCREEN (screen));
  panel_return_if_fail (XFW_IS_WINDOW (window));
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->screen == screen);

  /* check if the window is in our skipped window list */
  if (xfw_window_is_skip_tasklist (window)
      && (lp = g_slist_find (tasklist->skipped_windows, window)) != NULL)
    {
      tasklist->skipped_windows = g_slist_delete_link (tasklist->skipped_windows, lp);
      g_signal_handlers_disconnect_by_func (window, xfce_tasklist_skipped_windows_state_changed, tasklist);

      return;
    }

  /* remove the child from the taskbar */
  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->window == window)
        {
          /* disconnect from all the window watch functions */
          panel_return_if_fail (XFW_IS_WINDOW (window));
          n = g_signal_handlers_disconnect_matched (G_OBJECT (window),
                                                    G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, child);

#ifdef ENABLE_X11
          /* hide the wireframe */
          if (G_UNLIKELY (n > 5 && tasklist->show_wireframes))
            {
              xfce_tasklist_wireframe_hide (tasklist);
              n--;
            }
#endif

          panel_return_if_fail (n == 5);

          /* destroy the button, this will free the child data in the
           * container remove function */
          gtk_widget_destroy (child->button);

          break;
        }
    }

  gtk_widget_queue_resize (GTK_WIDGET (tasklist));
}



static void
xfce_tasklist_viewports_changed (XfwWorkspaceGroup *group,
                                 XfceTasklist *tasklist)
{
  XfwWorkspace *active_ws;

  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->workspace_group == group);

  /* pretend we changed workspace, this will update the
   * visibility of all the buttons */
  active_ws = xfw_workspace_group_get_active_workspace (group);
  xfce_tasklist_active_workspace_changed (group, active_ws, tasklist);
}



static void
xfce_tasklist_skipped_windows_state_changed (XfwWindow *window,
                                             XfwWindowState changed_state,
                                             XfwWindowState new_state,
                                             XfceTasklist *tasklist)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (XFW_IS_WINDOW (window));
  panel_return_if_fail (g_slist_find (tasklist->skipped_windows, window) != NULL);

  if (PANEL_HAS_FLAG (changed_state, XFW_WINDOW_STATE_SKIP_TASKLIST))
    {
      /* remove from list */
      tasklist->skipped_windows = g_slist_remove (tasklist->skipped_windows, window);
      g_signal_handlers_disconnect_by_func (window, xfce_tasklist_skipped_windows_state_changed, tasklist);

      /* pretend a normal window insert */
      xfce_tasklist_window_added (xfw_window_get_screen (window), window, tasklist);
    }
}



static void
xfce_tasklist_sort (XfceTasklist *tasklist,
                    gboolean sort_groups)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->sort_order != XFCE_TASKLIST_SORT_ORDER_DND)
    {
      tasklist->windows = g_list_sort_with_data (tasklist->windows,
                                                 xfce_tasklist_button_compare,
                                                 tasklist);
      if (sort_groups && tasklist->grouping)
        for (GList *lp = tasklist->windows; lp != NULL; lp = lp->next)
          {
            XfceTasklistChild *child = lp->data;

            if (child->type == CHILD_TYPE_GROUP)
              xfce_tasklist_group_button_sort (child);
          }
    }

  gtk_widget_queue_resize (GTK_WIDGET (tasklist));
}



static gboolean
xfce_tasklist_update_icon_geometries (gpointer data)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (data);
  GList *li;
  GSList *lp;
  gint root_x, root_y;
  GtkWidget *toplevel;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (tasklist));
  gtk_window_get_position (GTK_WINDOW (toplevel), &root_x, &root_y);
  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), G_SOURCE_REMOVE);

  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      XfceTasklistChild *child, *child2;
      GtkAllocation alloc;
      GdkWindow *window;

      child = li->data;

      switch (child->type)
        {
        case CHILD_TYPE_WINDOW:
          window = gtk_widget_get_window (child->button);
          gtk_widget_get_allocation (child->button, &alloc);
          alloc.x += root_x;
          alloc.y += root_y;
          xfw_window_set_button_geometry (child->window, window, &alloc, NULL);
          break;

        case CHILD_TYPE_GROUP:
          window = gtk_widget_get_window (child->button);
          gtk_widget_get_allocation (child->button, &alloc);
          alloc.x += root_x;
          alloc.y += root_y;
          for (lp = child->windows; lp != NULL; lp = lp->next)
            {
              child2 = lp->data;
              xfw_window_set_button_geometry (child2->window, window, &alloc, NULL);
            }
          break;

        case CHILD_TYPE_OVERFLOW_MENU:
          window = gtk_widget_get_window (tasklist->arrow_button);
          gtk_widget_get_allocation (tasklist->arrow_button, &alloc);
          alloc.x += root_x;
          alloc.y += root_y;
          xfw_window_set_button_geometry (child->window, window, &alloc, NULL);
          break;

        case CHILD_TYPE_GROUP_MENU:
          /* we already handled those in the group button */
          break;
        };
    }

  return G_SOURCE_REMOVE;
}



static void
xfce_tasklist_update_icon_geometries_destroyed (gpointer data)
{
  XFCE_TASKLIST (data)->update_icon_geometries_id = 0;
}



static gboolean
xfce_tasklist_update_monitor_geometry_idle (gpointer data)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (data);

  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), FALSE);

  /* can happen e.g. when the panel is associated with a disconnected device at startup */
  if (tasklist->display == NULL)
    return FALSE;

  tasklist->n_monitors = gdk_display_get_n_monitors (tasklist->display);

  /* update visibility of buttons */
  if (tasklist->screen != NULL)
    xfce_tasklist_active_workspace_changed (tasklist->workspace_group, NULL, tasklist);

  return FALSE;
}



static void
xfce_tasklist_update_monitor_geometry_idle_destroy (gpointer data)
{
  XFCE_TASKLIST (data)->update_monitor_geometry_id = 0;
}



static gboolean
xfce_tasklist_child_drag_motion_timeout (gpointer data)
{
  XfceTasklistChild *child = data;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (XFW_IS_SCREEN (child->tasklist->screen), FALSE);

  if (child->type == CHILD_TYPE_WINDOW)
    {
      xfce_tasklist_button_activate (child, child->motion_timestamp);
    }
  else if (child->type == CHILD_TYPE_GROUP)
    {
      /* TODO popup menu */
    }

  return FALSE;
}



static void
xfce_tasklist_child_drag_motion_timeout_destroyed (gpointer data)
{
  XfceTasklistChild *child = data;

  child->motion_timeout_id = 0;
  child->motion_timestamp = 0;
}



static gboolean
xfce_tasklist_child_drag_motion (XfceTasklistChild *child,
                                 GdkDragContext *context,
                                 gint x,
                                 gint y,
                                 guint timestamp)
{
  GtkWidget *dnd_widget;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);

  /* don't respond to dragging our own children or panel plugins */
  dnd_widget = gtk_drag_get_source_widget (context);
  if (dnd_widget == NULL
      || (gtk_widget_get_parent (dnd_widget) != GTK_WIDGET (child->tasklist)
          && !XFCE_IS_PANEL_PLUGIN (dnd_widget)))
    {
      child->motion_timestamp = timestamp;
      if (child->motion_timeout_id == 0
          && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (child->button)))
        {
          child->motion_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, DRAG_ACTIVATE_TIMEOUT,
                                                                   xfce_tasklist_child_drag_motion_timeout, child,
                                                                   xfce_tasklist_child_drag_motion_timeout_destroyed);
        }

      /* keep emitting the signal */
      gdk_drag_status (context, 0, timestamp);

      /* we want to receive leave signal as well */
      return TRUE;
    }
  else if (gtk_drag_dest_find_target (child->button, context, NULL) != GDK_NONE)
    {
      /* dnd to reorder buttons */
      gdk_drag_status (context, GDK_ACTION_MOVE, timestamp);

      return TRUE;
    }

  /* also send drag-motion to other widgets */
  return FALSE;
}



static void
xfce_tasklist_child_drag_leave (XfceTasklistChild *child,
                                GdkDragContext *context,
                                GtkDragResult result)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));

  if (child->motion_timeout_id != 0)
    g_source_remove (child->motion_timeout_id);
}



static void
xfce_tasklist_child_drag_begin_event (GtkWidget *widget,
                                      GdkDragContext *context,
                                      gpointer user_data)
{
  GtkWidget *plugin = user_data;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (plugin), TRUE);
}



static void
xfce_tasklist_child_drag_end_event (GtkWidget *widget,
                                    GdkDragContext *context,
                                    gpointer user_data)
{
  GtkWidget *plugin = user_data;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (plugin), FALSE);
}



static XfceTasklistChild *
xfce_tasklist_child_new (XfceTasklist *tasklist)
{
  XfceTasklistChild *child;
  XfcePanelPlugin *plugin;
  GtkCssProvider *provider;
  gchar *css_string;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), NULL);

  child = g_slice_new0 (XfceTasklistChild);
  child->tasklist = tasklist;

  /* create the window button */
  child->button = xfce_arrow_button_new (GTK_ARROW_NONE);
  gtk_widget_set_parent (child->button, GTK_WIDGET (tasklist));
  gtk_button_set_relief (GTK_BUTTON (child->button),
                         tasklist->button_relief);
  gtk_widget_add_events (GTK_WIDGET (child->button), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
  g_object_bind_property (tasklist, "show_tooltips", child->button, "has-tooltip",
                          G_BINDING_SYNC_CREATE);

  child->box = gtk_box_new (!xfce_tasklist_vertical (tasklist) ? GTK_ORIENTATION_HORIZONTAL
                                                               : GTK_ORIENTATION_VERTICAL,
                            6);
  gtk_container_add (GTK_CONTAINER (child->button), child->box);
  gtk_widget_show (child->box);

  provider = gtk_css_provider_new ();
  /* silly workaround for gtkcss only accepting "." as decimal separator and floats returning
     with "," as decimal separator in some locales */
  css_string = g_strdup_printf ("image { padding: 3px; } image.minimized { opacity: %d.%02d; }",
                                tasklist->minimized_icon_lucency / 100,
                                tasklist->minimized_icon_lucency % 100);
  gtk_css_provider_load_from_data (provider, css_string, -1, NULL);
  child->icon = gtk_image_new ();
  child->pixbuf = NULL;
  gtk_style_context_add_provider (gtk_widget_get_style_context (child->icon),
                                  GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  g_free (css_string);

  if (tasklist->show_labels)
    gtk_box_pack_start (GTK_BOX (child->box), child->icon, FALSE, TRUE, 0);
  else
    gtk_box_pack_start (GTK_BOX (child->box), child->icon, TRUE, TRUE, 0);
  if (tasklist->minimized_icon_lucency > 0)
    gtk_widget_show (child->icon);

  child->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (child->box), child->label, TRUE, TRUE, 0);
  if (!xfce_tasklist_vertical (tasklist))
    {
      /* gtk_box_reorder_child (GTK_BOX (child->box), child->icon, 0); */
      gtk_label_set_xalign (GTK_LABEL (child->label), 0.0);
      gtk_label_set_yalign (GTK_LABEL (child->label), 0.5);
      gtk_label_set_ellipsize (GTK_LABEL (child->label), tasklist->ellipsize_mode);
    }
  else
    {
      /* gtk_box_reorder_child (GTK_BOX (child->box), child->icon, -1); */
      gtk_label_set_yalign (GTK_LABEL (child->label), 0.0);
      gtk_label_set_xalign (GTK_LABEL (child->label), 0.5);
      gtk_label_set_angle (GTK_LABEL (child->label), 270);
      /* TODO can we already ellipsize here yet? */
    }

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, ".label-hidden { opacity: 0.75; }", -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (child->label),
                                  GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  /* don't show the label if we're in iconbox style */
  if (tasklist->show_labels)
    gtk_widget_show (child->label);

  plugin = xfce_tasklist_get_panel_plugin (tasklist);
  gtk_drag_dest_set (GTK_WIDGET (child->button), 0,
                     NULL, 0, GDK_ACTION_DEFAULT);
  g_signal_connect_swapped (G_OBJECT (child->button), "drag-motion",
                            G_CALLBACK (xfce_tasklist_child_drag_motion), child);
  g_signal_connect_swapped (G_OBJECT (child->button), "drag-leave",
                            G_CALLBACK (xfce_tasklist_child_drag_leave), child);
  g_signal_connect_after (G_OBJECT (child->button), "drag-begin",
                          G_CALLBACK (xfce_tasklist_child_drag_begin_event), plugin);
  g_signal_connect_after (G_OBJECT (child->button), "drag-end",
                          G_CALLBACK (xfce_tasklist_child_drag_end_event), plugin);

  return child;
}



/**
 * Wire Frame
 **/
#ifdef ENABLE_X11
static void
xfce_tasklist_wireframe_hide (XfceTasklist *tasklist)
{
  GdkDisplay *dpy;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->wireframe_window != 0)
    {
      /* unmap the window */
      dpy = gtk_widget_get_display (GTK_WIDGET (tasklist));
      XUnmapWindow (GDK_DISPLAY_XDISPLAY (dpy), tasklist->wireframe_window);
    }
}



static void
xfce_tasklist_wireframe_destroy (XfceTasklist *tasklist)
{
  GdkDisplay *dpy;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->wireframe_window != 0)
    {
      /* unmap and destroy the window */
      dpy = gtk_widget_get_display (GTK_WIDGET (tasklist));
      XUnmapWindow (GDK_DISPLAY_XDISPLAY (dpy), tasklist->wireframe_window);
      XDestroyWindow (GDK_DISPLAY_XDISPLAY (dpy), tasklist->wireframe_window);

      tasklist->wireframe_window = 0;
    }
}



static void
xfce_tasklist_wireframe_update (XfceTasklist *tasklist,
                                XfceTasklistChild *child)
{
  Display *dpy;
  GdkDisplay *gdpy;
  GdkWindow *gdkwindow;
  GdkRectangle rect;
  gint x_root, y_root;
  XSetWindowAttributes attrs;
  GC gc;
  XRectangle xrect;
  GtkBorder extents;
  GtkAllocation alloc;
  guint scale_factor;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (tasklist->show_wireframes);
  panel_return_if_fail (XFW_IS_WINDOW (child->window));

  gdpy = gtk_widget_get_display (GTK_WIDGET (tasklist));
  dpy = GDK_DISPLAY_XDISPLAY (gdpy);

  /* get the window geometry */
  rect = *(xfw_window_get_geometry (child->window));

  /* check if we're dealing with a CSD window */
  gdkwindow = gdk_x11_window_foreign_new_for_display (gdpy, xfw_window_x11_get_xid (child->window));
  if (gdkwindow != NULL)
    {
      if (xfce_has_gtk_frame_extents (gdkwindow, &extents))
        {
          rect.x += extents.left;
          rect.y += extents.top;
          rect.width -= extents.left + extents.right;
          rect.height -= extents.top + extents.bottom;
        }

      g_object_unref (gdkwindow);
    }

  if (G_LIKELY (tasklist->wireframe_window != 0))
    {
      /* reposition the wireframe */
      XMoveResizeWindow (dpy, tasklist->wireframe_window, rect.x, rect.y, rect.width, rect.height);

      /* full window rectangle */
      xrect.x = 0;
      xrect.y = 0;
      xrect.width = rect.width;
      xrect.height = rect.height;

      /* we need to restore the window first */
      XShapeCombineRectangles (dpy, tasklist->wireframe_window, ShapeBounding,
                               0, 0, &xrect, 1, ShapeSet, Unsorted);
    }
  else
    {
      /* set window attributes */
      attrs.override_redirect = True;
      attrs.background_pixel = 0x000000;

      /* create new window */
      tasklist->wireframe_window = XCreateWindow (dpy, DefaultRootWindow (dpy),
                                                  rect.x, rect.y, rect.width, rect.height, 0,
                                                  CopyFromParent, InputOutput,
                                                  CopyFromParent,
                                                  CWOverrideRedirect | CWBackPixel,
                                                  &attrs);
    }

  /* create rectangle what will be 'transparent' in the window */
  xrect.x = WIREFRAME_SIZE;
  xrect.y = WIREFRAME_SIZE;
  xrect.width = rect.width - WIREFRAME_SIZE * 2;
  xrect.height = rect.height - WIREFRAME_SIZE * 2;

  /* substruct rectangle from the window */
  XShapeCombineRectangles (dpy, tasklist->wireframe_window, ShapeBounding,
                           0, 0, &xrect, 1, ShapeSubtract, Unsorted);

  /* create rectangle for the window button so that the wireframe window does not
   * interfere with the reception of pointer events (issue #543) */
  gtk_widget_get_allocation (child->button, &alloc);
  gdk_window_get_origin (gtk_widget_get_window (child->button), &x_root, &y_root);
  scale_factor = gdk_window_get_scale_factor (gtk_widget_get_window (GTK_WIDGET (tasklist)));
  xrect.x = (x_root + alloc.x) * scale_factor - rect.x;
  xrect.y = (y_root + alloc.y) * scale_factor - rect.y;
  xrect.width = alloc.width * scale_factor;
  xrect.height = alloc.height * scale_factor;

  /* substruct rectangle from the window */
  XShapeCombineRectangles (dpy, tasklist->wireframe_window, ShapeBounding,
                           0, 0, &xrect, 1, ShapeSubtract, Unsorted);

  /* map the window */
  XMapWindow (dpy, tasklist->wireframe_window);

  /* create a white gc */
  gc = XCreateGC (dpy, tasklist->wireframe_window, 0, NULL);
  XSetForeground (dpy, gc, 0xffffff);

  /* draw the outer white rectangle */
  XDrawRectangle (dpy, tasklist->wireframe_window, gc,
                  0, 0, rect.width - 1, rect.height - 1);

  /* draw the inner white rectangle */
  XDrawRectangle (dpy, tasklist->wireframe_window, gc,
                  WIREFRAME_SIZE - 1, WIREFRAME_SIZE - 1,
                  rect.width - 2 * (WIREFRAME_SIZE - 1) - 1,
                  rect.height - 2 * (WIREFRAME_SIZE - 1) - 1);

  XFreeGC (dpy, gc);
}
#endif



/**
 * Tasklist Buttons
 **/
static inline gboolean
xfce_tasklist_button_visible (XfceTasklistChild *child,
                              XfwWorkspace *active_ws)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (child->tasklist);

  panel_return_val_if_fail (active_ws == NULL || XFW_IS_WORKSPACE (active_ws), FALSE);
  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), FALSE);
  panel_return_val_if_fail (XFW_IS_WINDOW (child->window), FALSE);
  panel_return_val_if_fail (GDK_IS_DISPLAY (tasklist->display), FALSE);

  if (xfce_tasklist_filter_monitors (tasklist))
    {
      GdkMonitor *monitor = tasklist_get_monitor (tasklist);
      GList *monitors = xfw_window_get_monitors (child->window);
      if (!g_list_find_custom (monitors, monitor, panel_utils_compare_xfw_gdk_monitors))
        return FALSE;
    }

  if (tasklist->all_workspaces
      || (active_ws != NULL
          && (G_UNLIKELY (xfw_workspace_get_state (active_ws) & XFW_WORKSPACE_STATE_VIRTUAL)
                ? xfw_window_is_in_viewport (child->window, active_ws)
                : xfw_window_is_on_workspace (child->window, active_ws)))
      || (tasklist->all_blinking
          && xfce_arrow_button_get_blinking (XFCE_ARROW_BUTTON (child->button))))
    {
      return (!tasklist->only_minimized
              || xfw_window_is_minimized (child->window));
    }

  return FALSE;
}



static gint
xfce_tasklist_button_compare (gconstpointer child_a,
                              gconstpointer child_b,
                              gpointer user_data)
{
  const XfceTasklistChild *a = child_a, *b = child_b;
  XfceTasklist *tasklist = XFCE_TASKLIST (user_data);
  gint retval;
  XfwApplication *app_a, *app_b;
  const gchar *name_a, *name_b;
  XfwWorkspace *workspace_a, *workspace_b;
  gint num_a = -1, num_b = -1;

  panel_return_val_if_fail (a->type == CHILD_TYPE_GROUP || XFW_IS_WINDOW (a->window), 0);
  panel_return_val_if_fail (b->type == CHILD_TYPE_GROUP || XFW_IS_WINDOW (b->window), 0);

  /* just append to the list */
  if (tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_DND)
    return a->unique_id - b->unique_id;

  if (tasklist->all_workspaces)
    {
      /* get workspace (this is slightly inefficient because the XfwWindow
       * also stores the workspace number, not the structure, and we use that
       * for comparing too */
      workspace_a = a->window != NULL ? xfw_window_get_workspace (a->window) : NULL;
      workspace_b = b->window != NULL ? xfw_window_get_workspace (b->window) : NULL;

      /* skip this if windows are in same worspace, or both pinned (== NULL) */
      if (workspace_a != workspace_b)
        {
          /* NULL means the window is pinned */
          if (workspace_a == NULL)
            workspace_a = xfw_workspace_group_get_active_workspace (tasklist->workspace_group);
          if (workspace_b == NULL)
            workspace_b = xfw_workspace_group_get_active_workspace (tasklist->workspace_group);

          /* compare by workspace number */
          if (workspace_a != NULL)
            num_a = xfw_workspace_get_number (workspace_a);
          if (workspace_b != NULL)
            num_b = xfw_workspace_get_number (workspace_b);

          if (num_a != num_b)
            return num_a - num_b;
        }
    }

  if (tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_GROUP_TITLE
      || tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP)
    {
      /* compare by app names */
      app_a = a->app;
      app_b = b->app;

      /* skip this if windows are in same group (or both NULL) */
      if (app_a != app_b)
        {
          name_a = NULL;
          name_b = NULL;

          /* get the group name if available */
          if (G_LIKELY (app_a != NULL))
            name_a = xfce_tasklist_app_get_name (app_a);
          if (G_LIKELY (app_b != NULL))
            name_b = xfce_tasklist_app_get_name (app_b);

          /* if there is no app name, use the window name */
          if (xfce_str_is_empty (name_a)
              && a->window != NULL)
            name_a = xfw_window_get_name (a->window);
          if (xfce_str_is_empty (name_b)
              && b->window != NULL)
            name_b = xfw_window_get_name (b->window);

          if (name_a == NULL)
            name_a = "";
          if (name_b == NULL)
            name_b = "";

          retval = strcasecmp (name_a, name_b);
          if (retval != 0)
            return retval;
        }
      else if (a->type != b->type)
        {
          /* put the group in front of the other window buttons
           * with the same group */
          return b->type - a->type;
        }
    }

  if (tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_TIMESTAMP
      || tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_GROUP_TIMESTAMP)
    {
      return a->unique_id - b->unique_id;
    }
  else
    {
      if (a->window != NULL)
        name_a = xfw_window_get_name (a->window);
      else if (a->app != NULL)
        name_a = xfce_tasklist_app_get_name (a->app);
      else
        name_a = NULL;

      if (b->window != NULL)
        name_b = xfw_window_get_name (b->window);
      else if (b->app != NULL)
        name_b = xfce_tasklist_app_get_name (b->app);
      else
        name_b = NULL;

      if (name_a == NULL)
        name_a = "";
      if (name_b == NULL)
        name_b = "";

      return strcasecmp (name_a, name_b);
    }
}



static void
force_box_layout_update (XfceTasklistChild *child)
{
  gint box_baseline;
  GtkAllocation box_allocation;

  /* Workarounds needed in order to force the box to layout its children.
   * Sometimes required if the icon has been resized. */
  gtk_container_check_resize (GTK_CONTAINER (child->box));
  gtk_widget_get_allocated_size (child->box, &box_allocation, &box_baseline);
  if (box_allocation.width > 0 && box_allocation.height > 0)
    gtk_widget_size_allocate_with_baseline (child->box, &box_allocation, box_baseline);
}



static void
xfce_tasklist_button_icon_changed (XfwWindow *window,
                                   XfceTasklistChild *child)
{
  GtkStyleContext *context;
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;
  XfceTasklist *tasklist = child->tasklist;
  gint icon_size, scale_factor, old_width = -1, old_height = -1;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (GTK_IS_WIDGET (child->icon));
  panel_return_if_fail (XFW_IS_WINDOW (window));
  panel_return_if_fail (child->window == window);

  /* 0 means icons are disabled */
  if (tasklist->minimized_icon_lucency == 0)
    return;

  context = gtk_widget_get_style_context (GTK_WIDGET (child->icon));
  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (child->tasklist));
  if (child->type == CHILD_TYPE_GROUP_MENU
      && !gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &icon_size, NULL))
    icon_size = 16;
  else
    icon_size = xfce_panel_plugin_get_icon_size (xfce_tasklist_get_panel_plugin (tasklist));

  if (child->tasklist->show_labels)
    {
      gint rows = MAX (child->tasklist->nrows, 1);
      rows = MAX (rows, child->tasklist->size / child->tasklist->max_button_size);
      if (xfce_tasklist_deskbar (child->tasklist))
        icon_size = MIN (icon_size, child->tasklist->max_button_size - XFCE_PANEL_PLUGIN_ICON_PADDING);
      else
        icon_size = MIN (icon_size, child->tasklist->size / rows - XFCE_PANEL_PLUGIN_ICON_PADDING);
    }

  /* get the window icon */
  pixbuf = xfw_window_get_icon (child->window, icon_size, scale_factor);

  /* leave when there is no valid pixbuf */
  if (G_UNLIKELY (pixbuf == NULL))
    {
      g_clear_object (&child->pixbuf);
      gtk_image_clear (GTK_IMAGE (child->icon));
      force_box_layout_update (child);
      return;
    }

  /* create a spotlight version of the icon when minimized */
  if (!tasklist->only_minimized
      && tasklist->minimized_icon_lucency < 100
      && xfw_window_is_minimized (window))
    {
      if (!gtk_style_context_has_class (context, "minimized"))
        gtk_style_context_add_class (context, "minimized");
    }
  else
    {
      if (gtk_style_context_has_class (context, "minimized"))
        gtk_style_context_remove_class (context, "minimized");
    }

  if (child->pixbuf != NULL)
    {
      old_width = gdk_pixbuf_get_width (child->pixbuf);
      old_height = gdk_pixbuf_get_height (child->pixbuf);
      g_object_unref (child->pixbuf);
    }

  child->pixbuf = g_object_ref (pixbuf);
  surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
  gtk_image_set_from_surface (GTK_IMAGE (child->icon), surface);
  cairo_surface_destroy (surface);

  if (old_width != gdk_pixbuf_get_width (pixbuf) || old_height != gdk_pixbuf_get_height (pixbuf))
    force_box_layout_update (child);
}



static void
xfce_tasklist_button_name_changed (XfwWindow *window,
                                   XfceTasklistChild *child)
{
  const gchar *name;
  gchar *label = NULL;
  GtkStyleContext *ctx;

  panel_return_if_fail (window == NULL || child->window == window);
  panel_return_if_fail (XFW_IS_WINDOW (child->window));
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));

  name = xfw_window_get_name (child->window);
  gtk_widget_set_tooltip_text (GTK_WIDGET (child->button), name);
  gtk_widget_set_has_tooltip (GTK_WIDGET (child->button), child->tasklist->show_tooltips);

  ctx = gtk_widget_get_style_context (child->label);
  gtk_style_context_remove_class (ctx, "label-hidden");

  if (child->tasklist->label_decorations)
    {
      /* create the button label */
      if (!child->tasklist->only_minimized
          && xfw_window_is_minimized (child->window))
        name = label = g_strdup_printf ("[%s]", name);
      else if (xfw_window_is_shaded (child->window))
        name = label = g_strdup_printf ("=%s=", name);
    }
  else
    {
      if ((!child->tasklist->only_minimized && xfw_window_is_minimized (child->window))
          || xfw_window_is_shaded (child->window))
        gtk_style_context_add_class (ctx, "label-hidden");
    }

  gtk_label_set_text (GTK_LABEL (child->label), name);
  gtk_label_set_ellipsize (GTK_LABEL (child->label), child->tasklist->ellipsize_mode);

  g_free (label);

  /* if window is null, we have not inserted the button the in
   * tasklist, so no need to sort, because we insert with sorting */
  if (window != NULL)
    xfce_tasklist_sort (child->tasklist, FALSE);
}



static void
xfce_tasklist_button_state_changed (XfwWindow *window,
                                    XfwWindowState changed_state,
                                    XfwWindowState new_state,
                                    XfceTasklistChild *child)
{
  gboolean blink;
  XfwScreen *screen;
  XfceTasklist *tasklist;
  XfwWorkspace *active_ws;
  XfceTasklistChild *temp_child;

  panel_return_if_fail (XFW_IS_WINDOW (window));
  panel_return_if_fail (child->window == window);
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));

  /* remove if the new state is hidding the window from the tasklist */
  if (PANEL_HAS_FLAG (changed_state, XFW_WINDOW_STATE_SKIP_TASKLIST))
    {
      screen = xfw_window_get_screen (window);
      tasklist = child->tasklist;

      /* remove button from tasklist */
      xfce_tasklist_window_removed (screen, window, child->tasklist);

      /* add the window to the skipped_windows list */
      xfce_tasklist_window_added (screen, window, tasklist);

      return;
    }

  /* update the button name */
  if (PANEL_HAS_FLAG (changed_state, XFW_WINDOW_STATE_SHADED | XFW_WINDOW_STATE_MINIMIZED)
      && !child->tasklist->only_minimized)
    xfce_tasklist_button_name_changed (window, child);

  /* update the button icon if needed */
  if (PANEL_HAS_FLAG (changed_state, XFW_WINDOW_STATE_MINIMIZED))
    {
      if (G_UNLIKELY (child->tasklist->only_minimized))
        {
          if (PANEL_HAS_FLAG (new_state, XFW_WINDOW_STATE_MINIMIZED))
            gtk_widget_show (child->button);
          else
            gtk_widget_hide (child->button);
        }
      else
        {
          /* update the icon opacity */
          xfce_tasklist_button_icon_changed (window, child);
          if (child->tasklist->grouping)
            {
              XfceTasklistChild *group_child = g_hash_table_lookup (child->tasklist->apps, child->app);
              xfce_tasklist_group_button_icon_changed (child->app, group_child);
            }
        }
    }

  /* update the blinking state */
  if (PANEL_HAS_FLAG (changed_state, XFW_WINDOW_STATE_URGENT))
    {
      /* only start blinking if the window requesting urgency
       * notification is not the active window */
      blink = PANEL_HAS_FLAG (new_state, XFW_WINDOW_STATE_URGENT);
      if (!blink || (blink && !xfw_window_is_active (window)))
        {
          /* if we have all_blinking set make sure we toggle visibility of the button
           * in case the window is not in the current workspace */
          active_ws = xfw_workspace_group_get_active_workspace (child->tasklist->workspace_group);
          if (child->tasklist->all_blinking && blink
              && !xfce_tasklist_button_visible (child, active_ws))
            {
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child->button), FALSE);
              gtk_widget_show (child->button);
            }

          /* update button blinking even if grouped so it is in right state when ungrouped */
          xfce_arrow_button_set_blinking (XFCE_ARROW_BUTTON (child->button), blink);

          /* also update group button blinking if needed */
          if (child->tasklist->grouping)
            {
              /* find the child for the group */
              XfceTasklistChild *group_child = g_hash_table_lookup (child->tasklist->apps, child->app);

              /* stop blinking only if no window in group needs attention */
              if (!blink)
                for (GSList *lp = group_child->windows; lp != NULL; lp = lp->next)
                  {
                    temp_child = lp->data;
                    if (xfw_window_is_urgent (temp_child->window))
                      {
                        blink = TRUE;
                        break;
                      }
                  }

              xfce_arrow_button_set_blinking (XFCE_ARROW_BUTTON (group_child->button), blink);
            }

          if (child->tasklist->all_blinking
              && !xfce_tasklist_button_visible (child, active_ws))
            gtk_widget_hide (child->button);
        }
    }
}



static void
xfce_tasklist_button_workspace_changed (XfwWindow *window,
                                        XfceTasklistChild *child)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (child->tasklist);

  panel_return_if_fail (child->window == window);
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));

  xfce_tasklist_sort (tasklist, FALSE);
  xfce_tasklist_active_window_changed (tasklist->screen, window, tasklist);
  if (!tasklist->all_workspaces)
    xfce_tasklist_active_workspace_changed (tasklist->workspace_group, NULL, tasklist);
}



static void
xfce_tasklist_button_monitors_changed (XfwWindow *window,
                                       GParamSpec *pspec,
                                       XfceTasklistChild *child)
{
  XfwWorkspace *active_ws;

  panel_return_if_fail (child->window == window);
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));
  panel_return_if_fail (XFW_IS_SCREEN (child->tasklist->screen));

  if (xfce_tasklist_filter_monitors (child->tasklist))
    {
      /* check if we need to change the visibility of the button */
      active_ws = xfw_workspace_group_get_active_workspace (child->tasklist->workspace_group);
      if (xfce_tasklist_button_visible (child, active_ws))
        gtk_widget_show (child->button);
      else
        gtk_widget_hide (child->button);
    }
}



#ifdef ENABLE_X11
static void
xfce_tasklist_button_geometry_changed (XfwWindow *window,
                                       XfceTasklistChild *child)
{
  panel_return_if_fail (child->window == window);
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));

  xfce_tasklist_wireframe_update (child->tasklist, child);
}



static gboolean
xfce_tasklist_button_leave_notify_event (GtkWidget *button,
                                         GdkEventCrossing *event,
                                         XfceTasklistChild *child)
{
  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (child->type != CHILD_TYPE_GROUP, FALSE);

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (button, xfce_tasklist_button_leave_notify_event, child);
  g_signal_handlers_disconnect_by_func (child->window, xfce_tasklist_button_geometry_changed, child);

  /* unmap and destroy the wireframe window */
  xfce_tasklist_wireframe_hide (child->tasklist);

  return FALSE;
}
#endif



static gboolean
xfce_tasklist_button_enter_notify_event (GtkWidget *button,
                                         GdkEventCrossing *event,
                                         XfceTasklistChild *child)
{
  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (child->type != CHILD_TYPE_GROUP, FALSE);
  panel_return_val_if_fail (GTK_IS_WIDGET (button), FALSE);
  panel_return_val_if_fail (XFW_IS_WINDOW (child->window), FALSE);

#ifdef ENABLE_X11
  /* leave when there is nothing to do */
  if (!child->tasklist->show_wireframes)
    return FALSE;

  /* show wireframe for the child */
  xfce_tasklist_wireframe_update (child->tasklist, child);

  /* connect signal to destroy the window when the user leaves the button */
  g_signal_connect (G_OBJECT (button), "leave-notify-event",
                    G_CALLBACK (xfce_tasklist_button_leave_notify_event), child);

  /* watch geometry changes */
  g_signal_connect (G_OBJECT (child->window), "geometry-changed",
                    G_CALLBACK (xfce_tasklist_button_geometry_changed), child);
#endif

  return FALSE;
}



static gchar *
xfce_tasklist_button_get_child_path (XfceTasklistChild *child)
{
  XfwApplicationInstance *instance = xfw_application_get_instance (child->app, child->window);
  gchar *path = NULL;

  if (instance != NULL)
    {
      gint pid = xfw_application_instance_get_pid (instance);
      if (pid > 0)
        {
          path = g_strdup_printf ("/proc/%d/exe", pid);
          if (!g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_SYMLINK))
            {
              g_free (path);
              path = NULL;
            }
        }
    }

  return path;
}



static void
xfce_tasklist_button_start_new_instance_clicked (GtkWidget *widget,
                                                 XfceTasklistChild *child)
{
  GError *error = NULL;
  gchar *path = xfce_tasklist_button_get_child_path (child);

  if (path == NULL)
    return;

  if (!g_spawn_command_line_async (path, &error))
    {
      xfce_dialog_show_error (NULL, error, _("Unable to start new instance of '%s'"), path);
      g_error_free (error);
    }

  g_free (path);
}



static void
xfce_tasklist_button_add_launch_new_instance_item (XfceTasklistChild *child,
                                                   GtkWidget *menu,
                                                   gboolean append)
{
  gchar *path;
  GtkWidget *sep;
  GtkWidget *item;

  /* add "Launch New Instance" item to menu if supported by the platform */
  path = xfce_tasklist_button_get_child_path (child);

  if (path == NULL)
    return;

  sep = gtk_separator_menu_item_new ();
  gtk_widget_show (sep);

  item = gtk_menu_item_new_with_label (_("Launch New Instance"));
  gtk_widget_show (item);
  g_signal_connect (item,
                    "activate",
                    G_CALLBACK (xfce_tasklist_button_start_new_instance_clicked),
                    child);

  if (append)
    {
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), sep);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }
  else
    {
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), sep);
      gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
    }

  g_free (path);
}



static void
xfce_tasklist_button_menu_destroy (GtkWidget *menu,
                                   XfceTasklistChild *child)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (child->button));
  panel_return_if_fail (GTK_IS_WIDGET (menu));

  panel_utils_destroy_later (menu);
  if (!xfw_window_is_active (child->window))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child->button), FALSE);
}



static gboolean
xfce_tasklist_button_button_press_event (GtkWidget *button,
                                         GdkEventButton *event,
                                         XfceTasklistChild *child)
{
  XfcePanelPlugin *plugin;
  GtkWidget *menu;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (child->type != CHILD_TYPE_GROUP, FALSE);

  if (event->type != GDK_BUTTON_PRESS
      || xfce_taskbar_is_locked (child->tasklist))
    return FALSE;

  /* send the event to the panel plugin if control is pressed */
  plugin = xfce_tasklist_get_panel_plugin (child->tasklist);
  if (PANEL_HAS_FLAG (event->state, GDK_CONTROL_MASK))
    {
      /* send the event to the panel plugin */
      if (G_LIKELY (plugin != NULL))
        gtk_widget_event (GTK_WIDGET (plugin), (GdkEvent *) event);

      return TRUE;
    }

  if (event->button == 3)
    {
      menu = xfw_window_action_menu_new (child->window);
      xfce_tasklist_button_add_launch_new_instance_item (child, menu, FALSE);
      g_signal_connect (G_OBJECT (menu), "deactivate",
                        G_CALLBACK (xfce_tasklist_button_menu_destroy), child);

      gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);
      xfce_panel_plugin_popup_menu (plugin, GTK_MENU (menu), button, (GdkEvent *) event);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      return TRUE;
    }

  return FALSE;
}



static gboolean
xfce_tasklist_button_button_release_event (GtkWidget *button,
                                           GdkEventButton *event,
                                           XfceTasklistChild *child)
{
  GtkAllocation allocation;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (child->type != CHILD_TYPE_GROUP, FALSE);

  gtk_widget_get_allocation (button, &allocation);

  /* only respond to in-button events */
  if (event->type == GDK_BUTTON_RELEASE
      && !xfce_taskbar_is_locked (child->tasklist)
      && !(event->x == 0 && event->y == 0) /* 0,0 = outside the widget in Gtk */
      && event->x >= 0 && event->x < allocation.width
      && event->y >= 0 && event->y < allocation.height)
    {
      if (event->button == 1 && !GTK_IS_MENU_ITEM (button))
        {
          /* press the button */
          return !xfce_tasklist_button_activate (child, event->time);
        }
      else if (event->button == 2)
        {
          switch (child->tasklist->middle_click)
            {
            case XFCE_TASKLIST_MIDDLE_CLICK_NOTHING:
              break;

            case XFCE_TASKLIST_MIDDLE_CLICK_CLOSE_WINDOW:
              if (child->type == CHILD_TYPE_GROUP_MENU
                  && GTK_IS_MENU_ITEM (button))
                xfce_tasklist_group_button_menu_close (button, child, event->time);
              else
                xfw_window_close (child->window, event->time, NULL);
              return TRUE;

            case XFCE_TASKLIST_MIDDLE_CLICK_MINIMIZE_WINDOW:
              if (!xfw_window_is_minimized (child->window))
                xfw_window_set_minimized (child->window, TRUE, NULL);
              return FALSE;

            case XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE:
              xfce_tasklist_button_start_new_instance_clicked (button, child);
              return TRUE;
            }
        }
    }

  return FALSE;
}



static void
xfce_tasklist_button_enter_notify_event_disconnected (gpointer data,
                                                      GClosure *closure)
{
  XfceTasklistChild *child = data;

  panel_return_if_fail (XFW_IS_WINDOW (child->window));

#ifdef ENABLE_X11
  /* we need to detach the geometry watch because that is connected
   * to the window we proxy and thus not disconnected when the
   * proxy dies */
  g_signal_handlers_disconnect_by_func (child->window, xfce_tasklist_button_geometry_changed, child);
#endif

  g_object_unref (G_OBJECT (child->window));
}



static void
xfce_tasklist_button_proxy_menu_item_activate (GtkMenuItem *mi,
                                               XfceTasklistChild *child)
{
  gint64 timestamp;

  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));
  panel_return_if_fail (GTK_IS_MENU_ITEM (mi));

  timestamp = g_get_real_time () / 1000;
  xfce_tasklist_button_activate (child, timestamp);
}



static GtkWidget *
xfce_tasklist_button_proxy_menu_item (XfceTasklistChild *child,
                                      gboolean allow_wireframe)
{
  GtkWidget *mi;
  GtkWidget *image;
  GtkWidget *label;
  GtkStyleContext *context_button;
  GtkStyleContext *context_menuitem;
  GtkCssProvider *provider;
  gchar *label_text = NULL, *css_string;
  XfceTasklist *tasklist = child->tasklist;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), NULL);
  panel_return_val_if_fail (child->type == CHILD_TYPE_OVERFLOW_MENU
                              || child->type == CHILD_TYPE_GROUP_MENU,
                            NULL);
  panel_return_val_if_fail (GTK_IS_LABEL (child->label), NULL);
  panel_return_val_if_fail (XFW_IS_WINDOW (child->window), NULL);

  mi = panel_image_menu_item_new ();
  g_object_bind_property (G_OBJECT (child->label), "label",
                          G_OBJECT (mi), "label",
                          G_BINDING_SYNC_CREATE);

  if (tasklist->show_tooltips)
    g_object_bind_property (G_OBJECT (child->label), "label",
                            G_OBJECT (mi), "tooltip-text",
                            G_BINDING_SYNC_CREATE);

  label = gtk_bin_get_child (GTK_BIN (mi));
  panel_return_val_if_fail (GTK_IS_LABEL (label), NULL);
  gtk_label_set_max_width_chars (GTK_LABEL (label), tasklist->menu_max_width_chars);
  gtk_label_set_ellipsize (GTK_LABEL (label), tasklist->ellipsize_mode);

  if (xfw_window_is_active (child->window))
    label_text = g_strdup_printf ("<b><i>%s</i></b>", gtk_label_get_text (GTK_LABEL (label)));
  else if (xfw_window_is_urgent (child->window))
    label_text = g_strdup_printf ("<b>%s</b>", gtk_label_get_text (GTK_LABEL (label)));

  if (label_text != NULL)
    {
      gtk_label_set_markup (GTK_LABEL (label), label_text);
      g_free (label_text);
    }

  image = gtk_image_new ();
  panel_image_menu_item_set_image (mi, image);
  /* sync the minimized state css style class between the button and the menuitem */
  context_button = gtk_widget_get_style_context (GTK_WIDGET (child->icon));
  context_menuitem = gtk_widget_get_style_context (GTK_WIDGET (image));

  provider = gtk_css_provider_new ();
  /* silly workaround for gtkcss only accepting "." as decimal separator and floats returning
     with "," as decimal separator in some locales */
  css_string = g_strdup_printf ("image { padding: 3px; } image.minimized { opacity: %d.%02d; }",
                                tasklist->minimized_icon_lucency / 100,
                                tasklist->minimized_icon_lucency % 100);
  gtk_css_provider_load_from_data (provider, css_string, -1, NULL);
  gtk_style_context_add_provider (context_menuitem,
                                  GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  g_free (css_string);

  if (gtk_style_context_has_class (context_button, "minimized"))
    {
      if (!gtk_style_context_has_class (context_menuitem, "minimized"))
        gtk_style_context_add_class (context_menuitem, "minimized");
    }
  else
    {
      if (gtk_style_context_has_class (context_menuitem, "minimized"))
        gtk_style_context_remove_class (context_menuitem, "minimized");
    }

  gtk_image_set_pixel_size (GTK_IMAGE (image), GTK_ICON_SIZE_MENU);
  g_object_bind_property (G_OBJECT (child->icon), "surface",
                          G_OBJECT (image), "surface",
                          G_BINDING_SYNC_CREATE);
  gtk_widget_show (image);

  if (allow_wireframe)
    {
      g_object_ref (G_OBJECT (child->window));
      g_signal_connect_data (G_OBJECT (mi), "enter-notify-event",
                             G_CALLBACK (xfce_tasklist_button_enter_notify_event), child,
                             xfce_tasklist_button_enter_notify_event_disconnected, 0);
    }

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (xfce_tasklist_button_proxy_menu_item_activate), child);
  g_signal_connect (G_OBJECT (mi), "button-release-event",
                    G_CALLBACK (xfce_tasklist_button_button_release_event), child);

  /* TODO item dnd */

  return mi;
}



static gboolean
xfce_tasklist_button_activate (XfceTasklistChild *child,
                               guint32 timestamp)
{
  XfwWorkspace *workspace;
  GdkScreen *screen;
  GdkRectangle window_geom, workspace_geom;
  gint screen_width, screen_height;
  guint scale_factor;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (child->tasklist), FALSE);
  panel_return_val_if_fail (XFW_IS_WINDOW (child->window), FALSE);
  panel_return_val_if_fail (XFW_IS_SCREEN (child->tasklist->screen), FALSE);

  if (xfw_window_is_active (child->window))
    {
      /* minimize does not work when this is assigned to the
       * middle mouse button */
      if (child->tasklist->middle_click == XFCE_TASKLIST_MIDDLE_CLICK_MINIMIZE_WINDOW)
        return FALSE;

      xfw_window_set_minimized (child->window, TRUE, NULL);
    }
  else
    {
      /* we only change worksapces/viewports for non-pinned windows
       * and if all workspaces/viewports are shown or if we have
       * all blinking enabled and the current button is blinking */
      if ((child->tasklist->all_workspaces && !xfw_window_is_pinned (child->window))
          || (child->tasklist->all_blinking
              && xfce_arrow_button_get_blinking (XFCE_ARROW_BUTTON (child->button))))
        {
          screen = gtk_widget_get_screen (GTK_WIDGET (child->tasklist));
          scale_factor = gdk_window_get_scale_factor (gtk_widget_get_window (GTK_WIDGET (child->tasklist)));
          workspace = xfw_window_get_workspace (child->window);

          /* only switch workspaces/viewports if switch_workspace is enabled or
           * we want to restore a minimized window to the current workspace/viewport */
          if (workspace != NULL
              && (child->tasklist->switch_workspace
                  || !xfw_window_is_minimized (child->window)))
            {
              if (G_UNLIKELY (xfw_workspace_get_state (workspace) & XFW_WORKSPACE_STATE_VIRTUAL))
                {
                  if (!xfw_window_is_in_viewport (child->window, workspace))
                    {
                      /* viewport info */
                      workspace_geom = *(xfw_workspace_get_geometry (workspace));
                      screen_width = panel_screen_get_width (screen) * scale_factor;
                      screen_height = panel_screen_get_height (screen) * scale_factor;

                      /* we only support multiple viewports like compiz has
                       * (all equally spread across the screen) */
                      if ((workspace_geom.width % screen_width) == 0
                          && (workspace_geom.height % screen_height) == 0)
                        {
                          window_geom = *(xfw_window_get_geometry (child->window));

                          /* lookup nearest workspace edge */
                          workspace_geom.x = window_geom.x - (window_geom.x % screen_width);
                          workspace_geom.x = CLAMP (workspace_geom.x, 0, workspace_geom.width - screen_width);

                          workspace_geom.y = window_geom.y - (window_geom.y % screen_height);
                          workspace_geom.y = CLAMP (workspace_geom.y, 0, workspace_geom.height - screen_height);

                          /* move to the other viewport */
                          xfw_workspace_group_move_viewport (child->tasklist->workspace_group,
                                                             workspace_geom.x, workspace_geom.y, NULL);
                        }
                      else
                        {
                          g_warning ("only viewport with equally distributed screens are supported: %dx%d & %dx%d",
                                     workspace_geom.width, workspace_geom.height, screen_width, screen_height);
                        }
                    }
                }
              else if (xfw_workspace_group_get_active_workspace (child->tasklist->workspace_group) != workspace)
                {
                  /* switch to the other workspace before we activate the window */
                  xfw_workspace_activate (workspace, NULL);
                  gtk_main_iteration ();
                }
            }
          else if (workspace != NULL
                   && xfw_workspace_get_state (workspace) & XFW_WORKSPACE_STATE_VIRTUAL
                   && !xfw_window_is_in_viewport (child->window, workspace))
            {
              /* viewport info */
              workspace_geom = *(xfw_workspace_get_geometry (workspace));
              screen_width = panel_screen_get_width (screen) * scale_factor;
              screen_height = panel_screen_get_height (screen) * scale_factor;

              /* we only support multiple viewports like compiz has
               * (all equaly spread across the screen) */
              if ((workspace_geom.width % screen_width) == 0
                  && (workspace_geom.height % screen_height) == 0)
                {
                  /* note that the x and y might be negative numbers, since they are relative
                   * to the current screen, not to the edge of the screen they are on. this is
                   * not a problem since the mod result will always be positive */
                  window_geom = *(xfw_window_get_geometry (child->window));

                  /* get the new screen position, with the same screen offset */
                  window_geom.x = workspace_geom.x + (window_geom.x % screen_width);
                  window_geom.y = workspace_geom.y + (window_geom.y % screen_height);
                  window_geom.width = -1;
                  window_geom.height = -1;

                  /* move the window */
                  xfw_window_set_geometry (child->window, &window_geom, NULL);
                }
              else
                {
                  g_warning ("only viewport with equally distributed screens are supported: %dx%d & %dx%d",
                             workspace_geom.width, workspace_geom.height, screen_width, screen_height);
                }
            }
        }

      xfw_window_activate (child->window, NULL, timestamp, NULL);
    }

  return TRUE;
}



static void
xfce_tasklist_button_drag_data_get (GtkWidget *button,
                                    GdkDragContext *context,
                                    GtkSelectionData *selection_data,
                                    guint info,
                                    guint timestamp,
                                    XfceTasklistChild *child)
{
  gulong wid;

  panel_return_if_fail (XFW_IS_WINDOW (child->window));

  wid = tasklist_window_get_wid (child->window);
  gtk_selection_data_set (selection_data,
                          gtk_selection_data_get_target (selection_data),
                          8, (guchar *) &wid, sizeof (gulong));
}



static void
xfce_tasklist_button_drag_begin (GtkWidget *button,
                                 GdkDragContext *context,
                                 XfceTasklistChild *child)
{
  GdkPixbuf *pixbuf;
  gint size, scale_factor;

  panel_return_if_fail (XFW_IS_WINDOW (child->window));

  if (!gtk_icon_size_lookup (GTK_ICON_SIZE_DND, &size, NULL))
    size = 32;
  scale_factor = gtk_widget_get_scale_factor (button);
  pixbuf = xfw_window_get_icon (child->window, size, scale_factor);
  if (G_LIKELY (pixbuf != NULL))
    {
      cairo_surface_t *surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
      gtk_drag_set_icon_surface (context, surface);
      cairo_surface_destroy (surface);
    }
}



static void
xfce_tasklist_button_drag_data_received (GtkWidget *button,
                                         GdkDragContext *context,
                                         gint x,
                                         gint y,
                                         GtkSelectionData *selection_data,
                                         guint info,
                                         guint drag_time,
                                         XfceTasklistChild *child2)
{
  GList *li, *sibling;
  gulong wid;
  XfceTasklistChild *child;
  XfceTasklist *tasklist = XFCE_TASKLIST (child2->tasklist);
  GtkAllocation allocation;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->sort_order != XFCE_TASKLIST_SORT_ORDER_DND)
    return;

  gtk_widget_get_allocation (button, &allocation);

  sibling = g_list_find (tasklist->windows, child2);
  panel_return_if_fail (sibling != NULL);

  if ((xfce_tasklist_horizontal (tasklist) && x >= allocation.width / 2)
      || (!xfce_tasklist_horizontal (tasklist) && y >= allocation.height / 2))
    sibling = g_list_next (sibling);

  wid = *((gulong *) (gpointer) gtk_selection_data_get_data (selection_data));
  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      if (sibling != li /* drop on end previous button */
          && child != child2 /* drop on the same button */
          && g_list_next (li) != sibling /* drop start of next button */
          && child->window != NULL
          && tasklist_window_get_wid (child->window) == wid)
        {
          /* swap items */
          tasklist->windows = g_list_delete_link (tasklist->windows, li);
          tasklist->windows = g_list_insert_before (tasklist->windows, sibling, child);

          gtk_widget_queue_resize (GTK_WIDGET (tasklist));

          break;
        }
    }
}



static XfceTasklistChild *
xfce_tasklist_button_new (XfwWindow *window,
                          XfceTasklist *tasklist)
{
  XfceTasklistChild *child;
  static guint unique_id_counter = 0;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), NULL);
  panel_return_val_if_fail (XFW_IS_WINDOW (window), NULL);

  /* avoid integer overflows */
  if (G_UNLIKELY (unique_id_counter >= G_MAXUINT))
    unique_id_counter = 0;

  child = xfce_tasklist_child_new (tasklist);
  child->type = CHILD_TYPE_WINDOW;
  child->window = window;
  child->app = xfw_window_get_application (window);
  child->unique_id = unique_id_counter++;

  /* drag and drop to the pager */
  gtk_drag_source_set (child->button, GDK_BUTTON1_MASK,
                       source_targets, G_N_ELEMENTS (source_targets),
                       GDK_ACTION_MOVE);
  gtk_drag_dest_set (child->button, GTK_DEST_DEFAULT_DROP,
                     source_targets, G_N_ELEMENTS (source_targets),
                     GDK_ACTION_MOVE);
  g_signal_connect (G_OBJECT (child->button), "drag-data-get",
                    G_CALLBACK (xfce_tasklist_button_drag_data_get), child);
  g_signal_connect (G_OBJECT (child->button), "drag-begin",
                    G_CALLBACK (xfce_tasklist_button_drag_begin), child);
  g_signal_connect (G_OBJECT (child->button), "drag-data-received",
                    G_CALLBACK (xfce_tasklist_button_drag_data_received), child);

  /* note that the same signals should be in the proxy menu item too */
  g_signal_connect (G_OBJECT (child->button), "enter-notify-event",
                    G_CALLBACK (xfce_tasklist_button_enter_notify_event), child);
  g_signal_connect (G_OBJECT (child->button), "button-press-event",
                    G_CALLBACK (xfce_tasklist_button_button_press_event), child);
  g_signal_connect (G_OBJECT (child->button), "button-release-event",
                    G_CALLBACK (xfce_tasklist_button_button_release_event), child);

  /* monitor window changes */
  g_signal_connect (G_OBJECT (window), "icon-changed",
                    G_CALLBACK (xfce_tasklist_button_icon_changed), child);
  g_signal_connect (G_OBJECT (window), "name-changed",
                    G_CALLBACK (xfce_tasklist_button_name_changed), child);
  g_signal_connect (G_OBJECT (window), "state-changed",
                    G_CALLBACK (xfce_tasklist_button_state_changed), child);
  g_signal_connect (G_OBJECT (window), "workspace-changed",
                    G_CALLBACK (xfce_tasklist_button_workspace_changed), child);
  g_signal_connect (G_OBJECT (window), "notify::monitors",
                    G_CALLBACK (xfce_tasklist_button_monitors_changed), child);

  /* poke functions */
  xfce_tasklist_button_icon_changed (window, child);
  xfce_tasklist_button_name_changed (NULL, child);

  /* insert */
  tasklist->windows = g_list_insert_sorted_with_data (tasklist->windows, child,
                                                      xfce_tasklist_button_compare,
                                                      tasklist);

  return child;
}



/**
 * Group Buttons
 **/
static void
xfce_tasklist_group_button_menu_minimize_all (XfceTasklistChild *group_child)
{
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          panel_return_if_fail (XFW_IS_WINDOW (child->window));
          xfw_window_set_minimized (child->window, TRUE, NULL);
        }
    }
}



static void
xfce_tasklist_group_button_menu_unminimize_all (XfceTasklistChild *group_child)
{
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          panel_return_if_fail (XFW_IS_WINDOW (child->window));
          xfw_window_set_minimized (child->window, FALSE, NULL);
        }
    }
}



static void
xfce_tasklist_group_button_menu_maximize_all (XfceTasklistChild *group_child)
{
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          panel_return_if_fail (XFW_IS_WINDOW (child->window));
          xfw_window_set_maximized (child->window, TRUE, NULL);
        }
    }
}



static void
xfce_tasklist_group_button_menu_unmaximize_all (XfceTasklistChild *group_child)
{
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          panel_return_if_fail (XFW_IS_WINDOW (child->window));
          xfw_window_set_maximized (child->window, FALSE, NULL);
        }
    }
}



static void
xfce_tasklist_group_button_menu_close_all (XfceTasklistChild *group_child)
{
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          panel_return_if_fail (XFW_IS_WINDOW (child->window));
          xfw_window_close (child->window, gtk_get_current_event_time (), NULL);
        }
    }
}



static void
xfce_tasklist_group_button_menu_close (GtkWidget *menuitem,
                                       XfceTasklistChild *child,
                                       guint32 time)
{
  GtkWidget *menu = gtk_widget_get_parent (menuitem);

  panel_return_if_fail (XFW_IS_WINDOW (child->window));
  panel_return_if_fail (GTK_IS_MENU (menu));

  gtk_container_remove (GTK_CONTAINER (menu), menuitem);
  gtk_menu_popdown (GTK_MENU (menu));
  xfw_window_close (child->window, time, NULL);
}



static GtkWidget *
xfce_tasklist_group_button_menu (XfceTasklistChild *group_child,
                                 gboolean action_menu_entries)
{
  GSList *li;
  XfceTasklistChild *child;
  GtkWidget *mi;
  GtkWidget *menu;
  GtkWidget *image;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (group_child->tasklist), NULL);
  panel_return_val_if_fail (XFW_IS_APPLICATION (group_child->app), NULL);

  menu = gtk_menu_new ();

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        {
          mi = xfce_tasklist_button_proxy_menu_item (child, !action_menu_entries);
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
          gtk_widget_show (mi);

          if (action_menu_entries)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi),
                                       xfw_window_action_menu_new (child->window));

          if (li->next == NULL)
            xfce_tasklist_button_add_launch_new_instance_item (child, menu, TRUE);
        }
    }

  if (action_menu_entries)
    {
      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_show (mi);

      image = gtk_image_new_from_icon_name ("window-minimize-symbolic", GTK_ICON_SIZE_MENU);
      mi = panel_image_menu_item_new_with_mnemonic (_("Mi_nimize All"));
      panel_image_menu_item_set_image (mi, image);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                G_CALLBACK (xfce_tasklist_group_button_menu_minimize_all), group_child);
      gtk_widget_show_all (mi);

      mi = gtk_menu_item_new_with_mnemonic (_("Un_minimize All"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                G_CALLBACK (xfce_tasklist_group_button_menu_unminimize_all), group_child);
      gtk_widget_show (mi);

      image = gtk_image_new_from_icon_name ("window-maximize-symbolic", GTK_ICON_SIZE_MENU);
      mi = panel_image_menu_item_new_with_mnemonic (_("Ma_ximize All"));
      panel_image_menu_item_set_image (mi, image);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                G_CALLBACK (xfce_tasklist_group_button_menu_maximize_all), group_child);
      gtk_widget_show_all (mi);

      mi = gtk_menu_item_new_with_mnemonic (_("_Unmaximize All"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                G_CALLBACK (xfce_tasklist_group_button_menu_unmaximize_all), group_child);
      gtk_widget_show (mi);

      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_show (mi);

      image = gtk_image_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_MENU);
      mi = panel_image_menu_item_new_with_mnemonic (_("_Close All"));
      panel_image_menu_item_set_image (mi, image);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                G_CALLBACK (xfce_tasklist_group_button_menu_close_all), group_child);
      gtk_widget_show_all (mi);
    }

  return menu;
}



static void
xfce_tasklist_group_button_menu_destroy (GtkWidget *menu,
                                         XfceTasklistChild *group_child)
{
  GSList *lp;

  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (group_child->button));
  panel_return_if_fail (GTK_IS_WIDGET (menu));

  panel_utils_destroy_later (menu);

  /* restore button state if inactive */
  for (lp = group_child->windows; lp != NULL; lp = lp->next)
    {
      XfceTasklistChild *child = lp->data;
      if (xfw_window_is_active (child->window))
        break;
    }
  if (lp == NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (group_child->button), FALSE);

#ifdef ENABLE_X11
  /* make sure the wireframe is hidden */
  xfce_tasklist_wireframe_hide (group_child->tasklist);
#endif
}



static gboolean
xfce_tasklist_group_button_button_draw (GtkWidget *widget,
                                        cairo_t *cr,
                                        XfceTasklistChild *group_child)
{
  if (group_child->n_windows > 1)
    {
      GtkStyleContext *context;
      GtkAllocation allocation;
      PangoRectangle ink_extent, log_extent;
      PangoFontDescription *desc;
      PangoLayout *n_windows_layout;
      gchar *n_windows;
      GdkRGBA fg, bg;
      gdouble radius, x, y;
      GdkRectangle icon_pixbuf_rect = { 0 };

      gtk_widget_get_allocation (GTK_WIDGET (widget), &allocation);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      /* Get the theme fg color for drawing the circle background. We then use a
         simple calculation to decide whether the background color - which is ironically
         used for the text - should be white or black.
         We use the theme fg color alpha for the fg and the bg for consistency. */
      context = gtk_widget_get_style_context (widget);
      gtk_style_context_get_color (context, gtk_style_context_get_state (context), &fg);
      /* The magical number 1.5 is a third of the sum of the max rgb values */
      if ((fg.red + fg.green + fg.blue) < 1.5)
        gdk_rgba_parse (&bg, "#ffffff");
      else
        gdk_rgba_parse (&bg, "#000000");

      n_windows = g_strdup_printf ("%d", group_child->n_windows);
      n_windows_layout = gtk_widget_create_pango_layout (GTK_WIDGET (widget), n_windows);
      desc = pango_font_description_from_string ("Mono Bold 8");
      if (desc)
        {
          pango_layout_set_font_description (n_windows_layout, desc);
          pango_font_description_free (desc);
        }

      if (group_child->pixbuf != NULL)
        {
          gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (group_child->tasklist));
          icon_pixbuf_rect.width = gdk_pixbuf_get_width (group_child->pixbuf) / scale_factor;
          icon_pixbuf_rect.height = gdk_pixbuf_get_height (group_child->pixbuf) / scale_factor;
        }

      pango_layout_get_pixel_extents (n_windows_layout, &ink_extent, &log_extent);
      radius = log_extent.height / 2;
      if (group_child->tasklist->show_labels)
        {
          GdkPoint icon_coords = {};
          g_warn_if_fail (gtk_widget_translate_coordinates (group_child->icon, widget, 0, 0, &icon_coords.x, &icon_coords.y));

          if (xfce_tasklist_vertical (group_child->tasklist))
            {
              x = allocation.width / 2 + icon_pixbuf_rect.width / 2;
              y = icon_coords.y + icon_pixbuf_rect.height;
            }
          else
            {
              x = icon_coords.x + icon_pixbuf_rect.width;
              y = allocation.height / 2 + icon_pixbuf_rect.height / 2;
            }
        }
      else
        {
          if (xfce_tasklist_vertical (group_child->tasklist))
            {
              x = allocation.width / 2 + icon_pixbuf_rect.width / 2;
              y = allocation.width / 2 + icon_pixbuf_rect.height / 2;
            }
          else
            {
              x = allocation.height / 2 + icon_pixbuf_rect.width / 2;
              y = allocation.height / 2 + icon_pixbuf_rect.height / 2;
            }
        }

      if (x + radius > allocation.width - 2)
        x = allocation.width - radius - 2;
      if (y + radius > allocation.height - 2)
        y = allocation.height - radius - 2;
      if (G_UNLIKELY (x - radius < 0))
        x = radius;
      if (G_UNLIKELY (y - radius < 0))
        y = radius;

      /* Draw the background circle */
      cairo_move_to (cr, x, y);
      cairo_arc (cr, x, y, radius, 0.0, 2 * M_PI);
      cairo_close_path (cr);
      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, bg.red, bg.green, bg.blue, fg.alpha);
      cairo_stroke_preserve (cr);
      cairo_set_source_rgba (cr, fg.red, fg.green, fg.blue, fg.alpha);
      cairo_fill (cr);

      /* Draw the number of windows */
      cairo_move_to (cr,
                     x - (log_extent.width / 2),
                     y - (log_extent.height / 2) + 0.25);
      cairo_set_source_rgba (cr, bg.red, bg.green, bg.blue, fg.alpha);
      pango_cairo_show_layout (cr, n_windows_layout);

      g_object_unref (n_windows_layout);
      g_free (n_windows);
    }

  return FALSE;
}



static gboolean
xfce_tasklist_group_button_button_press_event (GtkWidget *button,
                                               GdkEventButton *event,
                                               XfceTasklistChild *group_child)
{
  XfcePanelPlugin *plugin;
  GtkWidget *menu;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (group_child->tasklist), FALSE);
  panel_return_val_if_fail (group_child->type == CHILD_TYPE_GROUP, FALSE);

  if (event->type != GDK_BUTTON_PRESS
      || xfce_taskbar_is_locked (group_child->tasklist))
    return FALSE;

  /* send the event to the panel plugin if control is pressed */
  plugin = xfce_tasklist_get_panel_plugin (group_child->tasklist);
  if (PANEL_HAS_FLAG (event->state, GDK_CONTROL_MASK))
    {
      /* send the event to the panel plugin */
      if (G_LIKELY (plugin != NULL))
        gtk_widget_event (GTK_WIDGET (plugin), (GdkEvent *) event);

      return TRUE;
    }

  if (event->button == 1 || event->button == 3)
    {
      menu = xfce_tasklist_group_button_menu (group_child, event->button == 3);
      g_signal_connect (G_OBJECT (menu), "deactivate",
                        G_CALLBACK (xfce_tasklist_group_button_menu_destroy), group_child);

      gtk_menu_attach_to_widget (GTK_MENU (menu), button, NULL);
      xfce_panel_plugin_popup_menu (plugin, GTK_MENU (menu), button, (GdkEvent *) event);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
      return TRUE;
    }

  return FALSE;
}



static gboolean
xfce_tasklist_group_button_button_release_event (GtkWidget *button,
                                                 GdkEventButton *event,
                                                 XfceTasklistChild *group_child)
{
  GtkAllocation allocation;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (group_child->tasklist), FALSE);
  panel_return_val_if_fail (group_child->type == CHILD_TYPE_GROUP, FALSE);

  gtk_widget_get_allocation (button, &allocation);

  /* only respond to in-button events */
  if (event->type == GDK_BUTTON_RELEASE
      && !xfce_taskbar_is_locked (group_child->tasklist)
      && !(event->x == 0 && event->y == 0) /* 0,0 = outside the widget in Gtk */
      && event->x >= 0 && event->x < allocation.width
      && event->y >= 0 && event->y < allocation.height)
    {
      if (event->button == 2)
        {
          switch (group_child->tasklist->middle_click)
            {
            case XFCE_TASKLIST_MIDDLE_CLICK_NOTHING:
              break;

            case XFCE_TASKLIST_MIDDLE_CLICK_CLOSE_WINDOW:
              xfce_tasklist_group_button_menu_close_all (group_child);
              return TRUE;

            case XFCE_TASKLIST_MIDDLE_CLICK_MINIMIZE_WINDOW:
              xfce_tasklist_group_button_menu_minimize_all (group_child);
              return TRUE;

            case XFCE_TASKLIST_MIDDLE_CLICK_NEW_INSTANCE:
              xfce_tasklist_button_start_new_instance_clicked (button,
                                                               group_child->windows->data);
              return TRUE;
            }
        }
    }

  return FALSE;
}



static void
xfce_tasklist_group_button_button_size_allocate (GtkWidget *widget,
                                                 GtkAllocation *allocation,
                                                 XfceTasklistChild *child)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (child->tasklist));
  panel_return_if_fail (child->type == CHILD_TYPE_GROUP);
  xfce_tasklist_group_button_icon_changed (child->app, child);
}



static void
xfce_tasklist_group_button_name_changed (XfwApplication *app,
                                         GParamSpec *pspec,
                                         XfceTasklistChild *group_child)
{
  const gchar *name;
  GSList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (app == NULL || group_child->app == app);
  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  /* count number of windows in the menu */
  for (li = group_child->windows, group_child->n_windows = 0; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button)
          && child->type == CHILD_TYPE_GROUP_MENU)
        group_child->n_windows++;
    }

  /* create the button label */
  name = xfce_tasklist_app_get_name (group_child->app);
  gtk_label_set_text (GTK_LABEL (group_child->label), name);

  /* don't sort if there is no need to update the sorting (ie. only number
   * of windows is changed or button is not inserted in the tasklist yet */
  if (app != NULL)
    xfce_tasklist_sort (group_child->tasklist, FALSE);
}



static void
xfce_tasklist_group_button_icon_changed (XfwApplication *app,
                                         XfceTasklistChild *group_child)
{
  GtkStyleContext *context;
  GdkPixbuf *pixbuf;
  cairo_surface_t *surface;
  GSList *li;
  gboolean all_minimized_in_group = TRUE;
  gint icon_size, scale_factor;

  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (XFW_IS_APPLICATION (app));
  panel_return_if_fail (group_child->app == app);
  panel_return_if_fail (GTK_IS_WIDGET (group_child->icon));

  /* 0 means icons are disabled, although the grouping button does
   * not use lucient icons */
  if (group_child->tasklist->minimized_icon_lucency == 0)
    return;

  icon_size = xfce_panel_plugin_get_icon_size (xfce_tasklist_get_panel_plugin (group_child->tasklist));

  if (group_child->tasklist->show_labels)
    {
      gint rows = MAX (group_child->tasklist->nrows, 1);
      rows = MAX (rows, group_child->tasklist->size / group_child->tasklist->max_button_size);
      if (xfce_tasklist_deskbar (group_child->tasklist))
        icon_size = MIN (icon_size, group_child->tasklist->max_button_size - XFCE_PANEL_PLUGIN_ICON_PADDING);
      else
        icon_size = MIN (icon_size, group_child->tasklist->size / rows - XFCE_PANEL_PLUGIN_ICON_PADDING);
    }

  scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (group_child->tasklist));
  context = gtk_widget_get_style_context (GTK_WIDGET (group_child->icon));

  /* get the app icon */
  pixbuf = xfw_application_get_icon (app, icon_size, scale_factor);

  /* check if all the windows in the group are minimized */
  for (li = group_child->windows; li != NULL; li = li->next)
    {
      XfceTasklistChild *child = li->data;
      if (!xfw_window_is_minimized (child->window))
        {
          all_minimized_in_group = FALSE;
          break;
        }
    }
  /* if the icons in the group are ALL minimized, then display
   * a minimized lucent effect for the group icon too */
  if (!group_child->tasklist->only_minimized
      && group_child->tasklist->minimized_icon_lucency < 100
      && all_minimized_in_group)
    {
      if (!gtk_style_context_has_class (context, "minimized"))
        gtk_style_context_add_class (context, "minimized");
    }
  else
    {
      if (gtk_style_context_has_class (context, "minimized"))
        gtk_style_context_remove_class (context, "minimized");
    }

  if (G_LIKELY (pixbuf != NULL))
    {
      gint old_width = -1, old_height = -1;

      if (group_child->pixbuf != NULL)
        {
          old_width = gdk_pixbuf_get_width (group_child->pixbuf);
          old_height = gdk_pixbuf_get_height (group_child->pixbuf);
          g_object_unref (group_child->pixbuf);
        }

      group_child->pixbuf = g_object_ref (pixbuf);
      surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale_factor, NULL);
      gtk_image_set_from_surface (GTK_IMAGE (group_child->icon), surface);
      cairo_surface_destroy (surface);

      if (old_width != gdk_pixbuf_get_width (pixbuf) || old_height != gdk_pixbuf_get_height (pixbuf))
        force_box_layout_update (group_child);
    }
  else
    {
      g_clear_object (&group_child->pixbuf);
      gtk_image_clear (GTK_IMAGE (group_child->icon));
      force_box_layout_update (group_child);
    }
}



static void
xfce_tasklist_group_button_remove (XfceTasklistChild *group_child)
{
  GSList *li;
  guint n;
  XfceTasklistChild *child;

  /* leave if hash table triggers this function where no group
   * child is set */
  if (group_child == NULL)
    return;

  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));
  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (g_list_find (group_child->tasklist->windows, group_child) != NULL);

  /* disconnect from all the group watch functions */
  n = g_signal_handlers_disconnect_matched (G_OBJECT (group_child->app),
                                            G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, group_child);
  panel_return_if_fail (n == 2);

  /* disconnect from visible windows */
  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      panel_return_if_fail (GTK_IS_BUTTON (child->button));
      n = g_signal_handlers_disconnect_matched (G_OBJECT (child->button),
                                                G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, group_child);
      panel_return_if_fail (n == 2);
      n = g_signal_handlers_disconnect_matched (G_OBJECT (child->window),
                                                G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, group_child);
      panel_return_if_fail (n == 2);
    }

  g_slist_free (group_child->windows);
  group_child->windows = NULL;

  /* destroy the button, this will free the remaining child
   * data in the container remove function */
  gtk_widget_destroy (group_child->button);
}



static void
xfce_tasklist_group_button_keep_dnd_position (XfceTasklistChild *group_child,
                                              XfceTasklistChild *sibling,
                                              XfceTasklistChild *moved)
{
  XfceTasklist *tasklist = group_child->tasklist;

  tasklist->windows = g_list_remove (tasklist->windows, moved);
  for (GList *lp = tasklist->windows; lp != NULL; lp = lp->next)
    if (lp->data == sibling)
      {
        tasklist->windows = g_list_insert_before (tasklist->windows, lp, moved);
        break;
      }
}



static void
xfce_tasklist_group_button_child_visible_changed (XfceTasklistChild *group_child)
{
  XfceTasklistChild *child;
  GSList *li;
  gint visible_counter = 0;
  XfceTasklistChildType type;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));
  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (group_child->tasklist->grouping);
  panel_return_if_fail (group_child->windows != NULL);

  /* the group id is defined below as that of the last added window */
  group_child->unique_id = 0;

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button))
        {
          visible_counter++;
          group_child->unique_id = MAX (group_child->unique_id, child->unique_id);
        }
    }

  if (visible_counter > 1)
    {
      if (group_child->tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_DND
          && !gtk_widget_get_visible (group_child->button))
        xfce_tasklist_group_button_keep_dnd_position (group_child, group_child->windows->data,
                                                      group_child);

      /* show the button and take the windows */
      gtk_widget_show (group_child->button);
      type = CHILD_TYPE_GROUP_MENU;
    }
  else
    {
      if (group_child->tasklist->sort_order == XFCE_TASKLIST_SORT_ORDER_DND
          && gtk_widget_get_visible (group_child->button))
        xfce_tasklist_group_button_keep_dnd_position (group_child, group_child,
                                                      group_child->windows->data);

      /* hide the button and ungroup the buttons */
      gtk_widget_hide (group_child->button);
      type = CHILD_TYPE_WINDOW;
    }

  for (li = group_child->windows; li != NULL; li = li->next)
    {
      child = li->data;
      if (gtk_widget_get_visible (child->button))
        child->type = type;
    }

  xfce_tasklist_group_button_name_changed (group_child->app, NULL, group_child);

  /* update group button urgency blinking if needed: do this last as it may change window
   * buttons visibility and therefore be recursive */
  if (visible_counter > 1)
    xfce_tasklist_button_state_changed (child->window, XFW_WINDOW_STATE_URGENT,
                                        xfw_window_is_urgent (child->window) ? XFW_WINDOW_STATE_URGENT : 0,
                                        child);
}



static void
xfce_tasklist_group_button_child_destroyed (XfceTasklistChild *group_child,
                                            GtkWidget *child_button)
{
  GSList *li, *lnext;
  XfceTasklistChild *child;
  guint n_children;

  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (GTK_IS_BUTTON (child_button));
  panel_return_if_fail (group_child->windows != NULL);
  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));

  for (li = group_child->windows, n_children = 0; li != NULL; li = lnext)
    {
      child = li->data;
      lnext = li->next;
      if (G_UNLIKELY (child->button == child_button))
        group_child->windows = g_slist_delete_link (group_child->windows, li);
      else
        n_children++;
    }

  if (n_children > 0)
    {
      xfce_tasklist_group_button_child_visible_changed (group_child);
    }
  else
    {
      g_hash_table_remove (group_child->tasklist->apps, group_child->app);
    }
}



static void
xfce_tasklist_group_button_sort (XfceTasklistChild *group_child)
{
  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);

  if (group_child->tasklist->sort_order != XFCE_TASKLIST_SORT_ORDER_DND)
    group_child->windows = g_slist_sort_with_data (group_child->windows,
                                                   xfce_tasklist_button_compare,
                                                   group_child->tasklist);
}



static void
xfce_tasklist_group_button_add_window (XfceTasklistChild *group_child,
                                       XfceTasklistChild *window_child)
{
  panel_return_if_fail (group_child->type == CHILD_TYPE_GROUP);
  panel_return_if_fail (window_child->type != CHILD_TYPE_GROUP);
  panel_return_if_fail (XFW_IS_APPLICATION (group_child->app));
  panel_return_if_fail (XFW_IS_WINDOW (window_child->window));
  panel_return_if_fail (window_child->app == group_child->app);
  panel_return_if_fail (XFCE_IS_TASKLIST (group_child->tasklist));
  panel_return_if_fail (g_slist_find (group_child->windows, window_child) == NULL);

  /* watch child visibility changes */
  g_signal_connect_swapped (G_OBJECT (window_child->button), "notify::visible",
                            G_CALLBACK (xfce_tasklist_group_button_child_visible_changed), group_child);
  g_signal_connect_swapped (G_OBJECT (window_child->button), "destroy",
                            G_CALLBACK (xfce_tasklist_group_button_child_destroyed), group_child);
  g_signal_connect_swapped (G_OBJECT (window_child->window), "name-changed",
                            G_CALLBACK (xfce_tasklist_group_button_sort), group_child);
  g_signal_connect_swapped (G_OBJECT (window_child->window), "workspace-changed",
                            G_CALLBACK (xfce_tasklist_group_button_sort), group_child);

  /* add to internal list */
  group_child->windows = g_slist_insert_sorted_with_data (group_child->windows, window_child,
                                                          xfce_tasklist_button_compare,
                                                          group_child->tasklist);

  /* update visibility */
  xfce_tasklist_group_button_child_visible_changed (group_child);
}



static XfceTasklistChild *
xfce_tasklist_group_button_new (XfwApplication *app,
                                XfceTasklist *tasklist)
{
  XfceTasklistChild *child;

  panel_return_val_if_fail (XFCE_IS_TASKLIST (tasklist), NULL);
  panel_return_val_if_fail (XFW_IS_APPLICATION (app), NULL);

  child = xfce_tasklist_child_new (tasklist);
  child->type = CHILD_TYPE_GROUP;
  child->app = app;

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (child->button)), "group-button");
  g_signal_connect_after (G_OBJECT (child->button), "draw",
                          G_CALLBACK (xfce_tasklist_group_button_button_draw), child);
  /* note that the same signals should be in the proxy menu item too */
  g_signal_connect (G_OBJECT (child->button), "button-press-event",
                    G_CALLBACK (xfce_tasklist_group_button_button_press_event), child);
  g_signal_connect (G_OBJECT (child->button), "button-release-event",
                    G_CALLBACK (xfce_tasklist_group_button_button_release_event), child);
  g_signal_connect (G_OBJECT (child->button), "size-allocate",
                    G_CALLBACK (xfce_tasklist_group_button_button_size_allocate), child);

  /* monitor app changes */
  g_signal_connect (G_OBJECT (app), "icon-changed",
                    G_CALLBACK (xfce_tasklist_group_button_icon_changed), child);
  g_signal_connect (G_OBJECT (app), "notify::name",
                    G_CALLBACK (xfce_tasklist_group_button_name_changed), child);

  /* poke functions */
  xfce_tasklist_group_button_icon_changed (app, child);
  xfce_tasklist_group_button_name_changed (NULL, NULL, child);

  /* insert */
  tasklist->windows = g_list_insert_sorted_with_data (tasklist->windows, child,
                                                      xfce_tasklist_button_compare,
                                                      tasklist);

  return child;
}



/**
 * Potential Public Functions
 **/
static void
xfce_tasklist_set_include_all_workspaces (XfceTasklist *tasklist,
                                          gboolean all_workspaces)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  all_workspaces = !!all_workspaces;

  if (tasklist->all_workspaces != all_workspaces)
    {
      tasklist->all_workspaces = all_workspaces;

      if (tasklist->screen != NULL)
        {
          /* update visibility of buttons */
          xfce_tasklist_active_workspace_changed (tasklist->workspace_group, NULL, tasklist);

          /* make sure sorting is ok */
          xfce_tasklist_sort (tasklist, TRUE);
        }
    }
}



static void
xfce_tasklist_set_include_all_monitors (XfceTasklist *tasklist,
                                        gboolean all_monitors)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  all_monitors = !!all_monitors;

  if (tasklist->all_monitors != all_monitors)
    {
      tasklist->all_monitors = all_monitors;

      /* update all windows */
      if (tasklist->screen != NULL)
        xfce_tasklist_active_workspace_changed (tasklist->workspace_group, NULL, tasklist);
    }
}



static void
xfce_tasklist_set_button_relief (XfceTasklist *tasklist,
                                 GtkReliefStyle button_relief)
{
  GList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->button_relief != button_relief)
    {
      tasklist->button_relief = button_relief;

      /* change the relief of all buttons in the list */
      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;
          gtk_button_set_relief (GTK_BUTTON (child->button),
                                 button_relief);
        }

      /* arrow button for overflow menu */
      gtk_button_set_relief (GTK_BUTTON (tasklist->arrow_button),
                             button_relief);
    }
}



static void
xfce_tasklist_set_show_labels (XfceTasklist *tasklist,
                               gboolean show_labels)
{
  GList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  show_labels = !!show_labels;

  if (tasklist->show_labels != show_labels)
    {
      tasklist->show_labels = show_labels;

      /* change the mode of all the buttons */
      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;

          /* show or hide the label */
          if (show_labels)
            {
              gtk_widget_show (child->label);
              gtk_box_set_child_packing (GTK_BOX (child->box),
                                         child->icon,
                                         FALSE, FALSE, 0,
                                         GTK_PACK_START);
            }
          else
            {
              gtk_widget_hide (child->label);
              gtk_box_set_child_packing (GTK_BOX (child->box),
                                         child->icon,
                                         TRUE, TRUE, 0,
                                         GTK_PACK_START);
            }

          /* update the icon (we use another size for
           * icon box mode) */
          if (child->type == CHILD_TYPE_GROUP)
            xfce_tasklist_group_button_icon_changed (child->app, child);
          else
            xfce_tasklist_button_icon_changed (child->window, child);
          gtk_widget_queue_resize (GTK_WIDGET (tasklist));
        }
    }
}



static void
xfce_tasklist_set_show_only_minimized (XfceTasklist *tasklist,
                                       gboolean only_minimized)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  only_minimized = !!only_minimized;

  if (tasklist->only_minimized != only_minimized)
    {
      tasklist->only_minimized = only_minimized;

      /* update all windows */
      if (tasklist->screen != NULL)
        xfce_tasklist_active_workspace_changed (tasklist->workspace_group, NULL, tasklist);
    }
}



static void
xfce_tasklist_set_show_wireframes (XfceTasklist *tasklist,
                                   gboolean show_wireframes)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  tasklist->show_wireframes = !!show_wireframes;

#ifdef ENABLE_X11
  /* destroy the window if needed */
  xfce_tasklist_wireframe_destroy (tasklist);
#endif
}



static void
xfce_tasklist_set_label_decorations (XfceTasklist *tasklist,
                                     gboolean label_decorations)
{
  GList *li;
  XfceTasklistChild *child;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->label_decorations != label_decorations)
    {
      tasklist->label_decorations = label_decorations;

      for (li = tasklist->windows; li != NULL; li = li->next)
        {
          child = li->data;
          xfce_tasklist_button_name_changed (NULL, child);
        }
    }
}



static void
xfce_tasklist_set_grouping (XfceTasklist *tasklist,
                            gboolean grouping)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  grouping = !!grouping;

  if (tasklist->grouping != grouping)
    {
      tasklist->grouping = grouping;

      if (tasklist->screen != NULL)
        {
          xfce_tasklist_disconnect_screen (tasklist);
          xfce_tasklist_connect_screen (tasklist);
        }
    }
}


static void
xfce_tasklist_update_orientation (XfceTasklist *tasklist)
{
  gboolean horizontal;
  GList *li;
  XfceTasklistChild *child;

  horizontal = !xfce_tasklist_vertical (tasklist);

  /* update the tasklist */
  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      child = li->data;

      /* update task box */
      gtk_orientable_set_orientation (GTK_ORIENTABLE (child->box),
                                      horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);

      /* update the label */
      if (horizontal)
        {
          /* gtk_box_reorder_child (GTK_BOX (child->box), child->icon, 0); */
          gtk_label_set_xalign (GTK_LABEL (child->label), 0.0);
          gtk_label_set_yalign (GTK_LABEL (child->label), 0.5);
          gtk_label_set_angle (GTK_LABEL (child->label), 0);
        }
      else
        {
          /* gtk_box_reorder_child (GTK_BOX (child->box), child->icon, -1); */
          gtk_label_set_yalign (GTK_LABEL (child->label), 0.0);
          gtk_label_set_xalign (GTK_LABEL (child->label), 0.5);
          gtk_label_set_angle (GTK_LABEL (child->label), 270);
        }
    }

  gtk_widget_queue_resize (GTK_WIDGET (tasklist));
}



void
xfce_tasklist_set_nrows (XfceTasklist *tasklist,
                         gint nrows)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  panel_return_if_fail (nrows >= 1);

  if (tasklist->nrows != nrows)
    {
      tasklist->nrows = nrows;
      gtk_widget_queue_resize (GTK_WIDGET (tasklist));
    }
}



void
xfce_tasklist_set_mode (XfceTasklist *tasklist,
                        XfcePanelPluginMode mode)
{
  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->mode != mode)
    {
      tasklist->mode = mode;
      xfce_tasklist_update_orientation (tasklist);
    }
}



void
xfce_tasklist_set_size (XfceTasklist *tasklist,
                        gint size)
{
  GList *li;

  panel_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->size != size)
    {
      tasklist->size = size;
      gtk_widget_queue_resize (GTK_WIDGET (tasklist));
    }

  for (li = tasklist->windows; li != NULL; li = li->next)
    {
      XfceTasklistChild *child = li->data;
      if (child->type == CHILD_TYPE_GROUP)
        xfce_tasklist_group_button_icon_changed (child->app, child);
      else
        xfce_tasklist_button_icon_changed (child->window, child);
    }
}



void
xfce_tasklist_update_monitor_geometry (XfceTasklist *tasklist)
{
  if (tasklist->update_monitor_geometry_id == 0)
    {
      tasklist->update_monitor_geometry_id =
        gdk_threads_add_idle_full (G_PRIORITY_LOW, xfce_tasklist_update_monitor_geometry_idle,
                                   tasklist, xfce_tasklist_update_monitor_geometry_idle_destroy);
    }
}
