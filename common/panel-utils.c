/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "panel-private.h"
#include "panel-utils.h"

#include <libxfce4ui/libxfce4ui.h>



typedef struct _PanelUtilsGtkLabelData
{
  const gchar *label_text;
  GtkLabel *label;
} PanelUtilsGtkLabelData;



static void
panel_utils_help_button_clicked (GtkWidget *button,
                                 XfcePanelPlugin *panel_plugin)
{
  GtkWidget *toplevel;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));
  panel_return_if_fail (GTK_IS_WIDGET (button));

  toplevel = gtk_widget_get_toplevel (button);
  panel_utils_show_help (GTK_WINDOW (toplevel),
                         xfce_panel_plugin_get_name (panel_plugin),
                         NULL);
}



static void
panel_utils_block_autohide (XfcePanelPlugin *panel_plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));

  xfce_panel_plugin_block_autohide (panel_plugin, TRUE);
}



static void
panel_utils_unblock_autohide (XfcePanelPlugin *panel_plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));

  xfce_panel_plugin_block_autohide (panel_plugin, FALSE);
}



GtkBuilder *
panel_utils_builder_new (XfcePanelPlugin *panel_plugin,
                         const gchar *resource,
                         GObject **dialog_return)
{
  GError *error = NULL;
  GtkBuilder *builder;
  GObject *dialog, *button;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin), NULL);
  panel_return_val_if_fail (dialog_return != NULL, NULL);

  if (*dialog_return != NULL)
    {
      gtk_window_present (GTK_WINDOW (*dialog_return));
      return NULL;
    }

  builder = gtk_builder_new ();
  gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
  if (gtk_builder_add_from_resource (builder, resource, &error))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      if (G_LIKELY (dialog != NULL))
        {
          g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) G_CALLBACK (g_object_unref), builder);
          xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

          g_signal_connect_swapped (dialog, "show",
                                    G_CALLBACK (panel_utils_block_autohide), panel_plugin);
          g_signal_connect_swapped (dialog, "hide",
                                    G_CALLBACK (panel_utils_unblock_autohide), panel_plugin);

          button = gtk_builder_get_object (builder, "close-button");
          if (G_LIKELY (button != NULL))
            g_signal_connect_swapped (G_OBJECT (button), "clicked",
                                      G_CALLBACK (gtk_widget_destroy), dialog);

          button = gtk_builder_get_object (builder, "help-button");
          if (G_LIKELY (button != NULL))
            g_signal_connect (G_OBJECT (button), "clicked",
                              G_CALLBACK (panel_utils_help_button_clicked), panel_plugin);

          *dialog_return = dialog;
          g_object_add_weak_pointer (dialog, (gpointer *) dialog_return);

          return builder;
        }
      else
        {
          g_set_error_literal (&error, 0, 0, "No widget with the name \"dialog\" found");
        }
    }

  g_critical ("Failed to construct the builder for plugin %s-%d: %s.",
              xfce_panel_plugin_get_name (panel_plugin),
              xfce_panel_plugin_get_unique_id (panel_plugin),
              error->message);
  g_error_free (error);
  g_object_unref (G_OBJECT (builder));

  return NULL;
}



void
panel_utils_show_help (GtkWindow *parent,
                       const gchar *page,
                       const gchar *offset)
{
  xfce_dialog_show_help (parent, "xfce4-panel", page, offset);
}



gboolean
panel_utils_device_grab (GtkWidget *widget)
{
  GdkScreen *screen = gtk_widget_get_screen (widget);
  GdkDisplay *display = gdk_screen_get_display (screen);
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkWindow *window = gdk_window_get_effective_toplevel (gtk_widget_get_window (widget));

  return xfce_gdk_device_grab (seat, window, GDK_SEAT_CAPABILITY_ALL, NULL);
}



void
panel_utils_set_atk_info (GtkWidget *widget,
                          const gchar *name,
                          const gchar *description)
{
  static gboolean initialized = FALSE;
  static gboolean atk_enabled = TRUE;
  AtkObject *object;

  panel_return_if_fail (GTK_IS_WIDGET (widget));

  if (atk_enabled)
    {
      object = gtk_widget_get_accessible (widget);

      if (!initialized)
        {
          initialized = TRUE;
          atk_enabled = GTK_IS_ACCESSIBLE (object);

          if (!atk_enabled)
            return;
        }

      if (name != NULL)
        atk_object_set_name (object, name);

      if (description != NULL)
        atk_object_set_description (object, description);
    }
}



