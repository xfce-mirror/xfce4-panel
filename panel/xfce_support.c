/*  $Id$
 *  
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *  
 *  Startup notification added by Olivier fourdan based on gnome-desktop
 *  developed by Elliot Lee <sopwith@redhat.com> and Sid Vicious.
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

#include <libxfce4util/libxfce4util.h>
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
char *
get_save_dir (void)
{
    char *dir;

    dir = xfce_resource_save_location (XFCE_RESOURCE_CONFIG,
                                       "xfce4" G_DIR_SEPARATOR_S "panel", 
                                       FALSE);

    if (!xfce_mkdirhier (dir, 0700, NULL))
    {
        g_free (dir);
        dir = g_strdup (xfce_get_homedir());
    }

    return dir;
}

char *
get_save_file (const char * name)
{
    int scr;
    char *path, *file = NULL;

    scr = DefaultScreen (gdk_display);

    if (scr == 0)
    {
        path = g_build_filename ("xfce4", "panel", name, NULL);
    }
    else
    {
	char *realname;

	realname = g_strdup_printf ("%s.%u", name, scr);

        path = g_build_filename ("xfce4", "panel", realname, NULL);
        
	g_free (realname);
    }

    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, TRUE);
    g_free (path);
    
    return file;
}

static char *
get_localized_rcfile (const char *path)
{
    char *fmt, *result;
    char buffer[PATH_MAX];

    fmt = g_strconcat (path, ".%l", NULL);

    result = xfce_get_path_localized (buffer, PATH_MAX, fmt, NULL,
				      G_FILE_TEST_EXISTS);

    g_free (fmt);

    if (result)
    {
	DBG ("file: %s", buffer);
	return g_strdup (buffer);
    }

    if (g_file_test (path, G_FILE_TEST_EXISTS))
    {
	DBG ("file: %s", path);
	return g_strdup (path);
    }

    return NULL;
}

char *
get_read_file (const char * name)
{
    char **paths, **p, *file = NULL;

    if (G_UNLIKELY (disable_user_config))
    {
	p = paths = g_new (char *, 2);

	paths[0] = g_build_filename (SYSCONFDIR, "xdg", "xfce4", "panel", 
                                     name, NULL);
	paths[1] = NULL;
    }
    else
    {
        int n;
        char **d;
        
        d = xfce_resource_dirs (XFCE_RESOURCE_CONFIG);

        for (n = 0; d[n] != NULL; ++n)
            /**/ ;

        p = paths = g_new0 (char*, n + 1);

        for (n = 0; d[n] != NULL; ++n)
            paths[n] = g_build_filename (d[n], "xfce4", "panel", name, NULL);
            
        g_strfreev (d);

        /* first entry is not localized ($XDG_CONFIG_HOME)
         * unless disable_user_config == TRUE */
        if (g_file_test (*p, G_FILE_TEST_EXISTS))
            file = g_strdup (*p);

        p++;
    }
    
    for (; file == NULL && *p != NULL; ++p)
        file = get_localized_rcfile (*p);

    g_strfreev (paths);

    return file;
}

void
write_backup_file (const char * path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1], c;

    snprintf (bakfile, MAXSTRLEN, "%s.bak", path);

    if (!(bakfp = fopen (bakfile, "w")))
        return;
    
    if (!(fp = fopen (path, "r")))
    {
        fclose (bakfp);
        return;
    }
    
    while ((c = getc (fp)) != EOF)
        putc (c, bakfp);

    fclose (fp);
    fclose (bakfp);
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
#if GTK_CHECK_VERSION(2, 2, 0)
    g_object_set (G_OBJECT (win), "skip_taskbar_hint", TRUE, NULL);
    g_object_set (G_OBJECT (win), "skip_pager_hint", TRUE, NULL);
#else
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
#endif
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
gnome_uri_list_extract_uris (const char * uri_list)
{
    const char *p, *q;
    char *retval;
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
gnome_uri_list_extract_filenames (const char * uri_list)
{
    GList *tmp_list, *node, *result;

    g_return_val_if_fail (uri_list != NULL, NULL);

    result = gnome_uri_list_extract_uris (uri_list);

    tmp_list = result;
    while (tmp_list)
    {
	char *s = (char *) tmp_list->data;

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

static void
real_exec_cmd (const char *cmd, gboolean in_terminal,
	       gboolean use_sn, gboolean silent)
{
    GError *error = NULL;

    if (!xfce_exec (cmd, in_terminal, use_sn, &error))
    {
	if (error)
	{
	    char *msg = g_strcompress (error->message);

	    if (silent)
	    {
		g_warning ("%s", msg);
	    }
	    else
	    {
		xfce_warn ("%s", msg);
	    }

	    g_free (msg);
	    g_error_free (error);
	}
    }
}

/*
 * This is used to get better UI response. Exec the command when the panel
 * becomes idle, so Gtk has time to do the important UI work first.
 */
typedef struct _ActionCommand ActionCommand;
struct _ActionCommand
{
    char *cmd;
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
schedule_exec (const char * cmd, gboolean in_terminal, gboolean use_sn,
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
