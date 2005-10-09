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
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define SEPARATOR_WIDTH  10
#define SEP_START        0.15
#define SEP_END          0.85

static void separator_properties_dialog (XfcePanelPlugin *plugin);

static void separator_construct (XfcePanelPlugin *plugin);

/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (separator_construct);


/* Interface Implementation */

static gboolean
separator_expose (GtkWidget *widget, GdkEventExpose *event, 
                  XfcePanelPlugin *plugin)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        GtkAllocation *allocation = &(widget->allocation);
        int start, end, position;

        if (xfce_panel_plugin_get_orientation (plugin) ==
                GTK_ORIENTATION_HORIZONTAL)
        {
            start = allocation->y + SEP_START * allocation->height;
            end = allocation->y + SEP_END * allocation->height;
            position = allocation->x + allocation->width / 2;
        
            gtk_paint_vline (widget->style, widget->window,
                             GTK_STATE_NORMAL,
                             &(event->area), widget, "separator",
                             start, end, position);
        }
        else
        {
            start = allocation->x + SEP_START * allocation->width;
            end = allocation->x + SEP_END * allocation->width;
            position = allocation->y + allocation->height / 2;
        
            gtk_paint_hline (widget->style, widget->window, 
                             GTK_STATE_NORMAL,
                             &(event->area), widget, "separator",
                             start, end, position);
        }

        return TRUE;
    }

    return FALSE;
}

static void
separator_add_widget (XfcePanelPlugin *plugin)
{
    GtkWidget *widget;
    
    widget = gtk_drawing_area_new ();
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (plugin), widget);
    
    g_signal_connect (widget, "expose-event", 
                      G_CALLBACK (separator_expose), plugin);
}

static void
separator_orientation_changed (XfcePanelPlugin *plugin, 
                               GtkOrientation orientation)
{
    if (GTK_BIN (plugin)->child)
        gtk_widget_queue_draw (GTK_BIN (plugin)->child);
}

static gboolean 
separator_set_size (XfcePanelPlugin *plugin, int size)
{
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     SEPARATOR_WIDTH, -1);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     -1, SEPARATOR_WIDTH);
    }

    return TRUE;
}

static void
separator_read_rc_file (XfcePanelPlugin *plugin)
{
    char *file;
    XfceRc *rc;
    int line, expand;
    
    line = 1;
    expand = 0;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            line = xfce_rc_read_int_entry (rc, "draw-separator", 1);
            
            expand = xfce_rc_read_int_entry (rc, "expand", 0);
            
            xfce_rc_close (rc);
        }
    }

    if (line)
        separator_add_widget (plugin);
    
    if (expand)
        xfce_panel_plugin_set_expand (plugin, TRUE);
}

static void
separator_write_rc_file (XfcePanelPlugin *plugin)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "draw-separator", 
                             GTK_BIN (plugin)->child ? 1 : 0);

    xfce_rc_write_int_entry (rc, "expand", 
                             xfce_panel_plugin_get_expand (plugin) ? 1 : 0);    
    
    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static void 
separator_construct (XfcePanelPlugin *plugin)
{
    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (separator_orientation_changed), NULL);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (separator_set_size), NULL);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (separator_write_rc_file), NULL);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (separator_properties_dialog), NULL);

    separator_read_rc_file (plugin);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
separator_toggled (GtkToggleButton *tb, XfcePanelPlugin *plugin)
{
    if (gtk_toggle_button_get_active (tb))
        separator_add_widget (plugin);
    else
        gtk_widget_destroy (GTK_BIN (plugin)->child);
}

static void
expand_toggled (GtkToggleButton *tb, XfcePanelPlugin *plugin)
{
    xfce_panel_plugin_set_expand (plugin, gtk_toggle_button_get_active (tb));
}

static void
separator_dialog_response (GtkWidget *dlg, int reponse, 
                          XfcePanelPlugin *plugin)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (plugin);
    separator_write_rc_file (plugin);
}

static void
separator_properties_dialog (XfcePanelPlugin *plugin)
{
    GtkWidget *dlg, *header, *vbox, *tb;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_signal_connect (dlg, "response", G_CALLBACK (separator_dialog_response),
                      plugin);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Separator"));
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

    tb = gtk_check_button_new_with_mnemonic (_("_Draw Separator"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);

    if (GTK_BIN (plugin)->child != NULL)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);

    g_signal_connect (tb, "toggled", G_CALLBACK (separator_toggled), plugin);
    
    tb = gtk_check_button_new_with_mnemonic (_("_Expand"));
    gtk_widget_show (tb);
    gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);

    if (xfce_panel_plugin_get_expand (plugin))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), TRUE);

    g_signal_connect (tb, "toggled", G_CALLBACK (expand_toggled), plugin);
    
    gtk_widget_show (dlg);
}

