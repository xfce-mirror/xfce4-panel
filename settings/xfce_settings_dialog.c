/*  xfce4
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

#include <libxfce4util/i18n.h>
#include <libxfcegui4/dialogs.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#define strequals(s1,s2) !strcmp(s1, s2)

#define BORDER 5

/* panel sides / popup orientation */
enum
{ LEFT, RIGHT, TOP, BOTTOM };

/* panel orientation */
enum
{ HORIZONTAL, VERTICAL };

static McsManager *mcs_manager;

/*  Global settings
 *  ---------------
 *  size: option menu
 *  panel orientation: option menu
 *  popup position: option menu
 *  icon theme: option menu
 *  position: option menu + button (centering only)
*/
static GtkWidget *orientation_menu;
static GtkWidget *size_menu;
static GtkWidget *popup_position_menu;
static GtkWidget *theme_menu;

static GtkWidget *layer_menu;
static GtkWidget *pos_button;

GtkShadowType main_shadow = GTK_SHADOW_NONE;
GtkShadowType header_shadow = GTK_SHADOW_NONE;
GtkShadowType option_shadow = GTK_SHADOW_NONE;

static gboolean is_running = FALSE;
static GtkWidget *dialog = NULL;

/* stop gcc from complaining when using -Wall:
 * this variable will not be used, but I want the 
 * definition of names to be available in the 
 * xfce-settings.h header for other modules */
char **names = xfce_settings_names;

/* useful widgets */
#define SKIP BORDER

static void
add_spacer (GtkBox * box)
{
    GtkWidget *eventbox = gtk_alignment_new (0, 0, 0, 0);

    gtk_widget_set_size_request (eventbox, SKIP, SKIP);
    gtk_widget_show (eventbox);
    gtk_box_pack_start (box, eventbox, FALSE, TRUE, 0);
}

/* size */
static void
size_menu_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);
    McsSetting *setting = &xfce_options[XFCE_SIZE];

    if (n == setting->data.v_int)
        return;

    setting->data.v_int = n;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_size_menu (GtkWidget * option_menu, int size)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label (_("Tiny"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Small"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Medium"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Large"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), size);

    g_signal_connect (option_menu, "changed", G_CALLBACK (size_menu_changed),
                      NULL);
}

/* Panel Orientation */
static void
orientation_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);
    int pos = xfce_options[XFCE_POPUPPOSITION].data.v_int;
    McsSetting *setting = &xfce_options[XFCE_ORIENTATION];

    if (n == setting->data.v_int)
        return;

    setting->data.v_int = n;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);

    /* this seems more logical */
    switch (pos)
    {
        case LEFT:
            pos = BOTTOM;
            break;
        case RIGHT:
            pos = TOP;
            break;
        case TOP:
            pos = RIGHT;
            break;
        case BOTTOM:
            pos = LEFT;
            break;
    }

    gtk_option_menu_set_history (GTK_OPTION_MENU (popup_position_menu), pos);
}

static void
add_orientation_menu (GtkWidget * option_menu, int orientation)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;

    item = gtk_menu_item_new_with_label (_("Horizontal"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Vertical"));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), orientation);

    g_signal_connect (option_menu, "changed",
                      G_CALLBACK (orientation_changed), NULL);
}

/* popup position */
static void
popup_position_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);
    McsSetting *setting = &xfce_options[XFCE_POPUPPOSITION];

    if (n == setting->data.v_int)
        return;

    setting->data.v_int = n;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_popup_position_menu (GtkWidget * option_menu, int position)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;

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
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), position);

    g_signal_connect (option_menu, "changed",
                      G_CALLBACK (popup_position_changed), NULL);
}

