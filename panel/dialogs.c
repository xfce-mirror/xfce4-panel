/*  dialogs.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <j.b.huijsmans@hetnet.nl>
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

#include <stdio.h>
#include <string.h>
#include <gmodule.h>

#include "dialogs.h"

#include "xfce.h"
#include "dnd.h"
#include "callbacks.h"
#include "settings.h"
#include "central.h"
#include "side.h"
#include "item.h"
#include "popup.h"
#include "module.h"
#include "icons.h"

enum
{ RESPONSE_REMOVE, RESPONSE_CHANGE, RESPONSE_CANCEL, RESPONSE_REVERT };

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Convenience functions

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* Taken from ROX Filer (http://rox.sf.net)
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

   General dialogs

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

   Panel dialogs

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

GtkWidget *dialog = NULL;

void set_transient_for_dialog(GtkWidget * window)
{
    if(dialog)
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(dialog));
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Item frame

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* item frame structure */

typedef struct _ItemFrame ItemFrame;

struct _ItemFrame
{
    gboolean is_menuitem;

    int icon_id;
    char *icon_path;

    /* menu item */
    int num_items;

    /* gui */
    GtkWidget *frame;

    GtkWidget *command_entry;
    GtkWidget *command_browse_button;
    GtkWidget *term_check_button;

    GtkWidget *caption_entry;
    GtkWidget *tooltip_entry;

    GtkWidget *icon_option_menu;
    GtkWidget *icon_entry;
    GtkWidget *icon_browse_button;

    GtkWidget *spinbutton;

    GdkPixbuf *pb;
    GtkWidget *preview;
};

static ItemFrame *item_frame_new(gboolean is_menuitem)
{
    ItemFrame *iframe = g_new(ItemFrame, 1);

    iframe->is_menuitem = is_menuitem;

    iframe->icon_id = -1;
    iframe->icon_path = NULL;

    iframe->num_items = 0;

    iframe->frame = NULL;
    iframe->command_entry = NULL;
    iframe->command_browse_button = NULL;
    iframe->term_check_button = NULL;
    iframe->caption_entry = NULL;
    iframe->tooltip_entry = NULL;
    iframe->icon_option_menu = NULL;
    iframe->icon_entry = NULL;
    iframe->icon_browse_button = NULL;
    iframe->pb = NULL;
    iframe->preview = NULL;

    return iframe;
}

static void item_frame_free(ItemFrame * iframe)
{
    g_free(iframe->icon_path);
    g_object_unref(iframe->pb);

    g_free(iframe);
}

ItemFrame *iframe = NULL;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* icon option menu */

static void icon_id_changed(void)
{
    int new_id =
        gtk_option_menu_get_history(GTK_OPTION_MENU(iframe->icon_option_menu));

    if(iframe->icon_id == new_id || (iframe->icon_id == -1 && new_id == 0))
        return;

    g_object_unref(iframe->pb);
    iframe->pb = NULL;

    if(new_id == 0)
    {
        iframe->icon_id = -1;

        if(iframe->icon_path)
        {
            gtk_entry_set_text(GTK_ENTRY(iframe->icon_entry), iframe->icon_path);

            iframe->pb = gdk_pixbuf_new_from_file(iframe->icon_path, NULL);
        }

        if(!iframe->pb)
            iframe->pb = get_pixbuf_from_id(UNKNOWN_ICON);

        gtk_widget_set_sensitive(iframe->icon_browse_button, TRUE);
        gtk_widget_set_sensitive(iframe->icon_entry, TRUE);
    }
    else
    {
        if(iframe->icon_id == -1)
        {
            /* save path to put back if extern icon is chosen again */
            g_free(iframe->icon_path);
            iframe->icon_path =
                gtk_editable_get_chars(GTK_EDITABLE(iframe->icon_entry), 0, -1);
        }

        iframe->icon_id = new_id;

        gtk_entry_set_text(GTK_ENTRY(iframe->icon_entry), "");
        gtk_widget_set_sensitive(iframe->icon_browse_button, FALSE);
        gtk_widget_set_sensitive(iframe->icon_entry, FALSE);
        iframe->pb = get_themed_pixbuf_from_id(new_id, settings.icon_theme);
    }


    gtk_image_set_from_pixbuf(GTK_IMAGE(iframe->preview), iframe->pb);
}

