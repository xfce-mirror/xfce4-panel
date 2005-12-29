/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include "panel-app.h"
#include "panel-app-messages.h"
#include "panel-item-manager.h"
#include "panel-config.h"
#include "panel-properties.h"
#include "panel-dialogs.h"
#include "panel.h"

#ifndef _
#define _(x) x
#endif

#define SELECTION_NAME "XFCE4_PANEL"
#define PANEL_LAUNCHER "launcher"

/* types and global variables */

typedef enum
{
    PANEL_RUN_STATE_NORMAL,
    PANEL_RUN_STATE_RESTART,
    PANEL_RUN_STATE_QUIT,
    PANEL_RUN_STATE_QUIT_NOCONFIRM,
    PANEL_RUN_STATE_QUIT_NOSAVE
}
PanelRunState;

typedef struct _PanelApp PanelApp;

struct _PanelApp
{
    guint initialized:1;

    GtkWidget *gtk_ipc_window;
    Window ipc_window;

    SessionClient *session_client;
    PanelRunState runstate;
    GPtrArray *panel_list;
    GPtrArray *monitor_list;

    int check_id;
    int save_id;

    int current_panel;

    GList *dialogs;

    /* check whether monitors in Xinerama are aligned */
    guint xinerama_and_equal_width:1;
    guint xinerama_and_equal_height:1;
};

static PanelApp panel_app = {0};


/* cleanup */

static void
cleanup_panels (void)
{
    int i;
    GList *l;

    l = panel_app.dialogs;
    panel_app.dialogs = NULL;
    
    while (l)
    {
        gtk_dialog_response (GTK_DIALOG (l->data), GTK_RESPONSE_NONE);
        l = g_list_delete_link (l, l);
    }

    for (i = 0; i < panel_app.panel_list->len; ++i)
    {
        Panel *panel = g_ptr_array_index (panel_app.panel_list, i);

        gtk_widget_hide (GTK_WIDGET (panel));

        panel_free_data (panel);

        gtk_widget_destroy (GTK_WIDGET (panel));

        DBG ("Destroyed panel %d", i + 1);
    }

    g_ptr_array_free (panel_app.panel_list, TRUE);
}

/* signal handling */

/** copied from glibc manual, does this prevent zombies? */
static void
sigchld_handler (int sig)
{
    int pid, status, serrno;

    serrno = errno;

    while (1)
    {
        pid = waitpid (WAIT_ANY, &status, WNOHANG);

        if (pid < 0 || pid == 0)
            break;
    }

    errno = serrno;
}

static void
sighandler (int sig)
{
    /* Don't do any real stuff here.
     * Only set a signal state flag. There's a timeout in the main loop
     * that tests the flag.
     * This will prevent problems with gtk main loop threads and stuff
     */
    switch (sig)
    {
	case SIGUSR1:
            DBG ("USR1 signal caught");
	    panel_app.runstate = PANEL_RUN_STATE_RESTART;
	    break;

	case SIGUSR2:
            DBG ("USR2 signal caught");
	    panel_app.runstate = PANEL_RUN_STATE_QUIT;
	    break;

	case SIGINT:
        case SIGABRT:
            DBG ("INT or ABRT signal caught");
            panel_app.runstate = PANEL_RUN_STATE_QUIT_NOSAVE;
            break;

	default:
            DBG ("Signal caught: %d", sig);
	    panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
    }
}

