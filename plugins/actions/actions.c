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
    
    ActionType type;
    GtkWidget *button1;
    GtkWidget *image1;
    GtkWidget *button2;
    GtkWidget *image2;

    int screen_id;
    int style_id;
}
Action;

static void actions_properties_dialog (XfcePanelPlugin *plugin, Action *action);

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
    if (action->type == ACTION_QUIT_LOCK)
    {
        xfce_hvbox_set_orientation (XFCE_HVBOX (GTK_BIN (plugin)->child), 
                (orientation == GTK_ORIENTATION_HORIZONTAL) ?
                        GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
    }
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
    int width;
    GdkPixbuf *pb = NULL;
    int border;
    
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        border = 2 + 2 * GTK_WIDGET(plugin)->style->ythickness;
    }
    else
    {
        border = 2 + 2 * GTK_WIDGET(plugin)->style->xthickness;
    }
    
    width = MIN(size - border, MAX(16, size/2 - border));

    switch (action->type)
    {
        case ACTION_QUIT_LOCK:
            pb = actions_load_icon (ACTION_LOCK, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image1), pb);
            g_object_unref (pb);
            
            pb = actions_load_icon (ACTION_QUIT, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image2), pb);
            g_object_unref (pb);
            
            break;
        case ACTION_QUIT:
        case ACTION_LOCK:
            pb = actions_load_icon (action->type, width);
            gtk_image_set_from_pixbuf (GTK_IMAGE (action->image1), pb);
            g_object_unref (pb);
            
            break;
    }
            
    return TRUE;
}

static void
actions_read_rc_file (XfcePanelPlugin *plugin, Action *action)
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

    action->type = type;
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

    g_free (action);
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
actions_create_widgets (XfcePanelPlugin *plugin, Action *action)
{
    GtkWidget *widget, *box, *button, *img;
    GtkOrientation orientation;
    
    widget = GTK_WIDGET (plugin);

    switch (action->type)
    {
        case ACTION_QUIT_LOCK:
            orientation = (xfce_panel_plugin_get_orientation (plugin) == 
                           GTK_ORIENTATION_HORIZONTAL) ?
                                GTK_ORIENTATION_VERTICAL : 
                                GTK_ORIENTATION_HORIZONTAL;

            box = xfce_hvbox_new (orientation, TRUE, 0);
            gtk_widget_show (box);
            gtk_container_add (GTK_CONTAINER (plugin), box);

            action->button1 = button = xfce_create_panel_button ();
            gtk_widget_show (button);
            gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

            xfce_panel_plugin_add_action_widget (plugin, button);
            
            g_signal_connect (button, "clicked", 
                              G_CALLBACK (actions_do_lock), plugin);
    
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
    Action *action = g_new0 (Action, 1);

    action->plugin = plugin;
    
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
    GtkWidget *dlg, *header, *vbox, *hbox, *label, *box;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect (dlg, "response", G_CALLBACK (actions_dialog_response),
                      action);

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
    
    gtk_combo_box_set_active (GTK_COMBO_BOX (box), action->type);
    
    g_signal_connect (box, "changed", G_CALLBACK (action_type_changed), 
                      action);
    
    gtk_widget_show (dlg);
}

