/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *  Copyright (c) 2006 Jani Monoses <jani@ubuntu.com> 
 *  Copyright (c) 2006 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
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

#define DEF_BUTTON_LAYOUT		0
#define DEF_SHOW_ALL_WORKSPACES		TRUE
#define DEF_SHOW_WINDOW_ICONS		TRUE
#define DEF_SHOW_WORKSPACE_ACTIONS	FALSE
#define DEF_NOTIFY			1

#define ARROW_WIDTH 	16
#define SEARCH_TIMEOUT	1000
#define FLASH_TIMEOUT	500

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfcegui4/netk-window-action-menu.h>

#include "windowlist.h"
#include "windowlist-dialog.h"
#include "xfce4-popup-windowlist.h"

typedef enum { WS_ADD = 1, WS_REMOVE } WorkspaceAction;

static gboolean         windowlist_blink                (gpointer         data);
static gboolean         windowlist_set_size             (XfcePanelPlugin *plugin,
                                                         int              size,
                                                         Windowlist      *wl);
static void             windowlist_construct            (XfcePanelPlugin *plugin);



/**
 * REGISTER PLUGIN
 **/
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (windowlist_construct);



/**
 * Common functions
 **/
static char *
menulist_utf8_string (const char *string)
{
    char *utf8 = NULL;
    if (!g_utf8_validate(string, -1, NULL))
    {
        utf8 = g_locale_to_utf8(string, -1, NULL, NULL, NULL);
        DBG("Title was not UTF-8 complaint");
    }
    else
    {
        utf8 = g_strdup (string);
    }

    if (!utf8)
        utf8 = g_strdup ("?");

    return utf8;
}



static gchar *
menulist_workspace_name (NetkWorkspace *workspace,
                         const gchar   *num_title,
                         const gchar   *name_title)
{
    const gchar *ws_name = NULL;
    gchar       *ws_title;
    gint         ws_num;

    ws_num  = netk_workspace_get_number (workspace);    
    ws_name = netk_workspace_get_name (workspace);

    if (!ws_name || strtol((const char *)ws_name, NULL, 0) == ws_num + 1)
        ws_title = g_strdup_printf (num_title, ws_num + 1);
    else
        ws_title = g_markup_printf_escaped (name_title, ws_name);

    return ws_title;
}



/**
 * Menu Actions
 **/
static void
menu_deactivated (GtkWidget *menu,
                  GtkWidget *button)
{
    DBG ("Destroy menu");

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  FALSE);
    if (menu)
        gtk_widget_destroy (menu);
}



static gboolean
menulist_goto_workspace (GtkWidget      *mi,
                         GdkEventButton *ev,
                         NetkWorkspace  *workspace)
{
    netk_workspace_activate (workspace);

    return FALSE;
}



static void
window_destroyed (gpointer  data,
                  GObject  *object)
{
    GtkWidget *mi   = data;
    GtkWidget *menu = gtk_widget_get_parent(mi);

    if(mi && menu)
        gtk_container_remove (GTK_CONTAINER(menu), mi);

    gtk_menu_reposition (GTK_MENU(menu));
}



static void
mi_destroyed (GtkObject *object,
              gpointer   data)
{
    g_object_weak_unref (G_OBJECT(data),
                         (GWeakNotify)window_destroyed, object);
}



static void
action_menu_deactivated (GtkMenu *menu,
                         GtkMenu *parent)
{
    DBG ("Destroy Action Menu");

    gtk_menu_popdown (parent);
    g_signal_emit_by_name (parent, "deactivate", 0);
}



static void
popup_action_menu (GtkWidget  *widget,
                   NetkWindow *window)
{
    static GtkWidget *menu = NULL;

    if (menu)
        gtk_widget_destroy (menu);

    menu = netk_create_window_action_menu (window);

    g_signal_connect (menu, "deactivate",
                      G_CALLBACK (action_menu_deactivated), widget->parent);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0,
                    gtk_get_current_event_time());
}



