/*  $Id$
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif

#include "launcher.h"
#include "launcher-exec.h"

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#ifdef GDK_WINDOWING_X11
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#endif
#include <libsn/sn.h>
#endif



#ifdef HAVE_LIBSTARTUP_NOTIFICATION
typedef struct
{
  SnLauncherContext *sn_launcher;
  guint              timeout_id;
  guint              watch_id;
  GPid               pid;
} LauncherStartupData;
#endif



/* Prototypes */
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
static gint             launcher_exec_get_active_workspace_number (GdkScreen             *screen);
static gboolean         launcher_exec_startup_timeout             (gpointer               data);
static void             launcher_exec_startup_timeout_destroy     (gpointer               data);
static void             launcher_exec_startup_watch               (GPid                   pid,
                                                                   gint                   status,
                                                                   gpointer               data);
#endif
static void             launcher_exec_string_append_quoted        (GString               *string,
                                                                   const gchar           *unquoted);
static gchar          **launcher_exec_parse_argv                  (LauncherEntry         *entry,
                                                                   GSList                *list,
                                                                   GError               **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;;
static gboolean         launcher_exec_on_screen                   (GdkScreen              *screen,
                                                                   LauncherEntry          *entry,
                                                                   GSList                 *list);



#ifdef HAVE_LIBSTARTUP_NOTIFICATION
static gint
launcher_exec_get_active_workspace_number (GdkScreen *screen)
{
    GdkWindow *root;
    gulong     bytes_after_ret = 0;
    gulong     nitems_ret = 0;
    guint     *prop_ret = NULL;
    Atom       _NET_CURRENT_DESKTOP;
    Atom       _WIN_WORKSPACE;
    Atom       type_ret = None;
    gint       format_ret;
    gint       ws_num = 0;

    gdk_error_trap_push ();

    root = gdk_screen_get_root_window (screen);

    /* determine the X atom values */
    _NET_CURRENT_DESKTOP = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_NET_CURRENT_DESKTOP", False);
    _WIN_WORKSPACE = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_WIN_WORKSPACE", False);

    if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root), GDK_WINDOW_XWINDOW (root),
                            _NET_CURRENT_DESKTOP, 0, 32, False, XA_CARDINAL,
                            &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                            (gpointer) &prop_ret) != Success)
    {
        if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root), GDK_WINDOW_XWINDOW (root),
                                _WIN_WORKSPACE, 0, 32, False, XA_CARDINAL,
                                &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                                (gpointer) &prop_ret) != Success)
        {
            if (G_UNLIKELY (prop_ret != NULL))
            {
                XFree (prop_ret);
                prop_ret = NULL;
            }
        }
    }

    if (G_LIKELY (prop_ret != NULL))
    {
        if (G_LIKELY (type_ret != None && format_ret != 0))
            ws_num = *prop_ret;
        XFree (prop_ret);
    }

    gdk_error_trap_pop ();

    return ws_num;
}



static gboolean
launcher_exec_startup_timeout (gpointer data)
{
    LauncherStartupData *startup_data = data;
    GTimeVal             now;
    gdouble              elapsed;
    glong                tv_sec;
    glong                tv_usec;

    /* determine the amount of elapsed time */
    g_get_current_time (&now);
    sn_launcher_context_get_last_active_time (startup_data->sn_launcher, &tv_sec, &tv_usec);
    elapsed = (((gdouble) now.tv_sec - tv_sec) * G_USEC_PER_SEC + (now.tv_usec - tv_usec)) / 1000.0;

    /* check if the timeout was reached */
    if (elapsed >= STARTUP_TIMEOUT)
    {
        /* abort the startup notification */
        sn_launcher_context_complete (startup_data->sn_launcher);
        sn_launcher_context_unref (startup_data->sn_launcher);
        startup_data->sn_launcher = NULL;
    }

    /* keep the startup timeout if not elapsed */
    return (elapsed < STARTUP_TIMEOUT);
}



