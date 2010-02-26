/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-private.h>
#include <panel/panel-window.h>
#include <panel/panel-glue.h>
#include <panel/panel-application.h>
#include <panel/panel-preferences-dialog.h>

#define BORDER               (6)
#define PREFERENCES_HELP_URL "http://www.xfce.org"



static void panel_preferences_dialog_class_init (PanelPreferencesDialogClass *klass);
static void panel_preferences_dialog_init (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void panel_preferences_dialog_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void panel_preferences_dialog_finalize (GObject *object);
static void panel_preferences_dialog_response (GtkDialog *dialog, gint response_id);
static gboolean panel_preferences_dialog_save_timeout (gpointer user_data);
static void panel_preferences_dialog_save_timeout_destroyed (gpointer user_data);
static void panel_preferences_dialog_rebuild_selector (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_set_window (PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_add_window (GtkWidget *widget, PanelPreferencesDialog *dialog);
static void panel_preferences_dialog_remove_window (GtkWidget *widget, PanelPreferencesDialog *dialog);



struct _PanelPreferencesDialogClass
{
  XfceTitledDialogClass __parent__;
};

struct _PanelPreferencesDialog
{
  XfceTitledDialog  __parent__;

  /* application we're handling */
  PanelApplication *application;

  /* currently selected window in the selector */
  PanelWindow      *active;

  /* panel selector widget */
  GtkWidget        *selector;

  /* remove button */
  GtkWidget        *remove_button;

  /* save timeout id */
  guint             save_timeout_id;
};

enum
{
  PROP_0,
  PROP_COMPOSITE,
  PROP_COMPOSITE_RGBA,
  PROP_LOCKED,
  PROP_ORIENTATION,
  PROP_AUTOHIDE,
  PROP_SIZE,
  PROP_LENGTH,
  PROP_BACKGROUND_ALPHA,
  PROP_ENTER_OPACITY,
  PROP_LEAVE_OPACITY,
  PROP_SPAN_MONITORS
};



G_DEFINE_TYPE (PanelPreferencesDialog, panel_preferences_dialog, XFCE_TYPE_TITLED_DIALOG);



static void
panel_preferences_dialog_class_init (PanelPreferencesDialogClass *klass)
{
  GObjectClass *gobject_class;
  GtkDialogClass *gtkdialog_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_preferences_dialog_finalize;
  gobject_class->get_property = panel_preferences_dialog_get_property;
  gobject_class->set_property = panel_preferences_dialog_set_property;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->response = panel_preferences_dialog_response;

  g_object_class_install_property (gobject_class,
                                   PROP_COMPOSITE,
                                   g_param_spec_boolean ("composite", "composite", "composite",
                                                         FALSE,
                                                         EXO_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_COMPOSITE_RGBA,
                                   g_param_spec_boolean ("composite-rgba", "composite-rgba", "composite-rgba",
                                                         FALSE,
                                                         EXO_PARAM_READABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_LOCKED,
                                   g_param_spec_boolean ("locked", "locked", "locked",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", "orientation", "orientation",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_AUTOHIDE,
                                   g_param_spec_boolean ("autohide", "autohide", "autohide",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_int ("size", "size", "size",
                                                     16, 128, 16,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LENGTH,
                                   g_param_spec_int ("length", "length", "length",
                                                     0, 100, 0,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BACKGROUND_ALPHA,
                                   g_param_spec_int ("background-alpha", "background-alpha", "background-alpha",
                                                     0, 100, 0,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ENTER_OPACITY,
                                   g_param_spec_int ("enter-opacity", "enter-opacity", "enter-opacity",
                                                     0, 100, 0,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_LEAVE_OPACITY,
                                   g_param_spec_int ("leave-opacity", "leave-opacity", "leave-opacity",
                                                     0, 100, 0,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SPAN_MONITORS,
                                   g_param_spec_boolean ("span-monitors", "span-monitors", "span-monitors",
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
}



static void
panel_preferences_dialog_init (PanelPreferencesDialog *dialog)
{
  GtkWidget    *main_vbox;
  GtkWidget    *hbox;
  GtkWidget    *combo;
  GtkWidget    *button;
  GtkWidget    *image;
  GtkWidget    *notebook;
  GtkWidget    *label;
  GtkWidget    *notebook_vbox;
  GtkWidget    *vbox;
  GtkWidget    *frame;
  GtkWidget    *scale;
  GtkSizeGroup *sg;
  GtkObject    *adjustment;

  /* initialize */
  dialog->save_timeout_id = 0;

  /* get application */
  dialog->application = panel_application_get ();

  /* block all autohides */
  panel_application_windows_autohide (dialog->application, TRUE);

  /* register the dialog in the application */
  panel_application_take_dialog (dialog->application, GTK_WINDOW (dialog));

  /* set the first window */
  dialog->active = panel_application_get_window (dialog->application, 0);

  /* signal to monitor compositor changes */
  g_signal_connect_swapped (G_OBJECT (dialog), "composited-changed", G_CALLBACK (panel_preferences_dialog_set_window), dialog);

  /* setup dialog */
  gtk_window_set_title (GTK_WINDOW (dialog), _("Xfce Panel Preferences"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PREFERENCES);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  /* add buttons */
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  /* create main box */
  main_vbox = gtk_vbox_new (FALSE, BORDER * 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), BORDER);
  gtk_widget_show (main_vbox);

  /* box for panel selector */
  hbox = gtk_hbox_new (FALSE, BORDER);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  combo = dialog->selector = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  g_signal_connect_swapped (G_OBJECT (combo), "changed", G_CALLBACK (panel_preferences_dialog_set_window), dialog);
  gtk_widget_show (combo);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (panel_preferences_dialog_add_window), dialog);
  gtk_widget_show (button);

  button = dialog->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (panel_preferences_dialog_remove_window), dialog);
  gtk_widget_set_sensitive (button, panel_application_get_n_windows (dialog->application) > 1);
  gtk_widget_show (button);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  /* display tab */
  notebook_vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_container_set_border_width (GTK_CONTAINER (notebook_vbox), BORDER);
  gtk_widget_show (notebook_vbox);

  label = gtk_label_new (_("Display"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), notebook_vbox, label);

  /* general frame */
  vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_widget_show (vbox);

  frame = xfce_gtk_frame_box_new_with_content (_("General"), vbox);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Orientation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Horizontal");
  gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "Vertical");
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
  exo_mutual_binding_new (G_OBJECT (dialog), "orientation", G_OBJECT (combo), "active");
  gtk_widget_show (combo);

  button = gtk_check_button_new_with_mnemonic (_("_Lock Panel"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  _widget_set_tooltip_text (button, _("When the panel is locked, it cannot be moved and the handles will be hidden."));
  exo_mutual_binding_new (G_OBJECT (dialog), "locked", G_OBJECT (button), "active");
  gtk_widget_show (button);

  button = gtk_check_button_new_with_mnemonic (_("Automatically show and hi_de the panel"));
  _widget_set_tooltip_text (button, _("Hide the panel when you move your mouse out of it. "
                                      "When the poiner enter the panel border on the screen egde, "
                                      "the panel will reappear."));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "autohide", G_OBJECT (button), "active");
  gtk_widget_show (button);

  /* measurements frame */
  vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_widget_show (vbox);

  frame = xfce_gtk_frame_box_new_with_content (_("Measurements"), vbox);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Size (pixels):"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  gtk_size_group_add_widget (sg, label);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (16, 16, 128, 1, 10, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "size", G_OBJECT (adjustment), "value");

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scale);
  gtk_widget_show (scale);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("L_ength (%):"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  gtk_size_group_add_widget (sg, label);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (1, 1, 100, 1, 10, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "length", G_OBJECT (adjustment), "value");

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scale);
  gtk_widget_show (scale);

  g_object_unref (G_OBJECT (sg));

  /* compositing tab */
  notebook_vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_container_set_border_width (GTK_CONTAINER (notebook_vbox), BORDER);

  label = gtk_label_new (_("Compositing"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), notebook_vbox, label);
  exo_binding_new (G_OBJECT (dialog), "composite", G_OBJECT (notebook_vbox), "visible");

  sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* background alpha frame */
  vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_widget_show (vbox);

  frame = xfce_gtk_frame_box_new_with_content (_("Background"), vbox);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Alpha:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  exo_binding_new (G_OBJECT (dialog), "composite-rgba", G_OBJECT (label), "visible");
  gtk_size_group_add_widget (sg, label);

  adjustment = gtk_adjustment_new (100, 0, 100, 1, 10, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "background-alpha", G_OBJECT (adjustment), "value");

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  exo_binding_new (G_OBJECT (dialog), "composite-rgba", G_OBJECT (scale), "visible");
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scale);

  label = gtk_label_new (_("You need to restart the panel to set the alpha channel of the panel background."));
  exo_binding_new_with_negation (G_OBJECT (dialog), "composite-rgba", G_OBJECT (label), "visible");
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  /* transparency frame */
  vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_widget_show (vbox);

  frame = xfce_gtk_frame_box_new_with_content (_("Transparency"), vbox);
  gtk_box_pack_start (GTK_BOX (notebook_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Enter:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  gtk_size_group_add_widget (sg, label);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (100, 0, 100, 1, 10, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "enter-opacity", G_OBJECT (adjustment), "value");

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scale);
  gtk_widget_show (scale);

  hbox = gtk_hbox_new (FALSE, BORDER * 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Leave:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.00, 0.50);
  gtk_size_group_add_widget (sg, label);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (100, 0, 100, 1, 10, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "leave-opacity", G_OBJECT (adjustment), "value");

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_RIGHT);
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), scale);
  gtk_widget_show (scale);

  g_object_unref (G_OBJECT (sg));

  /* multi screen tab */
  notebook_vbox = gtk_vbox_new (FALSE, BORDER);
  gtk_container_set_border_width (GTK_CONTAINER (notebook_vbox), BORDER);
  gtk_widget_show (notebook_vbox);

  label = gtk_label_new (_("Multi Screen"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), notebook_vbox, label);

  button = gtk_check_button_new_with_mnemonic (_("Span Monitors"));
  gtk_box_pack_start (GTK_BOX (notebook_vbox), button, FALSE, FALSE, 0);
  exo_mutual_binding_new (G_OBJECT (dialog), "span-monitors", G_OBJECT (button), "active");
  gtk_widget_show (button);

  /* rebuild the selector */
  panel_preferences_dialog_rebuild_selector (dialog);
}



static void
panel_preferences_dialog_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (object);
  PanelWindow            *window = dialog->active;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  switch (prop_id)
    {
      case PROP_COMPOSITE:
        g_value_set_boolean (value, gtk_widget_is_composited (GTK_WIDGET (window)));
        break;

      case PROP_COMPOSITE_RGBA:
        g_value_set_boolean (value, panel_window_is_composited (window));
        break;

      case PROP_LOCKED:
         g_value_set_boolean (value, panel_window_get_locked (window));
        break;

      case PROP_ORIENTATION:
        g_value_set_enum (value, panel_window_get_orientation (window));
        break;

      case PROP_AUTOHIDE:
        g_value_set_boolean (value, panel_window_get_autohide (window));
        break;

      case PROP_SIZE:
        g_value_set_int (value, panel_window_get_size (window));
        break;

      case PROP_LENGTH:
        g_value_set_int (value, panel_window_get_length (window));
        break;

      case PROP_BACKGROUND_ALPHA:
        g_value_set_int (value, panel_window_get_background_alpha (window));
        break;

      case PROP_ENTER_OPACITY:
        g_value_set_int (value, panel_window_get_enter_opacity (window));
        break;

      case PROP_LEAVE_OPACITY:
        g_value_set_int (value, panel_window_get_leave_opacity (window));
        break;

      case PROP_SPAN_MONITORS:
        g_value_set_boolean (value, panel_window_get_span_monitors (window));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
panel_preferences_dialog_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (object);
  PanelWindow            *window = dialog->active;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (PANEL_IS_PREFERENCES_DIALOG (dialog));

  /* stop pending timeout id */
  if (dialog->save_timeout_id != 0)
    g_source_remove (dialog->save_timeout_id);

  switch (prop_id)
    {
      case PROP_LOCKED:
        panel_window_set_locked (window, g_value_get_boolean (value));
        break;

      case PROP_ORIENTATION:
        panel_glue_set_orientation (window, g_value_get_enum (value));
        break;

      case PROP_AUTOHIDE:
        panel_window_set_autohide (window, g_value_get_boolean (value));
        break;

      case PROP_SIZE:
        panel_glue_set_size (window, g_value_get_int (value));
        break;

      case PROP_LENGTH:
        panel_window_set_length (window, g_value_get_int (value));
        break;

      case PROP_BACKGROUND_ALPHA:
        panel_window_set_background_alpha (window, g_value_get_int (value));
        break;

      case PROP_ENTER_OPACITY:
        panel_window_set_enter_opacity (window, g_value_get_int (value));
        break;

      case PROP_LEAVE_OPACITY:
        panel_window_set_leave_opacity (window, g_value_get_int (value));
        break;

      case PROP_SPAN_MONITORS:
        panel_window_set_span_monitors (window, g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }

  /* schedule a new save timeout */
  dialog->save_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 1000,
                                                panel_preferences_dialog_save_timeout, dialog,
                                                panel_preferences_dialog_save_timeout_destroyed);

  /* don't leave the window */
  gtk_window_present (GTK_WINDOW (dialog));
}



static void
panel_preferences_dialog_finalize (GObject *object)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (object);

  /* save the settings */
  if (dialog->save_timeout_id != 0)
    {
      /* stop pending timeout */
      g_source_remove (dialog->save_timeout_id);

      /* save */
      panel_application_save (dialog->application);
    }

  /* thaw all autohide blocks */
  panel_application_windows_autohide (dialog->application, FALSE);

  /* release the application */
  g_object_unref (G_OBJECT (dialog->application));

  (*G_OBJECT_CLASS (panel_preferences_dialog_parent_class)->finalize) (object);
}



static gboolean
panel_preferences_dialog_save_timeout (gpointer user_data)
{
  PanelPreferencesDialog *dialog = PANEL_PREFERENCES_DIALOG (user_data);

  panel_return_val_if_fail (PANEL_IS_PREFERENCES_DIALOG (user_data), FALSE);
  panel_return_val_if_fail (PANEL_IS_APPLICATION (dialog->application), FALSE);

  /* save settings */
  panel_application_save (dialog->application);

  return FALSE;
}



static void
panel_preferences_dialog_save_timeout_destroyed (gpointer user_data)
{
  PANEL_PREFERENCES_DIALOG (user_data)->save_timeout_id = 0;
}



static void
panel_preferences_dialog_response (GtkDialog *dialog,
                                   gint       response_id)
{
  GError    *error = NULL;
  GdkScreen *screen;

  if (response_id == GTK_RESPONSE_HELP)
    {
      /* get the dialog screen */
      screen = gtk_widget_get_screen (GTK_WIDGET (dialog));

      /* open the help url */
      if (exo_url_show_on_screen (PREFERENCES_HELP_URL, NULL, screen, &error) == FALSE)
        {
          /* show error */
          g_warning ("Failed to open help: %s", error->message);

          /* cleanup */
          g_error_free (error);
        }
    }
  else
    {
      /* deselect all windows */
      panel_application_window_select (PANEL_PREFERENCES_DIALOG (dialog)->application, NULL);

      /* destroy the dialog */
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}



static void
panel_preferences_dialog_rebuild_selector (PanelPreferencesDialog *dialog)
{
  GtkComboBox  *combo = GTK_COMBO_BOX (dialog->selector);
  gint          n, n_items;
  gchar        *name;
  GtkTreeModel *model;

  /* block signal */
  g_signal_handlers_block_by_func (G_OBJECT (combo), panel_preferences_dialog_set_window, dialog);

  /* empty the combo box */
  model = gtk_combo_box_get_model (combo);
  if (GTK_IS_LIST_STORE (model))
    gtk_list_store_clear (GTK_LIST_STORE (model));

  /* add new names */
  n_items = panel_application_get_n_windows (dialog->application);
  for (n = 0; n < n_items; n++)
    {
      name = g_strdup_printf ("Panel %d", n + 1);
      gtk_combo_box_append_text (combo, name);
      g_free (name);
    }

  /* unblock signal */
  g_signal_handlers_unblock_by_func (G_OBJECT (combo), panel_preferences_dialog_set_window, dialog);
}



static void
panel_preferences_dialog_set_window (PanelPreferencesDialog *dialog)
{
  gint         active;
  GParamSpec **specs;
  guint        n, nspecs;

  /* get selected number */
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->selector));

  /* set window */
  dialog->active = panel_application_get_window (dialog->application, active);

  /* notify all properties */
  specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (dialog), &nspecs);
  for (n = 0; n < nspecs; n++)
    g_object_notify (G_OBJECT (dialog), specs[n]->name);
  g_free (specs);

  /* update selection state */
  panel_application_window_select (dialog->application, dialog->active);
}



static void
panel_preferences_dialog_add_window (GtkWidget              *widget,
                                     PanelPreferencesDialog *dialog)
{
  gint         active;
  PanelWindow *window;

  /* create new window */
  window = panel_application_new_window (dialog->application, gtk_widget_get_screen (widget));

  /* block autohide */
  panel_window_freeze_autohide (window);

  /* rebuild the selector */
  panel_preferences_dialog_rebuild_selector (dialog);

  /* set the sensitivity of the remove button */
  gtk_widget_set_sensitive (dialog->remove_button, panel_application_get_n_windows (dialog->application) > 1);

  /* select new panel */
  active = panel_application_get_n_windows (dialog->application) - 1;
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->selector), active);

  /* show window */
  gtk_widget_show (GTK_WIDGET (window));
}



static void
panel_preferences_dialog_remove_window (GtkWidget              *widget,
                                        PanelPreferencesDialog *dialog)
{
  gint active;

  /* get active panel */
  active = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->selector));

  /* destroy the window */
  if (xfce_dialog_confirm (dialog, GTK_STOCK_REMOVE, NULL,
                           "Are you sure you want to remove panel %d?", active + 1))
    {
      /* destroy the panel */
      gtk_widget_destroy (GTK_WIDGET (dialog->active));

      /* rebuild the selector */
      panel_preferences_dialog_rebuild_selector (dialog);

      /* set the sensitivity of the remove button */
      gtk_widget_set_sensitive (widget, panel_application_get_n_windows (dialog->application) > 1);

      /* select new active window */
      gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->selector), MAX (0, active - 1));
    }
}



void
panel_preferences_dialog_show (PanelWindow *active)
{
  static PanelPreferencesDialog *dialog = NULL;
  gint                           idx;

  panel_return_if_fail (active == NULL || PANEL_IS_WINDOW (active));

  if (G_LIKELY (dialog == NULL))
    {
      /* create new dialog singleton */
      dialog = g_object_new (PANEL_TYPE_PREFERENCES_DIALOG, NULL);
      g_object_add_weak_pointer (G_OBJECT (dialog), (gpointer) &dialog);
      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* focus the window */
      gtk_window_present (GTK_WINDOW (dialog));
    }

  /* get the active window index */
  if (G_LIKELY (active))
    idx = panel_application_get_window_index (dialog->application, active);
  else
    idx = 0;

  /* select the active window in the dialog */
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->selector), idx);
}