static gboolean
check_signal_state (void)
{
    static int recursive = 0;
    gboolean quit = FALSE;
 
    /* micro-optimization */
    if (G_LIKELY (panel_app.runstate == PANEL_RUN_STATE_NORMAL))
        return TRUE;
    
    if (G_UNLIKELY (recursive))
        return TRUE;
    
    recursive++;

    switch (panel_app.runstate)
    {
        case PANEL_RUN_STATE_RESTART:
        case PANEL_RUN_STATE_QUIT_NOCONFIRM:
            panel_app_save ();
            quit = TRUE;
            break;
        case PANEL_RUN_STATE_QUIT_NOSAVE:
            quit = TRUE;
            break;
        default:
            if (panel_app.session_client)
            {
                logout_session (panel_app.session_client);
            }
            else if (xfce_confirm (_("Exit Xfce Panel?"),
                                   GTK_STOCK_QUIT, NULL))
            {
                panel_app_save ();
                quit = TRUE;
            }
            
            /* don't forget to set back the run state */
            panel_app.runstate = PANEL_RUN_STATE_NORMAL;
            break;
    }

    if (quit)
    {
        /* this is necessary, because the function is not only called from the
         * timeout, so just returning FALSE is not enough. */
        g_source_remove (panel_app.check_id);
        gtk_main_quit ();
        return FALSE;
    }

    recursive--;

    return TRUE;
}

/* session */

static void
session_save_yourself (gpointer data, int save_style, gboolean shutdown,
                       int interact_style, gboolean fast)
{
    panel_app_save ();
}

static void
session_die (gpointer client_data)
{
    panel_app_quit_noconfirm ();
}

/* screen layout */
static void
monitor_size_changed (GdkScreen *screen)
{
    int i;
    XfceMonitor *monitor;
    GtkWidget *panel;

    for (i = 0; i < panel_app.monitor_list->len; ++i)
    {
        monitor = g_ptr_array_index (panel_app.monitor_list, i);

        if (monitor->screen == screen)
        {
            gdk_screen_get_monitor_geometry (screen, monitor->num, 
                                             &(monitor->geometry));
        }
    }

    for (i = 0; i < panel_app.panel_list->len; ++i)
    {
        panel = g_ptr_array_index (panel_app.panel_list, i);

        gtk_widget_queue_resize (panel);
    }
}

static void
create_monitor_list (void)
{
    GdkDisplay *display;
    GdkScreen *screen;
    XfceMonitor *monitor;
    int n_screens, n_monitors, i, j, w, h;
    gboolean equal_w, equal_h;
    
    panel_app.monitor_list = g_ptr_array_new ();

    display = gdk_display_get_default ();

    n_screens = gdk_display_get_n_screens (display);

    equal_w = equal_h = TRUE;
    
    for (i = 0; i < n_screens; ++i)
    {
        screen = gdk_display_get_screen (display, i);

        n_monitors = gdk_screen_get_n_monitors (screen);

        for (j = 0; j < n_monitors; ++j)
        {
            monitor = g_new (XfceMonitor, 1);

            monitor->screen = screen;
            monitor->num = j;

            gdk_screen_get_monitor_geometry (screen, j, &(monitor->geometry));

            g_ptr_array_add (panel_app.monitor_list, monitor);
            
            if (j > 0)
            {
                if (w != monitor->geometry.width)
                    equal_w = FALSE;
                if (h != monitor->geometry.height)
                    equal_h = FALSE;
            }

            w = monitor->geometry.width;
            h = monitor->geometry.height;
            
#if TEST_MULTIPLE_MONITORS
            monitor = g_new (XfceMonitor, 1);

            monitor->screen = screen;
            monitor->num = j;

            gdk_screen_get_monitor_geometry (screen, j, &(monitor->geometry));

            g_ptr_array_add (panel_app.monitor_list, monitor);
#endif
        }

        g_signal_connect (screen, "size-changed", 
                          G_CALLBACK (monitor_size_changed), NULL);
    }

    if (n_screens == 1 && n_monitors > 1)
    {
        panel_app.xinerama_and_equal_width = equal_w;
        panel_app.xinerama_and_equal_height = equal_h;
    }
}

/* open dialogs */
static void
unregister_dialog (GtkWidget *dialog)
{
    GList *l;

    for (l = panel_app.dialogs; l != NULL; l = l->next)
    {
        if (dialog == GTK_WIDGET (l->data))
        {
            panel_app.dialogs = g_list_delete_link (panel_app.dialogs, l);
            break;
        }
    }
}

/* public API */

