/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-client.h>

#include "xfce-panel-window.h"

#include "iconbox-settings.h"


#define N_ICONBOX_CONNECTIONS  4
#define N_ICON_CONNECTIONS     4

#define DEFAULT_JUSTIFICATION  GTK_JUSTIFY_LEFT
#define DEFAULT_SIDE           GTK_SIDE_BOTTOM
#define DEFAULT_SIZE           32
#define DEFAULT_OPACITY        0xc0000000;
#define OPAQUE                 0xffffffff

#define IS_HORIZONTAL(s) (s == GTK_SIDE_TOP || s == GTK_SIDE_BOTTOM)

/* the iconbox */
struct _Iconbox
{
    McsClient *mcs_client;
    SessionClient *session_client;

    Display *dpy;
    int scr;

    GdkScreen *gdk_screen;
    int monitor;
    
    NetkScreen *netk_screen;
    int connections[N_ICONBOX_CONNECTIONS];

    int x, y;
    int width, height;
    int offset;
    int block_resize;
    gboolean in_move;

    guint opacity;
    
    GtkWidget *menu;
    GtkWidget *win;
    GtkWidget *box;

    GSList *iconlist;

    GtkJustification justification;
    GtkSideType side;
    int icon_size;
    gboolean only_hidden;
    gboolean all_workspaces;
    gboolean hide;
};

/* icons */
typedef struct _Icon Icon;

struct _Icon
{
    NetkWindow *window;
    int connections[N_ICON_CONNECTIONS];

    Iconbox *ib;
    gboolean skip;
    
    GdkPixbuf *pb;

    GtkWidget *button;
    GtkWidget *image;
};

/* globals */
static GtkTooltips *icon_tooltips = NULL;

static gboolean sig_quit = FALSE;


