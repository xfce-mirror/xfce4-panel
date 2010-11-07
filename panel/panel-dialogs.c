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
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-dialogs.h>
#include <panel/panel-application.h>
#include <panel/panel-tic-tac-toe.h>



static void
panel_dialogs_show_about_email_hook (GtkAboutDialog *dialog,
                                     const gchar    *uri,
                                     gpointer        data)
{
  if (g_strcmp0 ("tictactoe@xfce.org", uri) == 0)
    {
      /* open tic-tac-toe */
      panel_tic_tac_toe_show ();
    }
  else
    {
      exo_gtk_url_about_dialog_hook (dialog, uri, data);
    }
}



void
panel_dialogs_show_about (void)
{
  static const gchar *authors[] =
  {
    "Jasper Huijsmans <jasper@xfce.org>",
    "Nick Schermer <nick@xfce.org>",
    "Tic-tac-toe <tictactoe@xfce.org>",
    NULL
  };

  gtk_about_dialog_set_email_hook (panel_dialogs_show_about_email_hook, NULL, NULL);
#if !GTK_CHECK_VERSION (2, 18, 0)
  gtk_about_dialog_set_url_hook (exo_gtk_url_about_dialog_hook, NULL, NULL);
#endif

  gtk_show_about_dialog (NULL,
                         "authors", authors,
                         "comments", _("The panel of the Xfce Desktop Environment"),
                         "copyright", "Copyright \302\251 2004-2010 Xfce Development Team",
                         "destroy-with-parent", TRUE,
                         "license", XFCE_LICENSE_GPL,
                         "program-name", PACKAGE_NAME,
                         "translator-credits", _("translator-credits"),
                         "version", PACKAGE_VERSION,
                         "website", "http://www.xfce.org/",
                         "logo-icon-name", PACKAGE_NAME,
                         NULL);

}



static void
panel_dialogs_choose_panel_combo_changed (GtkComboBox      *combo,
                                          PanelApplication *application)
{
  gint idx;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (GTK_IS_COMBO_BOX (combo));

  /* select active panel */
  idx = gtk_combo_box_get_active (combo);
  panel_application_window_select (application,
      panel_application_get_nth_window (application, idx));
}



gint
panel_dialogs_choose_panel (PanelApplication *application)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *combo;
  guint      i;
  gint       response = -1;
  gchar     *name;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), -1);

  /* setup the dialog */
  dialog = gtk_dialog_new_with_buttons (_("Add New Item"), NULL,
                                        GTK_DIALOG_NO_SEPARATOR,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_ADD, GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_ADD);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* create widgets */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Please choose a panel for the new plugin:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gtk_combo_box_new_text ();
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /* insert the panels */
  for (i = 0; i < panel_application_get_n_windows (application); i++)
    {
      name = g_strdup_printf (_("Panel %d"), i + 1);
      gtk_combo_box_append_text (GTK_COMBO_BOX (combo), name);
      g_free (name);
    }

  /* select first panel (changed will start marching ants) */
  g_signal_connect (G_OBJECT (combo), "changed",
       G_CALLBACK (panel_dialogs_choose_panel_combo_changed), application);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  /* run the dialog */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    response = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  gtk_widget_destroy (dialog);

  /* unset panel selections */
  panel_application_window_select (application, NULL);

  return response;
}



gboolean
panel_dialogs_kiosk_warning (void)
{
  PanelApplication *application;
  gboolean          locked;

  application = panel_application_get ();
  locked = panel_application_get_locked (application);
  g_object_unref (G_OBJECT (application));

  if (locked)
    {
      xfce_dialog_show_warning (NULL,
          _("Because the panel is running in kiosk mode, you are not allowed "
            "to make changes to the panel configuration as a regular user"),
          _("Modifying the panel is not allowed"));
    }

  return locked;
}