static void create_icon_option_menu(void)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *mi;
    int i;

    mi = gtk_menu_item_new_with_label(_("External Icon"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    for(i = 1; i < NUM_ICONS; i++)
    {
        mi = gtk_menu_item_new_with_label(icon_names[i]);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }

    iframe->icon_option_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(iframe->icon_option_menu), menu);

    g_signal_connect_swapped(iframe->icon_option_menu, "changed",
                             G_CALLBACK(icon_id_changed), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* icon preview */

static void new_extern_icon(const char *path)
{
    g_return_if_fail(path && g_file_test(path, G_FILE_TEST_EXISTS));

    g_free(iframe->icon_path);
    iframe->icon_path = g_strdup(path);

    /* this takes care of setting the preview */
    iframe->icon_id = NUM_ICONS;        /* something it definitely is not now */
    gtk_option_menu_set_history(GTK_OPTION_MENU(iframe->icon_option_menu), 0);
    icon_id_changed();
}

static void
icon_drop_cb(GtkWidget * widget, GdkDragContext * context,
             gint x, gint y, GtkSelectionData * data,
             guint info, guint time, gpointer user_data)
{
    GList *fnames;
    guint count;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        char *icon;

        icon = (char *)fnames->data;

        new_extern_icon(icon);
    }

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

static GtkWidget *create_icon_preview_frame(void)
{
    GtkWidget *frame;
    GtkWidget *eventbox;

    frame = gtk_frame_new(_("Icon Preview"));

    eventbox = gtk_event_box_new();
    add_tooltip(eventbox, _("Drag file onto this frame to change the icon"));
    gtk_container_add(GTK_CONTAINER(frame), eventbox);

    iframe->preview = gtk_image_new_from_pixbuf(iframe->pb);
    gtk_container_add(GTK_CONTAINER(eventbox), iframe->preview);

    /* signals */
    dnd_set_drag_dest(eventbox);

    g_signal_connect(eventbox, "drag_data_received", G_CALLBACK(icon_drop_cb), NULL);

    return frame;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void browse_cb(GtkWidget * b, GtkEntry * entry)
{
    char *file;
    char *path = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    if(!path || !strlen(path))
        path = NULL;

    file = select_file_name(NULL, path, dialog);

    if(file && strlen(file))
    {
        if(b == iframe->icon_browse_button)
        {
            if(g_file_test(file, G_FILE_TEST_EXISTS))
            {
                new_extern_icon(file);
                gtk_entry_set_text(entry, file);
            }
        }
        else
        {
            gtk_entry_set_text(entry, file);
        }
    }

    g_free(file);
    g_free(path);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* create the frame to add to the dialog */

static void create_item_frame(void)
{
    GtkWidget *main_hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    GtkWidget *frame;

    iframe->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(iframe->frame), GTK_SHADOW_NONE);

    main_hbox = gtk_hbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(main_hbox), 10);
    gtk_container_add(GTK_CONTAINER(iframe->frame), main_hbox);

    vbox = gtk_vbox_new(TRUE, 8);
    gtk_box_pack_start(GTK_BOX(main_hbox), vbox, TRUE, TRUE, 0);

    /* command */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    iframe->command_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), iframe->command_entry, TRUE, TRUE, 0);

    iframe->command_browse_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), iframe->command_browse_button, FALSE, FALSE,
                       0);

    g_signal_connect(iframe->command_browse_button, "clicked", G_CALLBACK(browse_cb),
                     iframe->command_entry);

    /* terminal */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    iframe->term_check_button =
        gtk_check_button_new_with_mnemonic(_("Run in _terminal"));
    gtk_box_pack_start(GTK_BOX(hbox), iframe->term_check_button, FALSE, FALSE, 0);
    gtk_widget_show(iframe->term_check_button);

    /* icon */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Icon:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    create_icon_option_menu();
    gtk_box_pack_start(GTK_BOX(hbox), iframe->icon_option_menu, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    iframe->icon_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(iframe->icon_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), iframe->icon_entry, TRUE, TRUE, 0);

    iframe->icon_browse_button = gtk_button_new_with_label(" ... ");
    gtk_box_pack_start(GTK_BOX(hbox), iframe->icon_browse_button, FALSE, FALSE, 0);

    g_signal_connect(iframe->icon_browse_button, "clicked", G_CALLBACK(browse_cb),
                     iframe->icon_entry);

    if(iframe->icon_id == -1 || iframe->icon_id == 0)
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(iframe->icon_option_menu), 0);
        if(iframe->icon_path)
            gtk_entry_set_text(GTK_ENTRY(iframe->icon_entry), iframe->icon_path);
    }
    else
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(iframe->icon_option_menu),
                                    iframe->icon_id);
        gtk_widget_set_sensitive(iframe->icon_entry, FALSE);
        gtk_widget_set_sensitive(iframe->icon_browse_button, FALSE);
    }

    /* caption */
    if(iframe->is_menuitem)
    {
        hbox = gtk_hbox_new(FALSE, 4);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        label = gtk_label_new(_("Caption:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_size_group_add_widget(sg, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        iframe->caption_entry = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), iframe->caption_entry, TRUE, TRUE, 0);
    }

    /* tooltip */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Tooltip:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    iframe->tooltip_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), iframe->tooltip_entry, TRUE, TRUE, 0);

    /* position */
    if(iframe->is_menuitem && iframe->num_items > 1)
    {
        hbox = gtk_hbox_new(FALSE, 4);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        label = gtk_label_new(_("Position:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_size_group_add_widget(sg, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        iframe->spinbutton = gtk_spin_button_new_with_range(1, iframe->num_items, 1);
        gtk_box_pack_start(GTK_BOX(hbox), iframe->spinbutton, FALSE, FALSE, 0);
    }

    /* icon preview */
    frame = create_icon_preview_frame();
    gtk_box_pack_start(GTK_BOX(main_hbox), frame, TRUE, TRUE, 0);

    gtk_widget_show_all(iframe->frame);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Module frame

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

typedef struct _ModuleFrame ModuleFrame;

struct _ModuleFrame
{
    GtkWidget *frame;
    GtkWidget *module_option_menu;
    int module_id;
    GtkWidget *configure_button;
    GList *modules;
    PanelModule *current;
};

static ModuleFrame *module_frame_new(void)
{
    ModuleFrame *mframe = g_new(ModuleFrame, 1);

    mframe->frame = NULL;
    mframe->module_option_menu = NULL;
    mframe->module_id = 0;
    mframe->configure_button = NULL;
    mframe->modules = NULL;
    mframe->current = NULL;

    return mframe;
}

static void module_frame_free(ModuleFrame * mframe)
{
    g_list_foreach(mframe->modules, (GFunc) panel_module_free, NULL);

    g_free(mframe);
}

ModuleFrame *mframe = NULL;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* module option menu */

static void module_id_changed(void)
{
    int new_id =
        gtk_option_menu_get_history(GTK_OPTION_MENU(mframe->module_option_menu));

    if(mframe->module_id == new_id)
        return;

    mframe->module_id = new_id;

    if(new_id == 0)
    {
        mframe->current = NULL;

        gtk_widget_set_sensitive(mframe->configure_button, FALSE);
    }
    else if(new_id <= BUILTIN_MODULES)
    {
        mframe->current =
            (PanelModule *) g_list_nth(mframe->modules, new_id - 1)->data;
    }
    else
    {
        mframe->current =
            (PanelModule *) g_list_nth(mframe->modules, new_id - 1)->data;
    }

    if(mframe->current && mframe->current->configure)
    {
        gtk_widget_set_sensitive(mframe->configure_button, TRUE);
    }
}

static char *get_module_name_from_file(const char *file)
{
    char name[strlen(file)];
    char *s;

    strcpy(name, file);

    s = strchr(name, '.');

    while(s)
    {
        if(s + 1 && !strncmp(s + 1, G_MODULE_SUFFIX, strlen(G_MODULE_SUFFIX)))
        {
            *(s + 1 + strlen(G_MODULE_SUFFIX)) = '\0';
            break;
        }

        s = strchr(s + 1, '.');
    }

    return g_strdup(name);
}

int compare_modules(PanelModule * pm1, PanelModule * pm2)
{
    g_return_val_if_fail(pm1->caption && pm2->caption, -1);

    return strcmp(pm1->caption, pm2->caption);
}

static void create_module_option_menu(void)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *mi;
    int i;
    const char *home, *file;
    char dirname[MAXSTRLEN + 1];
    GList *module_names = NULL;
    GDir *gdir;

    mframe->modules = NULL;
    mframe->current = NULL;

    /* first empty item */
    mi = gtk_menu_item_new_with_label(_("None"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    /* builtins */
    for(i = 0; i < BUILTIN_MODULES; i++)
    {
        PanelModule *pm = panel_module_new(NULL);

        pm->id = i;
        create_panel_module(pm);

        mframe->modules = g_list_append(mframe->modules, pm);

        mi = gtk_menu_item_new_with_label(pm->caption);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }

    /* separator
       mi = gtk_separator_menu_item_new ();
       gtk_widget_show (mi);
       gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
     */
    /* plugins */
    home = g_getenv("HOME");
    for(i = 0; i < 2; i++)
    {
        if(i == 0)
            snprintf(dirname, MAXSTRLEN, "%s/.xfce4/plugins", home);
        else
            snprintf(dirname, MAXSTRLEN, "%s/plugins", DATADIR);

        gdir = g_dir_open(dirname, 0, NULL);

        if(gdir)
        {
            while((file = g_dir_read_name(gdir)))
            {
                PanelModule *pm = panel_module_new(NULL);
                char *path;
                GModule *tmp;

                pm->id = EXTERN_MODULE;

                path = g_build_filename(dirname, file, NULL);

                tmp = g_module_open(path, 0);
                g_free(path);

                if(!tmp)
                {
                    panel_module_free(pm);
                    continue;
                }

                pm->name = get_module_name_from_file(file);

                g_module_close(tmp);

                pm->dir = g_strdup(dirname);

                if(!create_panel_module(pm))
                {
                    panel_module_free(pm);
                    continue;
                }

                /* check if we already have a module with the same name */
                if(g_list_find_custom
                   (mframe->modules, pm, (GCompareFunc) compare_modules))
                {
                    panel_module_free(pm);
                    continue;
                }

                mframe->modules = g_list_append(mframe->modules, pm);
                module_names = g_list_append(module_names, pm->name);

                if(pm->caption)
                    mi = gtk_menu_item_new_with_label(pm->caption);
                else
                    mi = gtk_menu_item_new_with_label(pm->name);

                gtk_widget_show(mi);
                gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
            }

            g_dir_close(gdir);
        }
    }

    g_list_free(module_names);

    mframe->module_option_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(mframe->module_option_menu), menu);
    module_id_changed();

    g_signal_connect_swapped(mframe->module_option_menu, "changed",
                             G_CALLBACK(module_id_changed), NULL);

}



/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void configure_cb(void)
{
    if(mframe->current && mframe->current->configure)
        mframe->current->configure(mframe->current);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* create frame to add to dialog */

static void create_module_frame(PanelModule * pm)
{
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    GList *modules;
    int i;

    mframe->frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(mframe->frame), GTK_SHADOW_NONE);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(mframe->frame), vbox);

    /* option menu */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Module:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    create_module_option_menu();
    gtk_box_pack_start(GTK_BOX(hbox), mframe->module_option_menu, TRUE, TRUE, 0);

    /* configure button */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    mframe->configure_button = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
    gtk_widget_set_sensitive(mframe->configure_button, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), mframe->configure_button, FALSE, FALSE, 0);

    g_signal_connect_swapped(mframe->configure_button, "clicked",
                             G_CALLBACK(configure_cb), NULL);

    if(pm)
    {
        /* option menu id 0 is empty */
        for(i = 1, modules = mframe->modules; modules; modules = modules->next, i++)
        {
            PanelModule *module = (PanelModule *) modules->data;

            if(pm->id != module->id)
                continue;

            if((!module->id != EXTERN_MODULE && module->id == pm->id) ||
               (module->name && pm->name && strequal(module->name, pm->name)))
            {
                gtk_option_menu_set_history(GTK_OPTION_MENU
                                            (mframe->module_option_menu), i);
                break;
            }
        }
    }

    module_id_changed();
    gtk_widget_show_all(mframe->frame);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Panel group dialog

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static gboolean is_icon = FALSE;
GtkWidget *notebook;

static void control_type_changed(GtkToggleButton * b, gpointer data)
{
    if(gtk_toggle_button_get_active(b))
    {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
        is_icon = TRUE;
    }
    else
    {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
        is_icon = FALSE;
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static GtkWidget *create_panel_control_dialog(PanelItem * pi, PanelModule * pm)
{
    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *separator;
    GtkWidget *icon_radio_button;
    GtkWidget *module_radio_button;

    dialog = gtk_dialog_new_with_buttons(_("Change item"), GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL, GTK_STOCK_CANCEL,
                                         RESPONSE_CANCEL, GTK_STOCK_APPLY,
                                         RESPONSE_CHANGE, NULL);
    /* radio buttons */
    hbox = gtk_hbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Item type:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    icon_radio_button = gtk_radio_button_new_with_mnemonic(NULL, _("_Icon"));
    gtk_box_pack_start(GTK_BOX(hbox), icon_radio_button, FALSE, FALSE, 0);

    module_radio_button =
        gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON
                                                       (icon_radio_button),
                                                       _("_Module"));
    gtk_box_pack_start(GTK_BOX(hbox), module_radio_button, FALSE, FALSE, 0);

    gtk_widget_show_all(hbox);

    /* separator */
    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       separator, FALSE, FALSE, 0);

    /* notebook */
    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, FALSE, FALSE, 0);

    /* item page */
    if(iframe)
        item_frame_free(iframe);

    iframe = item_frame_new(FALSE);

    if(pi)
    {
        iframe->icon_id = pi->id;
        iframe->pb = pi->pb;
        if(pi->path)
            iframe->icon_path = g_strdup(pi->path);

        g_object_ref(iframe->pb);
    }
    else
    {
        iframe->pb = get_pixbuf_from_id(UNKNOWN_ICON);
    }

    create_item_frame();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), iframe->frame, NULL);

    if(pi && pi->path)
        gtk_entry_set_text(GTK_ENTRY(iframe->icon_entry), pi->path);
    if(pi && pi->command)
        gtk_entry_set_text(GTK_ENTRY(iframe->command_entry), pi->command);
    if(pi && pi->in_terminal)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iframe->term_check_button),
                                     TRUE);
    if(pi && pi->tooltip)
        gtk_entry_set_text(GTK_ENTRY(iframe->tooltip_entry), pi->tooltip);

    /* module page */
    if(mframe)
        module_frame_free(mframe);

    mframe = module_frame_new();

    create_module_frame(pm);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), mframe->frame, NULL);

    /* signals */
    g_signal_connect(icon_radio_button, "toggled",
                     G_CALLBACK(control_type_changed), NULL);

    if(is_icon)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(icon_radio_button), TRUE);
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(module_radio_button), TRUE);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
    }

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    return dialog;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   dialogs

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void reposition_popup(PanelPopup * pp)
{
    if(pp->detached)
        return;

    gtk_button_clicked(GTK_BUTTON(pp->button));
    gtk_button_clicked(GTK_BUTTON(pp->button));
}

