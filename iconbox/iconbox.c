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
#define DEFAULT_SIZE           48

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
        ABS (w - icon->ib->icon_size) > 10 || 
        ABS (h - icon->ib->icon_size) > 10)
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
    
    if (!(type == NETK_WINDOW_NORMAL || type == NETK_WINDOW_UTILITY) ||
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

    netk_screen_force_update (ib->netk_screen);
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
        
        /* fall through: unrecognized option, version or help */
        g_print ("\n"
                 " Xfce Iconbox " VERSION "\n"
                 " Licensed under the GNU GPL\n"
                 "\n"
                 " Usage: xfce4-iconbox [--version|-V] [--help|-h]\n"
                 "\n");

        exit (0);
    }
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
            *y = CLAMP (*y, r.y, r.y + r.height - win->allocation.height);
            break;
        case GTK_SIDE_RIGHT:
            *x = r.x + r.width - win->allocation.width;
            *y = CLAMP (*y, r.y, r.y + r.height - win->allocation.height);
            break;
        case GTK_SIDE_TOP:
            *x = CLAMP (*x, r.x, r.x + r.width - win->allocation.width);
            *y = r.y;
            break;
        default:
            *x = CLAMP (*x, r.x, r.x + r.width - win->allocation.width);
            *y = r.y + r.height - win->allocation.height;
    }
}

static void
iconbox_resize_function (GtkWidget *win, Iconbox *ib, GtkAllocation *old, 
                         GtkAllocation *new, int *x, int *y)
{
    if(ib->justification == GTK_JUSTIFY_RIGHT)
    {
        if (ib->side == GTK_SIDE_LEFT || ib->side == GTK_SIDE_RIGHT)
            *y = *y - new->height + old->height;
        else
            *x = *x - new->width + old->width;
    }

    ib->width = new->width;
    ib->height = new->height;

    iconbox_move_function (win, ib, x, y);
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
    ib->side      = DEFAULT_SIDE;
    ib->icon_size = DEFAULT_SIZE;
    
    return ib;
}

/* settings */
static void
iconbox_read_settings (Iconbox *ib)
{
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
                                                XFCE_HANDLE_STYLE_BOTH);
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
}

static void
iconbox_set_position (Iconbox *ib)
{
    GdkRectangle r;
    GtkRequisition req;

    gtk_widget_size_request (ib->win, &req);
    ib->width = req.width;
    ib->height = req.height;

    gdk_screen_get_monitor_geometry (ib->gdk_screen, ib->monitor, &r);

    if (ib->side == GTK_SIDE_TOP || ib->side == GTK_SIDE_BOTTOM)
    {
        switch (ib->justification)
        {
            case GTK_JUSTIFY_RIGHT:
                ib->x = r.x + r.width - ib->width - ib->offset;
                break;
            case GTK_JUSTIFY_LEFT:
                ib->x = r.x + ib->offset;
                break;
            case GTK_JUSTIFY_CENTER:
                ib->x = r.x + (r.width - ib->width) / 2;
                break;
            case GTK_JUSTIFY_FILL:
                ib->x = r.x;
                break;
        }

        if (ib->side == GTK_SIDE_BOTTOM)
            ib->y = r.y + r.height- ib->height;
        else
            ib->y = r.y;
    }
    else
    {
        switch (ib->justification)
        {
            case GTK_JUSTIFY_RIGHT:
                ib->y = r.y + r.height - ib->height - ib->offset;
                break;
            case GTK_JUSTIFY_LEFT:
                ib->y = r.y + ib->offset;
                break;
            case GTK_JUSTIFY_CENTER:
                ib->y = r.y + (r.height - ib->height) / 2;
                break;
            case GTK_JUSTIFY_FILL:
                ib->y = r.y;
                break;
        }

        if (ib->side == GTK_SIDE_RIGHT)
            ib->x = r.x + r.width - ib->width;
        else
            ib->x = r.x;
    }

    gtk_window_move (GTK_WINDOW (ib->win), ib->x, ib->y);
}

/* session management */
static void
save (gpointer data, int save_style, gboolean shutdown, int interact_style, 
      gboolean fast)
{
    Iconbox *ib = (Iconbox *)data;
    
    /* save offset */
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

    iconbox_disconnect_mcs_client (ib->mcs_client);
    
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
    
    g_free (ib);
}

/* main program */
int
main (int argc, char **argv)
{
    Iconbox *ib;
    
    iconbox_check_options (argc, argv);

    gtk_init (&argc, &argv);

    ib = create_iconbox ();

    iconbox_read_settings (ib);
    iconbox_init_icons (ib);
    iconbox_set_position (ib);
    gtk_widget_show (ib->win);
    
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
                                                XFCE_HANDLE_STYLE_BOTH);
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
}

void
iconbox_set_icon_size (Iconbox * ib, int size)
{
    GSList *l;
    
    g_return_if_fail (ib != NULL);

    if (size == ib->icon_size)
        return;

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

McsClient *
iconbox_get_mcs_client (Iconbox *ib)
{
    return ib->mcs_client;
}