static gboolean
menulist_goto_window (GtkWidget      *mi,
                      GdkEventButton *ev,
                      NetkWindow     *window)
{
    if (ev->button == 1) /* Goto workspace and show window */
    {
        gtk_menu_popdown (GTK_MENU (mi->parent));
        if (!netk_window_is_sticky (window))
        {
            netk_workspace_activate(netk_window_get_workspace(window));
        }
        netk_window_activate (window);
        g_signal_emit_by_name (mi->parent, "deactivate", 0);
    }
    else if (ev->button == 2) /* Show window on current workspace */
    {
        gtk_menu_popdown (GTK_MENU (mi->parent));
        netk_window_activate (window);
        g_signal_emit_by_name (mi->parent, "deactivate", 0);
    }
    else if (ev->button == 3) /* Show the popup menu */
    {
        popup_action_menu (mi, window);

        /* the right-click menu will pop down the main menu */
        return TRUE;
    }

    return FALSE;
}



static gboolean
menulist_add_screen (GtkWidget      *mi,
                     GdkEventButton *ev,
                     Windowlist     *wl)
{
    gint num = netk_screen_get_workspace_count (wl->screen) + 1;

    netk_screen_change_workspace_count (netk_screen_get_default (), num);

    return FALSE;
}



static gboolean
menulist_remove_screen (GtkWidget      *mi,
                        GdkEventButton *ev,
                        Windowlist     *wl)
{
    NetkWorkspace *workspace;
    gint           ws_num;
    char          *text;

    ws_num    = netk_screen_get_workspace_count (wl->screen) - 1;
    workspace = netk_screen_get_workspace (wl->screen, ws_num);

    text = menulist_workspace_name (workspace,
            _("Are you sure you want to remove workspace %d?"),
            _("Are you sure you want to remove workspace '%s'?"));

    if (xfce_confirm (text, GTK_STOCK_REMOVE, NULL))
    {
        netk_screen_change_workspace_count (netk_screen_get_default (),
                                            ws_num);
    }

    g_free (text);

    return FALSE;
}



static gboolean
menulist_keypress (GtkWidget   *menu, 
                   GdkEventKey *ev,
                   Windowlist  *wl)
{
    GdkEventButton  evb;
    GList          *l;
    GtkWidget      *mi = NULL;
    NetkWindow     *window;
    NetkWorkspace  *workspace;
    gpointer        ws_action;
    guint           state;

    for (l = GTK_MENU_SHELL (menu)->children; l != NULL; l = l->next)
    {
        if (GTK_WIDGET_STATE (l->data) == GTK_STATE_PRELIGHT)
        {
            mi = l->data;
            break;
        }
    }

    if (mi == NULL)
        return FALSE;

    state = ev->state & gtk_accelerator_get_default_mod_mask ();

    switch (ev->keyval)
    {
        case GDK_space:
        case GDK_Return:
        case GDK_KP_Space:
        case GDK_KP_Enter:
            evb.button = 1;
            break;
        case GDK_Menu:
            evb.button = 3;
            break;
        default:
            return FALSE;
    }

    if (evb.button == 1)
    {
        if (state == GDK_SHIFT_MASK)
            evb.button = 2;
        else if (state == GDK_CONTROL_MASK)
            evb.button = 3;
    }

    if((window = g_object_get_data (G_OBJECT (mi), "netk-window")) != NULL)
    {
        if (!NETK_IS_WINDOW (window))
            return FALSE;

        return menulist_goto_window (mi, &evb, window);
    }
    else if (evb.button == 1 && 
             (workspace = g_object_get_data (G_OBJECT (mi), "netk-workspace")) 
             != NULL)
    {
        if (!NETK_IS_WORKSPACE (workspace))
            return FALSE;

        return menulist_goto_workspace (mi, NULL, workspace);
    }
    else if (evb.button == 1 && 
             (ws_action = g_object_get_data (G_OBJECT (mi), "ws-action"))
             != NULL)
    {
        if (GPOINTER_TO_INT (ws_action) == WS_REMOVE)
        {
            return menulist_remove_screen (mi, NULL, wl);
        }
        else 
        {
            return menulist_add_screen (mi, NULL, wl);
        }
    }

    return FALSE;
}



/**
 * Window List Menu functions
 **/