void add_menu_item_dialog(PanelPopup * pp)
{
    int response;
    GtkWidget *box;

    if (disable_user_config)
	return;
    
    /* dialog */
    dialog = gtk_dialog_new_with_buttons(_("Add item"), GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         RESPONSE_CHANGE,
                                         GTK_STOCK_ADD, RESPONSE_CHANGE, NULL);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    /* item frame */
    if(iframe)
        item_frame_free(iframe);

    iframe = item_frame_new(TRUE);

    iframe->num_items = g_list_length(pp->items) + 1;

    iframe->pb = get_pixbuf_from_id(UNKNOWN_ICON);
    create_item_frame();

    box = GTK_DIALOG(dialog)->vbox;
    gtk_box_pack_start(GTK_BOX(box), iframe->frame, TRUE, TRUE, 0);

    /* run */
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if(response == RESPONSE_CHANGE)
    {
        MenuItem *mi = menu_item_new(pp);

        mi->command =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->command_entry), 0, -1);
        mi->in_terminal =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                         (iframe->term_check_button));
        mi->caption =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->caption_entry), 0, -1);
        mi->tooltip =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->tooltip_entry), 0, -1);

        mi->icon_path = g_strdup(iframe->icon_path);
        mi->icon_id = iframe->icon_id;

        if(iframe->num_items > 1)
            mi->pos =
                gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(iframe->spinbutton))
                - 1;

        create_menu_item(mi);
        gtk_box_pack_start(GTK_BOX(pp->vbox), mi->button, TRUE, TRUE, 0);

        gtk_box_reorder_child(GTK_BOX(pp->vbox), mi->button, mi->pos + 2);

        pp->items = g_list_insert(pp->items, mi, mi->pos);

        menu_item_set_popup_size(mi, settings.size);
        menu_item_set_style(mi, settings.style);
	
	if (settings.icon_theme)
	    menu_item_set_icon_theme(mi, settings.icon_theme);

        reposition_popup(pp);
    }

    /* clean up */
    gtk_widget_destroy(dialog);
    dialog = NULL;

    item_frame_free(iframe);
    iframe = NULL;
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void edit_menu_item_dialog(MenuItem * mi)
{
    GtkWidget *box;
    int response;

    if (disable_user_config)
	return;
    
    dialog = gtk_dialog_new_with_buttons(_("Edit item"), GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         RESPONSE_CANCEL,
                                         GTK_STOCK_REMOVE,
                                         RESPONSE_REMOVE,
                                         GTK_STOCK_APPLY, RESPONSE_CHANGE, NULL);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    /* item frame */
    if(iframe)
        item_frame_free(iframe);

    iframe = item_frame_new(TRUE);

    iframe->num_items = g_list_length(mi->parent->items);

    iframe->icon_id = mi->icon_id;
    iframe->pb = mi->pixbuf;
    g_object_ref(iframe->pb);

    create_item_frame();

    box = GTK_DIALOG(dialog)->vbox;
    gtk_box_pack_start(GTK_BOX(box), iframe->frame, TRUE, TRUE, 0);

    if(mi->command)
        gtk_entry_set_text(GTK_ENTRY(iframe->command_entry), mi->command);
    if(mi->in_terminal)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iframe->term_check_button),
                                     TRUE);
    if(mi->caption)
        gtk_entry_set_text(GTK_ENTRY(iframe->caption_entry), mi->caption);
    if(mi->tooltip)
        gtk_entry_set_text(GTK_ENTRY(iframe->tooltip_entry), mi->tooltip);

    if(iframe->num_items > 1)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(iframe->spinbutton),
                                  (gfloat) mi->pos + 1);

    /* run */
    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if(response == RESPONSE_CHANGE)
    {
        PanelPopup *pp = mi->parent;

        gtk_container_remove(GTK_CONTAINER(mi->parent->vbox), mi->button);
        pp->items = g_list_remove(pp->items, mi);
        menu_item_free(mi);

        mi = menu_item_new(pp);

        mi->command =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->command_entry), 0, -1);
        mi->in_terminal =
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                         (iframe->term_check_button));
        mi->caption =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->caption_entry), 0, -1);
        mi->tooltip =
            gtk_editable_get_chars(GTK_EDITABLE(iframe->tooltip_entry), 0, -1);

        mi->icon_path = g_strdup(iframe->icon_path);
        mi->icon_id = iframe->icon_id;

        if(iframe->num_items > 1)
            mi->pos =
                gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
                                                 (iframe->spinbutton)) - 1;
        pp->items = g_list_insert(pp->items, mi, mi->pos);

        create_menu_item(mi);
        gtk_box_pack_start(GTK_BOX(pp->vbox), mi->button, TRUE, TRUE, 0);

        gtk_box_reorder_child(GTK_BOX(pp->vbox), mi->button, mi->pos + 2);

        menu_item_set_popup_size(mi, settings.size);
        menu_item_set_style(mi, settings.style);

	if (settings.icon_theme)
	    menu_item_set_icon_theme(mi, settings.icon_theme);

        reposition_popup(pp);
    }
    else if(response == RESPONSE_REMOVE)
    {
        PanelPopup *pp = mi->parent;

        pp->items = g_list_remove(pp->items, mi);

        gtk_container_remove(GTK_CONTAINER(mi->parent->vbox), mi->button);
        menu_item_free(mi);
	
        reposition_popup(pp);
    }

    /* clean up */
    gtk_widget_destroy(dialog);
    dialog = NULL;

    item_frame_free(iframe);
    iframe = NULL;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* change panel control (item or module) */

