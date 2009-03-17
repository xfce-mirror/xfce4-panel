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
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>

typedef enum
{
    ACTION_QUIT,
    ACTION_LOCK,
    ACTION_QUIT_LOCK
}
ActionType;

typedef struct
{
    XfcePanelPlugin *plugin;
    GtkTooltips *tips;
    
    ActionType type;
    GtkWidget *button1;
    GtkWidget *image1;
    GtkWidget *button2;
    GtkWidget *image2;
    
    GtkWidget *box;

    int screen_id;
    int style_id;
    
    GtkOrientation orientation;
}
Action;

static void actions_properties_dialog (XfcePanelPlugin *plugin, Action *action);
static gboolean actions_set_size (XfcePanelPlugin *plugin, int size, Action *action);
static void actions_create_widgets (XfcePanelPlugin *plugin, Action *action);
static void actions_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (actions_construct);


/* Interface Implementation */

static void
actions_orientation_changed (XfcePanelPlugin *plugin, 
                             GtkOrientation orientation,
                             Action *action)
{
    gtk_widget_destroy (GTK_BIN (plugin)->child);
    actions_create_widgets (plugin, action);
    actions_set_size (plugin, xfce_panel_plugin_get_size (plugin), action);
}

static const char *action_icon_names[2][2] = {
    { "xfce-system-exit", "system-log-out" },
    { "xfce-system-lock", "system-lock-screen" }
};

static GdkPixbuf *
actions_load_icon (ActionType type, int size)
{
    GdkPixbuf *pb = NULL;
    
    /* first try name from icon nameing spec... */
    pb = xfce_themed_icon_load (action_icon_names[type][1], size);
    
    /* ...then try xfce name, if necessary */
    if (!pb)
    {
        pb = xfce_themed_icon_load (action_icon_names[type][0], size);
    }

    return pb;
}

static gboolean 
actions_set_size (XfcePanelPlugin *plugin, int size, Action *action)
{
    gint       width;
    GdkPixbuf *pb = NULL;
    
    width = size - 2 - 2 * MAX (action->button1->style->xthickness,
                                action->button1->style->ythickness);
    
    switch (action->type)
    {
        case ACTION_QUIT_LOCK:
            if (xfce_panel_plugin_get_orientation (plugin) != action->orientation)
            {
                width = (size / 2) - 4 - 4 * MAX (action->button1->style->xthickness,
                                                  action->button1->style->ythickness);
                width = MAX (width, 5);
            }
            else
            {
                gtk_widget_set_size_request (GTK_WIDGET (action->button1), size, size);
                gtk_widget_set_size_request (GTK_WIDGET (action->button2), size, size);
            }
             
            gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, -1);  
                
            pb = actions_load_icon (ACTION_LOCK, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image1), pb);
            g_object_unref (G_OBJECT (pb));
            
            pb = actions_load_icon (ACTION_QUIT, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image2), pb);
            g_object_unref (G_OBJECT (pb));           
            break;
            
        case ACTION_QUIT:
        case ACTION_LOCK:
            pb = actions_load_icon (action->type, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image1), pb);
            g_object_unref (G_OBJECT (pb));
            gtk_widget_set_size_request (GTK_WIDGET (plugin), size, size);
            break;
    }
            
    return TRUE;
}

static void
actions_read_rc_file (XfcePanelPlugin *plugin, Action *action)
{
    char *file;
    XfceRc *rc;
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
    int type = ACTION_QUIT;
       
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            type = xfce_rc_read_int_entry (rc, "type", type);
            orientation = (xfce_rc_read_int_entry (rc, "orientation", orientation) == 0 ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
            
            xfce_rc_close (rc);
        }
    }

    action->type = type;
    action->orientation = orientation;
}

static void
actions_write_rc_file (XfcePanelPlugin *plugin, Action *action)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "type", action->type);
    xfce_rc_write_int_entry (rc, "orientation", action->orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 1);

    xfce_rc_close (rc);
}

static void
actions_free_data (XfcePanelPlugin *plugin, Action *action)
{
    GtkWidget *dlg;

    if (action->screen_id)
        g_signal_handler_disconnect (plugin, action->screen_id);
    
    if (action->style_id)
        g_signal_handler_disconnect (plugin, action->style_id);
    
    action->screen_id = action->style_id = 0;

    dlg = g_object_get_data (G_OBJECT (plugin), "dialog");
    if (dlg)
        gtk_widget_destroy (dlg);

    gtk_object_sink (GTK_OBJECT (action->tips));

    panel_slice_free (Action, action);
}

/* create widgets and connect to signals */
static void
actions_do_quit (GtkWidget *b, XfcePanelPlugin *plugin)
{
    g_spawn_command_line_async ("xfce4-panel -q", NULL);
}

static void
actions_do_lock (GtkWidget *b, XfcePanelPlugin *plugin)
{
    g_spawn_command_line_async ("xflock4", NULL);
}

