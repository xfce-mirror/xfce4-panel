/*  controls_dialog.h
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

/*  panel control dialog
 *  --------------------
 *  The dialog consists of three parts:
 *  - Option menu to choose the type of control (icon or one of the available
 *    modules);
 *  - Notebook containing the options that can be changed. This is provided
 *    by the panel controls. Changes must auto-apply if possible. 
 *  - Buttons: 'Revert' and 'Done'
 *    
 *  Important data are kept as global variables for easy access.
*/

#include "global.h"
#include "controls_dialog.h"
#include "controls.h"
#include "xfce_support.h"
#include "xfce.h"
#include "settings.h"
#include "side.h"

static GtkContainer *container;        /* container on the panel to hold the 
                                   panel control */
static GList *controls = NULL;         /* list of all available controls */
static PanelControl *current_pc = NULL;
static int current_index = 0;
static GtkWidget *type_option_menu;
static GtkWidget *pos_spin;
static GtkWidget *notebook;
static GtkWidget *done;
static GtkWidget *revert;

/*  Global controls list
 *  --------------------
 *  Make a list of all available panel controls
 *  starting with the current one
*/
void create_controls_list(PanelControl * pc)
{
    PanelControl *new_pc;
    GList *names = NULL;
    char **dirs, **d;
    int i;

    controls = NULL;
    current_pc = pc;

    /* add the original control as first in the list */
    controls = g_list_append(controls, pc);

    /* most common control */
    if(pc->id != ICON)
    {
        new_pc = panel_control_new(pc->index);
        create_panel_control(new_pc);

        controls = g_list_append(controls, new_pc);
    }

    /* builtin modules */
    for(i = 0; i < NUM_BUILTINS; i++)
    {
        if(i == pc->id)
            continue;

        new_pc = panel_control_new(pc->index);
        new_pc->id = i;

        create_panel_control(new_pc);

        if(!new_pc->main || new_pc->id == ICON)
        {
            panel_control_free(new_pc);
            continue;
        }

        controls = g_list_append(controls, new_pc);
    }

    /* Plugins. We identify them by caption; that should be sufficient. */
    dirs = get_plugin_dirs();

    if(pc->id == PLUGIN)
        names = g_list_append(names, pc->caption);

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

            new_pc = panel_control_new(pc->index);
            new_pc->id = PLUGIN;
            new_pc->filename = g_strdup(file);
            new_pc->dir = g_strdup(*d);

            create_panel_control(new_pc);

            if(!new_pc->main || new_pc->id == ICON)
            {
                panel_control_free(new_pc);
                continue;
            }

            if(g_list_find_custom
               (names, new_pc->caption, (GCompareFunc) strcmp))
            {
                panel_control_free(new_pc);
                continue;
            }

            names = g_list_append(names, new_pc->caption);
            controls = g_list_append(controls, new_pc);
        }
    }

    g_strfreev(dirs);
}

void clear_controls_list(void)
{
    GList *current;
    PanelControl *pc;

    /* remove current control from the list */
    controls = g_list_remove(controls, current_pc);

    /* free all other controls */
    for(current = controls; current; current = current->next)
    {
        pc = (PanelControl *) current->data;

        if(pc)
            panel_control_free(pc);
    }

    controls = NULL;
    current_pc = NULL;
    current_index = 0;
}

/*  Type options menu
 *  -----------------
*/
static void type_option_changed(GtkOptionMenu * om)
{
    int n = gtk_option_menu_get_history(om);
    GList *li;
    PanelControl *new_pc;

    li = g_list_nth(controls, n);
    new_pc = (PanelControl *) li->data;

    /* this may have changed */
    new_pc->index = current_pc->index;
    
    panel_control_unpack(current_pc);
    panel_control_pack(new_pc, GTK_BOX(container));
    side_panel_register_control(new_pc); 
    container = new_pc->container;

    current_pc = new_pc;
    current_index = n;

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), n);
}