void edit_panel_control_dialog(PanelGroup * pg)
{
    PanelItem *pi;
    PanelModule *pm;
    gboolean was_item;
    int response;

    pi = pg->item;
    pm = pg->module;

    was_item = (pg->type == ICON);

    /* global var */
    is_icon = was_item;

    dialog = create_panel_control_dialog(pi, pm);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if(response == RESPONSE_CHANGE)
    {
        if(pm)
        {
            panel_module_stop(pm);
            panel_module_unpack(pm, GTK_CONTAINER(pg->vbox));
            panel_module_free(pm);
        }
        else
        {
            panel_item_unpack(pi, GTK_CONTAINER(pg->vbox));
            gtk_widget_destroy(pi->button);
            panel_item_free(pi);
        }

        pg->item = NULL;
        pg->module = NULL;

        if(is_icon)
        {
            pg->type = ICON;
            pi = pg->item = panel_item_new(pg);
            pi->command =
                gtk_editable_get_chars(GTK_EDITABLE(iframe->command_entry), 0, -1);
            pi->in_terminal =
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                             (iframe->term_check_button));
            pi->tooltip =
                gtk_editable_get_chars(GTK_EDITABLE(iframe->tooltip_entry), 0, -1);
            pi->id = iframe->icon_id;
            pi->path = iframe->icon_path ? g_strdup(iframe->icon_path) : NULL;

            create_panel_item(pi);
            panel_item_pack(pi, GTK_BOX(pg->vbox));

            panel_item_set_size(pi, settings.size);
	    
	    if (settings.icon_theme)
		panel_item_set_icon_theme(pi, settings.icon_theme);
        }
        else
        {
            if(mframe->current)
            {
                pg->type = MODULE;
                pm = pg->module = panel_module_new(pg);
                pm->id = mframe->current->id;
                pm->name =
                    (mframe->current->name) ? g_strdup(mframe->current->name) : NULL;

                create_panel_module(pm);
                panel_module_pack(pm, GTK_BOX(pg->vbox));
                panel_module_start(pm);

                panel_module_set_size(pm, settings.size);
                panel_module_set_style(pm, settings.style);
                panel_module_start(pm);
            }
            else
            {
                pg->type = ICON;
                pg->item = panel_item_unknown_new(pg);
                create_panel_item(pg->item);
                panel_item_pack(pg->item, GTK_BOX(pg->vbox));

                panel_item_set_size(pi, settings.size);
            }
        }
    }

    /* clean up */
    gtk_widget_destroy(dialog);
    dialog = NULL;

    item_frame_free(iframe);
    iframe = NULL;

    module_frame_free(mframe);
    mframe = NULL;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Global settings

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* size: option menu
 * popup size: option menu
 * style: option menu
 * icon theme: option menu
 * left : spinbutton
 * right: spinbutton
 * screens: spinbutton
 * position: button (restore default)
 * lock command : entry + browse button
 * exit command : entry + browse button
 */

