/*  dialogs.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <j.b.huijsmans@hetnet.nl>
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

#include <stdio.h>
#include <string.h>
#include <gmodule.h>

#include "dialogs.h"

#include "xfce.h"
#include "xfce_support.h"
#include "callbacks.h"
#include "settings.h"
#include "central.h"
#include "side.h"
#include "item.h"
#include "popup.h"
#include "module.h"
#include "icons.h"

enum
{ RESPONSE_REMOVE, RESPONSE_CHANGE, RESPONSE_CANCEL, RESPONSE_REVERT };

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Central panel dialogs
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
void screen_button_dialog(ScreenButton * sb)
{
    GtkWidget *dialog;
    GtkWidget *entry;
    GtkWidget *vbox1, *vbox2;
    const char *temp;
    int response = GTK_RESPONSE_NONE;

    dialog = gtk_dialog_new_with_buttons(_("Change name"), GTK_WINDOW(toplevel),
                                         GTK_DIALOG_MODAL,
                                         GTK_STOCK_CANCEL,
                                         RESPONSE_CANCEL,
                                         GTK_STOCK_APPLY, RESPONSE_CHANGE, NULL);

    vbox1 = GTK_DIALOG(dialog)->vbox;

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
    gtk_widget_show(vbox2);
    gtk_box_pack_start(GTK_BOX(vbox1), vbox2, TRUE, TRUE, 0);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), sb->name);
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(vbox2), entry, TRUE, TRUE, 0);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if(response == RESPONSE_CHANGE)
    {
        temp = gtk_entry_get_text(GTK_ENTRY(entry));

        if(temp && strlen(temp))
        {
            sb->name = g_strdup(temp);
            gtk_label_set_text(GTK_LABEL(sb->label), temp);
        }
    }

    gtk_widget_destroy(dialog);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Side panel dialogs
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

GtkWidget *dialog = NULL;

void set_transient_for_dialog(GtkWidget * window)
{
    if(dialog)
        gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(dialog));
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Global settings
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* size: option menu
 * popup size: option menu
 * style: option menu
 * icon theme: option menu
 * left : spinbutton
 * right: spinbutton
 * screens: spinbutton
 * desktop buttons: checkbox
 * position: button (restore default)
 * lock command : entry + browse button
 * exit command : entry + browse button
 */

static GtkWidget *size_menu;
static GtkWidget *popup_menu;
static GtkWidget *style_menu;
static GtkWidget *theme_menu;
static GtkWidget *left_spin;
static GtkWidget *right_spin;
static GtkWidget *buttons_checkbox;
static GtkWidget *screens_spin;
static GtkWidget *pos_button;
static GtkWidget *lock_entry;
static GtkWidget *exit_entry;

static GtkSizeGroup *sg = NULL;
static GtkWidget *revert;

static Settings backup;
static int backup_theme_index = 0;

GtkShadowType main_shadow = GTK_SHADOW_IN;
GtkShadowType header_shadow = GTK_SHADOW_NONE;
GtkShadowType option_shadow = GTK_SHADOW_NONE;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* backup */

static void create_backup(void)
{
    backup.x = settings.x;
    backup.y = settings.y;
    backup.size = settings.size;
    backup.popup_size = settings.popup_size;
    backup.style = settings.style;
    backup.icon_theme = g_strdup(settings.icon_theme);
    backup.num_left = settings.num_left;
    backup.num_right = settings.num_right;
    backup.num_screens = settings.num_screens;
    backup.show_desktop_buttons = settings.show_desktop_buttons;
    backup.lock_command = g_strdup(settings.lock_command);
    backup.exit_command = g_strdup(settings.exit_command);
}

static void restore_backup(void)
{
    /* we just let the calbacks of our dialog do all the work */

    gtk_option_menu_set_history(GTK_OPTION_MENU(size_menu), backup.size);
    gtk_option_menu_set_history(GTK_OPTION_MENU(popup_menu), backup.popup_size);
    gtk_option_menu_set_history(GTK_OPTION_MENU(style_menu), backup.style);
    gtk_option_menu_set_history(GTK_OPTION_MENU(theme_menu), backup_theme_index);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spin), backup.num_left);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spin), backup.num_right);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(screens_spin), backup.num_screens);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons_checkbox), 
				 backup.show_desktop_buttons);

    if(backup.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), backup.lock_command);
    else
        gtk_entry_set_text(GTK_ENTRY(lock_entry), "");

    if(backup.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), backup.exit_command);
    else
        gtk_entry_set_text(GTK_ENTRY(exit_entry), "");

    settings.x = backup.x;
    settings.y = backup.y;
    panel_set_position();
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* sections */

