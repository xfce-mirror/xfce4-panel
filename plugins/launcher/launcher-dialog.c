/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <garcon/garcon.h>
#include <xfconf/xfconf.h>
#include <gio/gio.h>

#include <common/panel-private.h>
#include <common/panel-builder.h>

#include "launcher.h"
#include "launcher-dialog.h"
#include "launcher-dialog_ui.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#define LAUNCHER_WIDGET_XID(widget) ((gint) GDK_WINDOW_XID (GDK_WINDOW ((widget)->window)))
#else
#define LAUNCHER_WIDGET_XID(widget) (0)
#endif



typedef struct
{
  LauncherPlugin *plugin;
  GtkBuilder     *builder;
  guint           idle_populate_id;
  GSList         *items;
}
LauncherPluginDialog;

enum
{
  COL_ICON,
  COL_NAME,
  COL_ITEM,
  COL_SEARCH,
  COL_TOOLTIP
};



static void launcher_dialog_items_set_item (GtkTreeModel *model, GtkTreeIter *iter, GarconMenuItem *item);
static void launcher_dialog_tree_save (LauncherPluginDialog *dialog);
static void launcher_dialog_tree_selection_changed (GtkTreeSelection *selection, LauncherPluginDialog *dialog);
static void launcher_dialog_items_unload (LauncherPluginDialog *dialog);
static void launcher_dialog_items_load (LauncherPluginDialog *dialog);



static gboolean
launcher_dialog_add_visible_function (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
  gchar       *string, *escaped;
  gboolean     visible = FALSE;
  const gchar *text;
  gchar       *normalized;
  gchar       *text_casefolded;
  gchar       *name_casefolded;

  /* get the search string from the item */
  text = gtk_entry_get_text (GTK_ENTRY (user_data));
  if (G_UNLIKELY (exo_str_is_empty (text)))
    return TRUE;

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  /* try the pre-build search string first */
  gtk_tree_model_get (model, iter, COL_SEARCH, &string, -1);
  if (!exo_str_is_empty (string))
    {
      /* search */
      visible = (strstr (string, text_casefolded) != NULL);
    }
  else
    {
      /* get the name */
      gtk_tree_model_get (model, iter, COL_NAME, &string, -1);
      if (!exo_str_is_empty (string))
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

          /* cleanup */
          g_free (name_casefolded);
        }
    }

  /* cleanup */
  g_free (text_casefolded);
  g_free (string);

  return visible;
}



static void
launcher_dialog_add_store_insert (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
  GtkTreeIter     iter;
  gchar          *tooltip;
  GFile          *gfile;
  GarconMenuItem *item = GARCON_MENU_ITEM (value);
  GtkTreeModel   *model = GTK_TREE_MODEL (user_data);

  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));
  panel_return_if_fail (GTK_IS_LIST_STORE (model));

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  launcher_dialog_items_set_item (model, &iter, item);

  gfile = garcon_menu_item_get_file (item);
  tooltip = g_file_get_parse_name (gfile);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      COL_TOOLTIP, tooltip, -1);
  g_object_unref (G_OBJECT (gfile));
  g_free (tooltip);
}



static gboolean
launcher_dialog_add_populate_model_idle (gpointer user_data)
{
  LauncherPluginDialog *dialog = user_data;
  GObject              *store;
  GHashTable           *pool;

  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);

  GDK_THREADS_ENTER ();

  /* load the item pool */
  pool = launcher_plugin_garcon_menu_pool ();

  /* insert the items in the store */
  store = gtk_builder_get_object (dialog->builder, "add-store");
  g_hash_table_foreach (pool, launcher_dialog_add_store_insert, store);

  g_hash_table_destroy (pool);

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
launcher_dialog_add_populate_model_idle_destroyed (gpointer user_data)
{
  ((LauncherPluginDialog *) user_data)->idle_populate_id = 0;
}



