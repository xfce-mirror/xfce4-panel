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
                                                        TasklistPlugin  *tasklist);
static void        tasklist_grouping_changed           (GtkComboBox     *cb,
                                                        TasklistPlugin  *tasklist);
static void        tasklist_expand_toggled             (GtkToggleButton *tb,
                                                        TasklistPlugin  *tasklist);
static void        tasklist_flat_buttons_toggled       (GtkToggleButton *tb,
                                                        TasklistPlugin  *tasklist);
static void        tasklist_show_handle_toggled        (GtkToggleButton *tb,
                                                        TasklistPlugin  *tasklist);
static void        tasklist_width_changed              (GtkSpinButton   *sb,
                                                        TasklistPlugin  *tasklist);
static void        tasklist_dialog_response            (GtkWidget       *dlg,
                                                        gint             reponse,
                                                        TasklistPlugin  *tasklist);



static void
tasklist_all_workspaces_toggled (GtkToggleButton *tb,
                                 TasklistPlugin  *tasklist)
{
    tasklist->all_workspaces = gtk_toggle_button_get_active (tb);

    wnck_tasklist_set_include_all_workspaces (WNCK_TASKLIST (tasklist->list),
                                              tasklist->all_workspaces);
}



static void
tasklist_grouping_changed (GtkComboBox    *cb,
                           TasklistPlugin *tasklist)
{
    tasklist->grouping = gtk_combo_box_get_active (cb);

    wnck_tasklist_set_grouping (WNCK_TASKLIST (tasklist->list),
                                tasklist->grouping);
}



static void
tasklist_expand_toggled (GtkToggleButton *tb,
                         TasklistPlugin  *tasklist)
{
    tasklist->expand = gtk_toggle_button_get_active (tb);

    xfce_panel_plugin_set_expand (tasklist->panel_plugin, tasklist->expand);
}



static void
tasklist_flat_buttons_toggled (GtkToggleButton *tb,
                               TasklistPlugin  *tasklist)
{
    tasklist->flat_buttons = gtk_toggle_button_get_active (tb);

    wnck_tasklist_set_button_relief (WNCK_TASKLIST (tasklist->list),
                                     tasklist->flat_buttons ?
                                        GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);
}



static void
tasklist_show_handle_toggled (GtkToggleButton *tb,
                              TasklistPlugin  *tasklist)
{
    tasklist->show_handles = gtk_toggle_button_get_active (tb);

    if (tasklist->show_handles)
        gtk_widget_show (tasklist->handle);
    else
        gtk_widget_hide (tasklist->handle);
}



static void
tasklist_fixed_width_toggled (GtkToggleButton *tb,
                              TasklistPlugin  *tasklist)
{
    tasklist->fixed_width = gtk_toggle_button_get_active (tb);

    gtk_widget_queue_resize (GTK_WIDGET (tasklist->panel_plugin));
}



static void
tasklist_width_changed (GtkSpinButton  *sb,
                        TasklistPlugin *tasklist)
{
    tasklist->width = gtk_spin_button_get_value_as_int (sb);

    gtk_widget_queue_resize (GTK_WIDGET (tasklist->panel_plugin));
}



static void
tasklist_width_sensitive (GtkToggleButton *tb,
                          GtkWidget       *sb)
{
    gtk_widget_set_sensitive (sb, gtk_toggle_button_get_active (tb));
}



static void
tasklist_dialog_response (GtkWidget       *dlg,
                          gint             reponse,
                          TasklistPlugin  *tasklist)
{
    g_object_set_data (G_OBJECT (tasklist->panel_plugin), I_("dialog"), NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (tasklist->panel_plugin);
    tasklist_plugin_write (tasklist);
}



void
tasklist_dialogs_configure (TasklistPlugin *tasklist)
{
    GtkWidget *dlg, *mainvbox, *vbox, *frame, *cb,
              *hbox, *label, *spin;

    xfce_panel_plugin_block_menu (tasklist->panel_plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("Task List"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg), gtk_widget_get_screen (GTK_WIDGET (tasklist->panel_plugin)));

    g_object_set_data (G_OBJECT (tasklist->panel_plugin), I_("dialog"), dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), GTK_STOCK_PROPERTIES);

    g_signal_connect (G_OBJECT (dlg), "response",
                      G_CALLBACK (tasklist_dialog_response), tasklist);

    mainvbox = gtk_vbox_new (FALSE, 6);
    gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 6);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), mainvbox,
                        TRUE, TRUE, 0);

    /* Size */
    vbox = gtk_vbox_new (FALSE, 6);

    frame = xfce_create_framebox_with_content (_("Appearance"), vbox);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic (_("Fi_xed length (pixels):"));
    gtk_box_pack_start (GTK_BOX (hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->fixed_width);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_fixed_width_toggled), tasklist);

    /* an arbitrary max of 4000 should be future proof, right? */
    spin = gtk_spin_button_new_with_range (100, 4000, 10);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), tasklist->width);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin);
    g_signal_connect (G_OBJECT (spin), "value-changed",
                      G_CALLBACK (tasklist_width_changed), tasklist);
    gtk_widget_set_sensitive (spin, tasklist->fixed_width);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_width_sensitive), spin);

    if (tasklist_using_xinerama (tasklist->panel_plugin))
    {
        cb = gtk_check_button_new_with_mnemonic (_("Use all available _space"));
        gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->expand);
        g_signal_connect (G_OBJECT (cb), "toggled",
                          G_CALLBACK (tasklist_expand_toggled), tasklist);
    }

    cb = gtk_check_button_new_with_mnemonic (_("Use _flat buttons"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->flat_buttons);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_flat_buttons_toggled), tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Show _handle"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->show_handles);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_show_handle_toggled), tasklist);

    /* Tasks */
    vbox = gtk_vbox_new (FALSE, 6);

    frame = xfce_create_framebox_with_content (_("Task List"), vbox);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic (_("Show tasks from _all workspaces"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), tasklist->all_workspaces);
    g_signal_connect (G_OBJECT (cb), "toggled",
                      G_CALLBACK (tasklist_all_workspaces_toggled), tasklist);

    cb = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Never group tasks"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Automatically group tasks"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Always group tasks"));

    /* keep order above in sync with WnckTasklistGroupingType */

    gtk_combo_box_set_active (GTK_COMBO_BOX (cb), tasklist->grouping);

    g_signal_connect (G_OBJECT (cb), "changed",
                      G_CALLBACK (tasklist_grouping_changed), tasklist);

    gtk_widget_show_all (dlg);
}
