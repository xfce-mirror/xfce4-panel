/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2003 Andre Lerche <a.lerche@gmx.net>
 *  Copyright © 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *  Copyright © 2006 Jani Monoses <jani@ubuntu.com> 
 *  Copyright © 2006 Jasper Huijsmans <jasper@xfce.org>
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
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/netk-window-action-menu.h>

#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget *windowlist;
    GtkWidget *img;
    NetkScreen *screen;
    int screen_callback_id; 
    gboolean all_workspaces;

    GtkTooltips *tooltips;
}
Windowlist;

/* Panel Plugin Interface */

static void windowlist_construct (XfcePanelPlugin * plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (windowlist_construct);


/* internal functions */
static void
action_menu_deactivated (GtkMenu *menu, GtkMenu *parent)
{
    gtk_menu_popdown (parent);
    g_signal_emit_by_name (parent, "deactivate", 0);
}

static void
popup_action_menu (GtkWidget * widget, gpointer data)
{
    NetkWindow *win = data;
    static GtkWidget *menu = NULL;

    if (menu)
    {
        gtk_widget_destroy (menu);
    }

    menu = netk_create_window_action_menu (win);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0,
                    GDK_CURRENT_TIME);

    g_signal_connect (menu, "deactivate", 
                      G_CALLBACK (action_menu_deactivated), widget->parent);
}

static gboolean
menu_item_clicked (GtkWidget * item, GdkEventButton * ev, gpointer data)
{
    NetkWindow *win = data;

    if (ev->button == 1)
    {
        netk_window_activate (win);
    }
    else if (ev->button == 3)
    {
        popup_action_menu (item, win);
        return TRUE;
    }

    return FALSE;
}

static void
set_num_screens (gpointer num)
{
    netk_screen_change_workspace_count (netk_screen_get_default (),
                                        GPOINTER_TO_INT (num));
}

/* the following two functions are based on xfdesktop code */
static GtkWidget *
create_menu_item (NetkWindow * win, Windowlist * wl)
{
    const char *wname = NULL;
    GtkWidget *item;
    GString *label;
    GdkPixbuf *icon, *tmp = NULL;

    if (netk_window_is_skip_pager (win) || netk_window_is_skip_tasklist (win))
        return NULL;

    wname = netk_window_get_name (win);
    label = g_string_new (wname);
    if (label->len >= 20)
    {
        g_string_truncate (label, 20);
        g_string_append (label, " ...");
    }

    if (netk_window_is_minimized (win))
    {
        g_string_prepend (label, "[");
        g_string_append (label, "]");
    }

    icon = netk_window_get_icon (win);
    if (icon)
    {
        GtkWidget *img;
        gint w, h;

        w = gdk_pixbuf_get_width (tmp);
        h = gdk_pixbuf_get_height (tmp);
        if (w != 24 || h != 24)
        {
            tmp = gdk_pixbuf_scale_simple (icon, 24, 24, GDK_INTERP_BILINEAR);
            icon = tmp;
        }
        
        item = gtk_image_menu_item_new_with_label (label->str);
        gtk_widget_show (item);
        
        img = gtk_image_new_from_pixbuf (icon);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), img);

        if (tmp)
            g_object_unref (tmp);
    }
    else
    {
        item = gtk_menu_item_new_with_label (label->str);
    }

    gtk_tooltips_set_tip (wl->tooltips, item, wname, NULL);

    g_string_free (label, TRUE);
    return item;
}

static void
menu_deactivated (GtkWidget *menu, GtkToggleButton *tb)
{
    gtk_toggle_button_set_active (tb, FALSE);
}

static void
position_menu (GtkMenu *menu, int *x, int *y, gboolean *push_in, 
               Windowlist *wl)
{
    XfceScreenPosition pos;
    GtkRequisition req;

    gtk_widget_size_request(GTK_WIDGET(menu), &req);

    gdk_window_get_origin (GTK_WIDGET (wl->plugin)->window, x, y);

    pos = xfce_panel_plugin_get_screen_position(wl->plugin);

    switch(pos) {
        case XFCE_SCREEN_POSITION_NW_V:
        case XFCE_SCREEN_POSITION_W:
        case XFCE_SCREEN_POSITION_SW_V:
            *x += wl->windowlist->allocation.width;
            *y += wl->windowlist->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NE_V:
        case XFCE_SCREEN_POSITION_E:
        case XFCE_SCREEN_POSITION_SE_V:
            *x -= req.width;
            *y += wl->windowlist->allocation.height - req.height;
            break;
        
        case XFCE_SCREEN_POSITION_NW_H:
        case XFCE_SCREEN_POSITION_N:
        case XFCE_SCREEN_POSITION_NE_H:
            *y += wl->windowlist->allocation.height;
            break;
        
        case XFCE_SCREEN_POSITION_SW_H:
        case XFCE_SCREEN_POSITION_S:
        case XFCE_SCREEN_POSITION_SE_H:
            *y -= req.height;
            break;
        
        default:  /* floating */
            gdk_display_get_pointer(
                    gtk_widget_get_display(GTK_WIDGET(wl->plugin)),
                    NULL, x, y, NULL);
    }

    if (*x < 0)
        *x = 0;

    if (*y < 0)
        *y = 0;
}

