/*  item_dialog.c
 *
 *  Copyright (C) Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#include "global.h"
#include "item_dialog.h"
#include "xfce_support.h"
#include "module.h"
#include "item.h"
#include "side.h"
#include "popup.h"

/*  item_dialog.c
 *  -------------
 *  Defines two types of configuration dialogs: for panel item and for 
 *  menu items.
 *
 *  1) Dialog for changing items on the panel. 
 *  On top is an option menu where you can choose 'Icon' for a 
 *  normal launcher icon or one of the available modules.
 *  Below the separator is the configuration area which is a notebook
 *  page where options are presented.
 *
 *  When the 'Apply' button is pressed, the configuration is applied to
 *  the panel item.
 *
 *  2) Dialogs for changing or adding menu items.
 *  Basically the same as the notebook page for icon panel items. Adds
 *  options for caption and position in menu.
 */

enum
{ RESPONSE_CANCEL, RESPONSE_CHANGE, RESPONSE_REMOVE };

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Globals
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static GtkWidget *dlg;
static GtkWidget *notebook;

static PanelItem *orig_item;
static PanelModule *orig_module;

/* icon config */
static GtkWidget *command_entry;
static GtkWidget *command_browse_button;
static GtkWidget *term_checkbutton;
static GtkWidget *icon_id_menu;
static GtkWidget *icon_entry;
static GtkWidget *icon_browse_button;
static GtkWidget *tip_entry;
static GtkWidget *preview_image;

static char *icon_path = NULL;
static int id_callback;

/* for menu items */
GtkWidget *caption_entry;
GtkWidget *pos_spin;
int num_items = 0;

/* modules */
static GList *modules;
static PanelModule *current_module;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Panel item configuration
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void change_icon(int id, const char *path)
{
    GdkPixbuf *pb = NULL;

    if(EXTERN_ICON && path)
        pb = gdk_pixbuf_new_from_file(path, NULL);
    else
        pb = get_pixbuf_by_id(id);

    if(!pb || !GDK_IS_PIXBUF(pb))
        return;

    gtk_image_set_from_pixbuf(GTK_IMAGE(preview_image), pb);
    g_object_unref(pb);

    if(id == EXTERN_ICON)
    {
        if(path)
        {
            g_free(icon_path);
            icon_path = g_strdup(path);

            gtk_entry_set_text(GTK_ENTRY(icon_entry), path);
        }

        gtk_widget_set_sensitive(icon_entry, TRUE);
        gtk_widget_set_sensitive(icon_browse_button, TRUE);
    }
    else
    {
        gtk_entry_set_text(GTK_ENTRY(icon_entry), "");

        gtk_widget_set_sensitive(icon_entry, FALSE);
        gtk_widget_set_sensitive(icon_browse_button, FALSE);
    }

    g_signal_handler_block(icon_id_menu, id_callback);
    gtk_option_menu_set_history(GTK_OPTION_MENU(icon_id_menu),
                                (id == EXTERN_MODULE) ? 0 : id);
    g_signal_handler_unblock(icon_id_menu, id_callback);
}

/* icon option menu */
static void icon_id_changed(void)
{
    int new_id = gtk_option_menu_get_history(GTK_OPTION_MENU(icon_id_menu));

    if(new_id == 0)
    {
        change_icon(EXTERN_ICON, icon_path);
        gtk_widget_set_sensitive(icon_entry, TRUE);
        gtk_widget_set_sensitive(icon_browse_button, TRUE);
    }
    else
    {
        change_icon(new_id, NULL);
    }
}

static GtkWidget *create_icon_option_menu(void)
{
    GtkWidget *om;
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

    om = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    id_callback =
        g_signal_connect_swapped(om, "changed", G_CALLBACK(icon_id_changed), NULL);

    return om;
}

/* icon options */
static void command_browse_cb(GtkWidget * b, GtkEntry * entry)
{
    char *file =
        select_file_name(_("Select command"), gtk_entry_get_text(entry), dlg);

    if(file)
    {
        gtk_entry_set_text(entry, file);
        g_free(file);
    }
}