static gboolean
destroy_later (gpointer widget)
{
  gtk_widget_destroy (GTK_WIDGET (widget));
  g_object_unref (G_OBJECT (widget));
  return FALSE;
}



void
panel_utils_destroy_later (GtkWidget *widget)
{
  panel_return_if_fail (GTK_IS_WIDGET (widget));

  g_idle_add_full (G_PRIORITY_HIGH, destroy_later, widget, NULL);
  g_object_ref_sink (G_OBJECT (widget));
}



/*
 * We need to do this when GTK refuses to do it itself, for example to bring back a window
 * that has been moved off-screen, see https://github.com/wmww/gtk-layer-shell/issues/143.
 * This manual intervention should only be done if necessary though, as it can have side
 * effects. It is not performed systematically in Gtk Layer Shell for this reason, and, for
 * example, it causes the pointer to re-enter the autohide window when moved off screen,
 * which, coupled with widget_remap() below, can cause the panel to flicker.
 */
void
panel_utils_wl_surface_commit (GtkWidget *widget)
{
#ifdef ENABLE_WAYLAND
  GdkWindow *window = gtk_widget_get_window (widget);
  if (window != NULL)
    {
      /* yes, it can be null when the window is not */
      struct wl_surface *wl_surface = gdk_wayland_window_get_wl_surface (window);
      if (wl_surface != NULL)
        wl_surface_commit (wl_surface);
    }
#endif
}



/*
 * Like wl_surface_commit() above: use sparingly. It is about forcing GTK and/or the
 * compositor to take into account a request (resizing, layer change), but this can
 * have side effects.
 */
void
panel_utils_widget_remap (GtkWidget *widget)
{
  if (gtk_widget_get_visible (widget))
    {
      gtk_widget_hide (widget);
      gtk_widget_show (widget);
    }
}



static void
panel_utils_gtk_dialog_find_label_by_text_cb (GtkWidget *widget,
                                              gpointer data)
{
  PanelUtilsGtkLabelData *label_data = data;

  panel_return_if_fail (widget);
  panel_return_if_fail (label_data && label_data->label_text);

  if (GTK_IS_LABEL (widget) && g_strcmp0 (label_data->label_text, gtk_label_get_text (GTK_LABEL (widget))) == 0)
    {
      if (label_data->label)
        g_warning ("%s: Found multiple labels with text value '%s'", G_STRFUNC, label_data->label_text);
      else
        label_data->label = GTK_LABEL (widget);
    }
  else if (GTK_IS_BOX (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), panel_utils_gtk_dialog_find_label_by_text_cb, data);
}



/*
 * Recursively searches the given GtkDialog and obtains a GtkLabel that has the given text.
 */
GtkLabel *
panel_utils_gtk_dialog_find_label_by_text (GtkDialog *dialog,
                                           const gchar *label_text)
{
  PanelUtilsGtkLabelData *label_data;
  GtkLabel *label;

  panel_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);

  label_data = g_new0 (PanelUtilsGtkLabelData, 1);
  label_data->label_text = label_text;
  label_data->label = NULL;

  gtk_container_foreach (GTK_CONTAINER (dialog), panel_utils_gtk_dialog_find_label_by_text_cb, label_data);
  if (label_data->label == NULL)
    g_warning ("%s: Could not find a label with the given text '%s'", G_STRFUNC, label_text);

  label = label_data->label;
  g_free (label_data);
  return label;
}



gint
panel_utils_compare_xfw_gdk_monitors (gconstpointer a,
                                      gconstpointer b)
{
  return xfw_monitor_get_gdk_monitor ((XfwMonitor *) a) == b ? 0 : 1;
}



GdkMonitor *
panel_utils_get_monitor_at_widget (GtkWidget *widget)
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkWindow *window = gtk_widget_get_window (widget);
  if (window != NULL)
    return gdk_display_get_monitor_at_window (display, window);

  return gdk_display_get_monitor (display, 0);
}



