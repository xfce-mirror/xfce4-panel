/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <common/panel-private.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-window.h>
#include <panel/panel-application.h>
#include <panel/panel-module.h>
#include <panel/panel-itembar.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-preferences-dialog-ui.h>

#define PREFERENCES_HELP_URL "http://www.xfce.org"



static void panel_preferences_dialog_finalize (GObject *object);
static void panel_preferences_dialog_response (GtkWidget *window, gint response_id, PanelPreferencesDialog *dialog);


static void panel_preferences_dialog_bindings_unbind (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_bindings_add (PanelPreferencesDialog *dialog, const gchar *property1, const gchar *property2);
static void panel_preferences_dialog_bindings_update (PanelPreferencesDialog *dialog);

static void panel_preferences_dialog_output_changed (GtkComboBox *combobox, PanelPreferencesDialog *dialog);

static void panel_preferences_dialog_panel_combobox_changed (GtkComboBox *combobox, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_panel_combobox_rebuild (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_panel_add (GtkWidget *widget, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_panel_remove (GtkWidget *widget, PanelPreferencesDialog *dialog);


static XfcePanelPluginProvider *panel_preferences_dialog_item_get_selected (PanelPreferencesDialog *dialog, GtkTreeIter *return_iter);
static void panel_preferences_dialog_item_store_rebuild (GtkWidget *itembar, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_move (GtkWidget *button, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_remove (GtkWidget *button, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_add (GtkWidget *button, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_properties (GtkWidget *button, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_about (GtkWidget *button, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_item_selection_changed (GtkTreeSelection *selection, PanelPreferencesDialog *dialog);



enum
{
  ITEM_COLUMN_ICON_NAME,
  ITEM_COLUMN_DISPLAY_NAME,
  ITEM_COLUMN_TOOLTIP,
  ITEM_COLUMN_PROVIDER,
  N_ITEM_COLUMNS
};

enum
{
  OUTPUT_NAME,
  OUTPUT_TITLE
};

struct _PanelPreferencesDialogClass
{
  GtkBuilderClass __parent__;
};

struct _PanelPreferencesDialog
{
  GtkBuilder  __parent__;

  PanelApplication *application;

  /* currently selected window in the selector */
  PanelWindow      *active;

  /* ExoMutualBinding's between dialog <-> window */
  GSList           *bindings;

  /* store for the items list */
  GtkListStore     *store;

  /* changed signal for the active panel's itembar */
  gulong            items_changed_handler_id;

  /* changed signal for the output selector */
  gulong            output_changed_handler_id;
};



G_DEFINE_TYPE (PanelPreferencesDialog, panel_preferences_dialog, GTK_TYPE_BUILDER)



static PanelPreferencesDialog *dialog_singleton = NULL;



static void
panel_preferences_dialog_class_init (PanelPreferencesDialogClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_preferences_dialog_finalize;
}



static void
panel_preferences_dialog_init (PanelPreferencesDialog *dialog)
{
  GObject           *window;
  GObject           *object;
  GObject           *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  GtkTreeSelection  *selection;

  dialog->bindings = NULL;
  dialog->application = panel_application_get ();

  /* block all autohides */
  panel_application_windows_autohide (dialog->application, TRUE);

  /* load the builder data into the object */
  gtk_builder_add_from_string (GTK_BUILDER (dialog), panel_preferences_dialog_ui,
                               panel_preferences_dialog_ui_length, NULL);

  /* get the dialog */
  window = gtk_builder_get_object (GTK_BUILDER (dialog), "dialog");
  panel_return_if_fail (GTK_IS_WIDGET (window));
  panel_application_take_dialog (dialog->application, GTK_WINDOW (window));
  g_signal_connect (G_OBJECT (window), "response",
      G_CALLBACK (panel_preferences_dialog_response), dialog);

#define connect_signal(name,detail_signal,c_handler) \
  object = gtk_builder_get_object (GTK_BUILDER (dialog), name); \
  panel_return_if_fail (G_IS_OBJECT (object)); \
  g_signal_connect (G_OBJECT (object), detail_signal, G_CALLBACK (c_handler), dialog);

  /* panel selector buttons and combobox */
  connect_signal ("panel-add", "clicked", panel_preferences_dialog_panel_add);
  connect_signal ("panel-remove", "clicked", panel_preferences_dialog_panel_remove);
  connect_signal ("panel-combobox", "changed", panel_preferences_dialog_panel_combobox_changed);

  /* items treeview and buttons */
  connect_signal ("item-up", "clicked", panel_preferences_dialog_item_move);
  connect_signal ("item-down", "clicked", panel_preferences_dialog_item_move);
  connect_signal ("item-remove", "clicked", panel_preferences_dialog_item_remove);
  connect_signal ("item-add", "clicked", panel_preferences_dialog_item_add);
  connect_signal ("item-properties", "clicked", panel_preferences_dialog_item_properties);
  connect_signal ("item-about", "clicked", panel_preferences_dialog_item_about);

  /* create store for panel items */
  dialog->store = gtk_list_store_new (N_ITEM_COLUMNS,
                                      G_TYPE_STRING, /* ITEM_COLUMN_ICON_NAME */
                                      G_TYPE_STRING, /* ITEM_COLUMN_DISPLAY_NAME */
                                      G_TYPE_STRING, /* ITEM_COLUMN_TOOLTIP */
                                      G_TYPE_OBJECT); /* ITEM_COLUMN_PROVIDER */

  /* build tree for panel items */
  treeview = gtk_builder_get_object (GTK_BUILDER (dialog), "item-treeview");
  panel_return_if_fail (GTK_IS_WIDGET (treeview));
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (dialog->store));
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (treeview), ITEM_COLUMN_TOOLTIP);

  /* setup tree selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (G_OBJECT (selection), "changed",
      G_CALLBACK (panel_preferences_dialog_item_selection_changed), dialog);

  /* icon renderer */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer, "icon-name",
                                                     ITEM_COLUMN_ICON_NAME, NULL);
  g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_BUTTON, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* text renderer */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer, "text",
                                       ITEM_COLUMN_DISPLAY_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* connect the output changed signal */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "output-name");
  panel_return_if_fail (GTK_IS_COMBO_BOX (object));
  dialog->output_changed_handler_id =
      g_signal_connect (G_OBJECT (object), "changed",
                        G_CALLBACK (panel_preferences_dialog_output_changed),
                        dialog);

  /* rebuild the panel combobox */
  panel_preferences_dialog_panel_combobox_rebuild (dialog);

  /* show the dialog */
  gtk_widget_show (GTK_WIDGET (window));
}



static void
panel_preferences_dialog_finalize (GObject *object)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (object);
  GtkWidget              *itembar;

  /* disconnect changed signal */
  if (dialog->active != NULL && dialog->items_changed_handler_id != 0)
    {
      itembar = gtk_bin_get_child (GTK_BIN (dialog->active));
      g_signal_handler_disconnect (G_OBJECT (itembar),
          dialog->items_changed_handler_id);
    }

  /* thaw all autohide blocks */
  panel_application_windows_autohide (dialog->application, FALSE);

  /* deselect all windows */
  if (!panel_item_dialog_visible ())
    panel_application_window_select (dialog->application, NULL);

  g_object_unref (G_OBJECT (dialog->application));
  g_object_unref (G_OBJECT (dialog->store));

  (*G_OBJECT_CLASS (panel_preferences_dialog_parent_class)->finalize) (object);
}



static void
panel_preferences_dialog_response (GtkWidget              *window,
                                   gint                    response_id,
                                   PanelPreferencesDialog *dialog)
{
  GError    *error = NULL;
  GdkScreen *screen;

  panel_return_if_fail (GTK_IS_DIALOG (window));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  if (G_UNLIKELY (response_id == 1))
    {
      /* get the dialog screen */
      screen = gtk_widget_get_screen (window);

      /* open the help url */
      if (!gtk_show_uri (screen, PREFERENCES_HELP_URL,
                         gtk_get_current_event_time (), &error))
        {
          xfce_dialog_show_error (GTK_WINDOW (window), error,
                                  _("Failed to open manual"));
          g_error_free (error);
        }
    }
  else
    {
      gtk_widget_destroy (window);
      g_object_unref (G_OBJECT (dialog));
    }
}



static void
panel_preferences_dialog_bindings_unbind (PanelPreferencesDialog *dialog)
{
  GSList *li;

  if (dialog->bindings)
    {
      /* remove all bindings */
      for (li = dialog->bindings; li != NULL; li = li->next)
        exo_mutual_binding_unbind (li->data);

      g_slist_free (dialog->bindings);
      dialog->bindings = NULL;
    }
}



static void
panel_preferences_dialog_bindings_add (PanelPreferencesDialog *dialog,
                                       const gchar            *property1,
                                       const gchar            *property2)
{
  ExoMutualBinding *binding;
  GObject          *object;

  /* get the object from the builder */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), property1);
  panel_return_if_fail (G_IS_OBJECT (object));

  /* create the binding and prepend to the list */
  binding = exo_mutual_binding_new (G_OBJECT (dialog->active), property1, object, property2);
  dialog->bindings = g_slist_prepend (dialog->bindings, binding);
}



static void
panel_preferences_dialog_bindings_update (PanelPreferencesDialog *dialog)
{
  GdkScreen   *screen;
  GdkDisplay  *display;
  gint         n_screens, n_monitors = 1;
  GObject     *object;
  GObject     *store;
  gchar       *output_name = NULL;
  gboolean     selector_visible = TRUE;
  GtkTreeIter  iter;
  gboolean     output_selected = FALSE;
  gint         n = 0, i;
  gchar       *name, *title;

  /* remove all the active bindings */
  panel_preferences_dialog_bindings_unbind (dialog);

  /* leave when there is no active panel */
  panel_return_if_fail (G_IS_OBJECT (dialog->active));
  if (dialog->active == NULL)
    return;

  /* hook up the bindings */
  panel_preferences_dialog_bindings_add (dialog, "horizontal", "active");
  panel_preferences_dialog_bindings_add (dialog, "span-monitors", "active");
  panel_preferences_dialog_bindings_add (dialog, "locked", "active");
  panel_preferences_dialog_bindings_add (dialog, "autohide", "active");
  panel_preferences_dialog_bindings_add (dialog, "size", "value");
  panel_preferences_dialog_bindings_add (dialog, "length", "value");
  panel_preferences_dialog_bindings_add (dialog, "background-alpha", "value");
  panel_preferences_dialog_bindings_add (dialog, "enter-opacity", "value");
  panel_preferences_dialog_bindings_add (dialog, "leave-opacity", "value");
  panel_preferences_dialog_bindings_add (dialog, "composited", "visible");

  /* get run mode of the driver (multiple screens or randr) */
  display = gtk_widget_get_display (GTK_WIDGET (dialog->active));
  n_screens = gdk_display_get_n_screens (display);
  n_monitors = 1;
  if (G_LIKELY (n_screens <= 1))
    {
      screen = gtk_widget_get_screen (GTK_WIDGET (dialog->active));
      n_monitors = gdk_screen_get_n_monitors (screen);
    }

  /* show or hide the span-monitors option */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "span-monitors");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  g_object_set (G_OBJECT (object), "visible", n_monitors > 1, NULL);

  /* update the output selector */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "output-name");
  panel_return_if_fail (GTK_IS_COMBO_BOX (object));

  g_signal_handler_block (G_OBJECT (object), dialog->output_changed_handler_id);

  store = gtk_builder_get_object (GTK_BUILDER (dialog), "output-store");
  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  gtk_list_store_clear (GTK_LIST_STORE (store));

  g_object_get (G_OBJECT (dialog->active), "output-name", &output_name, NULL);

  if (n_screens > 1
      || n_monitors > 1
      || !exo_str_is_empty (output_name))
    {
      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                         OUTPUT_NAME, NULL,
                                         OUTPUT_TITLE, _("Automatic"), -1);
      if (exo_str_is_empty (output_name))
        {
          gtk_combo_box_set_active_iter  (GTK_COMBO_BOX (object), &iter);
          output_selected = TRUE;
        }

      if (n_screens > 1)
        {
          for (i = 0; i < n_screens; i++)
            {
              /* warn the user about layouts we don't support */
              screen = gdk_display_get_screen (display, i);
              if (gdk_screen_get_n_monitors (screen) > 1)
                g_message ("Screen %d has multiple monitors, the panel does not "
                           "support such a configuration", i + 1);

              /* I18N: screen name in the output selector */
              title = g_strdup_printf (_("Screen %d"), i + 1);
              name = g_strdup_printf ("screen-%d", i);
              gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                                 OUTPUT_NAME, name,
                                                 OUTPUT_TITLE, title, -1);

              if (!output_selected && exo_str_is_equal (name, output_name))
                {
                  gtk_combo_box_set_active_iter  (GTK_COMBO_BOX (object), &iter);
                  output_selected = TRUE;
                }

              g_free (name);
              g_free (title);
            }
        }
      else if (n_monitors >= 1)
        {
          for (i = 0; i < n_monitors; i++)
            {
              name = gdk_screen_get_monitor_plug_name (screen, i);
              if (exo_str_is_empty (name))
                {
                  g_free (name);

                  /* I18N: monitor name in the output selector */
                  title = g_strdup_printf (_("Monitor %d"), i + 1);
                  name = g_strdup_printf ("monitor-%d", i);
                }
              else
                {
                  /* use the randr name for the title */
                  title = g_strdup (name);
                }

              gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                                 OUTPUT_NAME, name,
                                                 OUTPUT_TITLE, title, -1);
              if (!output_selected && exo_str_is_equal (name, output_name))
                {
                  gtk_combo_box_set_active_iter  (GTK_COMBO_BOX (object), &iter);
                  output_selected = TRUE;
                }

              g_free (name);
              g_free (title);
            }
        }

      /* add the output from the config if still nothing has been selected */
      if (!output_selected && !exo_str_is_empty (output_name))
        {
          gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                             OUTPUT_NAME, output_name,
                                             OUTPUT_TITLE, output_name, -1);
          gtk_combo_box_set_active_iter  (GTK_COMBO_BOX (object), &iter);
        }
    }
  else
    {
      /* hide the selector */
      selector_visible = FALSE;
    }

  g_free (output_name);

  g_signal_handler_unblock (G_OBJECT (object), dialog->output_changed_handler_id);

  /* update visibility of the output selector */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "output-box");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  g_object_set (G_OBJECT (object), "visible", selector_visible, NULL);
}



