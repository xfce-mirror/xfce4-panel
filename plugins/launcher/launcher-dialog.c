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

#include "launcher-dialog.h"
#include "launcher-item-info.h"
#include "launcher-item-list-view.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <garcon/garcon.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>



typedef struct
{
  LauncherPlugin *plugin;
  GtkBuilder *builder;
} LauncherPluginDialog;

enum
{
  COL_ICON,
  COL_NAME,
  COL_ITEM,
  COL_TOOLTIP,
  COL_SEARCH,
};



static void
launcher_dialog_add_store_insert (gpointer key,
                                  gpointer value,
                                  gpointer user_data);
static void
launcher_dialog_add_show (LauncherPluginDialog *dialog);
static void
launcher_desktop_item_editor_show (LauncherPluginDialog *dialog,
                                   const gchar *uri,
                                   const gchar *type);
static gboolean
launcher_dialog_press_event (LauncherPluginDialog *dialog,
                             const gchar *object_name);



static const GtkTargetEntry add_drag_targets[] = {
  { "text/uri-list", 0, 0 }
};



static void
launcher_dialog_add_store_insert (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
  /* Append row */
  GarconMenuItem *item = GARCON_MENU_ITEM (value);
  GtkTreeModel *model = GTK_TREE_MODEL (user_data);
  GtkListStore *store = user_data;
  GtkTreeIter iter;

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);

  /* Filling a row */
  GIcon *icon = launcher_get_item_icon (item);
  gchar *name = launcher_get_item_name (item);
  gchar *tooltip = launcher_get_item_tooltip (item);

  gtk_list_store_set (store, &iter,
                      COL_ICON, icon,
                      COL_NAME, name,
                      COL_ITEM, item,
                      COL_TOOLTIP, tooltip,
                      -1);
  g_clear_object (&icon);
  g_free (name);
  g_free (tooltip);
}



static void
launcher_dialog_add_show (LauncherPluginDialog *dialog)
{
  GObject *dialog_add = gtk_builder_get_object (dialog->builder, "dialog-add");
  GHashTable *pool = launcher_plugin_garcon_menu_pool ();
  GObject *store = gtk_builder_get_object (dialog->builder, "add-store");

  gtk_list_store_clear (GTK_LIST_STORE (store));
  g_hash_table_foreach (pool, launcher_dialog_add_store_insert, GTK_LIST_STORE (store));
  g_hash_table_destroy (pool);
  gtk_widget_show (GTK_WIDGET (dialog_add));
}



static void
launcher_desktop_item_editor_show (LauncherPluginDialog *dialog,
                                   const gchar *uri,
                                   const gchar *type)
{
  gchar *command = NULL;

  if (uri != NULL)
    {
      command = g_strdup_printf ("xfce-desktop-item-edit '%s'", uri);
    }
  else
    {
      gchar *filename = launcher_plugin_unique_filename (dialog->plugin);

      command = g_strdup_printf ("xfce-desktop-item-edit -t %s -c '%s'", type, filename);
      g_free (filename);
    }

  /* spawn item editor */
  GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "dialog"));
  GdkScreen *screen = gtk_widget_get_screen (widget);
  GError *error = NULL;

  if (!xfce_spawn_command_line (screen, command, FALSE, FALSE, TRUE, &error))
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

      xfce_dialog_show_error (GTK_WINDOW (toplevel), error, _("Failed to open desktop item editor"));
      g_error_free (error);
    }
  g_free (command);
}



static gboolean
launcher_dialog_add_visible_function (GtkTreeModel *model,
                                      GtkTreeIter *iter,
                                      gpointer user_data)
{
  gchar *string, *escaped;
  gboolean visible = FALSE;
  const gchar *text;
  gchar *normalized;
  gchar *text_casefolded;
  gchar *name_casefolded;

  /* get the search string from the item */
  text = gtk_entry_get_text (GTK_ENTRY (user_data));
  if (G_UNLIKELY (xfce_str_is_empty (text)))
    return TRUE;

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  /* try the pre-build search string first */
  gtk_tree_model_get (model, iter, COL_SEARCH, &string, -1);
  if (!xfce_str_is_empty (string))
    {
      /* search */
      visible = (strstr (string, text_casefolded) != NULL);
    }
  else
    {
      /* get the name */
      gtk_tree_model_get (model, iter, COL_NAME, &string, -1);
      if (!xfce_str_is_empty (string))
        {
          /* escape and casefold the name */
          escaped = g_markup_escape_text (string, -1);
          normalized = g_utf8_normalize (escaped, -1, G_NORMALIZE_ALL);
          name_casefolded = g_utf8_casefold (normalized, -1);
          g_free (normalized);
          g_free (escaped);

          /* search */
          visible = (strstr (name_casefolded, text_casefolded) != NULL);

          /* store the generated search string */
          gtk_list_store_set (GTK_LIST_STORE (model), iter, COL_SEARCH,
                              name_casefolded, -1);

          g_free (name_casefolded);
        }
    }

  g_free (text_casefolded);
  g_free (string);

  return visible;
}



