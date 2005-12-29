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

#include <math.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-window-action-menu.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define N_ICONBOX_CONNECTIONS  4
#define N_ICON_CONNECTIONS     4

typedef struct
{
    XfcePanelPlugin *plugin;

    NetkScreen *netk_screen;
    int connections[N_ICONBOX_CONNECTIONS];
    int screen_changed_id;
    GtkWidget *box;
    GtkWidget *handle;
    GtkWidget *handle2;
    GtkWidget *iconbox;

    GSList *iconlist;
    GtkTooltips *icon_tooltips;

    int icon_size;
    gboolean only_hidden;
}
Iconbox;

typedef struct
{
    Iconbox *ib;
    
    NetkWindow *window;
    int connections[N_ICON_CONNECTIONS];

    GdkPixbuf *pb;
    
    GtkWidget *button;
    GtkWidget *image;
    gboolean was_minimized;
}
Icon;


static void iconbox_properties_dialog (XfcePanelPlugin *plugin, 
                                       Iconbox *iconbox);

static void iconbox_construct (XfcePanelPlugin *plugin);


/* -------------------------------------------------------------------- *
 *                            Iconbox                                   *
 * -------------------------------------------------------------------- */

/* icons */
static void 
update_visibility (Icon *icon, NetkWorkspace *optional_active_ws)
{
    NetkWorkspace *ws;

    gdk_flush ();

    if (netk_window_is_skip_tasklist (icon->window))
    {
        gtk_widget_hide (icon->button);
        return;
    }
    
    if (icon->ib->only_hidden && !netk_window_is_minimized (icon->window))
    {
        gtk_widget_hide (icon->button);
        return;
    }
    
    if (optional_active_ws)
    {
        ws = optional_active_ws;
    }
    else
    {
        ws = netk_screen_get_active_workspace 
                (netk_window_get_screen (icon->window));
    }
    
    if (ws == netk_window_get_workspace (icon->window) ||
        netk_window_is_sticky (icon->window))
    {
        gtk_widget_show (icon->button);
    }
    else
    {
        gtk_widget_hide (icon->button);
    }
}

static void
icon_update_image (Icon *icon)
{
    GdkPixbuf *scaled;

    g_return_if_fail (GDK_IS_PIXBUF (icon->pb));
    
    if (netk_window_is_minimized (icon->window))
    {
        if (!icon->was_minimized)
        {
            /* copied from netk-tasklist.c: dimm_icon */
            int x, y, w, h, pixel_stride, row_stride;
            guchar *row, *pixels;

            if (gdk_pixbuf_get_has_alpha (icon->pb))
                scaled = gdk_pixbuf_copy (icon->pb);
            else
                scaled = gdk_pixbuf_add_alpha (icon->pb, FALSE, 0, 0, 0);
        
            w = gdk_pixbuf_get_width (scaled);
            h = gdk_pixbuf_get_height (scaled);

            pixel_stride = 4;

            row = gdk_pixbuf_get_pixels (scaled);
            row_stride = gdk_pixbuf_get_rowstride (scaled);

            for (y = 0; y < h; y++)
            {
                pixels = row;

                for (x = 0; x < w; x++)
                {
                    pixels[3] /= 2;

                    pixels += pixel_stride;
                }

                row += row_stride;
            }

            xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image), 
                                               scaled);

            g_object_unref (scaled);

            icon->was_minimized = TRUE;
        }
    }
    else if (icon->was_minimized)
    {
        xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image), 
                                           icon->pb);

        icon->was_minimized = FALSE;
    }

    update_visibility (icon, NULL);
}

/* callbacks */
static gboolean
icon_button_pressed (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (ev->button == 1)
    {
        if (netk_window_is_active (icon->window))
            netk_window_minimize (icon->window);
        else
            netk_window_activate (icon->window);

        return TRUE;
    }
    else if (ev->button == 3)
    {
        GtkWidget *action_menu;
        
        action_menu = netk_create_window_action_menu(icon->window);
        
        g_signal_connect(G_OBJECT(action_menu), "selection-done", 
                         G_CALLBACK(gtk_widget_destroy), NULL);
        
        gtk_menu_popup(GTK_MENU(action_menu), NULL, NULL, NULL, NULL, 
                       ev->button, ev->time);

        return TRUE;
    }

    return FALSE;
}

static void
icon_name_changed (NetkWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    gtk_tooltips_set_tip (icon->ib->icon_tooltips, icon->button, 
                          netk_window_get_name (window), NULL);
}