static void
panel_preferences_dialog_output_changed (GtkComboBox            *combobox,
                                         PanelPreferencesDialog *dialog)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *output_name = NULL;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combobox));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));
  panel_return_if_fail (PANEL_WINDOW (dialog->active));

  if (gtk_combo_box_get_active_iter (combobox, &iter))
    {
      model = gtk_combo_box_get_model (combobox);
      gtk_tree_model_get (model, &iter, OUTPUT_NAME, &output_name, -1);
      g_object_set (G_OBJECT (dialog->active), "output-name", output_name, NULL);
      g_free (output_name);
    }
}



static void
panel_preferences_dialog_panel_combobox_changed (GtkComboBox            *combobox,
                                                 PanelPreferencesDialog *dialog)
{
  gint       nth;
  GtkWidget *itembar;
  GObject   *object;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combobox));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  /* disconnect signal we used to monitor changes in the itembar */
  if (dialog->active != NULL && dialog->items_changed_handler_id != 0)
    {
      itembar = gtk_bin_get_child (GTK_BIN (dialog->active));
      g_signal_handler_disconnect (G_OBJECT (itembar), dialog->items_changed_handler_id);
    }

  /* set the selected window */
  nth = gtk_combo_box_get_active (combobox);
  dialog->active = panel_application_get_nth_window (dialog->application, nth);
  panel_application_window_select (dialog->application, dialog->active);

  if (G_LIKELY (dialog->active != NULL))
    {
      itembar = gtk_bin_get_child (GTK_BIN (dialog->active));
      dialog->items_changed_handler_id =
          g_signal_connect (G_OBJECT (itembar), "changed",
                            G_CALLBACK (panel_preferences_dialog_item_store_rebuild),
                            dialog);

      /* rebind the dialog bindings */
      panel_preferences_dialog_bindings_update (dialog);

      /* update the items treeview */
      panel_preferences_dialog_item_store_rebuild (itembar, dialog);
    }

  /* sensitivity of the add button in item tab */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-add");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  gtk_widget_set_sensitive (GTK_WIDGET (object), !!(dialog->active != NULL));
}



