/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *  Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfcegui4/xfce-titled-dialog.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "launcher.h"
#include "launcher-dialog.h"


enum
{
    COLUMN_ICON = 0,
    COLUMN_NAME
};

typedef struct _LauncherDialog LauncherDialog;

struct _LauncherDialog
{
    LauncherPlugin *launcher;

    /* stored setting */
    guint           stored_move_first : 1;

    /* arrow position */
    GtkWidget      *arrow_position;

    /* entries list */
    GtkWidget      *treeview;
    GtkListStore   *store;

    /* tree buttons */
    GtkWidget      *up;
    GtkWidget      *down;
    GtkWidget      *add;
    GtkWidget      *remove;

    /* lock */
    guint           updating : 1;

    /* active entry */
    LauncherEntry  *entry;

    /* entry widgets */
    GtkWidget      *entry_name;
    GtkWidget      *entry_comment;
    GtkWidget      *entry_icon;
    GtkWidget      *entry_exec;
    GtkWidget      *entry_path;
    GtkWidget      *entry_terminal;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    GtkWidget      *entry_startup;
#endif
};



/**
 * Prototypes
 **/
static void        launcher_dialog_g_list_swap               (GList                 *li_a,
                                                              GList                 *li_b);
static gboolean    launcher_dialog_read_desktop_file         (const gchar           *file,
                                                              LauncherEntry         *entry);
static void        launcher_dialog_tree_drag_data_received   (GtkWidget             *widget,
                                                              GdkDragContext        *context,
                                                              gint                   x,
                                                              gint                   y,
                                                              GtkSelectionData      *selection_data,
                                                              guint                  info,
                                                              guint                  time,
                                                              LauncherDialog        *ld);
static void        launcher_dialog_frame_drag_data_received  (GtkWidget             *widget,
                                                              GdkDragContext        *context,
                                                              gint                   x,
                                                              gint                   y,
                                                              GtkSelectionData      *selection_data,
                                                              guint                  info,
                                                              guint                  time,
                                                              LauncherDialog        *ld);
static void        launcher_dialog_save_entry                (GtkWidget             *entry,
                                                               LauncherDialog        *ld);
static void        launcher_dialog_save_button               (GtkWidget             *button,
                                                              LauncherDialog        *ld);
static void        launcher_dialog_update_entries            (LauncherDialog        *ld);
static void        launcher_dialog_update_icon               (LauncherDialog        *ld);
static void        launcher_dialog_folder_chooser            (LauncherDialog        *ld);
static void        launcher_dialog_command_chooser           (LauncherDialog        *ld);
static void        launcher_dialog_icon_chooser              (LauncherDialog        *ld);
static void        launcher_dialog_tree_update_row           (LauncherDialog        *ld,
                                                              gint                   column);
static void        launcher_dialog_tree_selection_changed    (LauncherDialog        *ld,
                                                              GtkTreeSelection      *selection);
static void        launcher_dialog_tree_button_clicked       (GtkWidget             *button,
                                                              LauncherDialog        *ld);
static GtkWidget  *launcher_dialog_add_properties            (LauncherDialog        *ld) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static GtkWidget  *launcher_dialog_add_tree                  (LauncherDialog        *ld) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static GtkWidget  *launcher_dialog_add_tree_buttons          (LauncherDialog        *ld) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static void        launcher_dialog_response                  (GtkWidget             *dialog,
                                                              gint                   response,
                                                              LauncherDialog        *ld);



/**
 * .Desktop entry
 **/
static void
launcher_dialog_g_list_swap (GList *li_a,
                             GList *li_b)
{
    gpointer data;

    /* swap the data pointers */
    data = li_a->data;
    li_a->data = li_b->data;
    li_b->data = data;
}


static gboolean
launcher_dialog_read_desktop_file (const gchar   *path,
                                   LauncherEntry *entry)
{
    XfceRc      *rc = NULL;
    const gchar *value = NULL;
    const gchar *p;
    gchar       *tmp;

    /* we only support .desktop files */
    if (G_UNLIKELY (g_str_has_suffix (path, ".desktop") == FALSE ||
                    g_path_is_absolute (path) == FALSE))
        return FALSE;

    /* open de .desktop file */
    rc = xfce_rc_simple_open (path, TRUE);
    if (G_UNLIKELY (rc == NULL))
        return FALSE;

    /* set the desktop entry group */
    xfce_rc_set_group (rc, "Desktop Entry");

    /* name */
    value = xfce_rc_read_entry (rc, "Name", NULL);
    if (G_LIKELY (value != NULL))
    {
        g_free (entry->name);
        entry->name = g_strdup (value);
    }

    /* comment */
    value = xfce_rc_read_entry (rc, "Comment", NULL);
    if (G_LIKELY (value != NULL))
    {
        g_free (entry->comment);
        entry->comment = g_strdup (value);
    }

    /* icon */
    value = xfce_rc_read_entry_untranslated (rc, "Icon", NULL);
    if (G_LIKELY (value != NULL))
    {
        g_free (entry->icon);

        /* get rid of extensions in non-absolute names */
        if (G_UNLIKELY (g_path_is_absolute (value) == FALSE) &&
            ((p = g_strrstr (value, ".")) && strlen (p) < 6))
            entry->icon = g_strndup (value, p-value);
        else
            entry->icon = g_strdup (value);
            
#if LAUNCHER_NEW_TOOLTIP_API
        /* release the cached icon */
        if (entry->tooltip_cache)
        {
            g_object_unref (G_OBJECT (entry->tooltip_cache));
            entry->tooltip_cache = NULL;
        }
#endif
    }

    /* exec */
    value = xfce_rc_read_entry_untranslated (rc, "Exec", NULL);
    if (G_LIKELY (value != NULL))
    {
        g_free (entry->exec);

        /* expand and quote command and store */
        tmp = xfce_expand_variables (value, NULL);
        entry->exec = g_shell_quote (tmp);
        g_free (tmp);
    }

    /* working directory */
    value = xfce_rc_read_entry_untranslated (rc, "Path", NULL);
    if (G_UNLIKELY (value != NULL))
    {
        g_free (entry->path);

        /* expand variables and store */
        entry->path = xfce_expand_variables (value, NULL);
    }

    /* terminal */
    entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    /* startup notification */
    entry->startup = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);
