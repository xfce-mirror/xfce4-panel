/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2003 Andre Lerche <a.lerche@gmx.net>
 *  Copyright © 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *  Copyright © 2006 Jani Monoses <jani@ubuntu.com> 
 *  Copyright © 2006 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright © 2006 Nick Schermer <nick@xfce.org>
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
#include <libxfce4panel/xfce-panel-convenience.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkTooltips *tooltips;
    
    GtkWidget *button;
    GtkWidget *img;
    
    NetkScreen *screen;
    int screen_callback_id; 
    
    guint show_all_workspaces:1;
    guint show_windowlist_icons:1;
}
Windowlist;

/**
 * REGISTER PLUGIN
 **/
static void windowlist_construct (XfcePanelPlugin * plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (windowlist_construct);


/**
 * MENU FUNCTIONS
 **/
static void
action_menu_deactivated (GtkMenu *menu,
			 GtkMenu *parent)
{
    gtk_menu_popdown (parent);
    g_signal_emit_by_name (parent, "deactivate", 0);
}

static void
popup_action_menu (GtkWidget * widget,
		   gpointer data)
{
    NetkWindow *win = data;
    static GtkWidget *menu = NULL;

    if (menu)
	gtk_widget_destroy (menu);

    menu = netk_create_window_action_menu (win);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0,
                    GDK_CURRENT_TIME);

    g_signal_connect (menu, "deactivate", 
                      G_CALLBACK (action_menu_deactivated), widget->parent);
}

