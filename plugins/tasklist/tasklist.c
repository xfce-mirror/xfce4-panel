/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget *box;
    GtkWidget *handle;
    GtkWidget *handle2;
    GtkWidget *list;

    int screen_changed_id;
    
    NetkTasklistGroupingType grouping;
    gboolean all_workspaces;
    gboolean show_label;

    gboolean expand;
    int width;
}
Tasklist;

static void tasklist_properties_dialog (XfcePanelPlugin *plugin, 
                                        Tasklist *tasklist);

static void tasklist_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (tasklist_construct);


/* Interface Implementation */

static void
tasklist_orientation_changed (XfcePanelPlugin *plugin, 
                              GtkOrientation orientation, 
                              Tasklist *tasklist)
{
    GtkWidget *box;

    box = orientation == GTK_ORIENTATION_HORIZONTAL ?
            gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0);
    gtk_container_set_reallocate_redraws (GTK_CONTAINER (box), TRUE);
    gtk_widget_show (box);

    gtk_widget_reparent (tasklist->handle, box);
    gtk_box_set_child_packing (GTK_BOX (box), tasklist->handle,
                               FALSE, FALSE, 0, GTK_PACK_START);
    gtk_widget_reparent (tasklist->list, box);
    gtk_widget_reparent (tasklist->handle2, box);
    gtk_box_set_child_packing (GTK_BOX (box), tasklist->handle2,
                               FALSE, FALSE, 0, GTK_PACK_START);

    gtk_widget_destroy (tasklist->box);
    tasklist->box = box;
    
    gtk_container_add (GTK_CONTAINER (plugin), box);

    gtk_widget_queue_draw (tasklist->handle);
    gtk_widget_queue_draw (tasklist->handle2);
}

static gboolean 
tasklist_set_size (XfcePanelPlugin *plugin, int size, Tasklist *tasklist)
{
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     tasklist->width, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     size, tasklist->width);
    }

    return TRUE;
}

static void
tasklist_free_data (XfcePanelPlugin *plugin, Tasklist *tasklist)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);
    
    g_signal_handler_disconnect (plugin, tasklist->screen_changed_id);
    g_free (tasklist);
}

static void
tasklist_read_rc_file (XfcePanelPlugin *plugin, Tasklist *tasklist)
{
    char *file;
    XfceRc *rc;
    int grouping, all_workspaces, labels, expand, width;
    
    grouping = NETK_TASKLIST_AUTO_GROUP;
    all_workspaces = 0;
    labels = 1;
    expand = 1;
    width = 300;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            grouping = xfce_rc_read_int_entry (rc, "grouping", grouping);
            
            all_workspaces = xfce_rc_read_int_entry (rc, "all_workspaces", 
                                                     all_workspaces);
            
            labels = xfce_rc_read_int_entry (rc, "show_label", labels);
            
            expand = xfce_rc_read_int_entry (rc, "expand", expand);
            
            width = xfce_rc_read_int_entry (rc, "width", width);
            
            xfce_rc_close (rc);
        }
    }

    tasklist->grouping = grouping;
    tasklist->all_workspaces = (all_workspaces == 1);
    tasklist->show_label = (labels != 0);
    tasklist->expand = (expand != 0);
    tasklist->width = MAX (100, width);
}

static void
tasklist_write_rc_file (XfcePanelPlugin *plugin, Tasklist *tasklist)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "grouping", tasklist->grouping);

    xfce_rc_write_int_entry (rc, "all_workspaces", tasklist->all_workspaces);

    xfce_rc_write_int_entry (rc, "show_label", tasklist->show_label);

    xfce_rc_write_int_entry (rc, "expand", tasklist->expand);

    xfce_rc_write_int_entry (rc, "width", tasklist->width);

    xfce_rc_close (rc);
}

static void 
tasklist_realize (XfcePanelPlugin *plugin, Tasklist *tasklist)
{
    GdkScreen *screen;
    int screen_idx;

    screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
    screen_idx = gdk_screen_get_number (screen);
    
    netk_tasklist_set_screen (NETK_TASKLIST (tasklist->list), 
                              netk_screen_get (screen_idx));
}

/* create widgets and connect to signals */
static gboolean
handle_expose (GtkWidget *widget, GdkEventExpose *ev, Tasklist *tasklist)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        GtkAllocation *allocation = &(widget->allocation);
        int x, y, w, h;

        x = allocation->x + widget->style->xthickness;
        y = allocation->y + widget->style->ythickness;
        w = allocation->width - 2 * widget->style->xthickness;
        h = allocation->height - 2 * widget->style->ythickness;

        gtk_paint_box (widget->style, widget->window, 
                       GTK_WIDGET_STATE (widget),
                       GTK_SHADOW_OUT,
                       &(ev->area), widget, "xfce-panel",
                       x, y, w, h);        

        return TRUE;
    }

    return FALSE;
}

static void
tasklist_screen_changed (GtkWidget *plugin, GdkScreen *screen, 
                         Tasklist *tasklist)
{
    if (!screen)
        return;

    netk_tasklist_set_screen (NETK_TASKLIST (tasklist->list), 
            netk_screen_get (gdk_screen_get_number (screen)));
}

