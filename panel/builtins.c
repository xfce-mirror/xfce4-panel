/*  builtins.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
 *                     Xavier Maillard (zedek@fxgsproject.org)
 *                     Olivier Fourdan
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <xfce_iconbutton.h>

#include "builtins.h"
#include "controls.h"
#include "xfce_support.h"
#include "icons.h"
#include "iconbutton.h"
#include "callbacks.h"
#include "xfce.h"

#include "debug.h"

/* panel control configuration
   Global widget used in all the module configuration
   to revert the settings
*/
GtkWidget *revert_button;

/*  Exit module
 *  -----------
*/
typedef struct
{
    GtkWidget *vbox;

    GtkWidget *exit_button;
    GtkWidget *lock_button;
}
t_exit;

static t_exit *exit_new(void)
{
    GtkWidget *im;
    t_exit *exit = g_new(t_exit, 1);

    exit->vbox = gtk_vbox_new(TRUE, 0);
    gtk_widget_show(exit->vbox);

    exit->lock_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(exit->lock_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->lock_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->lock_button, TRUE, TRUE, 0);

    im = gtk_image_new();
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(exit->lock_button), im);

    exit->exit_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(exit->exit_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->exit_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->exit_button, TRUE, TRUE, 0);

    im = gtk_image_new();
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(exit->exit_button), im);

    add_tooltip(exit->lock_button, _("Lock the screen"));
    add_tooltip(exit->exit_button, _("Exit"));

    g_signal_connect_swapped(exit->lock_button, "clicked",
                             G_CALLBACK(mini_lock_cb), NULL);

    g_signal_connect_swapped(exit->exit_button, "clicked", G_CALLBACK(quit),
                             NULL);

    return exit;
}

static void exit_free(PanelControl * pc)
{
    t_exit *exit = (t_exit *) pc->data;

    if(GTK_IS_WIDGET(pc->main))
        gtk_widget_destroy(pc->main);

    g_free(exit);
}

static void exit_set_size(PanelControl * pc, int size)
{
    t_exit *exit = (t_exit *) pc->data;
    GdkPixbuf *pb1, *pb2;
    GtkWidget *im;
    int width, height;

    width = icon_size[size];
    height = icon_size[size] / 2;

    gtk_widget_set_size_request(exit->exit_button, width + 4, height + 2);
    gtk_widget_set_size_request(exit->lock_button, width + 4, height + 2);

    pb1 = get_system_pixbuf(MINIPOWER_ICON);
    im = gtk_bin_get_child(GTK_BIN(exit->exit_button));
    pb2 = get_scaled_pixbuf(pb1, height - 4);
    gtk_image_set_from_pixbuf(GTK_IMAGE(im), pb2);

    g_object_unref(pb1);
    g_object_unref(pb2);

    pb1 = get_system_pixbuf(MINILOCK_ICON);
    im = gtk_bin_get_child(GTK_BIN(exit->lock_button));
    pb2 = get_scaled_pixbuf(pb1, height - 4);
    gtk_image_set_from_pixbuf(GTK_IMAGE(im), pb2);

    g_object_unref(pb1);
    g_object_unref(pb2);
}

static void exit_set_theme(PanelControl * pc, const char *theme)
{
    exit_set_size(pc, settings.size);
}

GtkWidget *lock_entry;
GtkWidget *exit_entry;

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
    GtkWidget *vbox, *hbox, *label;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);

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

    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(exit_apply_configuration), pc);
}

void create_exit(PanelControl * pc)
{
    t_exit *exit = exit_new();

    gtk_container_add(GTK_CONTAINER(pc->base), exit->vbox);

    pc->caption = g_strdup(_("Exit/Lock"));
    pc->data = (gpointer) exit;
    pc->main = exit->exit_button;

    pc->interval = 0;
    pc->update = NULL;

    pc->free = (gpointer) exit_free;

    pc->set_size = (gpointer) exit_set_size;
    pc->set_theme = (gpointer) exit_set_theme;

    pc->add_options = (gpointer) exit_add_options;

    exit_set_size(pc, settings.size);
}

/*  Config module
 *  -------------
*/
static GtkWidget *config_new(void)
{
    GtkWidget *button;
    GdkPixbuf *pb = NULL;

    pb = get_system_pixbuf(MINIPALET_ICON);
    button = xfce_iconbutton_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(button);

    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    add_tooltip(button, _("Setup..."));

    g_signal_connect_swapped(button, "clicked", G_CALLBACK(mini_palet_cb), NULL);

    return button;
}

static void config_set_size(PanelControl * pc, int size)
{
    int s = icon_size[size] + border_width;
    
    gtk_widget_set_size_request(pc->main, s, s);
}

static void config_set_theme(PanelControl * pc, const char *theme)
{
    GdkPixbuf *pb;
    int s = icon_size[settings.size] + border_width;
    
    pb = get_system_pixbuf(MINIPALET_ICON);
    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(pc->main), pb);
    g_object_unref(pb);
    
    gtk_widget_set_size_request(pc->main, s, s);
}

void create_config(PanelControl * pc)
{
    GtkWidget *b = config_new();

    gtk_container_add(GTK_CONTAINER(pc->base), b);
    pc->caption = g_strdup(_("Setup"));
    pc->data = (gpointer) b;
    pc->main = b;

    pc->set_size = (gpointer) config_set_size;
    pc->set_theme = (gpointer) config_set_theme;
}