static void
panel_preferences_dialog_panel_combobox_rebuild (PanelPreferencesDialog *dialog)
{
  GObject *store, *combo, *object;
  gint     n, n_items;
  gchar   *name;

  /* get the combo box and model */
  store = gtk_builder_get_object (GTK_BUILDER (dialog), "panel-store");
  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  combo = gtk_builder_get_object (GTK_BUILDER (dialog), "panel-combobox");
  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));

  /* block signal */
  g_signal_handlers_block_by_func (combo,
      panel_preferences_dialog_panel_combobox_changed, dialog);

  /* empty the combo box */
  gtk_list_store_clear (GTK_LIST_STORE (store));

  /* add new names */
  n_items = panel_application_get_n_windows (dialog->application);
  for (n = 0; n < n_items; n++)
    {
      /* I18N: panel combo box in the preferences dialog */
      name = g_strdup_printf (_("Panel %d"), n + 1);
      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, n, 0, name, -1);
      g_free (name);
    }

  /* set sensitivity of some widgets */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "notebook");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  gtk_widget_set_sensitive (GTK_WIDGET (object), !!(n_items > 0));

  object = gtk_builder_get_object (GTK_BUILDER (dialog), "panel-remove");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  gtk_widget_set_sensitive (GTK_WIDGET (object), !!(n_items > 1));

  /* unblock signal */
  g_signal_handlers_unblock_by_func (combo,
      panel_preferences_dialog_panel_combobox_changed, dialog);
}



