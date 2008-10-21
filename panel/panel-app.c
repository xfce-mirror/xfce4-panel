/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005-2006 Jasper Huijsmans <jasper@xfce.org>
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

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-convenience.h>

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

#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif

#define SELECTION_NAME "XFCE4_PANEL"
#define PANEL_LAUNCHER "launcher"
#define SAVE_TIMEOUT   30000

#if defined(TIMER) && defined(G_HAVE_ISO_VARARGS)
void
xfce_panel_program_log (const char *file, 
                        const int   line, 
                        const char *format, 
                        ...)
{
    va_list args;
    gchar  *formatted;
    gchar  *message;

    va_start (args, format);
    formatted = g_strdup_vprintf (format, args);
    va_end (args);

    message = g_strdup_printf ("MARK: %s: %s:%d: %s", 
                               g_get_prgname(), file, line, formatted);

    access (message, F_OK);

    g_free (formatted);
    g_free (message);
}
#endif

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
    GtkWidget     *gtk_ipc_window;
    Window         ipc_window;

    SessionClient *session_client;
    PanelRunState  runstate;
    GPtrArray     *panel_list;
    GPtrArray     *monitor_list;

    int            save_id;
    int            current_panel;
    GList         *dialogs;

    /* Initialization. Also unset before cleanup. */
    guint          initialized:1; 

    /* Check whether monitors in Xinerama are aligned. */
    guint          xinerama_and_equal_width:1;
    guint          xinerama_and_equal_height:1;
};

static PanelApp panel_app = {0};
static int signal_pipe[2];


/* cleanup */

static void
cleanup_panels (void)
{
    int    i;
    GList *l;

    if (!panel_app.initialized)
        return;

    panel_app.initialized = FALSE;

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

    for (i = 0; i < panel_app.monitor_list->len; ++i)
    {
        XfceMonitor *xmon = g_ptr_array_index (panel_app.monitor_list, i);
        panel_slice_free (XfceMonitor, xmon);
    }
    g_ptr_array_free (panel_app.monitor_list, TRUE);
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
    if (write (signal_pipe[1], &sig, sizeof (int)) != sizeof (int))
    {
        g_printerr ("unix signal %d lost\n", sig);
    }
}

static gboolean
check_run_state (void)
{
    static int recursive = 0;
    gboolean   quit      = FALSE;
 
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
        if (panel_app.save_id)
            g_source_remove (panel_app.save_id);
        panel_app.save_id = 0;

        /* this is necessary, because the function is not only called from the
         * timeout, so just returning FALSE is not enough. */
        gtk_main_quit ();
        return FALSE;
    }

    recursive--;

    return TRUE;
}

static gboolean
set_run_state (GIOChannel   *source, 
               GIOCondition  cond, 
               gpointer      data)
{
    GError    *error = NULL;
    GIOStatus  status;
    gsize      bytes_read;
    /* 
     * There is no g_io_channel_read or g_io_channel_read_int, so we read
     * char's and use a union to recover the unix signal number.
     */
    union
    {
        gchar chars[sizeof (int)];
        int   signal;
    } buf;

    while ((status = g_io_channel_read_chars (source, buf.chars, sizeof (int),
                                              &bytes_read, &error)
           ) == G_IO_STATUS_NORMAL)
    {
        if (bytes_read != sizeof (int))
        {
            g_printerr ("lost data in signal pipe: expected %d, receieved %d",
                        sizeof (int), bytes_read);
            /* always at least quit if we received data */
            panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
            continue;
        }

        switch (buf.signal)
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
                DBG ("Signal caught: %d", buf.signal);
                panel_app.runstate = PANEL_RUN_STATE_QUIT_NOSAVE;
        }
    }

    if (error != NULL)
    {
        g_printerr ("reading signal pipe failed: %s\n", error->message);
        g_error_free (error);
        panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
    }
    if (status != G_IO_STATUS_AGAIN)
    {
        g_printerr ("signal pipe has been closed\n");
        panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
    }

    return check_run_state ();
}

/* session */

static void
session_save_yourself (gpointer data, 
                       int      save_style, 
                       gboolean shutdown,
                       int      interact_style, 
                       gboolean fast)
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
    int          i;
    XfceMonitor *monitor;
    GtkWidget   *panel;

    for (i = 0; i < panel_app.monitor_list->len; ++i)
    {
        monitor = g_ptr_array_index (panel_app.monitor_list, i);

        /* 
         * With xrandr 1.2, monitors can be added/removed, so need 
         * to double check the number of monitors (bug #3620)...
         */
        if ((monitor->screen == screen) && (monitor->num < gdk_screen_get_n_monitors (screen)))
        {
            gdk_screen_get_monitor_geometry (screen, monitor->num, 
                                             &(monitor->geometry));
        }
    }

    for (i = 0; i < panel_app.panel_list->len; ++i)
    {
        panel = g_ptr_array_index (panel_app.panel_list, i);

        if (screen == gtk_widget_get_screen (panel))
            panel_screen_size_changed (screen, PANEL (panel));
    }
}

