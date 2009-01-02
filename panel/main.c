/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "panel-app.h"
#include "panel-app-messages.h"

#ifndef _
#define _(x) x
#endif

/* globals */
static gboolean  opt_version   = FALSE;
static gboolean  opt_customize = FALSE;
static gboolean  opt_save      = FALSE;
static gboolean  opt_restart   = FALSE;
static gboolean  opt_quit      = FALSE;
static gboolean  opt_exit      = FALSE;
static gboolean  opt_add       = FALSE;
static gchar    *opt_client_id = NULL;

/* command line options */
static GOptionEntry option_entries[] =
{
    { "version",   'V', 0, G_OPTION_ARG_NONE, &opt_version,   N_ ("Show this message and exit"), NULL },
    { "customize", 'c', 0, G_OPTION_ARG_NONE, &opt_customize, N_ ("Show configuration dialog"), NULL },
    { "save",      's', 0, G_OPTION_ARG_NONE, &opt_save,      N_ ("Save configuration"), NULL },
    { "restart",   'r', 0, G_OPTION_ARG_NONE, &opt_restart,   N_ ("Restart panels"), NULL },
    { "quit",      'q', 0, G_OPTION_ARG_NONE, &opt_quit,      N_ ("End the session"), NULL },
    { "exit",      'x', 0, G_OPTION_ARG_NONE, &opt_exit,      N_ ("Close all panels and end the program"), NULL },
    { "add",       'a', 0, G_OPTION_ARG_NONE, &opt_add,       N_ ("Add new items"), NULL },

    { "sm-client-id", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING, &opt_client_id, NULL, NULL },
    { NULL }
};

/* main program */
gint
main (gint argc, gchar **argv)
{
    gint    msg = -1;
    GError *error = NULL;

    /* translation domain */
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    /* application name */
    g_set_application_name (PACKAGE_NAME);

    MARK ("start gtk_init_with_args ()");

    /* initialize gtk */
    if (!gtk_init_with_args (&argc, &argv, (gchar *) "", option_entries, (gchar *) GETTEXT_PACKAGE, &error))
    {
        g_print ("%s: %s\n", PACKAGE_NAME, error ? error->message : _("Failed to open display"));

        if (error != NULL)
            g_error_free (error);

        return EXIT_FAILURE;
    }

    /* handle the options */
    if (G_UNLIKELY (opt_version))
    {
        g_print ("%s %s (Xfce %s)\n\n", PACKAGE_NAME, PACKAGE_VERSION, xfce_version_string ());
        g_print ("%s\n", _("Copyright (c) 2004-2007"));
        g_print ("\t%s\n\n", _("The Xfce development team. All rights reserved."));
        g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
        g_print ("\n");

        return EXIT_SUCCESS;
    }
    else if (G_UNLIKELY (opt_customize))
    	msg = PANEL_APP_CUSTOMIZE;
    else if (G_UNLIKELY (opt_save))
    	msg = PANEL_APP_SAVE;
    else if (G_UNLIKELY (opt_restart))
    	msg = PANEL_APP_RESTART;
    else if (G_UNLIKELY (opt_quit))
    	msg = PANEL_APP_QUIT;
    else if (G_UNLIKELY (opt_exit))
    	msg = PANEL_APP_EXIT;
    else if (G_UNLIKELY (opt_add))
    	msg = PANEL_APP_ADD;

    /* handle the message, if there is any */
    if (G_UNLIKELY (msg >= 0))
    {
        if (!panel_app_send (msg))
        {
            if (msg != PANEL_APP_RESTART )
            {
                return EXIT_FAILURE;
            }
            /* else: continue and start new panel */
        }
        else
        {
            return EXIT_SUCCESS;
        }
    }

    MARK ("start panel_init()");
    msg = panel_app_init ();

    if (G_UNLIKELY (msg == INIT_FAILURE))
    {
    	return EXIT_FAILURE;
    }
    else if (G_UNLIKELY (msg == INIT_RUNNING))
    {
        g_message (_("xfce4-panel already running"));

        return EXIT_SUCCESS;
    }

    MARK ("start panel_app_run()");
    msg = panel_app_run (opt_client_id);
    MARK ("end panel_app_run()");

    if (G_UNLIKELY (msg == RUN_RESTART))
    {
        /* restart */
        g_message (_("Restarting xfce4-panel..."));
        execvp (argv[0], argv);
    }

    return G_UNLIKELY (msg == RUN_FAILURE) ? EXIT_FAILURE : EXIT_SUCCESS;
}