#endif

    /* release rc file */
    xfce_rc_close (rc);

    return TRUE;
}



static void
launcher_dialog_tree_drag_data_received (GtkWidget        *widget,
                                         GdkDragContext   *context,
                                         gint              x,
                                         gint              y,
                                         GtkSelectionData *selection_data,
                                         guint             info,
                                         guint             time,
                                         LauncherDialog   *ld)
{
    GtkTreePath             *path = NULL;
    GtkTreeViewDropPosition  position;
    GtkTreeModel            *model;
    GtkTreeIter              iter_a;
    GtkTreeIter              iter_b;
    GSList                  *filenames = NULL;
    GSList                  *li;
    const gchar             *file;
    gboolean                 insert_before = FALSE;
    gboolean                 update_icon = FALSE;
    gint                     i = 0;
    LauncherEntry           *entry;
    GdkPixbuf               *pixbuf;

    /* get drop position in the tree */
    if (gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (ld->treeview),
                                           x, y, &path, &position) == FALSE)
    {
        /* probably droped in empty tree space, drop after last item */
        path = gtk_tree_path_new_from_indices (g_list_length (ld->launcher->entries) -1 , -1);
        position = GTK_TREE_VIEW_DROP_AFTER;
    }

    if (G_LIKELY (path != NULL))
    {
        /* get the iter we're going to drop after */
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (ld->treeview));
        gtk_tree_model_get_iter (model,  &iter_a, path);

        /* array position or current item */
        i = gtk_tree_path_get_indices (path)[0];

        /* insert position, array correction and the path we select afterwards */
        switch (position)
        {
            case GTK_TREE_VIEW_DROP_BEFORE:
            case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
                insert_before = TRUE;
                break;

            case GTK_TREE_VIEW_DROP_AFTER:
            case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
                gtk_tree_path_next (path);
                ++i;
                insert_before = FALSE;
                break;
         }

        /* we need to update the button icon afterwards */
        if (i == 0)
            update_icon = TRUE;

        /* create list from selection data */
        filenames = launcher_utility_filenames_from_selection_data (selection_data);
    }

    if (G_LIKELY (filenames != NULL))
    {
        for (li = filenames; li != NULL; li = li->next)
        {
            file = li->data;

            /* create new entry */
            entry = launcher_entry_new ();

            /* try to parse desktop file */
            if (G_LIKELY (launcher_dialog_read_desktop_file (file, entry) == TRUE))
            {
                /* insert new row in store */
                if (insert_before)
                    gtk_list_store_insert_before (ld->store, &iter_b, &iter_a);
                else
                    gtk_list_store_insert_after (ld->store, &iter_b, &iter_a);

                /* try to load the pixbuf */
                pixbuf = launcher_utility_load_pixbuf (gtk_widget_get_screen (ld->treeview), entry->icon, LAUNCHER_TREE_ICON_SIZE);

                /* set tree data */
                gtk_list_store_set (ld->store, &iter_b,
                                    COLUMN_ICON, pixbuf,
                                    COLUMN_NAME, entry->name,
                                    -1);

                /* release pixbuf */
                if (G_LIKELY (pixbuf != NULL))
                    g_object_unref (G_OBJECT (pixbuf));

                /* insert in list */
                ld->launcher->entries = g_list_insert (ld->launcher->entries, entry, i);

                /* copy iter, so we add after last item */
                iter_a = iter_b;

                /* raise position counter */
                ++i;

                /* 1st item is inserted before existing item, after
                 * that we insert after the 1st item */
                insert_before = FALSE;
            }
            else
            {
                /* desktop file pasring failed, free new entry */
                launcher_entry_free (entry, NULL);
            }
        }

        /* select the new item (also updates treeview buttons) */
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->treeview), path, NULL, FALSE);

        /* update the panel */
        launcher_plugin_rebuild (ld->launcher, update_icon);

        /* cleanup */
        launcher_free_filenames (filenames);
    }

    /* free path */
    if (G_LIKELY (path != NULL))
        gtk_tree_path_free (path);

    /* finish drag */
    gtk_drag_finish (context, TRUE, FALSE, time);
}



