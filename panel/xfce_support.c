/*  xfce_support.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *  startup notification added by Olivier fourdan based on gnome-desktop
 *  developed by Elliot Lee <sopwith@redhat.com> and Sid Vicious
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* xfce_support.c 
 * --------------
 * miscellaneous support functions that can be used by
 * all modules (and external plugins).
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <glib.h>
#include <gdk/gdk.h>

#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>
#include <libxfcegui4/libxfcegui4.h>

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#include <gdk/gdkx.h>
#define STARTUP_TIMEOUT (30 /* seconds */ * 1000)
#endif

#include "xfce.h"

extern char **environ;

/*  Files and directories
 *  ---------------------
*/
gchar *
get_save_dir (void)
{
    return (g_strdup (xfce_get_userdir ()));
}

gchar *
get_save_file (const gchar * name)
{
    return (xfce_get_userfile (name, NULL));
}

gchar **
get_read_dirs (void)
{
    gchar **dirs;

    if (disable_user_config)
    {
	dirs = g_new (gchar *, 2);

	dirs[0] = g_build_filename (SYSCONFDIR, SYSRCDIR, NULL);
	dirs[1] = NULL;
    }
    else
    {
	dirs = g_new (gchar *, 3);

	dirs[0] = get_save_dir ();
	dirs[1] = g_build_filename (SYSCONFDIR, SYSRCDIR, NULL);
	dirs[2] = NULL;
    }

    return dirs;
}

static gchar *
get_localized_system_rcfile (const gchar * name)
{
    gchar *sysrcfile, *result;

    sysrcfile = g_build_filename (SYSCONFDIR, SYSRCDIR, name, NULL);

    result = xfce_get_file_localized (sysrcfile);

    g_free (sysrcfile);

    return result;
}

gchar *
get_read_file (const gchar * name)
{
    if (!disable_user_config)
    {
	gchar *file = get_save_file (name);

	if (g_file_test (file, G_FILE_TEST_EXISTS))
	    return (file);
	else
	    g_free (file);
    }

    /* fall through */
    return get_localized_system_rcfile (name);
}

gchar **
get_plugin_dirs (void)
{
    gchar **dirs;

    if (disable_user_config)
    {
	dirs = g_new (gchar *, 2);

	dirs[0] = g_build_filename (LIBDIR, "panel-plugins", NULL);
	dirs[1] = NULL;
    }
    else
    {
	dirs = g_new (gchar *, 3);

	dirs[0] = xfce_get_userfile ("panel-plugins", NULL);
	dirs[1] = g_build_filename (LIBDIR, "panel-plugins", NULL);
	dirs[2] = NULL;
    }

    return (dirs);
}

gchar **
get_theme_dirs (void)
{
    gchar **dirs;

    if (disable_user_config)
    {
	dirs = g_new (gchar *, 2);

	dirs[0] = g_build_filename (DATADIR, "themes", NULL);
	dirs[1] = NULL;
    }
    else
    {
	dirs = g_new (gchar *, 3);

	dirs[0] = xfce_get_userfile ("themes", NULL);
	dirs[1] = g_build_filename (DATADIR, "themes", NULL);
	dirs[2] = NULL;
    }

    return (dirs);
}

void
write_backup_file (const gchar * path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1];

    snprintf (bakfile, MAXSTRLEN, "%s.bak", path);

    if ((bakfp = fopen (bakfile, "w")) && (fp = fopen (path, "r")))
    {
	int c;

	while ((c = fgetc (fp)) != EOF)
	    putc (c, bakfp);

	fclose (fp);
	fclose (bakfp);
    }
}

/*  Tooltips
 *  --------
*/
static GtkTooltips *tooltips = NULL;

void
add_tooltip (GtkWidget * widget, const char *tip)
{
    if (!tooltips)
	tooltips = gtk_tooltips_new ();

    gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
}

