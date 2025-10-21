#include "launcher-item-list-view.h"
#include "launcher-item-info.h"



struct _LauncherItemListView
{
  GtkBox __parent__;

  GtkWidget *list_view;
};

enum
{
  ADD_ITEM,
  EDIT_ITEM,
  N_SIGNALS,
};

enum
{
  DROP_TARGET_URI,
  DROP_TARGET_ROW
};

static int signals[N_SIGNALS];



static gboolean
launcher_item_list_view_add_item (LauncherItemListView *view);
static gboolean
launcher_item_list_view_remove_items (LauncherItemListView *view,
                                      gint *items,
                                      gint n_items);
static gboolean
launcher_item_list_view_edit_item (LauncherItemListView *view,
                                   gint index);
static void
launcher_item_list_view_new_item (LauncherItemListView *view);
static void
launcher_item_list_view_new_link (LauncherItemListView *view);
static void
launcher_item_list_view_drag_data_received (LauncherItemListView *view,
                                            GdkDragContext *context,
                                            gint x,
                                            gint y,
                                            GtkSelectionData *data,
                                            guint info,
                                            guint timestamp,
                                            GtkWidget *tree_view);
static void
launcher_item_list_view_append_uris (LauncherItemListView *view,
                                     gint position,
                                     gchar **uris);
static void
launcher_item_list_view_select_n_last (LauncherItemListView *view,
                                       gint n_last);



G_DEFINE_TYPE (LauncherItemListView, launcher_item_list_view, GTK_TYPE_BOX)



static void
launcher_item_list_view_class_init (LauncherItemListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  signals[ADD_ITEM] = g_signal_new ("add-item",
                                    G_TYPE_FROM_CLASS (object_class),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    NULL,
                                    G_TYPE_NONE, 0);

  signals[EDIT_ITEM] = g_signal_new ("edit-item",
                                     G_TYPE_FROM_CLASS (object_class),
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL, NULL,
                                     NULL,
                                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}



static void
launcher_item_list_view_init (LauncherItemListView *view)
{
  view->list_view = xfce_item_list_view_new (NULL);
  g_signal_connect_swapped (view->list_view, "add-item", G_CALLBACK (launcher_item_list_view_add_item), view);
  g_signal_connect_swapped (view->list_view, "remove-items", G_CALLBACK (launcher_item_list_view_remove_items), view);
  g_signal_connect_swapped (view->list_view, "edit-item", G_CALLBACK (launcher_item_list_view_edit_item), view);
  gtk_box_pack_start (GTK_BOX (view), view->list_view, TRUE, TRUE, 0);
  gtk_widget_show (view->list_view);

  GtkWidget *tree_view = xfce_item_list_view_get_tree_view (XFCE_ITEM_LIST_VIEW (view->list_view));
  g_signal_connect_swapped (tree_view, "drag-data-received", G_CALLBACK (launcher_item_list_view_drag_data_received), view);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

  GtkTreeViewColumn *icon_column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree_view), XFCE_ITEM_LIST_VIEW_COLUMN_ICON);
  GtkCellRenderer *icon_renderer = g_object_get_data (G_OBJECT (icon_column), "renderer");
  gtk_tree_view_column_set_spacing (icon_column, 2);
  g_object_set (icon_renderer, "stock-size", 5, NULL);

  GSimpleActionGroup *action_group = g_simple_action_group_new ();

  GSimpleAction *new_item_action = g_simple_action_new ("new-item", NULL);
  g_signal_connect_swapped (new_item_action, "activate", G_CALLBACK (launcher_item_list_view_new_item), view);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (new_item_action));

  GSimpleAction *new_link_action = g_simple_action_new ("new-link", NULL);
  g_signal_connect_swapped (new_link_action, "activate", G_CALLBACK (launcher_item_list_view_new_link), view);
  g_action_map_add_action (G_ACTION_MAP (action_group), G_ACTION (new_link_action));

  gtk_widget_insert_action_group (GTK_WIDGET (view), "launcher-item-list-view", G_ACTION_GROUP (action_group));

  g_object_unref (action_group);
  g_object_unref (new_item_action);
  g_object_unref (new_link_action);

  GMenu *menu = xfce_item_list_view_get_menu (XFCE_ITEM_LIST_VIEW (view->list_view));

  GMenuItem *new_item_item = g_menu_item_new (NULL, "launcher-item-list-view.new-item");
  GIcon *new_item_icon = g_themed_icon_new ("document-new-symbolic");
  GVariant *new_item_icon_value = g_icon_serialize (new_item_icon);
  g_menu_item_set_attribute_value (new_item_item, G_MENU_ATTRIBUTE_ICON, new_item_icon_value);
  g_menu_item_set_attribute_value (new_item_item, XFCE_MENU_ATTRIBUTE_TOOLTIP, g_variant_new_string (_("Add a new empty item")));
  g_menu_item_set_attribute_value (new_item_item, XFCE_MENU_ATTRIBUTE_HIDE_LABEL, g_variant_new_boolean (TRUE));
  g_menu_append_item (menu, new_item_item);
  g_object_unref (new_item_item);
  g_clear_object (&new_item_icon);
  g_clear_pointer (&new_item_icon_value, g_variant_unref);

  GMenuItem *new_link_item = g_menu_item_new (NULL, "launcher-item-list-view.new-link");
  GIcon *new_link_icon = g_themed_icon_new ("applications-internet-symbolic");
  GVariant *new_link_icon_value = g_icon_serialize (new_link_icon);
  g_menu_item_set_attribute_value (new_link_item, G_MENU_ATTRIBUTE_ICON, new_link_icon_value);
  g_menu_item_set_attribute_value (new_link_item, XFCE_MENU_ATTRIBUTE_TOOLTIP, g_variant_new_string (_("Add a new hyperlink")));
  g_menu_item_set_attribute_value (new_link_item, XFCE_MENU_ATTRIBUTE_HIDE_LABEL, g_variant_new_boolean (TRUE));
  g_menu_append_item (menu, new_link_item);
  g_object_unref (new_link_item);
  g_clear_object (&new_link_icon);
  g_clear_pointer (&new_link_icon_value, g_variant_unref);
}