static void
launcher_dialog_frame_drag_data_received (GtkWidget        *widget,
                                          GdkDragContext   *context,
                                          gint              x,
                                          gint              y,
                                          GtkSelectionData *selection_data,
                                          guint             info,
                                          guint             time,
                                          LauncherDialog   *ld)
{
    GSList   *filenames, *li;
    gchar    *file;
    gboolean  update_icon = FALSE;

    /* create list from all the uri list */
    filenames = launcher_utility_filenames_from_selection_data (selection_data);

    if (G_LIKELY (filenames != NULL))
    {
        for (li = filenames; li != NULL; li = li->next)
        {
            file = (gchar *) li->data;

            /* try to update the current entry settings */
            if (G_LIKELY (launcher_dialog_read_desktop_file (file, ld->entry) == TRUE))
            {
                /* update the widgets */
                launcher_dialog_update_entries (ld);

                /* update the tree */
                launcher_dialog_tree_update_row (ld, COLUMN_NAME);
                launcher_dialog_tree_update_row (ld, COLUMN_ICON);

                /* also update the panel button icon */
                if (g_list_index (ld->launcher->entries, ld->entry) == 0)
                    update_icon = TRUE;

                /* update the panel */
                launcher_plugin_rebuild (ld->launcher, update_icon);

                /* stop trying */
                break;
            }
        }

        /* cleanup */
        launcher_free_filenames (filenames);
    }

    /* finish drag */
    gtk_drag_finish (context, TRUE, FALSE, time);
}



/**
 * Properties update and save functions
 **/
static void
launcher_dialog_save_entry (GtkWidget      *entry,
                            LauncherDialog *ld)
{
    const gchar *text;

    /* quit if locked or no active entry set */
    if (G_UNLIKELY (ld->updating == TRUE || ld->entry == NULL))
        return;

    /* get entry text */
    text = gtk_entry_get_text (GTK_ENTRY (entry));

    /* set text to null, if there is no valid text */
    if (G_UNLIKELY (text == NULL || *text == '\0'))
        text = NULL;

    /* save new value */
    if (entry == ld->entry_name)
    {
        g_free (ld->entry->name);
        ld->entry->name = g_strdup (text);

        /* update tree, when triggered by widget */
        launcher_dialog_tree_update_row (ld, COLUMN_NAME);
    }
    else if (entry == ld->entry_comment)
    {
        g_free (ld->entry->comment);
        ld->entry->comment = g_strdup (text);
    }
    else if (entry == ld->entry_exec)
    {
        g_free (ld->entry->exec);
        ld->entry->exec = text ? xfce_expand_variables (text, NULL) : NULL;
    }
    else if (entry == ld->entry_path)
    {
        g_free (ld->entry->path);
        ld->entry->path = text ? xfce_expand_variables (text, NULL) : NULL;
    }

    /* update panel */
    launcher_plugin_rebuild (ld->launcher, FALSE);
}



static void
launcher_dialog_save_button (GtkWidget      *button,
                             LauncherDialog *ld)
{
    gboolean active;

    /* quit if locked or no active entry set */
    if (G_UNLIKELY (ld->updating == TRUE || ld->entry == NULL))
        return;

    /* get toggle button state */
    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    /* update entry or global setting */
    if (button == ld->entry_terminal)
    {
        ld->entry->terminal = active;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
        gtk_widget_set_sensitive (ld->entry_startup, !active);
#endif
    }
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    else if (button == ld->entry_startup)
        ld->entry->startup = active;
#endif
}



static void
launcher_dialog_update_entries (LauncherDialog *ld)
{
    /* quit if locked or no active entry set */
    if (G_UNLIKELY (ld->updating == TRUE || ld->entry == NULL))
        return;

    /* lock the save functions */
    ld->updating = TRUE;

    /* set new entry values */
    gtk_entry_set_text (GTK_ENTRY (ld->entry_name),
                        (ld->entry->name != NULL) ? ld->entry->name : "");

    gtk_entry_set_text (GTK_ENTRY (ld->entry_comment),
                        (ld->entry->comment != NULL) ? ld->entry->comment : "");

    gtk_entry_set_text (GTK_ENTRY (ld->entry_exec),
                        (ld->entry->exec != NULL) ? ld->entry->exec : "");

    gtk_entry_set_text (GTK_ENTRY (ld->entry_path),
                        (ld->entry->path != NULL) ? ld->entry->path : "");

    /* set toggle buttons */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ld->entry_terminal),
                                  ld->entry->terminal);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ld->entry_startup),
                                  ld->entry->startup);

    gtk_widget_set_sensitive (ld->entry_startup, !ld->entry->terminal);
#endif

    /* update icon button */
    launcher_dialog_update_icon (ld);

    /* unlock */
    ld->updating = FALSE;
}