static void
launcher_dialog_add_populate_model (LauncherPluginDialog *dialog)
{
  GObject *store;

  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  /* get the store and make sure it's empty */
  store = gtk_builder_get_object (dialog->builder, "add-store");
  gtk_list_store_clear (GTK_LIST_STORE (store));

  /* fire an idle menu system load */
  if (G_LIKELY (dialog->idle_populate_id == 0))
    dialog->idle_populate_id = g_idle_add_full (
        G_PRIORITY_DEFAULT_IDLE,
        launcher_dialog_add_populate_model_idle,
        dialog, launcher_dialog_add_populate_model_idle_destroyed);
}



static void
launcher_dialog_add_selection_changed (GtkTreeSelection     *selection,
                                       LauncherPluginDialog *dialog)
{
  GObject  *object;
  gboolean  sensitive;

  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));
  panel_return_if_fail (GTK_IS_TREE_SELECTION (selection));

  object = gtk_builder_get_object (dialog->builder, "button-add");
  sensitive = !!(gtk_tree_selection_count_selected_rows (selection) > 0);
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);
}



static void
launcher_dialog_add_response (GtkWidget            *widget,
                              gint                  response_id,
                              LauncherPluginDialog *dialog)
{
  GObject          *treeview, *store;
  GtkTreeSelection *selection;
  GtkTreeModel     *item_model, *add_model;
  GtkTreeIter       iter, sibling, tmp;
  GarconMenuItem   *item;
  GList            *list, *li;

  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (dialog->plugin));

  if (response_id != 0)
    {
      /* add all the selected rows in the add dialog */
      treeview = gtk_builder_get_object (dialog->builder, "add-treeview");
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      list = gtk_tree_selection_get_selected_rows (selection, &add_model);

      /* append after the selected item in the item dialog */
      treeview = gtk_builder_get_object (dialog->builder, "item-treeview");
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      item_model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
      if (gtk_tree_selection_get_selected (selection, NULL, &sibling))
        gtk_list_store_insert_after (GTK_LIST_STORE (item_model),
                                     &iter, &sibling);
      else
        gtk_list_store_append (GTK_LIST_STORE (item_model), &iter);

      for (li = list; li != NULL; li = g_list_next (li))
        {
          /* get the selected file in the add dialog */
          gtk_tree_model_get_iter (add_model, &tmp, li->data);
          gtk_tree_model_get (add_model, &tmp, COL_ITEM, &item, -1);

          /* insert the item in the item store */
          if (G_LIKELY (item != NULL))
            {
              launcher_dialog_items_set_item (item_model, &iter, item);
              g_object_unref (G_OBJECT (item));

              /* select the first item */
              if (li == list)
                gtk_tree_selection_select_iter (selection, &iter);
            }

          /* cleanup */
          gtk_tree_path_free (li->data);

          if (g_list_next (li) != NULL)
            {
              /* insert a new iter after the new added item */
              sibling = iter;
              gtk_list_store_insert_after (GTK_LIST_STORE (item_model),
                                           &iter, &sibling);
            }
        }

      /* cleanup */
      g_list_free (list);

      /* write the model to xfconf */
      launcher_dialog_tree_save (dialog);

      /* update the selection */
      launcher_dialog_tree_selection_changed (selection, dialog);
    }

  /* empty the store */
  store = gtk_builder_get_object (dialog->builder, "add-store");
  gtk_list_store_clear (GTK_LIST_STORE (store));

  /* hide the dialog, since it's owned by gtkbuilder */
  gtk_widget_hide (widget);
}



static gboolean
launcher_dialog_tree_save_foreach (GtkTreeModel *model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
  GPtrArray      *array = user_data;
  GValue         *value;
  GarconMenuItem *item;

  /* get the desktop id of the item from the store */
  gtk_tree_model_get (model, iter, COL_ITEM, &item, -1);
  if (G_LIKELY (item != NULL))
    {
      /* create a value with the filename */
      value = g_new0 (GValue, 1);
      g_value_init (value, G_TYPE_STRING);
      g_value_take_string (value, garcon_menu_item_get_uri (item));

      /* put it in the array and release */
      g_ptr_array_add (array, value);
      g_object_unref (G_OBJECT (item));
    }

  return FALSE;
}