static GtkWidget *create_type_option_menu(void)
{
    GtkWidget *om;
    GtkWidget *menu, *mi;
    GList *li;
    PanelControl *pc;

    om = gtk_option_menu_new();
    menu = gtk_menu_new();

    for(li = controls; li; li = li->next)
    {
        pc = (PanelControl *) li->data;

        mi = gtk_menu_item_new_with_label(pc->caption);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    g_signal_connect(om, "changed", G_CALLBACK(type_option_changed), NULL);

    return om;
}

/*  Notebook containing all options
 *  -------------------------------
 *  Every panel control adds its options to a notebook page.
*/
static void add_notebook(GtkBox * box)
{
    GList *li;
    PanelControl *pc;
    GtkWidget *frame;

    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);

    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    for(li = controls; li; li = li->next)
    {
        pc = (PanelControl *) li->data;

        frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
        gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
        gtk_widget_show(frame);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, NULL);

        panel_control_add_options(pc, GTK_CONTAINER(frame), revert, done);
    }

    gtk_box_pack_start(box, notebook, TRUE, TRUE, 0);
}

/*  The main dialog
 *  ---------------
*/

static int backup_pos;

static void revert_pos(GtkWidget *w)
{
    if (backup_pos == current_pc->index)
	return;

    side_panel_move(current_pc->index, backup_pos);
}

static void pos_changed(GtkSpinButton *spin)
{
    int n;
    gboolean changed = FALSE;
    
    n = gtk_spin_button_get_value_as_int(spin);

    if (n == current_pc->index)
	return;

    side_panel_move(current_pc->index, n);

    gtk_widget_set_sensitive(revert, TRUE);
}

void change_panel_control_dialog(PanelControl * pc)
{
    GtkWidget *dlg;
    GtkWidget *button;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *separator;
    GtkWidget *main_vbox;
    int response;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    /* Keep track of the panel container */
    container = pc->container;

    dlg = gtk_dialog_new_with_buttons(_("Change item"), GTK_WINDOW(toplevel),
                                      GTK_DIALOG_MODAL, NULL);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);

    revert = button = mixed_button_new(GTK_STOCK_UNDO, _("Revert"));
    gtk_widget_show(button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_CANCEL);

    done = button = mixed_button_new(GTK_STOCK_OK, _("Done"));
    gtk_widget_show(button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), button, GTK_RESPONSE_OK);

    main_vbox = GTK_DIALOG(dlg)->vbox;

    create_controls_list(pc);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(main_vbox), vbox, FALSE, FALSE, 0);
    
    /* option menu */
    hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox);

    label = gtk_label_new(_("Type:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    type_option_menu = create_type_option_menu();
    gtk_widget_show(type_option_menu);
    gtk_box_pack_start(GTK_BOX(hbox), type_option_menu, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    /* position */
    hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(hbox);

    label = gtk_label_new(_("Position:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.1, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    backup_pos = pc->index;
    pos_spin = gtk_spin_button_new_with_range(0, settings.num_groups, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pos_spin), pc->index);
    gtk_widget_show(pos_spin);
    gtk_box_pack_start(GTK_BOX(hbox), pos_spin, FALSE, FALSE, 0);
    
    g_signal_connect(pos_spin, "value-changed", G_CALLBACK(pos_changed), NULL);
    g_signal_connect(revert, "clicked", G_CALLBACK(revert_pos), NULL);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    /* separator */
    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(main_vbox), separator, FALSE, FALSE, 0);

    /* notebook */
    add_notebook(GTK_BOX(main_vbox));

    /* run dialog until 'Done' */
    while(1)
    {
        int response = GTK_RESPONSE_NONE;

        gtk_widget_set_sensitive(revert, FALSE);

        response = gtk_dialog_run(GTK_DIALOG(dlg));

        if(response != GTK_RESPONSE_CANCEL)
            break;

        gtk_option_menu_set_history(GTK_OPTION_MENU(type_option_menu), 0);
    }

    gtk_widget_destroy(dlg);

    clear_controls_list();

    write_panel_config();
}
