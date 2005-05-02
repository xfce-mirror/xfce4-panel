/*  $Id$
 *
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfcegui4/libxfcegui4.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#define STREQUAL(s1,s2) !strcmp(s1, s2)
#define BORDER 6

enum
{ LEFT, RIGHT, TOP, BOTTOM };

enum
{ HORIZONTAL, VERTICAL };

static McsManager *mcs_manager;
static gboolean is_running = FALSE;
static GtkWidget *dialog = NULL;

/* useful widgets */

static void
add_spacer (GtkBox * box, int size)
{
    GtkWidget *align = gtk_alignment_new (0, 0, 0, 0);

    gtk_widget_set_size_request (align, size, size);
    gtk_widget_show (align);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}

/* size */

static void
size_menu_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_SIZE],
			 CHANNEL, n);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_size_menu (GtkWidget * option_menu)
{
    GtkWidget *menu, *item;
    McsSetting *setting;

    menu = gtk_menu_new ();
    
    item = gtk_menu_item_new_with_label (_("Small"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Medium"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Large"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Huge"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

    setting = mcs_manager_setting_lookup (mcs_manager,
					  xfce_settings_names[XFCE_SIZE],
					  CHANNEL);

    if (setting)
    {
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
				     setting->data.v_int);
    }

    g_signal_connect (option_menu, "changed", G_CALLBACK (size_menu_changed),
		      NULL);
}

/* orientation */

static void
orientation_changed (GtkOptionMenu * menu, GtkOptionMenu *popup_menu)
{
    int n, pos;
    McsSetting *setting;

    n = gtk_option_menu_get_history (menu);

    setting =
	mcs_manager_setting_lookup (mcs_manager, "orientation", CHANNEL);

    if (!setting || n == setting->data.v_int)
	return;

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_ORIENTATION],
			 CHANNEL, n);

    /* also change popup position */
    setting =
	mcs_manager_setting_lookup (mcs_manager, "popupposition", CHANNEL);

    if (setting)
    {
        pos = setting->data.v_int;

        switch (pos)
        {
            case LEFT:
                pos = TOP;
                break;
            case RIGHT:
                pos = BOTTOM;
                break;
            case TOP:
                pos = LEFT;
                break;
            case BOTTOM:
                pos = RIGHT;
                break;
        }

        gtk_option_menu_set_history (GTK_OPTION_MENU (popup_menu), pos);
    }
}

static void
add_orientation_menu (GtkWidget * option_menu, GtkWidget *popup_menu)
{
    GtkWidget *menu;
    GtkWidget *item;
    McsSetting *setting;

    menu = gtk_menu_new ();
    
    item = gtk_menu_item_new_with_label (_("Horizontal"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Vertical"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

    setting =
	mcs_manager_setting_lookup (mcs_manager,
				    xfce_settings_names[XFCE_ORIENTATION],
				    CHANNEL);

    if (setting)
    {
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
				     setting->data.v_int);
    }

    g_signal_connect (option_menu, "changed",
		      G_CALLBACK (orientation_changed), popup_menu);
}

/* popup position */

static void
popup_position_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_POPUPPOSITION],
			 CHANNEL, n);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_popup_position_menu (GtkWidget * option_menu)
{
    GtkWidget *menu;
    GtkWidget *item;
    McsSetting *setting;

    menu = gtk_menu_new ();
    
    item = gtk_menu_item_new_with_label (_("Left"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Right"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Top"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Bottom"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

    setting =
	mcs_manager_setting_lookup (mcs_manager,
				    xfce_settings_names[XFCE_POPUPPOSITION],
				    CHANNEL);

    if (setting)
    {
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
				     setting->data.v_int);
    }

    g_signal_connect (option_menu, "changed",
		      G_CALLBACK (popup_position_changed), NULL);
}

/* handle style */

static void
handle_style_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_HANDLESTYLE],
			 CHANNEL, n);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_handle_style_menu (GtkWidget * option_menu)
{
    GtkWidget *menu;
    GtkWidget *item;
    McsSetting *setting;

    menu = gtk_menu_new ();
    
    item = gtk_menu_item_new_with_label (_("None"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("At both sides"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("At the start"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("At the end"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

    setting =
	mcs_manager_setting_lookup (mcs_manager,
				    xfce_settings_names[XFCE_HANDLESTYLE],
				    CHANNEL);

    if (setting)
    {
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu),
				     setting->data.v_int);
    }

    g_signal_connect (option_menu, "changed",
		      G_CALLBACK (handle_style_changed), NULL);
}

static void
add_style_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *vbox, *hbox, *label, *omenu, *popup_menu;

    vbox = GTK_WIDGET (box);

    /* size */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel size:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    omenu = gtk_option_menu_new ();
    gtk_widget_show (omenu);
    add_size_menu (omenu);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);

    /* panel orientation */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel orientation:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    /* needed here already */
    popup_menu = gtk_option_menu_new ();
    
    omenu = gtk_option_menu_new ();
    gtk_widget_show (omenu);
    add_orientation_menu (omenu, popup_menu);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);

    /* popup button */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Popup position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    omenu = popup_menu;
    gtk_widget_show (omenu);
    add_popup_position_menu (omenu);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);

    /* handle style */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Handles:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    omenu = gtk_option_menu_new ();
    gtk_widget_show (omenu);
    add_handle_style_menu (omenu);
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);
}