static void
launcher_dialog_add_drag_data_get (GtkWidget *treeview,
                                   GdkDragContext *drag_context,
                                   GtkSelectionData *data,
                                   guint info,
                                   guint timestamp,
                                   LauncherPluginDialog *dialog)
{
  GtkTreeSelection *selection;
  GList *rows, *li;
  GtkTreeModel *model;
  gchar **uris;
  GarconMenuItem *item;
  guint i;
  GtkTreeIter iter;

  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  rows = gtk_tree_selection_get_selected_rows (selection, &model);
  if (G_UNLIKELY (rows == NULL))
    return;

  uris = g_new0 (gchar *, g_list_length (rows) + 1);
  for (li = rows, i = 0; li != NULL; li = li->next)
    {
      if (!gtk_tree_model_get_iter (model, &iter, li->data))
        continue;

      gtk_tree_model_get (model, &iter, COL_ITEM, &item, -1);
      if (G_UNLIKELY (item == NULL))
        continue;

      uris[i++] = garcon_menu_item_get_uri (item);
      g_object_unref (G_OBJECT (item));
    }

  gtk_selection_data_set_uris (data, uris);
  g_list_free (rows);
  g_strfreev (uris);
}



static void
launcher_dialog_add_selection_changed (GtkTreeSelection *selection,
                                       LauncherPluginDialog *dialog)
{
  GObject *object;
  gboolean sensitive;

  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (GTK_IS_TREE_SELECTION (selection));

  object = gtk_builder_get_object (dialog->builder, "button-add");
  sensitive = !!(gtk_tree_selection_count_selected_rows (selection) > 0);
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);
}



static gboolean
launcher_dialog_add_button_press_event (GtkTreeView *treeview,
                                        GdkEventButton *event,
                                        LauncherPluginDialog *dialog)
{
  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);
  panel_return_val_if_fail (GTK_IS_TREE_VIEW (treeview), FALSE);

  if (event->button == 1
      && event->type == GDK_2BUTTON_PRESS
      && event->window == gtk_tree_view_get_bin_window (treeview)
      && gtk_tree_view_get_path_at_pos (treeview, event->x, event->y,
                                        NULL, NULL, NULL, NULL))
    return launcher_dialog_press_event (dialog, "button-add");

  return FALSE;
}



static gboolean
launcher_dialog_add_key_press_event (GtkTreeView *treeview,
                                     GdkEventKey *event,
                                     LauncherPluginDialog *dialog)
{
  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);
  panel_return_val_if_fail (GTK_IS_TREE_VIEW (treeview), FALSE);

  if (event->keyval == GDK_KEY_Return
      || event->keyval == GDK_KEY_KP_Enter)
    return launcher_dialog_press_event (dialog, "button-add");

  return FALSE;
}



static void
launcher_dialog_add_response (GtkWidget *widget,
                              gint response_id,
                              LauncherPluginDialog *dialog)
{
  GObject *store;

  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (LAUNCHER_IS_PLUGIN (dialog->plugin));

  if (response_id == 1)
    {
      /* add all the selected rows in the add dialog */
      GObject *treeview = gtk_builder_get_object (dialog->builder, "add-treeview");
      GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      GtkTreeModel *add_model;
      GList *list = gtk_tree_selection_get_selected_rows (selection, &add_model);
      GList *items = NULL;

      /* creating a list of items */
      for (GList *l = list; l != NULL; l = l->next)
        {
          GarconMenuItem *item;
          GtkTreeIter iter;

          /* get the selected file in the add dialog */
          gtk_tree_model_get_iter (add_model, &iter, l->data);
          gtk_tree_model_get (add_model, &iter, COL_ITEM, &item, -1);

          if (item != NULL)
            items = g_list_append (items, item);
        }

      /* adding new items */
      GObject *item_list_view = gtk_builder_get_object (dialog->builder, "item-list-view");

      launcher_item_list_view_append (LAUNCHER_ITEM_LIST_VIEW (item_list_view), items);

      g_list_free (list);
      g_list_free (items);
    }
  else
    {
      /* empty the store */
      store = gtk_builder_get_object (dialog->builder, "add-store");
      gtk_list_store_clear (GTK_LIST_STORE (store));

      /* hide the dialog, since it's owned by gtkbuilder */
      gtk_widget_hide (widget);
    }
}



static gboolean
launcher_dialog_press_event (LauncherPluginDialog *dialog,
                             const gchar *object_name)
{
  GObject *object;

  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);

  object = gtk_builder_get_object (dialog->builder, object_name);
  panel_return_val_if_fail (GTK_IS_BUTTON (object), FALSE);
  if (gtk_widget_get_sensitive (GTK_WIDGET (object)))
    {
      gtk_button_clicked (GTK_BUTTON (object));
      return TRUE;
    }

  return FALSE;
}



