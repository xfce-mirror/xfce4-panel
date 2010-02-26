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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-application.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-module.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-preferences-dialog.h>



#define BORDER         (6)
#define ITEMS_HELP_URL "http://www.xfce.org"



static void         panel_item_dialog_finalize               (GObject            *object);
static void         panel_item_dialog_response               (GtkDialog          *dialog,
                                                              gint                response_id);
static void         panel_item_dialog_unique_changed         (PanelModuleFactory *factory,
                                                              PanelModule        *module,
                                                              PanelItemDialog    *dialog);
static gboolean     panel_item_dialog_unique_changed_foreach (GtkTreeModel       *model,
                                                              GtkTreePath        *path,
                                                              GtkTreeIter        *iter,
                                                              gpointer            user_data);
static gboolean     panel_item_dialog_separator_func         (GtkTreeModel       *model,
                                                              GtkTreeIter        *iter,
                                                              gpointer            user_data);
static PanelModule *panel_item_dialog_get_selected_module    (GtkTreeView        *treeview);
static void         panel_item_dialog_drag_begin             (GtkWidget          *treeview,
                                                              GdkDragContext     *context,
                                                              PanelItemDialog    *dialog);
static void         panel_item_dialog_drag_data_get          (GtkWidget          *treeview,
                                                              GdkDragContext     *context,
                                                              GtkSelectionData   *selection_data,
                                                              guint              info,
                                                              guint              drag_time,
                                                              PanelItemDialog   *dialog);
static void         panel_item_dialog_populate_store         (PanelItemDialog   *dialog);
static gint         panel_item_dialog_compare_func           (GtkTreeModel      *model,
                                                              GtkTreeIter       *a,
                                                              GtkTreeIter       *b,
                                                              gpointer           user_data);
static gboolean     panel_item_dialog_visible_func           (GtkTreeModel      *model,
                                                              GtkTreeIter       *iter,
                                                              gpointer           user_data);
static void         panel_item_dialog_text_renderer          (GtkTreeViewColumn *column,
                                                              GtkCellRenderer   *renderer,
                                                              GtkTreeModel      *model,
                                                              GtkTreeIter       *iter,
                                                              gpointer           user_data);



struct _PanelItemDialogClass
{
  XfceTitledDialogClass __parent__;
};

struct _PanelItemDialog
{
  XfceTitledDialog  __parent__;

  PanelApplication   *application;

  PanelModuleFactory *factory;

  /* pointers to list */
  GtkListStore       *store;
  GtkTreeView        *treeview;
};

enum
{
  COLUMN_ICON_NAME,
  COLUMN_MODULE,
  COLUMN_SENSITIVE,
  N_COLUMNS
};

static const GtkTargetEntry drag_targets[] =
{
  { "xfce-panel/plugin-name", 0, 0 },
};



G_DEFINE_TYPE (PanelItemDialog, panel_item_dialog, XFCE_TYPE_TITLED_DIALOG)



static PanelItemDialog *dialog_singleton = NULL;



static void
panel_item_dialog_class_init (PanelItemDialogClass *klass)
{
  GObjectClass   *gobject_class;
  GtkDialogClass *gtkdialog_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_item_dialog_finalize;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = panel_item_dialog_response;
}



static void
panel_item_dialog_init (PanelItemDialog *dialog)
{
  GtkWidget         *main_vbox;
  GtkWidget         *hbox;
  GtkWidget         *label;
  GtkWidget         *entry;
  GtkWidget         *scroll;
  GtkWidget         *treeview;
  GtkTreeModel      *filter;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;

  dialog->application = panel_application_get ();

  /* register the window in the application */
  panel_application_take_dialog (dialog->application, GTK_WINDOW (dialog));

  /* make the application windows insensitive */
  panel_application_windows_sensitive (dialog->application, FALSE);

  /* block autohide */
  panel_application_windows_autohide (dialog->application, TRUE);

  dialog->factory = panel_module_factory_get ();

  /* monitor unique changes */
  g_signal_connect (G_OBJECT (dialog->factory), "unique-changed",
      G_CALLBACK (panel_item_dialog_unique_changed), dialog);

  gtk_window_set_title (GTK_WINDOW (dialog), _("Add New Items"));
  xfce_titled_dialog_set_subtitle (XFCE_TITLED_DIALOG (dialog),
      _("Add new plugins to your Xfce panels"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_ADD);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 450);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                          GTK_STOCK_ADD, GTK_RESPONSE_OK,
                          GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  main_vbox = gtk_vbox_new (FALSE, BORDER * 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), BORDER);
  gtk_widget_show (main_vbox);

  /* search widget */
  hbox = gtk_hbox_new (FALSE, BORDER);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_set_tooltip_text (entry, _("Enter search phrase here"));