/* autohide */

static void
autohide_changed (GtkToggleButton * tb)
{
    int hide;

    hide = gtk_toggle_button_get_active (tb) ? 1 : 0;

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_AUTOHIDE],
			 CHANNEL, hide);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_autohide_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *hbox, *label, *check;
    McsSetting *setting;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Autohide:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg, label);

    check = gtk_check_button_new ();
    gtk_widget_show (check);
    gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);

    setting = mcs_manager_setting_lookup (mcs_manager,
					  xfce_settings_names[XFCE_AUTOHIDE],
					  CHANNEL);

    if (setting)
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				      setting->data.v_int == 1);
    }

    g_signal_connect (check, "toggled", G_CALLBACK (autohide_changed), NULL);
}

/* full_width */
static void
full_width_changed (GtkToggleButton * tb)
{
    int full;

    full = gtk_toggle_button_get_active (tb) ? 1 : 0;

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_FULLWIDTH],
			 CHANNEL, full);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_full_width_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *hbox, *label, *check;
    McsSetting *setting;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Full width:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg, label);

    check = gtk_check_button_new ();
    gtk_widget_show (check);
    gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);

    setting = mcs_manager_setting_lookup (mcs_manager,
					  xfce_settings_names[XFCE_FULLWIDTH],
					  CHANNEL);

    if (setting)
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check),
				      setting->data.v_int == 1);
    }

    g_signal_connect (check, "toggled", G_CALLBACK (full_width_changed), NULL);
}


/* the dialog */

static void
dialog_delete (GtkWidget * dialog)
{
    gtk_widget_destroy (dialog);
    is_running = FALSE;
    dialog = NULL;

    xfce_write_options (mcs_manager);
}

void
run_xfce_settings_dialog (McsPlugin * mp)
{
    GtkWidget *header, *frame, *vbox;
    GtkSizeGroup *sg;

    if (is_running)
    {
	gtk_window_present (GTK_WINDOW (dialog));
	return;
    }

    is_running = TRUE;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    mcs_manager = mp->manager;

    dialog = gtk_dialog_new_with_buttons (_("Xfce Panel"),
					  NULL, GTK_DIALOG_NO_SEPARATOR,
					  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
					  NULL);

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_icon (GTK_WINDOW (dialog), mp->icon);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    
    g_signal_connect (dialog, "response", G_CALLBACK (dialog_delete), NULL);
    g_signal_connect (dialog, "delete_event", G_CALLBACK (dialog_delete),
		      NULL);

    /* pretty header */
    vbox = GTK_DIALOG (dialog)->vbox;

    header = xfce_create_header (mp->icon, _("Xfce Panel Settings"));
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 0);

    add_spacer (GTK_BOX (vbox), 2 * BORDER);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* Appearance */
    vbox = GTK_DIALOG (dialog)->vbox;

    frame = xfce_framebox_new (_("Appearance"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

    add_style_box (GTK_BOX (vbox), sg);

    add_spacer (GTK_BOX (vbox), BORDER);

    /* Behaviour */
    vbox = GTK_DIALOG (dialog)->vbox;

    frame = xfce_framebox_new (_("Behaviour"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

    add_autohide_box (GTK_BOX (vbox), sg);

    add_full_width_box (GTK_BOX (vbox), sg);

    g_object_unref (sg);

    add_spacer (GTK_BOX (vbox), BORDER);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dialog));
    gtk_widget_show (dialog);
}
