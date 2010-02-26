/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-window.h>
#include <panel/panel-glue.h>
#include <panel/panel-application.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-preferences-dialog-glade.h>

#define BORDER               (6)
#define PREFERENCES_HELP_URL "http://www.xfce.org"



static void panel_preferences_dialog_class_init (PanelPreferencesDialogClass *klass);
static void panel_preferences_dialog_init (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_finalize (GObject *object);
static void panel_preferences_dialog_help (GtkWidget *button);

static void panel_preferences_dialog_rebuild_selector (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_bindings_unbind (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_bindings_add (PanelPreferencesDialog *dialog, const gchar *property1, const gchar *property2);
static void panel_preferences_dialog_bindings_update (GtkComboBox *combobox, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_window_add (GtkWidget *widget, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_window_remove (GtkWidget *widget, PanelPreferencesDialog *dialog);



struct _PanelPreferencesDialogClass
{
  GtkBuilderClass __parent__;
};

struct _PanelPreferencesDialog
{
  GtkBuilder  __parent__;

  /* application we're handling */
  PanelApplication *application;

  /* currently selected window in the selector */
  PanelWindow      *active;

  /* list of exo bindings */
  GSList           *bindings;
};



G_DEFINE_TYPE (PanelPreferencesDialog, panel_preferences_dialog, GTK_TYPE_BUILDER);



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
  GObject *window, *object;

  /* init */
  dialog->bindings = NULL;
  dialog->application = panel_application_get ();

  /* block all autohides */
  panel_application_windows_autohide (dialog->application, TRUE);

  /* load the builder data into the object */
  gtk_builder_add_from_string (GTK_BUILDER (dialog), panel_preferences_dialog_glade, panel_preferences_dialog_glade_length, NULL);

  /* get the dialog */
  window = gtk_builder_get_object (GTK_BUILDER (dialog), "dialog");
  panel_application_take_dialog (dialog->application, GTK_WINDOW (window));
  g_object_weak_ref (G_OBJECT (window), (GWeakNotify) g_object_unref, dialog);

  /* connect the close button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "close-button");
  g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), window);

  /* connect the help button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "help-button");
  g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (panel_preferences_dialog_help), NULL);

  /* connect the add button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "add-panel");
  g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (panel_preferences_dialog_window_add), dialog);

  /* connect the remove button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "remove-panel");
  g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (panel_preferences_dialog_window_remove), dialog);

  /* connect the panel selector */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "panels");
  g_signal_connect (G_OBJECT (object), "changed", G_CALLBACK (panel_preferences_dialog_bindings_update), dialog);

  /* TODO remove when implemented by glade */
  GtkCellRenderer *cell1 = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), cell1, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), cell1, "text", 0, NULL);

  GtkCellRenderer *cell2 = gtk_cell_renderer_text_new ();
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "horizontal");
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), cell2, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), cell2, "text", 0, NULL);

  /* rebuild the selector */
  panel_preferences_dialog_rebuild_selector (dialog);

  /* show the dialog */
  gtk_widget_show (GTK_WIDGET (window));
}



static void
panel_preferences_dialog_finalize (GObject *object)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (object);

  /* thaw all autohide blocks */
  panel_application_windows_autohide (dialog->application, FALSE);

  /* deselect all windows */
  panel_application_window_select (dialog->application, NULL);

  /* release the application */
  g_object_unref (G_OBJECT (dialog->application));

  (*G_OBJECT_CLASS (panel_preferences_dialog_parent_class)->finalize) (object);
}



static void
panel_preferences_dialog_help (GtkWidget *button)
{
  GError    *error = NULL;
  GdkScreen *screen;

  /* get the dialog screen */
  screen = gtk_widget_get_screen (button);

  /* open the help url */
  if (exo_url_show_on_screen (PREFERENCES_HELP_URL, NULL, screen, &error) == FALSE)
    {
      /* show error and cleanup */
      g_warning ("Failed to open help: %s", error->message);
      g_error_free (error);
    }
}



static void
panel_preferences_dialog_rebuild_selector (PanelPreferencesDialog *dialog)
{
  GObject      *model, *combo;
  gint          n, n_items;
  gchar        *name;

  /* get the combo box and model */
  model = gtk_builder_get_object (GTK_BUILDER (dialog), "panels-store");
  combo = gtk_builder_get_object (GTK_BUILDER (dialog), "panels");

  /* block signal */
  g_signal_handlers_block_by_func (combo, panel_preferences_dialog_bindings_update, dialog);

  /* empty the combo box */
  if (GTK_IS_LIST_STORE (model))
    gtk_list_store_clear (GTK_LIST_STORE (model));

  /* add new names */
  n_items = panel_application_get_n_windows (dialog->application);
  for (n = 0; n < n_items; n++)
    {
      name = g_strdup_printf ("Panel %d", n + 1);
      gtk_list_store_insert_with_values (GTK_LIST_STORE (model), NULL, n, 0, name, -1);
      g_free (name);
    }

  /* unblock signal */
  g_signal_handlers_unblock_by_func (combo, panel_preferences_dialog_bindings_update, dialog);
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

      /* cleanup */
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

  /* create the binding and prepend to the list */
  binding = exo_mutual_binding_new (G_OBJECT (dialog->active), property1, object, property2);
  dialog->bindings = g_slist_prepend (dialog->bindings, binding);
}



