/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#include "pager-buttons.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <libxfce4ui/libxfce4ui.h>



static void
pager_buttons_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec);
static void
pager_buttons_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec);
static void
pager_buttons_finalize (GObject *object);
static void
pager_buttons_queue_rebuild (PagerButtons *pager);
static void
pager_buttons_screen_workspace_changed (XfwWorkspaceGroup *group,
                                        XfwWorkspace *previous_workspace,
                                        PagerButtons *pager);
static void
pager_buttons_screen_workspace_created (XfwWorkspaceGroup *group,
                                        XfwWorkspace *created_workspace,
                                        PagerButtons *pager);
static void
pager_buttons_screen_workspace_destroyed (XfwWorkspaceGroup *group,
                                          XfwWorkspace *destroyed_workspace,
                                          PagerButtons *pager);
static void
pager_buttons_screen_viewports_changed (XfwWorkspaceGroup *group,
                                        PagerButtons *pager);
static void
pager_buttons_workspace_button_toggled (GtkWidget *button,
                                        XfwWorkspace *workspace);
static void
pager_buttons_workspace_button_label (XfwWorkspace *workspace,
                                      GtkWidget *label);
static void
pager_buttons_viewport_button_toggled (GtkWidget *button,
                                       PagerButtons *pager);



struct _PagerButtons
{
  GtkGrid __parent__;

  GSList *buttons;

  guint rebuild_id;

  XfwScreen *xfw_screen;

  gint rows;
  gboolean numbering;
  GtkOrientation orientation;
};

enum
{
  PROP_0,
  PROP_SCREEN,
  PROP_ROWS,
  PROP_ORIENTATION,
  PROP_NUMBERING
};

enum
{
  VIEWPORT_X,
  VIEWPORT_Y,
  N_INFOS
};



G_DEFINE_FINAL_TYPE (PagerButtons, pager_buttons, GTK_TYPE_GRID)