static void
launcher_exec_startup_timeout_destroy (gpointer data)
{
    LauncherStartupData *startup_data = data;

    g_return_if_fail (startup_data->sn_launcher == NULL);

    /* cancel the watch (if any) */
    if (startup_data->watch_id != 0)
        g_source_remove (startup_data->watch_id);

    /* close the PID */
    g_spawn_close_pid (startup_data->pid);

    /* release the startup data */
    panel_slice_free (LauncherStartupData, startup_data);
}



static void
launcher_exec_startup_watch (GPid     pid,
                             gint     status,
                             gpointer data)
{
    LauncherStartupData *startup_data = data;
    gint                 ret, serrno;

    g_return_if_fail (startup_data->sn_launcher != NULL);
    g_return_if_fail (startup_data->watch_id != 0);
    g_return_if_fail (startup_data->pid == pid);

    /* abort the startup notification (application exited) */
    sn_launcher_context_complete (startup_data->sn_launcher);
    sn_launcher_context_unref (startup_data->sn_launcher);
    startup_data->sn_launcher = NULL;

    /* avoid zombie processes */
    serrno = errno;
    while (1)
      {
        /* get the child process state without hanging */
        ret = waitpid (WAIT_ANY, NULL, WNOHANG);
        
        /* exit if there is nothing to wait for */
        if (ret == 0 || ret < 0)
          break;
      }
    errno = serrno;

    /* cancel the startup notification timeout */
    /* this will also activate the timeout_destroy function */
    g_source_remove (startup_data->timeout_id);
}
#endif



static void
launcher_exec_string_append_quoted (GString     *string,
                                    const gchar *unquoted)
{
    gchar *quoted;

    quoted = g_shell_quote (unquoted);
    g_string_append (string, quoted);
    g_free (quoted);
}



static gchar **
launcher_exec_parse_argv (LauncherEntry   *entry,
                          GSList          *list,
                          GError         **error)
{
    GString      *command_line = g_string_new (NULL);
    const gchar  *p;
    gchar        *t;
    GSList       *li;
    gchar       **argv = NULL;

    /* build the full command */
    for (p = entry->exec; *p != '\0'; ++p)
    {
        if (p[0] == '%' && p[1] != '\0')
        {
            switch (*++p)
            {
                case 'u':
                case 'f':
                    /* a single filename or url */
                    if (list != NULL)
                        launcher_exec_string_append_quoted (command_line, (gchar *) list->data);
                    break;

                case 'U':
                case 'F':
                    /* a list of filenames or urls */
                    for (li = list; li != NULL; li = li->next)
                    {
                        if (G_LIKELY (li != list))
                            g_string_append_c (command_line, ' ');

                        launcher_exec_string_append_quoted (command_line, (gchar *) li->data);
                    }
                    break;

                case 'd':
                    /* directory containing the file that would be passed in a %f field */
                    if (list != NULL)
                    {
                        t = g_path_get_dirname ((gchar *) list->data);
                        if (t != NULL)
                        {
                            launcher_exec_string_append_quoted (command_line, t);
                            g_free (t);
                        }
                    }
                    break;

                case 'D':
                    /* list of directories containing the files that would be passed in to a %F field */
                    for (li = list; li != NULL; li = li->next)
                    {
                        t = g_path_get_dirname (li->data);
                        if (t != NULL)
                        {
                            if (G_LIKELY (li != list))
                                g_string_append_c (command_line, ' ');

                            launcher_exec_string_append_quoted (command_line, t);
                            g_free (t);
                        }
                    }
                    break;

                case 'n':
                    /* a single filename (without path). */
                    if (list != NULL)
                    {
                        t = g_path_get_basename ((gchar *) list->data);
                        if (t != NULL)
                        {
                            launcher_exec_string_append_quoted (command_line, t);
                            g_free (t);
                        }
                    }
                    break;

                case 'N':
                    /* a list of filenames (without paths) */
                    for (li = list; li != NULL; li = li->next)
                    {
                        t = g_path_get_basename (li->data);
                        if (t != NULL)
                        {
                            if (G_LIKELY (li != list))
                                g_string_append_c (command_line, ' ');

                            launcher_exec_string_append_quoted (command_line, t);
                            g_free (t);
                        }
                    }
                    break;

                case 'i':
                    /* the icon key of the desktop entry */
                    if (G_LIKELY (entry->icon != NULL))
                    {
                        g_string_append (command_line, "--icon ");
                        launcher_exec_string_append_quoted (command_line, entry->icon);
                    }
                    break;

                case 'c':
                    /* the translated name of the application */
                    if (G_LIKELY (entry->name != NULL))
                        launcher_exec_string_append_quoted (command_line, entry->name);
                    break;

                case '%':
                    /* percentage character */
                    g_string_append_c (command_line, '%');
                    break;
            }
        }
        else
        {
            g_string_append_c (command_line, *p);
        }
    }

    DBG ("Execute: %s", command_line->str);

    /* create the argv */
    if (G_LIKELY (command_line->str != NULL))
    {
        if (entry->terminal == FALSE)
        {
            /* use glib to parge the argv */
            g_shell_parse_argv (command_line->str, NULL, &argv, error);
        }
        else
        {
            /* we parse our own argv here so exo-open will handle all attributes without problems */
            argv = g_new (gchar *, 5);
            argv[0] = g_strdup ("exo-open");
            argv[1] = g_strdup ("--launch");
            argv[2] = g_strdup ("TerminalEmulator");
            argv[3] = g_strdup (command_line->str);
            argv[4] = NULL;
        }
    }

    /* cleanup */
    g_string_free (command_line, TRUE);

    return argv;
}