static void
plugin_windowlist_clicked (GtkWidget * w, GdkEventButton *ev, Windowlist * wl)
{
    static GtkWidget *menu = NULL;
    GtkWidget *item, *label, *img;
    NetkWindow *win;
    NetkWorkspace *ws, *aws, *winws;
    int wscount, i;
    GList *windows, *li;

    if (menu)
    {
        gtk_widget_destroy (menu);
    }

    windows = netk_screen_get_windows_stacked (wl->screen);
    aws = netk_screen_get_active_workspace (wl->screen);

    menu = gtk_menu_new ();

    item = gtk_menu_item_new_with_label (_("Window List"));
    gtk_widget_set_sensitive (item, FALSE);
    gtk_widget_show (item);
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    if (wl->all_workspaces)
    {
        wscount = netk_screen_get_workspace_count (wl->screen);
    }
    else
    {
        wscount = 1;
    }

    for (i = 0; i < wscount; i++)
    {
        char ws_name[32];
        const char *realname;

        if (wl->all_workspaces)
        {
            ws = netk_screen_get_workspace (wl->screen, i);
        }
        else
        {
            ws = netk_screen_get_active_workspace (wl->screen);
        }

        realname = netk_workspace_get_name (ws);

        if (realname)
        {
            g_snprintf (ws_name, 32, "<i>%s</i>", realname);
        }
        else
        {
            g_snprintf (ws_name, 32, "<i>%d</i>", i + 1);
        }

        item = gtk_menu_item_new_with_label (ws_name);
        g_signal_connect_swapped (item, "activate",
                          G_CALLBACK (netk_workspace_activate), ws);

        label = gtk_bin_get_child (GTK_BIN (item));
        gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        if (ws == aws)
        {
            gtk_widget_set_sensitive (item, FALSE);
        }

        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        for (li = windows; li; li = li->next)
        {
            win = li->data;
            winws = netk_window_get_workspace (win);
            if (ws != winws && !(netk_window_is_sticky (win) && ws == aws))
            {
                continue;
            }

            item = create_menu_item (win, wl);

            if (!item)
                continue;

            if (ws != aws)
            {
                gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (item)),
                        GTK_STATE_NORMAL,
                        &(menu->style->fg[GTK_STATE_INSENSITIVE]));
            }

            g_signal_connect (item, "button-release-event",
                              G_CALLBACK (menu_item_clicked), win);

            gtk_widget_show (item);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
        }

        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }

    wscount = netk_screen_get_workspace_count (wl->screen);

    item = gtk_image_menu_item_new_with_label (_("Add workspace"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (set_num_screens),
                              GINT_TO_POINTER (wscount + 1));

    img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
    gtk_widget_show (img);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), img);
    
    item = gtk_image_menu_item_new_with_label (_("Delete workspace"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    g_signal_connect_swapped (item, "activate", G_CALLBACK (set_num_screens),
                              GINT_TO_POINTER (wscount - 1));

    img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
    gtk_widget_show (img);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), img);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wl->windowlist), TRUE);
    g_signal_connect (menu, "deactivate", G_CALLBACK (menu_deactivated), 
                      wl->windowlist);
    
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
                    (GtkMenuPositionFunc)position_menu, wl, 
                    ev->button, ev->time);
}

static void
plugin_active_window_changed (GtkWidget * w, Windowlist * wl)
{
    NetkWindow *win;
    GdkPixbuf *pb;

    if ((win = netk_screen_get_active_window (wl->screen)) != NULL)
    {
        pb = netk_window_get_icon (win);
        if (pb)
        {
            xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (wl->img), pb);
            gtk_tooltips_set_tip (wl->tooltips, wl->windowlist,
                                  netk_window_get_name (win), NULL);
        }
    }
}

