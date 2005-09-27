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

    XfceSystemTray *tray;
    gboolean tray_registered;

    GtkWidget *frame;
    GtkWidget *align;
    GtkWidget *iconbox;

    gboolean show_frame;
}
Systray;

static void systray_properties_dialog (XfcePanelPlugin *plugin, 
                                       Systray *systray);

static void systray_construct (XfcePanelPlugin *plugin);


/* -------------------------------------------------------------------- *
 *                             Systray                                  *
 * -------------------------------------------------------------------- */

static gboolean
register_tray (Systray * systray)
{
    GError *error = NULL;
    Screen *scr = GDK_SCREEN_XSCREEN (gtk_widget_get_screen (systray->frame));

    if (xfce_system_tray_check_running (scr))
    {
        xfce_info (_("There is already a system tray running on this "
                     "screen"));
        return FALSE;
    }
    else if (!xfce_system_tray_register (systray->tray, scr, &error))
    {
        xfce_err (_("Unable to register system tray: %s"), error->message);
        g_error_free (error);

        return FALSE;
    }

    return TRUE;
}

static void
icon_docked (XfceSystemTray * tray, GtkWidget * icon, Systray * systray)
{
    if (systray->tray_registered)
    {
        gtk_widget_hide (systray->iconbox);
        gtk_box_pack_start (GTK_BOX (systray->iconbox), icon, 
                            FALSE, FALSE, 0);
        gtk_widget_show (icon);
        gtk_widget_show (systray->iconbox);
        gtk_widget_queue_draw (systray->iconbox);
    }
}

static void
icon_undocked (XfceSystemTray * tray, GtkWidget * icon, Systray *systray)
{
    if (systray->tray_registered)
    {
        gtk_widget_hide (systray->iconbox);
        gtk_container_remove (GTK_CONTAINER (systray->iconbox), icon);
        gtk_widget_show (systray->iconbox);
        gtk_widget_queue_draw (systray->iconbox);
    }
}

static void
message_new (XfceSystemTray * tray, GtkWidget * icon, glong id,
             glong timeout, const gchar * text)
{
    g_print ("++ notification: %s\n", text);
}

static void
systray_start (Systray *systray)
{
    if (!systray->tray_registered)
    {
        systray->tray_registered = register_tray (systray);
    }
}

static void
systray_stop (Systray *systray)
{
    if (systray->tray_registered)
    {
        xfce_system_tray_unregister (systray->tray);
        systray->tray_registered = FALSE;
    }
}

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (systray_construct);


/* Interface Implementation */

static void
systray_orientation_changed (XfcePanelPlugin *plugin, 
                              GtkOrientation orientation, 
                              Systray *systray)
{
    systray_stop (systray);
    
    gtk_widget_destroy (systray->iconbox);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        systray->iconbox =  gtk_hbox_new (TRUE, 3);
        
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align), 
                                   0, 0, 3, 3);
    }
    else
    {
        systray->iconbox =  gtk_vbox_new (TRUE, 3);
        
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align), 
                                   3, 3, 0, 0);
    }
        
    gtk_widget_show (systray->iconbox);

    gtk_container_add (GTK_CONTAINER (systray->align), systray->iconbox);

    systray_start (systray);
}

static gboolean 
systray_set_size (XfcePanelPlugin *plugin, int size, Systray *systray)
{
    if (size > 26)
        gtk_container_set_border_width (GTK_CONTAINER (systray->frame), 2);
    else
        gtk_container_set_border_width (GTK_CONTAINER (systray->frame), 0);
    
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);
    }

    return TRUE;
}

static void
systray_free_data (XfcePanelPlugin *plugin, Systray *systray)
{
    systray_stop (systray);
    g_free (systray);
}

static void
systray_read_rc_file (XfcePanelPlugin *plugin, Systray *systray)
{
    char *file;
    XfceRc *rc;
    int show_frame = 1;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            show_frame = xfce_rc_read_int_entry (rc, "show_frame", 1);
            
            xfce_rc_close (rc);
        }
    }

    systray->show_frame = (show_frame != 0);
}

static void
systray_write_rc_file (XfcePanelPlugin *plugin, Systray *systray)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "show_frame", systray->show_frame);

    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static void 
systray_construct (XfcePanelPlugin *plugin)
{
    Systray *systray = g_new0 (Systray, 1);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (systray_orientation_changed), systray);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (systray_set_size), systray);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (systray_free_data), systray);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (systray_write_rc_file), systray);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (systray_properties_dialog), systray);

    systray->plugin = plugin;

    systray_read_rc_file (plugin, systray);

    systray->frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (systray->frame), 
            systray->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    gtk_widget_show (systray->frame);
    gtk_container_add (GTK_CONTAINER (plugin), systray->frame);

    systray->align = gtk_alignment_new (0, 0, 1, 1);
    gtk_widget_show (systray->align);
    gtk_container_add (GTK_CONTAINER (systray->frame), systray->align);
    
    if (xfce_panel_plugin_get_orientation (plugin) == 
            GTK_ORIENTATION_HORIZONTAL)
    {
        systray->iconbox =  gtk_hbox_new (TRUE, 3);
        
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align), 
                                   0, 0, 3, 3);
    }
    else
    {
        systray->iconbox =  gtk_vbox_new (TRUE, 3);
        
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align), 
                                   3, 3, 0, 0);
    }
    
    gtk_widget_show (systray->iconbox);
    gtk_container_add (GTK_CONTAINER (systray->align), systray->iconbox);

    systray_set_size (plugin, xfce_panel_plugin_get_size (plugin), systray);

    systray->tray = xfce_system_tray_new ();

    g_signal_connect (systray->tray, "icon_docked", G_CALLBACK (icon_docked),
                      systray);
    
    g_signal_connect (systray->tray, "icon_undocked",
                      G_CALLBACK (icon_undocked), systray);
    
    g_signal_connect (systray->tray, "message_new", G_CALLBACK (message_new),
                      systray);
    
    systray_start (systray);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
show_frame_toggled (GtkToggleButton *tb, Systray *systray)
{
    systray->show_frame = gtk_toggle_button_get_active (tb);

    gtk_frame_set_shadow_type (GTK_FRAME (systray->frame),
            systray->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

static void
systray_dialog_response (GtkWidget *dlg, int reponse, 
                         Systray *systray)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (systray->plugin);
    systray_write_rc_file (systray->plugin, systray);
}

static void
systray_properties_dialog (XfcePanelPlugin *plugin, Systray *systray)
{
    GtkWidget *dlg, *header, *vbox, *cb;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Configure Status Area"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_signal_connect (dlg, "response", G_CALLBACK (systray_dialog_response),
                      systray);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Status Area"));
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

    cb = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  systray->show_frame);
    g_signal_connect (cb, "toggled", G_CALLBACK (show_frame_toggled),
                      systray);

    gtk_widget_show (dlg);
}