/* transparency */
static void
iconbox_set_transparent (Iconbox *ib, gboolean transparent)
{
    guint opacity = (transparent ? ib->opacity : OPAQUE);
    
    gdk_error_trap_push ();

    gdk_property_change (ib->win->window,
			 gdk_atom_intern ("_NET_WM_WINDOW_OPACITY", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & opacity, 1L);

    gdk_error_trap_pop ();

    gtk_widget_queue_draw (ib->win);
}

static gboolean
iconbox_enter (GtkWidget *w, GdkEventCrossing *event, gpointer data)
{
    Iconbox *ib = data;
    
    if (event->detail != GDK_NOTIFY_INFERIOR && ib->opacity != OPAQUE)
    {
        iconbox_set_transparent (ib, FALSE);
    }
    
    return FALSE;
}

static gboolean
iconbox_leave (GtkWidget *w, GdkEventCrossing *event, gpointer data)
{
    Iconbox *ib = data;
    
    if (event->detail != GDK_NOTIFY_INFERIOR && ib->opacity != OPAQUE)
    {
        iconbox_set_transparent (ib, TRUE);
    }
    
    return FALSE;
}

/* icons */
static void
icon_update_image (Icon *icon)
{
    GdkPixbuf *scaled;
    int w, h;

    g_return_if_fail (GDK_IS_PIXBUF (icon->pb));
    
    w = gdk_pixbuf_get_width (icon->pb);
    h = gdk_pixbuf_get_height (icon->pb);

    if (w > icon->ib->icon_size || h > icon->ib->icon_size ||
        ABS (w - icon->ib->icon_size) > 20 || 
        ABS (h - icon->ib->icon_size) > 20)
    {
        if (w > h)
        {
            h = rint ((double) h * icon->ib->icon_size / (double) w);

            w = icon->ib->icon_size;
        }
        else
        {
            w = rint ((double) w * icon->ib->icon_size / (double) h);

            h = icon->ib->icon_size;
        }

        scaled = gdk_pixbuf_scale_simple (icon->pb, w, h, 
                                          GDK_INTERP_BILINEAR);
    }
    else
    {
        scaled = gdk_pixbuf_copy (icon->pb);
    }
    
    if (scaled && netk_window_is_minimized (icon->window))
    {
        /* copied from netk-tasklist.c: dimm_icon */
        int x, y, pixel_stride, row_stride;
        guchar *row, *pixels;

        g_assert (gdk_pixbuf_get_has_alpha (scaled));

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
    }
    
    gtk_image_set_from_pixbuf (GTK_IMAGE (icon->image), scaled);

    if (scaled)
        g_object_unref (scaled);
}

static void 
update_visibility (Icon *icon, NetkWorkspace *optional_active_ws)
{
    NetkWorkspace *ws;

    if (icon->skip)
    {
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

/* callbacks */
static gboolean
icon_button_pressed (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (ev->button == 1 && !(ev->state & GDK_SHIFT_MASK))
    {
        if (netk_window_is_active (icon->window))
            netk_window_minimize (icon->window);
        else
            netk_window_activate (icon->window);
    }

    return TRUE;
}

static void
icon_name_changed (NetkWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    gtk_tooltips_set_tip (icon_tooltips, icon->button, 
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
    NetkWindowType type;

    icon->ib = ib;
        
    icon->window = window;    
    icon->pb = netk_window_get_icon (window);
    if (icon->pb)
        g_object_ref (icon->pb);

    icon->button = gtk_toggle_button_new ();
    gtk_button_set_relief (GTK_BUTTON (icon->button), GTK_RELIEF_NONE);
    
    g_signal_connect (icon->button, "button-press-event",
                      G_CALLBACK (icon_button_pressed), icon);
    
    icon->image = gtk_image_new ();
    gtk_widget_show (icon->image);
    gtk_container_add (GTK_CONTAINER (icon->button), icon->image);
    
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
    
    type = netk_window_get_window_type (window);
    
    if (!(type == NETK_WINDOW_NORMAL || type == NETK_WINDOW_UTILITY ||
          type == NETK_WINDOW_DIALOG) ||            
        netk_window_is_skip_tasklist (window))
    {
        icon->skip = TRUE;
        return icon;
    }

    icon_update_image (icon);

    gtk_tooltips_set_tip (icon_tooltips, icon->button, 
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

    if (ib->justification != GTK_JUSTIFY_RIGHT)
        gtk_box_pack_start (GTK_BOX (ib->box), icon->button, FALSE, FALSE, 0);
    else
        gtk_box_pack_end (GTK_BOX (ib->box), icon->button, FALSE, FALSE, 0);
}

static void
iconbox_window_closed (NetkScreen *screen, NetkWindow *window, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l, *prev = NULL;
    
    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;
        
        if (window == icon->window)
        {
            gtk_widget_destroy (icon->button);
            icon_destroy (icon);

            if (prev)
                prev->next = l->next;
            else
                ib->iconlist = l->next;

            l->next = NULL;
            g_slist_free (l);

            break;
        }

        prev = l;
            
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


/* iconbox */

/* options */
static void
iconbox_check_options (int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; ++i)
    {
        if (*argv[i] != '-')
            break;

        /* check for useful options ;-) */
        
        if (!strcmp ("-V", argv[i]) || !strcmp ("--version", argv[i]) ||
            !strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
        {
            g_print ("\n"
                     " Xfce Iconbox " VERSION "\n"
                     " Licensed under the GNU GPL\n"
                     "\n"
                     " Usage: xfce4-iconbox [--version|-V] [--help|-h]\n"
                     "\n");

            exit (0);
        }
    }
}

/* right-click menu */
static void
menu_about (GtkWidget *w, Iconbox *ib)
{
    XfceAboutInfo *info;
    GtkWidget *dlg;
    GdkPixbuf *pb;

    info = xfce_about_info_new ("xfce4-iconbox", VERSION, _("Xfce Iconbox"), 
                                XFCE_COPYRIGHT_TEXT ("2005", 
                                                     "Jasper Huijsmans"),
                                XFCE_LICENSE_GPL);

    xfce_about_info_set_homepage (info, "http://www.xfce.org");

    xfce_about_info_add_credit (info, "Jasper Huijsmans", "jasper@xfce.org",
                                _("Developer"));

    pb = xfce_themed_icon_load ("xfce4-iconbox", 32);
    dlg = xfce_about_dialog_new (GTK_WINDOW (ib->win), info, pb);
    g_object_unref (pb);

    gtk_widget_set_size_request (dlg, 400, 300);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dlg));

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    xfce_about_info_free (info);
}

static void
menu_properties (GtkWidget *w, Iconbox *ib)
{
    mcs_open_dialog (ib->gdk_screen, "iconbox");
}

static void
menu_quit (GtkWidget *w, Iconbox *ib)
{
    gtk_main_quit ();
}

static GtkWidget *
create_menu (Iconbox *ib)
{
    GtkWidget *menu, *mi;
    
    menu = gtk_menu_new ();

    mi = gtk_menu_item_new_with_mnemonic (_("_About"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    g_signal_connect (mi, "activate", G_CALLBACK (menu_about), ib);

    mi = gtk_menu_item_new_with_mnemonic (_("_Properties..."));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    g_signal_connect (mi, "activate", G_CALLBACK (menu_properties), ib);

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_menu_item_new_with_mnemonic (_("E_xit"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    g_signal_connect (mi, "activate", G_CALLBACK (menu_quit), ib);
    
    return menu;
}

static gboolean
iconbox_popup_menu (GtkWidget *w, GdkEventButton *ev, Iconbox *ib)
{
    if (ev->button == 3)
    {
        if (G_UNLIKELY (ib->menu == NULL))
        {
            ib->menu = create_menu (ib);
        }
        
        gtk_menu_popup (GTK_MENU (ib->menu), NULL, NULL, NULL, NULL,
                        ev->button, ev->time);
        return TRUE;
    }

    return FALSE;
}

/* widgets */
static void
iconbox_move_function (GtkWidget *win, Iconbox *ib, int *x, int *y)
{
    GdkRectangle r;

    gdk_screen_get_monitor_geometry (ib->gdk_screen, ib->monitor, &r);
    
    switch (ib->side)
    {
        case GTK_SIDE_LEFT:
            *x = r.x;
            break;
        case GTK_SIDE_RIGHT:
            *x = r.x + r.width - win->allocation.width;
            break;
        case GTK_SIDE_TOP:
            *y = r.y;
            break;
        default:
            *y = r.y + r.height - win->allocation.height;
    }

    if (!ib->in_move)
    {
        if (IS_HORIZONTAL (ib->side))
        {
            switch (ib->justification)
            {
                case GTK_JUSTIFY_LEFT:
                    *x = r.x + ib->offset;
                    break;
                case GTK_JUSTIFY_RIGHT:
                    *x = r.x + r.width - win->allocation.width - ib->offset;
                    break;
                case GTK_JUSTIFY_CENTER:
                    /* XXX should we use an offset here as well? */
                    *x = r.x + (r.width - win->allocation.width) / 2;
                    break;
                case GTK_JUSTIFY_FILL:
                default:
                    *x = r.x;
            }

            *x = CLAMP (*x, r.x, r.x + r.width - win->allocation.width);
        }
        else
        {
            switch (ib->justification)
            {
                case GTK_JUSTIFY_LEFT:
                    *y = r.y + ib->offset;
                    break;
                case GTK_JUSTIFY_RIGHT:
                    *y = r.y + r.height - win->allocation.height - ib->offset;
                    break;
                case GTK_JUSTIFY_CENTER:
                    /* XXX should we use an offset here as well? */
                    *y = r.y + (r.height - win->allocation.height) / 2;
                    break;
                case GTK_JUSTIFY_FILL:
                default:
                    *y = r.y;
            }
            
            *y = CLAMP (*y, r.y, r.y + r.height - win->allocation.height);
        }
    }
    else
    {
        if (IS_HORIZONTAL (ib->side))
        {
            *x = CLAMP (*x, r.x, r.x + r.width - win->allocation.width);

            if (ib->justification == GTK_JUSTIFY_RIGHT)
                ib->offset = r.x + r.width - win->allocation.width - *x;
            else
                ib->offset = *x - r.x;
        }
        else
        {
            *y = CLAMP (*y, r.y, r.y + r.height - win->allocation.height);

            if (ib->justification == GTK_JUSTIFY_RIGHT)
                ib->offset = r.y + r.height - win->allocation.height - *y;
            else
                ib->offset = *y - r.y;
        }
    }
    
    ib->x = *x;
    ib->y = *y;
}

static void
iconbox_resize_function (GtkWidget *win, Iconbox *ib, GtkAllocation *old, 
                         GtkAllocation *new, int *x, int *y)
{
    ib->width = new->width;
    ib->height = new->height;

    if (ib->block_resize)
        return;
    
    iconbox_move_function (win, ib, x, y);
}

static void
iconbox_move_start (GtkWidget *win, Iconbox *ib)
{
    ib->in_move = TRUE;
}

static void
iconbox_move_end (GtkWidget *win, int x, int y, Iconbox *ib)
{
    ib->in_move = FALSE;
}

static Iconbox *
create_iconbox (void)
{
    Iconbox *ib = g_new0 (Iconbox, 1);

    ib->win = xfce_panel_window_new ();
    
    xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (ib->win), 
            (XfcePanelWindowMoveFunc)iconbox_move_function, ib);
    
    xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (ib->win), 
            (XfcePanelWindowResizeFunc)iconbox_resize_function, ib);
    
    ib->gdk_screen = gtk_widget_get_screen (ib->win);

    ib->netk_screen = 
        netk_screen_get (gdk_screen_get_number (ib->gdk_screen));

    if (!icon_tooltips)
        icon_tooltips = gtk_tooltips_new ();
    
    ib->justification = DEFAULT_JUSTIFICATION;
    ib->side          = DEFAULT_SIDE;
    ib->icon_size     = DEFAULT_SIZE;
    ib->opacity       = DEFAULT_OPACITY;

    g_signal_connect (ib->win, "enter-notify-event", 
                      G_CALLBACK (iconbox_enter), ib);
    
    g_signal_connect (ib->win, "leave-notify-event", 
                      G_CALLBACK (iconbox_leave), ib);
    
    g_signal_connect (ib->win, "button-press-event", 
                      G_CALLBACK (iconbox_popup_menu), ib);

    g_signal_connect (ib->win, "move-start", G_CALLBACK (iconbox_move_start),
                      ib);

    g_signal_connect (ib->win, "move-end", G_CALLBACK (iconbox_move_end), ib);
    
    return ib;
}

/* settings */
static void
iconbox_read_settings (Iconbox *ib)
{
    char *file;

    ib->mcs_client = iconbox_connect_mcs_client (ib->gdk_screen, ib);
    
    switch (ib->justification)
    {
        case GTK_JUSTIFY_LEFT:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_END);
            break;
        case GTK_JUSTIFY_RIGHT:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_START);
            break;
        case GTK_JUSTIFY_CENTER:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_NONE);
            break;
        case GTK_JUSTIFY_FILL:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_NONE);
            break;
    }
    
    if (ib->side == GTK_SIDE_TOP || ib->side == GTK_SIDE_BOTTOM)
    {
        ib->box = gtk_hbox_new (TRUE, 0);
        xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (ib->win),
                                           GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_size_request (ib->box, -1, ib->icon_size + 8);
    }
    else
    {
        ib->box = gtk_vbox_new (TRUE, 0);
        xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (ib->win),
                                           GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_size_request (ib->box, ib->icon_size + 8, -1);
    }
    
    gtk_widget_show (ib->box);
    gtk_container_add (GTK_CONTAINER (ib->win), ib->box);
    
    /* read offset */
    file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG,
                                 "xfce4" G_DIR_SEPARATOR_S "iconboxrc");

    if (file)
    {
        FILE *fp;
        
        if ((fp = fopen (file, "r")) != NULL)
        {
            char line[255];
            int offset;

            while (fgets(line, sizeof (line), fp))
            {
                if (sscanf (line, "offset = %d", &offset) > 0)
                {
                    ib->offset = offset;

                    break;
                }
            }

            fclose (fp);
        }
        
        g_free (file);
    }
}

static void
iconbox_set_position (Iconbox *ib)
{
    GdkRectangle r;
    GtkRequisition req;
    gboolean top, bottom, left, right;

    gtk_widget_size_request (ib->win, &req);
    ib->width = req.width;
    ib->height = req.height;

    top = bottom = left = right = TRUE;
    
    gdk_screen_get_monitor_geometry (ib->gdk_screen, ib->monitor, &r);

    if (IS_HORIZONTAL (ib->side))
    {
        switch (ib->justification)
        {
            case GTK_JUSTIFY_RIGHT:
                ib->x = r.x + r.width - ib->width - ib->offset;
                if (!ib->offset)
                    right = FALSE;
                break;
            case GTK_JUSTIFY_LEFT:
                ib->x = r.x + ib->offset;
                if (!ib->offset)
                    left = FALSE;
                break;
            case GTK_JUSTIFY_CENTER:
                ib->x = r.x + (r.width - ib->width) / 2;
                break;
            case GTK_JUSTIFY_FILL:
                ib->x = r.x;
                left = FALSE;
                right = FALSE;
                break;
        }

        if (ib->side == GTK_SIDE_BOTTOM)
        {
            ib->y = r.y + r.height- ib->height;
            top = FALSE;
        }
        else
        {
            ib->y = r.y;
            bottom = FALSE;
        }
    }
    else
    {
        switch (ib->justification)
        {
            case GTK_JUSTIFY_RIGHT:
                ib->y = r.y + r.height - ib->height - ib->offset;
                if (!ib->offset)
                    bottom = FALSE;
                break;
            case GTK_JUSTIFY_LEFT:
                ib->y = r.y + ib->offset;
                if (!ib->offset)
                    top = FALSE;
                break;
            case GTK_JUSTIFY_CENTER:
                ib->y = r.y + (r.height - ib->height) / 2;
                break;
            case GTK_JUSTIFY_FILL:
                ib->y = r.y;
                top = FALSE;
                bottom = FALSE;
                break;
        }

        if (ib->side == GTK_SIDE_RIGHT)
        {
            ib->x = r.x + r.width - ib->width;
            right = FALSE;
        }
        else
        {
            ib->x = r.x;
            left = FALSE;
        }
    }

    xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (ib->win),
                                       top, bottom, left, right);
        
    gtk_window_move (GTK_WINDOW (ib->win), ib->x, ib->y);
}

