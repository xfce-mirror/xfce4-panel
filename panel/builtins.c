/*  builtins.c
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "builtins.h"
#include "controls.h"
#include "iconbutton.h"
#include "xfce_support.h"
#include "icons.h"
#include "iconbutton.h"
#include "callbacks.h"
#include "xfce.h"

/* panel control configuration 
*/
GtkWidget *revert_button;


/*  Clock module
 *  ------------
 *  To be replaced by the 'real clock widget' 
*/
enum
{ ANALOG, DIGITAL, LED };

typedef struct
{
    int type;
    gboolean twentyfour;

    gboolean show_colon;

    GtkWidget *frame;
    GtkWidget *eventbox;
    GtkWidget *label;
}
t_clock;

static t_clock *clock_new(void)
{
    t_clock *clock = g_new(t_clock, 1);

    clock->show_colon = TRUE;
    clock->type = DIGITAL;
    clock->twentyfour = TRUE;

    clock->frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(clock->frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_IN);
    gtk_widget_show(clock->frame);

    clock->eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(clock->frame), clock->eventbox);
    gtk_widget_show(clock->eventbox);

    clock->label = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(clock->eventbox), clock->label);
    gtk_widget_show(clock->label);

    return clock;
}

static gboolean adjust_time(PanelControl * pc)
{
    time_t t;
    struct tm *tm;
    int hrs, mins;
    char *text, *markup;

    t_clock *clock = (t_clock *) pc->data;

    t = time(0);
    tm = localtime(&t);

    hrs = tm->tm_hour;
    mins = tm->tm_min;

    text =
        g_strdup_printf("%.2d%c%.2d", hrs, clock->show_colon ? ':' : ' ', mins);

    switch (settings.size)
    {
        case TINY:
            markup =
                g_strconcat("<tt><span size=\"x-small\">", text, "</span></tt>",
                            NULL);
            break;
        case SMALL:
            markup =
                g_strconcat("<tt><span size=\"small\">", text, "</span></tt>",
                            NULL);
            break;
        case LARGE:
            markup =
                g_strconcat("<tt><span size=\"large\">", text, "</span></tt>",
                            NULL);
            break;
        default:
            markup =
                g_strconcat("<tt><span size=\"medium\">", text, "</span></tt>",
                            NULL);
            break;
    }

    clock->show_colon = !clock->show_colon;

    gtk_label_set_markup(GTK_LABEL(clock->label), markup);

    g_free(text);
    g_free(markup);

    return TRUE;
}

void clock_free(PanelControl * pc)
{
    t_clock *clock = (t_clock *) pc->data;

    g_free(clock);
}

void clock_set_size(PanelControl * pc, int size)
{
    int s = icon_size[size];
    t_clock *clock = (t_clock *) pc->data;

    gtk_widget_set_size_request(clock->label, -1, s);
    adjust_time(pc);
}