static void
launcher_dialog_tree_save (LauncherPluginDialog *dialog)
{
  GObject   *store;
  GPtrArray *array;

  store = gtk_builder_get_object (dialog->builder, "item-store");

  array = g_ptr_array_new ();
  gtk_tree_model_foreach (GTK_TREE_MODEL (store),
                          launcher_dialog_tree_save_foreach, array);

  g_object_set (dialog->plugin, "items", array, NULL);
  xfconf_array_free (array);
}



static void
launcher_dialog_tree_selection_changed (GtkTreeSelection     *selection,
                                        LauncherPluginDialog *dialog)
{
  GObject        *object;
  GtkTreeModel   *model;
  GtkTreeIter     iter;
  gint            n_children = -1;
  gint            position = 0;
  GtkTreePath    *path;
  gboolean        sensitive;
  GarconMenuItem *item = NULL;
  gboolean        editable = FALSE;

  panel_return_if_fail (GTK_IS_TREE_SELECTION (selection));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      /* get the number of launchers in the tree */
      n_children = gtk_tree_model_iter_n_children (model, NULL);

      /* get the position of the selected item in the tree */
      path = gtk_tree_model_get_path (model, &iter);
      position = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);

      gtk_tree_model_get (model, &iter, COL_ITEM, &item, -1);
      if (G_LIKELY (item != NULL))
        {
          editable = launcher_plugin_item_is_editable (dialog->plugin, item, NULL);
          g_object_unref (G_OBJECT (item));
        }
    }

  /* update the sensitivity of the buttons */
  object = gtk_builder_get_object (dialog->builder, "item-delete");
  gtk_widget_set_sensitive (GTK_WIDGET (object), !!(n_children > 0));

  object = gtk_builder_get_object (dialog->builder, "item-move-up");
  sensitive = !!(position > 0 && position <= n_children);
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);

  object = gtk_builder_get_object (dialog->builder, "item-move-down");
  sensitive = !!(position >= 0 && position < n_children - 1);
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);

  object = gtk_builder_get_object (dialog->builder, "item-edit");
  gtk_widget_set_sensitive (GTK_WIDGET (object), editable);

  object = gtk_builder_get_object (dialog->builder, "arrow-position");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > 1);

  object = gtk_builder_get_object (dialog->builder, "move-first");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > 1);

  object = gtk_builder_get_object (dialog->builder, "arrow-position-label");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > 1);
}