static void
actions_create_widgets (XfcePanelPlugin *plugin, Action *action)
{
    GtkWidget *widget, *box, *button, *img;
    
    widget = GTK_WIDGET (plugin);

    switch (action->type)
    {
        case ACTION_QUIT_LOCK:
            box = xfce_hvbox_new (action->orientation, TRUE, 0);
            gtk_widget_show (box);
            gtk_container_add (GTK_CONTAINER (plugin), box);

            action->button1 = button = xfce_create_panel_button ();
            gtk_widget_show (button);
            gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_lock), plugin);
    
            gtk_tooltips_set_tip (action->tips, action->button1, _("Lock screen"), NULL);

            img = action->image1 = gtk_image_new ();
            gtk_widget_show (img);
            gtk_container_add (GTK_CONTAINER (button), img);
            
            action->button2 = button = xfce_create_panel_button ();
            gtk_widget_show (button);
            gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

            xfce_panel_plugin_add_action_widget (plugin, button);

            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_quit), plugin);
            
            img = action->image2 = gtk_image_new ();
            gtk_widget_show (img);
            gtk_container_add (GTK_CONTAINER (button), img);
            
            gtk_tooltips_set_tip (action->tips, action->button2, _("Quit"), NULL);

            break;
        case ACTION_LOCK:
            action->button1 = button = xfce_create_panel_button ();
            gtk_widget_show (button);
            gtk_container_add (GTK_CONTAINER (plugin), button);
            
            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_lock), plugin);
            
            img = action->image1 = gtk_image_new ();
            gtk_widget_show (img);
            gtk_container_add (GTK_CONTAINER (button), img);
            
            gtk_tooltips_set_tip (action->tips, action->button1, _("Lock screen"), NULL);

            break;
        default:
            action->button1 = button = xfce_create_panel_button ();
            gtk_widget_show (button);
            gtk_container_add (GTK_CONTAINER (plugin), button);
            
            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_quit), plugin);
            
            img = action->image1 = gtk_image_new ();
            gtk_widget_show (img);
            gtk_container_add (GTK_CONTAINER (button), img);

            gtk_tooltips_set_tip (action->tips, action->button1, _("Quit"), NULL);
    }
}

static void
actions_icontheme_changed (XfcePanelPlugin *plugin, gpointer ignored,
                           Action *action)
{
    actions_set_size (plugin, xfce_panel_plugin_get_size (plugin), action);
}

static void 
actions_construct (XfcePanelPlugin *plugin)
{
    Action *action = panel_slice_new0 (Action);

    action->plugin = plugin;

    action->tips = gtk_tooltips_new();
    
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (actions_orientation_changed), action);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (actions_set_size), action);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (actions_write_rc_file), action);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (actions_free_data), action);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (actions_properties_dialog), action);

    actions_read_rc_file (plugin, action);

    actions_create_widgets (plugin, action);
    
    action->style_id =
        g_signal_connect (plugin, "style-set", 
                          G_CALLBACK (actions_icontheme_changed), action);
    
    action->screen_id =
        g_signal_connect (plugin, "screen-changed", 
                          G_CALLBACK (actions_icontheme_changed), action);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
action_type_changed (GtkComboBox *box, Action *action)
{
    action->type = gtk_combo_box_get_active (box);
    
    /* orientation only sensitive when 2 buttons are shown */
    gtk_widget_set_sensitive (action->box, action->type == 2 ? TRUE : FALSE);

    gtk_widget_destroy (GTK_BIN (action->plugin)->child);
    actions_create_widgets (action->plugin, action);

    actions_set_size (action->plugin, 
                      xfce_panel_plugin_get_size (action->plugin), action);
}

static void
orientation_changed (GtkComboBox *box, Action *action)
{
    action->orientation = (gtk_combo_box_get_active (box) == 0 ? 
                             GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
                             
    gtk_widget_destroy (GTK_BIN (action->plugin)->child);
    actions_create_widgets (action->plugin, action);

    actions_set_size (action->plugin, 
                      xfce_panel_plugin_get_size (action->plugin), action);
}

static void
actions_dialog_response (GtkWidget *dlg, int reponse, Action *action)
{
    g_object_set_data (G_OBJECT (action->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (action->plugin);
    actions_write_rc_file (action->plugin, action);
}

static void
actions_properties_dialog (XfcePanelPlugin *plugin, Action *action)
{
    GtkWidget *dlg, *vbox, *hbox, *label, *box, *box2;
    GtkSizeGroup *sg;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = xfce_titled_dialog_new_with_buttons (_("Action Buttons"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg), 
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), GTK_STOCK_PROPERTIES);
    
    g_signal_connect (dlg, "response", G_CALLBACK (actions_dialog_response),
                      action);
    
    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox,
                        TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Select action type:"));
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    box = gtk_combo_box_new_text ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), box);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Quit"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Lock screen"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (box), _("Quit + Lock screen"));
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (box), action->type);
    
    g_signal_connect (box, "changed", G_CALLBACK (action_type_changed), 
                      action);
                      
    hbox = gtk_hbox_new (FALSE, 12);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    label = gtk_label_new_with_mnemonic (_("_Orientation:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    box2 = action->box = gtk_combo_box_new_text ();
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), box2);
    gtk_widget_show (box2);
    gtk_box_pack_start (GTK_BOX (hbox), box2, TRUE, TRUE, 0);
    
    /* only sensitive when 2 buttons are shown */
    gtk_widget_set_sensitive (action->box, action->type == 2 ? TRUE : FALSE);
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (box2), _("Horizontal"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (box2), _("Vertical"));
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (box2), 
        action->orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 1);
    
    g_signal_connect (box2, "changed", G_CALLBACK (orientation_changed), 
                      action);
    
    g_object_unref (G_OBJECT (sg));
    
    gtk_widget_show (dlg);
}

