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
 *  - Spinbox with the position on the panel;
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

static GtkWidget *controlsdialog = NULL;	/* keep track of it for 
						   signal handling */

static GSList *control_list = NULL;	/* list of available controls */

static GtkWidget *container;	/* container on the panel to hold the 
				   panel control */
static Control *old_control = NULL;	/* original panel control */
static Control *current_control = NULL;	/* current control == old_control, 
					   if type is not changed */
static GtkWidget *pos_spin;
static GtkWidget *notebook;
static GtkWidget *done;

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
	    control_free (new_control);
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

#if 0				/* NEVER USED */
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
#endif

#if 0				/* NEVER USED */
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
#endif

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

	control_create_options (control, GTK_CONTAINER (frame), done);
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

    n = gtk_spin_button_get_value_as_int (spin) - 1;

    if (n != current_control->index)
    {
	groups_move (current_control->index, n);
	current_control->index = n;
    }
}

enum
{ RESPONSE_DONE, RESPONSE_REMOVE };

void
controls_dialog (Control * control)
{
    GtkWidget *dlg;
    GtkWidget **dlg_ptr;
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

    controlsdialog = dlg = gtk_dialog_new ();

    dlg_ptr = &controlsdialog;
    g_object_add_weak_pointer (G_OBJECT (dlg), (gpointer *) dlg_ptr);

    gtk_window_set_title (GTK_WINDOW (dlg), _("Change item"));

    button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), button, RESPONSE_REMOVE);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX
					(GTK_DIALOG (dlg)->action_area),
					button, TRUE);

    done = button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), button, RESPONSE_DONE);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

/*    gtk_widget_show(GTK_DIALOG(dlg)->action_area);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (GTK_DIALOG(dlg)->action_area),
                               GTK_BUTTONBOX_END);
*/

    main_vbox = GTK_DIALOG (dlg)->vbox;

    vbox = gtk_vbox_new (FALSE, 7);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

    /* find all available controls */
    create_control_list (control);

    /* position */
    hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.1, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    if (settings.num_groups > 1)
    {
	pos_spin = gtk_spin_button_new_with_range (1, settings.num_groups, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (pos_spin),
				   backup_index + 1);

	g_signal_connect (pos_spin, "value-changed", G_CALLBACK (pos_changed),
			  NULL);
    }
    else
    {
	char postext[2];

	sprintf (postext, "%d", 1);
	pos_spin = gtk_label_new (postext);
    }

    gtk_widget_show (pos_spin);
    gtk_box_pack_start (GTK_BOX (hbox), pos_spin, FALSE, FALSE, 0);

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

	gtk_widget_grab_default (done);
	gtk_widget_grab_focus (done);

	gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
	response = gtk_dialog_run (GTK_DIALOG (dlg));

	if (response == RESPONSE_REMOVE)
	{
	    PanelPopup *popup;

	    gtk_widget_hide (dlg);

	    popup = groups_get_popup (control->index);

	    if (!(control->with_popup) || !popup || popup->items == NULL ||
		confirm (_
			 ("Removing an item will also remove its popup menu.\n\n"
			  "Do you want to remove the item?"),
			 GTK_STOCK_REMOVE, NULL))
	    {
		break;
	    }

	    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
	    gtk_widget_show (dlg);
	}
	else
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

void
destroy_controls_dialog (void)
{
    if (controlsdialog)
	gtk_dialog_response (GTK_DIALOG (controlsdialog), GTK_RESPONSE_OK);
}