static GtkWidget *size_menu;
static GtkWidget *popup_menu;
static GtkWidget *style_menu;
static GtkWidget *theme_menu;
static GtkWidget *left_spin;
static GtkWidget *right_spin;
static GtkWidget *screens_spin;
static GtkWidget *pos_button;
static GtkWidget *lock_entry;
static GtkWidget *exit_entry;

static GtkSizeGroup *sg = NULL;
static GtkWidget *revert;

static Settings backup;
static int backup_theme_index = 0;

GtkShadowType main_shadow = GTK_SHADOW_IN;
GtkShadowType header_shadow = GTK_SHADOW_NONE;
GtkShadowType option_shadow = GTK_SHADOW_NONE;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* backup */

static void create_backup(void)
{
    backup.x = settings.x;
    backup.y = settings.y;
    backup.size = settings.size;
    backup.popup_size = settings.popup_size;
    backup.style = settings.style;
    backup.icon_theme = g_strdup(settings.icon_theme);
    backup.num_left = settings.num_left;
    backup.num_right = settings.num_right;
    backup.num_screens = settings.num_screens;
    backup.lock_command = g_strdup(settings.lock_command);
    backup.exit_command = g_strdup(settings.exit_command);
}

static void restore_backup(void)
{
    settings.x = backup.x;
    settings.y = backup.y;
    panel_set_position();

    gtk_option_menu_set_history(GTK_OPTION_MENU(size_menu), backup.size);
    gtk_option_menu_set_history(GTK_OPTION_MENU(popup_menu), backup.popup_size);
    gtk_option_menu_set_history(GTK_OPTION_MENU(theme_menu), backup_theme_index);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spin), backup.num_left);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spin), backup.num_right);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(screens_spin), backup.num_screens);

    if(backup.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), backup.lock_command);
    else
        gtk_entry_set_text(GTK_ENTRY(lock_entry), "");

    if(backup.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), backup.exit_command);
    else
        gtk_entry_set_text(GTK_ENTRY(exit_entry), "");
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* sections */