static GtkWidget *
menulist_menu_item (NetkWindow *window,
                    Windowlist *wl,
                    gint        size)
{
    GtkWidget *mi;
    char      *window_name = NULL;
    GString   *label;
    GdkPixbuf *icon = NULL;
    GdkPixbuf *tmp = NULL;

    window_name = menulist_utf8_string (netk_window_get_name (window));
    label = g_string_new (window_name);

    if (netk_window_is_minimized (window))
    {
        g_string_prepend (label, "[");
        g_string_append (label, "]");
    }

    /* hack: italic fonts are not completely shown, otherwise */
    g_string_append (label, " ");

    if (wl->show_window_icons)
        icon = netk_window_get_icon (window);

    if (icon)
    {
        GtkWidget *img;
        gint w, h;

        w = gdk_pixbuf_get_width (icon);
        h = gdk_pixbuf_get_height (icon);

        if (G_LIKELY (w > size || h > size))
        {
            tmp = gdk_pixbuf_scale_simple (icon, size, size, 
                                           GDK_INTERP_BILINEAR);
            icon = tmp;
        }

        mi = gtk_image_menu_item_new_with_label (label->str);

        img = gtk_image_new_from_pixbuf (icon);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        if (G_LIKELY (tmp))
            g_object_unref (G_OBJECT (tmp));
    }
    else
    {
        mi = gtk_menu_item_new_with_label (label->str);
    }

    gtk_label_set_ellipsize (GTK_LABEL (GTK_BIN (mi)->child), 
                             PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars (GTK_LABEL (GTK_BIN (mi)->child), 24);

    gtk_tooltips_set_tip (wl->tooltips, mi, window_name, NULL);

    g_string_free (label, TRUE);
    g_free (window_name);

    return mi;
}



static gboolean
menulist_popup_menu (Windowlist     *wl,
                     GdkEventButton *ev,
                     gboolean        at_pointer)
{
    GtkWidget            *menu, *mi, *icon;
    NetkWindow           *window;
    NetkWorkspace        *netk_workspace;
    NetkWorkspace        *active_workspace;
    NetkWorkspace        *window_workspace;
    gchar                *ws_label, *rm_label;
    gint                  size, i, wscount;
    GList                *windows, *li;
    PangoFontDescription *italic, *bold;

    /* Menu item styles */
    italic = pango_font_description_from_string ("italic");
    bold   = pango_font_description_from_string ("bold");

    menu = gtk_menu_new ();

    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, NULL);

    windows = netk_screen_get_windows_stacked (wl->screen);
    active_workspace = netk_screen_get_active_workspace (wl->screen);

    if (wl->show_all_workspaces)
        wscount = netk_screen_get_workspace_count (wl->screen);
    else
        wscount = 1;

    /* For Each Workspace */
    for (i = 0; i < wscount; i++)
    {
        /* Load workspace */
        if (wl->show_all_workspaces)
            netk_workspace = netk_screen_get_workspace (wl->screen, i);
        else
            netk_workspace = netk_screen_get_active_workspace (wl->screen);

        /* Create workspace menu item */
        ws_label = 
            menulist_workspace_name (netk_workspace, _("Workspace %d"), "%s");

        mi = gtk_menu_item_new_with_label (ws_label);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        g_free (ws_label);

        g_object_set_data (G_OBJECT (mi), "netk-workspace", netk_workspace);

        g_signal_connect (mi, "button-release-event",
                          G_CALLBACK (menulist_goto_workspace), 
                          netk_workspace);

        /* Apply layout */
        if (netk_workspace == active_workspace)
            gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), bold);
        else
            gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), italic); 

        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        /* Foreach Window */
        for (li = windows; li; li = li->next)
        {
            /* If window is not on current workspace; continue */
            window = li->data;
            window_workspace = netk_window_get_workspace (window);

            if (netk_workspace != window_workspace && 
                !(netk_window_is_sticky (window) && 
                netk_workspace == active_workspace))
            {
                continue;
            }

            if (netk_window_is_skip_pager (window) || 
                netk_window_is_skip_tasklist (window))
            {
                continue;
            }

            /* Create menu item */
            mi = menulist_menu_item (window, wl, size);

            if (G_UNLIKELY (!mi))
                continue;

            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

            if (netk_window_is_active (window))
            {
                gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)), 
                        italic);
            }

            /* Apply some styles for windows on !current workspace and 
             * if they are urgent */
            if (netk_window_or_transient_demands_attention (window))
            {
                if (netk_workspace == active_workspace)
                {
                    gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)),
                                            bold);
                }
                else
                {
                    gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (mi)),
                                          GTK_STATE_NORMAL,
                                          &(menu->style->fg[GTK_STATE_INSENSITIVE]));
                    gtk_widget_modify_font (gtk_bin_get_child (GTK_BIN (mi)),
                                            bold);
                }
            }
            else if (netk_workspace != active_workspace && 
                     !netk_window_is_sticky (window))
            {
                gtk_widget_modify_fg (gtk_bin_get_child (GTK_BIN (mi)),
                        GTK_STATE_NORMAL,
                        &(menu->style->fg[GTK_STATE_INSENSITIVE]));
            }

            g_object_set_data (G_OBJECT (mi), "netk-window", window);

            /* Connect some signals */
            g_signal_connect (mi, "button-release-event",
                              G_CALLBACK (menulist_goto_window), window);

            g_object_weak_ref(G_OBJECT(window),
                              (GWeakNotify)window_destroyed, mi);

            g_signal_connect(G_OBJECT(mi), "destroy",
                             G_CALLBACK(mi_destroyed), window);
        }

        if (i < wscount-1)
        {
            mi = gtk_separator_menu_item_new ();
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        }
    }

    pango_font_description_free(italic);
    pango_font_description_free(bold);

    /* Show add/remove workspace buttons */
    if (wl->show_workspace_actions)
    {
        int ws_action;

        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        /* Add workspace */
        if (wl->show_window_icons)
        {
            mi = gtk_image_menu_item_new_with_label (_("Add workspace"));
            icon = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), icon);
        }
        else
        {
            mi = gtk_menu_item_new_with_label (_("Add workspace"));
        }

        ws_action = WS_ADD;
        g_object_set_data (G_OBJECT (mi), "ws-action", GINT_TO_POINTER (ws_action));

        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        g_signal_connect (mi, "button-release-event",
                          G_CALLBACK (menulist_add_screen), wl);

        /* Remove workspace */
        wscount = netk_screen_get_workspace_count (wl->screen);

        if (wscount > 1)
        {
            netk_workspace = netk_screen_get_workspace (wl->screen, wscount-1);

            rm_label = menulist_workspace_name (netk_workspace,
                                                _("Remove Workspace %d"),
                                                _("Remove Workspace '%s'"));

            if (wl->show_window_icons)
            {
                mi = gtk_image_menu_item_new_with_label (rm_label);
                icon = gtk_image_new_from_stock (GTK_STOCK_REMOVE, 
                                                 GTK_ICON_SIZE_MENU);
                gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), icon);
            }
            else
            {
                mi = gtk_menu_item_new_with_label (rm_label);
            }

            g_free (rm_label);

            ws_action = WS_REMOVE;
            g_object_set_data (G_OBJECT (mi), "ws-action", GINT_TO_POINTER (ws_action));

            gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

            g_signal_connect (mi, "button-release-event",
                              G_CALLBACK (menulist_remove_screen), wl);
        }
    }

    /* key presses work on the menu, not the items */
    g_signal_connect (menu, "key-press-event",
                      G_CALLBACK (menulist_keypress), 
                      wl);

    /* Activate toggle button */
    if (!at_pointer)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wl->button), TRUE);

    /* Connect signal, show widgets and popup */
    g_signal_connect (menu, "deactivate",
                      G_CALLBACK (menu_deactivated), wl->button);

    gtk_widget_show_all (menu);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                    at_pointer ? NULL : xfce_panel_plugin_position_menu,
                    at_pointer ? NULL : wl->plugin,
                    ev ? ev->button : 0, 
                    ev ? ev->time : gtk_get_current_event_time());

    return TRUE;
}