#if GTK_CHECK_VERSION (2, 16, 0)
  gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
#endif
  gtk_widget_show (entry);

  /* scroller */
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_vbox), scroll, TRUE, TRUE, 0);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
  gtk_widget_show (scroll);

  /* create the store and automatically sort it */
  dialog->store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_BOOLEAN);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (dialog->store), COLUMN_MODULE, panel_item_dialog_compare_func, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (dialog->store), COLUMN_MODULE, GTK_SORT_ASCENDING);

  /* create treemodel with filter */
  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (dialog->store), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), panel_item_dialog_visible_func, entry, NULL);
  g_signal_connect_swapped (G_OBJECT (entry), "changed", G_CALLBACK (gtk_tree_model_filter_refilter), filter);

  /* treeview */
  treeview = gtk_tree_view_new_with_model (filter);
  dialog->treeview = GTK_TREE_VIEW (treeview);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (treeview), FALSE);
  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (treeview), panel_item_dialog_separator_func, NULL, NULL);
  g_signal_connect_swapped (G_OBJECT (treeview), "start-interactive-search", G_CALLBACK (gtk_widget_grab_focus), entry);
  gtk_container_add (GTK_CONTAINER (scroll), treeview);
  gtk_widget_show (treeview);

  g_object_unref (G_OBJECT (filter));

  /* signals for treeview dnd */
  gtk_drag_source_set (treeview, GDK_BUTTON1_MASK, drag_targets, G_N_ELEMENTS (drag_targets), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (treeview), "drag-begin", G_CALLBACK (panel_item_dialog_drag_begin), dialog);
  g_signal_connect (G_OBJECT (treeview), "drag-data-get", G_CALLBACK (panel_item_dialog_drag_data_get), dialog);

  /* icon renderer */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes ("", renderer, "icon-name", COLUMN_ICON_NAME, "sensitive", COLUMN_SENSITIVE, NULL);
  g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_DND, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* text renderer */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (column, renderer, panel_item_dialog_text_renderer, NULL, NULL);
  gtk_tree_view_column_set_attributes (column, renderer, "sensitive", COLUMN_SENSITIVE, NULL);
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  panel_item_dialog_populate_store (dialog);
}



static void
panel_item_dialog_finalize (GObject *object)
{
  PanelItemDialog *dialog = PANEL_ITEM_DIALOG (object);

  /* disconnect unique-changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (dialog->factory),
      panel_item_dialog_unique_changed, dialog);

  /* make the windows sensitive again */
  panel_application_windows_sensitive (dialog->application, TRUE);

  /* free autohide block */
  panel_application_windows_autohide (dialog->application, FALSE);

  g_object_unref (G_OBJECT (dialog->store));
  g_object_unref (G_OBJECT (dialog->factory));
  g_object_unref (G_OBJECT (dialog->application));

  (*G_OBJECT_CLASS (panel_item_dialog_parent_class)->finalize) (object);
}



