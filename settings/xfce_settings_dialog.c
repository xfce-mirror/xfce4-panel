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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfcegui4/dialogs.h>
#include <libxfcegui4/xfce_framebox.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#define STREQUAL(s1,s2) !strcmp(s1, s2)

#define BORDER 5

enum { LEFT, RIGHT, TOP, BOTTOM };

enum { HORIZONTAL, VERTICAL };


static McsManager *mcs_manager;

/*  Global settings
 *  ---------------
 *  size: option menu
 *  panel orientation: option menu
 *  popup position: option menu
 *  icon theme: option menu
*/
static GtkWidget *orientation_menu;
static GtkWidget *size_menu;
static GtkWidget *popup_position_menu;
static GtkWidget *theme_menu;

static GtkWidget *layer_menu;

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
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;
    McsSetting *setting;

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

/* Panel Orientation */
static void
orientation_changed (GtkOptionMenu * menu)
{
    int n = gtk_option_menu_get_history (menu);
    int pos;
    McsSetting *setting;
    
    setting = 
	mcs_manager_setting_lookup (mcs_manager, "orientation", CHANNEL);
    
    if (!setting || n == setting->data.v_int)
	return;

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_ORIENTATION],
	    		 CHANNEL, n);
/*    TODO: find out why this crashed the panel
 *    the error is in libxfce4mcs
 *    
 *    mcs_manager_notify(mcs_manager, CHANNEL);
 *
 *    gdk_flush();
 *    g_usleep(10);
*/

    setting = 
	mcs_manager_setting_lookup (mcs_manager, "popupposition", CHANNEL);
    
    if (!setting)
	return;

    pos = setting->data.v_int;

    /* this seems more logical */
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

    gtk_option_menu_set_history (GTK_OPTION_MENU (popup_position_menu), pos);
}

static void
add_orientation_menu (GtkWidget * option_menu)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;
    McsSetting *setting;

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
		      G_CALLBACK (orientation_changed), NULL);
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
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;
    McsSetting *setting;

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

    dirs[0] = g_build_filename (g_get_home_dir (), ".xfce4", "icons", NULL);
    dirs[1] = g_build_filename (DATADIR, "icons", NULL);

    for (d = dirs; *d; d++)
    {
	gdir = g_dir_open (*d, 0, NULL);

	if (gdir)
	{
	    while ((file = g_dir_read_name (gdir)))
	    {
		char *path = g_build_filename (*d, file, "index.theme", NULL);

		if (!g_list_find_custom (list, file, (GCompareFunc) strcmp) &&
		    g_file_test (path, G_FILE_TEST_EXISTS))
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
	themes[i] = (char *) li->data;
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

    /* Right, this is weird. Apparently the option menu
     * button reparents the label connected to the menuitem
     * that is selected. So to get to the label we have to go 
     * to the child of the button and not of the menu item!
     *
     * This took a while to find out :-)
     */
    label = gtk_bin_get_child (GTK_BIN (option_menu));

    theme = gtk_label_get_text (GTK_LABEL (label));

    mcs_manager_set_string (mcs_manager, xfce_settings_names[XFCE_THEME],
	    		    CHANNEL, theme);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_theme_menu (GtkWidget * option_menu)
{
    GtkWidget *menu = gtk_menu_new ();
    GtkWidget *item;
    int i = 0, n = 0;
    char **themes = find_themes ();
    char **s;
    char *theme;
    McsSetting *setting;

    setting = mcs_manager_setting_lookup (mcs_manager,
	    				  xfce_settings_names[XFCE_THEME],
					  CHANNEL);
    
    if (setting)
    {
	theme = setting->data.v_string;
    }
    else
    {
	g_warning ("Theme setting not available");
	return;
    }
    
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

    g_signal_connect (option_menu, "changed", G_CALLBACK (theme_changed),
		      NULL);
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
    add_size_menu (size_menu);
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
    add_orientation_menu (orientation_menu);
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
    add_popup_position_menu (popup_position_menu);
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
    add_theme_menu (theme_menu);
    gtk_box_pack_start (GTK_BOX (hbox), theme_menu, TRUE, TRUE, 0);
}

/* layer and popup position */
static void
layer_changed (GtkWidget * om, gpointer data)
{
    int layer = gtk_option_menu_get_history (GTK_OPTION_MENU (om));

    mcs_manager_set_int (mcs_manager, xfce_settings_names[XFCE_LAYER],
	    		 CHANNEL, layer);
    mcs_manager_notify (mcs_manager, CHANNEL);
}

static void
add_layer_box (GtkBox * box, GtkSizeGroup * sg)
{
    GtkWidget *hbox, *label, *menu;
    McsSetting *setting;

    /* checkbutton */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Panel layer:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    layer_menu = gtk_option_menu_new ();
    gtk_widget_show (layer_menu);
    gtk_box_pack_start (GTK_BOX (hbox), layer_menu, TRUE, TRUE, 0);

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

    setting = mcs_manager_setting_lookup (mcs_manager, 
	    				  xfce_settings_names[XFCE_LAYER],
					  CHANNEL);
    
    if (setting)
    {
	gtk_option_menu_set_history (GTK_OPTION_MENU (layer_menu), 
				     setting->data.v_int);
    }

    g_signal_connect (layer_menu, "changed", G_CALLBACK (layer_changed),
		      NULL);
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
add_autohide_box (GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *label, *check;
    McsSetting *setting;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Autohide:"));
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
    GdkPixbuf *pb;

    if (is_running)
    {
	gtk_window_present (GTK_WINDOW (dialog));
	return;
    }

    is_running = TRUE;

    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    mcs_manager = mp->manager;

    dialog = gtk_dialog_new_with_buttons (_("Xfce Panel"),
					  NULL, GTK_DIALOG_NO_SEPARATOR,
					  GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
					  NULL);

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    gtk_window_stick (GTK_WINDOW (dialog));
    gtk_window_set_icon (GTK_WINDOW (dialog), mp->icon);

    g_signal_connect (dialog, "response", G_CALLBACK (dialog_delete), NULL);
    g_signal_connect (dialog, "delete_event", G_CALLBACK (dialog_delete),
		      NULL);

    /* pretty header */
    vbox = GTK_DIALOG (dialog)->vbox;

    pb = gdk_pixbuf_scale_simple (mp->icon, 32, 32, GDK_INTERP_HYPER);
    header = xfce_create_header (pb, _("Xfce Panel Settings"));
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, TRUE, 0);
    g_object_unref (pb);

    add_spacer (GTK_BOX (vbox), BORDER);

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

    /* autohide */
    vbox = GTK_DIALOG (dialog)->vbox;

    frame = xfce_framebox_new (_("Behaviour"), TRUE);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    xfce_framebox_add (XFCE_FRAMEBOX (frame), vbox);

    add_layer_box (GTK_BOX (vbox), sg);

    add_autohide_box (GTK_BOX (vbox), sg);

    g_object_unref (sg);

    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_widget_show (dialog);
}