static void
panel_preferences_dialog_panel_add (GtkWidget              *widget,
                                    PanelPreferencesDialog *dialog)
{
  gint         active;
  PanelWindow *window;
  GObject     *object;

  /* create new window */
  window = panel_application_new_window (dialog->application,
      gtk_widget_get_screen (widget), TRUE);

  /* block autohide */
  panel_window_freeze_autohide (window);

  /* rebuild the selector */
  panel_preferences_dialog_panel_combobox_rebuild (dialog);

  /* show window */
  gtk_widget_show (GTK_WIDGET (window));

  /* select new panel (new window is appended) */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "panel-combobox");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  active = panel_application_get_n_windows (dialog->application) - 1;
  gtk_combo_box_set_active (GTK_COMBO_BOX (object), active);
}



static void
panel_preferences_dialog_panel_remove (GtkWidget              *widget,
                                       PanelPreferencesDialog *dialog)
{
  gint       nth;
  GObject   *combo;
  GtkWidget *toplevel;

  /* get active panel */
  nth = panel_application_get_window_index (dialog->application, dialog->active);

  toplevel = gtk_widget_get_toplevel (widget);
  if (xfce_dialog_confirm (GTK_WINDOW (toplevel), GTK_STOCK_REMOVE, NULL,
          _("The panel and plugin configurations will be permanently removed"),
          _("Are you sure you want to remove panel %d?"), nth + 1))
    {
      /* release the bindings */
      panel_preferences_dialog_bindings_unbind (dialog);

      /* destroy the panel */
      gtk_widget_destroy (GTK_WIDGET (dialog->active));
      dialog->active = NULL;

      /* rebuild the selector */
      panel_preferences_dialog_panel_combobox_rebuild (dialog);

      /* select new active window */
      combo = gtk_builder_get_object (GTK_BUILDER (dialog), "panel-combobox");
      panel_return_if_fail (GTK_IS_WIDGET (combo));
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), MAX (0, nth - 1));
    }
}



