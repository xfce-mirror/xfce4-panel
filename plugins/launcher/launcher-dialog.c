/* $Id$ */
/*
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
#include <libxfce4menu/libxfce4menu.h>
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



static gboolean
launcher_dialog_add_visible_function (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      user_data)
{
  gchar       *name;
  gboolean     visible = FALSE;
  const gchar *text;
  gchar       *normalized;
  gchar       *text_casefolded;
  gchar       *name_casefolded;

  /* get the search string from the entry */
  text = gtk_entry_get_text (GTK_ENTRY (user_data));
  if (G_UNLIKELY (!IS_STRING (text)))
    return TRUE;

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  gtk_tree_model_get (model, iter, 0, &name, -1);
  if (G_LIKELY (name != NULL))
    {
      /* casefold the name */
      normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
      name_casefolded = g_utf8_casefold (normalized, -1);
      g_free (normalized);

      /* search */
      visible = (strstr (name_casefolded, text_casefolded) != NULL);

      /* cleanup */
      g_free (name);
      g_free (name_casefolded);
    }

  /* cleanup */
  g_free (text_casefolded);

  return visible;
}



static void
launcher_dialog_add_store_insert (gpointer filename,
                                  gpointer item,
                                  gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (user_data);
  GtkTreeIter   iter;

  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));
  panel_return_if_fail (GTK_IS_LIST_STORE (user_data));

  /* insert the item */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, xfce_menu_item_get_name (item), -1);
}



static gboolean
launcher_dialog_add_populate_model_idle (gpointer user_data)
{
  LauncherPluginDialog *dialog = user_data;
  XfceMenu             *root;
  GError               *error = NULL;
  GObject              *store;
  XfceMenuItemCache    *cache;

  panel_return_val_if_fail (GTK_IS_BUILDER (dialog->builder), FALSE);

  /* initialize the menu library */
  xfce_menu_init (NULL);

  GDK_THREADS_ENTER ();

  /* get the root menu */
  root = xfce_menu_get_root (&error);
  if (G_LIKELY (root != NULL))
    {
      /* start appending items in the store */
      store = gtk_builder_get_object (dialog->builder, "add-store");

      /* get the item cache and insert everything in the store */
      cache = xfce_menu_item_cache_get_default ();
      xfce_menu_item_cache_foreach (cache, launcher_dialog_add_store_insert, store);

      /* release the root menu and cache */
      g_object_unref (G_OBJECT (root));
      g_object_unref (G_OBJECT (cache));
    }
  else
    {
      /* TODO make this a warning dialog or something */
      g_message ("Failed to load the root menu: %s", error->message);
      g_error_free (error);
    }

  GDK_THREADS_LEAVE ();

  /* shutdown menu library */
  xfce_menu_shutdown ();

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
    dialog->idle_populate_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, launcher_dialog_add_populate_model_idle,
                                                dialog, launcher_dialog_add_populate_model_idle_destroyed);
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
  object = gtk_builder_get_object (dialog->builder, "entry-remove");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > 1);

  object = gtk_builder_get_object (dialog->builder, "entry-move-up");
  gtk_widget_set_sensitive (GTK_WIDGET (object), position > 0);

  object = gtk_builder_get_object (dialog->builder, "entry-move-down");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > position);

  object = gtk_builder_get_object (dialog->builder, "entry-edit");
  gtk_widget_set_sensitive (GTK_WIDGET (object), position >= 0 /* TODO custom only */);
}



