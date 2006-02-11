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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "panel-app.h"
#include "panel-app-messages.h"

#ifndef _
#define _(x) x
#endif

/* handle options */

static void
version_and_usage (void)
{
    setlocale(LC_ALL, "");
    g_print (_("\n"
               " Xfce Panel %s\n\n"
               " Part of the Xfce Desktop Environment\n"
               " http://www.xfce.org\n\n"
               " Licensed under the GNU GPL.\n\n"), VERSION);

    g_print (_(" Usage: %s [OPTIONS]\n\n"), PACKAGE);

    /* Only translate the descriptions, not the options itself */
    g_print (_(" OPTIONS\n"
               " -h, --help      Show this message and exit\n"
               " -v, --version   Show this message and exit\n"
               " -c, --customize Show configuration dialog\n"
               " -s, --save      Save configuration\n"
               " -r, --restart   Restart panels\n"
               " -q, --quit      End the session\n"
               " -x, --exit      Close all panels and end the program\n"
               " -a, --add       Add new items\n\n"
               ));
}

static gboolean
handle_options (int argc, char **argv, int *success)
{
    gboolean handled = FALSE;

    *success = 0;

    if (argc > 1 && argv[1][0] == '-')
    {
        /* help / version */
        if (!strcmp (argv[1], "-h")     || 
            !strcmp (argv[1], "-v")     ||
            !strcmp (argv[1], "--help") ||
            !strcmp (argv[1], "--version"))
        {
            handled = TRUE;

            version_and_usage ();
        }
        else
        {
            int msg = -1;
            
            if (!strcmp (argv[1], "-c") ||
                !strcmp (argv[1], "--customize"))
            {
                handled = TRUE;
                msg = PANEL_APP_CUSTOMIZE;
            }
            else if (!strcmp (argv[1], "-s") ||
                     !strcmp (argv[1], "--save"))
            {
                handled = TRUE;
                msg = PANEL_APP_SAVE;
            }
            else if (!strcmp (argv[1], "-r") ||
                     !strcmp (argv[1], "--restart"))
            {
                handled = TRUE;
                msg = PANEL_APP_RESTART;
            }
            else if (!strcmp (argv[1], "-q") ||
                     !strcmp (argv[1], "--quit"))
            {
                handled = TRUE;
                msg = PANEL_APP_QUIT;
            }
            else if (!strcmp (argv[1], "-x") ||
                     !strcmp (argv[1], "--exit"))
            {
                handled = TRUE;
                msg = PANEL_APP_EXIT;
            }
            else if (!strcmp (argv[1], "-a") ||
                     !strcmp (argv[1], "--add"))
            {
                handled = TRUE;
                msg = PANEL_APP_ADD;
            }

            if (msg >= 0)
            {
                gtk_init (&argc, &argv);
                panel_app_send (msg);
            }
        }
    }
    
    return handled;
}

/* main program */

int
main (int argc, char **argv)
{
    int success = 0;
    
    TIMER_INIT();
    
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");
    TIMER_ELAPSED("end xfce_textdomain()");
    
    if (handle_options (argc, argv, &success))
        exit (success);

    gtk_init (&argc, &argv);
    TIMER_ELAPSED("end gtk_init()");
    
    success = panel_app_init ();
    TIMER_ELAPSED("end panel_init()");
    
    if (success == -1)
    {
        return 1;
    }
    else if (success == 1)
    {
        g_message ("%s already running", PACKAGE);
        return 0;
    }

    TIMER_ELAPSED("start panel_app_run()");
    success = panel_app_run (argc, argv);
    TIMER_ELAPSED("end panel_app_run()");
    
    if (success == 1)
    {
        /* restart */
        g_message ("Restarting %s...", argv[0]);
        execvp (argv[0], argv);
    }

    return 0;
}