static void
launcher_dialog_update_icon (LauncherDialog *ld)
{
    GdkPixbuf *icon = NULL;
    GtkWidget *child;

    /* drop the previous button child */
    if (GTK_BIN (ld->entry_icon)->child != NULL)
        gtk_widget_destroy (GTK_BIN (ld->entry_icon)->child);

    if (G_LIKELY (ld->entry->icon))
        icon = launcher_utility_load_pixbuf (gtk_widget_get_screen (ld->entry_icon), ld->entry->icon, LAUNCHER_CHOOSER_ICON_SIZE);

    /* create icon button */
    if (G_LIKELY (icon != NULL))
    {
        /* create image from pixbuf */
        child = gtk_image_new_from_pixbuf (icon);

        /* release icon */
        g_object_unref (G_OBJECT (icon));

        gtk_widget_set_size_request (child, LAUNCHER_CHOOSER_ICON_SIZE, LAUNCHER_CHOOSER_ICON_SIZE);
    }
    else
    {
        child = gtk_label_new (_("No icon"));

        gtk_widget_set_size_request (child, -1, LAUNCHER_CHOOSER_ICON_SIZE);
    }

    gtk_container_add (GTK_CONTAINER (ld->entry_icon), child);
    gtk_widget_show (child);
}



/**
 * Icon and command search dialogs
 **/
static void
launcher_dialog_folder_chooser (LauncherDialog *ld)
{
    GtkWidget *chooser;
    gchar     *path;

    chooser = gtk_file_chooser_dialog_new (_("Select a Directory"),
                                           GTK_WINDOW (gtk_widget_get_toplevel (ld->treeview)),
                                           GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                           NULL);

    /* only here */
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);

    /* use the bindir as default folder */
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), BINDIR);

    /* select folder from field */
    if (G_LIKELY (ld->entry->path != NULL))
    {
        if (G_LIKELY (g_path_is_absolute (ld->entry->path)))
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), ld->entry->path);
    }

    /* run the chooser dialog */
    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
        path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

        /* set the new entry text */
        gtk_entry_set_text (GTK_ENTRY (ld->entry_path), path);

        /* cleanup */
        g_free (path);
    }

    /* destroy dialog */
    gtk_widget_destroy (chooser);
}

static void
launcher_dialog_command_chooser (LauncherDialog *ld)
{
    GtkFileFilter *filter;
    GtkWidget     *chooser;
    gchar         *filename;
    gchar         *s;

    chooser = gtk_file_chooser_dialog_new (_("Select an Application"),
                                           NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                           NULL);
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);

    /* add file chooser filters */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Executable Files"));
    gtk_file_filter_add_mime_type (filter, "application/x-csh");
    gtk_file_filter_add_mime_type (filter, "application/x-executable");
    gtk_file_filter_add_mime_type (filter, "application/x-perl");
    gtk_file_filter_add_mime_type (filter, "application/x-python");
    gtk_file_filter_add_mime_type (filter, "application/x-ruby");
    gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
    gtk_file_filter_add_pattern (filter, "*.pl");
    gtk_file_filter_add_pattern (filter, "*.py");
    gtk_file_filter_add_pattern (filter, "*.rb");
    gtk_file_filter_add_pattern (filter, "*.sh");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Perl Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-perl");
    gtk_file_filter_add_pattern (filter, "*.pl");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Python Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-python");
    gtk_file_filter_add_pattern (filter, "*.py");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Ruby Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-ruby");
    gtk_file_filter_add_pattern (filter, "*.rb");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Shell Scripts"));
    gtk_file_filter_add_mime_type (filter, "application/x-csh");
    gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
    gtk_file_filter_add_pattern (filter, "*.sh");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

    /* use the bindir as default folder */
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), BINDIR);

    /* get the current command */
    filename = gtk_editable_get_chars (GTK_EDITABLE (ld->entry_exec), 0, -1);
    if (G_LIKELY (filename != NULL))
    {
        /* use only the first argument */
        s = strchr (filename, ' ');
        if (G_UNLIKELY (s != NULL))
            *s = '\0';

        /* check if we have a file name */
        if (G_LIKELY (*filename != '\0'))
        {
            /* check if the filename is not an absolute path */
            if (G_LIKELY (!g_path_is_absolute (filename)))
            {
                /* try to lookup the filename in $PATH */
                s = g_find_program_in_path (filename);
                if (G_LIKELY (s != NULL))
                {
                    /* use the absolute path instead */
                    g_free (filename);
                    filename = s;
                }
            }

            /* check if we have an absolute path now */
            if (G_LIKELY (g_path_is_absolute (filename)))
                gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);
        }

        /* release the filename */
        g_free (filename);
    }

    /* run the chooser dialog */
    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
        /* get the filename and quote it */
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        s = g_shell_quote (filename);
        g_free (filename);
        filename = s;

        /* set the new entry text */
        gtk_entry_set_text (GTK_ENTRY (ld->entry_exec), filename);

        /* cleanup */
        g_free (filename);
    }

    /* destroy dialog */
    gtk_widget_destroy (chooser);
}



static void
launcher_dialog_icon_chooser (LauncherDialog *ld)
{
    const gchar *name;
    GtkWidget   *chooser;
    gchar       *title;
    gboolean     update_icon = FALSE;

    /* determine the name of the entry being edited */
    name = gtk_entry_get_text (GTK_ENTRY (ld->entry_name));
    if (G_UNLIKELY (name == NULL || *name == '\0'))
        name = _("Unknown");

    /* allocate the chooser dialog */
    title = g_strdup_printf (_("Select an Icon for \"%s\""), name);
    chooser = exo_icon_chooser_dialog_new (title, NULL,
                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                           NULL);
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT, GTK_RESPONSE_CANCEL, -1);
    gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);
    g_free (title);

    /* set the current icon, if there is any */
    if (G_LIKELY (ld->entry->icon))
        exo_icon_chooser_dialog_set_icon (EXO_ICON_CHOOSER_DIALOG (chooser), ld->entry->icon);

    /* run the icon chooser dialog */
    if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
        /* free the old icon name */
        if (G_LIKELY (ld->entry->icon))
            g_free (ld->entry->icon);

        /* set new icon */
        ld->entry->icon = exo_icon_chooser_dialog_get_icon (EXO_ICON_CHOOSER_DIALOG (chooser));