static void add_header(const char *text, GtkBox * box)
{
    GtkWidget *frame, *eventbox, *label;
    char *markup;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), header_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, FALSE, TRUE, 0);

    eventbox = gtk_event_box_new();
    gtk_widget_set_name(eventbox, "gxfce_color2");
    gtk_widget_show(eventbox);
    gtk_container_add(GTK_CONTAINER(frame), eventbox);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    markup = g_strconcat("<b><span size=\"large\">", text, "</span></b>", NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
}

#define SKIP 12

static void add_spacer(GtkBox * box)
{
    GtkWidget *eventbox = gtk_event_box_new();

    gtk_widget_set_size_request(eventbox, SKIP, SKIP);
    gtk_widget_show(eventbox);
    gtk_box_pack_start(box, eventbox, FALSE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* sizes */

static void size_menu_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);
    gboolean changed = TRUE;

    if(GTK_WIDGET(menu) == size_menu && n != settings.size)
    {
        panel_set_size(n);

        if(n == TINY)
        {
            gtk_option_menu_set_history(GTK_OPTION_MENU(style_menu), NEW_STYLE);

            gtk_widget_set_sensitive(style_menu, FALSE);
        }
        else
        {
            gtk_widget_set_sensitive(style_menu, TRUE);
        }
    }
    else if(n != settings.popup_size)
    {
        panel_set_popup_size(n);
    }
    else
    {
        changed = FALSE;
    }

    if(changed)
    {
        panel_set_position();
        gtk_widget_set_sensitive(revert, TRUE);
    }
}

static void add_size_menu(GtkWidget * option_menu, int size, gboolean is_popup)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    if(!is_popup)
    {
        item = gtk_menu_item_new_with_label(_("Tiny"));
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    item = gtk_menu_item_new_with_label(_("Small"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Medium"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Large"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), size);

    g_signal_connect(option_menu, "changed", G_CALLBACK(size_menu_changed), NULL);
}

static void add_size_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* size */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    size_menu = gtk_option_menu_new();
    gtk_widget_show(size_menu);
    add_size_menu(size_menu, settings.size, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), size_menu, TRUE, TRUE, 0);

    /* popup */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Popup menu size:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    popup_menu = gtk_option_menu_new();
    gtk_widget_show(popup_menu);
    add_size_menu(popup_menu, settings.popup_size, TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), popup_menu, TRUE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* style */

static void style_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);

    if(n == settings.style)
        return;

    panel_set_style(n);
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_style_menu(GtkWidget * option_menu, int style)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Traditional"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Modern"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), style);

    g_signal_connect(option_menu, "changed", G_CALLBACK(style_changed), NULL);

    if(settings.size == TINY)
        gtk_widget_set_sensitive(option_menu, FALSE);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static char **find_icon_themes(void)
{
    char **themes = NULL;
    GList *list = NULL;
    GDir *gdir;
    char **dirs, **d;
    const char *file;
    int i;

    dirs = get_theme_dirs();

    for(d = dirs; *d; d++)
    {
        gdir = g_dir_open(*d, 0, NULL);

        if(gdir)
        {
            while((file = g_dir_read_name(gdir)))
            {
                char *path = g_build_filename(*d, file);

                if(!g_list_find_custom(list, file, (GCompareFunc) strcmp) &&
                   g_file_test(path, G_FILE_TEST_IS_DIR))
                {
                    list = g_list_append(list, g_strdup(file));
                }

                g_free(path);
            }

            g_dir_close(gdir);
        }
    }

    if(list)
    {
        int len = g_list_length(list);
        GList *li;

        themes = g_new0(char *, len + 1);

        for(i = 0, li = list; li; li = li->next, i++)
        {
            themes[i] = (char *)li->data;
        }
    }

    g_strfreev(dirs);
    return themes;
}

