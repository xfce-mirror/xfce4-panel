/* $Id$ */
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
#include <common/panel-private.h>

#include "launcher.h"
#include "launcher-dialog.h"
#include "launcher-dialog_glade.h"



typedef struct
{
  LauncherPlugin *plugin;
  GtkBuilder     *builder;
  guint           idle_populate_id;
}
LauncherPluginDialog;

enum
{
  COL_ICON,
  COL_NAME,
  COL_ITEM,
  COL_SEARCH
};



static void launcher_dialog_items_insert_item (GtkListStore *store, GtkTreeIter *iter, XfceMenuItem *item);



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
  if (G_UNLIKELY (!IS_STRING (text)))
    return TRUE;

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  /* try the pre-build search string first */
  gtk_tree_model_get (model, iter, COL_SEARCH, &string, -1);
  if (IS_STRING (string))
    {
      /* search */
      visible = (strstr (string, text_casefolded) != NULL);
    }
  else
    {
      /* get the name */
      gtk_tree_model_get (model, iter, COL_NAME, &string, -1);
      if (IS_STRING (string))
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
launcher_dialog_add_store_insert (gpointer filename,
                                  gpointer item,
                                  gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (user_data);
  GtkTreeIter   iter;
  const gchar  *icon_name, *name, *comment;
  gchar        *markup;

  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));
  panel_return_if_fail (GTK_IS_LIST_STORE (user_data));

  /* TODO get rid of this and support absolute paths too */
  icon_name = xfce_menu_item_get_icon_name (item);
  if (icon_name != NULL
      && (g_path_is_absolute (icon_name)
          || !gtk_icon_theme_has_icon (gtk_icon_theme_get_default (), icon_name)))
    icon_name = NULL;

  /* create a good looking name */
  comment = xfce_menu_item_get_comment (item);
  name = xfce_menu_item_get_name (item);
  if (IS_STRING (comment))
    markup = g_strdup_printf ("<b>%s</b>\n%s", name, comment);
  else
    markup = g_strdup_printf ("<b>%s</b>", name);

  /* insert the item */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COL_ICON, icon_name,
                      COL_NAME, markup,
                      COL_ITEM, item,
                      -1);

  g_free (markup);
}



static gboolean
launcher_dialog_add_populate_model_idle (gpointer user_data)
{
  LauncherPluginDialog *dialog = user_data;
  GObject              *store;
  XfceMenu             *menu;
  GError               *error = NULL;

  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);

  GDK_THREADS_ENTER ();

  /* initialize the menu library */
  xfce_menu_init (NULL);

  /* load our menu file */
  menu = xfce_menu_new (SYSCONFDIR "/xdg/menus/launcher.menu", &error);
  if (G_UNLIKELY (menu != NULL))
    {
      /* start appending items in the store */
      store = gtk_builder_get_object (dialog->builder, "add-store");

      /* get the item pool and insert everything in the store */
      xfce_menu_item_pool_foreach (xfce_menu_get_item_pool (menu),
                                   launcher_dialog_add_store_insert,
                                   store);

      /* release the menu */
      g_object_unref (G_OBJECT (menu));
    }
  else
    {
      /* TODO */
      g_error_free (error);
    }

  /* shutdown menu library */
  xfce_menu_shutdown ();

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



static gboolean
launcher_dialog_tree_save_foreach (GtkTreeModel *model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
  GPtrArray    *array = user_data;
  GValue       *value;
  XfceMenuItem *item;

  /* get the desktop id of the item from the store */
  gtk_tree_model_get (model, iter, COL_ITEM, &item, -1);
  if (G_LIKELY (item != NULL))
    {
      /* create a value with the filename */
      value = g_new0 (GValue, 1);
      g_value_init (value, G_TYPE_STRING);
      g_value_set_static_string (value, xfce_menu_item_get_filename (item));

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
  GObject      *object;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          n_children = -1, position = 0;
  GtkTreePath  *path;
  gboolean      sensitive;

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
  sensitive = !!(position >= 0 && n_children > 0); /* TODO custom items only (??) */
  gtk_widget_set_sensitive (GTK_WIDGET (object), sensitive);

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
  GObject          *object;
  GtkWidget        *window;
  GObject          *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter_a, iter_b;
  GtkTreePath      *path;

  panel_return_if_fail (GTK_IS_BUILDABLE (button));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  /* name of the button */
  name = gtk_buildable_get_name (GTK_BUILDABLE (button));
  if (exo_str_is_equal (name, "item-add"))
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

      if (exo_str_is_equal (name, "item-delete"))
        {
          /* create question dialog */
          window = gtk_message_dialog_new (
              GTK_WINDOW (gtk_widget_get_toplevel (button)),
              GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
              _("Are you sure you want to remove the selected item?"));
          gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (window),
              _("If you delete an item, it is permanently removed from the launcher."));
          gtk_dialog_add_buttons (GTK_DIALOG (window),
                                  GTK_STOCK_REMOVE, GTK_RESPONSE_ACCEPT,
                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                  NULL);
          gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_ACCEPT);

          /* run the dialog */
          if (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_ACCEPT)
            gtk_list_store_remove (GTK_LIST_STORE (model), &iter_a);

          /* destroy */
          gtk_widget_destroy (window);
        }
      else if (exo_str_is_equal (name, "item-edit"))
        {
          object = gtk_builder_get_object (dialog->builder, "dialog-editor");
          gtk_widget_show (GTK_WIDGET (object));
        }
      else if (exo_str_is_equal (name, "item-move-up"))
        {
          path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter_a);
          if (gtk_tree_path_prev (path)
              && gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter_b, path))
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter_a, &iter_b);
          gtk_tree_path_free (path);
        }
      else
        {
          panel_return_if_fail (exo_str_is_equal (name, "item-move-down"));
          iter_b = iter_a;
          if (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter_b))
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter_a, &iter_b);
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

      /* destroy the dialog and release the builder */
      gtk_widget_destroy (widget);
      g_object_unref (G_OBJECT (dialog->builder));

      /* unblock plugin menu */
      xfce_panel_plugin_unblock_menu (XFCE_PANEL_PLUGIN (dialog->plugin));

      /* cleanup */
      g_slice_free (LauncherPluginDialog, dialog);
    }
}



