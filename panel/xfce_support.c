/*  xfce_support.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfce.h"

/*  Files and directories
 *  ---------------------
*/
char *get_save_dir(void)
{
    const char *home = g_getenv("HOME");

    return g_build_filename(home, HOMERCDIR, NULL);
}

char *get_save_file(const char *name)
{
    const char *home = g_getenv("HOME");

    return g_build_filename(home, HOMERCDIR, name, NULL);
}

char **get_read_dirs(void)
{
    char **dirs;

    if(disable_user_config)
    {
        dirs = g_new0(char *, 2);

        dirs[0] = g_build_filename(SYSCONFDIR, SYSRCDIR, NULL);
    }
    else
    {
        dirs = g_new0(char *, 3);

        dirs[0] = g_build_filename(g_getenv("HOME"), HOMERCDIR, NULL);
        dirs[1] = g_build_filename(SYSCONFDIR, SYSRCDIR, NULL);
    }

    return dirs;
}

static char *get_localized_system_rcfile(const char *name)
{
    const char *locale;
    char base_locale[3];
    char *sysrcfile;

    if(!(locale = g_getenv("LC_MESSAGES")))
        locale = g_getenv("LANG");

    if(locale)
    {
        base_locale[0] = locale[0];
        base_locale[1] = locale[1];
        base_locale[3] = '\0';
    }

    sysrcfile = g_build_filename(SYSCONFDIR, SYSRCDIR, name, NULL);

    if(!locale)
    {
        if(g_file_test(sysrcfile, G_FILE_TEST_EXISTS))
            return sysrcfile;
        else
        {
            g_free(sysrcfile);
            return NULL;
        }
    }
    else
    {
        char *file = g_build_filename(sysrcfile, locale, NULL);

        if(g_file_test(file, G_FILE_TEST_EXISTS))
        {
            g_free(sysrcfile);
            return file;
        }
        else
        {
            g_free(file);
        }

        file = g_build_filename(sysrcfile, base_locale, NULL);

        if(g_file_test(file, G_FILE_TEST_EXISTS))
        {
            g_free(sysrcfile);
            return file;
        }
        else if(g_file_test(sysrcfile, G_FILE_TEST_EXISTS))
        {
            return sysrcfile;
        }
        else
        {
            g_free(sysrcfile);
            return NULL;
        }
    }
}

char *get_read_file(const char *name)
{
    if(!disable_user_config)
    {
        char *dir = get_save_dir();
        char *file = g_build_filename(dir, name, NULL);
        g_free(dir);

        if(g_file_test(file, G_FILE_TEST_EXISTS))
            return file;
        else
            g_free(file);
    }

    /* fall through */
    return get_localized_system_rcfile(name);
}

char **get_plugin_dirs(void)
{
    char **dirs;

    if(disable_user_config)
    {
        dirs = g_new0(char *, 2);

        dirs[0] = g_build_filename(LIBDIR, "plugins", NULL);
    }
    else
    {
        dirs = g_new0(char *, 3);

        dirs[0] =
            g_build_filename(g_getenv("HOME"), HOMERCDIR, "plugins", NULL);
        dirs[1] = g_build_filename(LIBDIR, "plugins", NULL);
    }

    return dirs;
}

char **get_theme_dirs(void)
{
    char **dirs;

    if(disable_user_config)
    {
        dirs = g_new0(char *, 2);

        dirs[0] = g_build_filename(DATADIR, "themes", NULL);
    }
    else
    {
        dirs = g_new0(char *, 3);

        dirs[0] = g_build_filename(g_getenv("HOME"), HOMERCDIR, "themes", NULL);
        dirs[1] = g_build_filename(DATADIR, "themes", NULL);
    }

    return dirs;
}

void write_backup_file(const char *path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1];

    snprintf(bakfile, MAXSTRLEN, "%s.bak", path);

    if((bakfp = fopen(bakfile, "w")) && (fp = fopen(path, "r")))
    {
        char c;

        while((c = fgetc(fp)) != EOF)
            putc(c, bakfp);

        fclose(fp);
        fclose(bakfp);
    }
}

