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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
    if (!disable_user_config)
	write_panel_config();
}

static void die (gpointer client_data)
{
    quit(TRUE);
}

/*  Main program
 *  ------------
*/
static void xfce_run(void)
{
    mcs_watch_xfce_channel();
    
    create_panel();

    gtk_main();
}

/* quit and restart */

void quit(gboolean force)
{
    if(!force)
    {
	if (session_managed)
	{
	    logout_session(client_session);
	    return;
	}
	else if (!confirm(_("Are you sure you want to Exit ?"), GTK_STOCK_QUIT, NULL))
	{
	    return;
	}
    }
    
    gtk_widget_hide(toplevel);

    if (!disable_user_config)
	write_panel_config();

    mcs_stop_watch();

    panel_cleanup();
    
    if (gtk_main_level())
	gtk_main_quit();

    exit(0);
}

void restart(void)
{
    int x, y;

    /* somehow the position gets lost here ... */
    x = position.x;
    y = position.y;
    
    if (!disable_user_config)
	write_panel_config();

    gtk_widget_hide(toplevel);

    panel_cleanup();
    gtk_widget_destroy(gtk_bin_get_child(GTK_BIN(toplevel)));

    create_panel();
    
    position.x = x;
    position.y = y;
    gtk_window_move(GTK_WINDOW(toplevel), x, y);
}

/* signal handler */

static void sighandler(int sig)
{
    switch (sig)
    {
        case SIGHUP:
	    /* FIXME: doesn't work when run from xterm and xterm closes
	       find something else!
	       ? does it still ?
	     */
	       
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
    int i;
    gboolean net_wm_support = FALSE;
    
    if(argc == 2 && (strequal(argv[1], "-v") || strequal(argv[1], "--version")))
    {
        g_print(_("xfce4, version %s\n\n"
                  "Part of the XFce Desktop Environment\n"
                  "http://www.xfce.org\n"), VERSION);
    }

    gtk_init(&argc, &argv);

    client_session = client_session_new(argc, argv, NULL /* data */ , 
	    				SESSION_RESTART_IF_RUNNING, 40);

    client_session->save_yourself = save_yourself;
    client_session->die = die;

    if(!(session_managed = session_init(client_session)))
        g_message("xfce4: Cannot connect to session manager");

    signal(SIGHUP, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

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
    
    icons_init();
    
    xfce_run();

    return 0;
}