static void
pager_buttons_class_init (PagerButtonsClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = pager_buttons_get_property;
  gobject_class->set_property = pager_buttons_set_property;
  gobject_class->finalize = pager_buttons_finalize;

  g_object_class_install_property (gobject_class,
                                   PROP_SCREEN,
                                   g_param_spec_object ("screen", NULL, NULL,
                                                        XFW_TYPE_SCREEN,
                                                        G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS
                                                          | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_ROWS,
                                   g_param_spec_int ("rows", NULL, NULL,
                                                     1, 100, 1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_NUMBERING,
                                   g_param_spec_boolean ("numbering", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
pager_buttons_init (PagerButtons *pager)
{
  pager->rows = 1;
  pager->xfw_screen = NULL;
  pager->orientation = GTK_ORIENTATION_HORIZONTAL;
  pager->numbering = FALSE;
  pager->buttons = NULL;
  pager->rebuild_id = 0;

  /* although I'd prefer normal allocation, the homogeneous setting
   * takes care of small panels, while non-homogeneous tables allocate
   * outside the panel size --nick */
  gtk_grid_set_row_homogeneous (GTK_GRID (pager), TRUE);
  gtk_grid_set_column_homogeneous (GTK_GRID (pager), TRUE);
}



static void
pager_buttons_get_property (GObject *object,
                            guint prop_id,
                            GValue *value,
                            GParamSpec *pspec)
{
  PagerButtons *pager = PAGER_BUTTONS (object);

  switch (prop_id)
    {
    case PROP_ROWS:
      g_value_set_int (value, pager->rows);
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, pager->orientation);
      break;

    case PROP_NUMBERING:
      g_value_set_boolean (value, pager->numbering);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
workspace_group_created (XfwWorkspaceManager *manager,
                         XfwWorkspaceGroup *group,
                         PagerButtons *pager)
{
  g_signal_connect (group, "active-workspace-changed",
                    G_CALLBACK (pager_buttons_screen_workspace_changed), pager);
  g_signal_connect (group, "workspace-added",
                    G_CALLBACK (pager_buttons_screen_workspace_created), pager);
  g_signal_connect (group, "workspace-removed",
                    G_CALLBACK (pager_buttons_screen_workspace_destroyed), pager);
  g_signal_connect (group, "viewports-changed",
                    G_CALLBACK (pager_buttons_screen_viewports_changed), pager);
}



static void
workspace_group_destroyed (XfwWorkspaceManager *manager,
                           XfwWorkspaceGroup *group,
                           PagerButtons *pager)
{
  g_signal_handlers_disconnect_by_func (group, pager_buttons_screen_workspace_changed, pager);
  g_signal_handlers_disconnect_by_func (group, pager_buttons_screen_workspace_created, pager);
  g_signal_handlers_disconnect_by_func (group, pager_buttons_screen_workspace_destroyed, pager);
  g_signal_handlers_disconnect_by_func (group, pager_buttons_screen_viewports_changed, pager);
}



static void
pager_buttons_set_property (GObject *object,
                            guint prop_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  PagerButtons *pager = PAGER_BUTTONS (object);
  XfwWorkspaceManager *manager;

  switch (prop_id)
    {
    case PROP_SCREEN:
      pager->xfw_screen = g_value_dup_object (value);
      panel_return_if_fail (XFW_IS_SCREEN (pager->xfw_screen));
      manager = xfw_screen_get_workspace_manager (pager->xfw_screen);
      g_signal_connect (manager, "workspace-group-created", G_CALLBACK (workspace_group_created), pager);
      g_signal_connect (manager, "workspace-group-destroyed", G_CALLBACK (workspace_group_destroyed), pager);
      for (GList *lp = xfw_workspace_manager_list_workspace_groups (manager); lp != NULL; lp = lp->next)
        workspace_group_created (manager, lp->data, pager);

      pager_buttons_queue_rebuild (pager);
      break;

    case PROP_ROWS:
      pager_buttons_set_n_rows (pager, g_value_get_int (value));
      break;

    case PROP_ORIENTATION:
      pager_buttons_set_orientation (pager, g_value_get_enum (value));
      break;

    case PROP_NUMBERING:
      pager_buttons_set_numbering (pager, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
pager_buttons_finalize (GObject *object)
{
  PagerButtons *pager = PAGER_BUTTONS (object);

  if (pager->rebuild_id != 0)
    g_source_remove (pager->rebuild_id);

  if (G_LIKELY (pager->xfw_screen != NULL))
    {
      XfwWorkspaceManager *manager = xfw_screen_get_workspace_manager (pager->xfw_screen);
      for (GList *lp = xfw_workspace_manager_list_workspace_groups (manager); lp != NULL; lp = lp->next)
        workspace_group_destroyed (manager, lp->data, pager);

      g_object_unref (G_OBJECT (pager->xfw_screen));
    }

  g_slist_free (pager->buttons);

  (*G_OBJECT_CLASS (pager_buttons_parent_class)->finalize) (object);
}



static gboolean
pager_buttons_button_press_event (GtkWidget *button,
                                  GdkEventButton *event)
{
  guint modifiers;

  panel_return_val_if_fail (GTK_IS_TOGGLE_BUTTON (button), FALSE);

  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  /* block toggle events on an active button */
  if (event->button == 1
      && modifiers != GDK_CONTROL_MASK
      && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return TRUE;

  return FALSE;
}



static gboolean
pager_buttons_rebuild_idle (gpointer user_data)
{
  PagerButtons *pager = PAGER_BUTTONS (user_data);
  XfwWorkspaceManager *manager;
  GList *li, *workspaces;
  XfwWorkspace *active_ws;
  gint n, n_workspaces;
  gint rows, cols;
  gint row, col;
  GtkWidget *button;
  XfwWorkspace *workspace = NULL;
  GtkWidget *panel_plugin;
  GtkWidget *label;
  gint screen_width = 0, screen_height = 0;
  gboolean viewport_mode = FALSE;
  gint n_viewports = 0;
  gint *vp_info;
  gchar text[8];
  GdkRectangle *rect = NULL;
  GdkScreen *screen;
  GdkMonitor *monitor;
  guint scale_factor;

  panel_return_val_if_fail (PAGER_IS_BUTTONS (pager), FALSE);
  panel_return_val_if_fail (XFW_IS_SCREEN (pager->xfw_screen), FALSE);

  gtk_container_foreach (GTK_CONTAINER (pager),
                         (GtkCallback) (void (*) (void)) gtk_widget_destroy, NULL);

  g_slist_free (pager->buttons);
  pager->buttons = NULL;

  monitor = panel_utils_get_monitor_at_widget (GTK_WIDGET (pager));
  active_ws = panel_utils_get_active_workspace_for_monitor (pager->xfw_screen, monitor);
  workspaces = panel_utils_list_workspaces_for_monitor (pager->xfw_screen, monitor);
  if (workspaces == NULL)
    goto leave;

  n_workspaces = g_list_length (workspaces);

  /* check if the user uses 1 workspace with viewports */
  if (G_UNLIKELY (n_workspaces == 1
                  && xfw_workspace_get_state (workspaces->data) & XFW_WORKSPACE_STATE_VIRTUAL))
    {
      workspace = XFW_WORKSPACE (workspaces->data);
      rect = xfw_workspace_get_geometry (workspace);
      scale_factor = gdk_window_get_scale_factor (gtk_widget_get_window (GTK_WIDGET (pager)));
      screen = gdk_screen_get_default ();
      screen_width = panel_screen_get_width (screen) * scale_factor;
      screen_height = panel_screen_get_height (screen) * scale_factor;

      /* we only support viewports that are equally spread */
      if ((rect->width % screen_width) == 0
          && (rect->height % screen_height) == 0)
        {
          n_viewports = (rect->width / screen_width) * (rect->height / screen_height);

          rows = CLAMP (1, pager->rows, n_viewports);
          cols = n_workspaces / rows;
          if (cols * rows < n_workspaces)
            cols++;

          viewport_mode = TRUE;
        }
      else
        {
          g_warning ("only viewports with equally distributed screens are supported: %dx%d & %dx%d",
                     rect->width, rect->height, screen_width, screen_height);

          goto workspace_layout;
        }
    }
  else
    {
workspace_layout:

      rows = CLAMP (1, pager->rows, n_workspaces);
      cols = n_workspaces / rows;
      if (cols * rows < n_workspaces)
        cols++;
    }

  /* set workspace layout so changing workspace and moving windows between workspaces
   * via keyboard shortcuts or dnd work correctly in all directions;
   * workspace layout is only supported on X11, where there is only one workspace group */
  manager = xfw_screen_get_workspace_manager (pager->xfw_screen);
  xfw_workspace_group_set_layout (xfw_workspace_manager_list_workspace_groups (manager)->data, rows, 0, NULL);

  panel_plugin = gtk_widget_get_ancestor (GTK_WIDGET (pager), XFCE_TYPE_PANEL_PLUGIN);

  if (G_UNLIKELY (viewport_mode))
    {
      panel_return_val_if_fail (XFW_IS_WORKSPACE (workspace), FALSE);

      for (n = 0; n < n_viewports; n++)
        {
          vp_info = g_new0 (gint, N_INFOS);
          vp_info[VIEWPORT_X] = (n % (rect->height / screen_height)) * screen_width;
          vp_info[VIEWPORT_Y] = (n / (rect->height / screen_height)) * screen_height;

          button = xfce_panel_create_toggle_button ();
          gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
          if (rect->x >= vp_info[VIEWPORT_X] && rect->x < vp_info[VIEWPORT_X] + screen_width
              && rect->y >= vp_info[VIEWPORT_Y] && rect->y < vp_info[VIEWPORT_Y] + screen_height)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
          g_signal_connect (G_OBJECT (button), "toggled",
                            G_CALLBACK (pager_buttons_viewport_button_toggled), pager);
          g_signal_connect (G_OBJECT (button), "button-press-event",
                            G_CALLBACK (pager_buttons_button_press_event), NULL);
          xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (panel_plugin), button);
          gtk_widget_show (button);

          g_object_set_data_full (G_OBJECT (button), "viewport-info", vp_info,
                                  (GDestroyNotify) g_free);

          g_snprintf (text, sizeof (text), "%d", n + 1);
          label = gtk_label_new (text);
          gtk_label_set_angle (GTK_LABEL (label),
                               pager->orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 270);
          gtk_container_add (GTK_CONTAINER (button), label);
          gtk_widget_show (label);

          if (pager->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              row = n % cols;
              col = n / cols;
            }
          else
            {
              row = n / cols;
              col = n % cols;
            }

          gtk_grid_attach (GTK_GRID (pager), button,
                           row, col, 1, 1);
        }
    }
  else
    {
      for (li = workspaces, n = 0; li != NULL; li = li->next, n++)
        {
          workspace = XFW_WORKSPACE (li->data);

          button = xfce_panel_create_toggle_button ();
          gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);
          if (workspace == active_ws)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
          g_signal_connect (G_OBJECT (button), "toggled",
                            G_CALLBACK (pager_buttons_workspace_button_toggled), workspace);
          g_signal_connect (G_OBJECT (button), "button-press-event",
                            G_CALLBACK (pager_buttons_button_press_event), NULL);
          xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (panel_plugin), button);
          gtk_widget_show (button);

          label = gtk_label_new (NULL);
          g_object_set_data (G_OBJECT (label), "pager", pager);
          g_signal_connect_object (G_OBJECT (workspace), "name-changed",
                                   G_CALLBACK (pager_buttons_workspace_button_label), label, 0);
          pager_buttons_workspace_button_label (workspace, label);
          gtk_label_set_angle (GTK_LABEL (label),
                               pager->orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 270);
          gtk_container_add (GTK_CONTAINER (button), label);
          gtk_widget_show (label);

          pager->buttons = g_slist_prepend (pager->buttons, button);

          if (pager->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              row = n % cols;
              col = n / cols;
            }
          else
            {
              row = n / cols;
              col = n % cols;
            }

          gtk_grid_attach (GTK_GRID (pager), button,
                           row, col, 1, 1);
        }
    }

  pager->buttons = g_slist_reverse (pager->buttons);

leave:
  g_list_free (workspaces);

  return FALSE;
}



static void
pager_buttons_rebuild_idle_destroyed (gpointer user_data)
{
  PAGER_BUTTONS (user_data)->rebuild_id = 0;
}



static void
pager_buttons_queue_rebuild (PagerButtons *pager)
{
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  if (pager->rebuild_id == 0)
    {
      pager->rebuild_id = gdk_threads_add_idle_full (G_PRIORITY_LOW, pager_buttons_rebuild_idle,
                                                     pager, pager_buttons_rebuild_idle_destroyed);
    }
}



static void
pager_buttons_screen_workspace_changed (XfwWorkspaceGroup *group,
                                        XfwWorkspace *previous_workspace,
                                        PagerButtons *pager)
{
  gint active = -1, n;
  XfwWorkspace *active_ws;
  GSList *li;
  GdkMonitor *monitor;
  GList *monitors;

  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (previous_workspace == NULL || XFW_IS_WORKSPACE (previous_workspace));
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  monitor = panel_utils_get_monitor_at_widget (GTK_WIDGET (pager));
  monitors = xfw_workspace_group_get_monitors (group);
  if (!g_list_find_custom (monitors, monitor, panel_utils_compare_xfw_gdk_monitors))
    return;

  active_ws = panel_utils_get_active_workspace_for_monitor (pager->xfw_screen, monitor);
  if (G_LIKELY (active_ws != NULL))
    active = panel_utils_get_workspace_number_for_monitor (pager->xfw_screen, monitor, active_ws);

  for (li = pager->buttons, n = 0; li != NULL; li = li->next, n++)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (li->data), n == active);
}



static void
pager_buttons_screen_workspace_created (XfwWorkspaceGroup *group,
                                        XfwWorkspace *created_workspace,
                                        PagerButtons *pager)
{
  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (XFW_IS_WORKSPACE (created_workspace));
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  pager_buttons_queue_rebuild (pager);
}



static void
pager_buttons_screen_workspace_destroyed (XfwWorkspaceGroup *group,
                                          XfwWorkspace *destroyed_workspace,
                                          PagerButtons *pager)
{
  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (XFW_IS_WORKSPACE (destroyed_workspace));
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  pager_buttons_queue_rebuild (pager);
}



static void
pager_buttons_screen_viewports_changed (XfwWorkspaceGroup *group,
                                        PagerButtons *pager)
{
  panel_return_if_fail (XFW_IS_WORKSPACE_GROUP (group));
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  /* yes we are extremely lazy here, but this event is
   * also emitted when the viewport setup changes... */
  if (pager->buttons == NULL)
    pager_buttons_queue_rebuild (pager);
}



static void
pager_buttons_workspace_button_label (XfwWorkspace *workspace,
                                      GtkWidget *label)
{
  const gchar *name;
  gchar *utf8 = NULL, *name_fallback = NULL, *name_num = NULL;
  PagerButtons *pager;
  GdkMonitor *monitor;
  gint number;

  panel_return_if_fail (XFW_IS_WORKSPACE (workspace));
  panel_return_if_fail (GTK_IS_LABEL (label));

  /* try to get a utf-8 valid name */
  name = xfw_workspace_get_name (workspace);
  if (!xfce_str_is_empty (name)
      && !g_utf8_validate (name, -1, NULL))
    name = utf8 = g_locale_to_utf8 (name, -1, NULL, NULL, NULL);

  pager = g_object_get_data (G_OBJECT (label), "pager");
  monitor = panel_utils_get_monitor_at_widget (GTK_WIDGET (pager));
  number = panel_utils_get_workspace_number_for_monitor (pager->xfw_screen, monitor, workspace);

  if (xfce_str_is_empty (name))
    name = name_fallback = g_strdup_printf (_("Workspace %d"), number + 1);

  if (pager->numbering)
    name = name_num = g_strdup_printf ("%d - %s", number + 1, name);

  gtk_label_set_text (GTK_LABEL (label), name);

  g_free (utf8);
  g_free (name_fallback);
  g_free (name_num);
}



static void
pager_buttons_workspace_button_toggled (GtkWidget *button,
                                        XfwWorkspace *workspace)
{
  PagerButtons *pager;
  XfwWorkspace *active_ws;
  GdkMonitor *monitor;

  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (XFW_IS_WORKSPACE (workspace));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      pager = PAGER_BUTTONS (gtk_widget_get_ancestor (button, PAGER_TYPE_BUTTONS));
      monitor = panel_utils_get_monitor_at_widget (button);
      active_ws = panel_utils_get_active_workspace_for_monitor (pager->xfw_screen, monitor);
      if (active_ws != workspace)
        xfw_workspace_activate (workspace, NULL);
    }
}



static void
pager_buttons_viewport_button_toggled (GtkWidget *button,
                                       PagerButtons *pager)
{
  XfwWorkspaceManager *manager;
  gint *vp_info;

  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  vp_info = g_object_get_data (G_OBJECT (button), "viewport-info");
  if (G_UNLIKELY (vp_info == NULL))
    return;

  /* viewports are only supported on X11, where there is only one workspace group */
  manager = xfw_screen_get_workspace_manager (pager->xfw_screen);
  xfw_workspace_group_move_viewport (xfw_workspace_manager_list_workspace_groups (manager)->data,
                                     vp_info[VIEWPORT_X], vp_info[VIEWPORT_Y], NULL);
}



GtkWidget *
pager_buttons_new (XfwScreen *screen)
{
  panel_return_val_if_fail (XFW_IS_SCREEN (screen), NULL);

  return g_object_new (PAGER_TYPE_BUTTONS,
                       "screen", screen, NULL);
}



void
pager_buttons_set_orientation (PagerButtons *pager,
                               GtkOrientation orientation)
{
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  if (pager->orientation == orientation)
    return;

  pager->orientation = orientation;
  pager_buttons_queue_rebuild (pager);
}



void
pager_buttons_set_n_rows (PagerButtons *pager,
                          gint rows)
{
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  if (pager->rows == rows)
    return;

  pager->rows = rows;
  pager_buttons_queue_rebuild (pager);
}



void
pager_buttons_set_numbering (PagerButtons *pager,
                             gboolean numbering)
{
  panel_return_if_fail (PAGER_IS_BUTTONS (pager));

  if (pager->numbering == numbering)
    return;

  pager->numbering = numbering;
  pager_buttons_queue_rebuild (pager);
}