/* session management */
static void
write_offset (Iconbox *ib)
{
    char *file;
    FILE *fp;

    /* save offset */
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                        "xfce4" G_DIR_SEPARATOR_S "iconboxrc",
                                        TRUE);

    if ((fp = fopen (file, "w")) != NULL)
    {
        fprintf (fp, "offset = %d\n", ib->offset);

        fclose (fp);
    }
    
    g_free (file);
}

static void
save (gpointer data, int save_style, gboolean shutdown, int interact_style, 
      gboolean fast)
{
    Iconbox *ib = data;

    write_offset (ib);
}

static void
quit (gpointer client_data)
{
    gtk_main_quit ();
}

static void
connect_to_session_manager (int argc, char **argv, Iconbox *ib)
{
    ib->session_client =
        client_session_new (argc, argv, ib, SESSION_RESTART_IF_RUNNING, 30);

    ib->session_client->save_yourself = save;
    ib->session_client->die = quit;

    session_init (ib->session_client);
}

/* signal handler */
static void
sighandler (int sig)
{
    switch (sig)
    {
        default:
            sig_quit = TRUE;
    }
}

static gboolean
check_signal_state (void)
{
    if (sig_quit)
    {
        sig_quit = FALSE;
        gtk_main_quit ();
    }

    /* keep running */
    return TRUE;
}