static void
panel_item_dialog_response (GtkDialog *gtk_dialog,
                            gint       response_id)
{
  GError          *error = NULL;
  PanelItemDialog *dialog = PANEL_ITEM_DIALOG (gtk_dialog);
  GdkScreen       *screen;
  PanelModule     *module;

  panel_return_if_fail (PANEL_IS_ITEM_DIALOG (dialog));
  panel_return_if_fail (GTK_IS_TREE_VIEW (dialog->treeview));
  panel_return_if_fail (PANEL_IS_APPLICATION (dialog->application));

  if (response_id == GTK_RESPONSE_HELP)
    {
      screen = gtk_widget_get_screen (GTK_WIDGET (gtk_dialog));
      if (!gtk_show_uri (screen, ITEMS_HELP_URL,
                         gtk_get_current_event_time (), &error))
        {
          xfce_dialog_show_error (GTK_WINDOW (gtk_dialog), error,
                                  _("Failed to open manual"));
          g_error_free (error);
        }
    }
  else if (response_id == GTK_RESPONSE_OK)
    {
      module = panel_item_dialog_get_selected_module (dialog->treeview);
      if (G_LIKELY (module != NULL))
        {
          panel_application_add_new_item (dialog->application,
              panel_module_get_name (module), NULL);
          g_object_unref (G_OBJECT (module));
        }
    }
  else
    {
      if (!panel_preferences_dialog_visible ())
        panel_application_window_select (dialog->application, NULL);

      gtk_widget_destroy (GTK_WIDGET (gtk_dialog));
    }
}



static void
panel_item_dialog_unique_changed (PanelModuleFactory *factory,
                                  PanelModule        *module,
                                  PanelItemDialog    *dialog)
{
  panel_return_if_fail (PANEL_IS_MODULE_FACTORY (factory));
  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (PANEL_IS_ITEM_DIALOG (dialog));
  panel_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  /* search the module and update its sensitivity */
  gtk_tree_model_foreach (GTK_TREE_MODEL (dialog->store),
      panel_item_dialog_unique_changed_foreach, module);
}



static gboolean
panel_item_dialog_unique_changed_foreach (GtkTreeModel *model,
                                          GtkTreePath  *path,
                                          GtkTreeIter  *iter,
                                          gpointer      user_data)
{
  PanelModule *module;
  gboolean     result;

  panel_return_val_if_fail (PANEL_IS_MODULE (user_data), FALSE);

  /* get the module of this iter */
  gtk_tree_model_get (model, iter, COLUMN_MODULE, &module, -1);

  /* skip the separator */
  if (G_UNLIKELY (module == NULL))
    return FALSE;

  /* check if this is the module we're looking for */
  result = !!(module == PANEL_MODULE (user_data));

  if (result)
    {
      /* update the module unique status */
      gtk_list_store_set (GTK_LIST_STORE (model), iter,
                          COLUMN_SENSITIVE, panel_module_is_usable (module), -1);
    }

  g_object_unref (G_OBJECT (module));

  /* continue searching or break if the module was found */
  return result;
}



static gboolean
panel_item_dialog_separator_func (GtkTreeModel *model,
                                  GtkTreeIter  *iter,
                                  gpointer      user_data)
{
  PanelModule *module;

  /* it's a separator if the module is null */
  gtk_tree_model_get (model, iter, COLUMN_MODULE, &module, -1);
  if (G_UNLIKELY (module == NULL))
    return TRUE;
  g_object_unref (G_OBJECT (module));

  return FALSE;
}



static PanelModule *
panel_item_dialog_get_selected_module (GtkTreeView *treeview)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  PanelModule      *module = NULL;

  panel_return_val_if_fail (GTK_IS_TREE_VIEW (treeview), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (G_LIKELY (selection != NULL))
    {
      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          gtk_tree_model_get (model, &iter, COLUMN_MODULE, &module, -1);
          if (G_LIKELY (module != NULL))
            {
              /* check if the module is still valid */
              if (!panel_module_is_valid (module))
                {
                  g_object_unref (G_OBJECT (module));

                  /* no, cannot add it, return null */
                  module = NULL;
                }
            }
        }
    }

  return module;
}



static void
panel_item_dialog_drag_begin (GtkWidget       *treeview,
                              GdkDragContext  *context,
                              PanelItemDialog *dialog)
{
  PanelModule      *module;
  const gchar      *icon_name;

  panel_return_if_fail (GTK_IS_TREE_VIEW (treeview));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_ITEM_DIALOG (dialog));

  module = panel_item_dialog_get_selected_module (GTK_TREE_VIEW (treeview));
  if (G_LIKELY (module != NULL))
    {
      if (panel_module_is_usable (module))
        {
          /* set the drag icon */
          icon_name = panel_module_get_icon_name (module);
          if (!exo_str_is_empty (icon_name))
            gtk_drag_set_icon_name (context, icon_name, 0, 0);
          else
            gtk_drag_set_icon_default (context);
        }
      else
        {
          /* plugin is not usable */
          gtk_drag_set_icon_name (context, GTK_STOCK_CANCEL, 0, 0);
        }

      g_object_unref (G_OBJECT (module));
    }
}



