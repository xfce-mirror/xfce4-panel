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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/i18n.h>

#include "xfce.h"
#include "controls_dialog.h"
#include "groups.h"
#include "popup.h"
#include "settings.h"

static GSList *control_list = NULL;     /* list of available controls */

static GtkWidget *container;    /* container on the panel to hold the 
                                   panel control */
static Control *old_control = NULL;     /* original panel control */
static Control *current_control = NULL; /* current control == old_control, 
                                           if type is not changed */
static GtkWidget *type_option_menu;
static GtkWidget *pos_spin;
static GtkWidget *notebook;
static GtkWidget *done;
static GtkWidget *revert;

static int backup_index;

/* control control list */
static void
create_control_list (Control * control)
{
    control_list = g_slist_append (control_list, control);
#if 0
    int i;
    GSList *li, *class_list;

    class_list = get_control_class_list ();

    /* first the original control */
    control_list = g_slist_append (control_list, control);

    /* then one for each other control class */
    for (i = 0, li = class_list; li; li = li->next, i++)
    {
        ControlClass *cc = li->data;
        Control *new_control;

        if (cc == control->cclass)
            continue;

        new_control = control_new (control->index);
        new_control->cclass = cc;
        
	if (!cc->create_control (new_control))
	{
	    new_control->cclass = NULL;
	    control_free(new_control);
	    continue;
	}	

        control_attach_callbacks (new_control);
        control_set_settings (new_control);

        control_list = g_slist_append (control_list, new_control);
    }
#endif
}

static void
clear_control_list (void)
{
    GSList *li;

    /* first remove the current control */
    control_list = g_slist_remove (control_list, current_control);

    for (li = control_list; li; li = li->next)
    {
        Control *control = li->data;

        control_free (control);
    }

    g_slist_free (control_list);
    control_list = NULL;
}

/*  Type options menu
 *  -----------------
*/
static void
type_option_changed (GtkOptionMenu * om)
{
    int n = gtk_option_menu_get_history (om);
    GSList *li;
    Control *control = NULL;

    li = g_slist_nth (control_list, n);
    control = li->data;

    if (control == current_control)
        return;

    control_unpack (current_control);
    control_pack (control, GTK_BOX (container));

    control->index = current_control->index;
    groups_register_control (control);

    container = control->base->parent;
    current_control = control;

    gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), n);

    gtk_widget_set_sensitive (revert, TRUE);
}

static GtkWidget *
create_type_option_menu (void)
{
    GtkWidget *om;
    GtkWidget *menu, *mi;
    GSList *li;
    Control *control;

    om = gtk_option_menu_new ();
    menu = gtk_menu_new ();

    for (li = control_list; li; li = li->next)
    {
        control = li->data;

        mi = gtk_menu_item_new_with_label (control->cclass->caption);
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    gtk_option_menu_set_menu (GTK_OPTION_MENU (om), menu);

    g_signal_connect (om, "changed", G_CALLBACK (type_option_changed), NULL);

    return om;
}

static void
add_notebook (GtkBox * box)
{
    GSList *li;
    GtkWidget *frame;

    notebook = gtk_notebook_new ();
    gtk_widget_show (notebook);

    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

    /* add page for every control in control_list */
    for (li = control_list; li; li = li->next)
    {
        Control *control = li->data;

        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
        gtk_widget_show (frame);
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, NULL);

        control_add_options (control, GTK_CONTAINER (frame), revert, done);
    }

    gtk_box_pack_start (box, notebook, TRUE, TRUE, 0);
}

/*  The main dialog
 *  ---------------
*/
static void
pos_changed (GtkSpinButton * spin)
{
    int n;
    gboolean changed = FALSE;

    n = gtk_spin_button_get_value_as_int (spin) - 1;

    if (n != current_control->index)
    {
        groups_move (current_control->index, n);
        current_control->index = n;

        changed = TRUE;
    }

    if (changed)
        gtk_widget_set_sensitive (revert, TRUE);
}