static void
connect_signal_handler (void)
{
#ifdef HAVE_SIGACTION
    struct sigaction act;
    
    act.sa_handler = sighandler;
    sigemptyset (&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#else
    act.sa_flags = 0;
#endif
    sigaction (SIGHUP, &act, NULL);
    sigaction (SIGUSR1, &act, NULL);
    sigaction (SIGUSR2, &act, NULL);
    sigaction (SIGINT, &act, NULL);
    sigaction (SIGTERM, &act, NULL);
#else
    signal (SIGHUP, sighandler);
    signal (SIGUSR1, sighandler);
    signal (SIGUSR2, sighandler);
    signal (SIGINT, sighandler);
    signal (SIGTERM, sighandler);
#endif
}

/* cleanup */
static void
cleanup_iconbox (Iconbox *ib)
{
    int i;
    GSList *l;

    write_offset (ib);
    
    iconbox_disconnect_mcs_client (ib->mcs_client);
    
    if (ib->menu)
        gtk_widget_destroy (ib->menu);
    
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
    
    g_free (ib);
}

/* main program */
int
main (int argc, char **argv)
{
    Iconbox *ib;
    
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    iconbox_check_options (argc, argv);

    gtk_init (&argc, &argv);

    ib = create_iconbox ();

    iconbox_read_settings (ib);
    iconbox_init_icons (ib);
    iconbox_set_position (ib);
    gtk_widget_show (ib->win);
    
    iconbox_set_transparent (ib, TRUE);
    
    connect_signal_handler ();
    g_timeout_add (500, (GSourceFunc)check_signal_state, NULL);
    
    connect_to_session_manager (argc, argv, ib);

    gtk_main ();

    cleanup_iconbox (ib);

    return 0;
}

/* public interface */
void
iconbox_set_justification (Iconbox * ib, GtkJustification justification)
{
    g_return_if_fail (ib != NULL);
    
    if (justification == ib->justification)
        return;

    ib->justification = justification;
    
    switch (ib->justification)
    {
        case GTK_JUSTIFY_LEFT:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_END);
            break;
        case GTK_JUSTIFY_RIGHT:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_START);
            break;
        case GTK_JUSTIFY_CENTER:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_NONE);
            break;
        case GTK_JUSTIFY_FILL:
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (ib->win),
                                                XFCE_HANDLE_STYLE_NONE);
            break;
    }

    ib->offset = 0;
    iconbox_set_position (ib);
}