static void icon_browse_cb(GtkWidget * b, GtkEntry * entry)
{
    char *file = select_file_name(_("Select icon"), gtk_entry_get_text(entry), dlg);

    if(file)
    {
        change_icon(EXTERN_ICON, file);
        g_free(file);
    }
}

gboolean icon_entry_lost_focus(GtkEntry *entry, GdkEventFocus *event, gpointer data)
{
	const char *temp = gtk_entry_get_text(entry);
	
	if (temp)
		change_icon(EXTERN_ICON, temp);
	
	/* we must return FALSE or gtk will crash :-( */
	return FALSE;
}

static GtkWidget *create_icon_options_box(gboolean is_menu_item)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    /* command */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    command_entry = gtk_entry_new();
    gtk_widget_show(command_entry);
    gtk_box_pack_start(GTK_BOX(hbox), command_entry, TRUE, TRUE, 0);

    command_browse_button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(command_browse_button);
    gtk_box_pack_start(GTK_BOX(hbox), command_browse_button, FALSE, FALSE, 0);

    g_signal_connect(command_browse_button, "clicked", G_CALLBACK(command_browse_cb),
                     command_entry);

    /* terminal */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    term_checkbutton = gtk_check_button_new_with_mnemonic(_("Run in _terminal"));
    gtk_widget_show(term_checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), term_checkbutton, FALSE, FALSE, 0);

    /* icon */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Icon:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    icon_id_menu = create_icon_option_menu();
    gtk_widget_show(icon_id_menu);
    gtk_box_pack_start(GTK_BOX(hbox), icon_id_menu, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    icon_entry = gtk_entry_new();
    gtk_widget_show(icon_entry);
    gtk_box_pack_start(GTK_BOX(hbox), icon_entry, TRUE, TRUE, 0);

	g_signal_connect(icon_entry, "focus-out-event", G_CALLBACK(icon_entry_lost_focus),
                     NULL);


    icon_browse_button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(icon_browse_button);
    gtk_box_pack_start(GTK_BOX(hbox), icon_browse_button, FALSE, FALSE, 0);

    g_signal_connect(icon_browse_button, "clicked", G_CALLBACK(icon_browse_cb),
                     icon_entry);

    /* caption (menu item) */
    if(is_menu_item)
    {
        hbox = gtk_hbox_new(FALSE, 4);
        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        label = gtk_label_new(_("Caption:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_size_group_add_widget(sg, label);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        caption_entry = gtk_entry_new();
        gtk_widget_show(caption_entry);
        gtk_box_pack_start(GTK_BOX(hbox), caption_entry, TRUE, TRUE, 0);
    }

    /* tooltip */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Tooltip:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    tip_entry = gtk_entry_new();
    gtk_widget_show(tip_entry);
    gtk_box_pack_start(GTK_BOX(hbox), tip_entry, TRUE, TRUE, 0);

    /* position (menu item) */
    if(is_menu_item && num_items > 1)
    {
        hbox = gtk_hbox_new(FALSE, 4);
        gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

        label = gtk_label_new(_("Position:"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
        gtk_size_group_add_widget(sg, label);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        pos_spin = gtk_spin_button_new_with_range(1, num_items, 1);
        gtk_widget_show(pos_spin);
        gtk_box_pack_start(GTK_BOX(hbox), pos_spin, FALSE, FALSE, 0);
    }

    return vbox;
}

/* icon preview */
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

        change_icon(EXTERN_ICON, icon);
    }

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

static GtkWidget *create_icon_preview_frame()
{
    GtkWidget *frame;
    GtkWidget *eventbox;

    frame = gtk_frame_new(_("Icon Preview"));
    gtk_widget_show(frame);

    eventbox = gtk_event_box_new();
    add_tooltip(eventbox, _("Drag file onto this frame to change the icon"));
    gtk_widget_show(eventbox);
    gtk_container_add(GTK_CONTAINER(frame), eventbox);

    preview_image = gtk_image_new();
    gtk_widget_show(preview_image);
    gtk_container_add(GTK_CONTAINER(eventbox), preview_image);

    /* signals */
    dnd_set_drag_dest(eventbox);

    g_signal_connect(eventbox, "drag_data_received", G_CALLBACK(icon_drop_cb), NULL);

    return frame;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Panel modules
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static GList *find_all_modules(void)
{
    GList *ml = NULL;
    GList *names = NULL;
    int i;
    PanelModule *pm;
    char **dirs, **d;

    for(i = 0; i < BUILTIN_MODULES; i++)
    {
        pm = panel_module_new(NULL);
        pm->id = i;
        create_panel_module(pm);

        ml = g_list_append(ml, pm);
    }

    dirs = get_plugin_dirs();

    for(d = dirs; *d; d++)
    {
        GDir *gdir = g_dir_open(*d, 0, NULL);
        const char *file;

        if(!gdir)
            continue;

        while((file = g_dir_read_name(gdir)))
        {
            char *s = strrchr(file, '.');

            s++;
            if(!strequal(s, G_MODULE_SUFFIX))
                continue;

            pm = panel_module_new(NULL);
            pm->id = EXTERN_MODULE;
            pm->name = g_strdup(file);
            pm->dir = g_strdup(*d);

            if(!create_panel_module(pm))
            {
                panel_module_free(pm);
                continue;
            }

            if(g_list_find_custom(names, pm->caption, (GCompareFunc) strcmp))
                continue;
            names = g_list_append(names, pm->caption);
            ml = g_list_append(ml, pm);
        }
    }

    g_strfreev(dirs);
    return ml;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Option menu
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void add_icon_item(GtkMenuShell * menu)
{
    GtkWidget *mi = gtk_menu_item_new_with_label("Icon");

    gtk_widget_show(mi);
    gtk_menu_shell_append(menu, mi);
}

static void add_module_items(GtkMenuShell * menu)
{
    GList *li;
    GtkWidget *mi;
    PanelModule *pm;

    for(li = modules; li; li = li->next)
    {
        pm = (PanelModule *) li->data;
        mi = gtk_menu_item_new_with_label(pm->caption);
        gtk_widget_show(mi);
        gtk_menu_shell_append(menu, mi);
    }
}

static void option_changed(GtkOptionMenu * om)
{
    int n = gtk_option_menu_get_history(om);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), n);

    if(n > 0)
    {
        GList *li = g_list_nth(modules, n - 1);

        current_module = (PanelModule *) li->data;
    }
    else
        current_module = NULL;
}

static GtkWidget *create_option_menu(void)
{
    GtkWidget *om = gtk_option_menu_new();
    GtkWidget *menu = gtk_menu_new();

    add_icon_item(GTK_MENU_SHELL(menu));

    add_module_items(GTK_MENU_SHELL(menu));

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    g_signal_connect(om, "changed", G_CALLBACK(option_changed), NULL);

    return om;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Notebook
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void add_icon_page(GtkNotebook * nb)
{
    GtkWidget *frame;
    GtkWidget *main_hbox;
    GtkWidget *options_box;
    GtkWidget *preview_frame;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
    gtk_widget_show(frame);

    main_hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(main_hbox);
    gtk_container_add(GTK_CONTAINER(frame), main_hbox);

    options_box = create_icon_options_box(FALSE);
    gtk_box_pack_start(GTK_BOX(main_hbox), options_box, TRUE, TRUE, 0);

    preview_frame = create_icon_preview_frame();
    gtk_box_pack_start(GTK_BOX(main_hbox), preview_frame, TRUE, FALSE, 0);

    if(orig_item)
    {
        gtk_entry_set_text(GTK_ENTRY(command_entry), orig_item->command);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                     orig_item->in_terminal);

        change_icon(orig_item->id, orig_item->path);

        gtk_entry_set_text(GTK_ENTRY(tip_entry), orig_item->tooltip);
    }

    gtk_notebook_append_page(nb, frame, NULL);
}

static void add_module_pages(GtkNotebook * nb)
{
    GtkWidget *frame;
    GList *li;

    for(li = modules; li; li = li->next)
    {
        PanelModule *pm = (PanelModule *) li->data;

        frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
        gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
        gtk_widget_show(frame);

		panel_module_add_options(pm, GTK_CONTAINER(frame));

        gtk_notebook_append_page(nb, frame, NULL);
    }
}

static GtkWidget *create_notebook(void)
{
    GtkWidget *notebook = gtk_notebook_new();

    gtk_widget_show(notebook);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    add_icon_page(GTK_NOTEBOOK(notebook));
    add_module_pages(GTK_NOTEBOOK(notebook));

    return notebook;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Panel group dialog
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static GtkWidget *create_panel_group_dialog(void)
{
    GtkWidget *dlg;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *option_menu;
    GtkWidget *separator;
    GtkWidget *main_vbox;

    dlg =
        gtk_dialog_new_with_buttons(_("Change item"), GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL, GTK_STOCK_CANCEL,
                                    RESPONSE_CANCEL, GTK_STOCK_APPLY,
                                    RESPONSE_CHANGE, NULL);

    main_vbox = GTK_DIALOG(dlg)->vbox;

    /* option menu */
    hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Item type:"));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    option_menu = create_option_menu();
    gtk_widget_show(option_menu);
    gtk_box_pack_start(GTK_BOX(hbox), option_menu, FALSE, FALSE, 0);

    /* separator */
    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(main_vbox), separator, FALSE, FALSE, 0);

    /* notebook */
    notebook = create_notebook();
    gtk_box_pack_start(GTK_BOX(main_vbox), notebook, FALSE, FALSE, 0);

    /* set correct notebook page */
    if(orig_module)
    {
        GList *li;
        int n;

        /* index 0 is icon item, modules start at 1 */
        for(n = 1, li = modules; li; n++, li = li->next)
        {
            PanelModule *pm = (PanelModule *) li->data;
            if(strequal(pm->caption, orig_module->caption))
            {
                gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), n);
                break;
            }
        }
    }

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    return dlg;
}

void edit_panel_group_dialog(PanelGroup * pg)
{
    PanelItem *pi;
    PanelModule *pm;
    int response;

    orig_item = pi = pg->item;
    orig_module = pm = pg->module;

    modules = find_all_modules();

    dlg = create_panel_group_dialog();

    response = GTK_RESPONSE_NONE;
    response = gtk_dialog_run(GTK_DIALOG(dlg));

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

        if(!current_module)
        {
            const char *temp;

            pg->type = ICON;
            pi = pg->item = panel_item_new(pg);

            pi->command = gtk_editable_get_chars(GTK_EDITABLE(command_entry), 0, -1);
            pi->in_terminal =
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));
            pi->tooltip = gtk_editable_get_chars(GTK_EDITABLE(tip_entry), 0, -1);
            pi->id = gtk_option_menu_get_history(GTK_OPTION_MENU(icon_id_menu));
            if(pi->id == 0)
                pi->id = EXTERN_MODULE;

            temp = gtk_entry_get_text(GTK_ENTRY(icon_entry));
            if(temp && strlen(temp))
                pi->path = g_strdup(temp);

            create_panel_item(pi);
            panel_item_pack(pi, GTK_BOX(pg->vbox));

            panel_item_set_size(pi, settings.size);
        }
        else
        {
            panel_module_apply_configuration(current_module);

            pg->type = MODULE;
            pm = pg->module = panel_module_new(pg);

            pm->id = current_module->id;
            pm->name = current_module->name ? g_strdup(current_module->name) : NULL;

            create_panel_module(pm);
            panel_module_pack(pm, GTK_BOX(pg->vbox));

            panel_module_set_size(pm, settings.size);
            panel_module_set_style(pm, settings.style);

            panel_module_start(pm);
        }
    }

    /* clean up */
    gtk_widget_destroy(dlg);
    dlg = NULL;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Menu item dialogs
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
GtkWidget *create_menu_item_dialog(MenuItem * mi)
{
    char *title;
    GtkWidget *dlg;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *main_vbox;
    GtkWidget *frame;
    GtkWidget *main_hbox;
    GtkWidget *options_box;
    GtkWidget *preview_frame;

    if(mi)
    {
        title = _("Change item");

        dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(toplevel),
                                          GTK_DIALOG_MODAL, GTK_STOCK_CANCEL,
                                          RESPONSE_CANCEL, GTK_STOCK_REMOVE,
                                          RESPONSE_REMOVE, GTK_STOCK_APPLY,
                                          RESPONSE_CHANGE, NULL);
    }
    else
    {
        title = _("Add item");

        dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(toplevel),
                                          GTK_DIALOG_MODAL, GTK_STOCK_CANCEL,
                                          RESPONSE_CANCEL, GTK_STOCK_APPLY,
                                          RESPONSE_CHANGE, NULL);
    }

	gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
	
    main_vbox = GTK_DIALOG(dlg)->vbox;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

    main_hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(main_hbox);
    gtk_container_add(GTK_CONTAINER(frame), main_hbox);

    options_box = create_icon_options_box(TRUE);
    gtk_box_pack_start(GTK_BOX(main_hbox), options_box, TRUE, TRUE, 0);

    preview_frame = create_icon_preview_frame();
    gtk_box_pack_start(GTK_BOX(main_hbox), preview_frame, TRUE, FALSE, 0);

    if(mi)
    {
		if (mi->command)
			gtk_entry_set_text(GTK_ENTRY(command_entry), mi->command);
		
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                     mi->in_terminal);

        change_icon(mi->icon_id, mi->icon_path);

		if (mi->caption)
			gtk_entry_set_text(GTK_ENTRY(caption_entry), mi->caption);
		
		if (mi->tooltip)
			gtk_entry_set_text(GTK_ENTRY(tip_entry), mi->tooltip);

        if(num_items > 1)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(pos_spin),
                                      (gfloat) mi->pos + 1);
    }

    return dlg;
}

