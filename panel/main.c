/*  xfce4
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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

#include <config.h>
#include <my_gettext.h>

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libxfcegui4/session-client.h>
#include <libxfcegui4/netk-util.h>

#include "xfce.h"
#include "settings.h"
#include "mcs_client.h"

/*  session management
 *  ------------------
*/
static SessionClient *client_session;
static gboolean session_managed = FALSE;

static void save_yourself(gpointer data, int save_style, gboolean shutdown, int interact_style, gboolean fast)
{
    write_panel_config();
}

static void die (gpointer client_data)
{
    quit(TRUE);
}

/*  Exported interface
 *  ------------------
*/
void quit(gboolean force)
{
    if(!force)
    {
	if (session_managed)
	{
	    logout_session(client_session);
	    return;
	}
	else if (!confirm(_("Are you sure you want to exit?"), GTK_STOCK_QUIT, NULL))
	{
	    return;
	}
    }
    
    gtk_widget_hide(toplevel);

    write_panel_config();

    mcs_stop_watch();

#ifdef HAVE_STARTUP_NOTIFICATION
    free_startup_timeout ();
#endif

    panel_cleanup();
    
    if (gtk_main_level())
	gtk_main_quit();

    exit(0);
}

void restart(void)
{
    int x, y;

    /* somehow the position gets lost here ... 
     * FIXME: find out why, may be a real bug */
    x = position.x;
    y = position.y;
	
    write_panel_config();

    gtk_widget_hide(toplevel);

#ifdef HAVE_STARTUP_NOTIFICATION
    free_startup_timeout ();
#endif

    panel_cleanup();
    gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(toplevel)));

    create_panel();
    
    position.x = x;
    position.y = y;
    panel_set_position();
}

/*  Main program
 *  ------------
*/
static void sighandler(int sig)
{
    switch (sig)
    {
        case SIGHUP:
	    /* sleep for a second, we may be exiting X */
	    g_usleep(1000000);
	    restart();
	    break;
	case SIGINT:
	    /* hack: prevent the panel from saving config */
	    disable_user_config = TRUE;
	    /* fall through */
	default:
            quit(TRUE);
    }
}

int main(int argc, char **argv)
{
    struct sigaction act;
    int i;
    gboolean net_wm_support = FALSE;
    
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    gtk_set_locale ();
    gtk_init(&argc, &argv);

    if(argc == 2 && 
	(strequal(argv[1], "-v") || strequal(argv[1], "--version") || 
	 strequal(argv[1], "-h") || strequal(argv[1], "--help")))
    {
        g_print(_("%s, version %s\n"
                  "Part of the XFce Desktop Environment\n"
                  "http://www.xfce.org\n"), PACKAGE, VERSION);
	return 0;
    }

    for (i = 0; i < 5; i++)
    {
	net_wm_support = check_net_wm_support();

	if (net_wm_support)
	    break;
	else
	    g_usleep(2000000);
    }
    
    if (!net_wm_support)
    {
	show_error(_("Your window manager does not seem to support "
		       "the new window manager hints as defined on "
		       "http://www.freedesktop.org. \n"
		       "Some XFce features may not work as intended."));
    }		
    
    client_session = client_session_new(argc, argv, NULL /* data */ , 
	    				SESSION_RESTART_IF_RUNNING, 40);

    client_session->save_yourself = save_yourself;
    client_session->die = die;

    if(!(session_managed = session_init(client_session)))
        g_message("%s: Running without session manager", PACKAGE);

    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#else
    act.sa_flags = 0;
#endif
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    /* icon framework: names and id's */
    icons_init();
    
    create_panel();

    gtk_main();

    return 0;
}