static void 
tasklist_construct (XfcePanelPlugin *plugin)
{
    Tasklist *tasklist = g_new0 (Tasklist, 1);

    tasklist->plugin = plugin;
    
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (tasklist_orientation_changed), tasklist);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (tasklist_set_size), tasklist);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (tasklist_free_data), tasklist);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (tasklist_write_rc_file), tasklist);

    g_signal_connect (plugin, "realize",
		      G_CALLBACK (tasklist_realize), tasklist);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (tasklist_properties_dialog), tasklist);

    tasklist_read_rc_file (plugin, tasklist);

    xfce_panel_plugin_set_expand (plugin, tasklist->expand);
    
    tasklist->box = (xfce_panel_plugin_get_orientation (plugin) == 
                        GTK_ORIENTATION_HORIZONTAL) ?
                    gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0);
    gtk_container_set_reallocate_redraws (GTK_CONTAINER (tasklist->box), TRUE);
    gtk_widget_show (tasklist->box);
    gtk_container_add (GTK_CONTAINER (plugin), tasklist->box);

    tasklist->handle = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (tasklist->handle, 6, 6);
    gtk_widget_show (tasklist->handle);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->handle, 
                        FALSE, FALSE, 0);

    xfce_panel_plugin_add_action_widget (plugin, tasklist->handle);

    g_signal_connect (tasklist->handle, "expose-event", 
                      G_CALLBACK (handle_expose), tasklist);

    tasklist->list = netk_tasklist_new (netk_screen_get_default ());
    gtk_widget_show (tasklist->list);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->list, 
                        TRUE, TRUE, 0);
            
    tasklist->handle2 = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (tasklist->handle2, 6, 6);
    gtk_widget_show (tasklist->handle2);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->handle2, 
                        FALSE, FALSE, 0);

    xfce_panel_plugin_add_action_widget (plugin, tasklist->handle2);

    g_signal_connect (tasklist->handle2, "expose-event", 
                      G_CALLBACK (handle_expose), tasklist);

    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->list), 
                                              tasklist->all_workspaces);

    netk_tasklist_set_grouping (NETK_TASKLIST (tasklist->list),
                                tasklist->grouping);

    netk_tasklist_set_show_label (NETK_TASKLIST (tasklist->list),
                                  tasklist->show_label);

    tasklist->screen_changed_id = 
        g_signal_connect (plugin, "screen-changed", 
                          G_CALLBACK (tasklist_screen_changed), tasklist);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
all_workspaces_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->all_workspaces = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->list),
                                              tasklist->all_workspaces);
}

static void
grouping_changed (GtkComboBox *cb, Tasklist *tasklist)
{
    tasklist->grouping = gtk_combo_box_get_active (cb);

    netk_tasklist_set_grouping (NETK_TASKLIST (tasklist->list),
                                tasklist->grouping);
}

static void
show_label_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->show_label = gtk_toggle_button_get_active (tb);

    netk_tasklist_set_show_label (NETK_TASKLIST (tasklist->list),
                                  tasklist->show_label);
}

static void
expand_toggled (GtkToggleButton *tb, Tasklist *tasklist)
{
    tasklist->expand = gtk_toggle_button_get_active (tb);

    xfce_panel_plugin_set_expand (tasklist->plugin, tasklist->expand);
}

static void
width_changed (GtkSpinButton *sb, Tasklist *tasklist)
{
    tasklist->width = gtk_spin_button_get_value_as_int (sb);

    tasklist_set_size (tasklist->plugin, 
                       xfce_panel_plugin_get_size (tasklist->plugin), 
                       tasklist);
}

static void
tasklist_dialog_response (GtkWidget *dlg, int reponse, 
                          Tasklist *tasklist)
{
    g_object_set_data (G_OBJECT (tasklist->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (tasklist->plugin);
    tasklist_write_rc_file (tasklist->plugin, tasklist);
}

static void
tasklist_properties_dialog (XfcePanelPlugin *plugin, Tasklist *tasklist)
{
    GtkWidget *dlg, *header, *mainvbox, *vbox, *frame, *cb, 
              *hbox, *label, *spin;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect (dlg, "response", G_CALLBACK (tasklist_dialog_response),
                      tasklist);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Task List"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, 200, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), 6);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);
    
    mainvbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 5);
    gtk_widget_show (mainvbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), mainvbox,
                        TRUE, TRUE, 0);

    /* Size */
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);

    frame = xfce_create_framebox_with_content (_("Size"), vbox);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    label = gtk_label_new (_("Width:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    /* an arbitrary max of 4000 should be future proof, right? */
    spin = gtk_spin_button_new_with_range (100, 4000, 10);
    gtk_widget_show (spin);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), tasklist->width);
    g_signal_connect (spin, "value-changed", G_CALLBACK (width_changed),
                      tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Expand"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->expand);
    g_signal_connect (cb, "toggled", G_CALLBACK (expand_toggled),
                      tasklist);

    /* Tasks */
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);

    frame = xfce_create_framebox_with_content (_("Task List"), vbox);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (mainvbox), frame, FALSE, FALSE, 0);
    
    cb = gtk_check_button_new_with_mnemonic (_("Show tasks "
                                               "from _all workspaces"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->all_workspaces);

    g_signal_connect (cb, "toggled", G_CALLBACK (all_workspaces_toggled),
                      tasklist);

    cb = gtk_check_button_new_with_mnemonic (_("Show application _names"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  tasklist->show_label);
    g_signal_connect (cb, "toggled", G_CALLBACK (show_label_toggled),
                      tasklist);

    cb = gtk_combo_box_new_text ();
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
        
    /* keep order in sync with NetkTasklistGroupingType */
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), 
                               _("Never group tasks"));
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), 
                               _("Automatically group tasks"));
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), 
                               _("Always group tasks"));

    gtk_combo_box_set_active (GTK_COMBO_BOX (cb), tasklist->grouping);
    
    g_signal_connect (cb, "changed", G_CALLBACK (grouping_changed),
                      tasklist);
  
    gtk_widget_show (dlg);
}