static gboolean
menulist_toggle_menu (GtkToggleButton *button,
                      GdkEventButton  *ev, 
                      Windowlist      *wl)
{
    if (ev->button != 1)
        return FALSE;
    return menulist_popup_menu (wl, ev, FALSE);
}



/**
 * Check for urgent on workspaces
 **/
static gboolean
windowlist_search_urgent (gpointer data)
{
    Windowlist    *wl = (Windowlist *) data;
    NetkWindow    *window;
    NetkWorkspace *active_workspace;
    NetkWorkspace *window_workspace;
    gboolean       blink = FALSE;
    GList         *windows, *li;

    windows = netk_screen_get_windows_stacked (wl->screen);
    active_workspace = netk_screen_get_active_workspace (wl->screen);

    /* For Each Window (stop when we've found an urgent window) */
    for (li = windows; li && !blink; li = li->next)
    {
        window = li->data;
        window_workspace = netk_window_get_workspace (window);

        /* Don't check for urgent windows on current workspace
           if enabled in properties */
        if (window_workspace == active_workspace && 
            wl->notify == OTHER_WORKSPACES)
        {
            continue;
        }

        /* Skip windows that are not in the tasklist */	
        if (netk_window_is_sticky (window)	   ||
            netk_window_is_skip_pager (window)	   ||
            netk_window_is_skip_tasklist (window))
        {
            continue;
        }

        /* Check if window is urgent */
        if (netk_window_or_transient_demands_attention (window))
            blink = TRUE;
    }

    wl->blink = blink;

    if (G_UNLIKELY (blink && !wl->blink_timeout_id))
    {
        wl->blink_timeout_id = 
            g_timeout_add (FLASH_TIMEOUT, windowlist_blink, wl);

        DBG ("New blink source started: %d", wl->blink_timeout_id);

        /* Start the blink and don't wait 0,5 second */
        windowlist_blink (wl);
    }


    if (G_UNLIKELY (!blink && wl->blink_timeout_id))
    {
        DBG ("Blink source stopped: %d", wl->blink_timeout_id);

        g_source_remove (wl->blink_timeout_id);
        wl->blink_timeout_id = 0;
        windowlist_blink (wl);
    }

    return TRUE;
}



