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

#include "builtins.h"
#include "controls.h"
#include "icons.h"
#include "xfce_support.h"

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
    char *cmd = settings.lock_command;

    if(!cmd)
        return;

    hide_current_popup_menu();

    exec_cmd(cmd, FALSE);
}

static void mini_info_cb(void)
{
    hide_current_popup_menu();

    info_panel_dialog();
}

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
    gboolean with_lock;

    GtkWidget *vbox;

    GtkWidget *exit_button;
    GtkWidget *lock_button;
}
t_exit;

static gboolean backup_lock;

static t_exit *exit_new(void)
{
    GdkPixbuf *pb;
    t_exit *exit = g_new(t_exit, 1);

    exit->with_lock = TRUE;

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
    t_exit *exit = (t_exit *)pc->data;

    g_free(exit);
}

static void exit_read_config(PanelControl * pc, xmlNodePtr node)
{
    xmlChar *value;
    int n;
    t_exit *exit = (t_exit *)pc->data;

    value = xmlGetProp(node, (const xmlChar *)"lock");

    if (value)
    {
	n = atoi(value);

	if (n == 0)
	{
	    exit->with_lock = FALSE;
	    gtk_widget_hide(exit->lock_button);
	}
    }
}

static void exit_write_config(PanelControl * pc, xmlNodePtr node)
{
    t_exit *exit = (t_exit *)pc->data;

    xmlSetProp(node, (const xmlChar *)"lock", exit->with_lock ? "1" : "0");
}

static void exit_attach_callback(PanelControl *pc, const char *signal,
				 GCallback callback, gpointer data)
{
    t_exit *exit = (t_exit *)pc->data;

    g_signal_connect(exit->exit_button, signal, callback, data);
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

/*    panel_control_set_size(pc, settings.size);*/
}

GtkWidget *lock_entry;
GtkWidget *exit_entry;

static void toggle_lock(GtkWidget *b, t_exit *exit)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b)))
    {
	gtk_widget_show(exit->lock_button);
	exit->with_lock = TRUE;
	gtk_widget_set_sensitive(lock_entry, TRUE);
    }
    else
    {
	gtk_widget_hide(exit->lock_button);
	exit->with_lock = FALSE;
	gtk_widget_set_sensitive(lock_entry, FALSE);
    }

    gtk_widget_set_sensitive(revert_button, TRUE);
}

static void exit_revert_configuration(GtkWidget *b)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), backup_lock);

    if(settings.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), settings.lock_command);
    
    if(settings.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), settings.exit_command);
}

static void exit_apply_configuration(PanelControl * pc)
{
    const char *tmp;
    
    tmp = gtk_entry_get_text(GTK_ENTRY(lock_entry));

    g_free(settings.lock_command);

    if(tmp && *tmp)
        settings.lock_command = g_strdup(tmp);
    else
        settings.lock_command = NULL;

    tmp = gtk_entry_get_text(GTK_ENTRY(exit_entry));

    g_free(settings.exit_command);

    if(tmp && *tmp)
        settings.exit_command = g_strdup(tmp);
    else
        settings.exit_command = NULL;
}

void exit_add_options(PanelControl * pc, GtkContainer * container,
                      GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label, *checkbox;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    t_exit *exit = (t_exit *)pc->data;

    revert_button = revert;

    backup_lock = exit->with_lock;
    
    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);

    /* lock checkbox */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Show lock button:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    checkbox = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbox), exit->with_lock);
    gtk_widget_show(checkbox);
    gtk_box_pack_start(GTK_BOX(hbox), checkbox, FALSE, FALSE, 0);

    /* lock command */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Lock command:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    lock_entry = gtk_entry_new();
    if(settings.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), settings.lock_command);
    gtk_widget_show(lock_entry);
    gtk_box_pack_start(GTK_BOX(hbox), lock_entry, FALSE, FALSE, 0);

    /* exit command */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Exit command"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    exit_entry = gtk_entry_new();
    if(settings.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), settings.exit_command);
    gtk_widget_show(exit_entry);
    gtk_box_pack_start(GTK_BOX(hbox), exit_entry, FALSE, FALSE, 0);

    gtk_container_add(container, vbox);

    /* signals */
    g_signal_connect(checkbox, "toggled", G_CALLBACK(toggle_lock), exit);
    
    g_signal_connect_swapped(revert, "clicked",
                             G_CALLBACK(exit_revert_configuration), checkbox);
    
    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(exit_apply_configuration), pc);
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