static gboolean
menu_item_clicked (GtkWidget *mi,
		   GdkEventButton * ev,
		   gpointer data)
{
    NetkWindow *netk_window = data;

    if (ev->button == 1)
    {
        netk_workspace_activate(netk_window_get_workspace(netk_window));
	netk_window_activate (netk_window);
    }
    else if (ev->button == 3)
    {
        popup_action_menu (mi, netk_window);
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
create_menu_item (NetkWindow * window,
		  Windowlist * wl,
		  gint icon_width,
		  gint icon_height)
{
    const char *window_name = NULL;
    GtkWidget *mi;
    GString *label;
    GdkPixbuf *icon = NULL, *tmp = NULL;
    gboolean truncated = FALSE;

    if (netk_window_is_skip_pager (window) || netk_window_is_skip_tasklist (window))
        return NULL;
    
    window_name = netk_window_get_name (window);
    label = g_string_new (window_name);
    
    if (label->len >= 20)
    {
        g_string_truncate (label, 20);
        g_string_append (label, " ...");
	truncated = TRUE;
    }

    if (netk_window_is_minimized (window))
    {
        g_string_prepend (label, "[");
        g_string_append (label, "]");
    }
    
    /* Italic fonts are not completely shown */
    g_string_append (label, " ");
    
    if (wl->show_windowlist_icons)
	icon = netk_window_get_icon (window);

    if (icon)
    {
        GtkWidget *img;
        gint w, h;
    
        w = gdk_pixbuf_get_width (icon);
        h = gdk_pixbuf_get_height (icon);

        if (w != icon_width || h != icon_height)
        {
            tmp = gdk_pixbuf_scale_simple (icon, icon_width, icon_height, GDK_INTERP_BILINEAR);
            icon = tmp;
        }
    
        mi = gtk_image_menu_item_new_with_label (label->str);
        
        img = gtk_image_new_from_pixbuf (icon);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
	
	if (tmp)
	    g_object_unref (tmp);
    }
    else
    {
        mi = gtk_menu_item_new_with_label (label->str);
    }
    
    if (truncated)
	gtk_tooltips_set_tip (wl->tooltips, mi, window_name, NULL);

    g_string_free (label, TRUE);
    return mi;
}

static void
menu_deactivated (GtkWidget *menu,
		  GtkToggleButton *tb)
{
    gtk_toggle_button_set_active (tb, FALSE);
}

static void
position_menu (GtkMenu *menu,
	       int *x,
	       int *y,
	       gboolean *push_in, 
	       Windowlist *wl)
{
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;
    
    gtk_widget_size_request (GTK_WIDGET (menu), &req);
    gdk_window_get_origin (GTK_WIDGET (wl->plugin)->window, x, y);
    
    switch (xfce_panel_plugin_get_screen_position(wl->plugin))
    {
	case XFCE_SCREEN_POSITION_SW_H:
	case XFCE_SCREEN_POSITION_S:
	case XFCE_SCREEN_POSITION_SE_H:
	    *y -= req.height;
	    break;

	case XFCE_SCREEN_POSITION_NW_H:
	case XFCE_SCREEN_POSITION_N:
	case XFCE_SCREEN_POSITION_NE_H:
	    *y += wl->button->allocation.height;
	    break;

	case XFCE_SCREEN_POSITION_NW_V:
	case XFCE_SCREEN_POSITION_W:
	case XFCE_SCREEN_POSITION_SW_V:
	    *x += wl->button->allocation.width;
	    *y += wl->button->allocation.height - req.height;
	    break;

	case XFCE_SCREEN_POSITION_NE_V:
	case XFCE_SCREEN_POSITION_E:
	case XFCE_SCREEN_POSITION_SE_V:
	    *x -= req.width;
	    *y += wl->button->allocation.height - req.height;
	    break;

	default: /* Floating */
	    gdk_display_get_pointer(gtk_widget_get_display(GTK_WIDGET(wl->plugin)),
				    NULL,
				    x, y, 
				    NULL);
    }

    screen = gtk_widget_get_screen (wl->button);
    num = gdk_screen_get_monitor_at_window (screen, wl->button->window);
    gdk_screen_get_monitor_geometry (screen, num, &geom);
    
    if (*x > geom.x + geom.width - req.width)
	*x = geom.x + geom.width - req.width;
    
    if (*x < geom.x)
	*x = geom.x;
    
    if (*y > geom.y + geom.height - req.height)
	*y = geom.y + geom.height - req.height;
    
    if (*y < geom.y)
	*y = geom.y;
}

static void
plugin_button_clicked (GtkWidget * w,
		       GdkEventButton *ev,
		       Windowlist * wl)
{
    static GtkWidget *menu = NULL;
    GtkWidget *mi, *img;
    NetkWindow *window;
    NetkWindowState state;
    NetkWorkspace *netk_workspace, *active_workspace, *window_workspace;
    gint icon_width, icon_height, wscount, i;
    const gchar *ws_name = NULL;
    gchar *ws_label, *rm_label;
    GList *windows, *li;
    
    PangoFontDescription *italic = pango_font_description_from_string ("italic");
    PangoFontDescription *bold = pango_font_description_from_string ("bold");
    PangoFontDescription *bold_italic = pango_font_description_from_string ("bold italic");

    if (menu)
	gtk_widget_destroy (menu);
    
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_width, &icon_height);

    windows = netk_screen_get_windows_stacked (wl->screen);
    active_workspace = netk_screen_get_active_workspace (wl->screen);

    menu = gtk_menu_new ();

    if (wl->show_all_workspaces)
	wscount = netk_screen_get_workspace_count (wl->screen);
    else
	wscount = 1;

    for (i = 0; i < wscount; i++)
    {
        if (wl->show_all_workspaces)
	    netk_workspace = netk_screen_get_workspace (wl->screen, i);
        else
	    netk_workspace = netk_screen_get_active_workspace (wl->screen);

        ws_name = netk_workspace_get_name (netk_workspace);

        if(!ws_name || atoi(ws_name) == i+1)
	    ws_label = g_strdup_printf(_("Workspace %d"), i+1);
	else
	    ws_label = g_markup_printf_escaped("%s", ws_name);

        mi = gtk_menu_item_new_with_label (ws_label);
	g_free (ws_label);
        g_signal_connect_swapped (mi, "activate",
                          G_CALLBACK (netk_workspace_activate), netk_workspace);

	if (netk_workspace == active_workspace)
	    gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), bold);
        else
            gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), italic); 

        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        for (li = windows; li; li = li->next)
        {
            window = li->data;
            window_workspace = netk_window_get_workspace (window);
            if (netk_workspace != window_workspace && !(netk_window_is_sticky (window) && netk_workspace == active_workspace))
		continue;
	    
	    state = netk_window_get_state (window);

            mi = create_menu_item (window,
				   wl,
				   icon_width,
				   icon_height);

            if (!mi)
                continue;
	    
	    if(state & NETK_WINDOW_STATE_URGENT && netk_workspace == active_workspace)
	    {
		gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), 
			bold);
	    }
	    else if (state & NETK_WINDOW_STATE_URGENT)
	    {
		gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (mi)),
                        GTK_STATE_NORMAL,
                        &(menu->style->fg[GTK_STATE_INSENSITIVE]));
		gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), 
			bold_italic);
	    }
	    else if (netk_workspace != active_workspace)
            {
                gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (mi)),
                        GTK_STATE_NORMAL,
                        &(menu->style->fg[GTK_STATE_INSENSITIVE]));
		gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), 
			italic);
            }

            g_signal_connect (mi, "button-release-event",
                              G_CALLBACK (menu_item_clicked), window);

            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }

        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }
    
    pango_font_description_free(italic);
    pango_font_description_free(bold);
    pango_font_description_free(bold_italic);

    /* Count workspace number */
    wscount = netk_screen_get_workspace_count (wl->screen);

    /* Add workspace */
    if (wl->show_windowlist_icons)
    {
	mi = gtk_image_menu_item_new_with_label (_("Add workspace"));
	img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
    }
    else
    {
	mi = gtk_menu_item_new_with_label (_("Add workspace"));
    }
    
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (set_num_screens),
			    GINT_TO_POINTER (wscount + 1));

    /* Remove workspace */
    if (!wl->show_all_workspaces)
    {
	netk_workspace = netk_screen_get_workspace (wl->screen, wscount-1);
	ws_name = netk_workspace_get_name (netk_workspace);
    }
    
    if(!ws_name || atoi(ws_name) == wscount)
	rm_label = g_strdup_printf(_("Remove Workspace %d"), wscount);
    else
	rm_label = g_markup_printf_escaped(_("Remove Workspace '%s'"), ws_name);
    
    if (wl->show_windowlist_icons)
    {
	mi = gtk_image_menu_item_new_with_label (rm_label);
	img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
    }
    else
    {
	mi = gtk_menu_item_new_with_label (rm_label);
    }
    
    g_free (rm_label);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    g_signal_connect_swapped (mi, "activate", G_CALLBACK (set_num_screens),
                              GINT_TO_POINTER (wscount - 1));
    
    /* Activate toggle button */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wl->button), TRUE);
    
    g_signal_connect (menu, "deactivate", G_CALLBACK (menu_deactivated), 
                      wl->button);

    gtk_widget_show_all (menu);
    
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
            gtk_tooltips_set_tip (wl->tooltips, wl->button,
                                  netk_window_get_name (win), NULL);
        }
    }
}