/*  theme
*/
static char **
find_themes (void)
{
    char **themes = NULL;
    GList *list = NULL, *li;
    GDir *gdir;
    char **dirs, **d;
    const char *file;
    int i, len;

    /* Add default theme */
    dirs = g_new0 (char *, 3);

    dirs[0] = g_build_filename (g_get_home_dir (), ".xfce4", "themes", NULL);
    dirs[1] = g_build_filename (DATADIR, "themes", NULL);

    for (d = dirs; *d; d++)
    {
        gdir = g_dir_open (*d, 0, NULL);

        if (gdir)
        {
            while ((file = g_dir_read_name (gdir)))
            {
                char *path = g_build_filename (*d, file, NULL);

                if (!g_list_find_custom (list, file, (GCompareFunc) strcmp) &&
                    g_file_test (path, G_FILE_TEST_IS_DIR))
                {
                    list = g_list_append (list, g_strdup (file));
                }

                g_free (path);
            }

            g_dir_close (gdir);
        }
    }

    len = g_list_length (list);

    themes = g_new0 (char *, len + 1);

    for (i = 0, li = list; li; li = li->next, i++)
    {
        themes[i] = (char *)li->data;
    }

    g_list_free (list);
    g_strfreev (dirs);

    return themes;
}

static void
theme_changed (GtkOptionMenu * option_menu)
{
    const char *theme;
    GtkWidget *label;
    McsSetting *setting = &xfce_options[XFCE_THEME];

    /* Right, this is weird, apparently the option menu
     * button reparents the label connected to the menuitem
     * that is selected. So to get to the label we have to go 
     * to the child of the button and not of the menu item!
     *
     * This took a while to find out :-)
     */
    label = gtk_bin_get_child (GTK_BIN (option_menu));

    theme = gtk_label_get_text (GTK_LABEL (label));

    if (strequals (theme, setting->data.v_string))
        return;

    g_free (setting->data.v_string);
    setting->data.v_string = g_strdup (theme);
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_theme_menu (GtkWidget * option_menu, const char *theme)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;
    int i = 0, n = 0;
    char **themes = find_themes ();
    char **s;

    for (i = 0, s = themes; *s; s++, i++)
    {
        item = gtk_menu_item_new_with_label (*s);
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        if (strcmp (theme, *s) == 0)
            n = i;
    }

    g_strfreev (themes);

    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), n);

    g_signal_connect (option_menu, "changed", G_CALLBACK (theme_changed), NULL);
}

static void
add_style_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *vbox, *hbox, *label;

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (box, vbox, TRUE, TRUE, 0);

    /* size */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel size:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    size_menu = gtk_option_menu_new ();
    gtk_widget_show (size_menu);
    add_size_menu (size_menu, xfce_options[XFCE_SIZE].data.v_int);
    gtk_box_pack_start (GTK_BOX (hbox), size_menu, TRUE, TRUE, 0);

    /* panel orientation */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel orientation:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    orientation_menu = gtk_option_menu_new ();
    gtk_widget_show (orientation_menu);
    add_orientation_menu (orientation_menu,
                          xfce_options[XFCE_ORIENTATION].data.v_int);
    gtk_box_pack_start (GTK_BOX (hbox), orientation_menu, TRUE, TRUE, 0);

    /* popup button */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Popup position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    popup_position_menu = gtk_option_menu_new ();
    gtk_widget_show (popup_position_menu);
    add_popup_position_menu (popup_position_menu,
                             xfce_options[XFCE_POPUPPOSITION].data.v_int);
    gtk_box_pack_start (GTK_BOX (hbox), popup_position_menu, TRUE, TRUE, 0);

    /* icon theme */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Icon theme:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    theme_menu = gtk_option_menu_new ();
    gtk_widget_show (theme_menu);
    add_theme_menu (theme_menu, xfce_options[XFCE_THEME].data.v_string);
    gtk_box_pack_start (GTK_BOX (hbox), theme_menu, TRUE, TRUE, 0);
}