static void
icon_state_changed (NetkWindow *window, NetkWindowState changed_mask,
               NetkWindowState new_state, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (changed_mask & NETK_WINDOW_STATE_MINIMIZED)
    {
        update_visibility (icon, NULL);

        icon_update_image (icon);
    }
}

static void
icon_workspace_changed (NetkWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    update_visibility (icon, NULL);
}

static void
icon_icon_changed (NetkWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;
    
    if (icon->pb)
        g_object_unref (icon->pb);

    icon->pb = netk_window_get_icon (icon->window);

    if (icon->pb)
        g_object_ref (icon->pb);
    
    icon_update_image (icon);
}

static void
icon_destroy (Icon *icon)
{
    int i;

    for (i = 0; i < N_ICON_CONNECTIONS; i++)
    {
        if (icon->connections[i])
            g_signal_handler_disconnect (icon->window, icon->connections[i]);
        
        icon->connections[i] = 0;
    }
    
    if (icon->pb)
        g_object_unref (icon->pb);
    
    g_free (icon);
}

static Icon *
icon_new (NetkWindow *window, Iconbox *ib)
{
    Icon *icon = g_new0 (Icon, 1);
    int i = 0;

    icon->ib = ib;
        
    icon->window = window;    

    icon->button = gtk_toggle_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (icon->button), FALSE);
    gtk_button_set_relief (GTK_BUTTON (icon->button), GTK_RELIEF_NONE);
    
    g_signal_connect (icon->button, "button-press-event",
                      G_CALLBACK (icon_button_pressed), icon);
    
    icon->image = xfce_scaled_image_new ();
    gtk_widget_show (icon->image);
    gtk_container_add (GTK_CONTAINER (icon->button), icon->image);
    
    icon->pb = netk_window_get_icon (window);
    if (icon->pb)
    {
        xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image), 
                                           icon->pb);
        g_object_ref (icon->pb);
    }
    
    icon->connections[i++] = 
        g_signal_connect (window, "name-changed", 
                                 G_CALLBACK (icon_name_changed), icon);
    
    icon->connections[i++] = 
        g_signal_connect (window, "state-changed", 
                                 G_CALLBACK (icon_state_changed), icon);
    
    icon->connections[i++] = 
        g_signal_connect (window, "workspace-changed", 
                                 G_CALLBACK (icon_workspace_changed), icon);
    
    icon->connections[i++] = 
        g_signal_connect (window, "icon-changed", 
                                 G_CALLBACK (icon_icon_changed), icon);
    
    g_assert (i == N_ICON_CONNECTIONS);
    
    if (netk_window_is_skip_tasklist (window))
    {
        return icon;
    }

    icon_update_image (icon);

    gtk_tooltips_set_tip (ib->icon_tooltips, icon->button, 
                          netk_window_get_name (window), NULL);

    update_visibility (icon, NULL);

    return icon;
}

/* iconlist */
static void
iconbox_active_window_changed (NetkScreen *screen, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;
    NetkWindow *window = netk_screen_get_active_window (screen);
    
    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;
        
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (icon->button), 
                                      (window == icon->window));
    }
}

static void
iconbox_active_workspace_changed (NetkScreen *screen, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;
    NetkWorkspace *ws = netk_screen_get_active_workspace (screen);
    
    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;
        
        update_visibility (icon, ws);
    }
}

static void
iconbox_window_opened (NetkScreen *screen, NetkWindow *window, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    Icon *icon;

    icon = icon_new (window, ib);

    ib->iconlist = g_slist_append (ib->iconlist, icon);

    gtk_box_pack_start (GTK_BOX (ib->iconbox), icon->button, FALSE, FALSE, 0);
}

static void
iconbox_window_closed (NetkScreen *screen, NetkWindow *window, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;
    
    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;
        
        if (window == icon->window)
        {
            gtk_widget_destroy (icon->button);
            icon_destroy (icon);

            ib->iconlist = g_slist_delete_link (ib->iconlist, l);

            break;
        }
    }
}

