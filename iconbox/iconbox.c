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

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-client.h>

/* the iconbox */
typedef struct
{
    McsClient *mcs_client;
    SessionClient *session_client;

    int x, y;
    int width, height;
    int side, offset;

    int size;
    int direction;
    
    gboolean all_tasks;
    gboolean group_tasks;

    GtkWidget *window;

    GtkWidget *iconlist;
}
Iconbox;

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
static Iconbox *
create_iconbox (void)
{
    Iconbox *ib;
    
    ib = g_new0 (Iconbox, 1);

    g_idle_add ((GSourceFunc)gtk_main_quit, NULL);

    return ib;
}

/* settings */
static void
iconbox_read_settings (Iconbox *ib)
{
}

/* session management */
static void
iconbox_connect_to_session_manager (Iconbox *ib)
{
}

/* cleanup */
static void
cleanup_iconbox (Iconbox *ib)
{
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

    iconbox_connect_to_session_manager (ib);

    gtk_main ();

    cleanup_iconbox (ib);

    return 0;
}