/**
 * panel_app_init
 *
 * Initialize application. Creates ipc window if no other instance is
 * running or sets the ipc window from the running instance.
 *
 * Returns: 0 on success, 1 when an xfce4-panel instance already exists, 
 *          and -1 on failure.
 **/
int
panel_app_init (void)
{
    Atom selection_atom, manager_atom;
    GtkWidget *invisible;
    XClientMessageEvent xev;

    if (panel_app.initialized)
        return 0;
    
    panel_app.initialized = TRUE;
    
    selection_atom = XInternAtom (GDK_DISPLAY (), SELECTION_NAME, False);

    panel_app.ipc_window = XGetSelectionOwner (GDK_DISPLAY (), selection_atom);

    if (panel_app.ipc_window)
        return 1;

    invisible = gtk_invisible_new ();
    gtk_widget_realize (invisible);
    
    panel_app.gtk_ipc_window = invisible;
    panel_app.ipc_window = GDK_WINDOW_XWINDOW (invisible->window);
    
    XSelectInput (GDK_DISPLAY (), panel_app.ipc_window, PropertyChangeMask);
    
    XSetSelectionOwner (GDK_DISPLAY (), selection_atom, panel_app.ipc_window, 
                        GDK_CURRENT_TIME);
    
    if (XGetSelectionOwner (GDK_DISPLAY (), selection_atom) != 
            panel_app.ipc_window)
    {
        g_critical ("Could not set ownership of selection \"%s\"",
                    SELECTION_NAME);
        return -1;
    }
    
    manager_atom = XInternAtom (GDK_DISPLAY (), "MANAGER", False);
    
    xev.type = ClientMessage;
    xev.window = GDK_ROOT_WINDOW ();
    xev.message_type = manager_atom;
    xev.format = 32;
    xev.data.l[0] = GDK_CURRENT_TIME;
    xev.data.l[1] = selection_atom;
    xev.data.l[2] = panel_app.ipc_window;
    xev.data.l[3] = 0;	/* manager specific data */
    xev.data.l[4] = 0;	/* manager specific data */

    XSendEvent (GDK_DISPLAY (), GDK_ROOT_WINDOW (), False,
                StructureNotifyMask, (XEvent *) & xev);

    /* listen for client messages */
    panel_app_listen (invisible);

    return 0;
}

/* fix position after showing panel for the first time */
static gboolean
expose_timeout (GtkWidget *panel)
{
    gtk_widget_queue_resize (panel);
    return FALSE;
}

static void
panel_app_init_panel (GtkWidget *panel)
{
    gtk_widget_show (panel);
    g_idle_add ((GSourceFunc)expose_timeout, panel);
}    

/**
 * panel_app_run
 * 
 * Run the panel application. Reads the configuration file(s) and sets up the
 * panels, before turning over control to the main event loop.
 *
 * Returns: 1 to restart and 0 to quit.
 **/
int
panel_app_run (int argc, char **argv)
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
    sigaction (SIGABRT, &act, NULL);
    sigaction (SIGTERM, &act, NULL);
    act.sa_handler = sigchld_handler;
    sigaction (SIGCHLD, &act, NULL);
#else
    signal (SIGHUP, sighandler);
    signal (SIGUSR1, sighandler);
    signal (SIGUSR2, sighandler);
    signal (SIGINT, sighandler);
    signal (SIGABRT, sighandler);
    signal (SIGTERM, sighandler);
    signal (SIGCHLD, sigchld_handler);