static void
launcher_dialog_item_button_clicked (GtkWidget            *button,
                                     LauncherPluginDialog *dialog)
{
  const gchar      *name;
  const gchar      *display_name = NULL;
  GObject          *object;
  GObject          *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter_a, iter_b;
  GtkTreePath      *path;
  gchar            *command, *uri;
  GdkScreen        *screen;
  GError           *error = NULL;
  GarconMenuItem   *item;
  GtkWidget        *toplevel;
  gchar            *filename;
  gboolean          can_delete;
  GFile            *item_file;

  panel_return_if_fail (GTK_IS_BUILDABLE (button));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  /* name of the button */
  name = gtk_buildable_get_name (GTK_BUILDABLE (button));
  if (G_UNLIKELY (name == NULL))
    return;

  if (strcmp (name, "item-add") == 0)
    {
      object = gtk_builder_get_object (dialog->builder, "dialog-add");
      launcher_dialog_add_populate_model (dialog);
      gtk_widget_show (GTK_WIDGET (object));
    }
  else
    {
      /* get the selected item in the tree, leave if none is found */
      treeview = gtk_builder_get_object (dialog->builder, "item-treeview");
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      if (!gtk_tree_selection_get_selected (selection, &model, &iter_a))
        return;

      if (strcmp (name, "item-delete") == 0)
        {
          /* get item name */
          gtk_tree_model_get (model, &iter_a, COL_ITEM, &item, -1);
          if (G_LIKELY (item != NULL))
            display_name = garcon_menu_item_get_name (item);

          /* ask the user */
          toplevel = gtk_widget_get_toplevel (button);
          if (xfce_dialog_confirm (GTK_WINDOW (toplevel), GTK_STOCK_DELETE, NULL,
                  _("If you delete an item, it will be permanently removed"),
                  _("Are you sure you want to remove \"%s\"?"),
                  exo_str_is_empty (display_name) ? _("Unnamed item") : display_name))
            {
              /* remove the item from the store */
              gtk_list_store_remove (GTK_LIST_STORE (model), &iter_a);

              /* delete the desktop file if possible */
              if (item != NULL
                  && launcher_plugin_item_is_editable (dialog->plugin, item, &can_delete)
                  && can_delete)
                {
                  item_file = garcon_menu_item_get_file (item);
                  if (!g_file_delete (item_file, NULL, &error))
                    {
                      toplevel = gtk_widget_get_toplevel (button);
                      xfce_dialog_show_error (GTK_WINDOW (toplevel), error,
                          _("Failed to remove the desktop file from the config directory"));
                      g_error_free (error);
                    }
                  g_object_unref (G_OBJECT (item_file));
                }
            }

          if (G_LIKELY (item != NULL))
            g_object_unref (G_OBJECT (item));
        }
      else if (strcmp (name, "item-new") == 0
               || strcmp (name, "item-edit") == 0)
        {
          if (strcmp (name, "item-edit") == 0)
            {
              gtk_tree_model_get (model, &iter_a, COL_ITEM, &item, -1);
              if (G_UNLIKELY (item == NULL))
                return;

              /* build command */
              uri = garcon_menu_item_get_uri (item);
              command = g_strdup_printf ("exo-desktop-item-edit --xid=%d '%s'",
                                         LAUNCHER_WIDGET_XID (button), uri);
              g_free (uri);
            }
          else
            {
              /* build command */
              filename = launcher_plugin_unique_filename (dialog->plugin);
              command = g_strdup_printf ("exo-desktop-item-edit -c --xid=%d '%s'",
                                         LAUNCHER_WIDGET_XID (button), filename);
              g_free (filename);
            }

          /* spawn item editor */
          screen = gtk_widget_get_screen (button);
          if (!gdk_spawn_command_line_on_screen (screen, command, &error))
            {
              toplevel = gtk_widget_get_toplevel (button);
              xfce_dialog_show_error (GTK_WINDOW (toplevel), error,
                  _("Failed to open desktop item editor"));
              g_error_free (error);
            }

          g_free (command);
        }
      else if (strcmp (name, "item-move-up") == 0)
        {
          path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter_a);
          if (gtk_tree_path_prev (path)
              && gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter_b, path))
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter_a, &iter_b);
          gtk_tree_path_free (path);
        }
      else if (strcmp (name, "item-move-down") == 0)
        {
          iter_b = iter_a;
          if (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter_b))
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter_a, &iter_b);
        }
      else
        {
          panel_assert_not_reached ();
        }

      /* store the new settings */
      launcher_dialog_tree_save (dialog);

      /* emit a changed signal to update the button states */
      launcher_dialog_tree_selection_changed (selection, dialog);
    }
}



static void
launcher_dialog_response (GtkWidget            *widget,
                          gint                  response_id,
                          LauncherPluginDialog *dialog)
{
  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (dialog->plugin));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  if (G_UNLIKELY (response_id == 1))
    {
      /* TODO open help */
    }
  else
    {
      /* stop idle if still running */
      if (G_UNLIKELY (dialog->idle_populate_id != 0))
        g_source_remove (dialog->idle_populate_id);

      /* disconnect from items-changed signal */
      g_signal_handlers_disconnect_by_func (G_OBJECT (dialog->plugin),
          G_CALLBACK (launcher_dialog_items_load), dialog);

      /* disconnect from the menu items */
      launcher_dialog_items_unload (dialog);

      /* destroy the dialog */
      gtk_widget_destroy (widget);

      /* cleanup */
      g_slice_free (LauncherPluginDialog, dialog);
    }
}



