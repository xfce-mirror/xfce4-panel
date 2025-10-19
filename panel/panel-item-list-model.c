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

#include "panel-item-list-model.h"
#include "panel-application.h"
#include "panel-item-dialog.h"
#include "panel-itembar.h"
#include "panel-module.h"
#include "panel-plugin-external.h"

#include "common/panel-private.h"

struct _PanelItemListModel
{
  XfceItemListModel __parent__;

  PanelApplication *application;
  XfconfChannel *channel;
  PanelWindow *panel;
  PanelItembar *itembar;
  GList *items;
};



static void
panel_item_list_model_finalize (GObject *object);
static gint
panel_item_list_model_get_list_n_columns (XfceItemListModel *list_model);
static GType
panel_item_list_model_get_list_column_type (XfceItemListModel *list_model,
                                            gint column);
static gint
panel_item_list_model_get_n_items (XfceItemListModel *list_model);
static void
panel_item_list_model_get_item_value (XfceItemListModel *list_model,
                                      gint index,
                                      gint column,
                                      GValue *value);
static void
panel_item_list_model_move (XfceItemListModel *list_model,
                            gint source_index,
                            gint dest_index);
static gboolean
panel_item_list_model_remove (XfceItemListModel *list_model,
                              gint index);
static void
panel_item_list_model_set_panel (PanelItemListModel *model,
                                 PanelWindow *panel);
static void
panel_item_list_model_reload_items (PanelItemListModel *model);
static void
panel_item_list_model_reload (PanelItemListModel *model);
static GKeyFile *
panel_item_list_model_get_launcher_keyfile (PanelItemListModel *model,
                                            gint index);
static GIcon *
panel_item_list_model_get_launcher_icon (PanelItemListModel *model,
                                         gint index);
static gchar *
panel_item_list_model_get_launcher_name (PanelItemListModel *model,
                                         gint index);
gboolean
panel_item_list_model_is_launcher (PanelItemListModel *model,
                                   gint index);
static GIcon *
panel_item_list_model_get_item_icon (PanelItemListModel *model,
                                     gint index);
static gchar *
panel_item_list_model_get_item_name (PanelItemListModel *model,
                                     gint index);
static gchar *
panel_item_list_model_get_item_tooltip (PanelItemListModel *model,
                                        gint index);



G_DEFINE_TYPE (PanelItemListModel, panel_item_list_model, XFCE_TYPE_ITEM_LIST_MODEL)



static void
panel_item_list_model_class_init (PanelItemListModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  XfceItemListModelClass *list_class = XFCE_ITEM_LIST_MODEL_CLASS (klass);

  object_class->finalize = panel_item_list_model_finalize;

  list_class->get_list_n_columns = panel_item_list_model_get_list_n_columns;
  list_class->get_list_column_type = panel_item_list_model_get_list_column_type;
  list_class->get_n_items = panel_item_list_model_get_n_items;
  list_class->get_item_value = panel_item_list_model_get_item_value;
  list_class->move = panel_item_list_model_move;
  list_class->remove = panel_item_list_model_remove;
}


static void
panel_item_list_model_init (PanelItemListModel *model)
{
  model->application = panel_application_get ();
  model->channel = xfconf_channel_get (XFCE_PANEL_CHANNEL_NAME);
}



static void
panel_item_list_model_finalize (GObject *object)
{
  PanelItemListModel *model = PANEL_ITEM_LIST_MODEL (object);

  if (model->itembar != NULL)
    g_signal_handlers_disconnect_by_data (model->itembar, model);

  g_clear_object (&model->panel);
  g_clear_object (&model->itembar);
  g_clear_pointer (&model->items, g_list_free);
  G_OBJECT_CLASS (panel_item_list_model_parent_class)->finalize (object);
}



static gint
panel_item_list_model_get_list_n_columns (XfceItemListModel *list_model)
{
  return PANEL_ITEM_LIST_MODEL_N_COLUMNS;
}



