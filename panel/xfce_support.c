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

#include <math.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <glib.h>
#include <gmodule.h>
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
G_MODULE_EXPORT /* EXPORT:get_save_dir */
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

G_MODULE_EXPORT /* EXPORT:get_save_file */
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

G_MODULE_EXPORT /* EXPORT:get_read_file */
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

        paths[0] = get_save_file (name);
        
        for (n = 1; d[n] != NULL; ++n)
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

G_MODULE_EXPORT /* EXPORT:write_backup_file */
void
write_backup_file (const char * path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1];
    int c;

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

G_MODULE_EXPORT /* EXPORT:add_tooltip */
void
add_tooltip (GtkWidget * widget, const char *tip)
{
    if (!tooltips)
	tooltips = gtk_tooltips_new ();

    gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
}

G_MODULE_EXPORT /* EXPORT:set_window_layer */
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

G_MODULE_EXPORT /* EXPORT:check_net_wm_support */
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

G_MODULE_EXPORT /* EXPORT:set_window_skip */
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
static GtkTargetEntry target_table[] = {
    {"text/uri-list", 0, 0},
    {"UTF8_STRING", 0, 1},
    {"STRING", 0, 2}
};

static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

G_MODULE_EXPORT /* EXPORT:dnd_set_drag_dest */
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

G_MODULE_EXPORT /* EXPORT:dnd_set_callback */
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
G_MODULE_EXPORT /* EXPORT:gnome_uri_list_free_strings */
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
G_MODULE_EXPORT /* EXPORT:gnome_uri_list_extract_uris */
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
G_MODULE_EXPORT /* EXPORT:gnome_uri_list_extract_filenames */
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
update_preview (XfceFileChooser *chooser, gpointer data)
{
    GtkImage *preview;
    char *filename;
    GdkPixbuf *pb = NULL;
    
    preview = GTK_IMAGE(data);
    filename = xfce_file_chooser_get_filename(chooser);
    
    if(g_file_test(filename, G_FILE_TEST_EXISTS)
       && (pb = gdk_pixbuf_new_from_file (filename, NULL)))
    {
        int w, h;
        
        w = gdk_pixbuf_get_width (pb);
        h = gdk_pixbuf_get_height (pb);

        if (h > 120 || w > 120)
        {
            double wratio, hratio;
            GdkPixbuf *tmp;
            
            wratio = (double)120 / w;
            hratio = (double)120 / h;

            if (hratio < wratio)
            {
                w = rint (hratio * w);
                h = 120;
            }
            else
            {
                w = 120;
                h = rint (wratio * h);
            }

            tmp = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
            g_object_unref (pb);
            pb = tmp;
        }
    }
    
    g_free(filename);
    
    gtk_image_set_from_pixbuf(preview, pb);

    if (pb)
        g_object_unref(pb);
}

/* Any of the arguments may be NULL */
static char *
real_select_file (const char *title, const char *path,
		  GtkWidget * parent, gboolean with_preview)
{
    const char *t = (title) ? title : _("Select file");
    GtkWidget *fs;
    char *name = NULL;

    fs = xfce_file_chooser_new(t, GTK_WINDOW(parent), 
                               XFCE_FILE_CHOOSER_ACTION_OPEN, 
                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
                               GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, 
                               NULL);

    if (path && *path && g_file_test (path, G_FILE_TEST_EXISTS))
    {
        if (!g_path_is_absolute (path))
        {
            char *current, *full;

            current = g_get_current_dir ();
            full = g_build_filename (current, path, NULL);

            xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(fs), full);

            g_free (current);
            g_free (full);
        }
        else
        {
            xfce_file_chooser_set_filename(XFCE_FILE_CHOOSER(fs), path);
        }
    }

    if (with_preview)
    {
        GtkWidget *frame, *preview;
        
        frame = gtk_frame_new (NULL);
        gtk_widget_set_size_request (frame, 130, 130);
        gtk_widget_show (frame);
        
	preview = gtk_image_new();
	gtk_widget_show(preview);
        gtk_container_add (GTK_CONTAINER (frame), preview);
        
	xfce_file_chooser_set_preview_widget(XFCE_FILE_CHOOSER(fs), frame);
	xfce_file_chooser_set_preview_callback(XFCE_FILE_CHOOSER(fs),
			(PreviewUpdateFunc)update_preview, preview);
	xfce_file_chooser_set_use_preview_label (XFCE_FILE_CHOOSER(fs), FALSE);

        if (path)
            update_preview (XFCE_FILE_CHOOSER(fs), preview);
    }
    
    if (gtk_dialog_run (GTK_DIALOG (fs)) == GTK_RESPONSE_ACCEPT)
    {
	name = xfce_file_chooser_get_filename(XFCE_FILE_CHOOSER(fs));
    }

    gtk_widget_destroy (fs);

    return name;
}

G_MODULE_EXPORT /* EXPORT:select_file_name */
char *
select_file_name (const char *title, const char *path, GtkWidget * parent)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    return real_select_file (title, path, parent, FALSE);
}

G_MODULE_EXPORT /* EXPORT:select_file_with_preview */
char *
select_file_with_preview (const char *title, const char *path,
			  GtkWidget * parent)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

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
    char *realcmd;

    realcmd = xfce_expand_variables (cmd, NULL);

    if (!realcmd)
        realcmd = g_strdup (cmd);

    if (!xfce_exec (realcmd, in_terminal, use_sn, &error))
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

    g_free (realcmd);
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

G_MODULE_EXPORT /* EXPORT:exec_cmd */
void
exec_cmd (const char *cmd, gboolean in_terminal, gboolean use_sn)
{
    g_return_if_fail (cmd != NULL);
    schedule_exec (cmd, in_terminal, use_sn, FALSE);
}

/* without error reporting dialog */
G_MODULE_EXPORT /* EXPORT:exec_cmd_silent */
void
exec_cmd_silent (const char *cmd, gboolean in_terminal, gboolean use_sn)
{
    g_return_if_fail (cmd != NULL);
    schedule_exec (cmd, in_terminal, use_sn, TRUE);
}