static void
controls_dialog_revert (void)
{
#if 0
    gtk_option_menu_set_history (GTK_OPTION_MENU (type_option_menu), 0);
#endif
    if (backup_index != current_control->index)
    {
        groups_move (current_control->index, backup_index);

        gtk_spin_button_set_value (GTK_SPIN_BUTTON (pos_spin),
                                   backup_index + 1);
    }
}

enum
{ RESPONSE_DONE, RESPONSE_REVERT, RESPONSE_REMOVE };

void
controls_dialog (Control * control)
{
    GtkWidget *dlg;
    GtkWidget *button;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *separator;
    GtkWidget *main_vbox;
    int response;
    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    old_control = current_control = control;
    backup_index = control->index;

    /* Keep track of the panel container */
    container = control->base->parent;

    dlg = gtk_dialog_new_with_buttons (_("Change item"), NULL,
                                       GTK_DIALOG_MODAL, NULL);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

    button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), button, RESPONSE_REMOVE);

    revert = button = mixed_button_new (GTK_STOCK_UNDO, _("_Revert"));
    GTK_WIDGET_SET_FLAGS (revert, GTK_CAN_DEFAULT);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), button, RESPONSE_REVERT);

    done = button = mixed_button_new (GTK_STOCK_OK, _("_Done"));
    GTK_WIDGET_SET_FLAGS (done, GTK_CAN_DEFAULT);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), button, RESPONSE_DONE);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    g_signal_connect (revert, "clicked",
                      G_CALLBACK (controls_dialog_revert), NULL);

    main_vbox = GTK_DIALOG (dlg)->vbox;

    vbox = gtk_vbox_new (FALSE, 7);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

    /* find all available controls */
    create_control_list (control);
#if 0

    /* option menu */
    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Type:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    type_option_menu = create_type_option_menu ();
    gtk_widget_show (type_option_menu);
    gtk_box_pack_start (GTK_BOX (hbox), type_option_menu, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
#endif
    /* position */
    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.1, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    pos_spin = gtk_spin_button_new_with_range (1, settings.num_groups, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (pos_spin), backup_index + 1);
    gtk_widget_show (pos_spin);
    gtk_box_pack_start (GTK_BOX (hbox), pos_spin, FALSE, FALSE, 0);

    g_signal_connect (pos_spin, "value-changed", G_CALLBACK (pos_changed),
                      NULL);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    /* separator */
    separator = gtk_hseparator_new ();
    gtk_widget_show (separator);
    gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, FALSE, 0);

    /* notebook */
    add_notebook (GTK_BOX (main_vbox));

    /* run dialog until 'Done' */
    while (1)
    {
        response = GTK_RESPONSE_NONE;

        gtk_widget_set_sensitive (revert, FALSE);
        gtk_widget_grab_default (done);
        gtk_widget_grab_focus (done);

        response = gtk_dialog_run (GTK_DIALOG (dlg));

        if (response == RESPONSE_REMOVE)
        {
	    PanelPopup *popup;
	    
            gtk_widget_hide (dlg);

	    popup = groups_get_popup(control->index);

            if (!(control->with_popup) || popup->items == NULL ||
                confirm (_
                         ("Removing an item will also remove its popup menu.\n\n"
                          "Do you want to remove the item?"), GTK_STOCK_REMOVE,
                         NULL))
            {
                break;
            }

            gtk_widget_show (dlg);
        }
        else if (response != RESPONSE_REVERT)
        {
            break;
        }
    }

    gtk_widget_destroy (dlg);

    clear_control_list ();

    if (response == RESPONSE_REMOVE)
    {
        groups_remove (current_control->index);
    }

    write_panel_config ();
}

/*  Add new control
 *  ---------------
*/
static GtkWidget *newcontrol_treeview = NULL;
GSList *class_list = NULL;

static void
create_class_store(GtkListStore *store)
{
    GSList *li;
    GtkTreeIter iter;
    
    class_list = get_control_class_list();

    for (li = class_list; li; li = li->next)
    {
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, li->data, -1);
    }
}

