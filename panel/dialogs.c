/*  dialogs.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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

#include <X11/Xlib.h>
#include <libxfcegui4/netk-util.h>

#include "xfce.h"
#include "dialogs.h"

#include "callbacks.h"
#include "settings.h"
#include "groups.h"
#include "item.h"
#include "popup.h"

enum
{ RESPONSE_REMOVE, RESPONSE_CHANGE, RESPONSE_CANCEL, RESPONSE_REVERT };

/*  Global settings
 *  ---------------
 *  size: option menu
 *  popup position: option menu
 *  style: option menu (should be radio buttons according to GNOME HIG)
 *  panel orientation: option menu
 *  icon theme: option menu
 *  num groups : spinbutton
 *  position: button (restore default)
*/
static GtkWidget *orientation_menu;
static GtkWidget *size_menu;
static GtkWidget *popup_position_menu;
static GtkWidget *style_menu;
static GtkWidget *theme_menu;
static GtkWidget *groups_spin;

static GtkWidget *layer_menu;
static GtkWidget *pos_button;

static GtkSizeGroup *sg = NULL;
static GtkWidget *revert;

static Position backup_pos;
static Settings backup;
static int backup_theme_index = 0;

GtkShadowType main_shadow = GTK_SHADOW_NONE;
GtkShadowType header_shadow = GTK_SHADOW_NONE;
GtkShadowType option_shadow = GTK_SHADOW_NONE;

/*  backup
*/
static void create_backup(void)
{
    backup_pos.x = position.x;
    backup_pos.y = position.y;
    backup.size = settings.size;
    backup.popup_position = settings.popup_position;
    backup.style = settings.style;
    backup.orientation = settings.orientation;
    backup.theme = g_strdup(settings.theme);
    backup.num_groups = settings.num_groups;
    backup.layer = settings.layer;
}

static void restore_backup(void)
{
    /* we just let the calbacks of our dialog do all the work */

    /* this must be first */
    gtk_option_menu_set_history(GTK_OPTION_MENU(orientation_menu),
    				backup.orientation);

    gtk_option_menu_set_history(GTK_OPTION_MENU(size_menu), backup.size);

    gtk_option_menu_set_history(GTK_OPTION_MENU(popup_position_menu),
    				backup.popup_position);

    gtk_option_menu_set_history(GTK_OPTION_MENU(style_menu), backup.style);
    gtk_option_menu_set_history(GTK_OPTION_MENU(theme_menu),
                                backup_theme_index);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(groups_spin), backup.num_groups);
    
    gtk_option_menu_set_history(GTK_OPTION_MENU(layer_menu), backup.layer);

    position.x = backup_pos.x;
    position.y = backup_pos.y;
    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
}

/*  sections
*/
static void add_header(const char *text, GtkBox * box)
{
    GtkWidget *frame, *label;
    char *markup;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), header_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, FALSE, TRUE, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    markup = g_strconcat("<b>", text, "</b>", NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_show(label);
    gtk_container_add(GTK_CONTAINER(frame), label);
}

#define SKIP 12

static void add_spacer(GtkBox * box)
{
    GtkWidget *eventbox = gtk_alignment_new(0,0,0,0);

    gtk_widget_set_size_request(eventbox, SKIP, SKIP);
    gtk_widget_show(eventbox);
    gtk_box_pack_start(box, eventbox, FALSE, TRUE, 0);
}

/*  size
*/
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

static void add_size_menu(GtkWidget * option_menu, int size)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Tiny"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

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

    g_signal_connect(option_menu, "changed", G_CALLBACK(size_menu_changed),
                     NULL);
}

/*  style
*/
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

    if (settings.orientation == VERTICAL)
    {
	if(settings.popup_position == TOP || settings.popup_position == BOTTOM)
	    gtk_widget_set_sensitive(option_menu, FALSE);
    }
    else
    {
	if(settings.popup_position == LEFT || settings.popup_position == RIGHT)
	    gtk_widget_set_sensitive(option_menu, FALSE);
    }
}


/*
 * Panel Orientation
 */

static void orientation_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);
    int pos = settings.popup_position;

    if(n == settings.orientation)
        return;

    if ((n == HORIZONTAL && (pos == LEFT || pos == RIGHT)) ||
        (n == VERTICAL && (pos == TOP || pos == BOTTOM)))
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(style_menu), NEW_STYLE);
        gtk_widget_set_sensitive(style_menu, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive(style_menu, TRUE);
    }

    gtk_widget_set_sensitive(revert, TRUE);

    panel_set_orientation(n);
}

static void add_orientation_menu(GtkWidget * option_menu, int orientation)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Horizontal"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Vertical"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), orientation);

    g_signal_connect(option_menu, "changed",
                     G_CALLBACK(orientation_changed), NULL);

}