static gboolean
launcher_dialog_item_changed_foreach (GtkTreeModel *model,
                                      GtkTreePath  *path,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
  GarconMenuItem *item;
  gboolean        found;

  panel_return_val_if_fail (GARCON_IS_MENU_ITEM (user_data), TRUE);

  /* check if this is the item in the model */
  gtk_tree_model_get (model, iter, COL_ITEM, &item, -1);
  found = item == user_data;

  if (G_UNLIKELY (found))
    launcher_dialog_items_set_item (model, iter, item);

  g_object_unref (G_OBJECT (item));

  return found;
}



static void
launcher_dialog_item_changed (GarconMenuItem       *item,
                              LauncherPluginDialog *dialog)
{
  GObject      *treeview;
  GtkTreeModel *model;

  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));

  treeview = gtk_builder_get_object (dialog->builder, "item-treeview");
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));

  /* find the item in the model and update it */
  gtk_tree_model_foreach (model, launcher_dialog_item_changed_foreach, item);
}



static void
launcher_dialog_items_set_item (GtkTreeModel   *model,
                                GtkTreeIter    *iter,
                                GarconMenuItem *item)
{
  const gchar *name, *comment;
  gchar       *markup;

  panel_return_if_fail (GTK_IS_LIST_STORE (model));
  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));

  name = garcon_menu_item_get_name (item);
  comment = garcon_menu_item_get_comment (item);

  if (!exo_str_is_empty (comment))
    markup = g_markup_printf_escaped ("<b>%s</b>\n%s", name, comment);
  else
    markup = g_markup_printf_escaped ("<b>%s</b>", name);

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
                      COL_ICON, garcon_menu_item_get_icon_name (item),
                      COL_NAME, markup,
                      COL_ITEM, item,
                      -1);

  g_free (markup);
}



static void
launcher_dialog_items_unload (LauncherPluginDialog *dialog)
{
  GSList *li;

  for (li = dialog->items; li != NULL; li = li->next)
    {
      panel_return_if_fail (GARCON_IS_MENU_ITEM (li->data));
      g_signal_handlers_disconnect_by_func (G_OBJECT (li->data),
          G_CALLBACK (launcher_dialog_item_changed), dialog);
      g_object_unref (G_OBJECT (li->data));
    }

  g_slist_free (dialog->items);
  dialog->items = NULL;
}