void
windowlist_start_blink (Windowlist * wl)
{
    /* Stop the Search function */
    if (wl->search_timeout_id)
    {
        DBG ("Search source stopped: %d", wl->search_timeout_id);
        g_source_remove (wl->search_timeout_id);
        wl->search_timeout_id = 0;
    }

    /* Stop the blink if it's running */
    if (wl->blink_timeout_id)
    {
        DBG ("Blink source stopped: %d", wl->blink_timeout_id);
        g_source_remove (wl->blink_timeout_id);
        wl->blink_timeout_id = 0;
    }

    /* force normal button normal */
    wl->blink = FALSE;

    if (wl->notify != DISABLED)
    {
        wl->search_timeout_id =
            g_timeout_add (SEARCH_TIMEOUT, windowlist_search_urgent, wl);

        DBG ("New search source started: %d", wl->search_timeout_id);

        windowlist_search_urgent (wl);
    }

    /* Make sure button is normal */
    windowlist_blink (wl);
}



/**
 * Button functions
 **/
static gboolean
windowlist_blink (gpointer data)
{
    Windowlist * wl = (Windowlist *) data;

    GtkStyle *style;
    GtkRcStyle *mod;
    GdkColor c;

    g_return_val_if_fail (wl, FALSE);
    g_return_val_if_fail (wl->button, FALSE);

    style = gtk_widget_get_style (wl->button);
    mod = gtk_widget_get_modifier_style (wl->button);
    c = style->bg[GTK_STATE_SELECTED];

    if(wl->blink && !wl->block_blink)
    {
        /* Paint the button */

        gtk_button_set_relief (GTK_BUTTON (wl->button),
                GTK_RELIEF_NORMAL);

        if(mod->color_flags[GTK_STATE_NORMAL] & GTK_RC_BG)
        {
            mod->color_flags[GTK_STATE_NORMAL] &= ~(GTK_RC_BG);
            gtk_widget_modify_style(wl->button, mod);
        }
        else
        {
            mod->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;
            mod->bg[GTK_STATE_NORMAL] = c;
            gtk_widget_modify_style(wl->button, mod);
        }
    }
    else
    {
        if (!wl->blink)
            gtk_button_set_relief (GTK_BUTTON (wl->button),
                                   GTK_RELIEF_NONE);

        mod->color_flags[GTK_STATE_NORMAL] &= ~(GTK_RC_BG);
        gtk_widget_modify_style(wl->button, mod);
    }

    return wl->blink;
}