static gboolean
launcher_exec_on_screen (GdkScreen     *screen,
                         LauncherEntry *entry,
                         GSList        *list)
{
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    SnLauncherContext    *sn_launcher = NULL;
    SnDisplay            *sn_display = NULL;
    LauncherStartupData  *startup_data;
    gint                  sn_workspace;
    extern gchar        **environ;
    gint                  n, m;
#endif
    gboolean              succeed = FALSE;
    GError               *error = NULL;
    gchar               **argv;
    gchar               **envp = NULL;
    GtkWidget            *dialog;
    GSpawnFlags           flags = G_SPAWN_SEARCH_PATH;
    GPid                  pid;

    /* parse the full command */
    if ((argv = launcher_exec_parse_argv (entry, list, &error)) == NULL)
        goto error;

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    /* setup startup notification, only when not running in terminal */
    if (entry->startup && !entry->terminal)
    {
        sn_display = sn_display_new (GDK_SCREEN_XDISPLAY (screen),
                                     (SnDisplayErrorTrapPush) gdk_error_trap_push,
                                     (SnDisplayErrorTrapPop) gdk_error_trap_pop);

        if (G_LIKELY (sn_display != NULL))
        {
            /* create new startup context */
            sn_launcher = sn_launcher_context_new (sn_display, GDK_SCREEN_XNUMBER (screen));

            if (G_LIKELY (sn_launcher != NULL && !sn_launcher_context_get_initiated (sn_launcher)))
            {
                /* setup the startup notification context */
                sn_workspace = launcher_exec_get_active_workspace_number (screen);
                sn_launcher_context_set_binary_name (sn_launcher, argv[0]);
                sn_launcher_context_set_workspace (sn_launcher, sn_workspace);
                sn_launcher_context_initiate (sn_launcher, g_get_prgname (), argv[0], CurrentTime);

                /* count environ items */
                for (n = 0; environ[n] != NULL; ++n)
                    ;

                /* alloc new envp string */
                envp = g_new (gchar *, n + 2);

                /* copy the environ vars into the envp */
                for (n = m = 0; environ[n] != NULL; ++n)
                    if (G_LIKELY (strncmp (environ[n], "DESKTOP_STARTUP_ID", 18) != 0))
                        envp[m++] = g_strdup (environ[n]);

                /* append the startup notification id */
                envp[m++] = g_strconcat ("DESKTOP_STARTUP_ID=", sn_launcher_context_get_startup_id (sn_launcher), NULL);
                envp[m] = NULL;

                /* we want to watch the child process */
                flags |= G_SPAWN_DO_NOT_REAP_CHILD;
            }
        }
    }
#endif

    /* spawn the application */
    succeed = gdk_spawn_on_screen (screen,
                                   entry->path,
                                   argv,
                                   envp,
                                   flags,
                                   NULL, NULL,
                                   &pid,
                                   &error);

    /* cleanup the argv */
    g_strfreev (argv);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    /* handle the sn launcher context */
    if (sn_launcher != NULL)
    {
        if (G_UNLIKELY (!succeed))
        {
            /* abort the sn sequence */
            sn_launcher_context_complete (sn_launcher);
            sn_launcher_context_unref (sn_launcher);
        }
        else
        {
            /* schedule a startup notification timeout */
            startup_data = panel_slice_new (LauncherStartupData);
            startup_data->sn_launcher = sn_launcher;
            startup_data->timeout_id = g_timeout_add_full (G_PRIORITY_LOW, STARTUP_TIMEOUT,
                                                           launcher_exec_startup_timeout,
                                                           startup_data, launcher_exec_startup_timeout_destroy);
            startup_data->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid, launcher_exec_startup_watch,
                                                             startup_data, NULL);
            startup_data->pid = pid;
        }
    }

    /* release the sn display */
    if (sn_display != NULL)
        sn_display_unref (sn_display);

    if (envp != NULL)
        g_strfreev (envp);