static GType
panel_item_list_model_get_list_column_type (XfceItemListModel *list_model,
                                            gint column)
{
  switch (column)
    {
    case PANEL_ITEM_LIST_MODEL_COLUMN_ABOUT:
      return G_TYPE_BOOLEAN;

    default:
      return XFCE_ITEM_LIST_MODEL_CLASS (panel_item_list_model_parent_class)->get_list_column_type (list_model, column);
    }
}



static gint
panel_item_list_model_get_n_items (XfceItemListModel *list_model)
{
  PanelItemListModel *model = PANEL_ITEM_LIST_MODEL (list_model);

  return g_list_length (model->items);
}



static void
panel_item_list_model_get_item_value (XfceItemListModel *list_model,
                                      gint index,
                                      gint column,
                                      GValue *value)
{
  PanelItemListModel *model = PANEL_ITEM_LIST_MODEL (list_model);
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);

  switch (column)
    {
    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE:
      g_value_set_boolean (value, TRUE);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE:
      g_value_set_boolean (value, FALSE);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ICON:
      GIcon *icon = panel_item_list_model_get_item_icon (model, index);
      g_value_set_object (value, icon);
      g_object_unref (icon);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_NAME:
      gchar *name = panel_item_list_model_get_item_name (model, index);
      g_value_set_string (value, name);
      g_free (name);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP:
      gchar *tooltip = panel_item_list_model_get_item_tooltip (model, index);
      g_value_set_string (value, tooltip);
      g_free (tooltip);
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE:
      g_value_set_boolean (value, !panel_window_get_locked (model->panel) && xfce_panel_plugin_provider_get_show_configure (provider));
      break;

    case XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE:
      g_value_set_boolean (value, !panel_window_get_locked (model->panel));
      break;

    case PANEL_ITEM_LIST_MODEL_COLUMN_ABOUT:
      g_value_set_boolean (value, xfce_panel_plugin_provider_get_show_about (provider));
      break;

    default:
      g_warn_if_reached ();
    }
}



static void
panel_item_list_model_move (XfceItemListModel *list_model,
                            gint source_index,
                            gint dest_index)
{
  PanelItemListModel *model = PANEL_ITEM_LIST_MODEL (list_model);
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, source_index);

  g_signal_handlers_block_by_func (model->itembar, G_CALLBACK (panel_item_list_model_reload), model);
  {
    panel_itembar_reorder_child (PANEL_ITEMBAR (model->itembar), GTK_WIDGET (provider), dest_index);
    panel_application_save_window (model->application, PANEL_WINDOW (model->panel), SAVE_PLUGIN_IDS);
    panel_item_list_model_reload_items (model);
  }
  g_signal_handlers_unblock_by_func (model->itembar, G_CALLBACK (panel_item_list_model_reload), model);
}



static gboolean
panel_item_list_model_remove (XfceItemListModel *list_model,
                              gint index)
{
  PanelItemListModel *model = PANEL_ITEM_LIST_MODEL (list_model);
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);

  g_signal_handlers_block_by_func (model->itembar, G_CALLBACK (panel_item_list_model_reload), model);
  {
    xfce_panel_plugin_provider_emit_signal (provider, PROVIDER_SIGNAL_REMOVE_PLUGIN);
    panel_item_list_model_reload_items (model);
  }
  g_signal_handlers_unblock_by_func (model->itembar, G_CALLBACK (panel_item_list_model_reload), model);

  return TRUE;
}