/**
 * This will block the blink function on mouse over
 **/
static void
windowlist_state_changed (GtkWidget *button,
                          GtkStateType state,
                          Windowlist * wl)
{
    if (G_LIKELY(wl->notify == DISABLED || wl->blink == FALSE))
        return;

    DBG ("Button State changed");

    if (GTK_WIDGET_STATE (button) == 0)
        wl->block_blink = FALSE;

    else   
    {
        wl->block_blink = TRUE;
        windowlist_blink (wl);
    }
}



/**
 *
 **/
static void
windowlist_active_window_changed (GtkWidget  *w,
                                  Windowlist *wl)
{
    NetkWindow *win;
    GdkPixbuf  *pb;

    if ((win = netk_screen_get_active_window (wl->screen)) != NULL)
    {
        pb = netk_window_get_icon (win);
        if (pb)
        {
            xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (wl->icon), 
                                               pb);
            gtk_tooltips_set_tip (wl->tooltips, wl->button,
                                  netk_window_get_name (win), NULL);
        }
    }
}



/**
 * Handle user messages
 **/
static gboolean
wl_message_received (GtkWidget      *w, 
                     GdkEventClient *ev, 
                     gpointer        user_data)
{
    Windowlist *wl = user_data;

    if (ev->data_format == 8 && *(ev->data.b) != '\0')
    {
        if (!strcmp (XFCE_WINDOW_LIST_MESSAGE, ev->data.b))
            return menulist_popup_menu (wl, NULL, FALSE);

        if (!strcmp (XFCE_WINDOW_LIST_AT_POINTER_MESSAGE, ev->data.b))
            return menulist_popup_menu (wl, NULL, TRUE);
    }

    return FALSE;
}



static gboolean
wl_set_selection (Windowlist *wl)
{
    GdkScreen *gscreen;
    gchar      selection_name[32];
    Atom       selection_atom;
    GtkWidget *win;
    Window     xwin;

    win = gtk_invisible_new ();
    gtk_widget_realize (win);
    xwin = GDK_WINDOW_XID (GTK_WIDGET (win)->window);

    gscreen = gtk_widget_get_screen (win);
    g_snprintf (selection_name, sizeof (selection_name), 
                XFCE_WINDOW_LIST_SELECTION"%d", gdk_screen_get_number (gscreen));
    selection_atom = XInternAtom (GDK_DISPLAY (), selection_name, False);

    if (XGetSelectionOwner (GDK_DISPLAY (), selection_atom))
    {
        gtk_widget_destroy (win);
        return FALSE;
    }

    XSelectInput (GDK_DISPLAY (), xwin, PropertyChangeMask);
    XSetSelectionOwner (GDK_DISPLAY (), selection_atom, xwin, GDK_CURRENT_TIME);

    g_signal_connect (G_OBJECT (win), "client-event",
                      G_CALLBACK (wl_message_received), wl);

    return TRUE;
}



/**
 * Build the panel button and connect signals and styles
 **/
