/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tasklist.h"
#include "tasklist-dialogs.h"



/* prototypes */
static void        tasklist_all_workspaces_toggled     (GtkToggleButton *tb,
                                                        Tasklist        *tasklist);
static void        tasklist_grouping_changed           (GtkComboBox     *cb,
                                                        Tasklist        *tasklist);
static void        tasklist_show_label_toggled         (GtkToggleButton *tb,
                                                        Tasklist        *tasklist);
static void        tasklist_expand_toggled             (GtkToggleButton *tb,
                                                        Tasklist        *tasklist);
static void        tasklist_flat_buttons_toggled       (GtkToggleButton *tb,
                                                       Tasklist         *tasklist);
static void        tasklist_show_handle_toggled        (GtkToggleButton *tb,
                                                       Tasklist         *tasklist);
static void        tasklist_width_changed              (GtkSpinButton   *sb,
                                                        Tasklist        *tasklist);
static void        tasklist_dialog_response            (GtkWidget       *dlg,
                                                        gint             reponse,
                                                        Tasklist        *tasklist);



static void
tasklist_all_workspaces_toggled (GtkToggleButton *tb,
                                 Tasklist        *tasklist)
{
    tasklist->all_workspaces = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->list),
                                              tasklist->all_workspaces);
}



static void
tasklist_grouping_changed (GtkComboBox *cb,
                           Tasklist    *tasklist)
{
    tasklist->grouping = gtk_combo_box_get_active (cb);

    netk_tasklist_set_grouping (NETK_TASKLIST (tasklist->list),
                                tasklist->grouping);
}



static void
tasklist_show_label_toggled (GtkToggleButton *tb,
                             Tasklist        *tasklist)
{
    tasklist->show_label = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_show_label (NETK_TASKLIST (tasklist->list),
                                  tasklist->show_label);
}



static void
tasklist_expand_toggled (GtkToggleButton *tb,
                         Tasklist        *tasklist)
{
    tasklist->expand = gtk_toggle_button_get_active (tb);

    xfce_panel_plugin_set_expand (tasklist->plugin, tasklist->expand);
}



static void
tasklist_flat_buttons_toggled (GtkToggleButton *tb,
                               Tasklist        *tasklist)
{
    tasklist->flat_buttons = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_button_relief (NETK_TASKLIST (tasklist->list),
                                     tasklist->flat_buttons ?
                                        GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);
}



static void
tasklist_show_handle_toggled (GtkToggleButton *tb,
                              Tasklist        *tasklist)
{
    tasklist->show_handles = gtk_toggle_button_get_active (tb);

    if (tasklist->show_handles)
    	gtk_widget_show (tasklist->handle);
    else
    	gtk_widget_hide (tasklist->handle);
}



static void
tasklist_width_changed (GtkSpinButton *sb,
                        Tasklist      *tasklist)
{
    tasklist->width = gtk_spin_button_get_value_as_int (sb);

    tasklist_set_size (tasklist, xfce_panel_plugin_get_size (tasklist->plugin));
}



static void
tasklist_dialog_response (GtkWidget *dlg,
                          gint       reponse,
                          Tasklist  *tasklist)
{
    g_object_set_data (G_OBJECT (tasklist->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (tasklist->plugin);
    tasklist_write_rc_file (tasklist);
}



void
tasklist_properties_dialog (Tasklist *tasklist)
{
    GtkWidget *dlg, *mainvbox, *vbox, *frame, *cb,
              *hbox, *label, *spin;

    xfce_panel_plugin_block_menu (tasklist->plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("Task List"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (tasklist->plugin)));

    g_object_set_data (G_OBJECT (tasklist->plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");

    g_signal_connect (G_OBJECT (dlg), "response",
                      G_CALLBACK (tasklist_dialog_response), tasklist);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    mainvbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 5);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), mainvbox,
                        TRUE, TRUE, 0);

    /* Size */
    vbox = gtk_vbox_new (FALSE, 8);

    frame = xfce_create_framebox_with_content (_("Appearance"), vbox);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Minimum Width:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    /* an arbitrary max of 4000 should be future proof, right? */
    spin = gtk_spin_button_new_with_range (100, 4000, 10);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), tasklist->width);
    g_signal_connect (G_OBJECT (spin), "value-changed",
                      G_CALLBACK (tasklist_width_changed), tasklist);

    if (tasklist_using_xinerama (tasklist->plugin))
    {
        cb = gtk_check_button_new_with_mnemonic (_("Use all available space"));
        gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->expand);
        g_signal_connect (G_OBJECT (cb), "toggled",
                          G_CALLBACK (tasklist_expand_toggled), tasklist);
    }

    cb = gtk_check_button_new_with_mnemonic (_("Use flat buttons"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->flat_buttons);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_flat_buttons_toggled), tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Show handle"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->show_handles);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_show_handle_toggled), tasklist);

    /* Tasks */
    vbox = gtk_vbox_new (FALSE, 8);

    frame = xfce_create_framebox_with_content (_("Task List"), vbox);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic (_("Show tasks from _all workspaces"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->all_workspaces);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_all_workspaces_toggled), tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Show application _names"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->show_label);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_show_label_toggled), tasklist);

    cb = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);

    /* keep order in sync with NetkTasklistGroupingType */
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Never group tasks"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Automatically group tasks"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Always group tasks"));

    gtk_combo_box_set_active (GTK_COMBO_BOX (cb), tasklist->grouping);

    g_signal_connect (G_OBJECT (cb), "changed",
                      G_CALLBACK (tasklist_grouping_changed), tasklist);

    gtk_widget_show_all (dlg);
}