static void
launcher_dialog_items_load (LauncherPluginDialog *dialog)
{
  GObject          *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GSList           *li;
  GtkTreeIter       iter;
  GtkTreePath      *path;

  /* get the treeview */
  treeview = gtk_builder_get_object (dialog->builder, "item-treeview");
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  /* we try to preserve the current selection */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    path = gtk_tree_model_get_path (model, &iter);
  else
    path = NULL;

  /* clear the old items */
  launcher_dialog_items_unload (dialog);
  gtk_list_store_clear (GTK_LIST_STORE (model));

  /* insert the launcher items */
  dialog->items = launcher_plugin_get_items (dialog->plugin);
  for (li = dialog->items; li != NULL; li = li->next)
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      launcher_dialog_items_set_item (model, &iter, GARCON_MENU_ITEM (li->data));
      g_signal_connect (G_OBJECT (li->data), "changed",
          G_CALLBACK (launcher_dialog_item_changed), dialog);
    }

  if (G_LIKELY (path != NULL))
    {
      /* restore the old selection */
      gtk_tree_selection_select_path (selection, path);
      gtk_tree_path_free (path);
    }
  else if (gtk_tree_model_get_iter_first (model, &iter))
    {
      /* select the first item */
      gtk_tree_selection_select_iter (selection, &iter);
    }
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  LauncherPluginDialog *dialog;
  GtkBuilder           *builder;
  GObject              *window, *object, *item;
  guint                 i;
  GtkTreeSelection     *selection;
  const gchar          *button_names[] = { "item-add", "item-delete",
                                           "item-move-up", "item-move-down",
                                           "item-edit", "item-new" };
  const gchar          *binding_names[] = { "disable-tooltips", "show-label",
                                            "move-first", "arrow-position" };

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* setup the dialog */
  PANEL_BUILDER_LINK_4UI
  builder = panel_builder_new (XFCE_PANEL_PLUGIN (plugin), launcher_dialog_ui,
                               launcher_dialog_ui_length, &window);
  if (G_UNLIKELY (builder == NULL))
    return;

  /* create structure */
  dialog = g_slice_new0 (LauncherPluginDialog);
  dialog->builder = builder;
  dialog->plugin = plugin;
  dialog->items = NULL;

  g_signal_connect (G_OBJECT (window), "response",
      G_CALLBACK (launcher_dialog_response), dialog);

  /* connect item buttons */
  for (i = 0; i < G_N_ELEMENTS (button_names); i++)
    {
      object = gtk_builder_get_object (builder, button_names[i]);
      panel_return_if_fail (GTK_IS_WIDGET (object));
      g_signal_connect (G_OBJECT (object), "clicked",
          G_CALLBACK (launcher_dialog_item_button_clicked), dialog);
    }

  /* setup treeview selection */
  object = gtk_builder_get_object (builder, "item-treeview");
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (G_OBJECT (selection), "changed",
      G_CALLBACK (launcher_dialog_tree_selection_changed), dialog);
  launcher_dialog_tree_selection_changed (selection, dialog);

  /* connect bindings to the advanced properties */
  for (i = 0; i < G_N_ELEMENTS (binding_names); i++)
    {
      object = gtk_builder_get_object (builder, binding_names[i]);
      panel_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (plugin), binding_names[i],
                              G_OBJECT (object), "active");
    }

  /* setup responses for the add dialog */
  object = gtk_builder_get_object (builder, "dialog-add");
  g_signal_connect (G_OBJECT (object), "response",
      G_CALLBACK (launcher_dialog_add_response), dialog);
  g_signal_connect (G_OBJECT (object), "delete-event",
      G_CALLBACK (gtk_true), NULL);

  /* setup sorting in the add dialog */
  object = gtk_builder_get_object (builder, "add-store");
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (object),
                                        COL_NAME, GTK_SORT_ASCENDING);

  /* allow selecting multiple items in the add dialog */
  object = gtk_builder_get_object (builder, "add-treeview");
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (selection), "changed",
      G_CALLBACK (launcher_dialog_add_selection_changed), dialog);

  /* setup search filter in the add dialog */
  object = gtk_builder_get_object (builder, "add-store-filter");
  item = gtk_builder_get_object (builder, "add-search");
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (object),
      launcher_dialog_add_visible_function, item, NULL);
  g_signal_connect_swapped (G_OBJECT (item), "changed",
      G_CALLBACK (gtk_tree_model_filter_refilter), object);

  /* setup the icon size in the icon renderers */
  object = gtk_builder_get_object (builder, "addrenderericon");
  g_object_set (G_OBJECT (object), "stock-size", GTK_ICON_SIZE_DND, NULL);
  object = gtk_builder_get_object (builder, "itemrenderericon");
  g_object_set (G_OBJECT (object), "stock-size", GTK_ICON_SIZE_DND, NULL);

  /* load the plugin items */
  launcher_dialog_items_load (dialog);
  g_signal_connect_swapped (G_OBJECT (plugin), "items-changed",
      G_CALLBACK (launcher_dialog_items_load), dialog);

  /* show the dialog */
  gtk_widget_show (GTK_WIDGET (window));
}
