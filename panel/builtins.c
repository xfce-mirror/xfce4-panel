/*  builtins.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
 *                     Xavier Maillard (zedek@fxgsproject.org)
 *                     Olivier Fourdan (fourdan@xfce.org)
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

#include <libxfcegui4/xfce_iconbutton.h>

#include "xfce.h"
#include "builtins.h"
#include "popup.h"
#include "dialogs.h"

/* panel control configuration
   Global widget used in all the module configuration
   to revert the settings
*/
static GtkWidget *revert_button;

/*  Callbacks
 *  ---------
*/
static void mini_lock_cb(void)
{
    hide_current_popup_menu();

    exec_cmd("xflock", FALSE);
}

#if 0
static void mini_info_cb(void)
{
    hide_current_popup_menu();

    info_panel_dialog();
}
#endif

static void mini_palet_cb(void)
{
    hide_current_popup_menu();

    if(disable_user_config)
    {
        show_info(_("Access to the configuration system has been disabled.\n\n"
                    "Ask your system administrator for more information"));
        return;
    }

    global_settings_dialog();
}

static void mini_power_cb(GtkButton * b, GdkEventButton * ev, gpointer data)
{
    hide_current_popup_menu();

    quit(FALSE);
}

/*  Exit module
 *  -----------
*/
typedef struct
{
    int buttons; /* 0, 1 or 2 for both, lock, exit */

    GtkWidget *vbox;
    GtkWidget *lock_button;
    GtkWidget *exit_button;
}
t_exit;

static int backup_buttons;

static t_exit *exit_new(void)
{
    GdkPixbuf *pb;
    t_exit *exit = g_new(t_exit, 1);

    exit->buttons = 0; /* both buttons */

    exit->vbox = gtk_vbox_new(TRUE, 0);
    gtk_widget_show(exit->vbox);

    pb = get_minibutton_pixbuf(MINILOCK_ICON);
    exit->lock_button = xfce_iconbutton_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_button_set_relief(GTK_BUTTON(exit->lock_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->lock_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->lock_button, TRUE, TRUE, 0);

    pb = get_minibutton_pixbuf(MINIPOWER_ICON);
    exit->exit_button = xfce_iconbutton_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_button_set_relief(GTK_BUTTON(exit->exit_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->exit_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->exit_button, TRUE, TRUE, 0);

    add_tooltip(exit->lock_button, _("Lock the screen"));
    add_tooltip(exit->exit_button, _("Exit"));

    g_signal_connect(exit->lock_button, "clicked", G_CALLBACK(mini_lock_cb), NULL);

    g_signal_connect(exit->exit_button, "clicked", G_CALLBACK(mini_power_cb), NULL);

    return exit;
}

static void exit_free(PanelControl * pc)
{
    t_exit *exit = pc->data;

    g_free(exit);
}

static void exit_read_config(PanelControl * pc, xmlNodePtr node)
{
    xmlChar *value;
    t_exit *exit = (t_exit *)pc->data;

    value = xmlGetProp(node, (const xmlChar *)"buttons");

    if (value)
    {
	exit->buttons = atoi(value);
	g_free(value);

	switch (exit->buttons)
	{
	    case 1:
		gtk_widget_hide(exit->exit_button);
		gtk_widget_show(exit->lock_button);
		break;
	    case 2:
		gtk_widget_hide(exit->lock_button);
		gtk_widget_show(exit->exit_button);
		break;
	    default:
		gtk_widget_show(exit->lock_button);
		gtk_widget_show(exit->exit_button);
		exit->buttons = 0;
	}
    }
}

static void exit_write_config(PanelControl * pc, xmlNodePtr node)
{
    char prop[3];
    t_exit *exit = pc->data;

    sprintf(prop, "%d", exit->buttons);

    xmlSetProp(node, (const xmlChar *)"buttons", prop);
}

static void exit_attach_callback(PanelControl *pc, const char *signal,
				 GCallback callback, gpointer data)
{
    t_exit *exit = (t_exit *)pc->data;

    g_signal_connect(exit->lock_button, signal, callback, data);
    g_signal_connect(exit->exit_button, signal, callback, data);
}

static void exit_set_theme(PanelControl * pc, const char *theme)
{
    GdkPixbuf *pb;
    t_exit *exit = (t_exit*)pc->data;
    
    pb = get_minibutton_pixbuf(MINILOCK_ICON);
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(exit->lock_button), pb);
    g_object_unref(pb);
    
    pb = get_minibutton_pixbuf(MINIPOWER_ICON);
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(exit->exit_button), pb);
    g_object_unref(pb);
}