#if LAUNCHER_NEW_TOOLTIP_API
        /* release cached tooltip icon */
        if (ld->entry->tooltip_cache)
        {
            g_object_unref (G_OBJECT (ld->entry->tooltip_cache));
            ld->entry->tooltip_cache = NULL;
        }
#endif

        /* update the icon button */
        launcher_dialog_update_icon (ld);

        /* update the icon column in the tree */
        launcher_dialog_tree_update_row (ld, COLUMN_ICON);

        /* check if we need to update the icon button image */
        if (g_list_index (ld->launcher->entries, ld->entry) == 0)
            update_icon = TRUE;

        /* update the panel widgets */
        launcher_plugin_rebuild (ld->launcher, update_icon);
    }

    /* destroy the chooser */
    gtk_widget_destroy (chooser);
}



/**
 * Tree functions
 **/
static void
launcher_dialog_tree_update_row (LauncherDialog *ld,
                                 gint            column)
{
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
    GdkPixbuf        *icon = NULL;
    const gchar      *name;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->treeview));

    if (G_LIKELY (gtk_tree_selection_get_selected (selection, NULL, &iter)))
    {
        switch (column)
        {
            case COLUMN_ICON:
                /* load entry icon */
                icon = launcher_utility_load_pixbuf (gtk_widget_get_screen (ld->treeview), ld->entry->icon, LAUNCHER_TREE_ICON_SIZE);

                /* set new icon */
                gtk_list_store_set (ld->store, &iter,
                                    COLUMN_ICON, icon,
                                    -1);

                /* release icon */
                if (G_LIKELY (icon != NULL))
                    g_object_unref (G_OBJECT (icon));

                break;

            case COLUMN_NAME:
                /* build name */
                name = ld->entry->name ? ld->entry->name : _("Unnamed");
            
                /* set new name */
                gtk_list_store_set (ld->store, &iter,
                                    COLUMN_NAME, name,
                                    -1);

                break;
        }
    }
}



static void
launcher_dialog_tree_selection_changed (LauncherDialog   *ld,
                                        GtkTreeSelection *selection)
{
    GtkTreeModel *model;
    GtkTreePath  *path;
    GtkTreeIter   iter;
    gboolean      selected;
    gint          position = 0;
    gint          items;

    if (G_UNLIKELY (ld->updating == TRUE))
        return;

    g_return_if_fail (GTK_IS_TREE_SELECTION (selection));

    /* check if we have currently selected an item */
    selected = gtk_tree_selection_get_selected (selection, &model, &iter);

    if (G_LIKELY (selected))
    {
        /* determine the path for the selected iter */
        path = gtk_tree_model_get_path (model, &iter);

        /* get position */
        position = gtk_tree_path_get_indices (path)[0];

        /* set new active entry */
        ld->entry = (LauncherEntry *) g_list_nth (ld->launcher->entries, position)->data;

        /* update fields */
        launcher_dialog_update_entries (ld);

        /* scroll new item to center of window */
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (ld->treeview), path, NULL,
                                      TRUE, 0.5, 0.0);

        /* release path */
        gtk_tree_path_free (path);
    }

    /* items in the list */
    items = gtk_tree_model_iter_n_children (model, NULL);

    /* change sensitivity of buttons */
    gtk_widget_set_sensitive (ld->up, selected && (position > 0));
    gtk_widget_set_sensitive (ld->down, selected && (position < items - 1));
    gtk_widget_set_sensitive (ld->remove, selected && (items > 1));
}



