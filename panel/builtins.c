/*  builtins.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#include "xfce.h"
#include "xfce_support.h"
#include "module.h"
#include "item.h"
#include "icons.h"
#include "dialogs.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Builtin modules
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static gboolean create_clock_module(PanelModule * pm);

static gboolean create_trash_module(PanelModule * pm);

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

gboolean create_builtin_module(PanelModule * pm)
{
    switch (pm->id)
    {
        case CLOCK_MODULE:
            return create_clock_module(pm);
            break;
        case TRASH_MODULE:
            return create_trash_module(pm);
            break;
        default:
            return FALSE;
    }
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Clock module
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
enum
{ ANALOG, DIGITAL, LED };

typedef struct
{
    int size;

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

    clock->size = MEDIUM;
    clock->show_colon = TRUE;
    clock->type = DIGITAL;
    clock->twentyfour = TRUE;

    clock->frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(clock->frame), 0);
    gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_IN);
    gtk_widget_show(clock->frame);
    /* protect against destruction when unpacking */
    g_object_ref(clock->frame);

    clock->eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(clock->frame), clock->eventbox);
    gtk_widget_show(clock->eventbox);

    clock->label = gtk_label_new(NULL);
    gtk_container_add(GTK_CONTAINER(clock->eventbox), clock->label);
    gtk_widget_show(clock->label);

    return clock;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean adjust_time(PanelModule * pm)
{
    time_t t;
    struct tm *tm;
    int hrs, mins;
    char *text, *markup;

    t_clock *clock = (t_clock *) pm->data;

    t = time(0);
    tm = localtime(&t);

    hrs = tm->tm_hour;
    mins = tm->tm_min;

    text = g_strdup_printf("%.2d%c%.2d", hrs, clock->show_colon ? ':' : ' ', mins);

    switch (clock->size)
    {
	case TINY:
            markup = g_strconcat("<tt><span size=\"x-small\">",
                                 text, "</span></tt>", NULL);
	    break;
        case SMALL:
            markup = g_strconcat("<tt><span size=\"small\">",
                                 text, "</span></tt>", NULL);
            break;
        case LARGE:
            markup = g_strconcat("<tt><span size=\"large\">",
                                 text, "</span></tt>", NULL);
            break;
        default:
            markup = g_strconcat("<tt><span size=\"medium\">",
                                 text, "</span></tt>", NULL);
            break;
    }

    clock->show_colon = !clock->show_colon;

    gtk_label_set_markup(GTK_LABEL(clock->label), markup);

    g_free(text);
    g_free(markup);

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void clock_pack(PanelModule * pm, GtkBox * box)
{
    t_clock *clock = (t_clock *) pm->data;

    gtk_box_pack_start(box, clock->frame, TRUE, TRUE, 0);
}

void clock_unpack(PanelModule * pm, GtkContainer * container)
{
    t_clock *clock = (t_clock *) pm->data;

    gtk_container_remove(container, clock->frame);
}

void clock_free(PanelModule * pm)
{
    t_clock *clock = (t_clock *) pm->data;

    if(GTK_IS_WIDGET(clock->frame))
        gtk_widget_destroy(clock->frame);

    g_free(clock);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void clock_set_size(PanelModule * pm, int size)
{
    int s = icon_size(size);
    t_clock *clock = (t_clock *) pm->data;

    clock->size = size;

    gtk_widget_set_size_request(clock->label, -1, s);
}

void clock_set_style(PanelModule * pm, int style)
{
    t_clock *clock = (t_clock *) pm->data;

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

void clock_add_options(PanelModule * pm, GtkContainer * container)
{
    GtkWidget *vbox, *om, *hbox, *label, *checkbutton;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    t_clock *clock = (t_clock *) pm->data;

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

    gtk_container_add(container, vbox);
}

static void clock_apply_configuration(PanelModule * pm)
{
}

void clock_configure(PanelModule * pm)
{
    report_error(_("No configuration possible (yet)."));
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean create_clock_module(PanelModule * pm)
{
    t_clock *clock = clock_new();

    pm->caption = g_strdup(_("XFce clock"));
    pm->data = (gpointer) clock;
    pm->main = clock->eventbox;

    pm->interval = 1000;        /* 1 sec */
    pm->update = (gpointer) adjust_time;

    pm->pack = (gpointer) clock_pack;
    pm->unpack = (gpointer) clock_unpack;
    pm->free = (gpointer) clock_free;

    pm->set_size = (gpointer) clock_set_size;
    pm->set_style = (gpointer) clock_set_style;

    pm->add_options = (gpointer) clock_add_options;
    pm->apply_configuration = (gpointer) clock_apply_configuration;
/*    pm->configure = (gpointer) clock_configure;*/

    if(pm->parent)
        adjust_time(pm);

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Trash module
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
typedef struct
{
    char *dirname;

    int size;

    gboolean empty;

    GdkPixbuf *empty_pb;
    GdkPixbuf *full_pb;

    /* we just overload the panel item a bit */
    PanelItem *item;
}
t_trash;

static t_trash *trash_new(PanelGroup * pg)
{
    t_trash *trash = g_new(t_trash, 1);
    const char *home = g_getenv("HOME");

    trash->dirname = g_strconcat(home, "/.xfce/trash", NULL);

    trash->size = MEDIUM;

    trash->empty = TRUE;

    trash->item = panel_item_new(pg);
    trash->item->command = g_strdup("xftrash");

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    create_panel_item(trash->item);

    g_object_unref(trash->item->pb);
    trash->item->pb = trash->empty_pb;

    panel_item_set_size(trash->item, MEDIUM);

    add_tooltip(trash->item->button, _("Trashcan: 0 files"));

    return trash;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean check_trash(PanelModule * pm)
{
    t_trash *trash = (t_trash *) pm->data;
    PanelItem *pi = trash->item;

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
            pi->pb = trash->empty_pb;
            trash->empty = TRUE;
            changed = TRUE;
            add_tooltip(pi->button, _("Trashcan: 0 files"));
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
            pi->pb = trash->full_pb;
            trash->empty = FALSE;
            changed = TRUE;
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

        add_tooltip(pi->button, text);
    }

    if(dir)
        g_dir_close(dir);

    if(changed)
    {
        panel_item_set_size(trash->item, settings.size);
    }

    return TRUE;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void trash_pack(PanelModule * pm, GtkBox * box)
{
    t_trash *trash = (t_trash *) pm->data;

    panel_item_pack(trash->item, box);
}

static void trash_unpack(PanelModule * pm, GtkContainer * container)
{
    t_trash *trash = (t_trash *) pm->data;

    panel_item_unpack(trash->item, container);
}

static void trash_free(PanelModule * pm)
{
    t_trash *trash = (t_trash *) pm->data;

    g_free(trash->dirname);

    /* will be unreffed in panel_item_free() */
    g_object_ref(trash->item->pb);
    panel_item_free(trash->item);

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    if(GTK_IS_WIDGET(pm->main))
        gtk_widget_destroy(pm->main);

    g_free(trash);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void trash_set_size(PanelModule * pm, int size)
{
    t_trash *trash = (t_trash *) pm->data;

    trash->size = size;
    panel_item_set_size(trash->item, size);
}

static void trash_set_icon_theme(PanelModule * pm, const char *theme)
{
    t_trash *trash = (t_trash *) pm->data;

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    if(trash->empty)
        trash->item->pb = trash->empty_pb;
    else
        trash->item->pb = trash->full_pb;

    panel_item_set_size(trash->item, trash->size);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static gboolean create_trash_module(PanelModule * pm)
{
    t_trash *trash = trash_new(pm->parent);

    pm->caption = g_strdup(_("Trash can"));
    pm->data = (gpointer) trash;
    pm->main = trash->item->button;

    pm->interval = 2000;        /* 2 sec */
    pm->update = (gpointer) check_trash;

    pm->pack = (gpointer) trash_pack;
    pm->unpack = (gpointer) trash_unpack;
    pm->free = (gpointer) trash_free;

    pm->set_size = (gpointer) trash_set_size;
    pm->set_icon_theme = (gpointer) trash_set_icon_theme;

    if(pm->parent)
        check_trash(pm);

    return TRUE;
}