void
windowlist_create_button (Windowlist *wl)
{
    GdkPixbuf *pb;

    if (wl->button)
        gtk_widget_destroy (wl->button);

    if (wl->screen_callback_id)
    {
        g_signal_handler_disconnect (wl->screen, wl->screen_callback_id);
        wl->screen_callback_id = 0;
    }

    switch (wl->layout)
    {
        case ICON_BUTTON:
            DBG ("Create Icon Button");

            wl->button = gtk_toggle_button_new ();

            pb = gtk_widget_render_icon (GTK_WIDGET (wl->plugin),
                                         GTK_STOCK_MISSING_IMAGE,
                                         GTK_ICON_SIZE_MENU, NULL);
            wl->icon = xfce_scaled_image_new_from_pixbuf (pb);
            gtk_container_add (GTK_CONTAINER (wl->button), wl->icon);
            g_object_unref (G_OBJECT (pb));

            wl->screen_callback_id =
                g_signal_connect (wl->screen, "active-window-changed",
                                  G_CALLBACK (windowlist_active_window_changed), wl);

            windowlist_active_window_changed (wl->button, wl);

            break;

        case ARROW_BUTTON:
            DBG ("Create Arrow Button");

            wl->arrowtype = xfce_panel_plugin_arrow_type (wl->plugin);
            wl->button = xfce_arrow_button_new (wl->arrowtype);

            break;
    }

    /* Button layout */
    GTK_WIDGET_UNSET_FLAGS (wl->button, GTK_CAN_DEFAULT|GTK_CAN_FOCUS);
    gtk_button_set_relief (GTK_BUTTON (wl->button), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (wl->button), FALSE);

    /* Set the button sizes again */
    windowlist_set_size (wl->plugin,
                         xfce_panel_plugin_get_size (wl->plugin),
                         wl);

    /* Show, add and connect signals */
    g_signal_connect (wl->button, "button-press-event",
                      G_CALLBACK (menulist_toggle_menu), wl);

    g_signal_connect (wl->button, "state-changed",
                      G_CALLBACK (windowlist_state_changed), wl);

    wl_set_selection (wl);

    gtk_widget_show_all (wl->button);
    gtk_container_add (GTK_CONTAINER (wl->plugin), wl->button);

    /* Add action widget */
    xfce_panel_plugin_add_action_widget (wl->plugin, wl->button);
}



/**
 * Read and write user settings
 **/
static void
windowlist_read (Windowlist *wl)
{
    XfceRc *rc;
    gchar  *file;

    if (!(file = xfce_panel_plugin_lookup_rc_file (wl->plugin)))
        return;

    DBG("Read from file: %s", file);

    rc = xfce_rc_simple_open (file, TRUE);
    g_free (file);

    if (!rc)
        return;

    switch (xfce_rc_read_int_entry (rc, "button_layout", DEF_BUTTON_LAYOUT))
    {
        case 0: 
            wl->layout = ICON_BUTTON;
            break;

        default: 
            wl->layout = ARROW_BUTTON;
            break;
    }

    switch (xfce_rc_read_int_entry (rc, "urgency_notify", DEF_NOTIFY))
    {
        case 0: 
            wl->notify = DISABLED;
            break;

        case 1:
            wl->notify = OTHER_WORKSPACES;
            break;

        default: 
            wl->notify = ALL_WORKSPACES;
            break;
    }

    wl->show_all_workspaces = 
        xfce_rc_read_bool_entry (rc, "show_all_workspaces",	
                                 DEF_SHOW_ALL_WORKSPACES);
    wl->show_window_icons = 
        xfce_rc_read_bool_entry (rc, "show_window_icons",	
                                 DEF_SHOW_WINDOW_ICONS);
    wl->show_workspace_actions = 
        xfce_rc_read_bool_entry (rc, "show_workspace_actions",	
                                 DEF_SHOW_WORKSPACE_ACTIONS);

    xfce_rc_close (rc);
}



static void
windowlist_write (XfcePanelPlugin *plugin,
                  Windowlist      *wl)
{
    XfceRc *rc;
    gchar  *file;

    if (!(file = xfce_panel_plugin_save_location (wl->plugin, TRUE)))
        return;

    DBG("Write to file: %s", file);

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    switch (wl->layout)
    {
        case ICON_BUTTON:
            xfce_rc_write_int_entry (rc, "button_layout", 0);
            break;

        case ARROW_BUTTON:
            xfce_rc_write_int_entry (rc, "button_layout", 1);
            break;
    }

    switch (wl->notify)
    {
        case DISABLED:
            xfce_rc_write_int_entry (rc, "urgency_notify", 0);
            break;

        case OTHER_WORKSPACES:
            xfce_rc_write_int_entry (rc, "urgency_notify", 1);
            break;

        case ALL_WORKSPACES:
            xfce_rc_write_int_entry (rc, "urgency_notify", 2);
            break;
    }

    xfce_rc_write_bool_entry (rc, "show_all_workspaces",	
                              wl->show_all_workspaces);
    xfce_rc_write_bool_entry (rc, "show_window_icons",		
                              wl->show_window_icons);
    xfce_rc_write_bool_entry (rc, "show_workspace_actions",	
                              wl->show_workspace_actions);

    xfce_rc_close (rc);
}