static XfcePanelPluginProvider *
panel_preferences_dialog_item_get_selected (PanelPreferencesDialog *dialog,
                                            GtkTreeIter            *return_iter)
{
  GObject                 *treeview;
  XfcePanelPluginProvider *provider = NULL;
  GtkTreeModel            *model;
  GtkTreeIter              iter;
  GtkTreeSelection        *selection;

  panel_return_val_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog), NULL);

  /* get the treeview selection */
  treeview = gtk_builder_get_object (GTK_BUILDER (dialog), "item-treeview");
  panel_return_val_if_fail (GTK_IS_WIDGET (treeview), NULL);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  /* get the selection item */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      /* get the selected provider */
      gtk_tree_model_get (model, &iter, ITEM_COLUMN_PROVIDER, &provider, -1);
      panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

      if (return_iter)
        *return_iter = iter;
    }

  return provider;
}



static void
panel_preferences_dialog_item_store_rebuild (GtkWidget              *itembar,
                                             PanelPreferencesDialog *dialog)
{
  GList       *items, *li;
  guint        i;
  PanelModule *module;
  gchar       *tooltip;

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));
  panel_return_if_fail (GTK_IS_LIST_STORE (dialog->store));
  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));

  gtk_list_store_clear (dialog->store);

  /* add items to the store */
  items = gtk_container_get_children (GTK_CONTAINER (itembar));
  for (li = items, i = 0; li != NULL; li = li->next, i++)
    {
      /* I18N: tooltip in preferences dialog when hovering an item in the list */
      tooltip = g_strdup_printf (_("Internal name: %s-%d"),
                                 xfce_panel_plugin_provider_get_name (li->data),
                                 xfce_panel_plugin_provider_get_unique_id (li->data));

      /* get the panel module from the plugin */
      module = panel_module_get_from_plugin_provider (li->data);

      gtk_list_store_insert_with_values (dialog->store, NULL, i,
                                         ITEM_COLUMN_ICON_NAME,
                                         panel_module_get_icon_name (module),
                                         ITEM_COLUMN_DISPLAY_NAME,
                                         panel_module_get_display_name (module),
                                         ITEM_COLUMN_TOOLTIP,
                                         tooltip,
                                         ITEM_COLUMN_PROVIDER, li->data, -1);

      g_free (tooltip);
    }

  g_list_free (items);
}



