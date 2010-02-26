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

#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>

#include "launcher.h"
#include "launcher-dialog.h"
#include "launcher-dialog_glade.h"



static void
launcher_dialog_tree_selection_changed (GtkTreeSelection *selection,
                                        GtkBuilder       *builder)
{
  GObject      *object;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          n_children = -1, position = 0;
  GtkTreePath  *path;

  panel_return_if_fail (GTK_IS_TREE_SELECTION (selection));
  panel_return_if_fail (GTK_IS_BUILDER (builder));

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
  object = gtk_builder_get_object (builder, "entry-remove");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > 1);

  object = gtk_builder_get_object (builder, "entry-move-up");
  gtk_widget_set_sensitive (GTK_WIDGET (object), position > 0);

  object = gtk_builder_get_object (builder, "entry-move-down");
  gtk_widget_set_sensitive (GTK_WIDGET (object), n_children > position);
  
  object = gtk_builder_get_object (builder, "entry-edit");
  gtk_widget_set_sensitive (GTK_WIDGET (object), position >= 0);
}



static void
launcher_dialog_entry_button_clicked (GtkWidget  *button,
                                      GtkBuilder *builder)
{
  const gchar *name;
  GObject     *dialog;

  panel_return_if_fail (GTK_IS_BUILDABLE (button));
  panel_return_if_fail (GTK_IS_BUILDER (builder));

  /* name of the button */
  name = gtk_buildable_get_name (GTK_BUILDABLE (button));

  if (exo_str_is_equal (name, "entry-add"))
    {
      dialog = gtk_builder_get_object (builder, "dialog-add");
      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else if (exo_str_is_equal (name, "entry-remove"))
    {

    }
  else if (exo_str_is_equal (name, "entry-move-up"))
    {

    }
  else if (exo_str_is_equal (name, "entry-move-down"))
    {

    }
  else /* entry-edit */
    {
      dialog = gtk_builder_get_object (builder, "dialog-editor");
      gtk_widget_show (GTK_WIDGET (dialog));
    }
}



static void
launcher_dialog_response (GtkWidget      *dialog,
                          gint            response_id,
                          LauncherPlugin *plugin)
{
  panel_return_if_fail (GTK_IS_DIALOG (dialog));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  if (G_UNLIKELY (response_id == 1))
    {
      /* TODO open help */
    }
  else
    {
      /* destroy the dialog */
      gtk_widget_destroy (dialog);
    }
}


static void
launcher_dialog_editor_response (GtkWidget      *dialog,
                                 gint            response_id,
                                 LauncherPlugin *plugin)
{
  panel_return_if_fail (GTK_IS_DIALOG (dialog));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  gtk_widget_hide (dialog);
}



static void
launcher_dialog_add_response (GtkWidget      *dialog,
                              gint            response_id,
                              LauncherPlugin *plugin)
{
  panel_return_if_fail (GTK_IS_DIALOG (dialog));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  gtk_widget_hide (dialog);
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  GtkBuilder       *builder;
  GObject          *dialog;
  GObject          *object;
  guint             i;
  GtkTreeSelection *selection;
  const gchar      *entry_names[] = { "entry-add", "entry-remove", "entry-move-up", "entry-move-down", "entry-edit" };

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, launcher_dialog_glade, launcher_dialog_glade_length, NULL))
    {
      /* get dialog from builder, release builder when dialog is destroyed */
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (XFCE_PANEL_PLUGIN (plugin), GTK_WINDOW (dialog));
      g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (launcher_dialog_response), plugin);

      /* block plugin, release block when the dialog is destroyed */
      xfce_panel_plugin_block_menu (XFCE_PANEL_PLUGIN (plugin));
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) xfce_panel_plugin_unblock_menu, plugin);

      /* connect entry buttons */
      for (i = 0; i < G_N_ELEMENTS (entry_names); i++)
        {
          object = gtk_builder_get_object (builder, entry_names[i]);
          g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (launcher_dialog_entry_button_clicked), builder);
        }

      /* setup treeview selection */
      object = gtk_builder_get_object (builder, "entry-treeview");
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (launcher_dialog_tree_selection_changed), builder);
      launcher_dialog_tree_selection_changed (selection, builder);

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
      g_signal_connect (G_OBJECT (object), "response", G_CALLBACK (launcher_dialog_editor_response), plugin);
      g_signal_connect (G_OBJECT (object), "delete-event", G_CALLBACK (exo_noop_true), NULL);
      
      object = gtk_builder_get_object (builder, "dialog-add");
      g_signal_connect (G_OBJECT (object), "response", G_CALLBACK (launcher_dialog_add_response), plugin);
      g_signal_connect (G_OBJECT (object), "delete-event", G_CALLBACK (exo_noop_true), NULL);

      /* show the dialog */
      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder and fire error */
      g_object_unref (G_OBJECT (builder));
      panel_assert_not_reached ();
    }
}