/* Configuration */

static void
windowlist_read_rc_file (XfcePanelPlugin * plugin, Windowlist * wl)
{
    char *file;
    XfceRc *rc;
    int all_workspaces;

    all_workspaces = 0;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            all_workspaces = xfce_rc_read_int_entry (rc, "all_workspaces",
                                                     all_workspaces);
            xfce_rc_close (rc);
        }
    }

    wl->all_workspaces = (all_workspaces == 1);
}

static void
windowlist_write_rc_file (XfcePanelPlugin * plugin, Windowlist * wl)
{
    char *file;
    XfceRc *rc;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "all_workspaces", wl->all_workspaces);

    xfce_rc_close (rc);

}

static void
plugin_cb_changed (GtkToggleButton * cb, Windowlist * wl)
{
    wl->all_workspaces = gtk_toggle_button_get_active (cb);
}

static void
windowlist_dialog_response (GtkWidget * dlg, int reponse, Windowlist * wl)
{
    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (wl->plugin);
    windowlist_write_rc_file (wl->plugin, wl);
}


static void
windowlist_properties_dialog (XfcePanelPlugin * plugin, Windowlist * wl)
{
    GtkWidget *dlg, *header, *mainvbox, *cb;

    xfce_panel_plugin_block_menu (plugin);

    dlg = gtk_dialog_new_with_buttons (_("Properties"),
                                       GTK_WINDOW (gtk_widget_get_toplevel
                                                   (GTK_WIDGET (plugin))),
                                       GTK_DIALOG_DESTROY_WITH_PARENT |
                                       GTK_DIALOG_NO_SEPARATOR,
                                       GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                       NULL);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

    g_signal_connect (dlg, "response",
                      G_CALLBACK (windowlist_dialog_response), wl);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    header = xfce_create_header (NULL, _("Window List"));
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

    cb = gtk_check_button_new_with_label (_("Include all Workspaces"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), wl->all_workspaces);
    g_signal_connect (cb, "toggled", G_CALLBACK (plugin_cb_changed), wl);
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (mainvbox), cb, FALSE, FALSE, 0);

    gtk_widget_show (dlg);
}

/* Handle free-data signal */

static void
windowlist_free_data (XfcePanelPlugin * plugin, Windowlist * wl)
{
    g_signal_handler_disconnect (wl->screen, wl->screen_callback_id);
    gtk_object_sink (GTK_OBJECT (wl->tooltips));
    g_free (wl);
}

static Windowlist *
windowlist_new (XfcePanelPlugin * plugin)
{
    GdkPixbuf *pb;
    Windowlist *wl = g_new0 (Windowlist, 1);

    wl->plugin = plugin;

    wl->tooltips = gtk_tooltips_new ();

    wl->all_workspaces = FALSE;
    wl->screen = netk_screen_get_default ();
    netk_screen_force_update (wl->screen);

    wl->windowlist = gtk_toggle_button_new ();
    gtk_button_set_relief (GTK_BUTTON (wl->windowlist), GTK_RELIEF_NONE);
    gtk_widget_show (wl->windowlist);

    pb = gtk_widget_render_icon (GTK_WIDGET (plugin), GTK_STOCK_MISSING_IMAGE,
                                 GTK_ICON_SIZE_MENU, NULL);
    wl->img = xfce_scaled_image_new_from_pixbuf (pb);
    gtk_widget_show (wl->img);
    gtk_container_add (GTK_CONTAINER (wl->windowlist), wl->img);
    g_object_unref (pb);

    xfce_panel_plugin_add_action_widget (plugin, wl->windowlist);
    g_signal_connect (wl->windowlist, "button-press-event",
                      G_CALLBACK (plugin_windowlist_clicked), wl);

    /* set icon */
    plugin_active_window_changed (NULL, wl);
    
    wl->screen_callback_id =
        g_signal_connect (wl->screen, "active-window-changed",
                          G_CALLBACK (plugin_active_window_changed), wl);

    gtk_container_add (GTK_CONTAINER (plugin), wl->windowlist);

    return wl;
}

static void
windowlist_construct (XfcePanelPlugin * plugin)
{
    Windowlist *wl = windowlist_new (plugin);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (windowlist_free_data), wl);

    g_signal_connect (plugin, "save",
                      G_CALLBACK (windowlist_write_rc_file), wl);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (windowlist_properties_dialog), wl);

    windowlist_read_rc_file (plugin, wl);
}