static void
launcher_dialog_response (GtkWidget *widget,
                          gint response_id,
                          LauncherPluginDialog *dialog)
{
  GObject *add_dialog;

  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (LAUNCHER_IS_PLUGIN (dialog->plugin));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  if (G_UNLIKELY (response_id != 1))
    {
      /* also destroy the add dialog */
      add_dialog = gtk_builder_get_object (dialog->builder, "dialog-add");
      gtk_widget_destroy (GTK_WIDGET (add_dialog));

      /* destroy the dialog */
      gtk_widget_destroy (widget);

      g_slice_free (LauncherPluginDialog, dialog);
    }
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  LauncherPluginDialog *dialog;
  GtkBuilder *builder;
  GObject **window, *object, *item;
  guint i;
  GtkTreeSelection *selection;
  const gchar *binding_names[] = { "disable-tooltips", "show-label",
                                   "move-first", "arrow-position" };

  panel_return_if_fail (LAUNCHER_IS_PLUGIN (plugin));

  /* setup the dialog */
  window = launcher_plugin_get_settings_dialog_pointer (plugin);
  g_type_ensure (LAUNCHER_TYPE_ITEM_LIST_VIEW);
  builder = panel_utils_builder_new (XFCE_PANEL_PLUGIN (plugin), "/org/xfce/panel/launcher-dialog.glade", window);
  if (G_UNLIKELY (builder == NULL))
    return;

  launcher_plugin_set_settings_dialog (plugin, *window);

  /* create structure */
  dialog = g_slice_new0 (LauncherPluginDialog);
  dialog->builder = builder;
  dialog->plugin = plugin;

  g_signal_connect (G_OBJECT (*window), "response",
                    G_CALLBACK (launcher_dialog_response), dialog);

  /* connect bindings to the advanced properties */
  for (i = 0; i < G_N_ELEMENTS (binding_names); i++)
    {
      object = gtk_builder_get_object (builder, binding_names[i]);
      panel_return_if_fail (GTK_IS_WIDGET (object));
      g_object_bind_property (G_OBJECT (plugin), binding_names[i],
                              G_OBJECT (object), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
    }

  /* setup responses for the add dialog */
  object = gtk_builder_get_object (builder, "dialog-add");
  gtk_window_set_screen (GTK_WINDOW (object),
                         gtk_window_get_screen (GTK_WINDOW (*window)));
  g_signal_connect (G_OBJECT (object), "response",
                    G_CALLBACK (launcher_dialog_add_response), dialog);
  g_signal_connect (G_OBJECT (object), "delete-event",
                    G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  /* setup sorting in the add dialog */
  object = gtk_builder_get_object (builder, "add-store");
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (object),
                                        COL_NAME, GTK_SORT_ASCENDING);

  /* setup item-list-view */
  object = gtk_builder_get_object (builder, "item-list-view");
  g_signal_connect_swapped (object, "add-item", G_CALLBACK (launcher_dialog_add_show), dialog);
  g_signal_connect_swapped (object, "edit-item", G_CALLBACK (launcher_desktop_item_editor_show), dialog);

  /* install model */
  XfceItemListModel *model = launcher_item_list_model_new (plugin);

  launcher_item_list_view_set_model (LAUNCHER_ITEM_LIST_VIEW (object), LAUNCHER_ITEM_LIST_MODEL (model));
  g_object_unref (model);

  /* allow selecting multiple items in the add dialog */
  object = gtk_builder_get_object (builder, "add-treeview");
  gtk_drag_source_set (GTK_WIDGET (object), GDK_BUTTON1_MASK,
                       add_drag_targets, G_N_ELEMENTS (add_drag_targets), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (object), "drag-data-get",
                    G_CALLBACK (launcher_dialog_add_drag_data_get), dialog);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (launcher_dialog_add_selection_changed), dialog);
  g_signal_connect (G_OBJECT (object), "button-press-event",
                    G_CALLBACK (launcher_dialog_add_button_press_event), dialog);
  g_signal_connect (G_OBJECT (object), "key-press-event",
                    G_CALLBACK (launcher_dialog_add_key_press_event), dialog);

  /* setup search filter in the add dialog */
  object = gtk_builder_get_object (builder, "add-store-filter");
  item = gtk_builder_get_object (builder, "add-search");
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (object),
                                          launcher_dialog_add_visible_function, item, NULL);
  g_signal_connect_swapped (G_OBJECT (item), "changed",
                            G_CALLBACK (gtk_tree_model_filter_refilter), object);

  /* show the dialog */
  gtk_widget_show (GTK_WIDGET (*window));
}
