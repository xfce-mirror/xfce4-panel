/*  $Id$
 *
 *  Copyright (c) 2002-2004 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@xfce.org>
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

/*  main.c 
 *  ------
 *  Contains 'main' function, quit and restart functions
 *  and session management.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/session-client.h>
#include <libxfcegui4/netk-util.h>

#include "xfce.h"
#include "settings.h"
#include "mcs_client.h"
#include "controls_dialog.h"
#include "item_dialog.h"
#include "popup.h"

#define XFCE_PANEL_SELECTION_FMT "XFCE_PANEL_SELECTION_%u"

#define RC_STRING \
    "style \"popupbutton\" { GtkWidget::focus-padding = 0 }\n" \
    "class \"XfceTogglebutton\" style \"popupbutton\"\n"

/* signal handling */
typedef enum
{
    NOSIGNAL,
    RESTART,
    QUIT,
    QUIT_CONFIRM,
    NUM_SIGNALS,
}
SignalState;

static SignalState sigstate = NOSIGNAL;

static char *progname = NULL;

/*  session management
 *  ------------------
*/
static SessionClient *client_session;
static gboolean session_managed = FALSE;

static void
save_panel (void)
{
    if (!disable_user_config)
	write_panel_config ();
}

static void
save_yourself (gpointer data, int save_style, gboolean shutdown,
	       int interact_style, gboolean fast)
{
    save_panel ();
}

static void
die (gpointer client_data)
{
    quit (TRUE);
}

/*  Exported interface
 *  ------------------
*/
G_MODULE_EXPORT /* EXPORT:quit */
void
quit (gboolean force)
{
    if (!force)
    {
	if (session_managed)
	{
	    logout_session (client_session);
	    return;
	}
	else if (!xfce_confirm (_("Are you sure you want to exit?"),
				GTK_STOCK_QUIT, NULL))
	{
	    return;
	}
    }

    mcs_stop_watch ();

    save_panel ();

    panel_cleanup ();

    if (gtk_main_level ())
	gtk_main_quit ();

    DBG ("sigstate: %d", sigstate);
    if (sigstate != RESTART)
    {
	g_message ("%s: Exit", PACKAGE);
	exit (0);
    }
}

G_MODULE_EXPORT /* EXPORT:restart */
void
restart (void)
{
    /* don't really quit */
    sigstate = RESTART;

    /* calls gtk_main_quit() */
    quit (TRUE);

    /* progname is saved on startup 
     * TODO: do we need to pass on arguments? */
    g_message ("%s: restarting %s ...", PACKAGE, progname);
    execlp (progname, progname, NULL);
}

/*  Signals
 *  -------
*/
static gboolean
check_signal_state (void)
{
    static gboolean restarting = FALSE;

    if (sigstate != NOSIGNAL)
    {
	/* close open dialogs */
	destroy_controls_dialog ();
	destroy_menu_dialog ();

	if (sigstate == RESTART && !restarting)
	{
	    restarting = TRUE;

	    restart ();
	}
	else if (sigstate == QUIT)
	{
	    quit (TRUE);
	}
	else if (sigstate == QUIT_CONFIRM)
	{
	    sigstate = NOSIGNAL;
	    quit (FALSE);
	}
    }

    /* keep running */
    return TRUE;
}

static void
sighandler (int sig)
{
    /* Don't do any reall stuff here.
     * Only set a signal state flag. There's a timeout in the main loop
     * that tests the flag.
     * This will prevent problems with gtk main loop threads and stuff
     */
    switch (sig)
    {
	case SIGUSR1:
	    sigstate = RESTART;
	    break;

	case SIGUSR2:
	    sigstate = QUIT_CONFIRM;
	    break;

	case SIGINT:
	    /* hack: prevent the panel from saving config on ^C */
	    disable_user_config = TRUE;
	    /* fall through */

	default:
	    sigstate = QUIT;
    }
}

/*  Uniqueness
 *  ----------
*/
static Atom selection_atom = 0;
static Atom manager_atom = 0;
static Window selection_window = 0;

static gboolean
xfce_panel_is_running (void)
{
    char *selection_name;
    int scr;

    scr = DefaultScreen (gdk_display);

    TRACE ("check for running instance on screen %u", scr);

    if (!selection_atom)
    {
	selection_name = g_strdup_printf (XFCE_PANEL_SELECTION_FMT, scr);
	selection_atom = XInternAtom (gdk_display, selection_name, False);
	g_free (selection_name);
    }

    if (XGetSelectionOwner (gdk_display, selection_atom))
	return TRUE;

    return FALSE;
}