/*  Tooltips
 *  --------
*/
static GtkTooltips *tooltips = NULL;

void add_tooltip(GtkWidget * widget, char *tip)
{
    if(!tooltips)
        tooltips = gtk_tooltips_new();

    gtk_tooltips_set_tip(tooltips, widget, tip, NULL);
}

/*  Button with text and stock icon
 *  -------------------------------
 *  Taken from ROX Filer (http://rox.sourceforge.net)
 *  by Thomas Leonard
*/
GtkWidget *mixed_button_new(const char *stock, const char *message)
{
    GtkWidget *button, *align, *image, *hbox, *label;

    button = gtk_button_new();
    label = gtk_label_new_with_mnemonic(message);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);

    image = gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON);
    hbox = gtk_hbox_new(FALSE, 2);

    align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(button), align);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_widget_show_all(align);

    return button;
}

/*  X atoms and properties
*/
void set_window_type_dock(GtkWidget * window, gboolean set)
{
    /* Copied from ROX Filer by Thomas Leonard */
    /* TODO: Use gdk function when it supports this type */
    GdkAtom window_type;
    gboolean mapped;

    if(set)
	window_type = gdk_atom_intern("_NET_WM_WINDOW_TYPE_DOCK", FALSE);
    else
	window_type = gdk_atom_intern("_NET_WM_WINDOW_TYPE_NORMAL", FALSE);

    if(!GTK_WIDGET_REALIZED(window))
	gtk_widget_realize(window);

    if((mapped = GTK_WIDGET_MAPPED(window)))
	gtk_widget_unmap(window);

    gdk_property_change(window->window,
			gdk_atom_intern("_NET_WM_WINDOW_TYPE", FALSE),
			gdk_atom_intern("ATOM", FALSE), 32,
			GDK_PROP_MODE_REPLACE, (guchar *) & window_type, 1);

    if(!set)
	gdk_property_delete(window->window,
			    gdk_atom_intern("_WIN_LAYER", FALSE));

    if(mapped)
	gtk_widget_map(window);

    if(GTK_IS_WINDOW(toplevel))
    {
	panel_set_position();
	gtk_window_present(GTK_WINDOW(toplevel));
    }
}

void set_window_layer(GtkWidget *win, int layer)
{
    Screen *xscreen;
    Window xid;
    static Atom xa_ABOVE = 0;
    static Atom xa_BELOW = 0;

    if(!GTK_WIDGET_REALIZED(win))
	gtk_widget_realize(win);
    
    xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
    xid = GDK_WINDOW_XID(win->window);
    
    if (!xa_ABOVE)
    {
	xa_ABOVE = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_ABOVE", False);
	xa_BELOW = XInternAtom(GDK_DISPLAY(), "_NET_WM_STATE_BELOW", False);
    }
    
    switch (layer)
    {
	case ABOVE:
	    netk_change_state(xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
	    netk_change_state(xscreen, xid, TRUE, xa_ABOVE, None);
	    break;
	case BELOW:
	    netk_change_state(xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
	    netk_change_state(xscreen, xid, TRUE, xa_BELOW, None);
	    break;
	default:
	    netk_change_state(xscreen, xid, FALSE, xa_ABOVE, xa_BELOW);
    }
}

gboolean check_net_wm_support(void)
{
    static Atom xa_NET_WM_SUPPORT = 0;
    Window xid;

    if (!xa_NET_WM_SUPPORT)
    {
	xa_NET_WM_SUPPORT = XInternAtom(GDK_DISPLAY(), 
					"_NET_SUPPORTING_WM_CHECK", False);
    }

    if (netk_get_window(GDK_ROOT_WINDOW(), xa_NET_WM_SUPPORT, &xid))
    {
/*	if (netk_get_window(xid, xa_NET_WM_SUPPORT, &xid))
	    g_print("ok");
	else
	    g_print("not ok");
*/
	return TRUE;
    }

    return FALSE;
}

void set_window_skip(GtkWidget *win)
{
    Screen *xscreen;
    Window xid;
    static Atom xa_SKIP_PAGER = 0;
    static Atom xa_SKIP_TASKBAR = 0;

    if (!xa_SKIP_PAGER)
    {
	xa_SKIP_PAGER = XInternAtom(GDK_DISPLAY(), 
				    "_NET_WM_STATE_SKIP_PAGER", False);
	xa_SKIP_TASKBAR = XInternAtom(GDK_DISPLAY(), 
				      "_NET_WM_STATE_SKIP_TASKBAR", False);
    }
    
    xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
    xid = GDK_WINDOW_XID(win->window);
    
    netk_change_state(xscreen, xid, TRUE, xa_SKIP_PAGER, xa_SKIP_TASKBAR);
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

static guint n_targets = sizeof(target_table) / sizeof(target_table[0]);

void dnd_set_drag_dest(GtkWidget * widget)
{
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL,
                      target_table, n_targets, GDK_ACTION_COPY);
}

static void dnd_drop_cb(GtkWidget * widget, GdkDragContext * context,
                        gint x, gint y, GtkSelectionData * data,
                        guint info, guint time, gpointer user_data)
{
    GList *fnames;
    DropCallback f;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);

    f = DROP_CALLBACK(g_object_get_data(G_OBJECT(widget), "dropfunction"));

    f(widget, fnames, user_data);

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, FALSE, (context->action == GDK_ACTION_MOVE), time);
}