static gboolean
launcher_item_list_view_add_item (LauncherItemListView *view)
{
  g_signal_emit (view, signals[ADD_ITEM], 0);
  return TRUE;
}



static gboolean
launcher_item_list_view_remove_items (LauncherItemListView *view,
                                      gint *items,
                                      gint n_items)
{
  const gchar *secondary = NULL;
  gchar *primary = NULL;

  if (n_items == 1)
    {
      XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
      GarconMenuItem *item = launcher_item_list_model_get_item (LAUNCHER_ITEM_LIST_MODEL (model), items[0]);
      gchar *name = launcher_get_item_name (item);

      primary = g_strdup_printf (_("Are you sure you want to remove \"%s\"?"), name);
      secondary = _("If you delete an item, it will be permanently removed");
      g_free (name);
    }
  else
    {
      primary = g_strdup_printf (_("Are you sure that you want to remove these %d items?"), n_items);
      secondary = _("If you delete items, they will be deleted permanently");
    }

  GtkWindow *toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (view)));
  gboolean confirmed = xfce_dialog_confirm (toplevel, "edit-delete", _("_Remove"), secondary, "%s", primary);
  g_free (primary);

  return !confirmed;
}



static gboolean
launcher_item_list_view_edit_item (LauncherItemListView *view,
                                   gint index)
{
  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
  GarconMenuItem *item = launcher_item_list_model_get_item (LAUNCHER_ITEM_LIST_MODEL (model), index);

  g_signal_emit (view, signals[EDIT_ITEM], 0, garcon_menu_item_get_uri (item), NULL);
  return FALSE;
}



static void
launcher_item_list_view_new_item (LauncherItemListView *view)
{
  g_signal_emit (view, signals[EDIT_ITEM], 0, NULL, "Application");
}



static void
launcher_item_list_view_new_link (LauncherItemListView *view)
{
  g_signal_emit (view, signals[EDIT_ITEM], 0, NULL, "Link");
}