static void add_header(const char *text, GtkBox * box)
{
    GtkWidget *frame, *eventbox, *label;
    char *markup;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), header_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, FALSE, TRUE, 0);

    eventbox = gtk_event_box_new();
    gtk_widget_set_name(eventbox, "gxfce_color2");
    gtk_widget_show(eventbox);
    gtk_container_add(GTK_CONTAINER(frame), eventbox);
    
    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    markup = g_strconcat("<b><span size=\"large\">", text, "</span></b>", NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
}

#define SKIP 12

static void add_spacer(GtkBox * box)
{
    GtkWidget *eventbox = gtk_event_box_new();

    gtk_widget_set_size_request(eventbox, SKIP, SKIP);
    gtk_widget_show(eventbox);
    gtk_box_pack_start(box, eventbox, FALSE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* sizes */

static void size_menu_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);
    gboolean changed = TRUE;

    if(GTK_WIDGET(menu) == size_menu && n != settings.size)
        panel_set_size(n);
    else if(n != settings.popup_size)
        panel_set_popup_size(n);
    else
        changed = FALSE;

    if(changed)
    {
        panel_set_position();
        gtk_widget_set_sensitive(revert, TRUE);
    }
}

static void add_size_menu(GtkWidget * option_menu, int size)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Small"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Medium"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Large"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), size);

    g_signal_connect(option_menu, "changed", G_CALLBACK(size_menu_changed), NULL);
}