static void
panel_item_list_model_set_panel (PanelItemListModel *model,
                                 PanelWindow *panel)
{
  if (model->itembar != NULL)
    g_signal_handlers_disconnect_by_data (model->itembar, model);

  g_clear_object (&model->panel);
  g_clear_object (&model->itembar);

  if (panel != NULL)
    {
      PanelItembar *itembar = PANEL_ITEMBAR (gtk_bin_get_child (GTK_BIN (panel)));
      model->panel = g_object_ref (panel);
      model->itembar = g_object_ref (itembar);
      g_signal_connect_swapped (model->itembar, "changed", G_CALLBACK (panel_item_list_model_reload), model);
    }

  if (panel == NULL || panel_window_get_locked (panel))
    {
      g_object_set (model, "list-flags", XFCE_ITEM_LIST_MODEL_NONE, NULL);
    }
  else
    {
      g_object_set (model, "list-flags",
                    XFCE_ITEM_LIST_MODEL_REORDERABLE
                      | XFCE_ITEM_LIST_MODEL_EDITABLE
                      | XFCE_ITEM_LIST_MODEL_ADDABLE
                      | XFCE_ITEM_LIST_MODEL_REMOVABLE,
                    NULL);
    }


  panel_item_list_model_reload (model);
}



static void
panel_item_list_model_reload_items (PanelItemListModel *model)
{
  g_clear_pointer (&model->items, g_list_free);

  if (model->itembar != NULL)
    model->items = gtk_container_get_children (GTK_CONTAINER (model->itembar));
}



static void
panel_item_list_model_reload (PanelItemListModel *model)
{
  /* Inform GktTreeModel about changes */
  gint prev_n_items = g_list_length (model->items);

  panel_item_list_model_reload_items (model);
  gint cur_n_items = g_list_length (model->items);

  for (gint i = prev_n_items - 1; i >= cur_n_items; --i)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
      gtk_tree_path_free (path);
    }

  for (gint i = prev_n_items; i < cur_n_items; ++i)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
      GtkTreeIter iter;
      xfce_item_list_model_set_index (XFCE_ITEM_LIST_MODEL (model), &iter, i);
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
    }

  xfce_item_list_model_changed (XFCE_ITEM_LIST_MODEL (model));
}



static GKeyFile *
panel_item_list_model_get_launcher_keyfile (PanelItemListModel *model,
                                            gint index)
{
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);
  gchar *property_name = g_strdup_printf (PLUGINS_PROPERTY_BASE "/items", xfce_panel_plugin_provider_get_unique_id (provider));
  GKeyFile *keyfile = NULL;

  if (xfconf_channel_has_property (model->channel, property_name))
    {
      gchar **desktop_files = xfconf_channel_get_string_list (model->channel, property_name);
      if (desktop_files[0] != NULL)
        {
          gchar *dirname = g_strdup_printf (PANEL_PLUGIN_RELATIVE_PATH G_DIR_SEPARATOR_S "%s-%d",
                                            xfce_panel_plugin_provider_get_name (provider),
                                            xfce_panel_plugin_provider_get_unique_id (provider));
          gchar *filename = g_build_filename (dirname, desktop_files[0], NULL);
          gchar *path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, filename, FALSE);

          keyfile = g_key_file_new ();
          if (!g_key_file_load_from_file (keyfile, path, G_KEY_FILE_NONE, NULL))
            g_clear_pointer (&keyfile, g_key_file_free);

          g_free (path);
          g_free (filename);
          g_free (dirname);
        }
      g_strfreev (desktop_files);
    }
  return keyfile;
}



static GIcon *
panel_item_list_model_get_launcher_icon (PanelItemListModel *model,
                                         gint index)
{
  GKeyFile *keyfile = panel_item_list_model_get_launcher_keyfile (model, index);
  gchar *icon_name = g_key_file_get_string (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_ICON, NULL);
  GIcon *icon = NULL;

  if (icon_name != NULL)
    {
      if (g_path_is_absolute (icon_name))
        {
          GFile *file = g_file_new_for_path (icon_name);
          icon = g_file_icon_new (file);
          g_object_unref (file);
        }
      else
        {
          icon = g_themed_icon_new (icon_name);
        }
    }
  g_free (icon_name);
  g_key_file_free (keyfile);
  return icon;
}