static void
launcher_item_list_view_drag_data_received (LauncherItemListView *view,
                                            GdkDragContext *context,
                                            gint x,
                                            gint y,
                                            GtkSelectionData *data,
                                            guint info,
                                            guint timestamp,
                                            GtkWidget *tree_view)
{
  switch (info)
    {
    case DROP_TARGET_ROW:
      break;

    case DROP_TARGET_URI:
      gchar **uris = gtk_selection_data_get_uris (data);
      if (uris == NULL)
        {
          gtk_drag_finish (context, FALSE, FALSE, timestamp);
        }
      else
        {
          gint position = -1;
          GtkTreePath *path = NULL;
          GtkTreeViewDropPosition drop_position;
          if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (tree_view), x, y, &path, &drop_position)
              && gtk_tree_path_get_depth (path) > 0)
            {
              position = *gtk_tree_path_get_indices (path);
              if (drop_position == GTK_TREE_VIEW_DROP_AFTER || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
                ++position;
            }
          else
            {
              XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
              position = xfce_item_list_model_get_n_items (model);
            }
          gtk_tree_path_free (path);
          launcher_item_list_view_append_uris (view, position, uris);
          gtk_drag_finish (context, TRUE, FALSE, timestamp);
        }
      g_strfreev (uris);
      break;
    }
}



static void
launcher_item_list_view_append_uris (LauncherItemListView *view,
                                     gint position,
                                     gchar **uris)
{
  GList *items = NULL;
  for (gint i = 0; uris[i] != NULL; ++i)
    {
      if (g_str_has_suffix (uris[i], ".desktop"))
        {
          GarconMenuItem *item = garcon_menu_item_new_for_uri (uris[i]);
          if (item != NULL)
            items = g_list_append (items, item);
        }
    }
  launcher_item_list_view_append (view, items);
  g_list_free_full (items, (GDestroyNotify) g_object_unref);
}



static void
launcher_item_list_view_select_n_last (LauncherItemListView *view,
                                       gint n_last)
{
  g_return_if_fail (LAUNCHER_IS_ITEM_LIST_VIEW (view));

  GtkWidget *tree_view = xfce_item_list_view_get_tree_view (XFCE_ITEM_LIST_VIEW (view->list_view));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  gtk_tree_selection_unselect_all (selection);

  XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
  gint n_items = xfce_item_list_model_get_n_items (model);
  g_return_if_fail (n_last <= n_items);
  gint index_start = n_items - n_last;

  for (gint i = index_start; i < n_items; ++i)
    {
      if (i == index_start)
        {
          GtkTreePath *cursor_path = gtk_tree_path_new_from_indices (index_start, -1);
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree_view), cursor_path, NULL, FALSE);
          gtk_tree_path_free (cursor_path);
        }

      GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
      gtk_tree_selection_select_path (selection, path);
      gtk_tree_path_free (path);
    }
}



void
launcher_item_list_view_set_model (LauncherItemListView *view,
                                   LauncherItemListModel *model)
{
  g_return_if_fail (LAUNCHER_IS_ITEM_LIST_VIEW (view));
  g_return_if_fail (LAUNCHER_IS_ITEM_LIST_MODEL (model));

  xfce_item_list_view_set_model (XFCE_ITEM_LIST_VIEW (view->list_view), XFCE_ITEM_LIST_MODEL (model));

  GtkWidget *tree_view = xfce_item_list_view_get_tree_view (XFCE_ITEM_LIST_VIEW (view->list_view));
  static const GtkTargetEntry drop_targets[] = {
    { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, DROP_TARGET_ROW },
    { "text/uri-list", 0, DROP_TARGET_URI },
  };
  gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (tree_view), drop_targets, G_N_ELEMENTS (drop_targets), GDK_ACTION_MOVE);
}



void
launcher_item_list_view_append (LauncherItemListView *view,
                                GList *items)
{
  g_return_if_fail (LAUNCHER_IS_ITEM_LIST_VIEW (view));

  if (items != NULL)
    {
      XfceItemListModel *model = xfce_item_list_view_get_model (XFCE_ITEM_LIST_VIEW (view->list_view));
      gint index = xfce_item_list_model_get_n_items (model);
      launcher_item_list_model_insert (LAUNCHER_ITEM_LIST_MODEL (model), index, items);
      launcher_item_list_view_select_n_last (view, g_list_length (items));
    }
}
