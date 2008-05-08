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

    NetkScreen *screen;
    GdkScreen *gdk_screen;
    GtkWidget *pager;

    
    int ws_created_id;
    int ws_destroyed_id;
    int screen_changed_id;
    int screen_size_changed_id;

    int rows;
    guint scrolling:1;
}
Pager;

static void pager_properties_dialog (XfcePanelPlugin *plugin, 
                                     Pager *pager);

static void pager_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (pager_construct);


/* Interface Implementation */

static void
pager_orientation_changed (XfcePanelPlugin *plugin, 
                           GtkOrientation orientation, 
                           Pager *pager)
{
    netk_pager_set_orientation (NETK_PAGER (pager->pager), orientation);
}

static gboolean 
pager_set_size (XfcePanelPlugin *plugin, int size)
{
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     -1, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     size, -1);
    }

    return TRUE;
}

static void
pager_free_data (XfcePanelPlugin *plugin, Pager *pager)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);
    
    g_signal_handler_disconnect (plugin, pager->screen_changed_id);

    if (pager->ws_created_id)
    {
        g_signal_handler_disconnect (pager->screen, pager->ws_created_id);
        pager->ws_created_id = 0;
    }
    
    if (pager->ws_destroyed_id)
    {
        g_signal_handler_disconnect (pager->screen, pager->ws_destroyed_id);
        pager->ws_destroyed_id = 0;
    }

    if (pager->screen_size_changed_id)
    {
        g_signal_handler_disconnect (pager->gdk_screen, pager->screen_size_changed_id);
        pager->screen_size_changed_id = 0;
    }

    panel_slice_free (Pager, pager);
}

static void
pager_read_rc_file (XfcePanelPlugin *plugin, Pager *pager)
{
    char *file;
    XfceRc *rc;
    int rows = 1;
    gboolean scrolling = TRUE;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            rows = xfce_rc_read_int_entry (rc, "rows", 1);
	    scrolling = xfce_rc_read_bool_entry (rc, "scrolling", TRUE);
        }
    }

    pager->rows = rows;
    pager->scrolling = scrolling;
}

static void
pager_write_rc_file (XfcePanelPlugin *plugin, Pager *pager)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "rows", pager->rows);
    
    xfce_rc_write_bool_entry (rc, "scrolling", pager->scrolling);

    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static void
pager_n_workspaces_changed (NetkScreen * screen, NetkWorkspace * ws, 
                            Pager * pager)
{
    pager_set_size (pager->plugin, 
                    xfce_panel_plugin_get_size (pager->plugin));
}

static void pager_screen_size_changed (GdkScreen *screen, Pager *pager);

static void
pager_screen_changed (GtkWidget *plugin, GdkScreen *screen, Pager *pager)
{
    screen = gtk_widget_get_screen (plugin);

    if (!screen)
        return;

    if (pager->ws_created_id)
    {
        g_signal_handler_disconnect (pager->screen, pager->ws_created_id);
        pager->ws_created_id = 0;
    }
    
    if (pager->ws_destroyed_id)
    {
        g_signal_handler_disconnect (pager->screen, pager->ws_destroyed_id);
        pager->ws_destroyed_id = 0;
    }

    if (pager->screen_size_changed_id)
    {
        g_signal_handler_disconnect (pager->gdk_screen, pager->screen_size_changed_id);
        pager->screen_size_changed_id = 0;
    }

    pager->gdk_screen = screen;
    pager->screen = netk_screen_get (gdk_screen_get_number (screen));

    netk_pager_set_screen (NETK_PAGER (pager->pager), pager->screen);
    
    pager->ws_created_id =
	g_signal_connect (pager->screen, "workspace-created",
			  G_CALLBACK (pager_n_workspaces_changed), pager);
    
    pager->ws_destroyed_id =
	g_signal_connect (pager->screen, "workspace-destroyed",
			  G_CALLBACK (pager_n_workspaces_changed), pager);

    pager->screen_size_changed_id = 
        g_signal_connect (screen, "size-changed", 
                          G_CALLBACK (pager_screen_size_changed), pager);
}

static void
pager_screen_size_changed (GdkScreen *screen, Pager *pager)
{
    pager_screen_changed (GTK_WIDGET (pager->plugin), screen, pager);
    gtk_widget_queue_resize (GTK_WIDGET (pager->plugin));
}

