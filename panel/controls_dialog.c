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
 *  - Container for the options that can be changed. The options are provided
 *    by the panel controls. Changes must auto-apply if possible. 
 *  - Buttons: 'Remove' and 'Close'
 *    
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

#define BORDER 6

static GtkWidget *cdialog = NULL;

/* container for control options */

static void
add_container (GtkBox * box, GtkWidget *close, Control *control)
{
    GtkWidget *align;

    align = gtk_alignment_new (0,0,0,0);
    gtk_widget_show (align);
    gtk_container_set_border_width (GTK_CONTAINER (align), BORDER);
    gtk_box_pack_start (box, align, TRUE, TRUE, 0);

    control_create_options (control, GTK_CONTAINER (align), close);
}

/* position */

static void
pos_changed (GtkSpinButton * spin, Control *control)
{
    int n;

    n = gtk_spin_button_get_value_as_int (spin) - 1;

    if (n != control->index)
    {
	groups_move (control->index, n);
	control->index = n;
    }
}

static void
add_position_option (GtkBox *box, Control *control)
{
    GtkWidget *pos_spin, *hbox, *label;
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    if (settings.num_groups > 1)
    {
	pos_spin = gtk_spin_button_new_with_range (1, settings.num_groups, 1);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (pos_spin), 
				   control->index + 1);

	g_signal_connect (pos_spin, "value-changed", 
			  G_CALLBACK (pos_changed), control);
    }
    else
    {
	pos_spin = gtk_label_new ("1");
    }

    gtk_widget_show (pos_spin);
    gtk_box_pack_start (GTK_BOX (hbox), pos_spin, FALSE, FALSE, 0);
}

/* main dialog */

static void add_spacer (GtkBox *box, int size)
{
    GtkWidget *align;

    align = gtk_alignment_new (0,0,0,0);
    gtk_widget_set_size_request (align, size, size);
    gtk_widget_show (align);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}

void
controls_dialog (Control * control)
{
    int response;
    GtkWidget *button, *close, *header, **ptr;
    GtkDialog *dlg;

    if (cdialog)
    {
	gtk_window_present (GTK_WINDOW (cdialog));
	return;
    }

    cdialog = gtk_dialog_new ();
    dlg = GTK_DIALOG (cdialog);

    /* keep gcc3 happy -- warns about this: (gpointer *)&cdialog; */
    ptr = &cdialog;
    g_object_add_weak_pointer (G_OBJECT (cdialog), (gpointer *)ptr);

    gtk_dialog_set_has_separator (dlg, FALSE);

    gtk_window_set_title (GTK_WINDOW (dlg), _("Change item"));

    button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (dlg, button, GTK_RESPONSE_CANCEL);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (dlg->action_area),
					button, TRUE);

    close = button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (dlg, button, GTK_RESPONSE_OK);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

    header = create_header (NULL, control->cclass->caption);
    gtk_container_set_border_width (GTK_CONTAINER (GTK_BIN (header)->child), 
	    			    BORDER);
    gtk_widget_set_size_request (header, -1, 32);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (dlg->vbox), header, FALSE, TRUE, 0);

    add_spacer (GTK_BOX (dlg->vbox), BORDER);
    
    /* position */
    add_position_option (GTK_BOX (dlg->vbox), control);

    add_spacer (GTK_BOX (dlg->vbox), BORDER);

    /* container */
    add_container (GTK_BOX (dlg->vbox), close, control);

    add_spacer (GTK_BOX (dlg->vbox), BORDER);

    /* run dialog until 'Close' or 'Remove' */
retry:
    response = GTK_RESPONSE_NONE;

    gtk_widget_grab_default (close);
    gtk_widget_grab_focus (close);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    response = gtk_dialog_run (dlg);

    if (response == GTK_RESPONSE_CANCEL)
    {
	PanelPopup *pp;

	gtk_widget_hide (cdialog);

	pp = groups_get_popup (control->index);

	if (!(control->with_popup) || !pp || pp->items == NULL ||
	    confirm (_("Removing the item will also remove its popup menu."),
		     GTK_STOCK_REMOVE, NULL))
	{
	    groups_remove (control->index);
	}
	else
	{
	    goto retry;
	}
    }

    gtk_widget_destroy (cdialog);

    write_panel_config ();
}

void
destroy_controls_dialog (void)
{
    if (!cdialog)
	return;

    gtk_dialog_response (GTK_DIALOG (cdialog), GTK_RESPONSE_OK);
}