static void
render_class_name (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
		   GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
    ControlClass *cc;

    gtk_tree_model_get(tree_model, iter, 0, &cc, -1);

    g_object_set(cell, "text", cc->caption, NULL);
}

static void 
add_types_treeview(GtkWidget *vbox)
{
    GtkWidget *treeview;
    GtkWidget *treeview_scroll;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeModel *model;
    char *markup;
    GtkWidget *label;

    treeview_scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (treeview_scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (treeview_scroll), 
	    			    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (treeview_scroll),
	    				GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), treeview_scroll, TRUE, TRUE, 0);

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    
    treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    gtk_widget_show(treeview);
    gtk_container_add(GTK_CONTAINER(treeview_scroll), treeview);

    newcontrol_treeview = treeview;

    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview), TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW(treeview), 
	    					-1, "Name", 
	    					renderer, render_class_name,
						NULL, NULL);
    create_class_store(store);
    g_object_unref (G_OBJECT (store));
}

static GtkWidget *
create_add_control_dialog(void)
{
    GtkWidget *dialog, *mainvbox, *header, *label, *vbox, *spacer;
    char *markup;

    dialog = gtk_dialog_new_with_buttons(_("Add item"), NULL,
	    				 GTK_DIALOG_NO_SEPARATOR,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_ADD, GTK_RESPONSE_OK,
					 NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    mainvbox = GTK_DIALOG(dialog)->vbox;
    gtk_container_set_border_width(GTK_CONTAINER(mainvbox), 0);

    header = create_header(NULL, _("Select Item Type"));
    gtk_widget_show(header);
    gtk_box_pack_start(GTK_BOX(mainvbox), header, FALSE, TRUE, 0);
    gtk_widget_set_size_request(header, -1, 36);
    
    spacer = gtk_alignment_new(0,0,0,0);
    gtk_widget_show(spacer);
    gtk_widget_set_size_request(spacer, 12, 12);
    gtk_box_pack_start(GTK_BOX(mainvbox), spacer, FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_widget_show(vbox);
    gtk_box_pack_start(GTK_BOX(mainvbox), vbox, TRUE, TRUE, 0);
    gtk_widget_set_size_request(vbox, -1, 200);

    add_types_treeview(vbox);
    
    label = gtk_label_new(NULL);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    markup = g_strdup_printf("<i>%s</i>", 
	    		     _("Select an item type from the list"));
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);

    spacer = gtk_alignment_new(0,0,0,0);
    gtk_widget_show(spacer);
    gtk_widget_set_size_request(spacer, 12, 12);
    gtk_box_pack_start(GTK_BOX(mainvbox), spacer, FALSE, FALSE, 0);
    
    return dialog;
}

static ControlClass *
get_class_from_treeview(GtkWidget *treeview)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    ControlClass *cc = NULL;

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    if (gtk_tree_selection_get_selected(sel, &model, &iter))
    {
	gtk_tree_model_get(model, &iter, 0, &cc, -1);
    }
    else
    {
	/* return first item */
	cc =  class_list->data;
    }

    return cc;
}

void
controls_add_dialog(int index)
{

    GtkWidget *dlg;
    int response;

    dlg = create_add_control_dialog();

    response = GTK_RESPONSE_NONE;
    response = gtk_dialog_run (GTK_DIALOG(dlg));

    if (response == GTK_RESPONSE_OK)
    {
	Control *newcontrol;
	ControlClass *cc;

	cc = get_class_from_treeview(newcontrol_treeview);
	gtk_widget_destroy(dlg);

	groups_add_control(cc->id, cc->filename, index);
	
	newcontrol = groups_get_control (index >= 0 ? index :
					 settings.num_groups - 1);

	controls_dialog(newcontrol);

	write_panel_config ();
    }
    else
    {
	gtk_widget_destroy(dlg);
    }
}