void
iconbox_set_side (Iconbox * ib, GtkSideType side)
{
    GtkWidget *oldbox;
    
    g_return_if_fail (ib != NULL);

    if (side == ib->side)
        return;

    ib->side = side;
    oldbox = ib->box;
    
    if (!oldbox)
        return;
    
    ib->block_resize++;

    if (IS_HORIZONTAL (ib->side))
    {
        if (!GTK_IS_HBOX (oldbox))
        {
            ib->box = gtk_hbox_new (TRUE, 0);
            xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (ib->win),
                                               GTK_ORIENTATION_HORIZONTAL);
            gtk_widget_set_size_request (ib->box, -1, ib->icon_size + 8);
        }
        else
        {
            oldbox = NULL;
        }
    }
    else
    {
        if (!GTK_IS_VBOX (oldbox))
        {
            ib->box = gtk_vbox_new (TRUE, 0);
            xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (ib->win),
                                               GTK_ORIENTATION_VERTICAL);
            gtk_widget_set_size_request (ib->box, ib->icon_size + 8, -1);
        }
        else
        {
            oldbox = NULL;
        }
    }
    
    if (oldbox)
    {
        GSList *l;
        
        for (l = ib->iconlist; l != NULL; l = l->next)
        {
            Icon *icon = l->data;
            GtkWidget *w = icon->button;

            g_object_ref (w);
            gtk_container_remove (GTK_CONTAINER (oldbox), w);
            
            if (ib->justification != GTK_JUSTIFY_RIGHT)
                gtk_box_pack_start (GTK_BOX (ib->box), w, FALSE, FALSE, 0);
            else
                gtk_box_pack_start (GTK_BOX (ib->box), w, FALSE, FALSE, 0);

            g_object_unref (w);
        }
        
        gtk_widget_destroy (oldbox);
        gtk_widget_show (ib->box);
        gtk_container_add (GTK_CONTAINER (ib->win), ib->box);
    }
    
    iconbox_set_position (ib);

    while (gtk_events_pending ())
        gtk_main_iteration ();

    ib->block_resize--;
}

