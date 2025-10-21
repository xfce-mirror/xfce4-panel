#include "launcher-item-list-model.h"
#include "launcher-item-info.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <garcon/garcon.h>
#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>



struct _LauncherItemListModel
{
  XfceItemListModel __parent__;

  LauncherPlugin *plugin;
  GSList *items;
};



static void
launcher_item_list_model_finalize (GObject *object);
static gint
launcher_item_list_model_get_n_items (XfceItemListModel *list_model);
static void
launcher_item_list_model_get_item_value (XfceItemListModel *list_model,
                                         gint index,
                                         gint column,
                                         GValue *value);
static void
launcher_item_list_model_move (XfceItemListModel *list_model,
                               gint source_index,
                               gint dest_index);
static gboolean
launcher_item_list_model_remove (XfceItemListModel *list_model,
                                 gint index);
static void
launcher_item_list_model_save (LauncherItemListModel *model);
static void
launcher_item_list_model_reload_items (LauncherItemListModel *model);
static void
launcher_item_list_model_reload (LauncherItemListModel *model);
static void
launcher_item_list_model_set_plugin (LauncherItemListModel *model,
                                     LauncherPlugin *plugin);



G_DEFINE_TYPE (LauncherItemListModel, launcher_item_list_model, XFCE_TYPE_ITEM_LIST_MODEL)



static void
launcher_item_list_model_class_init (LauncherItemListModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  XfceItemListModelClass *list_class = XFCE_ITEM_LIST_MODEL_CLASS (klass);

  object_class->finalize = launcher_item_list_model_finalize;

  list_class->get_n_items = launcher_item_list_model_get_n_items;
  list_class->get_item_value = launcher_item_list_model_get_item_value;
  list_class->move = launcher_item_list_model_move;
  list_class->remove = launcher_item_list_model_remove;
}



static void
launcher_item_list_model_init (LauncherItemListModel *model)
{
  g_object_set (model, "list-flags",
                XFCE_ITEM_LIST_MODEL_REORDERABLE
                  | XFCE_ITEM_LIST_MODEL_EDITABLE
                  | XFCE_ITEM_LIST_MODEL_ADDABLE
                  | XFCE_ITEM_LIST_MODEL_REMOVABLE,
                NULL);
}



static void
launcher_item_list_model_finalize (GObject *object)
{
  LauncherItemListModel *model = LAUNCHER_ITEM_LIST_MODEL (object);

  if (model->plugin != NULL)
    g_signal_handlers_disconnect_by_data (model->plugin, model);

  g_clear_object (&model->plugin);
  g_slist_free_full (model->items, (GDestroyNotify) g_object_unref);
  G_OBJECT_CLASS (launcher_item_list_model_parent_class)->finalize (object);
}



static gint
launcher_item_list_model_get_n_items (XfceItemListModel *list_model)
{
  LauncherItemListModel *model = LAUNCHER_ITEM_LIST_MODEL (list_model);

  return g_slist_length (model->items);
}



static void
launcher_item_list_model_get_item_value (XfceItemListModel *list_model,
                                         gint index,
                                         gint column,
                                         GValue *value)
{
  LauncherItemListModel *model = LAUNCHER_ITEM_LIST_MODEL (list_model);
  GarconMenuItem *item = launcher_item_list_model_get_item (model, index);

  switch (column)
    {
    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE:
      g_value_set_boolean (value, TRUE);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE:
      g_value_set_boolean (value, FALSE);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ICON:
      g_value_take_object (value, launcher_get_item_icon (item));
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_NAME:
      g_value_take_string (value, launcher_get_item_name_markup (item));
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP:
      g_value_take_string (value, launcher_get_item_tooltip (item));
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE:
      g_value_set_boolean (value, launcher_plugin_item_is_editable (model->plugin, item, NULL));
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE:
      gboolean removable = FALSE;
      gboolean editable = launcher_plugin_item_is_editable (model->plugin, item, &removable);
      g_value_set_boolean (value, editable && removable);
      break;
    }
}