static void
create_monitor_list (void)
{
    GdkDisplay  *display;
    GdkScreen   *screen;
    XfceMonitor *monitor;
    int          n_screens;
    int          n_monitors = 0;
    int          i, j;
    int          w = 0, h = 0;
    gboolean     equal_w, equal_h;
    
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
            monitor = panel_slice_new0 (XfceMonitor);

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
            monitor = panel_slice_new0 (XfceMonitor);

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

    /* check layout */
    for (i = 0; i < panel_app.monitor_list->len; ++i)
    {
        XfceMonitor *mon1, *mon2;

        mon1 = g_ptr_array_index (panel_app.monitor_list, i);

        for (j = 0; j < panel_app.monitor_list->len; ++j)
        {
            if (j == i)
                continue;

            mon2 = g_ptr_array_index (panel_app.monitor_list, j);

            if (mon2->geometry.x < mon1->geometry.x
                && mon2->geometry.y < mon1->geometry.y + mon1->geometry.height
                && mon2->geometry.y + mon2->geometry.height > mon1->geometry.y
               )
            {
                mon1->has_neighbor_left = TRUE;
            }

            if (mon2->geometry.x > mon1->geometry.x
                && mon2->geometry.y < mon1->geometry.y + mon1->geometry.height
                && mon2->geometry.y + mon2->geometry.height > mon1->geometry.y
               )
            {
                mon1->has_neighbor_right = TRUE;
            }

            if (mon2->geometry.y < mon1->geometry.y
                && mon2->geometry.x < mon1->geometry.x + mon1->geometry.width
                && mon2->geometry.x + mon2->geometry.width > mon1->geometry.x
               )
            {
                mon1->has_neighbor_above = TRUE;
            }

            if (mon2->geometry.y > mon1->geometry.y
                && mon2->geometry.x < mon1->geometry.x + mon1->geometry.width
                && mon2->geometry.x + mon2->geometry.width > mon1->geometry.x
               )
            {
                mon1->has_neighbor_below = TRUE;
            }
        }
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
    Atom                 selection_atom;
    Atom                 manager_atom;
    GtkWidget           *invisible;
    XClientMessageEvent  xev;

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
    
    xev.type         = ClientMessage;
    xev.window       = GDK_ROOT_WINDOW ();
    xev.message_type = manager_atom;
    xev.format       = 32;
    xev.data.l[0]    = GDK_CURRENT_TIME;
    xev.data.l[1]    = selection_atom;
    xev.data.l[2]    = panel_app.ipc_window;
    xev.data.l[3]    = 0;	/* manager specific data */
    xev.data.l[4]    = 0;	/* manager specific data */

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
    TIMER_ELAPSED("start panel_app_init_panel: %p", panel);
    panel_init_position (PANEL (panel));
    panel_init_signals (PANEL (panel));
    TIMER_ELAPSED(" + start show panel");
    gtk_widget_show (panel);
    TIMER_ELAPSED(" + end show panel");
    g_idle_add ((GSourceFunc)expose_timeout, panel);
    TIMER_ELAPSED("end panel_app_init_panel");
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
panel_app_run (int    argc, 
               char **argv)
{
#ifdef HAVE_SIGACTION
    struct sigaction  act;
#endif    
    GIOChannel       *g_signal_in;
    GError           *error = NULL;
    long              fd_flags;

    /* create pipe and set writing end in non-blocking mode */
    if (pipe (signal_pipe))
    {
        perror ("pipe");
        return -1;
    }
    fd_flags = fcntl (signal_pipe[1], F_GETFL);
    if (fd_flags == -1)
    {
        perror ("read descriptor flags");
        return -1;
    }
    if (fcntl (signal_pipe[1], F_SETFL, fd_flags | O_NONBLOCK) == -1)
    {
        perror ("write descriptor flags");
        return -1;
    }

    /* convert the reading end of the pipe into a GIOChannel */
    g_signal_in = g_io_channel_unix_new (signal_pipe[0]);
    g_io_channel_set_encoding (g_signal_in, NULL, &error);
    if (error != NULL)
    {                           /* handle potential errors */
        g_printerr ("g_io_channel_set_encoding failed %s\n",
                    error->message);
        g_error_free (error);
        return -1;
    }

    /* put the reading end also into non-blocking mode */
    g_io_channel_set_flags (g_signal_in,
                            g_io_channel_get_flags (g_signal_in) |
                            G_IO_FLAG_NONBLOCK, &error);
    if (error != NULL)
    {
        g_printerr ("g_io_set_flags failed %s\n", error->message);
        g_error_free (error);
        return -1;
    }

    /* register the reading end with the event loop */
    g_io_add_watch (g_signal_in, G_IO_IN | G_IO_PRI, set_run_state, NULL);

    /* register signals */
#ifdef HAVE_SIGACTION
    act.sa_handler = sighandler;
    sigemptyset (&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#else
    act.sa_flags = 0;
#endif
    sigaction (SIGHUP,  &act, NULL);
    sigaction (SIGUSR1, &act, NULL);
    sigaction (SIGUSR2, &act, NULL);
    sigaction (SIGINT,  &act, NULL);
    sigaction (SIGABRT, &act, NULL);
    sigaction (SIGTERM, &act, NULL);
    act.sa_handler = sigchld_handler;
    sigaction (SIGCHLD, &act, NULL);
#else
    signal (SIGHUP,  sighandler);
    signal (SIGUSR1, sighandler);
    signal (SIGUSR2, sighandler);
    signal (SIGINT,  sighandler);
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
    panel_app.session_client->die           = session_die;

    TIMER_ELAPSED("connect to session manager");
    if (!session_init (panel_app.session_client))
    {
        g_free (panel_app.session_client);
        panel_app.session_client = NULL;
    }   
    
    /* screen layout and geometry */
    TIMER_ELAPSED("start monitor list creation");
    create_monitor_list ();
    
    /* configuration */
    TIMER_ELAPSED("start init item manager");
    xfce_panel_item_manager_init ();

    TIMER_ELAPSED("start panel creation");
    panel_app.panel_list = panel_config_create_panels ();
    TIMER_ELAPSED("end panel creation");

    g_ptr_array_foreach (panel_app.panel_list, (GFunc)panel_app_init_panel, 
                         NULL);

    /* Run Forrest, Run! */
    panel_app.runstate = PANEL_RUN_STATE_NORMAL;
    TIMER_ELAPSED("start main loop");
    gtk_main ();
    
    /* cleanup */
    g_free (panel_app.session_client);

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
    if (!panel_app.initialized)
        return;

    if (panel_app.runstate == PANEL_RUN_STATE_NORMAL)
    {
        if (!panel_app.save_id)
            panel_app.save_id = 
                g_timeout_add (SAVE_TIMEOUT, (GSourceFunc)save_timeout, NULL);
    }
}

void 
panel_app_customize (void)
{
    if (xfce_allow_panel_customization())
        panel_manager_dialog (panel_app.panel_list);
}

void 
panel_app_customize_items (GtkWidget *active_item)
{
    if (xfce_allow_panel_customization())
        add_items_dialog (panel_app.panel_list, active_item);
}

void 
panel_app_save (void)
{
    if (!panel_app.initialized)
        return;

    if (xfce_allow_panel_customization())
        panel_config_save_panels (panel_app.panel_list);
}

void 
panel_app_restart (void)
{
    panel_app.runstate = PANEL_RUN_STATE_RESTART;
    check_run_state ();
}

void 
panel_app_quit (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT;
    check_run_state ();
}

void 
panel_app_quit_noconfirm (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT_NOCONFIRM;
    check_run_state ();
}

void 
panel_app_quit_nosave (void)
{
    panel_app.runstate = PANEL_RUN_STATE_QUIT_NOSAVE;
    check_run_state ();
}

void 
panel_app_add_panel (void)
{
    Panel *panel;

    if (!xfce_allow_panel_customization())
        return;

    panel = panel_new ();

    if (G_UNLIKELY (panel_app.panel_list == NULL))
        panel_app.panel_list = g_ptr_array_sized_new (1);
    
    g_ptr_array_add (panel_app.panel_list, panel);

    panel_set_screen_position (panel, XFCE_SCREEN_POSITION_FLOATING_H);
    panel_add_item (panel, PANEL_LAUNCHER);

    panel_center (panel);
    panel_init_position (panel);
    panel_init_signals (panel);
    gtk_widget_show (GTK_WIDGET (panel));

    panel_app.current_panel = panel_app.panel_list->len - 1;
    panel_app_customize ();
}

void 
panel_app_remove_panel (GtkWidget *panel)
{
    int   response = GTK_RESPONSE_NONE;
    int   n;
    char *first;

    if (!xfce_allow_panel_customization())
        return;
        
    if (panel_app.panel_list->len == 1)
    {
        response = 
            xfce_message_dialog (NULL, _("Xfce Panel"),
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
panel_app_about (GtkWidget *panel)
{
    XfceAboutInfo *info;
    GtkWidget     *dlg;
    GdkPixbuf     *pb;

    info = 
        xfce_about_info_new (_("Xfce Panel"), "", _("Xfce Panel"), 
                             XFCE_COPYRIGHT_TEXT ("2006", "Jasper Huijsmans"),
                             XFCE_LICENSE_GPL);

    xfce_about_info_set_homepage (info, "http://www.xfce.org");

    xfce_about_info_add_credit (info, "Jasper Huijsmans", "jasper@xfce.org",
                                _("Developer"));

    xfce_about_info_add_credit (info, "Nick Schermer", "nick@xfce.org",
                                _("Developer"));

    pb = xfce_themed_icon_load ("xfce4-panel", 48);
    dlg = xfce_about_dialog_new_with_values (NULL, info, pb);
    g_object_unref (G_OBJECT (pb));

    gtk_window_set_screen (GTK_WINDOW (dlg), gtk_widget_get_screen (panel));
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