/*  popup position
*/
static void popup_position_changed(GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history(menu);

    if(n == settings.popup_position)
        return;

    if ((settings.orientation == HORIZONTAL && (n == LEFT || n == RIGHT)) ||
        (settings.orientation == VERTICAL && (n == TOP || n == BOTTOM)))
    {
        gtk_option_menu_set_history(GTK_OPTION_MENU(style_menu), NEW_STYLE);
        gtk_widget_set_sensitive(style_menu, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive(style_menu, TRUE);
    }

    panel_set_popup_position(n);
    panel_set_position();
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_popup_position_menu(GtkWidget * option_menu, int position)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label(_("Left"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Right"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Top"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_menu_item_new_with_label(_("Bottom"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), position);

    g_signal_connect(option_menu, "changed",
                     G_CALLBACK(popup_position_changed), NULL);
}

/*  theme
*/
static char **find_themes(void)
{
    char **themes = NULL;
    GList *list = NULL, *li;
    GDir *gdir;
    char **dirs, **d;
    const char *file;
    int i, len;

    /* Add default theme */
    list = g_list_append(list, g_strdup(DEFAULT_THEME));
    dirs = get_theme_dirs();

    for(d = dirs; *d; d++)
    {
        gdir = g_dir_open(*d, 0, NULL);

        if(gdir)
        {
            while((file = g_dir_read_name(gdir)))
            {
                char *path = g_build_filename(*d, file, NULL);

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

    len = g_list_length(list);

    themes = g_new0(char *, len + 1);

    for(i = 0, li = list; li; li = li->next, i++)
    {
	themes[i] = (char *)li->data;
    }

    g_list_free(list);
    g_strfreev(dirs);

    return themes;
}

static void theme_changed(GtkOptionMenu * option_menu)
{
    const char *theme;
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

    if(settings.theme && theme && strequal(theme, settings.theme))
        return;

    panel_set_theme(theme);
    gtk_widget_set_sensitive(revert, TRUE);
}

static void add_theme_menu(GtkWidget * option_menu, const char *theme)
{
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item;
    int i = 0, n = 0;
    char **themes = find_themes();
    char **s;

    for(i = 0, s = themes; *s; s++, i++)
    {
	item = gtk_menu_item_new_with_label(*s);
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	if(settings.theme && strequal(settings.theme, *s))
	    n = backup_theme_index = i;
    }

    g_strfreev(themes);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), n);

    g_signal_connect(option_menu, "changed", G_CALLBACK(theme_changed), NULL);
}

static int lastpage = 0;

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
    add_size_menu(size_menu, settings.size);
    gtk_box_pack_start(GTK_BOX(hbox), size_menu, TRUE, TRUE, 0);

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

    /* panel orientation */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel Orientation:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    orientation_menu = gtk_option_menu_new();
    gtk_widget_show(orientation_menu);
    add_orientation_menu(orientation_menu, settings.orientation);
    gtk_box_pack_start(GTK_BOX(hbox), orientation_menu, TRUE, TRUE, 0);


    /* popup orientation */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Popup position:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    popup_position_menu = gtk_option_menu_new();
    gtk_widget_show(popup_position_menu);
    add_popup_position_menu(popup_position_menu, settings.popup_position);
    gtk_box_pack_start(GTK_BOX(hbox), popup_position_menu, TRUE, TRUE, 0);

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
    add_theme_menu(theme_menu, settings.theme);
    gtk_box_pack_start(GTK_BOX(hbox), theme_menu, TRUE, TRUE, 0);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* panel groups and screen buttons */

static void spin_changed(GtkWidget * spin)
{
    int n;
    gboolean changed = FALSE;
    n = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));


    if(n != settings.num_groups)
    {
        panel_set_num_groups(n);
        changed = TRUE;
    }

    if(changed)
        gtk_widget_set_sensitive(revert, TRUE);
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

    /* groups */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel controls:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    groups_spin = gtk_spin_button_new_with_range(1, 2*NBGROUPS, 1);
    gtk_widget_show(groups_spin);
    gtk_box_pack_start(GTK_BOX(hbox), groups_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(groups_spin), settings.num_groups);
    g_signal_connect(groups_spin, "value-changed", G_CALLBACK(spin_changed),
                     NULL);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* position */
static void layer_changed(GtkWidget * om, gpointer data)
{
    int layer = gtk_option_menu_get_history(GTK_OPTION_MENU(om));

    if (settings.layer == layer)
	return;
    
    panel_set_layer(layer);
    gtk_widget_set_sensitive(revert, TRUE);
}

enum
{
    BOTTOMCENTER,
    TOPCENTER,
    LEFTCENTER,
    RIGHTCENTER,
    NUM_POSITIONS
};

static char *position_names[] = {
    N_("Bottom"),
    N_("Top"),
    N_("Left"),
    N_("Right"),
};

static void position_clicked(GtkWidget * button, GtkOptionMenu *om)
{
    int width, height;
    DesktopMargins margins;
    Screen *xscreen;
    
    xscreen = DefaultScreenOfDisplay(GDK_DISPLAY());
    netk_get_desktop_margins(xscreen, &margins);

    gtk_window_get_size(GTK_WINDOW(toplevel), &width, &height);
    
    switch (gtk_option_menu_get_history(om))
    {
	case TOPCENTER:
	    position.y = margins.top;
	    position.x = gdk_screen_width() / 2 - width / 2;
	    break;
	case LEFTCENTER:
	    position.x = margins.left;
	    position.y = gdk_screen_height() / 2 - height / 2;
	    break;
	case RIGHTCENTER:
	    position.x = gdk_screen_width() - width - margins.right;
	    position.y = gdk_screen_height() / 2 - height / 2;
	    break;
	default:
	    position.x = gdk_screen_width() / 2 - width / 2;
	    position.y = gdk_screen_height() - height - margins.bottom;
	    break;
    }
    
    gtk_window_move(GTK_WINDOW(toplevel), position.x, position.y);
    gtk_widget_set_sensitive(revert, TRUE);
}

/* restore position and set on top or not */
static void add_position_box(GtkBox * box)
{
    GtkWidget *frame, *vbox, *hbox, *label, *optionmenu, *menu;
    int i;

    /* frame and vbox */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), option_shadow);
    gtk_widget_show(frame);
    gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    /* checkbutton */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Panel layer:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    layer_menu = gtk_option_menu_new();
    gtk_widget_show(layer_menu);
    gtk_box_pack_start(GTK_BOX(hbox), layer_menu, FALSE, FALSE, 0);

    menu = gtk_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(layer_menu), menu);

    {
	GtkWidget *mi;

	mi = gtk_menu_item_new_with_label(_("Top"));
	gtk_widget_show(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

	mi = gtk_menu_item_new_with_label(_("Normal"));
	gtk_widget_show(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

	mi = gtk_menu_item_new_with_label(_("Bottom"));
	gtk_widget_show(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    
    gtk_option_menu_set_history(GTK_OPTION_MENU(layer_menu), settings.layer);
    g_signal_connect(layer_menu, "changed", G_CALLBACK(layer_changed), NULL);    

    /* centering */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Center the panel:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    optionmenu = gtk_option_menu_new();
    gtk_widget_show(optionmenu);
    gtk_box_pack_start(GTK_BOX(hbox), optionmenu, FALSE, FALSE, 0);

    menu = gtk_menu_new();
    gtk_widget_show(menu);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);

    for (i = 0; i < NUM_POSITIONS; i++)
    {
	GtkWidget *mi = gtk_menu_item_new_with_label(_(position_names[i]));

	gtk_widget_show(mi);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    
    gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu), 0);
    
    pos_button = mixed_button_new(GTK_STOCK_APPLY, _("Set"));
    gtk_widget_show(pos_button);
    gtk_box_pack_start(GTK_BOX(hbox), pos_button, FALSE, FALSE, 0);

    g_signal_connect(pos_button, "clicked", G_CALLBACK(position_clicked), optionmenu);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* the dialog */

static gboolean running = FALSE;
static GtkWidget *dialog = NULL;

void global_settings_dialog(void)
{
    GtkWidget *frame, *vbox, *button;
    GtkWidget *notebook, *label;
    char *markup;

    if(running)
    {
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }

    running = TRUE;

    create_backup();

    /* we may have to recreate the panel so safe the changes now */
    if (!disable_user_config)
	write_panel_config();

    dialog =
        gtk_dialog_new_with_buttons(_("XFce Panel Preferences"),
                                    GTK_WINDOW(toplevel),
/*                             GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,*/
                                    GTK_DIALOG_NO_SEPARATOR,
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

    /* main notebook */
    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook,
                       TRUE, TRUE, 0);

    /* first notebook page */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), main_shadow);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
    gtk_widget_show(frame);

    markup = g_strconcat("<span> ", _("General"), " </span>", NULL);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_show(label);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    add_header(_("Panel controls"), GTK_BOX(vbox));
    add_controls_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    add_header(_("Position"), GTK_BOX(vbox));
    add_position_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    g_object_unref(sg);

    /* second notebook page */
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), main_shadow);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
    gtk_widget_show(frame);

    markup = g_strconcat("<span> ", _("Appearance"), " </span>", NULL);
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_show(label);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, label);

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    /* Appearance */
    add_header(_("Appearance"), GTK_BOX(vbox));
    add_style_box(GTK_BOX(vbox));
    add_spacer(GTK_BOX(vbox));

    g_object_unref(sg);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), lastpage);
    
    while(1)
    {
        int response = GTK_RESPONSE_NONE;

        response = gtk_dialog_run(GTK_DIALOG(dialog));

        if(response == RESPONSE_REVERT)
        {
            restore_backup();
            panel_set_settings();
            gtk_widget_set_sensitive(revert, FALSE);
	    gtk_widget_grab_focus(button);
        }
        else
            break;
    }

    lastpage = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    
    gtk_widget_destroy(dialog);
    running = FALSE;
    dialog = NULL;

    write_panel_config();
}

