#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "showdesktop.h"
#include "showdesktop-dialogs.h"

static void
showdesktop_configure_response (GtkWidget    *dialog,
                                gint          response,
                                ShowDesktopPlugin *sample)
{
  XfcePanelPlugin* plugin = XFCE_PANEL_PLUGIN (sample);

  /* remove the dialog data from the plugin */
  g_object_set_data (G_OBJECT(plugin), "dialog", NULL);

  /* unlock the panel menu */
  xfce_panel_plugin_unblock_menu (plugin);

  /* destroy the properties dialog */
  gtk_widget_destroy (dialog);
}



void
showdesktop_configure (XfcePanelPlugin *plugin,
                       ShowDesktopPlugin    *sample)
{
  GtkWidget *dialog, *vbox, *frame, *bin, *grid, *label, *show_on_mouse_hover_checkbox;

  /* block the plugin menu */
  xfce_panel_plugin_block_menu (plugin);

  /* create the dialog */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  dialog = xfce_titled_dialog_new_with_buttons (_("Show desktop"),
                                                NULL, 0, "gtk-close",
                                                GTK_RESPONSE_OK, NULL);
  G_GNUC_END_IGNORE_DEPRECATIONS

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
  gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox);

  /* center dialog on the screen */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* set dialog icon */
  gtk_window_set_icon_name (GTK_WINDOW (dialog), "org.xfce.panel.showdesktop");

  frame = xfce_gtk_frame_box_new (_("Behavior"), &bin);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gtk_alignment_set_padding (GTK_ALIGNMENT (bin), 6, 0, 12, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), TRUE);
  gtk_widget_set_size_request (grid, -1, -1);
  gtk_container_add (GTK_CONTAINER (bin), grid);

  label = gtk_label_new (_("Show desktop on mouse hover:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.f);
  gtk_widget_set_hexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);

  show_on_mouse_hover_checkbox = gtk_check_button_new();
  gtk_grid_attach (GTK_GRID (grid), show_on_mouse_hover_checkbox, 1, 0, 1, 1);

  gtk_widget_show_all (vbox);

  /* link the dialog to the plugin, so we can destroy it when the plugin
   * is closed, but the dialog is still open */
  g_object_set_data (G_OBJECT (plugin), "dialog", dialog);

  /* connect the response signal to the dialog */
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK(showdesktop_configure_response), sample);

  g_object_bind_property (G_OBJECT (sample), "show-on-mouse-hover",
                          G_OBJECT (show_on_mouse_hover_checkbox), "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  /* show the entire dialog */
  gtk_widget_show (dialog);
}
