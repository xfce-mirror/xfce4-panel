/*  xfce_support.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#include "global.h"
#include "xfce_support.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Files and directories
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#ifndef SYSCONFDIR
#define SYSCONFDIR "/usr/local/etc/"
#endif

#ifndef DATADIR
#define DATADIR "/usr/local/share/xfce4"
#endif

#define HOMERCDIR ".xfce4"
#define SYSRCDIR "xfce4"
#define XFCERC "xfce4rc"

#define PLUGINDIR "plugins"
#define THEMEDIR "themes"

char *get_save_dir(void)
{
    const char *home = g_getenv("HOME");

    return g_build_filename(home, HOMERCDIR, NULL);
}

char *get_save_file(void)
{
    const char *home = g_getenv("HOME");

    return g_build_filename(home, HOMERCDIR, XFCERC, NULL);
}

char *get_read_dir(void)
{
    if(disable_user_config)
    {
        return g_strdup(SYSCONFDIR);
    }
    else
    {
        /* same as save dir */
        return get_save_dir();
    }
}

static char *get_localized_system_rcfile(void)
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

    sysrcfile = g_build_filename(SYSCONFDIR, SYSRCDIR, RCFILE, NULL);

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

char *get_read_file(void)
{
    if(!disable_user_config)
    {
        /* same as save file */
        char file = get_save_file();

        if(g_file_test(file, G_FILE_TEST_EXISTS))
            return file;
        else
            g_free(file);
    }

    /* fall through */
    return get_localized_system_rcfile();
}

char **get_plugin_dirs(void)
{
    char **dirs;

    if(disable_user_config)
    {
        dirs = g_new0(char *, 2);

        dirs[0] = g_build_filename(DATADIR, "plugins", NULL);
    }
    else
    {
        dirs = g_new0(char *, 3);

        dirs[0] = g_build_filename(g_getenv("HOME"), HOMERCDIR, "themes", NULL);
        dirs[1] = g_build_filename(DATADIR, "plugins", NULL);

        return dirs;
    }
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

        return dirs;
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Gtk
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* Taken from ROX Filer (http://rox.sourceforge.net)
 * by Thomas Leonard
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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   DND
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Dialogs
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
void report_error(const char *text)
{
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE, text);

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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void fs_ok_cb(GtkDialog * fs)
{
    gtk_dialog_response(fs, GTK_RESPONSE_OK);
}


static void fs_cancel_cb(GtkDialog * fs)
{
    gtk_dialog_response(fs, GTK_RESPONSE_CANCEL);
}

/* Let's user select a file in a from a dialog. Any of the arguments may be NULL */
char *select_file_name(const char *title, const char *path, GtkWidget * parent)
{
    const char *t = (title) ? title : _("Select file");
    GtkWidget *fs = gtk_file_selection_new(t);
    char *name = NULL;
    const char *temp;

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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Executing commands
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* '~' doesn't get expanded by g_spawn_* */
static void *expand_path(void)
{
    static gboolean first = TRUE;

    /* we don't have to do this every time */
    if(first)
    {
        const char *path = g_getenv("PATH");
        const char *home = g_getenv("HOME");
        int homelen = strlen(home);
        char *c, *s;
        char newpath[MAXSTRLEN + 1];


        if(!path || !strlen(path))
            return NULL;

        c = path;
        s = newpath;

        while(*c)
        {
            if(*c == '~')
            {
                sprintf(s, home);
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

        setenv("PATH", newpath, 1);
    }
}

void exec_cmd(const char *cmd, gboolean in_terminal)
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

        report_error(msg);

        g_free(msg);
    }
}