static void
panel_item_dialog_drag_data_get (GtkWidget        *treeview,
                                 GdkDragContext   *context,
                                 GtkSelectionData *selection_data,
                                 guint             drag_info,
                                 guint             drag_time,
                                 PanelItemDialog  *dialog)
{
  PanelModule *module;
  const gchar *internal_name;

  panel_return_if_fail (GTK_IS_TREE_VIEW (treeview));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_ITEM_DIALOG (dialog));

  module = panel_item_dialog_get_selected_module (GTK_TREE_VIEW (treeview));
  if (G_LIKELY (module != NULL))
    {
      /* set the internal module name as selection data */
      internal_name = panel_module_get_name (module);
      gtk_selection_data_set (selection_data, selection_data->target, 8,
          (guchar *) internal_name, strlen (internal_name));
      g_object_unref (G_OBJECT (module));
    }
}



static void
panel_item_dialog_populate_store (PanelItemDialog *dialog)
{
  GList       *modules, *li;
  gint         n;
  GtkTreeIter  iter;
  PanelModule *module;

  panel_return_if_fail (PANEL_IS_ITEM_DIALOG (dialog));
  panel_return_if_fail (PANEL_IS_MODULE_FACTORY (dialog->factory));
  panel_return_if_fail (GTK_IS_LIST_STORE (dialog->store));

  /* add all known modules in the factory */
  modules = panel_module_factory_get_modules (dialog->factory);
  for (li = modules, n = 0; li != NULL; li = li->next, n++)
    {
      module = PANEL_MODULE (li->data);

      gtk_list_store_insert_with_values (dialog->store, &iter, n,
          COLUMN_MODULE, module,
          COLUMN_ICON_NAME, panel_module_get_icon_name (module),
          COLUMN_SENSITIVE, panel_module_is_usable (module), -1);
    }

  g_list_free (modules);

  /* add an empty item for separator in 2nd position */
  if (panel_module_factory_has_launcher (dialog->factory))
    gtk_list_store_insert_with_values (dialog->store, &iter, 1,
                                       COLUMN_MODULE, NULL, -1);
}



static gint
panel_item_dialog_compare_func (GtkTreeModel *model,
                                GtkTreeIter  *a,
                                GtkTreeIter  *b,
                                gpointer      user_data)
{
  PanelModule *module_a;
  PanelModule *module_b;
  const gchar *name_a;
  const gchar *name_b;
  gint         result;

  /* get modules a name */
  gtk_tree_model_get (model, a, COLUMN_MODULE, &module_a, -1);
  gtk_tree_model_get (model, b, COLUMN_MODULE, &module_b, -1);

  if (G_UNLIKELY (module_a == NULL || module_b == NULL))
    {
      /* don't move the separator */
      result = 0;
    }
  else if (exo_str_is_equal (LAUNCHER_PLUGIN_NAME,
                             panel_module_get_name (module_a)))
    {
      /* move the launcher to the first position */
      result = -1;
    }
  else if (exo_str_is_equal (LAUNCHER_PLUGIN_NAME,
                             panel_module_get_name (module_b)))
    {
      /* move the launcher to the first position */
      result = 1;
    }
  else
    {
      /* get the visible module names */
      name_a = panel_module_get_display_name (module_a);
      name_b = panel_module_get_display_name (module_b);

      /* get sort order */
      if (G_LIKELY (name_a && name_b))
        result = g_utf8_collate (name_a, name_b);
      else if (name_a == name_b)
        result = 0;
      else
        result = name_a == NULL ? 1 : -1;
    }

  if (G_LIKELY (module_a))
    g_object_unref (G_OBJECT (module_a));
  if (G_LIKELY (module_b))
    g_object_unref (G_OBJECT (module_b));

  return result;
}