static void
panel_preferences_dialog_item_move (GtkWidget              *button,
                                    PanelPreferencesDialog *dialog)
{
  GObject                 *treeview, *object;
  GtkTreeSelection        *selection;
  GtkTreeIter              iter_a, iter_b;
  XfcePanelPluginProvider *provider;
  GtkWidget               *itembar;
  gint                     position;
  gint                     direction;
  GtkTreePath             *path;

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  /* direction */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-up");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  direction = G_OBJECT (button) == object ? -1 : 1;

  provider = panel_preferences_dialog_item_get_selected (dialog, &iter_a);
  if (G_LIKELY (provider != NULL))
    {
      /* get the provider position on the panel */
      itembar = gtk_bin_get_child (GTK_BIN (dialog->active));
      position = panel_itembar_get_child_index (PANEL_ITEMBAR (itembar),
                                                GTK_WIDGET (provider));

      if (G_LIKELY (position != -1))
        {
          /* block the changed signal */
          g_signal_handler_block (G_OBJECT (itembar), dialog->items_changed_handler_id);

          /* move the item on the panel */
          panel_itembar_reorder_child (PANEL_ITEMBAR (itembar),
                                       GTK_WIDGET (provider),
                                       position + direction);

          /* unblock the changed signal */
          g_signal_handler_unblock (G_OBJECT (itembar), dialog->items_changed_handler_id);

          /* move the item up or down in the list */
          if (direction == 1)
            {
              /* swap the items in the list */
              iter_b = iter_a;
              if (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->store), &iter_b))
                gtk_list_store_swap (dialog->store, &iter_a, &iter_b);
            }
          else
            {
              /* get the previous item in the list */
              path = gtk_tree_model_get_path (GTK_TREE_MODEL (dialog->store), &iter_a);
              if (gtk_tree_path_prev (path))
                {
                  /* swap the items in the list */
                  gtk_tree_model_get_iter (GTK_TREE_MODEL (dialog->store), &iter_b, path);
                  gtk_list_store_swap (dialog->store, &iter_a, &iter_b);
                }

              gtk_tree_path_free (path);
            }

          /* fake update the selection */
          treeview = gtk_builder_get_object (GTK_BUILDER (dialog), "item-treeview");
          panel_return_if_fail (GTK_IS_WIDGET (treeview));
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
          panel_preferences_dialog_item_selection_changed (selection, dialog);
        }
    }
}