/* position */
static void
layer_changed (GtkWidget * om, gpointer data)
{
    int layer;
    McsSetting *setting = &xfce_options[XFCE_LAYER];

    layer = gtk_option_menu_get_history (GTK_OPTION_MENU (om));

    if (setting->data.v_int == layer)
        return;

    setting->data.v_int = layer;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static char *position_names[] = {
    N_("Bottom"),
    N_("Top"),
    N_("Left"),
    N_("Right"),
};

static void
position_clicked (GtkWidget * button, GtkOptionMenu * om)
{
    int n;
    McsSetting *setting = &xfce_options[XFCE_POSITION];

    n = gtk_option_menu_get_history (om);

    /* make sure it gets changed */
    setting->data.v_int = XFCE_POSITION_NONE;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
    gdk_flush ();
    g_usleep (10);

    setting->data.v_int = n;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_position_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *vbox, *hbox, *label, *optionmenu, *menu;
    int i;

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (box, vbox, TRUE, TRUE, 0);

    /* checkbutton */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel layer:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    layer_menu = gtk_option_menu_new ();
    gtk_widget_show (layer_menu);
    gtk_box_pack_start (GTK_BOX (hbox), layer_menu, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (layer_menu), menu);

    {
        GtkWidget *mi;

        mi = gtk_menu_item_new_with_label (_("Top"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_menu_item_new_with_label (_("Normal"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_menu_item_new_with_label (_("Bottom"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    gtk_option_menu_set_history (GTK_OPTION_MENU (layer_menu),
                                 xfce_options[XFCE_LAYER].data.v_int);

    g_signal_connect (layer_menu, "changed", G_CALLBACK (layer_changed), NULL);

    /* centering */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Center the panel:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    optionmenu = gtk_option_menu_new ();
    gtk_widget_show (optionmenu);
    gtk_box_pack_start (GTK_BOX (hbox), optionmenu, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    gtk_widget_show (menu);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

    for (i = 0; i < 4; i++)
    {
        GtkWidget *mi = gtk_menu_item_new_with_label (_(position_names[i]));

        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), 0);

    pos_button = mixed_button_new (GTK_STOCK_APPLY, _("Set"));
    GTK_WIDGET_SET_FLAGS (pos_button, GTK_CAN_DEFAULT);
    gtk_widget_show (pos_button);
    gtk_box_pack_start (GTK_BOX (hbox), pos_button, FALSE, FALSE, 0);

    g_signal_connect (pos_button, "clicked", G_CALLBACK (position_clicked),
                      optionmenu);
}

/* autohide */
static void
autohide_changed (GtkToggleButton *tb)
{
    int hide;
    McsSetting *setting = &xfce_options[XFCE_AUTOHIDE];

    hide = gtk_toggle_button_get_active(tb) ? 1 : 0;

    if (setting->data.v_int == hide)
        return;

    setting->data.v_int = hide;
    mcs_manager_set_setting (mcs_manager, setting, CHANNEL);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_autohide_box(GtkContainer *frame)
{
    GtkWidget *hbox, *label, *check;

    hbox = gtk_hbox_new(FALSE, BORDER);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), BORDER);
    gtk_widget_show(hbox);
    gtk_container_add (frame, hbox);

    label = gtk_label_new(_("Autohide:"));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    check = gtk_check_button_new();
    gtk_widget_show(check);
    gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), xfce_options[XFCE_AUTOHIDE].data.v_int == 1);

    g_signal_connect(check, "toggled", G_CALLBACK(autohide_changed), NULL);
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
    GtkWidget *header, *hbox, *frame, *vbox, *vbox2;
    GtkSizeGroup *sg;

    if (is_running)
    {
        gtk_window_present (GTK_WINDOW (dialog));
        return;
    }

    is_running = TRUE;

#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    mcs_manager = mp->manager;

    dialog = gtk_dialog_new_with_buttons (_("XFce Panel"),
                                          NULL, GTK_DIALOG_NO_SEPARATOR,
                                          GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                          NULL);

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_window_set_icon(GTK_WINDOW (dialog), mp->icon);

    g_signal_connect (dialog, "response", G_CALLBACK (dialog_delete), NULL);
    g_signal_connect (dialog, "delete_event", G_CALLBACK (dialog_delete), NULL);

    /* pretty header */
    vbox = GTK_DIALOG (dialog)->vbox;
    header = create_header (mp->icon, _("XFce Panel Settings"));
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 0);

    add_spacer (GTK_BOX (vbox));

    /* hbox */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER + 1);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    /* Appearance */
    frame = gtk_frame_new (_("Appearance"));
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    add_style_box (GTK_BOX (vbox), sg);

    g_object_unref (sg);

    /* second column */
    vbox2 = gtk_vbox_new(FALSE, BORDER);
    gtk_widget_show(vbox2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
    
    /* Position */
    frame = gtk_frame_new (_("Position"));
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    add_position_box (GTK_BOX (vbox), sg);

    g_object_unref (sg);

    /* autohide */
    frame = gtk_frame_new (_("Behaviour"));
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);

    add_autohide_box (GTK_CONTAINER (frame));

    gtk_widget_show (dialog);
}