GList *
panel_utils_list_workspace_groups_for_monitor (XfwScreen *xfw_screen,
                                               GdkMonitor *monitor)
{
  XfwWorkspaceManager *manager = xfw_screen_get_workspace_manager (xfw_screen);
  GList *groups = NULL;
  for (GList *lp = xfw_workspace_manager_list_workspace_groups (manager); lp != NULL; lp = lp->next)
    if (g_list_find_custom (xfw_workspace_group_get_monitors (lp->data), monitor, panel_utils_compare_xfw_gdk_monitors))
      groups = g_list_prepend (groups, lp->data);

  return g_list_reverse (groups);
}



GList *
panel_utils_list_workspaces_for_monitor (XfwScreen *xfw_screen,
                                         GdkMonitor *monitor)
{
  GList *groups = panel_utils_list_workspace_groups_for_monitor (xfw_screen, monitor);
  GList *workspaces = NULL;
  for (GList *lp = groups; lp != NULL; lp = lp->next)
    for (GList *lq = xfw_workspace_group_list_workspaces (lp->data); lq != NULL; lq = lq->next)
      workspaces = g_list_prepend (workspaces, lq->data);

  g_list_free (groups);
  return g_list_reverse (workspaces);
}



XfwWorkspace *
panel_utils_get_active_workspace_for_monitor (XfwScreen *xfw_screen,
                                              GdkMonitor *monitor)
{
  GList *groups = panel_utils_list_workspace_groups_for_monitor (xfw_screen, monitor);
  XfwWorkspace *workspace = NULL;
  for (GList *lp = groups; lp != NULL; lp = lp->next)
    {
      workspace = xfw_workspace_group_get_active_workspace (lp->data);
      if (workspace != NULL)
        break;
    }

  g_list_free (groups);
  return workspace;
}



guint
panel_utils_get_workspace_count_for_monitor (XfwScreen *xfw_screen,
                                             GdkMonitor *monitor)
{
  GList *groups = panel_utils_list_workspace_groups_for_monitor (xfw_screen, monitor);
  guint count = 0;
  for (GList *lp = groups; lp != NULL; lp = lp->next)
    count += xfw_workspace_group_get_workspace_count (lp->data);

  g_list_free (groups);
  return count;
}



gint
panel_utils_get_workspace_number_for_monitor (XfwScreen *xfw_screen,
                                              GdkMonitor *monitor,
                                              XfwWorkspace *workspace)
{
  GList *workspaces = panel_utils_list_workspaces_for_monitor (xfw_screen, monitor);
  gint number = g_list_index (workspaces, workspace);

  g_list_free (workspaces);
  return number;
}



void
panel_utils_populate_output_list (GtkListStore *store,
                                  GtkComboBox *box,
                                  const gchar *output_name,
                                  GdkDisplay *display,
                                  gint n_monitors,
                                  gboolean *output_selected,
                                  GtkTreeIter *iter,
                                  gint *n)
{
  GHashTable *models = g_hash_table_new (g_str_hash, g_str_equal);

  for (gint i = 0; i < n_monitors; i++)
    {
      GdkMonitor *monitor = gdk_display_get_monitor (display, i);
      const gchar *model = gdk_monitor_get_model (monitor);
      gchar *title, *name;

      if (xfce_str_is_empty (model) || !g_hash_table_add (models, (gpointer) model))
        {
          /* I18N: monitor name in the output selector */
          title = g_strdup_printf (_("Monitor %d"), i + 1);
          if (xfce_str_is_empty (model))
            name = g_strdup_printf ("monitor-%d", i);
          else
            name = g_strdup_printf ("monitor-%d-%s", i, model);
        }
      else
        {
          /* use the randr name for the title */
          name = g_strdup (model);
          title = g_strdup (name);
        }

      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), iter, (*n)++,
                                         OUTPUT_NAME, name,
                                         OUTPUT_TITLE, title, -1);
      if (!(*output_selected)
          && g_strcmp0 (name, output_name) == 0)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (box), iter);
          *output_selected = TRUE;
        }

      g_free (name);
      g_free (title);
    }

  g_hash_table_destroy (models);
}