static void
panel_preferences_dialog_item_remove (GtkWidget              *button,
                                      PanelPreferencesDialog *dialog)
{
  XfcePanelPluginProvider *provider;
  GtkWidget               *widget, *toplevel;
  PanelModule             *module;

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  provider = panel_preferences_dialog_item_get_selected (dialog, NULL);
  if (G_LIKELY (provider != NULL))
    {
      module = panel_module_get_from_plugin_provider (provider);

      /* create question dialog (same code is also in xfce-panel-plugin.c) */
      toplevel = gtk_widget_get_toplevel (button);
      widget = gtk_message_dialog_new (GTK_WINDOW (toplevel), GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                       _("Are you sure that you want to remove \"%s\"?"),
                                       panel_module_get_display_name (module));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (widget),
                                                _("If you remove the item from the panel, "
                                                  "it is permanently lost."));
      gtk_dialog_add_buttons (GTK_DIALOG (widget), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                              GTK_STOCK_REMOVE, GTK_RESPONSE_YES, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (widget), GTK_RESPONSE_NO);

      /* run the dialog */
      if (gtk_dialog_run (GTK_DIALOG (widget)) == GTK_RESPONSE_YES)
        {
          gtk_widget_hide (widget);
          xfce_panel_plugin_provider_emit_signal (provider, PROVIDER_SIGNAL_REMOVE_PLUGIN);
        }

      gtk_widget_destroy (widget);
    }
}



static void
panel_preferences_dialog_item_add (GtkWidget              *button,
                                   PanelPreferencesDialog *dialog)
{
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  panel_item_dialog_show (dialog->active);
}



static void
panel_preferences_dialog_item_properties (GtkWidget              *button,
                                          PanelPreferencesDialog *dialog)
{
  XfcePanelPluginProvider *provider;

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  provider = panel_preferences_dialog_item_get_selected (dialog, NULL);
  if (G_LIKELY (provider != NULL))
    xfce_panel_plugin_provider_show_configure (provider);
}



static void
panel_preferences_dialog_item_about (GtkWidget              *button,
                                     PanelPreferencesDialog *dialog)
{
  XfcePanelPluginProvider *provider;

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  provider = panel_preferences_dialog_item_get_selected (dialog, NULL);
  if (G_LIKELY (provider != NULL))
    xfce_panel_plugin_provider_show_about (provider);
}