/*  X atoms and properties
*/
void
set_window_type_dock (GtkWidget * window, gboolean set)
{
    /* Copied from ROX Filer by Thomas Leonard */
    /* TODO: Use gdk function when it supports this type */
    GdkAtom window_type;
    gboolean mapped;

    if (set)
	window_type = gdk_atom_intern ("_NET_WM_WINDOW_TYPE_DOCK", FALSE);
    else
	window_type = gdk_atom_intern ("_NET_WM_WINDOW_TYPE_NORMAL", FALSE);

    if (!GTK_WIDGET_REALIZED (window))
	gtk_widget_realize (window);

    if ((mapped = GTK_WIDGET_MAPPED (window)))
	gtk_widget_unmap (window);

    gdk_property_change (window->window,
			 gdk_atom_intern ("_NET_WM_WINDOW_TYPE", FALSE),
			 gdk_atom_intern ("ATOM", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & window_type, 1);

    if (!set)
	gdk_property_delete (window->window,
			     gdk_atom_intern ("_WIN_LAYER", FALSE));

    if (mapped)
	gtk_widget_map (window);

    if (GTK_IS_WINDOW (panel.toplevel))
    {
	panel_set_position ();
	gtk_window_present (GTK_WINDOW (panel.toplevel));
    }
}

void
set_window_layer (GtkWidget * win, int layer)
{
    Screen *xscreen;
    Window xid;
    static Atom xa_ABOVE = 0;
    static Atom xa_BELOW = 0;

    if (!GTK_WIDGET_REALIZED (win))
	gtk_widget_realize (win);

    xscreen = DefaultScreenOfDisplay (GDK_DISPLAY ());
    xid = GDK_WINDOW_XID (win->window);

    if (!xa_ABOVE)
    {
	xa_ABOVE = XInternAtom (GDK_DISPLAY (), "_NET_WM_STATE_ABOVE", False);
	xa_BELOW = XInternAtom (GDK_DISPLAY (), "_NET_WM_STATE_BELOW", False);
    }

    switch (layer)
    {
	case ABOVE:
	    netk_change_state (xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
	    netk_change_state (xscreen, xid, TRUE, xa_ABOVE, None);
	    break;
	case BELOW:
	    netk_change_state (xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
	    netk_change_state (xscreen, xid, TRUE, xa_BELOW, None);
	    break;
	default:
	    netk_change_state (xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
    }
}

gboolean
check_net_wm_support (void)
{
    static Atom xa_NET_WM_SUPPORT = 0;
    Window xid;

    if (!xa_NET_WM_SUPPORT)
    {
	xa_NET_WM_SUPPORT = XInternAtom (GDK_DISPLAY (),
					 "_NET_SUPPORTING_WM_CHECK", False);
    }

    if (netk_get_window (GDK_ROOT_WINDOW (), xa_NET_WM_SUPPORT, &xid))
    {
/*      if (netk_get_window(xid, xa_NET_WM_SUPPORT, &xid))
            g_print("ok");
        else
            g_print("not ok");
*/
	return TRUE;
    }

    return FALSE;
}

void
set_window_skip (GtkWidget * win)
{
    Screen *xscreen;
    Window xid;
    static Atom xa_SKIP_PAGER = 0;
    static Atom xa_SKIP_TASKBAR = 0;

    if (!xa_SKIP_PAGER)
    {
	xa_SKIP_PAGER = XInternAtom (GDK_DISPLAY (),
				     "_NET_WM_STATE_SKIP_PAGER", False);
	xa_SKIP_TASKBAR = XInternAtom (GDK_DISPLAY (),
				       "_NET_WM_STATE_SKIP_TASKBAR", False);
    }

    xscreen = DefaultScreenOfDisplay (GDK_DISPLAY ());
    xid = GDK_WINDOW_XID (win->window);

    netk_change_state (xscreen, xid, TRUE, xa_SKIP_PAGER, xa_SKIP_TASKBAR);
}

/*  DND
 *  ---
*/
enum
{
    TARGET_STRING,
    TARGET_ROOTWIN,
    TARGET_URL
};

static GtkTargetEntry target_table[] = {
    {"text/uri-list", 0, TARGET_URL},
    {"STRING", 0, TARGET_STRING}
};

static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

void
dnd_set_drag_dest (GtkWidget * widget)
{
    gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL,
		       target_table, n_targets, GDK_ACTION_COPY);
}

static void
dnd_drop_cb (GtkWidget * widget, GdkDragContext * context,
	     gint x, gint y, GtkSelectionData * data,
	     guint info, guint time, gpointer user_data)
{
    GList *fnames;
    DropCallback f;

    fnames = gnome_uri_list_extract_filenames ((char *) data->data);

    f = DROP_CALLBACK (g_object_get_data (G_OBJECT (widget), "dropfunction"));

    f (widget, fnames, user_data);

    gnome_uri_list_free_strings (fnames);
    gtk_drag_finish (context, FALSE, (context->action == GDK_ACTION_MOVE),
		     time);
}

void
dnd_set_callback (GtkWidget * widget, DropCallback function, gpointer data)
{
    g_object_set_data (G_OBJECT (widget), "dropfunction",
		       (gpointer) function);

    g_signal_connect (widget, "drag-data-received",
		      G_CALLBACK (dnd_drop_cb), data);
}

/*** the next three routines are taken straight from gnome-libs so that the
     gtk-only version can receive drag and drops as well ***/
/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void
gnome_uri_list_free_strings (GList * list)
{
    g_list_foreach (list, (GFunc) g_free, NULL);
    g_list_free (list);
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList *
gnome_uri_list_extract_uris (const gchar * uri_list)
{
    const gchar *p, *q;
    gchar *retval;
    GList *result = NULL;

    g_return_val_if_fail (uri_list != NULL, NULL);

    p = uri_list;

    /* We don't actually try to validate the URI according to RFC
     * 2396, or even check for allowed characters - we just ignore
     * comments and trim whitespace off the ends.  We also
     * allow LF delimination as well as the specified CRLF.
     */
    while (p)
    {
	if (*p != '#')
	{
	    while (isspace ((int) (*p)))
		p++;

	    q = p;
	    while (*q && (*q != '\n') && (*q != '\r'))
		q++;

	    if (q > p)
	    {
		q--;
		while (q > p && isspace ((int) (*q)))
		    q--;

		retval = (char *) g_malloc (q - p + 2);
		strncpy (retval, p, q - p + 1);
		retval[q - p + 1] = '\0';

		result = g_list_prepend (result, retval);
	    }
	}
	p = strchr (p, '\n');
	if (p)
	    p++;
    }

    return g_list_reverse (result);
}


/**
 * gnome_uri_list_extract_filenames:
 * @uri_list: an uri-list in the standard format
 *
 * Returns a GList containing strings allocated with g_malloc
 * that contain the filenames in the uri-list.
 *
 * Note that unlike gnome_uri_list_extract_uris() function, this
 * will discard any non-file uri from the result value.
 */
GList *
gnome_uri_list_extract_filenames (const gchar * uri_list)
{
    GList *tmp_list, *node, *result;

    g_return_val_if_fail (uri_list != NULL, NULL);

    result = gnome_uri_list_extract_uris (uri_list);

    tmp_list = result;
    while (tmp_list)
    {
	gchar *s = (char *) tmp_list->data;

	node = tmp_list;
	tmp_list = tmp_list->next;

	if (!strncmp (s, "file:", 5))
	{
	    /* added by Jasper Huijsmans
	       remove leading multiple slashes */
	    if (!strncmp (s + 5, "///", 3))
		node->data = g_strdup (s + 7);
	    else
		node->data = g_strdup (s + 5);
	}
	else
	{
	    node->data = g_strdup (s);
	}
	g_free (s);
    }
    return result;
}

/*  File open dialog
 *  ----------------
*/
static void
fs_ok_cb (GtkDialog * fs)
{
    gtk_dialog_response (fs, GTK_RESPONSE_OK);
}


static void
fs_cancel_cb (GtkDialog * fs)
{
    gtk_dialog_response (fs, GTK_RESPONSE_CANCEL);
}

#if 0
/* escaping spaces */
static char *
path_escape_spaces (const char *path)
{
    char *newpath;
    const char *s;
    int n = 0;

    for (s = path; (s = strchr (s, ' ')) != NULL; s++)
    {
	n++;
    }

    newpath = g_new (char, strlen (path) + n + 1);

    for (n = 0, s = path; s && *s; n++, s++)
    {
	if (*s == ' ')
	{
	    newpath[n] = '\\';
	    newpath[++n] = ' ';
	}
	else
	{
	    newpath[n] = *s;
	}
    }

    newpath[n] = '\0';

    return newpath;
}

static char *
path_unescape_spaces (const char *path)
{
    char *newpath;
    const char *s;
    int n = 0;

    for (s = path; s && *s; s++)
    {
	if (*s == '\\' && *(++s) == ' ')
	{
	    s++;
	    n++;
	}
	else if (*s == ' ')
	{
	    break;
	}
    }

    newpath = g_new (char, s - path - n + 1);

    for (n = 0, s = path; s && *s; n++, s++)
    {
	if (*s == '\\' && *(s + 1) == ' ')
	{
	    s++;
	}

	newpath[n] = *s;
    }

    newpath[n] = '\0';

    return newpath;
}
#endif

/* Any of the arguments may be NULL */
static char *
real_select_file (const char *title, const char *path,
		  GtkWidget * parent, gboolean with_preview)
{
    const char *t = (title) ? title : _("Select file");
    GtkWidget *fs;
    char *name = NULL;
    const char *temp;

    fs = preview_file_selection_new (t, with_preview);

    if (path)
    {
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (fs), path);
    }

    if (parent)
	gtk_window_set_transient_for (GTK_WINDOW (fs), GTK_WINDOW (parent));

    g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (fs)->ok_button),
			      "clicked", G_CALLBACK (fs_ok_cb), fs);

    g_signal_connect_swapped (G_OBJECT
			      (GTK_FILE_SELECTION (fs)->cancel_button),
			      "clicked", G_CALLBACK (fs_cancel_cb), fs);

    if (gtk_dialog_run (GTK_DIALOG (fs)) == GTK_RESPONSE_OK)
    {
	temp = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

	if (temp && strlen (temp))
	    /* Too messy: commands should be escaped, internal filenames
	     * should not. The user now has to escape commands manually
	     name = path_escape_spaces (temp); */
	    name = g_strdup (temp);
	else
	    name = NULL;
    }

    gtk_widget_destroy (fs);

    return name;
}

char *
select_file_name (const char *title, const char *path, GtkWidget * parent)
{
    return real_select_file (title, path, parent, FALSE);
}

char *
select_file_with_preview (const char *title, const char *path,
			  GtkWidget * parent)
{
    return real_select_file (title, path, parent, TRUE);
}

/*  Executing commands
 *  ------------------
*/

#ifdef HAVE_LIBSTARTUP_NOTIFICATION

typedef struct
{
    GSList *contexts;
    guint timeout_id;
}
StartupTimeoutData;
static StartupTimeoutData *startup_timeout_data = NULL;

static void
sn_error_trap_push (SnDisplay * display, Display * xdisplay)
{
    gdk_error_trap_push ();
}

static void
sn_error_trap_pop (SnDisplay * display, Display * xdisplay)
{
    gdk_error_trap_pop ();
}

extern char **environ;

static gchar **
make_spawn_environment_for_sn_context (SnLauncherContext *
				       sn_context, char **envp)
{
    gchar **retval = NULL;
    int i, j;
    int desktop_startup_id_len;

    if (envp == NULL)
    {
	envp = environ;
    }
    for (i = 0; envp[i]; i++);

    retval = g_new (gchar *, i + 2);

    desktop_startup_id_len = strlen ("DESKTOP_STARTUP_ID");

    for (i = 0, j = 0; envp[i]; i++)
    {
	if (strncmp (envp[i], "DESKTOP_STARTUP_ID", desktop_startup_id_len) !=
	    0)
	{
	    retval[j] = g_strdup (envp[i]);
	    ++j;
	}
    }
    retval[j] =
	g_strdup_printf ("DESKTOP_STARTUP_ID=%s",
			 sn_launcher_context_get_startup_id (sn_context));
    ++j;
    retval[j] = NULL;

    return retval;
}

static gboolean
startup_timeout (void *data)
{
    StartupTimeoutData *std = data;
    GSList *tmp;
    GTimeVal now;
    int min_timeout;

    min_timeout = STARTUP_TIMEOUT;

    g_get_current_time (&now);

    tmp = std->contexts;
    while (tmp != NULL)
    {
	SnLauncherContext *sn_context = tmp->data;
	GSList *next = tmp->next;
	long tv_sec, tv_usec;
	double elapsed;

	sn_launcher_context_get_last_active_time (sn_context, &tv_sec,
						  &tv_usec);

	elapsed =
	    ((((double) now.tv_sec - tv_sec) * G_USEC_PER_SEC +
	      (now.tv_usec - tv_usec))) / 1000.0;

	if (elapsed >= STARTUP_TIMEOUT)
	{
	    std->contexts = g_slist_remove (std->contexts, sn_context);
	    sn_launcher_context_complete (sn_context);
	    sn_launcher_context_unref (sn_context);
	}
	else
	{
	    min_timeout = MIN (min_timeout, (STARTUP_TIMEOUT - elapsed));
	}

	tmp = next;
    }

    if (std->contexts == NULL)
    {
	std->timeout_id = 0;
    }
    else
    {
	std->timeout_id = g_timeout_add (min_timeout, startup_timeout, std);
    }

    return FALSE;
}

static void
add_startup_timeout (SnLauncherContext * sn_context)
{
    if (startup_timeout_data == NULL)
    {
	startup_timeout_data = g_new (StartupTimeoutData, 1);
	startup_timeout_data->contexts = NULL;
	startup_timeout_data->timeout_id = 0;
    }

    sn_launcher_context_ref (sn_context);
    startup_timeout_data->contexts =
	g_slist_prepend (startup_timeout_data->contexts, sn_context);

    if (startup_timeout_data->timeout_id == 0)
    {
	startup_timeout_data->timeout_id =
	    g_timeout_add (STARTUP_TIMEOUT, startup_timeout,
			   startup_timeout_data);
    }
}

void
free_startup_timeout (void)
{
    StartupTimeoutData *std = startup_timeout_data;

    if (!std)
    {
	/* No startup notification used, return silently */
	return;
    }

    g_slist_foreach (std->contexts, (GFunc) sn_launcher_context_unref, NULL);
    g_slist_free (std->contexts);

    if (std->timeout_id != 0)
    {
	g_source_remove (std->timeout_id);
	std->timeout_id = 0;
    }

    g_free (std);
    startup_timeout_data = NULL;
}

#endif /* HAVE_LIBSTARTUP_NOTIFICATION */

static void
real_exec_cmd (const char *cmd, gboolean in_terminal,
	       gboolean use_sn, gboolean silent)
{
    gchar *execute = NULL;
    GError *error = NULL;
    gboolean success = TRUE;
    gchar **free_envp = NULL;
    gchar **envp = environ;
    gchar **argv = NULL;
    gboolean retval;

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    SnLauncherContext *sn_context = NULL;
    SnDisplay *sn_display = NULL;
#endif

    if (g_path_is_absolute (cmd) && g_file_test (cmd, G_FILE_TEST_IS_DIR))
    {
	if (in_terminal)
	{
	    execute = g_strdup_printf ("xfterm4 %s", cmd);
	}
	else
	{
	    execute = g_strdup_printf ("xftree4 %s", cmd);
	}
    }
    else if (in_terminal)
    {
	execute = g_strdup_printf ("xfterm4 -e %s", cmd);
    }
    else
    {
	execute = g_strdup (cmd);
    }

    if (!g_shell_parse_argv (execute, NULL, &argv, &error))
    {
	g_free (execute);
	if (error)
	{
	    g_warning ("%s: %s\n", PACKAGE, error->message);
	    g_error_free (error);
	}
	return;
    }

    free_envp = NULL;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    if (use_sn)
    {
	sn_display =
	    sn_display_new (gdk_display, sn_error_trap_push,
			    sn_error_trap_pop);
	if (sn_display != NULL)
	{
	    sn_context =
		sn_launcher_context_new (sn_display,
					 DefaultScreen (gdk_display));
	    if ((sn_context != NULL) &&
		!sn_launcher_context_get_initiated (sn_context))
	    {
		sn_launcher_context_set_binary_name (sn_context, execute);
		sn_launcher_context_initiate (sn_context,
					      g_get_prgname ()?
					      g_get_prgname () : "unknown",
					      argv[0], CurrentTime);
		free_envp =
		    make_spawn_environment_for_sn_context (sn_context, NULL);
	    }
	}
    }
#endif

    g_free (execute);

    if (silent)
    {
	retval =
	    g_spawn_async (NULL, argv, free_envp ? free_envp : envp,
			   G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
	if (!retval)
	{
	    if (error)
	    {
		g_warning ("%s: %s\n", PACKAGE, error->message);
		g_error_free (error);
	    }
	    success = FALSE;
	}
    }
    else
    {
	success =
	    exec_command_full_with_envp (argv, free_envp ? free_envp : envp);
    }

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    if (use_sn)
    {
	if (sn_context != NULL)
	{
	    if (!success)
	    {
		sn_launcher_context_complete (sn_context);	/* end sequence */
	    }
	    else
	    {
		add_startup_timeout (sn_context);
		sn_launcher_context_unref (sn_context);
	    }
	}
	if (free_envp)
	{
	    g_strfreev (free_envp);
	    free_envp = NULL;
	}
	if (sn_display)
	{
	    sn_display_unref (sn_display);
	}
    }
#endif /* HAVE_LIBSTARTUP_NOTIFICATION */
    g_strfreev (argv);
}

/*
 * This is used to get better UI response. Exec the command when the panel
 * becomes idle, so Gtk has time to do the important UI work first.
 */
typedef struct _ActionCommand ActionCommand;
struct _ActionCommand
{
    gchar *cmd;
    gboolean in_terminal;
    gboolean use_sn;
    gboolean silent;
};

static gboolean
delayed_exec (ActionCommand * command)
{
    real_exec_cmd (command->cmd, command->in_terminal, command->use_sn,
		   command->silent);

    g_free (command->cmd);
    g_free (command);

    return (FALSE);
}

static void
schedule_exec (const gchar * cmd, gboolean in_terminal, gboolean use_sn,
	       gboolean silent)
{
    ActionCommand *command;

    command = g_new (ActionCommand, 1);
    command->cmd = g_strdup (cmd);
    command->in_terminal = in_terminal;
    command->use_sn = use_sn;
    command->silent = silent;

    (void) g_idle_add ((GSourceFunc) delayed_exec, (gpointer) command);
}

void
exec_cmd (const char *cmd, gboolean in_terminal, gboolean use_sn)
{
    g_return_if_fail (cmd != NULL);
    schedule_exec (cmd, in_terminal, use_sn, FALSE);
}

/* without error reporting dialog */
void
exec_cmd_silent (const char *cmd, gboolean in_terminal, gboolean use_sn)
{
    g_return_if_fail (cmd != NULL);
    schedule_exec (cmd, in_terminal, use_sn, TRUE);
}
