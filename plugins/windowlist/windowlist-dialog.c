/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *  Copyright (c) 2006 Jani Monoses <jani@ubuntu.com> 
 *  Copyright (c) 2006 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "windowlist.h"
#include "windowlist-dialog.h"

typedef struct
{
    Windowlist *wl;
    
    GtkWidget *button_layout;
    
    GtkWidget *show_all_workspaces;
    GtkWidget *show_window_icons;
    GtkWidget *show_workspace_actions;
    
    GtkWidget *notify_disabled;
    GtkWidget *notify_other;
    GtkWidget *notify_all;
}
WindowlistDialog;

static void
windowlist_notify_toggled (GtkWidget *button,
			   WindowlistDialog *wd)
{
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	return;
    
    if (button == wd->notify_disabled)
	wd->wl->notify = DISABLED;
    
    else if (button == wd->notify_other)
	wd->wl->notify = OTHER_WORKSPACES;
    
    else if (button == wd->notify_all)
	wd->wl->notify = ALL_WORKSPACES;
    
    windowlist_start_blink (wd->wl);
}

static void
windowlist_button_toggled (GtkWidget *button,
			   WindowlistDialog *wd)
{
    if (button == wd->button_layout)
    {
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	    wd->wl->layout = ARROW_BUTTON;
	else
	    wd->wl->layout = ICON_BUTTON;
	
	windowlist_create_button (wd->wl);
    }
    else if (button == wd->show_all_workspaces)
	wd->wl->show_all_workspaces = 
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    
    else if (button == wd->show_window_icons)
	wd->wl->show_window_icons = 
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    
    else if (button == wd->show_workspace_actions)
	wd->wl->show_workspace_actions = 
	    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
}

static void
windowlist_properties_response (GtkWidget *dialog, 
				int response,
				WindowlistDialog *wd)
{
    gtk_widget_destroy (dialog);
    
    xfce_panel_plugin_unblock_menu (wd->wl->plugin);
    
    g_free (wd);
}

void
windowlist_properties (XfcePanelPlugin *plugin,
		       Windowlist * wl)
{
    WindowlistDialog *wd;
    
    GtkWidget *dialog, *header, *vbox, *vbox2, *frame, *hbox,
	      *alignment, *label, *button, *image;
    
    wd = g_new0 (WindowlistDialog, 1);
    
    wd->wl = wl;
    
    xfce_panel_plugin_block_menu (wd->wl->plugin);
    
    dialog = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);
    gtk_window_set_icon_name (GTK_WINDOW (dialog), GTK_STOCK_PROPERTIES);
    
    header = xfce_create_header (NULL, _("Window List"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), header,
			FALSE, TRUE, 0);
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER - 2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox,
                        TRUE, TRUE, 0);
			
    /* Urgency help */
    hbox = gtk_hbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                      GTK_ICON_SIZE_DND);
    gtk_misc_set_alignment (GTK_MISC (image), 0, 0);
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),
            _("<i>Urgency notification</i> will blink the button when "
              "an application needs attention."));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_set_size_request (label, 230, -1);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

    /* Button Urgency Notification */
    frame = xfce_create_framebox (_("Urgency Notification"), &alignment);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (alignment), vbox2);
    
    button = wd->notify_disabled = 
	gtk_radio_button_new_with_mnemonic (NULL, _("_Disabled"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  (wd->wl->notify == DISABLED));
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_notify_toggled), wd);

    button = wd->notify_other = 
	gtk_radio_button_new_with_mnemonic_from_widget (
                GTK_RADIO_BUTTON (button), _("For _other workspaces"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  (wd->wl->notify == OTHER_WORKSPACES));
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_notify_toggled), wd);
		    
    button = wd->notify_all = 
	gtk_radio_button_new_with_mnemonic_from_widget (
                GTK_RADIO_BUTTON (button), _("For _all workspaces"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  (wd->wl->notify == ALL_WORKSPACES));
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_notify_toggled), wd);
    
    /* Button Layout */
    frame = xfce_create_framebox (_("Appearance"), &alignment);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

    vbox2 = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (alignment), vbox2);
    
    button = gtk_radio_button_new_with_mnemonic (NULL, _("_Icon button"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  (wd->wl->layout == ICON_BUTTON));

    button = wd->button_layout =
	gtk_radio_button_new_with_mnemonic_from_widget (
                GTK_RADIO_BUTTON (button), _("A_rrow button"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  (wd->wl->layout == ARROW_BUTTON));
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_button_toggled), wd);
    
    /* Windowlist Settings */
    frame = xfce_create_framebox (_("Window List"), &alignment);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

    vbox2 = gtk_vbox_new (FALSE, 4);
    gtk_container_add (GTK_CONTAINER (alignment), vbox2);

    button = wd->show_all_workspaces =
	gtk_check_button_new_with_mnemonic (
                _("Show _windows from all workspaces"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  wd->wl->show_all_workspaces);
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_button_toggled), wd);

    button = wd->show_window_icons =
	gtk_check_button_new_with_mnemonic (_("Show a_pplication icons"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  wd->wl->show_window_icons);
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_button_toggled), wd);

    button = wd->show_workspace_actions =
	gtk_check_button_new_with_mnemonic (_("Show wor_kspace actions"));
    gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), 
                                  wd->wl->show_workspace_actions);
    g_signal_connect (button, "toggled",
                      G_CALLBACK (windowlist_button_toggled), wd);

    /* Show Dialog */
    g_signal_connect (dialog, "response", 
                      G_CALLBACK (windowlist_properties_response), wd);
    
    gtk_widget_show_all (dialog);
}