static void
panel_preferences_dialog_item_selection_changed (GtkTreeSelection       *selection,
                                                 PanelPreferencesDialog *dialog)
{
  XfcePanelPluginProvider *provider;
  GtkWidget               *itembar;
  gint                     position;
  gint                     items;
  gboolean                 active;
  GObject                 *object;
  guint                    i;
  const gchar             *button_names[] = { "item-remove", "item-up",
                                              "item-down", "item-about",
                                              "item-properties" };

  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  provider = panel_preferences_dialog_item_get_selected (dialog, NULL);
  if (G_LIKELY (provider != NULL))
    {
      /* get the current position and the items on the bar */
      itembar = gtk_bin_get_child (GTK_BIN (dialog->active));
      position = panel_itembar_get_child_index (PANEL_ITEMBAR (itembar), GTK_WIDGET (provider));
      items = panel_itembar_get_n_children (PANEL_ITEMBAR (itembar)) - 1;

      /* update sensitivity of buttons */
      object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-up");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      gtk_widget_set_sensitive (GTK_WIDGET (object), !!(position > 0 && position <= items));

      object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-down");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      gtk_widget_set_sensitive (GTK_WIDGET (object), !!(position >= 0 && position < items));

      object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-remove");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      gtk_widget_set_sensitive (GTK_WIDGET (object), TRUE);

      object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-properties");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      active = xfce_panel_plugin_provider_get_show_configure (provider);
      gtk_widget_set_sensitive (GTK_WIDGET (object), active);

      object = gtk_builder_get_object (GTK_BUILDER (dialog), "item-about");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      active = xfce_panel_plugin_provider_get_show_about (provider);
      gtk_widget_set_sensitive (GTK_WIDGET (object), active);
    }
  else
    {
      /* make all items insensitive, except for the add button */
      for (i = 0; i < G_N_ELEMENTS (button_names); i++)
        {
          object = gtk_builder_get_object (GTK_BUILDER (dialog), button_names[i]);
          panel_return_if_fail (GTK_IS_WIDGET (object));
          gtk_widget_set_sensitive (GTK_WIDGET (object), FALSE);
        }
    }
}



void
panel_preferences_dialog_show (PanelWindow *active)
{
  gint       idx = 0;
  GObject   *window, *combo;
  GdkScreen *screen;

  panel_return_if_fail (active == NULL || PANEL_IS_WINDOW (active));

  if (G_LIKELY (dialog_singleton == NULL))
    {
      /* create new dialog singleton */
      dialog_singleton = g_object_new (PANEL_TYPE_PREFERENCES_DIALOG, NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog_singleton), (gpointer) &dialog_singleton);
    }

  /* get the active window index */
  if (G_LIKELY (active))
    idx = panel_application_get_window_index (dialog_singleton->application, active);
  else
    active = panel_application_get_nth_window (dialog_singleton->application, idx);

  /* get the active screen */
  if (active != NULL)
    screen = gtk_widget_get_screen (GTK_WIDGET (active));
  else
    screen = gdk_screen_get_default ();

  /* show the dialog on the same screen as the panel */
  window = gtk_builder_get_object (GTK_BUILDER (dialog_singleton), "dialog");
  panel_return_if_fail (GTK_IS_WIDGET (window));
  gtk_window_set_screen (GTK_WINDOW (window), screen);
  gtk_window_present (GTK_WINDOW (window));

  /* select the active window in the dialog */
  combo = gtk_builder_get_object (GTK_BUILDER (dialog_singleton), "panel-combobox");
  panel_return_if_fail (GTK_IS_WIDGET (combo));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), idx);
}



gboolean
panel_preferences_dialog_visible (void)
{
  return !!(dialog_singleton != NULL);
}