#endif

    /* environment */
    xfce_setenv ("DISPLAY", gdk_display_get_name (gdk_display_get_default ()),
		 TRUE);
    
    /* session management */
    panel_app.session_client = 
        client_session_new (argc, argv, NULL /* data */, 
                            SESSION_RESTART_IF_RUNNING, 40);

    panel_app.session_client->save_yourself = session_save_yourself;
    panel_app.session_client->die = session_die;

    if (!session_init (panel_app.session_client))
    {
        g_free (panel_app.session_client);
        panel_app.session_client = NULL;
    }   
    
    /* screen layout and geometry */
    create_monitor_list ();
    
    /* configuration */
    xfce_panel_item_manager_init ();

    panel_app.panel_list = panel_config_create_panels ();

    g_ptr_array_foreach (panel_app.panel_list, (GFunc)panel_app_init_panel, 
                         NULL);

    /* Run Forrest, Run! */
    panel_app.check_id = 
        g_timeout_add (250, (GSourceFunc) check_signal_state, NULL);
    panel_app.runstate = PANEL_RUN_STATE_NORMAL;
    gtk_main ();
    
    g_free (panel_app.session_client);
    panel_app.session_client = NULL;

    cleanup_panels ();
    xfce_panel_item_manager_cleanup ();

    if (panel_app.runstate == PANEL_RUN_STATE_RESTART)
        return 1;

    return 0;
}

static gboolean
save_timeout (void)
{
    DBG (" ++ save timeout");
    
    if (panel_app.save_id)
        g_source_remove (panel_app.save_id);
    panel_app.save_id = 0;

    if (panel_app.runstate == PANEL_RUN_STATE_NORMAL)
    {
        DBG (" ++ really save");
        panel_app_save ();
    }

    return FALSE;
}

void 
panel_app_queue_save (void)
{
    if (panel_app.runstate == PANEL_RUN_STATE_NORMAL)
    {
        if (!panel_app.save_id)
            panel_app.save_id = 
                g_timeout_add (30000, (GSourceFunc)save_timeout, NULL);
    }
}

void 
panel_app_customize (void)
{
    panel_manager_dialog (panel_app.panel_list);
}

void 
panel_app_customize_items (GtkWidget *active_item)
{
    add_items_dialog (panel_app.panel_list, active_item);
}

void 
panel_app_save (void)
{
    panel_config_save_panels (panel_app.panel_list);
}

void 
panel_app_restart (void)
{
    panel_app.runstate = PANEL_RUN_STATE_RESTART;
    check_signal_state ();
}

void 
panel_app_quit (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT;
    check_signal_state ();
}

void 
panel_app_quit_noconfirm (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
    check_signal_state ();
}

void 
panel_app_quit_nosave (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT_NOSAVE;
    check_signal_state ();
}

void 
panel_app_add_panel (void)
{
    Panel *panel;

    panel = panel_new ();

    if (G_UNLIKELY (panel_app.panel_list == NULL))
        panel_app.panel_list = g_ptr_array_sized_new (1);
    
    g_ptr_array_add (panel_app.panel_list, panel);

    panel_set_screen_position (panel, XFCE_SCREEN_POSITION_FLOATING_H);
    panel_add_item (panel, PANEL_LAUNCHER);

    panel_center (panel);
    panel_init_position (panel);

    gtk_widget_show (GTK_WIDGET (panel));

    panel_app.current_panel = panel_app.panel_list->len - 1;
    panel_app_customize ();
}

void 
panel_app_remove_panel (GtkWidget *panel)
{
    int response = GTK_RESPONSE_NONE, n;
    char *first;

    if (panel_app.panel_list->len == 1)
    {
        response = 
            xfce_message_dialog (GTK_WINDOW (panel), _("Xfce Panel"),
                                 GTK_STOCK_DIALOG_WARNING, 
                                 _("Exit Xfce Panel?"), 
                                 _("You can't remove the last panel. "
                                   "Would you like to exit the program?"),
                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_QUIT, GTK_RESPONSE_ACCEPT,
                                 NULL);

        if (response == GTK_RESPONSE_ACCEPT)
            panel_app_quit_noconfirm ();

        return;
    }
    
    for (n = 0; n < panel_app.panel_list->len; ++n)
    {
        if (panel == g_ptr_array_index (panel_app.panel_list, n))
            break;
    }

    if (n == panel_app.panel_list->len)
        return;
    
    panel_block_autohide (PANEL (panel));
    
    panel_set_items_sensitive (PANEL (panel), FALSE);
    gtk_widget_set_sensitive (panel, FALSE);
    gtk_drag_highlight (panel);
    
    first = g_strdup_printf (_("Remove Panel \"%d\"?"), n + 1);
    
    response = xfce_message_dialog (GTK_WINDOW (panel), _("Xfce Panel"),
                                    GTK_STOCK_DIALOG_WARNING, first, 
                                    _("The selected panel and all its items "
                                      "will be removed."),
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_REMOVE, GTK_RESPONSE_ACCEPT,
                                    NULL);
    g_free (first);
                                    
    if (response != GTK_RESPONSE_ACCEPT)
    {
        gtk_drag_unhighlight (panel);
        gtk_widget_set_sensitive (panel, TRUE);
        panel_set_items_sensitive (PANEL (panel), TRUE);
        panel_unblock_autohide (PANEL (panel));
        return;
    }

    if (g_ptr_array_remove (panel_app.panel_list, (gpointer)panel))
    {
        gtk_widget_hide (panel);
        panel_free_data (PANEL (panel));
        gtk_widget_destroy (panel);
    }
}