static void
launcher_dialog_tree_button_clicked (GtkWidget      *button,
                                     LauncherDialog *ld)
{
    GtkTreeSelection *selection;
    GtkTreeModel     *model;
    GtkTreePath      *path;
    GtkTreeIter       iter_a;
    GtkTreeIter       iter_b;
    guint             position;
    gint              list_length;
    GList            *li;
    GdkPixbuf        *icon = NULL;
    LauncherEntry    *entry;
    gboolean          update_icon = FALSE;

    /* get the selected items in the treeview */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->treeview));

    /* get selected iter, quit if no iter found */
    if (G_UNLIKELY (gtk_tree_selection_get_selected (selection, &model, &iter_a) == FALSE))
        return;

    /* run the requested button action */
    if (button == ld->up)
    {
        /* get path */
        path = gtk_tree_model_get_path (model, &iter_a);

        /* position of the item in the list */
        position = gtk_tree_path_get_indices (path)[0];

        /* check if we need to update the icon button image */
        if (position == 1)
            update_icon = TRUE;

        /* get previous path */
        if (G_LIKELY (gtk_tree_path_prev (path)))
        {
            /* get iter for previous item */
            gtk_tree_model_get_iter (model, &iter_b, path);

            /* swap the entries */
            gtk_list_store_swap (ld->store, &iter_a, &iter_b);

            /* swap items in the list */
            li = g_list_nth (ld->launcher->entries, position);
            launcher_dialog_g_list_swap (li, li->prev);
        }

        /* release the path */
        gtk_tree_path_free (path);

        /* update tree view */
        launcher_dialog_tree_selection_changed (ld, selection);
    }
    else if (button == ld->down)
    {
        /* get path of selected item */
        path = gtk_tree_model_get_path (model, &iter_a);

        /* get position of item we're going to move */
        position = gtk_tree_path_get_indices (path)[0];

        /* check if we need to update the icon button image*/
        if (position == 0)
            update_icon = TRUE;

        /* get next item in the list */
        gtk_tree_path_next (path);

        /* get next iter */
        if (G_LIKELY (gtk_tree_model_get_iter (model, &iter_b, path)))
        {
            /* swap the entries */
            gtk_list_store_swap (ld->store, &iter_a, &iter_b);

            /* swap items in the list */
            li = g_list_nth (ld->launcher->entries, position);
            launcher_dialog_g_list_swap (li, li->next);
        }

        /* release the path */
        gtk_tree_path_free (path);

        /* update tree view */
        launcher_dialog_tree_selection_changed (ld, selection);
    }
    else if (button == ld->add)
    {
        /* create new entry */
        entry = launcher_entry_new ();

        /* load new launcher icon */
        icon = launcher_utility_load_pixbuf (gtk_widget_get_screen (ld->treeview), entry->icon, LAUNCHER_TREE_ICON_SIZE);

        /* append new entry */
        gtk_list_store_insert_after (ld->store, &iter_b, &iter_a);
        gtk_list_store_set (ld->store, &iter_b,
                            COLUMN_ICON, icon,
                            COLUMN_NAME, entry->name,
                            -1);

        /* release the pixbuf */
        if (G_LIKELY (icon != NULL))
            g_object_unref (G_OBJECT (icon));

        /* get path of new item */
        path = gtk_tree_model_get_path (model, &iter_b);

        /* position in the list */
        position = gtk_tree_path_get_indices (path)[0];

        /* insert in list */
        ld->launcher->entries = g_list_insert (ld->launcher->entries,
                                               entry, position);

        /* select the new item (also updates treeview buttons) */
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->treeview), path, NULL, FALSE);

        /* cleanup */
        gtk_tree_path_free (path);

        /* allow to set the arrow position */
        gtk_widget_set_sensitive (ld->arrow_position, TRUE);

    }
    else if (button == ld->remove)
    {
        /* path from row to remove */
        path = gtk_tree_model_get_path (model, &iter_a);

        /* get position of the item to remove */
        position = gtk_tree_path_get_indices (path)[0];

        /* check if we need to update the icon button image*/
        if (position == 0)
            update_icon = TRUE;

        /* lock */
        ld->updating = TRUE;

        /* remove active entry */
        launcher_entry_free (ld->entry, ld->launcher);
        ld->entry = NULL;

        /* remove row from store */
        gtk_list_store_remove (ld->store, &iter_a);

        /* unlock */
        ld->updating = FALSE;

        /* list length */
        list_length = g_list_length (ld->launcher->entries);

        /* select previous item, if last item was removed */
        if (position >= (guint) list_length)
            gtk_tree_path_prev (path);

        /* select the new item (also updates treeview buttons) */
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->treeview), path, NULL, FALSE);

        /* cleanup */
        gtk_tree_path_free (path);

        /* allow to set the arrow position */
        gtk_widget_set_sensitive (ld->arrow_position, list_length > 1);

        /* don't allow menu arrows */
        if (list_length == 1 && ld->launcher->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
            gtk_combo_box_set_active (GTK_COMBO_BOX (ld->arrow_position), LAUNCHER_ARROW_DEFAULT);
    }

    /* update panel */
    launcher_plugin_rebuild (ld->launcher, update_icon);
}



static void
launcher_dialog_arrow_position_changed (GtkComboBox    *combo,
                                        LauncherDialog *ld)
{
    ld->launcher->arrow_position = gtk_combo_box_get_active (combo);

    launcher_plugin_rebuild (ld->launcher, TRUE);
}



/**
 * Launcher dialog widgets
 **/