static void
panel_preferences_dialog_bindings_update (GtkComboBox            *combobox,
                                          PanelPreferencesDialog *dialog)
{
  gint nth;

  panel_return_if_fail (GTK_IS_COMBO_BOX (combobox));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  /* remove all the active bindings */
  panel_preferences_dialog_bindings_unbind (dialog);

  /* set the selected window */
  nth = gtk_combo_box_get_active (combobox);
  dialog->active = panel_application_get_window (dialog->application, nth);

  /* hook up the bindings */
  panel_preferences_dialog_bindings_add (dialog, "horizontal", "active");
  panel_preferences_dialog_bindings_add (dialog, "locked", "active");
  panel_preferences_dialog_bindings_add (dialog, "autohide", "active");
  panel_preferences_dialog_bindings_add (dialog, "size", "value");
  panel_preferences_dialog_bindings_add (dialog, "length", "value");
  panel_preferences_dialog_bindings_add (dialog, "background-alpha", "value");
  panel_preferences_dialog_bindings_add (dialog, "enter-opacity", "value");
  panel_preferences_dialog_bindings_add (dialog, "leave-opacity", "value");

  /* update selection state */
  panel_application_window_select (dialog->application, dialog->active);
}



static void
panel_preferences_dialog_window_add (GtkWidget              *widget,
                                     PanelPreferencesDialog *dialog)
{
  gint         active;
  PanelWindow *window;
  GObject     *object;

  /* create new window */
  window = panel_application_new_window (dialog->application, gtk_widget_get_screen (widget));

  /* block autohide */
  panel_window_freeze_autohide (window);

  /* rebuild the selector */
  panel_preferences_dialog_rebuild_selector (dialog);

  /* set the sensitivity of the remove button */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "remove-panel");
  gtk_widget_set_sensitive (GTK_WIDGET (object), panel_application_get_n_windows (dialog->application) > 1);

  /* select new panel (new window is appended) */
  object = gtk_builder_get_object (GTK_BUILDER (dialog), "panels");
  active = panel_application_get_n_windows (dialog->application) - 1;
  gtk_combo_box_set_active (GTK_COMBO_BOX (object), active);

  /* show window */
  gtk_widget_show (GTK_WIDGET (window));
}



static void
panel_preferences_dialog_window_remove (GtkWidget              *widget,
                                        PanelPreferencesDialog *dialog)
{
  gint     nth;
  GObject *combo;

  /* get active panel */
  nth = panel_application_get_window_index (dialog->application, dialog->active);

  /* destroy the window */
  if (xfce_dialog_confirm (dialog, GTK_STOCK_REMOVE, NULL,
                           "Are you sure you want to remove panel %d?", nth + 1))
    {
      /* release the bindings */
      panel_preferences_dialog_bindings_unbind (dialog);

      /* destroy the panel */
      gtk_widget_destroy (GTK_WIDGET (dialog->active));

      /* set the sensitivity of the remove button */
      gtk_widget_set_sensitive (widget, panel_application_get_n_windows (dialog->application) > 1);

      /* rebuild the selector */
      panel_preferences_dialog_rebuild_selector (dialog);

      /* select new active window */
      combo = gtk_builder_get_object (GTK_BUILDER (dialog), "panels");
      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), MAX (0, nth - 1));
    }
}



void
panel_preferences_dialog_show (PanelWindow *active)
{
  static PanelPreferencesDialog *dialog = NULL;
  gint                           idx = 0;
  GObject                       *window, *combo;

  panel_return_if_fail (active == NULL || PANEL_IS_WINDOW (active));

  if (G_LIKELY (dialog == NULL))
    {
      /* create new dialog singleton */
      dialog = g_object_new (PANEL_TYPE_PREFERENCES_DIALOG, NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);
    }

  /* get the active window index */
  if (G_LIKELY (active))
    idx = panel_application_get_window_index (dialog->application, active);
  else
    active = panel_application_get_window (dialog->application, idx);

  /* show the dialog on the same screen as the panel */
  window = gtk_builder_get_object (GTK_BUILDER (dialog), "dialog");
  gtk_window_set_screen (GTK_WINDOW (window), gtk_widget_get_screen (GTK_WIDGET (active)));
  gtk_window_present (GTK_WINDOW (window));

  /* select the active window in the dialog */
  combo = gtk_builder_get_object (GTK_BUILDER (dialog), "panels");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), idx);
}