static void
launcher_dialog_entry_button_clicked (GtkWidget            *button,
                                      LauncherPluginDialog *dialog)
{
  const gchar *name;
  GObject     *object;
  GtkWidget   *window;

  panel_return_if_fail (GTK_IS_BUILDABLE (button));
  panel_return_if_fail (GTK_IS_BUILDER (dialog->builder));

  /* name of the button */
  name = gtk_buildable_get_name (GTK_BUILDABLE (button));

  if (exo_str_is_equal (name, "entry-add"))
    {
      object = gtk_builder_get_object (dialog->builder, "dialog-add");
      launcher_dialog_add_populate_model (dialog);
      gtk_widget_show (GTK_WIDGET (object));
    }
  else if (exo_str_is_equal (name, "entry-remove"))
    {
      /* create question dialog */
      window = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (button)),
                                       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                       _("Are you sure you want to remove \"%s\"?"), "TODO");
      gtk_dialog_add_buttons (GTK_DIALOG (window), GTK_STOCK_REMOVE, GTK_RESPONSE_ACCEPT,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_ACCEPT);

      /* run the dialog */
      if (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_ACCEPT)
        {

        }

      /* destroy */
      gtk_widget_destroy (window);
    }
  else if (exo_str_is_equal (name, "entry-move-up"))
    {

    }
  else if (exo_str_is_equal (name, "entry-move-down"))
    {

    }
  else /* entry-edit */
    {
      object = gtk_builder_get_object (dialog->builder, "dialog-editor");
      gtk_widget_show (GTK_WIDGET (object));
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
  panel_return_if_fail (GTK_IS_DIALOG (widget));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (dialog->plugin));

  /* hide the dialog, since it's owned by gtkbuilder */
  gtk_widget_hide (widget);
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  GtkBuilder           *builder;
  GObject              *window, *object, *entry;
  guint                 i;
  GtkTreeSelection     *selection;
  const gchar          *entry_names[] = { "entry-add", "entry-remove", "entry-move-up", "entry-move-down", "entry-edit" };
  LauncherPluginDialog *dialog;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, launcher_dialog_glade, launcher_dialog_glade_length, NULL))
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
      g_signal_connect (G_OBJECT (window), "response", G_CALLBACK (launcher_dialog_response), dialog);

      /* connect entry buttons */
      for (i = 0; i < G_N_ELEMENTS (entry_names); i++)
        {
          object = gtk_builder_get_object (builder, entry_names[i]);
          g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (launcher_dialog_entry_button_clicked), dialog);
        }

      /* setup treeview selection */
      object = gtk_builder_get_object (builder, "entry-treeview");
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (launcher_dialog_tree_selection_changed), dialog);
      launcher_dialog_tree_selection_changed (selection, dialog);

      /* connect binding to the advanced properties */
      object = gtk_builder_get_object (builder, "disable-tooltips");
      xfconf_g_property_bind (plugin->channel, "/disable-tooltips", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "show-labels");
      xfconf_g_property_bind (plugin->channel, "/show-labels", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "move-first");
      xfconf_g_property_bind (plugin->channel, "/move-first", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "arrow-position");
      xfconf_g_property_bind (plugin->channel, "/arrow-position", G_TYPE_UINT, object, "active");

      /* setup responses for the other dialogs */
      object = gtk_builder_get_object (builder, "dialog-editor");
      g_signal_connect (G_OBJECT (object), "response", G_CALLBACK (launcher_dialog_editor_response), dialog);
      g_signal_connect (G_OBJECT (object), "delete-event", G_CALLBACK (exo_noop_true), NULL);

      object = gtk_builder_get_object (builder, "dialog-add");
      g_signal_connect (G_OBJECT (object), "response", G_CALLBACK (launcher_dialog_add_response), dialog);
      g_signal_connect (G_OBJECT (object), "delete-event", G_CALLBACK (exo_noop_true), NULL);

      object = gtk_builder_get_object (builder, "add-store");
      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (object), 0, GTK_SORT_ASCENDING);

      /* setup search filter in the add dialog */
      object = gtk_builder_get_object (builder, "add-store-filter");
      entry = gtk_builder_get_object (builder, "add-search");
      gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (object), launcher_dialog_add_visible_function, entry, NULL);
      g_signal_connect_swapped (G_OBJECT (entry), "changed", G_CALLBACK (gtk_tree_model_filter_refilter), object);

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
