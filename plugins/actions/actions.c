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

#include <signal.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef enum
{
    ACTION_QUIT,
    ACTION_LOCK,
    ACTION_QUIT_LOCK
}
ActionType;

static void actions_properties_dialog (XfcePanelPlugin *plugin);

static void actions_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (actions_construct);


/* Interface Implementation */

static void
actions_orientation_changed (XfcePanelPlugin *plugin, 
                             GtkOrientation orientation)
{
    int type = 
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin), "type"));

    if (type == ACTION_QUIT_LOCK)
    {
        GtkWidget *box;
        GList *children;

        box = orientation == GTK_ORIENTATION_HORIZONTAL ?
                gtk_vbox_new (TRUE, 0) : gtk_hbox_new (TRUE, 0);
        gtk_widget_show (box);

        children = gtk_container_get_children (
                GTK_CONTAINER (GTK_BIN (plugin)->child));

        gtk_widget_reparent (GTK_WIDGET (children->data), box);
        gtk_widget_reparent (GTK_WIDGET (children->next->data), box);

        g_list_free (children);
        
        gtk_widget_destroy (GTK_BIN (plugin)->child);
        
        gtk_container_add (GTK_CONTAINER (plugin), box);
    }
}

static gboolean 
actions_set_size (XfcePanelPlugin *plugin, int size)
{
    int width = MAX (size / 2, 
                     16 + 2 + 2 * GTK_WIDGET (plugin)->style->xthickness);
    
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     width, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     size, width);
    }

    return TRUE;
}

static void
actions_read_rc_file (XfcePanelPlugin *plugin)
{
    char *file;
    XfceRc *rc;
    int type = ACTION_QUIT;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            type = xfce_rc_read_int_entry (rc, "type", ACTION_QUIT);
            
            xfce_rc_close (rc);
        }
    }

    g_object_set_data (G_OBJECT (plugin), "type", GINT_TO_POINTER (type));
}

static void
actions_write_rc_file (XfcePanelPlugin *plugin)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "type", 
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin), "type")));

    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static void
actions_do_quit (GtkWidget *b, XfcePanelPlugin *plugin)
{
    /* this only works with internal plugins */
    raise (SIGUSR2);
}

static void
actions_do_lock (GtkWidget *b, XfcePanelPlugin *plugin)
{
    g_spawn_command_line_async ("xflock4", NULL);
}

static void
actions_create_widgets (XfcePanelPlugin *plugin)
{
    GtkWidget *widget, *box, *button;
    GdkPixbuf *pb;
    int size = xfce_panel_plugin_get_size (plugin);
    
    widget = GTK_WIDGET (plugin);
    
    switch (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin), "type")))
    {
        case ACTION_QUIT_LOCK:
            box = (xfce_panel_plugin_get_orientation (plugin) == 
                    GTK_ORIENTATION_HORIZONTAL) ? 
                  gtk_vbox_new (TRUE, 0) : gtk_hbox_new (TRUE, 0);
            gtk_widget_show (box);
            gtk_container_add (GTK_CONTAINER (plugin), box);

            pb = xfce_themed_icon_load ("xfce-system-lock", 16);
            button = xfce_iconbutton_new_from_pixbuf (pb);
            gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
            gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
            gtk_widget_show (button);
            gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
            g_object_unref (pb);

            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_lock), plugin);
            
            pb = xfce_themed_icon_load ("xfce-system-exit", 16);
            button = xfce_iconbutton_new_from_pixbuf (pb);
            gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
            gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
            gtk_widget_show (button);
            gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
            g_object_unref (pb);

            xfce_panel_plugin_add_action_widget (plugin, button);

            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_quit), plugin);
            
            break;
        case ACTION_LOCK:
            pb = xfce_themed_icon_load ("xfce-system-lock", 16);
            button = xfce_iconbutton_new_from_pixbuf (pb);
            gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
            gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
            gtk_widget_show (button);
            gtk_container_add (GTK_CONTAINER (plugin), button);
            g_object_unref (pb);
            
            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_lock), plugin);
            
            break;
        default:
            pb = xfce_themed_icon_load ("xfce-system-exit", 16);
            button = xfce_iconbutton_new_from_pixbuf (pb);
            gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
            gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
            gtk_widget_show (button);
            gtk_container_add (GTK_CONTAINER (plugin), button);
            g_object_unref (pb);
            
            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_quit), plugin);
    }

    actions_set_size (plugin, size);
}

static void 
actions_construct (XfcePanelPlugin *plugin)
{
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (actions_orientation_changed), NULL);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (actions_set_size), NULL);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (actions_write_rc_file), NULL);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (actions_properties_dialog), NULL);

    actions_read_rc_file (plugin);

    actions_create_widgets (plugin);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
action_type_changed (GtkComboBox *box, XfcePanelPlugin *plugin)
{
    g_object_set_data (G_OBJECT (plugin), "type", 
                       GINT_TO_POINTER (gtk_combo_box_get_active (box)));

    gtk_widget_destroy (GTK_BIN (plugin)->child);
    actions_create_widgets (plugin);
}

static void
actions_dialog_response (GtkWidget *dlg, int reponse, 
                          XfcePanelPlugin *plugin)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (plugin);
    actions_write_rc_file (plugin);
}

static void
actions_properties_dialog (XfcePanelPlugin *plugin)
{
    GtkWidget *dlg, *header, *vbox, *hbox, *label, *box;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_signal_connect (dlg, "response", G_CALLBACK (actions_dialog_response),
                      plugin);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Panel Actions"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, 200, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), 6);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);
    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox,
                        TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Select action type:"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    box = gtk_combo_box_new_text ();
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Quit"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Lock screen"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Quit + Lock screen"));
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (box), 
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin), "type")));
    
    g_signal_connect (box, "changed", G_CALLBACK (action_type_changed), 
                      plugin);
    
    gtk_widget_show (dlg);
}