void clock_set_style(PanelControl * pc, int style)
{
    t_clock *clock = (t_clock *) pc->data;

    if(style == OLD_STYLE)
    {
        gtk_widget_set_name(clock->frame, "gxfce_color2");
        gtk_widget_set_name(clock->label, "gxfce_color2");
        gtk_widget_set_name(clock->eventbox, "gxfce_color2");
    }
    else
    {
        gtk_widget_set_name(clock->frame, "gxfce_color4");
        gtk_widget_set_name(clock->label, "gxfce_color4");
        gtk_widget_set_name(clock->eventbox, "gxfce_color4");
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void clock_type_changed(GtkOptionMenu * om, t_clock * clock)
{
    clock->type = gtk_option_menu_get_history(om);
}

static GtkWidget *create_clock_type_option_menu(t_clock * clock)
{
    GtkWidget *om, *menu, *mi;

    om = gtk_option_menu_new();

    menu = gtk_menu_new();
    gtk_widget_show(menu);

    mi = gtk_menu_item_new_with_label(_("Analog"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("Digital"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("LED"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    gtk_option_menu_set_history(GTK_OPTION_MENU(om), clock->type);

    g_signal_connect(om, "changed", G_CALLBACK(clock_type_changed), clock);

    return om;
}

static void clock_hour_mode_changed(GtkToggleButton * tb, t_clock * clock)
{
    clock->twentyfour = gtk_toggle_button_get_active(tb);
}

static GtkWidget *create_clock_24hrs_button(t_clock * clock)
{
    GtkWidget *cb;

    cb = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->twentyfour);

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_hour_mode_changed), clock);

    return cb;
}

static void clock_create_backup(t_clock *clock)
{
}

static void clock_clean_backup(void)
{
}

static void clock_revert(t_clock *clock)
{
}

static void clock_apply_configuration(PanelControl * pc)
{
}

void clock_add_options(PanelControl * pc, GtkContainer * container,
                       GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *om, *hbox, *label, *checkbutton;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    t_clock *clock = (t_clock *) pc->data;

    clock_create_backup(clock);
    
    revert_button = revert;
    
    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Clock type:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    om = create_clock_type_option_menu(clock);
    gtk_widget_show(om);
    gtk_box_pack_start(GTK_BOX(hbox), om, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("24 hour clock:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    checkbutton = create_clock_24hrs_button(clock);
    gtk_widget_show(checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), checkbutton, FALSE, FALSE, 0);

    /* signals */
    g_signal_connect_swapped(revert, "clicked", 
	    		     G_CALLBACK(clock_revert), clock);
    g_signal_connect_swapped(done, "clicked", 
	    		     G_CALLBACK(clock_clean_backup), NULL);
    g_signal_connect_swapped(done, "clicked", 
	    		     G_CALLBACK(clock_apply_configuration), pc);
    
    gtk_container_add(container, vbox);
}

/*  create clock panel control
*/
void create_clock(PanelControl * pc)
{
    t_clock *clock = clock_new();

    gtk_container_add(GTK_CONTAINER(pc->base), clock->frame);

    pc->caption = g_strdup(_("XFce clock"));
    pc->data = (gpointer) clock;
    pc->main = clock->eventbox;

    pc->interval = 1000;        /* 1 sec */
    pc->update = (gpointer) adjust_time;

    pc->free = (gpointer) clock_free;

    pc->set_size = (gpointer) clock_set_size;
    pc->set_style = (gpointer) clock_set_style;

    pc->add_options = (gpointer) clock_add_options;
}

/*  Trash module
 *  ------------
*/
typedef struct
{
    char *dirname;
    char *command;
    gboolean in_terminal;

    gboolean empty;

    GdkPixbuf *empty_pb;
    GdkPixbuf *full_pb;

    IconButton *button;
}
t_trash;

static void trash_run(t_trash * trash)
{
    exec_cmd(trash->command, trash->in_terminal);
}

void trash_dropped(GtkWidget *widget, GList *drop_data, gpointer data)
{
    t_trash *trash = (t_trash *)data;
    char *cmd;
    GList *li;

    for (li = drop_data; li && li->data; li = li->next)
    {
	cmd = g_strconcat(trash->command, " ", (char *)li->data, NULL);

	exec_cmd_silent(cmd, FALSE);

	g_free(cmd);
    }
}

static t_trash *trash_new(void)
{
    t_trash *trash = g_new(t_trash, 1);
    const char *home = g_getenv("HOME");
    GtkWidget *b;

    trash->dirname = g_strconcat(home, "/.xfce/trash", NULL);

    trash->empty = TRUE;

    trash->command = g_strdup("xftrash");
    trash->in_terminal = FALSE;

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    trash->button = icon_button_new(trash->empty_pb);

    b = icon_button_get_button(trash->button);
    
    add_tooltip(b, _("Trashcan: 0 files"));

    /* signals */
    dnd_set_drag_dest(b);
    dnd_set_callback(b, DROP_CALLBACK(trash_dropped), trash);

    g_signal_connect_swapped(b, "clicked", G_CALLBACK(trash_run), trash);
    
    return trash;
}

static gboolean check_trash(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    GDir *dir;
    const char *file;
    char text[MAXSTRLEN];
    gboolean changed = FALSE;

    if(!trash->dirname)
        return TRUE;
    dir = g_dir_open(trash->dirname, 0, NULL);

    if(dir)
        file = g_dir_read_name(dir);

    if(!dir || !file)
    {
        if(!trash->empty)
        {
            trash->empty = TRUE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->empty_pb);
            add_tooltip(icon_button_get_button(trash->button),
                        _("Trashcan: 0 files"));
        }
    }
    else
    {
        struct stat s;
        int number = 0;
        int size = 0;
        char *cwd = g_get_current_dir();

        chdir(trash->dirname);

        if(trash->empty)
        {
            trash->empty = FALSE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->full_pb);
        }

        while(file)
        {
            number++;

            stat(file, &s);
            size += s.st_size;

            file = g_dir_read_name(dir);
        }

        chdir(cwd);
        g_free(cwd);

        if(size < 1024)
            sprintf(text, _("Trashcan: %d files / %d B"), number, size);
        else if(size < 1024 * 1024)
            sprintf(text, _("Trashcan: %d files / %d KB"), number, size / 1024);
        else
            sprintf(text, _("Trashcan: %d files / %d MB"), number,
                    size / (1024 * 1024));

        add_tooltip(icon_button_get_button(trash->button), text);
    }

    if(dir)
        g_dir_close(dir);

    return TRUE;
}

static void trash_free(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    g_free(trash->dirname);

    icon_button_free(trash->button);

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    g_free(trash);
}

static void trash_set_size(PanelControl * pc, int size)
{
    t_trash *trash = (t_trash *) pc->data;

    icon_button_set_size(trash->button, size);
}

static void trash_set_theme(PanelControl * pc, const char *theme)
{
    t_trash *trash = (t_trash *) pc->data;

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    if(trash->empty)
        icon_button_set_pixbuf(trash->button, trash->empty_pb);
    else
        icon_button_set_pixbuf(trash->button, trash->full_pb);
}

/*  create trash panel control
*/
void create_trash(PanelControl * pc)
{
    t_trash *trash = trash_new();
    GtkWidget *b = icon_button_get_button(trash->button);

    gtk_container_add(GTK_CONTAINER(pc->base), b);

    pc->caption = g_strdup(_("Trash can"));
    pc->data = (gpointer) trash;
    pc->main = b;

    pc->interval = 2000;        /* 2 sec */
    pc->update = (gpointer) check_trash;

    pc->free = (gpointer) trash_free;

    pc->set_size = (gpointer) trash_set_size;
    pc->set_theme = (gpointer) trash_set_theme;
}

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
static IconButton *config_new(void)
{
    IconButton *button;
    GtkWidget *b;
    GdkPixbuf *pb = NULL;

    pb = get_system_pixbuf(MINIPALET_ICON);
    button = icon_button_new(pb);
    g_object_unref(pb);

    b = icon_button_get_button(button);
    
    add_tooltip(b, _("Setup..."));

    g_signal_connect_swapped(b, "clicked", G_CALLBACK(mini_palet_cb),
                             NULL);

    return button;
}

static void config_free(PanelControl * pc)
{
    IconButton *b = (IconButton *)pc->data;

    icon_button_free(b);
}

static void config_set_size(PanelControl * pc, int size)
{
    IconButton *b = (IconButton *)pc->data;

    icon_button_set_size(b, size);
}

static void config_set_theme(PanelControl * pc, const char *theme)
{
    GdkPixbuf *pb;
    IconButton *b = (IconButton *)pc->data;

    pb = get_system_pixbuf(MINIPALET_ICON);
    icon_button_set_pixbuf(b, pb);
    g_object_unref(pb);
}

void create_config(PanelControl * pc)
{
    IconButton *config = config_new();
    GtkWidget *b = icon_button_get_button(config);

    gtk_container_add(GTK_CONTAINER(pc->base), b);
    pc->caption = g_strdup(_("Setup"));
    pc->data = (gpointer) config;
    pc->main = b;

    pc->interval = 0;
    pc->update = NULL;

    pc->free = (gpointer) config_free;

    pc->set_size = (gpointer) config_set_size;
    pc->set_theme = (gpointer) config_set_theme;
}