void
iconbox_set_icon_size (Iconbox * ib, int size)
{
    GSList *l;
    
    g_return_if_fail (ib != NULL);

    if (size == ib->icon_size)
        return;

    ib->block_resize++;

    ib->icon_size = size;

    if (ib->box)
    {
        if (IS_HORIZONTAL (ib->side))
            gtk_widget_set_size_request (ib->box, -1, ib->icon_size + 8);
        else
            gtk_widget_set_size_request (ib->box, ib->icon_size + 8, -1);
    }

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        icon_update_image (icon);
    }
    
    iconbox_set_position (ib);

    while (gtk_events_pending ())
        gtk_main_iteration ();

    ib->block_resize--;
}

void
iconbox_set_show_only_hidden (Iconbox * ib, gboolean only_hidden)
{
    GSList *l;
    
    g_return_if_fail (ib != NULL);

    if (only_hidden == ib->only_hidden)
        return;

    ib->only_hidden = only_hidden;

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;        

        update_visibility (icon, NULL);
    }
}

void
iconbox_set_transparency (Iconbox *ib, int transparency)
{
    g_return_if_fail (ib != NULL);

    ib->opacity = CLAMP (OPAQUE - rint ((double)transparency * OPAQUE / 100), 
                         0.2 * OPAQUE, OPAQUE);

    iconbox_set_transparent (ib, TRUE);
}

McsClient *
iconbox_get_mcs_client (Iconbox *ib)
{
    g_return_val_if_fail (ib != NULL, NULL);

    return ib->mcs_client;
}
