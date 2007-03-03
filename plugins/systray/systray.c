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
#include <libxfce4panel/xfce-hvbox.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    XfceSystemTray *tray;
    guint tray_registered : 1;

    GtkWidget *frame;
    GtkWidget *align;
    GtkWidget *iconbox;

    guint show_frame : 1;
}
Systray;

static void systray_properties_dialog (XfcePanelPlugin *plugin,
                                       Systray *systray);

static void systray_free_data (XfcePanelPlugin *plugin, Systray *systray);

static void systray_construct (XfcePanelPlugin *plugin);

static gboolean systray_check (GdkScreen *screen);


/* -------------------------------------------------------------------- *
 *                             Systray                                  *
 * -------------------------------------------------------------------- */

static gboolean
systray_check (GdkScreen *screen)
{
    Screen *scr = GDK_SCREEN_XSCREEN (screen);

    if (xfce_system_tray_check_running (scr))
    {
        xfce_info (_("There is already a system tray running on this "
                     "screen"));
        return FALSE;
    }
    return TRUE;
}

static gboolean
register_tray (Systray * systray)
{
    GError *error = NULL;
    Screen *scr = GDK_SCREEN_XSCREEN (gtk_widget_get_screen (systray->frame));

    if (!systray_check (gtk_widget_get_screen (systray->frame)))
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
        gtk_widget_show (icon);
        gtk_box_pack_start (GTK_BOX (systray->iconbox), icon,
                            FALSE, FALSE, 0);
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

static gboolean
systray_remove (Systray *systray)
{
    GtkWidget *widget = GTK_WIDGET (systray->plugin);

    systray_free_data (systray->plugin, systray);
    gtk_widget_destroy (widget);

    return FALSE;
}

static void
systray_start (Systray *systray)
{
    if (!systray->tray_registered)
    {
        systray->tray_registered = register_tray (systray);

        if (!systray->tray_registered)
        {
            g_idle_add ((GSourceFunc)systray_remove, systray);
        }
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

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK (systray_construct,
                                                systray_check);


/* Interface Implementation */

static void
systray_orientation_changed (XfcePanelPlugin *plugin,
                             GtkOrientation orientation,
                             Systray *systray)
{
    xfce_hvbox_set_orientation (XFCE_HVBOX (systray->iconbox), orientation);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align),
                                   0, 0, 3, 3);
    }
    else
    {
        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align),
                                   3, 3, 0, 0);
    }
}

static gboolean
systray_set_size (XfcePanelPlugin *plugin, int size, Systray *systray)
{
    int border = size > 26 ? 1 : 0;

    gtk_container_set_border_width (GTK_CONTAINER (systray->frame), border);

    size = size - (2*border) - 2 - MAX (systray->frame->style->xthickness,
                                    systray->frame->style->ythickness);

    if (xfce_panel_plugin_get_orientation (plugin)
            == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (systray->iconbox, -1, size);
    }
    else
    {
        gtk_widget_set_size_request (systray->iconbox, size, -1);
    }

     return TRUE;
}

static void
systray_free_data (XfcePanelPlugin *plugin, Systray *systray)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);

    systray_stop (systray);
    panel_slice_free (Systray, systray);
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

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
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
    Systray *systray = panel_slice_new0 (Systray);

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
        systray->iconbox =
            xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 3);

        gtk_alignment_set_padding (GTK_ALIGNMENT (systray->align),
                                   0, 0, 3, 3);
    }
    else
    {
        systray->iconbox =
            xfce_hvbox_new (GTK_ORIENTATION_VERTICAL, FALSE, 3);

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
    g_object_set_data (G_OBJECT (systray->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (systray->plugin);
    systray_write_rc_file (systray->plugin, systray);
}

static void
systray_properties_dialog (XfcePanelPlugin *plugin, Systray *systray)
{
    GtkWidget *dlg, *vbox, *cb;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("System Tray"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");

    g_signal_connect (dlg, "response", G_CALLBACK (systray_dialog_response),
                      systray);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

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