static void theme_changed(GtkOptionMenu * option_menu)
{
    int n = gtk_option_menu_get_history(option_menu);
    const char *theme;

    if(n == 0)
        theme = NULL;
    else
    {
        GtkWidget *label;

        /* Right, this is weird, apparently the option menu
         * button reparents the label connected to the menuitem
         * that is selected. So to get to the label we go to the
         * child of the button and not of the menu item!
         *
         * This took a while to find out :-)
         */

        label = gtk_bin_get_child(GTK_BIN(option_menu));

        theme = gtk_label_get_text(GTK_LABEL(label));
    }

    if((theme == NULL && settings.icon_theme == NULL) ||
       (settings.icon_theme && theme && strequal(theme, settings.icon_theme)))
        return;

    panel_set_icon_theme(theme);
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_theme_menu(GtkWidget * option_menu, const char *theme)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;
    int i = 0, n = 0;
    char **themes = find_icon_themes();
    char **s;

    item = gtk_menu_item_new_with_label(_("XFce standard theme"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    if(themes)
    {
        for(i = 0, s = themes; *s; s++, i++)
        {
            item = gtk_menu_item_new_with_label(*s);
            gtk_widget_show(item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

            if(settings.icon_theme && strequal(settings.icon_theme, *s))
                n = backup_theme_index = i + 1;
        }

        g_strfreev(themes);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), n);

    g_signal_connect(option_menu, "changed", G_CALLBACK(theme_changed), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void add_style_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* style */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel style:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    style_menu = gtk_option_menu_new();
    gtk_widget_show(style_menu);
    add_style_menu(style_menu, settings.style);
    gtk_box_pack_start(GTK_BOX(hbox), style_menu, TRUE, TRUE, 0);

    /* icon theme */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Icon theme:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    theme_menu = gtk_option_menu_new();
    gtk_widget_show(theme_menu);
    add_theme_menu(theme_menu, settings.icon_theme);
    gtk_box_pack_start(GTK_BOX(hbox), theme_menu, TRUE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* panel groups and screen buttons */

static void spin_changed(GtkWidget * spin)
{
    int n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
    gboolean changed = FALSE;

    if(spin == left_spin && n != settings.num_left)
    {
        panel_set_num_left(n);
        changed = TRUE;
    }
    else if(spin == right_spin && n != settings.num_right)
    {
        panel_set_num_right(n);
        changed = TRUE;
    }
    else if(spin == screens_spin && n != settings.num_screens)
    {
        request_net_number_of_desktops(n);
        changed = TRUE;
    }

    if(changed)
        gtk_widget_set_sensitive(revert, TRUE);
}


static void desktop_buttons_changed(GtkToggleButton *button, gpointer data)
{
    gboolean show = gtk_toggle_button_get_active(button);

    central_panel_set_show_desktop_buttons(show);

    gtk_widget_set_sensitive(screens_spin, show);
}

static void add_controls_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* left */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Left panel controls:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    left_spin = gtk_spin_button_new_with_range(1, NBGROUPS, 1);
    gtk_widget_show(left_spin);
    gtk_box_pack_start(GTK_BOX(hbox), left_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(left_spin), settings.num_left);
    g_signal_connect(left_spin, "value-changed", G_CALLBACK(spin_changed), NULL);

    /* right */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Right panel controls:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    right_spin = gtk_spin_button_new_with_range(1, NBGROUPS, 1);
    gtk_widget_show(right_spin);
    gtk_box_pack_start(GTK_BOX(hbox), right_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(right_spin), settings.num_right);
    g_signal_connect(right_spin, "value-changed", G_CALLBACK(spin_changed), NULL);

    /* screens */
    add_spacer(GTK_BOX(vbox));

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Show desktop buttons:")); 
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    buttons_checkbox = gtk_check_button_new();
    gtk_widget_show(buttons_checkbox);
    gtk_box_pack_start(GTK_BOX(hbox), buttons_checkbox, FALSE, FALSE, 0);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons_checkbox), 
				 settings.show_desktop_buttons);
    g_signal_connect(buttons_checkbox, "toggled", G_CALLBACK(desktop_buttons_changed),
		     NULL);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Virtual desktops:")); 
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    screens_spin = gtk_spin_button_new_with_range(1, NBSCREENS, 1);
    gtk_widget_show(screens_spin);
    gtk_box_pack_start(GTK_BOX(hbox), screens_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(screens_spin), settings.num_screens);
    g_signal_connect(screens_spin, "value-changed", G_CALLBACK(spin_changed), NULL);

    gtk_widget_set_sensitive(screens_spin, settings.show_desktop_buttons);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* position */

static void position_clicked(GtkWidget * button)
{
    settings.x = -1;
    settings.y = -1;

    panel_set_position();
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_position_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* button */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Default position:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    pos_button = mixed_button_new(GTK_STOCK_REFRESH, _("Set"));
    gtk_widget_show(pos_button);
    gtk_box_pack_start(GTK_BOX(hbox), pos_button, FALSE, FALSE, 0);

    g_signal_connect(pos_button, "clicked", G_CALLBACK(position_clicked), NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* lock and exit commands */

static void entry_changed(GtkWidget * w)
{
    gtk_widget_set_sensitive(revert, TRUE);
}

static void browse_clicked(GtkWidget * b, GtkEntry * entry)
{
    char *file;
    char *path = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);

    if(!path || !g_file_test(path, G_FILE_TEST_EXISTS))
    {
        g_free(path);
        path = NULL;
    }

    file = select_file_name(NULL, path, dialog);

    if(file && strlen(file))
    {
        gtk_entry_set_text(entry, file);
        gtk_widget_set_sensitive(revert, TRUE);
    }

    g_free(file);
    g_free(path);
}

static void add_advanced_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label, *button;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* lock */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Lock command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    lock_entry = gtk_entry_new();
    gtk_widget_show(lock_entry);
    gtk_box_pack_start(GTK_BOX(hbox), lock_entry, TRUE, TRUE, 0);

    if(settings.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), settings.lock_command);
    g_signal_connect(lock_entry, "insert-at-cursor", G_CALLBACK(entry_changed),
                     NULL);
    g_signal_connect(lock_entry, "delete-from-cursor", G_CALLBACK(entry_changed),
                     NULL);

    button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(browse_clicked), lock_entry);

    /* exit */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Exit command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    exit_entry = gtk_entry_new();
    gtk_widget_show(exit_entry);
    gtk_box_pack_start(GTK_BOX(hbox), exit_entry, TRUE, TRUE, 0);

    if(settings.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), settings.exit_command);
    g_signal_connect(exit_entry, "insert-at-cursor", G_CALLBACK(entry_changed),
                     NULL);
    g_signal_connect(exit_entry, "delete-from-cursor", G_CALLBACK(entry_changed),
                     NULL);

    button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(browse_clicked), exit_entry);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* the dialog */