void dnd_set_callback(GtkWidget * widget, DropCallback function, gpointer data)
{
    g_object_set_data(G_OBJECT(widget), "dropfunction", (gpointer) function);

    g_signal_connect(widget, "drag-data-received",
                     G_CALLBACK(dnd_drop_cb), data);
}

/*** the next three routines are taken straight from gnome-libs so that the
     gtk-only version can receive drag and drops as well ***/
/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void gnome_uri_list_free_strings(GList * list)
{
    g_list_foreach(list, (GFunc) g_free, NULL);
    g_list_free(list);
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList *gnome_uri_list_extract_uris(const gchar * uri_list)
{
    const gchar *p, *q;
    gchar *retval;
    GList *result = NULL;

    g_return_val_if_fail(uri_list != NULL, NULL);

    p = uri_list;

    /* We don't actually try to validate the URI according to RFC
     * 2396, or even check for allowed characters - we just ignore
     * comments and trim whitespace off the ends.  We also
     * allow LF delimination as well as the specified CRLF.
     */
    while(p)
    {
        if(*p != '#')
        {
            while(isspace((int)(*p)))
                p++;

            q = p;
            while(*q && (*q != '\n') && (*q != '\r'))
                q++;

            if(q > p)
            {
                q--;
                while(q > p && isspace((int)(*q)))
                    q--;

                retval = (char *)g_malloc(q - p + 2);
                strncpy(retval, p, q - p + 1);
                retval[q - p + 1] = '\0';

                result = g_list_prepend(result, retval);
            }
        }
        p = strchr(p, '\n');
        if(p)
            p++;
    }

    return g_list_reverse(result);
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
GList *gnome_uri_list_extract_filenames(const gchar * uri_list)
{
    GList *tmp_list, *node, *result;

    g_return_val_if_fail(uri_list != NULL, NULL);

    result = gnome_uri_list_extract_uris(uri_list);

    tmp_list = result;
    while(tmp_list)
    {
        gchar *s = (char *)tmp_list->data;

        node = tmp_list;
        tmp_list = tmp_list->next;

        if(!strncmp(s, "file:", 5))
        {
            /* added by Jasper Huijsmans
               remove leading multiple slashes */
            if(!strncmp(s + 5, "///", 3))
                node->data = g_strdup(s + 7);
            else
                node->data = g_strdup(s + 5);
        }
        else
        {
            node->data = g_strdup(s);
        }
        g_free(s);
    }
    return result;
}

/*  User interaction dialogs
 *  ------------------------
*/
gboolean confirm(const char *text, const char *stock, const char *action)
{
    GtkWidget *dialog, *button;
    int response = GTK_RESPONSE_NONE;

    dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                    text);

    button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_widget_show(button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button, GTK_RESPONSE_NO);

    if(action)
        button = mixed_button_new(stock, action);
    else
        button = gtk_button_new_from_stock(stock);
    gtk_widget_show(button);

    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button, GTK_RESPONSE_YES);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_hide(dialog);
    gtk_widget_destroy(dialog);

    if(response == GTK_RESPONSE_YES)
        return TRUE;
    else
        return FALSE;
}