static void
iconbox_init_icons (Iconbox * ib)
{
    int i = 0;
    GList *windows, *l;
    
    netk_screen_force_update (ib->netk_screen);

    ib->connections[i++] =
        g_signal_connect (ib->netk_screen, "active_window_changed",
                                 G_CALLBACK (iconbox_active_window_changed), 
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->netk_screen, "active_workspace_changed",
                                 G_CALLBACK (iconbox_active_workspace_changed), 
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->netk_screen, "window_opened",
                                 G_CALLBACK (iconbox_window_opened), 
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->netk_screen, "window_closed",
                                 G_CALLBACK (iconbox_window_closed), 
                                 ib);

    g_assert (i == N_ICONBOX_CONNECTIONS);

    windows = netk_screen_get_windows (ib->netk_screen);

    for (l = windows; l != NULL; l = l->next)
    {
        NetkWindow *w = l->data;

        iconbox_window_opened (ib->netk_screen, w, ib);
    }

    iconbox_active_window_changed (ib->netk_screen, ib);
}

/* cleanup */
static void
cleanup_icons (Iconbox *ib)
{
    int i;
    GSList *l;

    for (i = 0; i < N_ICONBOX_CONNECTIONS; i++)
    {
        if (ib->connections[i])
            g_signal_handler_disconnect (ib->netk_screen, ib->connections[i]);

        ib->connections[i] = 0;
    }

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        icon_destroy ((Icon *)l->data);
    }

    g_slist_free (ib->iconlist);
    ib->iconlist = NULL;
}

static void
cleanup_iconbox (Iconbox *ib)
{
    cleanup_icons (ib);

    g_object_unref (ib->icon_tooltips);
    
    g_free (ib);
}


/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (iconbox_construct);


/* Interface Implementation */

static void
iconbox_orientation_changed (XfcePanelPlugin *plugin, 
                              GtkOrientation orientation, 
                              Iconbox *iconbox)
{
    GtkWidget *box;
    GList *children, *l;

    /* iconbox */
    box = (orientation == GTK_ORIENTATION_HORIZONTAL) ?
            gtk_hbox_new (TRUE, 0) : gtk_vbox_new (TRUE, 0);
    gtk_container_set_reallocate_redraws (GTK_CONTAINER (box), TRUE);
    gtk_widget_show (box);

    children = gtk_container_get_children (GTK_CONTAINER (iconbox->iconbox));

    for (l = children; l != NULL; l = l->next)
    {
        gtk_widget_reparent (GTK_WIDGET (l->data), box);
    }
    
    g_list_free (children);

    iconbox->iconbox = box;
    
    box = (orientation == GTK_ORIENTATION_HORIZONTAL) ?
            gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0);
    gtk_widget_show (box);
    
    gtk_widget_reparent (iconbox->handle, box);
    gtk_box_set_child_packing (GTK_BOX (box), iconbox->handle,
                               FALSE, FALSE, 0, GTK_PACK_START);
    
    gtk_box_pack_start (GTK_BOX (box), iconbox->iconbox, FALSE, FALSE, 0);
    
    gtk_widget_reparent (iconbox->handle2, box);
    gtk_box_set_child_packing (GTK_BOX (box), iconbox->handle2,
                               FALSE, FALSE, 0, GTK_PACK_START);
    
    gtk_widget_destroy (iconbox->box);
    iconbox->box = box;
    gtk_container_add (GTK_CONTAINER (plugin), box);

    gtk_widget_queue_draw (iconbox->handle);
    gtk_widget_queue_draw (iconbox->handle2);
}

static gboolean 
iconbox_set_size (XfcePanelPlugin *plugin, int size, Iconbox *ib)
{
    GSList *l;
    Icon *icon = ib->iconlist ? ib->iconlist->data : NULL;
    GtkWidget *widget = icon ? icon->button : ib->iconbox;
    
    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        ib->icon_size = size - 2 * widget->style->xthickness - 2;
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     -1, size);
    }
    else
    {
        ib->icon_size = size - 2 * widget->style->ythickness - 2;
        gtk_widget_set_size_request (GTK_WIDGET (plugin), 
                                     size, -1);
    }

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        icon_update_image (icon);
    }
    
    return TRUE;
}

static void
iconbox_free_data (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);
    
    g_signal_handler_disconnect (plugin, iconbox->screen_changed_id);

    cleanup_iconbox (iconbox);
}

static void
iconbox_read_rc_file (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    char *file;
    XfceRc *rc;
    int only_hidden = 0;
    
    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            only_hidden = xfce_rc_read_int_entry (rc, "only_hidden", 0);
            
            xfce_rc_close (rc);
        }
    }

    iconbox->only_hidden = (only_hidden == 1);
}