static gboolean running = FALSE;

void global_settings_dialog(void)
{
    GtkWidget *frame, *vbox, *button;
    gboolean done;

    if(running)
    {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    running = TRUE;
    done = FALSE;

    create_backup();

    dialog =
        gtk_dialog_new_with_buttons(_("Xfce Panel Settings"), GTK_WINDOW(toplevel),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
                                    NULL);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 4);

    revert = mixed_button_new(GTK_STOCK_UNDO, _("_Revert"));
    gtk_widget_show(revert);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), revert, RESPONSE_REVERT);
    gtk_widget_set_sensitive(revert, FALSE);

    button = mixed_button_new(GTK_STOCK_OK, _("_Done"));
    gtk_widget_show(button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dialog), button, GTK_RESPONSE_OK);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), main_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    add_header(_("Sizes"), GTK_BOX(vbox));
    add_size_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Appearance"), GTK_BOX(vbox));
    add_style_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Panel controls"), GTK_BOX(vbox));
    add_controls_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Position"), GTK_BOX(vbox));
    add_position_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Advanced settings"), GTK_BOX(vbox));
    add_advanced_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    g_object_unref(sg);

    while(!done)
    {
        int response = GTK_RESPONSE_NONE;

        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if(response == RESPONSE_REVERT)
        {
            restore_backup();

            panel_set_settings();
            gtk_widget_set_sensitive(revert, FALSE);
        }
        else if(GTK_IS_WIDGET(dialog))
        {
            const char *cmd;

            cmd = gtk_entry_get_text(GTK_ENTRY(lock_entry));

            if(cmd && strlen(cmd))
            {
                g_free(settings.lock_command);
                settings.lock_command = g_strdup(cmd);
            }
            else
            {
                g_free(settings.lock_command);
                settings.lock_command = NULL;
            }

            cmd = gtk_entry_get_text(GTK_ENTRY(exit_entry));

            if(cmd && strlen(cmd))
            {
                g_free(settings.exit_command);
                settings.exit_command = g_strdup(cmd);
            }
            else
            {
                g_free(settings.exit_command);
                settings.exit_command = NULL;
            }

            done = TRUE;
        }
        else
            done = TRUE;
    }

    gtk_widget_destroy(dialog);
    running = FALSE;
    dialog = NULL;
}