static void add_size_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* size */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    size_menu = gtk_option_menu_new();
    gtk_widget_show(size_menu);
    add_size_menu(size_menu, settings.size);
    gtk_box_pack_start(GTK_BOX(hbox), size_menu, TRUE, TRUE, 0);

    /* popup */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Popup menu size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    popup_menu = gtk_option_menu_new();
    gtk_widget_show(popup_menu);
    add_size_menu(popup_menu, settings.popup_size);
    gtk_box_pack_start(GTK_BOX(hbox), popup_menu, TRUE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* style */

static void style_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);

    if(n == settings.style)
        return;

    panel_set_style(n);
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_style_menu(GtkWidget * option_menu, int style)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Traditional"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Modern"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), style);

    g_signal_connect(option_menu, "changed", G_CALLBACK(style_changed), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static char **find_icon_themes(void)
{
    char **themes = NULL;
    GList *list = NULL;
    GDir *gdir;
    char **dirs;
    const char *file;
    int i;

    dirs = g_new(char *, 3);
    dirs[0] = g_build_filename(g_getenv("HOME"), ".xfce4", "themes", NULL);
    dirs[1] = g_build_filename(DATADIR, "themes", NULL);
    dirs[2] = NULL;

    for(i = 0; i < 2; i++)
    {
        gdir = g_dir_open(dirs[i], 0, NULL);

        if(gdir)
        {
            while((file = g_dir_read_name(gdir)))
            {
                char *path = g_build_filename(dirs[i], file);

                if(!g_list_find_custom(list, file, (GCompareFunc) strcmp) &&
                   g_file_test(path, G_FILE_TEST_IS_DIR))
                {
                    list = g_list_append(list, g_strdup(file));
                }

                g_free(path);
            }

            g_dir_close(gdir);
        }
    }

    if(list)
    {
        int len = g_list_length(list);
        GList *li;

        themes = g_new0(char *, len + 1);

        for(i = 0, li = list; li; li = li->next, i++)
        {
            themes[i] = (char *)li->data;
        }
    }

    g_strfreev(dirs);
    return themes;
}

static void theme_changed(GtkOptionMenu * option_menu)
{
    int n = gtk_option_menu_get_history(option_menu);
    const char *theme;

    if(n == 0)
        theme = NULL;
    else
    {
        GtkWidget *label;

        /* Right, this is weird, apparently the option menu
         * button reparents the label connected to the menuitem
         * that is selected. So to get to the label we go to the
         * child of the button and not of the menu item!
         *
         * This took a while to find out :-)
         */

        label = gtk_bin_get_child(GTK_BIN(option_menu));

        theme = gtk_label_get_text(GTK_LABEL(label));
    }

    if((theme == NULL && settings.icon_theme == NULL) ||
       (settings.icon_theme && theme && strequal(theme, settings.icon_theme)))
        return;

    panel_set_icon_theme(theme);
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_theme_menu(GtkWidget * option_menu, const char *theme)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;
    int i = 0, n = 0;
    char **themes = find_icon_themes();
    char **s;

    item = gtk_menu_item_new_with_label(_("XFce standard theme"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if(themes)
    {
        for(i = 0, s = themes; *s; s++, i++)
        {
            item = gtk_menu_item_new_with_label(*s);
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

            if(settings.icon_theme && strequal(settings.icon_theme, *s))
                n = backup_theme_index = i + 1;
        }

        g_strfreev(themes);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), n);

    g_signal_connect(option_menu, "changed", G_CALLBACK(theme_changed), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void add_style_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* style */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel style:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    style_menu = gtk_option_menu_new();
    gtk_widget_show(style_menu);
    add_style_menu(style_menu, settings.style);
    gtk_box_pack_start(GTK_BOX(hbox), style_menu, TRUE, TRUE, 0);

    /* icon theme */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Icon theme:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    theme_menu = gtk_option_menu_new();
    gtk_widget_show(theme_menu);
    add_theme_menu(theme_menu, settings.icon_theme);
    gtk_box_pack_start(GTK_BOX(hbox), theme_menu, TRUE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* panel groups and screen buttons */

static void spin_changed(GtkWidget * spin)
{
    int n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    gboolean changed = FALSE;

    if(spin == left_spin && n != settings.num_left)
    {
        panel_set_num_left(n);
        changed = TRUE;
    }
    else if(spin == right_spin && n != settings.num_right)
    {
        panel_set_num_right(n);
        changed = TRUE;
    }
    else if(spin == screens_spin && n != settings.num_screens)
    {
        panel_set_num_screens(n);
        changed = TRUE;
    }

    if(changed)
        gtk_widget_set_sensitive(revert, TRUE);
}

static void add_controls_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* left */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Left panel controls:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    left_spin = gtk_spin_button_new_with_range(1, NBGROUPS, 1);
    gtk_widget_show(left_spin);
    gtk_box_pack_start(GTK_BOX(hbox), left_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spin), settings.num_left);
    g_signal_connect(left_spin, "value-changed", G_CALLBACK(spin_changed), NULL);

    /* right */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Right panel controls:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    right_spin = gtk_spin_button_new_with_range(1, NBGROUPS, 1);
    gtk_widget_show(right_spin);
    gtk_box_pack_start(GTK_BOX(hbox), right_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spin), settings.num_right);
    g_signal_connect(right_spin, "value-changed", G_CALLBACK(spin_changed), NULL);

    /* screens */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Virtual desktops:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    screens_spin = gtk_spin_button_new_with_range(1, NBSCREENS, 1);
    gtk_widget_show(screens_spin);
    gtk_box_pack_start(GTK_BOX(hbox), screens_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(screens_spin), settings.num_screens);
    g_signal_connect(screens_spin, "value-changed", G_CALLBACK(spin_changed), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* position */

static void position_clicked(GtkWidget * button)
{
    settings.x = -1;
    settings.y = -1;

    panel_set_position();
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_position_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* button */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Default position:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    pos_button = mixed_button_new(GTK_STOCK_REFRESH, _("Set"));
    gtk_widget_show(pos_button);
    gtk_box_pack_start(GTK_BOX(hbox), pos_button, FALSE, FALSE, 0);

    g_signal_connect(pos_button, "clicked", G_CALLBACK(position_clicked), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* lock and exit commands */

static void entry_changed(GtkWidget * w)
{
    gtk_widget_set_sensitive(revert, TRUE);
}

static void browse_clicked(GtkWidget * b, GtkEntry * entry)
{
    char *file;
    char *path = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    if(!path || !g_file_test(path, G_FILE_TEST_EXISTS))
    {
        g_free(path);
        path = NULL;
    }

    file = select_file_name(NULL, path, dialog);

    if(file && strlen(file))
    {
        gtk_entry_set_text(entry, file);
        gtk_widget_set_sensitive(revert, TRUE);
    }

    g_free(file);
    g_free(path);
}

static void add_advanced_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label, *button;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* lock */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Lock command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    lock_entry = gtk_entry_new();
    gtk_widget_show(lock_entry);
    gtk_box_pack_start(GTK_BOX(hbox), lock_entry, TRUE, TRUE, 0);

    if(settings.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), settings.lock_command);
    g_signal_connect(lock_entry, "insert-at-cursor", G_CALLBACK(entry_changed),
                     NULL);
    g_signal_connect(lock_entry, "delete-from-cursor", G_CALLBACK(entry_changed),
                     NULL);

    button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(browse_clicked), lock_entry);

    /* exit */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Exit command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    exit_entry = gtk_entry_new();
    gtk_widget_show(exit_entry);
    gtk_box_pack_start(GTK_BOX(hbox), exit_entry, TRUE, TRUE, 0);

    if(settings.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), settings.exit_command);
    g_signal_connect(exit_entry, "insert-at-cursor", G_CALLBACK(entry_changed),
                     NULL);
    g_signal_connect(exit_entry, "delete-from-cursor", G_CALLBACK(entry_changed),
                     NULL);

    button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(browse_clicked), exit_entry);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* the dialog */

static gboolean running = FALSE;

void global_settings_dialog(void)
{
    GtkWidget *frame, *vbox, *button;
    gboolean done;

    if(running)
    {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    running = TRUE;
    done = FALSE;

    create_backup();

    dialog =
        gtk_dialog_new_with_buttons(_("Xfce Panel Settings"), GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                    NULL);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);

    revert = mixed_button_new(GTK_STOCK_UNDO, _("_Revert"));
    gtk_widget_show(revert);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), revert, RESPONSE_REVERT);
    gtk_widget_set_sensitive(revert, FALSE);

    button = mixed_button_new(GTK_STOCK_OK, _("_Done"));
    gtk_widget_show(button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button, GTK_RESPONSE_OK);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), main_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    add_header(_("Sizes"), GTK_BOX(vbox));
    add_size_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Appearance"), GTK_BOX(vbox));
    add_style_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Panel controls"), GTK_BOX(vbox));
    add_controls_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Position"), GTK_BOX(vbox));
    add_position_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Advanced settings"), GTK_BOX(vbox));
    add_advanced_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    g_object_unref(sg);

    while(!done)
    {
        int response = GTK_RESPONSE_NONE;

        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if(response == RESPONSE_REVERT)
        {
            restore_backup();

            panel_set_settings();
            gtk_widget_set_sensitive(revert, FALSE);
        }
        else if(GTK_IS_WIDGET(dialog))
        {
            const char *cmd;

            cmd = gtk_entry_get_text(GTK_ENTRY(lock_entry));

            if(cmd && strlen(cmd))
            {
                g_free(settings.lock_command);
                settings.lock_command = g_strdup(cmd);
            }

            cmd = gtk_entry_get_text(GTK_ENTRY(exit_entry));

            if(cmd && strlen(cmd))
            {
                g_free(settings.exit_command);
                settings.exit_command = g_strdup(cmd);
            }

            done = TRUE;
        }
        else
            done = TRUE;
    }

    gtk_widget_destroy(dialog);
    running = FALSE;
    dialog = NULL;
}