GtkWidget *lock_entry;
GtkWidget *exit_entry;

static void buttons_changed(GtkOptionMenu *om, t_exit *exit)
{
    int n = gtk_option_menu_get_history(om);
    
    if (n == exit->buttons)
	return;
    
    exit->buttons = n;
    
    switch (n)
    {
	case 1:
	    gtk_widget_hide(exit->exit_button);
	    gtk_widget_show(exit->lock_button);
	    break;
	case 2:
	    gtk_widget_hide(exit->lock_button);
	    gtk_widget_show(exit->exit_button);
	    break;
	default:
	    gtk_widget_show(exit->lock_button);
	    gtk_widget_show(exit->exit_button);
    }

    gtk_widget_set_sensitive(revert_button, TRUE);
}

static void exit_revert_configuration(GtkOptionMenu *om)
{
    gtk_option_menu_set_history(om, backup_buttons);
}

void exit_add_options(PanelControl * pc, GtkContainer * container,
                      GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label, *om, *menu, *mi;
    t_exit *exit = pc->data;

    revert_button = revert;
    backup_buttons = exit->buttons;
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_add(container, vbox);
    
    /* buttons option menu */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Show buttons:"));
    gtk_widget_show(label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    om = gtk_option_menu_new();
    gtk_widget_show(om);
    gtk_box_pack_start(GTK_BOX(hbox), om, FALSE, FALSE, 0);

    menu = gtk_menu_new();
    
    mi = gtk_menu_item_new_with_label(_("Both"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("Lock"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("Exit"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(om), exit->buttons);

    g_signal_connect(om, "changed", G_CALLBACK(buttons_changed), exit);
    g_signal_connect_swapped(revert, "clicked", 
	    		     G_CALLBACK(exit_revert_configuration), om);
}

void create_exit(PanelControl * pc)
{
    t_exit *exit = exit_new();

    gtk_container_add(GTK_CONTAINER(pc->base), exit->vbox);

    pc->caption = g_strdup(_("Exit/Lock"));
    pc->data = (gpointer) exit;

    pc->free = exit_free;
    pc->read_config = exit_read_config;
    pc->write_config = exit_write_config;
    pc->attach_callback = exit_attach_callback;
    
    pc->set_theme = exit_set_theme;

    pc->add_options = exit_add_options;

    panel_control_set_size(pc, settings.size);
}

/*  Config module
 *  -------------
*/
static GtkWidget *config_new(void)
{
    GtkWidget *button;
    GdkPixbuf *pb = NULL;

    pb = get_minibutton_pixbuf(MINIPALET_ICON);
    button = xfce_iconbutton_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(button);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    add_tooltip(button, _("Setup..."));

    g_signal_connect(button, "clicked", G_CALLBACK(mini_palet_cb), NULL);

    return button;
}

static void config_attach_callback(PanelControl *pc, const char *signal,
				   GCallback callback, gpointer data)
{
    g_signal_connect(GTK_WIDGET(pc->data), signal, callback, data);
}

static void config_set_theme(PanelControl * pc, const char *theme)
{
    GdkPixbuf *pb;

    pb = get_minibutton_pixbuf(MINIPALET_ICON);
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(pc->data), pb);
    g_object_unref(pb);
}

void create_config(PanelControl * pc)
{
    GtkWidget *b = config_new();

    gtk_container_add(GTK_CONTAINER(pc->base), b);
    pc->caption = g_strdup(_("Setup"));
    pc->data = b;

    pc->attach_callback = config_attach_callback;
    pc->set_theme = config_set_theme;
}

