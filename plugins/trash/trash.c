/*  trash.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "global.h"
#include "debug.h"

#include "controls.h"
#include "icons.h"
#include "iconbutton.h"
#include "xfce_support.h"

/* panel control configuration
   Global widget used in all the module configuration
   to revert the settings
*/
GtkWidget *revert_button;

/* this is checked when the control is loaded */
int is_xfce_panel_control = 1;


/*  Trash module
 *  ------------
*/
typedef struct
{
    char *dirname;
    char *command;
    gboolean in_terminal;

    gboolean empty;

    GdkPixbuf *empty_pb;
    GdkPixbuf *full_pb;

    IconButton *button;
}
t_trash;

static void trash_run(t_trash * trash)
{
    exec_cmd(trash->command, trash->in_terminal);
}

void trash_dropped(GtkWidget * widget, GList * drop_data, gpointer data)
{
    t_trash *trash = (t_trash *) data;
    char *cmd;
    GList *li;

    for(li = drop_data; li && li->data; li = li->next)
    {
        cmd = g_strconcat(trash->command, " ", (char *)li->data, NULL);

        exec_cmd_silent(cmd, FALSE);

        g_free(cmd);
    }
}

static t_trash *trash_new(void)
{
    t_trash *trash = g_new(t_trash, 1);
    const char *home = g_getenv("HOME");
    GtkWidget *b;

    trash->dirname = g_strconcat(home, "/.xfce/trash", NULL);

    trash->empty = TRUE;

    trash->command = g_strdup("xftrash");
    trash->in_terminal = FALSE;

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    trash->button = icon_button_new(trash->empty_pb);

    b = icon_button_get_button(trash->button);

    add_tooltip(b, _("Trashcan: 0 files"));

    /* signals */
    dnd_set_drag_dest(b);
    dnd_set_callback(b, DROP_CALLBACK(trash_dropped), trash);

    g_signal_connect_swapped(b, "clicked", G_CALLBACK(trash_run), trash);

    return trash;
}

static gboolean check_trash(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    GDir *dir;
    const char *file;
    char text[MAXSTRLEN];
    gboolean changed = FALSE;

    if(!trash->dirname)
        return TRUE;
    dir = g_dir_open(trash->dirname, 0, NULL);

    if(dir)
        file = g_dir_read_name(dir);

    if(!dir || !file)
    {
        if(!trash->empty)
        {
            trash->empty = TRUE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->empty_pb);
            add_tooltip(icon_button_get_button(trash->button),
                        _("Trashcan: 0 files"));
        }
    }
    else
    {
        struct stat s;
        int number = 0;
        int size = 0;
        char *cwd = g_get_current_dir();

        chdir(trash->dirname);

        if(trash->empty)
        {
            trash->empty = FALSE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->full_pb);
        }

        while(file)
        {
            number++;

            stat(file, &s);
            size += s.st_size;

            file = g_dir_read_name(dir);
        }

        chdir(cwd);
        g_free(cwd);

        if(size < 1024)
            sprintf(text, _("Trashcan: %d files / %d B"), number, size);
        else if(size < 1024 * 1024)
            sprintf(text, _("Trashcan: %d files / %d KB"), number, size / 1024);
        else
            sprintf(text, _("Trashcan: %d files / %d MB"), number,
                    size / (1024 * 1024));

        add_tooltip(icon_button_get_button(trash->button), text);
    }

    if(dir)
        g_dir_close(dir);

    return TRUE;
}

static void trash_free(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    g_free(trash->dirname);

    icon_button_free(trash->button);

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    g_free(trash);
}

static void trash_set_size(PanelControl * pc, int size)
{
    t_trash *trash = (t_trash *) pc->data;

    icon_button_set_size(trash->button, size);
}

static void trash_set_theme(PanelControl * pc, const char *theme)
{
    t_trash *trash = (t_trash *) pc->data;

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    if(trash->empty)
        icon_button_set_pixbuf(trash->button, trash->empty_pb);
    else
        icon_button_set_pixbuf(trash->button, trash->full_pb);
}

/*  create trash panel control
*/
/* this must be called 'module_init', because that is what we look for 
 * when opening the gmodule */
void module_init(PanelControl * pc)
{
    t_trash *trash = trash_new();
    GtkWidget *b = icon_button_get_button(trash->button);

    gtk_container_add(GTK_CONTAINER(pc->base), b);

    pc->caption = g_strdup(_("Trash can"));
    pc->data = (gpointer) trash;
    pc->main = b;

    pc->interval = 2000;        /* 2 sec */
    pc->update = (gpointer) check_trash;

    pc->free = (gpointer) trash_free;

    pc->set_size = (gpointer) trash_set_size;
    pc->set_theme = (gpointer) trash_set_theme;
}