static void
launcher_dialog_editor_response (GtkWidget            *widget,
                                 gint                  response_id,
                                 LauncherPluginDialog *dialog)
{
  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (dialog->plugin));

  /* hide the dialog, since it's owned by gtkbuilder */
  gtk_widget_hide (widget);
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
  XfceMenuItem     *item;
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
              launcher_dialog_items_insert_item (GTK_LIST_STORE (item_model),
                                                 &iter, item);
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



static void
launcher_dialog_items_insert_item (GtkListStore *store,
                                   GtkTreeIter  *iter,
                                   XfceMenuItem *item)
{
  const gchar *name, *comment;
  gchar       *markup;

  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));

  name = xfce_menu_item_get_name (item);
  comment = xfce_menu_item_get_comment (item);

  if (IS_STRING (comment))
    markup = g_strdup_printf ("<b>%s</b>\n%s", name, comment);
  else
    markup = g_strdup_printf ("<b>%s</b>", name);

  gtk_list_store_set (GTK_LIST_STORE (store), iter,
                      COL_ICON, xfce_menu_item_get_icon_name (item),
                      COL_NAME, markup,
                      COL_ITEM, item,
                      -1);

  g_free (markup);
}



static void
launcher_dialog_items_load (LauncherPluginDialog *dialog)
{
  GObject          *treeview;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GSList           *items, *li;
  GtkTreeIter       iter;

  /* get the treeview */
  treeview = gtk_builder_get_object (dialog->builder, "item-treeview");

  /* get the model and clear it */
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
  gtk_list_store_clear (GTK_LIST_STORE (model));

  /* insert the launcher items */
  items = launcher_plugin_get_items (dialog->plugin);
  for (li = items; li != NULL; li = li->next)
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      launcher_dialog_items_insert_item (GTK_LIST_STORE (model),
                                         &iter, XFCE_MENU_ITEM (li->data));
    }

  /* select the first item */
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
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

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, launcher_dialog_glade,
                                   launcher_dialog_glade_length, NULL))
    {
      /* create structure */
      dialog = g_slice_new0 (LauncherPluginDialog);
      dialog->builder = builder;
      dialog->plugin = plugin;

      /* block plugin menu */
      xfce_panel_plugin_block_menu (XFCE_PANEL_PLUGIN (plugin));

      /* get dialog from builder, release builder when dialog is destroyed */
      window = gtk_builder_get_object (builder, "dialog");
      xfce_panel_plugin_take_window (XFCE_PANEL_PLUGIN (plugin), GTK_WINDOW (window));
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

      /* connect binding to the advanced properties */
      for (i = 0; i < G_N_ELEMENTS (binding_names); i++)
        {
          object = gtk_builder_get_object (builder, binding_names[i]);
          panel_return_if_fail (GTK_IS_WIDGET (object));
          exo_mutual_binding_new (G_OBJECT (plugin), binding_names[i],
                                  G_OBJECT (object), "active");
        }

      /* setup responses for the other dialogs */
      object = gtk_builder_get_object (builder, "dialog-editor");
      g_signal_connect (G_OBJECT (object), "response",
          G_CALLBACK (launcher_dialog_editor_response), dialog);
      g_signal_connect (G_OBJECT (object), "delete-event",
          G_CALLBACK (exo_noop_true), NULL);

      object = gtk_builder_get_object (builder, "dialog-add");
      g_signal_connect (G_OBJECT (object), "response",
          G_CALLBACK (launcher_dialog_add_response), dialog);
      g_signal_connect (G_OBJECT (object), "delete-event",
          G_CALLBACK (exo_noop_true), NULL);

      /* enable sorting in the add dialog */
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

      /* show the dialog */
      gtk_widget_show (GTK_WIDGET (window));
    }
  else
    {
      /* release the builder and fire error */
      g_object_unref (G_OBJECT (builder));
      panel_assert_not_reached ();
    }
}