void report_error(const char *text)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(toplevel ? GTK_WINDOW(toplevel) : NULL,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
                                    text);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_info(const char *text)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, text);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/*  File open dialog
 *  ----------------
*/
static void fs_ok_cb(GtkDialog * fs)
{
    gtk_dialog_response(fs, GTK_RESPONSE_OK);
}


static void fs_cancel_cb(GtkDialog * fs)
{
    gtk_dialog_response(fs, GTK_RESPONSE_CANCEL);
}

/* Any of the arguments may be NULL */
static char *real_select_file(const char *title, const char *path, GtkWidget * parent, gboolean with_preview)
{
    const char *t = (title) ? title : _("Select file");
    GtkWidget *fs;
    char *name = NULL;
    const char *temp;

    fs = preview_file_selection_new(t, with_preview);

    if(path)
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), path);

    if(parent)
        gtk_window_set_transient_for(GTK_WINDOW(fs), GTK_WINDOW(parent));

    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
                             "clicked", G_CALLBACK(fs_ok_cb), fs);

    g_signal_connect_swapped(G_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
                             "clicked", G_CALLBACK(fs_cancel_cb), fs);

    if(gtk_dialog_run(GTK_DIALOG(fs)) == GTK_RESPONSE_OK)
    {
        temp = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));

        if(temp && strlen(temp))
            name = g_strdup(temp);
        else
            name = NULL;
    }

    gtk_widget_destroy(fs);

    return name;
}

char *select_file_name(const char *title, const char *path, GtkWidget * parent)
{
    return real_select_file(title, path, parent, FALSE);
}

char *select_file_with_preview(const char *title, const char *path, GtkWidget * parent)
{
    return real_select_file(title, path, parent, TRUE);
}

/*  Executing commands
 *  ------------------
*/

/* '~' doesn't get expanded by g_spawn_* */
/* this has to be statically allocated for putenv !! */
static char newpath[MAXSTRLEN + 1];

static void expand_path(void)
{
    static gboolean first = TRUE;

    /* we don't have to do this every time */
    if(first)
    {
        const char *path = g_getenv("PATH");
        const char *home = g_getenv("HOME");
        int homelen = strlen(home);
        const char *c;
        char *s;

        if(!path || !strlen(path))
            return;

        c = path;
        s = newpath;

	strcpy(s, "PATH=");
	s+=5;
	
        while(*c)
        {
            if(*c == '~')
            {
                strcpy(s, home);
                s += homelen;
            }
            else
            {
                *s = *c;
                s++;
            }

            c++;
        }

        *s = '\0';
        first = FALSE;

        putenv(newpath);
    }
}

static void real_exec_cmd(const char *cmd, gboolean in_terminal,
                          gboolean silent)
{
    GError *error = NULL;       /* this must be NULL to prevent crash :( */
    char execute[MAXSTRLEN + 1];

    if(!cmd)
        return;

    /* make sure '~' is expanded in the users PATH */
    expand_path();

    if(in_terminal)
        snprintf(execute, MAXSTRLEN, "xterm -e %s", cmd);
    else
        snprintf(execute, MAXSTRLEN, "%s", cmd);

    if(!g_spawn_command_line_async(execute, &error))
    {
        char *msg;

        msg = g_strcompress(error->message);

        if(silent)
        {
            g_printerr("xfce: %s\n", msg);
        }
        else
        {
            report_error(msg);
        }

        g_free(msg);
    }
}

void exec_cmd(const char *cmd, gboolean in_terminal)
{
    real_exec_cmd(cmd, in_terminal, FALSE);
}

/* without error reporting dialog */
void exec_cmd_silent(const char *cmd, gboolean in_terminal)
{
    real_exec_cmd(cmd, in_terminal, TRUE);
}