static GtkWidget *
launcher_dialog_add_properties (LauncherDialog *ld)
{
    GtkWidget    *frame, *vbox, *hbox;
    GtkWidget    *label, *button, *image;
    GtkSizeGroup *sg;

    frame = gtk_frame_new (NULL);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* entry name field */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Name"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->entry_name = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_name, TRUE, TRUE, 0);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), ld->entry_name);

    g_signal_connect (G_OBJECT (ld->entry_name), "changed",
                      G_CALLBACK (launcher_dialog_save_entry), ld);

    /* entry comment field */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Description"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->entry_comment = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_comment, TRUE, TRUE, 0);
    gtk_widget_set_size_request (ld->entry_comment, 300, -1);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), ld->entry_comment);

    g_signal_connect (G_OBJECT (ld->entry_comment), "changed",
                      G_CALLBACK (launcher_dialog_save_entry), ld);

    /* entry icon chooser button */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Icon"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->entry_icon = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_icon, FALSE, FALSE, 0);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), ld->entry_icon);

    g_signal_connect_swapped (G_OBJECT (ld->entry_icon), "clicked",
                              G_CALLBACK (launcher_dialog_icon_chooser), ld);

    /* entry command field and button */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("Co_mmand"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->entry_exec = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_exec, TRUE, TRUE, 0);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), ld->entry_exec);

    g_signal_connect (G_OBJECT (ld->entry_exec), "changed",
                      G_CALLBACK (launcher_dialog_save_entry), ld);

    button = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                              G_CALLBACK (launcher_dialog_command_chooser), ld);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), image);

    /* working directory field */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("_Working Directory"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->entry_path = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_path, TRUE, TRUE, 0);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), ld->entry_path);

    g_signal_connect (G_OBJECT (ld->entry_path), "changed",
                      G_CALLBACK (launcher_dialog_save_entry), ld);

    button = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    g_signal_connect_swapped (G_OBJECT (button), "clicked",
                              G_CALLBACK (launcher_dialog_folder_chooser), ld);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
    gtk_container_add (GTK_CONTAINER (button), image);

    /* entry terminal toggle button with spacer */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_alignment_new (0, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ld->entry_terminal = gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_terminal, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT (ld->entry_terminal), "toggled",
                      G_CALLBACK (launcher_dialog_save_button), ld);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    /* startup notification */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_alignment_new (0, 0, 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ld->entry_startup = gtk_check_button_new_with_mnemonic (_("Use _startup notification"));
    gtk_box_pack_start (GTK_BOX (hbox), ld->entry_startup, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT (ld->entry_startup), "toggled",
                      G_CALLBACK (launcher_dialog_save_button), ld);
#endif

    /* release size group */
    g_object_unref (G_OBJECT (sg));

    /* setup dnd in frame */
    gtk_drag_dest_set (frame, GTK_DEST_DEFAULT_ALL,
                       drop_targets, G_N_ELEMENTS (drop_targets),
                       GDK_ACTION_COPY);

    g_signal_connect (frame, "drag-data-received",
                        G_CALLBACK (launcher_dialog_frame_drag_data_received), ld);

    return frame;
}




static GtkWidget *
launcher_dialog_add_tree (LauncherDialog *ld)
{
    GtkWidget         *scroll;
    GtkTreeViewColumn *column;
    GtkTreeSelection  *selection;
    GtkCellRenderer   *renderer;
    GtkTreeIter        iter;
    GList             *li;
    LauncherEntry     *entry;
    GdkPixbuf         *icon;
    const gchar       *name;

    /* scrolled window */
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
                                         GTK_SHADOW_IN);

    /* create new list store */
    ld->store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

    /* create tree view */
    ld->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (ld->store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ld->treeview), FALSE);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW (ld->treeview), COLUMN_NAME);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (ld->treeview), TRUE);
    gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (ld->treeview), TRUE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (ld->treeview), TRUE);
    gtk_container_add (GTK_CONTAINER (scroll), ld->treeview);

    /* create columns and cell renders */
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column (GTK_TREE_VIEW (ld->treeview), column);

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_renderer_set_fixed_size (renderer, LAUNCHER_TREE_ICON_SIZE, LAUNCHER_TREE_ICON_SIZE);
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", COLUMN_ICON, NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", COLUMN_NAME, NULL);
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    /* set selection change signal */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->treeview));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
    g_signal_connect_swapped (G_OBJECT (selection), "changed",
                              G_CALLBACK (launcher_dialog_tree_selection_changed), ld);

    /* append current items */
    for (li = ld->launcher->entries; li != NULL; li = li->next)
    {
        entry = li->data;

        if (G_LIKELY (entry))
        {
            /* load icon */
            icon = launcher_utility_load_pixbuf (gtk_widget_get_screen (ld->treeview), entry->icon, LAUNCHER_TREE_ICON_SIZE);
            
            /* build name */
            name = entry->name ? entry->name : _("Unnamed");

            /* create new row and add the data */
            gtk_list_store_append (ld->store, &iter);
            gtk_list_store_set (ld->store, &iter,
                                COLUMN_ICON, icon,
                                COLUMN_NAME, name, -1);

            /* release the pixbuf */
            if (G_LIKELY (icon))
                g_object_unref (G_OBJECT (icon));
        }
    }

    /* dnd support */
    gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (ld->treeview),
                                          drop_targets, G_N_ELEMENTS (drop_targets),
                                          GDK_ACTION_COPY);

    g_signal_connect (G_OBJECT (ld->treeview), "drag-data-received",
                      G_CALLBACK (launcher_dialog_tree_drag_data_received), ld);

    return scroll;
}