/**
 * Initialize plugin
 **/
static Windowlist *
windowlist_new (XfcePanelPlugin *plugin)
{
    GdkScreen *screen;
    int screen_idx;

    Windowlist *wl = panel_slice_new0 (Windowlist);

    /* Some default values if everything goes wrong */
    wl->layout			= DEF_BUTTON_LAYOUT;
    wl->show_all_workspaces	= DEF_SHOW_ALL_WORKSPACES;
    wl->show_window_icons	= DEF_SHOW_WINDOW_ICONS;
    wl->show_workspace_actions	= DEF_SHOW_WORKSPACE_ACTIONS;
    wl->notify			= DEF_NOTIFY;

    /* Reset */
    wl->screen_callback_id = 0;

    /* Reset Urgency Stuff */
    wl->search_timeout_id = 0;
    wl->blink_timeout_id = 0;
    wl->blink = FALSE;
    wl->block_blink = FALSE;

    wl->plugin = plugin;

    wl->tooltips = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (wl->tooltips));
    gtk_object_sink (GTK_OBJECT (wl->tooltips));

    /* get the screen where the widget is, for dual screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
    screen_idx = gdk_screen_get_number (screen);

    wl->screen = netk_screen_get (screen_idx);

    /* Read user settings */
    windowlist_read (wl);

    /* Build the panel button */

    windowlist_create_button (wl);

    return wl;
}



/**
 * Common plugin functions
 **/
static gboolean
windowlist_set_size (XfcePanelPlugin *plugin,
                     int              size,
                     Windowlist      *wl)
{
    DBG ("Size: %d", size);

    switch (wl->layout)
    {
        case ICON_BUTTON:
            gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                         size, size);
            break;

        case ARROW_BUTTON:
            switch (wl->arrowtype)
            {
		case GTK_ARROW_LEFT:
		case GTK_ARROW_RIGHT:
                    gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                                 size, ARROW_WIDTH);
                    break;

		case GTK_ARROW_UP:
		case GTK_ARROW_DOWN:
                    gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                                 ARROW_WIDTH, size);
                    break;

                default:
                    break;
            }
            break;

    }

    return TRUE;
}



static void
windowlist_free (XfcePanelPlugin *plugin,
                 Windowlist      *wl)
{
    g_object_unref (G_OBJECT (wl->tooltips));

    if (wl->screen_callback_id)
        g_signal_handler_disconnect (wl->screen, wl->screen_callback_id);

    if (wl->search_timeout_id)
    {
        DBG ("Stop urgent source: %d", wl->search_timeout_id);
        g_source_remove (wl->search_timeout_id);
        wl->search_timeout_id = 0;
    }

    if (wl->blink_timeout_id)
    {
        DBG ("Stop blink source: %d", wl->blink_timeout_id);
        g_source_remove (wl->blink_timeout_id);
        wl->blink_timeout_id = 0;
    }

    if (wl->icon)
        gtk_widget_destroy (wl->icon);

    if (wl->button)
        gtk_widget_destroy (wl->button);

    panel_slice_free (Windowlist, wl);
}



static void
windowlist_screen_position_changed (XfcePanelPlugin    *plugin, 
                                    XfceScreenPosition  position, 
                                    Windowlist         *wl)
{
    DBG ("...");

    wl->arrowtype = xfce_panel_plugin_arrow_type (plugin);
    
    if (wl->layout == ARROW_BUTTON)
	xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (wl->button),
                                          wl->arrowtype);
}



/**
 * Construct plugin
 **/
static void
windowlist_construct (XfcePanelPlugin *plugin)
{
    Windowlist *wl = windowlist_new (plugin);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (windowlist_free), wl);

    g_signal_connect (plugin, "save",
                      G_CALLBACK (windowlist_write), wl);

    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (windowlist_set_size), wl);

    g_signal_connect (plugin, "screen-position-changed", 
                      G_CALLBACK (windowlist_screen_position_changed), wl);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (windowlist_properties), wl);


    /* Start Urgency Search (if enabled ^_^) */
    windowlist_start_blink (wl);
}