static gboolean
panel_item_dialog_visible_func (GtkTreeModel *model,
                                GtkTreeIter  *iter,
                                gpointer      user_data)
{

  GtkEntry    *entry = GTK_ENTRY (user_data);
  const gchar *text, *name, *comment;
  PanelModule *module;
  gchar       *normalized;
  gchar       *text_casefolded;
  gchar       *name_casefolded;
  gchar       *comment_casefolded;
  gboolean     visible = FALSE;

  /* search string from dialog */
  text = gtk_entry_get_text (entry);
  if (G_UNLIKELY (exo_str_is_empty (text)))
    return TRUE;

  gtk_tree_model_get (model, iter, COLUMN_MODULE, &module, -1);

  /* hide separator when searching */
  if (G_UNLIKELY (module == NULL))
    return FALSE;

  /* casefold the search text */
  normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
  text_casefolded = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  name = panel_module_get_display_name (module);
  if (G_LIKELY (name != NULL))
    {
      /* casefold the name */
      normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
      name_casefolded = g_utf8_casefold (normalized, -1);
      g_free (normalized);

      /* search */
      visible = (strstr (name_casefolded, text_casefolded) != NULL);

      g_free (name_casefolded);
    }

  if (!visible)
    {
      comment = panel_module_get_comment (module);
      if (comment != NULL)
        {
          /* casefold the comment */
          normalized = g_utf8_normalize (comment, -1, G_NORMALIZE_ALL);
          comment_casefolded = g_utf8_casefold (normalized, -1);
          g_free (normalized);

          /* search */
          visible = (strstr (comment_casefolded, text_casefolded) != NULL);

          g_free (comment_casefolded);
        }
    }

  g_free (text_casefolded);
  g_object_unref (G_OBJECT (module));

  return visible;
}



static void
panel_item_dialog_text_renderer (GtkTreeViewColumn *column,
                                 GtkCellRenderer   *renderer,
                                 GtkTreeModel      *model,
                                 GtkTreeIter       *iter,
                                 gpointer           user_data)
{
  PanelModule *module;
  gchar       *markup;
  const gchar *name, *comment;

  gtk_tree_model_get (model, iter, COLUMN_MODULE, &module, -1);
  if (G_UNLIKELY (module == NULL))
    return;

  /* avoid (null) in markup string */
  comment = panel_module_get_comment (module);
  if (exo_str_is_empty (comment))
    comment = "";

  name = panel_module_get_display_name (module);
  markup = g_markup_printf_escaped ("<b>%s</b>\n%s", name, comment);
  g_object_set (G_OBJECT (renderer), "markup", markup, NULL);
  g_free (markup);

  g_object_unref (G_OBJECT (module));
}



void
panel_item_dialog_show (PanelWindow *window)
{
  GdkScreen        *screen;
  PanelApplication *application;

  panel_return_if_fail (window == NULL || PANEL_IS_WINDOW (window));

  /* check if not the entire application is locked */
  if (panel_dialogs_kiosk_warning ())
    return;

  if (G_LIKELY (dialog_singleton == NULL))
    {
      /* create new dialog singleton */
      dialog_singleton = g_object_new (PANEL_TYPE_ITEM_DIALOG, NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog_singleton), (gpointer) &dialog_singleton);
    }

  /* show the dialog on the same screen as the panel */
  if (G_UNLIKELY (window != NULL))
    {
      /* set the active panel */
      application = panel_application_get ();
      panel_application_window_select (application, window);
      g_object_unref (G_OBJECT (application));

      screen = gtk_window_get_screen (GTK_WINDOW (window));
    }
  else
    {
      screen = gdk_screen_get_default ();
    }
  gtk_window_set_screen (GTK_WINDOW (dialog_singleton), screen);

  /* show the dialog */
  gtk_widget_show (GTK_WIDGET (dialog_singleton));

  /* focus the window */
  gtk_window_present (GTK_WINDOW (dialog_singleton));
}



gboolean
panel_item_dialog_visible (void)
{
  return !!(dialog_singleton != NULL);
}