static GtkWidget *
launcher_dialog_add_tree_buttons (LauncherDialog *ld)
{
    GtkWidget *hbox, *button, *align, *image;

    hbox = gtk_hbox_new (FALSE, BORDER);

    /* up button */
    ld->up = button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (launcher_dialog_tree_button_clicked), ld);

    gtk_widget_set_sensitive (button, FALSE);

    /* down button */
    ld->down = button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (launcher_dialog_tree_button_clicked), ld);

    gtk_widget_set_sensitive (button, FALSE);

    /* free space between buttons */
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (align, 1, 1);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

    /* add button */
    ld->add = button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (launcher_dialog_tree_button_clicked), ld);

    /* remove button */
    ld->remove = button = gtk_button_new ();
    gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_container_add (GTK_CONTAINER (button), image);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (launcher_dialog_tree_button_clicked), ld);

    gtk_widget_set_sensitive (button, FALSE);

    return hbox;
}



/**
 * Dialog functions
 **/
static void
launcher_dialog_response (GtkWidget      *dialog,
                          gint            response,
                          LauncherDialog *ld)
{
    LauncherPlugin *launcher = ld->launcher;

    /* hide the dialog */
    gtk_widget_hide (dialog);

    /* lock for further updates */
    ld->updating = TRUE;
    ld->entry = NULL;

    /* cleanup the store */
    gtk_list_store_clear (ld->store);
    g_object_unref (G_OBJECT (ld->store));

    /* the launcher dialog dataS */
    g_object_set_data (G_OBJECT (launcher->panel_plugin), I_("launcher-dialog"), NULL);

    /* destroy the dialog */
    gtk_widget_destroy (dialog);

    /* unlock plugin menu */
    xfce_panel_plugin_unblock_menu (launcher->panel_plugin);

    /* restore move first */
    launcher->move_first = ld->stored_move_first;

    /* allow saving again */
    launcher->plugin_can_save = TRUE;

    if (response == GTK_RESPONSE_OK)
    {
        /* write new settings */
        launcher_plugin_save (launcher);
    }
    else /* revert changes */
    {
        /* remove all the entries */
        g_list_foreach (launcher->entries, (GFunc) launcher_entry_free, launcher);

        /* read the last saved settings */
        launcher_plugin_read (launcher);

        /* add new item if there are no entries yet */
        if (G_UNLIKELY (g_list_length (launcher->entries) == 0))
            launcher->entries = g_list_append (launcher->entries, launcher_entry_new ());
    }

    /* free the panel structure */
    panel_slice_free (LauncherDialog, ld);
}



void
launcher_dialog_show (LauncherPlugin  *launcher)
{
    LauncherDialog *ld;
    GtkWidget      *dialog;
    GtkWidget      *dialog_vbox;
    GtkWidget      *paned, *vbox, *hbox;
    GtkWidget      *widget, *label, *combo;
    GtkTreePath    *path;

    /* create new structure */
    ld = panel_slice_new0 (LauncherDialog);

    /* init */
    ld->launcher = launcher;
    ld->entry = g_list_first (launcher->entries)->data;

    /* prevent saving to be able to use the cancel button */
    launcher->plugin_can_save = FALSE;

    /* lock right-click plugin menu */
    xfce_panel_plugin_block_menu (launcher->panel_plugin);

    /* disable the auto sort of the list, while working in properties */
    ld->stored_move_first = launcher->move_first;
    launcher->move_first = FALSE;

    /* create new dialog */
    dialog = xfce_titled_dialog_new_with_buttons (_("Program Launcher"),
                                                  NULL,
                                                  GTK_DIALOG_NO_SEPARATOR,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);
    gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (launcher->panel_plugin)));
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), "xfce4-settings");
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

    /* connect dialog to plugin, so we can destroy it when plugin is closed */
    g_object_set_data (G_OBJECT (ld->launcher->panel_plugin), "dialog", dialog);

    dialog_vbox = GTK_DIALOG (dialog)->vbox;

    /* added the horizontal panes */
    paned = gtk_hpaned_new ();
    gtk_box_pack_start (GTK_BOX (dialog_vbox), paned, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (paned), BORDER - 2);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_paned_pack1 (GTK_PANED (paned), vbox, FALSE, FALSE);

    /* arrow button position */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new_with_mnemonic (_("A_rrow:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    ld->arrow_position = combo = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Default"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Left"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Right"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Top"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Bottom"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Inside Button"));
    gtk_widget_set_sensitive (combo, g_list_length (launcher->entries) > 1);
    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), launcher->arrow_position);
    g_signal_connect (G_OBJECT (combo), "changed", G_CALLBACK (launcher_dialog_arrow_position_changed), ld);
    gtk_widget_show (combo);

    /* add the entries list */
    widget = launcher_dialog_add_tree (ld);
    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

    /* add the tree navigation buttons */
    widget = launcher_dialog_add_tree_buttons (ld);
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

    /* add the entry widgets */
    widget = launcher_dialog_add_properties (ld);
    gtk_paned_pack2 (GTK_PANED (paned), widget, TRUE, FALSE);

    /* show all widgets inside dialog */
    gtk_widget_show_all (dialog_vbox);

    /* focus the title entry */
    gtk_widget_grab_focus (ld->entry_name);

    /* select first item in the tree (this also updates the fields) */
    path = gtk_tree_path_new_first ();
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->treeview), path, NULL, FALSE);
    gtk_tree_path_free (path);

    /* connect response signal */
    g_signal_connect (G_OBJECT (dialog), "response",
                      G_CALLBACK (launcher_dialog_response), ld);

    /* show the dialog */
    gtk_widget_show (dialog);
}