static void 
pager_construct (XfcePanelPlugin *plugin)
{
    GdkScreen *screen;
    int screen_idx;
    Pager *pager = panel_slice_new0 (Pager);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (pager_orientation_changed), pager);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (pager_set_size), NULL);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (pager_free_data), pager);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (pager_write_rc_file), pager);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (pager_properties_dialog), pager);

    pager->plugin = plugin;

    pager->gdk_screen = screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
    screen_idx = gdk_screen_get_number (screen);
    pager->screen = netk_screen_get (screen_idx);
    
    pager_read_rc_file (plugin, pager);

    pager->pager = netk_pager_new (pager->screen);
    netk_pager_set_orientation (NETK_PAGER (pager->pager), 
                                xfce_panel_plugin_get_orientation (plugin));
    netk_pager_set_n_rows (NETK_PAGER (pager->pager), pager->rows);
    netk_pager_set_workspace_scrolling (NETK_PAGER (pager->pager), pager->scrolling);
    gtk_widget_show (pager->pager);
    gtk_container_add (GTK_CONTAINER (plugin), pager->pager);
    
    pager->ws_created_id =
	g_signal_connect (pager->screen, "workspace-created",
			  G_CALLBACK (pager_n_workspaces_changed), pager);
    
    pager->ws_destroyed_id =
	g_signal_connect (pager->screen, "workspace-destroyed",
			  G_CALLBACK (pager_n_workspaces_changed), pager);
    
    xfce_panel_plugin_add_action_widget (plugin, pager->pager);
    
    pager->screen_changed_id = 
        g_signal_connect (plugin, "screen-changed", 
                          G_CALLBACK (pager_screen_changed), pager);

    pager->screen_size_changed_id = 
        g_signal_connect (screen, "size-changed", 
                          G_CALLBACK (pager_screen_size_changed), pager);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
rows_changed (GtkSpinButton * spin, Pager * pager)
{
    int rows = gtk_spin_button_get_value_as_int (spin);

    if (rows != pager->rows)
    {
	pager->rows = rows;

	netk_pager_set_n_rows (NETK_PAGER (pager->pager), pager->rows);
    }
}

static void
workspace_scrolling_toggled (GtkWidget *button, Pager *pager)
{
    gboolean scrolling = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    
    if (pager->scrolling != scrolling)
    {
	pager->scrolling = scrolling;
	
	netk_pager_set_workspace_scrolling (NETK_PAGER (pager->pager), scrolling);
    }
}

static void
pager_dialog_response (GtkWidget *dlg, int reponse, Pager *pager)
{
    g_object_set_data (G_OBJECT (pager->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (pager->plugin);
    pager_write_rc_file (pager->plugin, pager);
}

static void
pager_properties_dialog (XfcePanelPlugin *plugin, Pager *pager)
{
    GtkWidget *dlg, *vbox, *hbox, *label, *spin, *scrolling;
    int max;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = xfce_titled_dialog_new_with_buttons (_("Pager"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");
    
    gtk_window_set_screen (GTK_WINDOW (dlg), 
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_signal_connect (dlg, "response", G_CALLBACK (pager_dialog_response),
                      pager);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox,
                        TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    if (xfce_panel_plugin_get_orientation (plugin) == 
            GTK_ORIENTATION_HORIZONTAL)
    {
	label = gtk_label_new (_("Number of rows:"));
    }
    else
    {
	label = gtk_label_new (_("Number of columns:"));
    }    
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    max = netk_screen_get_workspace_count (pager->screen);

    if (max > 1)
    {
        spin = gtk_spin_button_new_with_range (1, max, 1);
        gtk_widget_show (spin);
        gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);

        gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), pager->rows);

        g_signal_connect (spin, "value-changed", G_CALLBACK (rows_changed),
                          pager);
    }
    else
    {
        GtkWidget *label = gtk_label_new ("1");

        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    }
    
    scrolling = gtk_check_button_new_with_mnemonic (_("Switch workspaces using the mouse wheel"));
    gtk_widget_show (scrolling);
    gtk_box_pack_start (GTK_BOX (vbox), scrolling, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (scrolling), pager->scrolling);
    
    g_signal_connect (scrolling, "toggled",
        G_CALLBACK (workspace_scrolling_toggled), pager);
  
    gtk_widget_show (dlg);
}