void 
panel_app_about (void)
{
    XfceAboutInfo *info;
    GtkWidget *dlg;
    GdkPixbuf *pb;

    info = xfce_about_info_new (_("Xfce Panel"), "", _("Xfce Panel"), 
                                XFCE_COPYRIGHT_TEXT ("2005", 
                                                     "Jasper Huijsmans"),
                                XFCE_LICENSE_GPL);

    xfce_about_info_set_homepage (info, "http://www.xfce.org");

    xfce_about_info_add_credit (info, "Jasper Huijsmans", "jasper@xfce.org",
                                _("Developer"));

    pb = xfce_themed_icon_load ("xfce4-panel", 48);
    dlg = xfce_about_dialog_new (NULL, info, pb);
    g_object_unref (pb);

    gtk_widget_set_size_request (dlg, 400, 300);

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    xfce_about_info_free (info);
}

Window
panel_app_get_ipc_window (void)
{
    panel_app_init ();

    return panel_app.ipc_window;
}

XfceMonitor *
panel_app_get_monitor (int n)
{
    return g_ptr_array_index (panel_app.monitor_list, 
                              CLAMP (n, 0, panel_app.monitor_list->len - 1));
}

int
panel_app_get_n_monitors (void)
{
    return panel_app.monitor_list->len;
}

/* open dialogs */
void 
panel_app_register_dialog (GtkWidget *dialog)
{
    g_return_if_fail (GTK_IS_WIDGET (dialog));

    g_signal_connect (dialog, "destroy", G_CALLBACK (unregister_dialog), NULL);

    panel_app.dialogs = g_list_prepend (panel_app.dialogs, dialog);
}

/* current panel */
void 
panel_app_set_current_panel (gpointer *panel)
{
    int i;

    panel_app.current_panel = 0;
    
    for (i = 0; i < panel_app.panel_list->len; ++i)
    {
        if (g_ptr_array_index (panel_app.panel_list, i) == panel)
        {
            panel_app.current_panel = i;
            break;
        }
    }

    DBG ("Current panel: %d", panel_app.current_panel);
}

void 
panel_app_unset_current_panel (gpointer *panel)
{
    int i;

    for (i = 0; i < panel_app.panel_list->len; ++i)
    {
        if (g_ptr_array_index (panel_app.panel_list, i) == panel)
        {
            if (i == panel_app.current_panel)
                panel_app.current_panel = 0;
            break;
        }
    }

    DBG ("Current panel: %d", panel_app.current_panel);
}

int 
panel_app_get_current_panel (void)
{
    return panel_app.current_panel;
}

G_CONST_RETURN GPtrArray *
panel_app_get_panel_list (void)
{
    return panel_app.panel_list;
}

/* check whether monitors in Xinerama are aligned */
gboolean
panel_app_monitors_equal_height (void)
{
    return panel_app.xinerama_and_equal_height;
}

gboolean
panel_app_monitors_equal_width (void)
{
    return panel_app.xinerama_and_equal_width;
}