static gchar *
panel_item_list_model_get_launcher_name (PanelItemListModel *model,
                                         gint index)
{
  GKeyFile *keyfile = panel_item_list_model_get_launcher_keyfile (model, index);
  gchar *name = g_key_file_get_locale_string (keyfile, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_NAME, NULL, NULL);

  g_key_file_free (keyfile);
  return name;
}



gboolean
panel_item_list_model_is_launcher (PanelItemListModel *model,
                                   gint index)
{
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);

  return g_strcmp0 (xfce_panel_plugin_provider_get_name (provider), "launcher") == 0;
}



static GIcon *
panel_item_list_model_get_item_icon (PanelItemListModel *model,
                                     gint index)
{
  GIcon *icon = NULL;

  if (panel_item_list_model_is_launcher (model, index))
    icon = panel_item_list_model_get_launcher_icon (model, index);

  if (icon == NULL)
    {
      XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);
      PanelModule *module = panel_module_get_from_plugin_provider (provider);
      icon = g_themed_icon_new (panel_module_get_icon_name (module));
    }

  return icon;
}



static gchar *
panel_item_list_model_get_item_name (PanelItemListModel *model,
                                     gint index)
{
  gchar *name = NULL;

  if (panel_item_list_model_is_launcher (model, index))
    name = panel_item_list_model_get_launcher_name (model, index);

  if (name == NULL)
    {
      XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);
      PanelModule *module = panel_module_get_from_plugin_provider (provider);
      const gchar *module_name = panel_module_get_display_name (module);

      if (PANEL_IS_PLUGIN_EXTERNAL (provider))
        {
          /* I18N: append (external) in the preferences dialog if the plugin
           * runs external */
          name = g_strdup_printf (_("%s <span color=\"grey\" size=\"small\">(external)</span>"), module_name);
        }
      else
        {
          name = g_strdup (module_name);
        }
    }

  return name;
}



static gchar *
panel_item_list_model_get_item_tooltip (PanelItemListModel *model,
                                        gint index)
{
  XfcePanelPluginProvider *provider = panel_item_list_model_get_item_provider (model, index);
  gchar *tooltip = NULL;

  if (PANEL_IS_PLUGIN_EXTERNAL (provider))
    {
      /* I18N: tooltip in preferences dialog when hovering an item in the list
       * for external plugins */
      tooltip = g_strdup_printf (_("Internal name: %s-%d\n"
                                   "PID: %d"),
                                 xfce_panel_plugin_provider_get_name (provider),
                                 xfce_panel_plugin_provider_get_unique_id (provider),
                                 panel_plugin_external_get_pid (PANEL_PLUGIN_EXTERNAL (provider)));
    }
  else
    {
      /* I18N: tooltip in preferences dialog when hovering an item in the list
       * for internal plugins */
      tooltip = g_strdup_printf (_("Internal name: %s-%d"),
                                 xfce_panel_plugin_provider_get_name (provider),
                                 xfce_panel_plugin_provider_get_unique_id (provider));
    }

  return tooltip;
}



XfceItemListModel *
panel_item_list_model_new (PanelWindow *panel)
{
  g_return_val_if_fail (panel == NULL || PANEL_IS_WINDOW (panel), NULL);

  PanelItemListModel *model = g_object_new (PANEL_TYPE_ITEM_LIST_MODEL, NULL);
  panel_item_list_model_set_panel (model, panel);
  return XFCE_ITEM_LIST_MODEL (model);
}



PanelWindow *
panel_item_list_model_get_panel (PanelItemListModel *model)
{
  g_return_val_if_fail (PANEL_IS_ITEM_LIST_MODEL (model), NULL);

  return model->panel;
}



XfcePanelPluginProvider *
panel_item_list_model_get_item_provider (PanelItemListModel *model,
                                         gint index)
{
  g_return_val_if_fail (PANEL_IS_ITEM_LIST_MODEL (model), NULL);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (XFCE_ITEM_LIST_MODEL (model)), NULL);

  return g_list_nth_data (model->items, index);
}