static void
iconbox_write_rc_file (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    char *file;
    XfceRc *rc;
    
    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;
    
    xfce_rc_write_int_entry (rc, "only_hidden", iconbox->only_hidden);

    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static gboolean
handle_expose (GtkWidget *widget, GdkEventExpose *ev, Iconbox *iconbox)
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
iconbox_screen_changed (GtkWidget *plugin, GdkScreen *screen, Iconbox *ib)
{
    if (!screen)
        return;

    cleanup_icons (ib);
    
    ib->netk_screen = netk_screen_get (gdk_screen_get_number (screen));
    
    iconbox_init_icons (ib);
}

static void
iconbox_realize (GtkWidget *plugin, Iconbox *ib)
{
  iconbox_screen_changed (GTK_WIDGET (plugin), 
                          gtk_widget_get_screen (plugin), ib);
}

static void
iconbox_construct (XfcePanelPlugin *plugin)
{
    Iconbox *iconbox = g_new0 (Iconbox, 1);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (iconbox_orientation_changed), iconbox);
    
    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (iconbox_set_size), iconbox);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (iconbox_free_data), iconbox);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (iconbox_write_rc_file), iconbox);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (iconbox_properties_dialog), iconbox);

    xfce_panel_plugin_set_expand (plugin, TRUE);
    
    iconbox_read_rc_file (plugin, iconbox);

    iconbox->plugin = plugin;
    
    iconbox->box = (xfce_panel_plugin_get_orientation (plugin) == 
                        GTK_ORIENTATION_HORIZONTAL) ?
                    gtk_hbox_new (FALSE, 0) : gtk_vbox_new (FALSE, 0);
    gtk_container_set_reallocate_redraws (GTK_CONTAINER (iconbox->box), TRUE);
    gtk_widget_show (iconbox->box);
    gtk_container_add (GTK_CONTAINER (plugin), iconbox->box);

    iconbox->handle = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (iconbox->handle, 6, 6);
    gtk_widget_show (iconbox->handle);
    gtk_box_pack_start (GTK_BOX (iconbox->box), iconbox->handle, 
                        FALSE, FALSE, 0);

    xfce_panel_plugin_add_action_widget (plugin, iconbox->handle);

    g_signal_connect (iconbox->handle, "expose-event", 
                      G_CALLBACK (handle_expose), iconbox);
    
    iconbox->iconbox = (xfce_panel_plugin_get_orientation (plugin) == 
                        GTK_ORIENTATION_HORIZONTAL) ?
                       gtk_hbox_new (TRUE, 0) : gtk_vbox_new (TRUE, 0);
    gtk_widget_show (iconbox->iconbox);
    gtk_box_pack_start (GTK_BOX (iconbox->box), iconbox->iconbox, 
                        FALSE, FALSE, 0);
    
    iconbox->handle2 = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (iconbox->handle2, 6, 6);
    gtk_widget_show (iconbox->handle2);
    gtk_box_pack_start (GTK_BOX (iconbox->box), iconbox->handle2, 
                        FALSE, FALSE, 0);

    xfce_panel_plugin_add_action_widget (plugin, iconbox->handle2);

    g_signal_connect (iconbox->handle2, "expose-event", 
                      G_CALLBACK (handle_expose), iconbox);
    
    iconbox->icon_tooltips = gtk_tooltips_new ();
    g_object_ref (iconbox->icon_tooltips);
    gtk_object_sink (GTK_OBJECT (iconbox->icon_tooltips));

    iconbox->screen_changed_id = 
        g_signal_connect (plugin, "screen-changed", 
                          G_CALLBACK (iconbox_screen_changed), iconbox);
    
    g_signal_connect (plugin, "realize", G_CALLBACK (iconbox_realize), iconbox);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
only_hidden_toggled (GtkToggleButton *tb, Iconbox *ib)
{
    GSList *l;
    
    ib->only_hidden = gtk_toggle_button_get_active (tb);

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;        

        update_visibility (icon, NULL);
    }
}

static void
iconbox_dialog_response (GtkWidget *dlg, int reponse, 
                         Iconbox *ib)
{
    g_object_set_data (G_OBJECT (ib->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (ib->plugin);
    iconbox_write_rc_file (ib->plugin, ib);
}

static void
iconbox_properties_dialog (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    GtkWidget *dlg, *header, *vbox, *cb;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect (dlg, "response", G_CALLBACK (iconbox_dialog_response),
                      iconbox);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    header = xfce_create_header (NULL, _("Icon Box"));
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

    cb = gtk_check_button_new_with_mnemonic (
                _("Only show minimized applications"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  iconbox->only_hidden);
    g_signal_connect (cb, "toggled", G_CALLBACK (only_hidden_toggled),
                      iconbox);
  
    gtk_widget_show (dlg);
}

