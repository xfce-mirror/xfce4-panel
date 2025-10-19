/*
 * Copyright (C) 2025 Dmitry Petrachkov <dmitry-petrachkov@outlook.com>
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

#include "panel-item-list-view.h"
#include "panel-item-dialog.h"

struct _PanelItemListView
{
  GtkBox __parent__;

  GtkWidget *list_view;
  GSimpleAction *about_action;
};



static void
panel_item_list_view_finalize (GObject *object);
static gboolean
panel_item_list_view_add_item (PanelItemListView *view);
static gboolean
panel_item_list_view_remove_items (PanelItemListView *view,
                                   gint *items,
                                   gint n_items);
static gboolean
panel_item_list_view_edit_item (PanelItemListView *view,
                                gint index);
static void
panel_item_list_view_selection_changed (PanelItemListView *view);
static void
panel_item_list_view_about_item (PanelItemListView *view);



G_DEFINE_TYPE (PanelItemListView, panel_item_list_view, GTK_TYPE_BOX)



static void
panel_item_list_view_class_init (PanelItemListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = panel_item_list_view_finalize;
}



static void
panel_item_list_view_init (PanelItemListView *view)
{
  /* Configure XfceItemListView */
  view->list_view = xfce_item_list_view_new (NULL);
  g_signal_connect_swapped (view->list_view, "add-item", G_CALLBACK (panel_item_list_view_add_item), view);
  g_signal_connect_swapped (view->list_view, "remove-items", G_CALLBACK (panel_item_list_view_remove_items), view);
  g_signal_connect_swapped (view->list_view, "edit-item", G_CALLBACK (panel_item_list_view_edit_item), view);
  gtk_box_pack_start (GTK_BOX (view), view->list_view, TRUE, TRUE, 0);
  gtk_widget_show (view->list_view);

  /* Configure TreeView */
  GtkWidget *tree_view = xfce_item_list_view_get_tree_view (XFCE_ITEM_LIST_VIEW (view->list_view));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  g_signal_connect_swapped (selection, "changed", G_CALLBACK (panel_item_list_view_selection_changed), view);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  /* Create "About" action */
  GSimpleActionGroup *action_group = g_simple_action_group_new ();
  view->about_action = g_simple_action_new ("about-item", NULL);
  g_signal_connect_swapped (view->about_action, "activate", G_CALLBACK (panel_item_list_view_about_item), view);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (view->about_action));
  gtk_widget_insert_action_group (GTK_WIDGET (view), "panel-item-list-view", G_ACTION_GROUP (action_group));
  g_object_unref (action_group);

  GMenu *menu = xfce_item_list_view_get_menu (XFCE_ITEM_LIST_VIEW (view->list_view));
  GMenuItem *menu_item = g_menu_item_new (_("About the item"), "panel-item-list-view.about-item");
  GIcon *icon = g_themed_icon_new ("help-about-symbolic");
  GVariant *serialized_icon = g_icon_serialize (icon);
  g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_ICON, serialized_icon);
  g_variant_unref (serialized_icon);
  g_object_unref (icon);
  g_menu_item_set_attribute_value (menu_item, XFCE_MENU_ATTRIBUTE_TOOLTIP, g_variant_new_string (_("Show about information of the currently selected item")));
  g_menu_item_set_attribute_value (menu_item, XFCE_MENU_ATTRIBUTE_HIDE_LABEL, g_variant_new_boolean (TRUE));
  g_menu_append_item (menu, menu_item);
  g_object_unref (menu_item);

  /* Reload state */
  panel_item_list_view_selection_changed (view);
}



static void
panel_item_list_view_finalize (GObject *object)
{
  PanelItemListView *view = PANEL_ITEM_LIST_VIEW (object);

  g_object_unref (view->about_action);
  G_OBJECT_CLASS (panel_item_list_view_parent_class)->finalize (object);
}



static gboolean
panel_item_list_view_add_item (PanelItemListView *view)
{
  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
  PanelWindow *panel = panel_item_list_model_get_panel (PANEL_ITEM_LIST_MODEL (model));

  panel_item_dialog_show (panel);
  return TRUE;
}



static gboolean
panel_item_list_view_remove_items (PanelItemListView *view,
                                   gint *items,
                                   gint n_items)
{
  const gchar *secondary = NULL;
  gchar *primary = NULL;

  if (n_items == 1)
    {
      XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
      XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (PANEL_ITEM_LIST_MODEL (model), items[0]);
      PanelModule *module = panel_module_get_from_plugin_provider (provider);
      primary = g_strdup_printf (_("Are you sure that you want to remove \"%s\"?"), panel_module_get_display_name (module));
      secondary = _("If you remove the item from the panel, it is permanently lost.");
    }
  else
    {
      primary = g_strdup_printf (_("Are you sure that you want to remove these %d items?"), n_items);
      secondary = _("If you remove them from the panel, they are permanently lost.");
    }

  GtkWindow *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view)));
  gboolean confirmed = xfce_dialog_confirm (toplevel, "list-remove", _("Remove"), secondary, "%s", primary);
  g_free (primary);

  return !confirmed;
}



static gboolean
panel_item_list_view_edit_item (PanelItemListView *view,
                                gint index)
{
  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (PANEL_ITEM_LIST_MODEL (model), index);

  xfce_panel_plugin_provider_show_configure (provider);
  return FALSE;
}



static void
panel_item_list_view_selection_changed (PanelItemListView *view)
{
  gint *sel_items = NULL;
  gint n_sel_items = xfce_item_list_view_get_selected_items (XFCE_ITEM_LIST_VIEW (view->list_view), &sel_items);
  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));

  g_simple_action_set_enabled (view->about_action, n_sel_items == 1 && xfce_item_list_model_test (model, sel_items[0], PANEL_ITEM_LIST_MODEL_COLUMN_ABOUT));
  g_free (sel_items);
}



static void
panel_item_list_view_about_item (PanelItemListView *view)
{
  gint *sel_items = NULL;
  xfce_item_list_view_get_selected_items (XFCE_ITEM_LIST_VIEW (view->list_view), &sel_items);
  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (PANEL_ITEM_LIST_MODEL (model), sel_items[0]);

  xfce_panel_plugin_provider_show_about (provider);
  g_free (sel_items);
}



void
panel_item_list_view_set_model (PanelItemListView *view,
                                PanelItemListModel *model)
{
  g_return_if_fail (PANEL_IS_ITEM_LIST_VIEW (view));
  g_return_if_fail (model == NULL || PANEL_IS_ITEM_LIST_MODEL (model));

  xfce_item_list_view_set_model (XFCE_ITEM_LIST_VIEW (view->list_view), XFCE_ITEM_LIST_MODEL (model));
}