/**
 * CONFIGURATION
 **/
static void
windowlist_read_rc_file (XfcePanelPlugin * plugin, Windowlist * wl)
{
    char *file;
    XfceRc *rc;
    gboolean show_all_workspaces, show_windowlist_icons;

    show_all_workspaces = FALSE;
    show_windowlist_icons = TRUE;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            show_all_workspaces = xfce_rc_read_bool_entry (rc, "show_all_workspaces",
                                                     show_all_workspaces);
	    show_windowlist_icons 	   = xfce_rc_read_bool_entry (rc, "show_windowlist_icons",
                                                     show_windowlist_icons);
            xfce_rc_close (rc);
        }
    }

    wl->show_all_workspaces = show_all_workspaces;
    wl->show_windowlist_icons = show_windowlist_icons;
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

    xfce_rc_write_bool_entry (rc, "show_all_workspaces", wl->show_all_workspaces);
    xfce_rc_write_bool_entry (rc, "show_windowlist_icons", wl->show_windowlist_icons);

    xfce_rc_close (rc);
}

static void
plugin_show_all_changed (GtkToggleButton * cb, Windowlist * wl)
{
    wl->show_all_workspaces = gtk_toggle_button_get_active (cb);
}

static void
plugin_show_windowlist_icons_changed (GtkToggleButton * cb, Windowlist * wl)
{
    wl->show_windowlist_icons = gtk_toggle_button_get_active (cb);
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
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), header,
                        FALSE, TRUE, 0);

    mainvbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (mainvbox), 5);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), mainvbox,
                        TRUE, TRUE, 0);

    cb = gtk_check_button_new_with_label (_("Include all Workspaces"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), wl->show_all_workspaces);
    g_signal_connect (cb, "toggled", G_CALLBACK (plugin_show_all_changed), wl);
    gtk_box_pack_start (GTK_BOX (mainvbox), cb, FALSE, FALSE, 0);
    
    cb = gtk_check_button_new_with_label (_("Show Window Icons"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), wl->show_windowlist_icons);
    g_signal_connect (cb, "toggled", G_CALLBACK (plugin_show_windowlist_icons_changed), wl);
    gtk_box_pack_start (GTK_BOX (mainvbox), cb, FALSE, FALSE, 0);

    gtk_widget_show_all (dlg);
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

    wl->screen = netk_screen_get_default ();
    netk_screen_force_update (wl->screen);

    wl->button = xfce_create_panel_toggle_button ();
    gtk_widget_show (wl->button);

    pb = gtk_widget_render_icon (GTK_WIDGET (plugin), GTK_STOCK_MISSING_IMAGE,
                                 GTK_ICON_SIZE_MENU, NULL);
    wl->img = xfce_scaled_image_new_from_pixbuf (pb);
    gtk_widget_show (wl->img);
    gtk_container_add (GTK_CONTAINER (wl->button), wl->img);
    g_object_unref (pb);

    xfce_panel_plugin_add_action_widget (plugin, wl->button);
    g_signal_connect (wl->button, "button-press-event",
                      G_CALLBACK (plugin_button_clicked), wl);
    
    /* set icon */
    plugin_active_window_changed (NULL, wl);
    
    wl->screen_callback_id =
        g_signal_connect (wl->screen, "active-window-changed",
                          G_CALLBACK (plugin_active_window_changed), wl);

    gtk_container_add (GTK_CONTAINER (plugin), wl->button);

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