static void
launcher_item_list_model_move (XfceItemListModel *list_model,
                               gint source_index,
                               gint dest_index)
{
  LauncherItemListModel *model = LAUNCHER_ITEM_LIST_MODEL (list_model);
  GSList *link = g_slist_nth (model->items, source_index);
  GarconMenuItem *item = link->data;

  model->items = g_slist_delete_link (model->items, link);
  model->items = g_slist_insert (model->items, item, dest_index);
  launcher_item_list_model_save (model);
}



static gboolean
launcher_item_list_model_remove (XfceItemListModel *list_model,
                                 gint index)
{
  LauncherItemListModel *model = LAUNCHER_ITEM_LIST_MODEL (list_model);
  GSList *link = g_slist_nth (model->items, index);
  GarconMenuItem *item = link->data;

  model->items = g_slist_delete_link (model->items, link);
  g_object_unref (item);
  launcher_item_list_model_save (model);
  return TRUE;
}



static void
launcher_item_list_model_save (LauncherItemListModel *model)
{
  GPtrArray *save_items = g_ptr_array_new ();

  for (GSList *l = model->items; l != NULL; l = l->next)
    {
      GValue *save_item = g_new0 (GValue, 1);

      g_value_init (save_item, G_TYPE_STRING);
      g_value_take_string (save_item, garcon_menu_item_get_uri (l->data));
      g_ptr_array_add (save_items, save_item);
    }

  g_object_set (model->plugin, "items", save_items, NULL);
  xfconf_array_free (save_items);

  launcher_item_list_model_reload_items (model);
}



static void
launcher_item_list_model_reload_items (LauncherItemListModel *model)
{
  g_slist_free_full (model->items, (GDestroyNotify) g_object_unref);
  model->items = launcher_plugin_get_items (model->plugin);
}



static void
launcher_item_list_model_reload (LauncherItemListModel *model)
{
  gint n_prev_items = xfce_item_list_model_get_n_items (XFCE_ITEM_LIST_MODEL (model));

  launcher_item_list_model_reload_items (model);
  xfce_item_list_model_reloaded (XFCE_ITEM_LIST_MODEL (model), n_prev_items);
}


static void
launcher_item_list_model_set_plugin (LauncherItemListModel *model,
                                     LauncherPlugin *plugin)
{
  g_return_if_fail (LAUNCHER_IS_PLUGIN (plugin));
  g_return_if_fail (model->plugin == NULL);

  model->plugin = g_object_ref (plugin);
  launcher_item_list_model_reload_items (model);
  g_signal_connect_swapped (plugin, "items-changed", G_CALLBACK (launcher_item_list_model_reload), model);
}



XfceItemListModel *
launcher_item_list_model_new (LauncherPlugin *plugin)
{
  LauncherItemListModel *model = g_object_new (LAUNCHER_TYPE_ITEM_LIST_MODEL, NULL);

  launcher_item_list_model_set_plugin (model, plugin);
  return XFCE_ITEM_LIST_MODEL (model);
}



GarconMenuItem *
launcher_item_list_model_get_item (LauncherItemListModel *model,
                                   gint index)
{
  g_return_val_if_fail (LAUNCHER_IS_ITEM_LIST_MODEL (model), NULL);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (XFCE_ITEM_LIST_MODEL (model)), NULL);

  return g_slist_nth_data (model->items, index);
}


void
launcher_item_list_model_insert (LauncherItemListModel *model,
                                 gint index,
                                 GList *items)
{
  g_return_if_fail (LAUNCHER_IS_ITEM_LIST_MODEL (model));
  g_return_if_fail (index >= 0 && index <= xfce_item_list_model_get_n_items (XFCE_ITEM_LIST_MODEL (model)));

  for (GList *l = items; l != NULL; l = l->next, ++index)
    {
      model->items = g_slist_insert (model->items, g_object_ref (l->data), index);

      GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
      GtkTreeIter iter;
      xfce_item_list_model_set_index (XFCE_ITEM_LIST_MODEL (model), &iter, index);
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
    }
  launcher_item_list_model_save (model);
}