#endif

error:
    if (G_UNLIKELY (error != NULL))
    {
        /* create new warning dialog */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("Failed to launch \"%s\""),
                                         entry->name);

        /* show g's error message, if there is any */
        if (G_LIKELY (error->message))
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                      "%s.", error->message);

        /* popup the dialog */
        gtk_dialog_run (GTK_DIALOG (dialog));

        /* cleanup */
        gtk_widget_destroy (dialog);
        g_error_free (error);
    }

    return succeed;
}



void
launcher_execute (GdkScreen     *screen,
                  LauncherEntry *entry,
                  GSList        *file_list)
{
    GSList   *li;
    GSList    fake;
    gboolean  proceed = TRUE;

    /* be secure */
    if (G_UNLIKELY (screen == NULL))
        screen = gdk_screen_get_default ();

    /* maybe no command have been filed yet */
    if (G_UNLIKELY (entry->exec == NULL))
        return;

    /* check if the launcher supports (and needs) multiple instances */
    if (file_list != NULL &&
        strstr (entry->exec, "%F") == NULL &&
        strstr (entry->exec, "%U") == NULL)
    {
        /* fake an empty list */
        fake.next = NULL;

        /* run new instance for each file in the list */
        for (li = file_list; li != NULL && proceed; li = li->next)
        {
            /* point to data */
            fake.data = li->data;

            /* spawn */
            proceed = launcher_exec_on_screen (screen, entry, &fake);
        }
    }
    else
    {
        /* spawn */
        launcher_exec_on_screen (screen, entry, file_list);
    }
}



void
launcher_execute_from_clipboard (GdkScreen     *screen,
                                 LauncherEntry *entry)
{
    GtkClipboard     *clipboard;
    gchar            *text = NULL;
    GSList           *file_list = NULL;
    GtkSelectionData  selection_data;

    /* maybe no command have been filed yet */
    if (G_UNLIKELY (entry->exec == NULL))
        return;

    /* get the clipboard */
    clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);

    /* get clipboard text */
    if (G_LIKELY (clipboard))
        text = gtk_clipboard_wait_for_text (clipboard);

    /* try other clipboard if this one was empty */
    if (text == NULL)
    {
        /* get the clipboard */
        clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

        /* get clipboard text */
        if (G_LIKELY (clipboard))
        text = gtk_clipboard_wait_for_text (clipboard);
    }

    if (G_LIKELY (text != NULL))
    {
        /* create some fake selection data */
        selection_data.data = (guchar *) text;
        selection_data.length = strlen (text);

        /* parse the filelist, this way we can handle 'copied' file from thunar */
        file_list = launcher_file_list_from_selection (&selection_data);

        if (G_LIKELY (file_list != NULL))
        {
            /* run the command with argument from clipboard */
            launcher_exec_on_screen (screen, entry, file_list);

            /* cleanup */
            g_slist_free_all (file_list);
    }

        /* cleanup */
        g_free (text);
    }
}