static void reposition_popup(PanelPopup * pp)
{
    if(pp->detached)
        return;

    gtk_button_clicked(GTK_BUTTON(pp->button));
    gtk_button_clicked(GTK_BUTTON(pp->button));
}

static void add_new_menu_item(PanelPopup * pp)
{
    const char *temp;
    MenuItem *mi = menu_item_new(pp);

    mi->command = gtk_editable_get_chars(GTK_EDITABLE(command_entry), 0, -1);
    mi->in_terminal =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));

    mi->caption = gtk_editable_get_chars(GTK_EDITABLE(caption_entry), 0, -1);
    mi->tooltip = gtk_editable_get_chars(GTK_EDITABLE(tip_entry), 0, -1);

    mi->icon_id = gtk_option_menu_get_history(GTK_OPTION_MENU(icon_id_menu));
    if(mi->icon_id == 0)
        mi->icon_id = EXTERN_MODULE;

    temp = gtk_entry_get_text(GTK_ENTRY(icon_entry));
    if(temp && strlen(temp))
        mi->icon_path = g_strdup(temp);

    if(num_items > 1)
        mi->pos = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pos_spin)) - 1;

    create_menu_item(mi);

    gtk_box_pack_start(GTK_BOX(pp->vbox), mi->button, TRUE, TRUE, 0);
    gtk_box_reorder_child(GTK_BOX(pp->vbox), mi->button, mi->pos + 2);

    pp->items = g_list_insert(pp->items, mi, mi->pos);

    menu_item_set_popup_size(mi, settings.popup_size);
    menu_item_set_style(mi, settings.style);

    reposition_popup(pp);
}

void edit_menu_item_dialog(MenuItem * mi)
{
    int response;

    num_items = g_list_length(mi->parent->items);

    dlg = create_menu_item_dialog(mi);

    response = GTK_RESPONSE_NONE;
    response = gtk_dialog_run(GTK_DIALOG(dlg));

    if(response == RESPONSE_CHANGE || response == RESPONSE_REMOVE)
    {
        PanelPopup *pp = mi->parent;

        gtk_container_remove(GTK_CONTAINER(pp->vbox), mi->button);
        pp->items = g_list_remove(pp->items, mi);
        menu_item_free(mi);

        if(response == RESPONSE_CHANGE)
            add_new_menu_item(pp);
		else
			reposition_popup(pp);
    }

    gtk_widget_destroy(dlg);
    num_items = 0;
}

void add_menu_item_dialog(PanelPopup * pp)
{
    int response;

    num_items = g_list_length(pp->items);

    dlg = create_menu_item_dialog(NULL);

    response = GTK_RESPONSE_NONE;
    response = gtk_dialog_run(GTK_DIALOG(dlg));

    if(response == RESPONSE_CHANGE)
    {
        add_new_menu_item(pp);
    }

    gtk_widget_destroy(dlg);
    num_items = 0;
}