static void
xfce_panel_set_xselection (void)
{
    Display *display;
    int scr;
    char *selection_name;
    GtkWidget *invisible;

    display = GDK_DISPLAY ();
    scr = DefaultScreen (display);

    TRACE ("claiming xfdesktop manager selection for screen %d", scr);

    invisible = gtk_invisible_new ();
    gtk_widget_realize (invisible);

    if (!selection_atom)
    {
	selection_name = g_strdup_printf (XFCE_PANEL_SELECTION_FMT, scr);
	selection_atom = XInternAtom (display, selection_name, False);
	g_free (selection_name);
    }

    if (!manager_atom)
	manager_atom = XInternAtom (display, "MANAGER", False);

    selection_window = GDK_WINDOW_XWINDOW (invisible->window);

    XSelectInput (display, selection_window, PropertyChangeMask);
    XSetSelectionOwner (display, selection_atom,
			selection_window, GDK_CURRENT_TIME);

    /* Check to see if we managed to claim the selection. If not,
     * we treat it as if we got it then immediately lost it
     */
    if (XGetSelectionOwner (display, selection_atom) == selection_window)
    {
	XClientMessageEvent xev;

	xev.type = ClientMessage;
	xev.window = GDK_ROOT_WINDOW ();
	xev.message_type = manager_atom;
	xev.format = 32;
	xev.data.l[0] = GDK_CURRENT_TIME;
	xev.data.l[1] = selection_atom;
	xev.data.l[2] = selection_window;
	xev.data.l[3] = 0;	/* manager specific data */
	xev.data.l[4] = 0;	/* manager specific data */

	XSendEvent (display, GDK_ROOT_WINDOW (), False,
		    StructureNotifyMask, (XEvent *) & xev);
    }
    else
    {
	g_warning ("%s could not set selection ownership", PACKAGE);
	exit (1);
    }
}

/* Base Dir Spec compliance */
static void
ensure_base_dir_spec (XfceResourceType type, 
                      const char *old_subdir, const char *old_file,
                      const char *new_subdir, const char *new_file)
{
    char  *old, *new, *path, c;
    FILE *r, *w;
    GError *error = NULL;

    new = g_build_filename ("xfce4", new_subdir, NULL);
    path = xfce_resource_save_location (type, new, FALSE);
    g_free (new);

    if (!xfce_mkdirhier (path, 0700, &error))
    {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        goto path_failed;
    }

    new = g_build_filename (path, new_file, NULL);
    
    if (g_file_test (new, G_FILE_TEST_EXISTS))
    {
        DBG ("New file exists: %s\n", new);
        goto new_exists;
    }
    
    old = g_build_filename (xfce_get_userdir (), old_subdir, old_file, NULL);
    
    if (!g_file_test (old, G_FILE_TEST_EXISTS))
    {
        DBG ("No old config file was found: %s\n", old);
        goto old_failed;
    }

    if (!(r = fopen (old, "r")))
    {
        g_printerr ("Could not open file for reading: %s\n", old);
        goto r_failed;
    }

    if (!(w = fopen (new, "w")))
    {
        g_printerr ("Could not open file for writing: %s\n", new);
        goto w_failed;
    }

    while ((c = getc (r)) != EOF)
        putc (c, w);

    fclose (w);
    
w_failed:
    fclose (r);

r_failed:

old_failed:
    g_free (old);

new_exists:
    g_free (new);
    
path_failed:
    g_free (path);
}

/*  Main program
 *  ------------
*/
int
main (int argc, char **argv)
{
#ifdef HAVE_SIGACTION
    struct sigaction act;
#endif

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    if (argc == 2 &&
	(strequal (argv[1], "-v") || strequal (argv[1], "--version") ||
	 strequal (argv[1], "-h") || strequal (argv[1], "--help")))
    {
	g_print (_("\n"
		   " The Xfce Panel\n"
		   " Version %s\n\n"
		   " Part of the Xfce Desktop Environment\n"
		   " http://www.xfce.org\n\n"
		   " Licensed under the GNU GPL.\n\n"), VERSION);

	return 0;
    }

    gtk_init (&argc, &argv);

    progname = argv[0];

    if (xfce_panel_is_running ())
    {
	g_message ("%s is already running", PACKAGE);
	return 0;
    }
    else
    {
	xfce_panel_set_xselection ();
    }

    /* so clients are started on the correct screen */
    xfce_setenv ("DISPLAY", gdk_display_get_name (gdk_display_get_default ()),
		 TRUE);
#ifdef HAVE_SIGACTION
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

    /* hack to prevent arrow buttons from being cropped */
    gtk_rc_parse_string (RC_STRING);

    /* copy files from old location when no Base Dir Spec compliant
     * directories are found */
    ensure_base_dir_spec (XFCE_RESOURCE_CONFIG, 
                          "", "xfce4rc", 
                          "panel", "contents.xml");
    
    /* icon framework: names and id's */
    icons_init ();

    create_panel ();


    client_session = client_session_new (argc, argv, NULL /* data */ ,
					 SESSION_RESTART_IF_RUNNING, 40);

    client_session->save_yourself = save_yourself;
    client_session->die = die;

    session_managed = session_init (client_session);

#if DEBUG
    if (!session_managed)
    {
	g_message (_("%s: Successfully started without session management"),
		   PACKAGE);
    }
    else
    {
	g_message (_("%s: Successfully started with session management"),
		   PACKAGE);
    }
#endif

    /* signal state */
    g_timeout_add (500, (GSourceFunc) check_signal_state, NULL);

    gtk_main ();

    return 0;
}
