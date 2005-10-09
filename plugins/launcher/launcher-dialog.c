/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "launcher-dialog.h"


typedef struct
{
    XfcePanelPlugin *plugin;
    LauncherPlugin *launcher;

    GtkWidget *dlg;
    
    /* launchers */
    GtkWidget *tree;
    GtkWidget *scroll;

    GtkWidget *up;
    GtkWidget *down;
    GtkWidget *add;
    GtkWidget *remove;
    
    /* edit current entry */
    gboolean updating;

    LauncherEntry *entry;
    
    GtkWidget *exec_name;
    GtkWidget *exec_comment;

    GtkWidget *icon_button;
    GtkWidget *icon_img;
    GtkWidget *icon_category;
    GtkWidget *icon_file_align;
    GtkWidget *icon_file;
    GtkWidget *icon_browse;
    GtkWidget *icon_label;

    GtkWidget *exec;
    GtkWidget *exec_browse;
    GtkWidget *exec_terminal;
    GtkWidget *exec_startup;
}
LauncherDialog;

/* Keep in sync with XfceIconTheme */
static const char *category_icons [] = {
    N_("Default"),
    N_("Editor"),
    N_("File management"),
    N_("Utilities"),
    N_("Games"),
    N_("Help browser"),
    N_("Multimedia"),
    N_("Network"),
    N_("Graphics"),
    N_("Printer"),
    N_("Productivity"),
    N_("Sound"),
    N_("Terminal")
};

/* DND */
static void entry_dialog_data_received (GtkWidget *w, 
                                        GdkDragContext *context, 
                                        gint x, 
                                        gint y, 
                                        GtkSelectionData *data, 
                                        guint info, 
                                        guint time, 
                                        LauncherDialog *ld);
        
static void launcher_dialog_data_received (GtkWidget *w, 
                                           GdkDragContext *context, 
                                           gint x, 
                                           gint y, 
                                           GtkSelectionData *data, 
                                           guint info, 
                                           guint time, 
                                           LauncherDialog *ld);

/* File dialog with preview */
static char *select_file_with_preview (const char *title, 
                                       const char *path,
                                       GtkWidget * parent,
                                       gboolean with_preview);


/* LauncherEntry Properties *
 * ------------------------ */

/* update entry from dialog */
static void
update_entry_info (LauncherDialog *ld)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ld->exec_name));

    if (!text || !strlen (text))
    {
        if (ld->entry->name)
        {
            g_free (ld->entry->name);
            ld->entry->name = NULL;
        }
    }
    else if (!ld->entry->name || strcmp (text, ld->entry->name) != 0)
    {
        g_free (ld->entry->name);
        ld->entry->name = g_strdup (text);
    }

    text = gtk_entry_get_text (GTK_ENTRY (ld->exec_comment));

    if (!text || !strlen (text))
    {
        if (ld->entry->comment)
        {
            g_free (ld->entry->comment);
            ld->entry->comment = NULL;
        }
    }
    else if (!ld->entry->comment || strcmp (text, ld->entry->comment) != 0)
    {
        g_free (ld->entry->comment);
        ld->entry->comment = g_strdup (text);
    }

    gtk_widget_queue_draw (ld->tree);
}

static void
update_entry_icon (LauncherDialog *ld)
{
    const char *text;
    GdkPixbuf *pb;

    text = gtk_entry_get_text (GTK_ENTRY (ld->icon_file));
        
    if (!text || !strlen (text))
    {
        if (ld->entry->icon.type == LAUNCHER_ICON_TYPE_NAME)
        {
            g_free (ld->entry->icon.icon.name);
            ld->entry->icon.icon.name = NULL;
        }

        ld->entry->icon.type = LAUNCHER_ICON_TYPE_NONE;

        pb = launcher_icon_load_pixbuf (&ld->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
        g_object_unref (pb);
    }
    else if (ld->entry->icon.type != LAUNCHER_ICON_TYPE_NAME || 
             strcmp (text, ld->entry->icon.icon.name) != 0)
    {
        if (ld->entry->icon.type == LAUNCHER_ICON_TYPE_NAME)
            g_free (ld->entry->icon.icon.name);

        ld->entry->icon.type = LAUNCHER_ICON_TYPE_NAME;
        ld->entry->icon.icon.name = g_strdup (text);
        
        pb = launcher_icon_load_pixbuf (&ld->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
        g_object_unref (pb);
    }

    gtk_widget_queue_draw (ld->tree);
}

static void
update_entry_exec (LauncherDialog *ld)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ld->exec));

    if (!text || !strlen (text))
    {
        if (ld->entry->exec)
        {
            g_free (ld->entry->exec);
            ld->entry->exec = NULL;
        }
    }
    else if (!ld->entry->exec || strcmp (text, ld->entry->exec) != 0)
    {
        g_free (ld->entry->exec);
        ld->entry->exec = g_strdup (text);
    }
}

/* text entries */
static gboolean
entry_lost_focus (GtkWidget *gentry, GdkEventFocus *ev, LauncherDialog *ld)
{
    if (ld->updating)
        return FALSE;
    
    if (gentry == ld->exec_name || gentry == ld->exec_comment)
    {
        update_entry_info (ld);
    }
    else if (gentry == ld->icon_file)
    {
        update_entry_icon (ld);
    }
    else if (gentry == ld->exec)
    {
        update_entry_exec (ld);
    }

    return FALSE;
}

/* toggle buttons */
static void
check_button_toggled (GtkWidget *tb, LauncherDialog *ld)
{
    if (ld->updating)
        return;
    
    if (tb == ld->exec_terminal)
        ld->entry->terminal = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
    else if (tb == ld->exec_startup)
        ld->entry->startup = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
}

/* icon callbacks */
static void
set_panel_icon (LauncherDialog *ld)
{
    GdkPixbuf *pb;
    LauncherEntry *entry = g_ptr_array_index (ld->launcher->entries, 0);

    pb = launcher_icon_load_pixbuf (&entry->icon, PANEL_ICON_SIZE);
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (ld->launcher->iconbutton),
                                pb);
    g_object_unref (pb);
}

static void
icon_menu_deactivated (GtkWidget *menu, LauncherDialog *ld)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ld->icon_button), FALSE);
}

static void
position_icon_menu (GtkMenu * menu, int *x, int *y, gboolean * push_in, 
                    GtkWidget *b)
{
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    gdk_window_get_origin (b->window, x, y);

    *x += b->allocation.x;
    *y += b->allocation.y - req.height;

    screen = gtk_widget_get_screen (b);

    num = gdk_screen_get_monitor_at_window (screen, b->window);

    gdk_screen_get_monitor_geometry (screen, num, &geom);

    if (*x > geom.x + geom.width - req.width)
        *x = geom.x + geom.width - req.width;
    if (*x < geom.x)
        *x = geom.x;

    if (*y > geom.y + geom.height - req.height)
        *y = geom.y + geom.height - req.height;
    if (*y < geom.y)
        *y = geom.y;
}

static void
popup_icon_menu (GtkWidget *tb, LauncherDialog *ld)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
    {
        gtk_menu_popup (GTK_MENU (ld->icon_category), NULL, NULL, 
                        (GtkMenuPositionFunc) position_icon_menu, tb,
                        0, gtk_get_current_event_time ());
    }
}

static void
icon_menu_activated (GtkWidget *mi, LauncherDialog *ld)
{
    int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), "category"));

    n = CLAMP (n, 0, NUM_CATEGORIES);

    if (ld->entry->icon.type != LAUNCHER_ICON_TYPE_CATEGORY 
        || ld->entry->icon.icon.category != n)
    {
        GdkPixbuf *pb;
        
        gtk_label_set_text (GTK_LABEL (ld->icon_label), _(category_icons[n]));
        gtk_widget_hide (ld->icon_file_align);
        gtk_widget_show (ld->icon_label);
        
        if (ld->entry->icon.type == LAUNCHER_ICON_TYPE_NAME)
            g_free (ld->entry->icon.icon.name);
        ld->entry->icon.icon.name = NULL;

        ld->entry->icon.type = LAUNCHER_ICON_TYPE_CATEGORY;
        ld->entry->icon.icon.category = (XfceIconThemeCategory) n;

        pb = launcher_icon_load_pixbuf (&ld->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
        g_object_unref (pb);

        if (ld->entry == g_ptr_array_index (ld->launcher->entries, 0))
            set_panel_icon (ld);
    }

    gtk_widget_queue_draw (ld->tree);
}

static void
icon_browse (GtkWidget *b, LauncherDialog *ld)
{
    char *file, *path;
    
    path = 
        (ld->entry->icon.type == LAUNCHER_ICON_TYPE_NAME 
         && g_path_is_absolute (ld->entry->icon.icon.name)) ?
                ld->entry->icon.icon.name : NULL;
    
    file = select_file_with_preview (_("Select image file"), path, ld->dlg, 
                                     TRUE);

    if (file && g_file_test (file, G_FILE_TEST_IS_REGULAR))
    {
        GdkPixbuf *pb;
        
        gtk_entry_set_text (GTK_ENTRY (ld->icon_file), file);
        gtk_editable_set_position (GTK_EDITABLE (ld->icon_file), -1);
        update_entry_icon (ld);

        pb = launcher_icon_load_pixbuf (&ld->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
        g_object_unref (pb);

        if (ld->entry == g_ptr_array_index (ld->launcher->entries, 0))
            set_panel_icon (ld);
    }

    g_free (file);

    gtk_widget_queue_draw (ld->tree);
}

static void
icon_menu_browse (GtkWidget *mi, LauncherDialog *ld)
{
    gtk_widget_hide (ld->icon_label);
    gtk_widget_show (ld->icon_file_align);

    update_entry_icon (ld);

    if (ld->entry->icon.type != LAUNCHER_ICON_TYPE_NAME || 
        !ld->entry->icon.icon.name)
    {
        icon_browse (NULL, ld);
    }
}

/* exec callback */
static void
exec_browse (GtkWidget *b, LauncherDialog *ld)
{
    char *file;
    
    file = select_file_with_preview (_("Select command"), ld->entry->exec, 
                                     ld->dlg, FALSE);

    if (file)
    {
        gtk_entry_set_text (GTK_ENTRY (ld->exec), file);
        gtk_editable_set_position (GTK_EDITABLE (ld->exec), -1);
        update_entry_exec (ld);
    }

    g_free (file);
}

/* info widgets */
static void
add_entry_info_options(LauncherDialog *ld, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *label;
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Name"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->exec_name = gtk_entry_new ();
    gtk_widget_show (ld->exec_name);
    gtk_box_pack_start (GTK_BOX (hbox), ld->exec_name, TRUE, TRUE, 0);

    g_signal_connect (ld->exec_name, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ld);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Description"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->exec_comment = gtk_entry_new ();
    gtk_widget_show (ld->exec_comment);
    gtk_box_pack_start (GTK_BOX (hbox), ld->exec_comment, TRUE, TRUE, 0);
    gtk_widget_set_size_request (ld->exec_comment, 300, -1);
    
    g_signal_connect (ld->exec_comment, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ld);
}

/* icon widgets */
static GtkWidget *
create_icon_category_menu (LauncherDialog *ld)
{
    GtkWidget *menu, *mi, *img;
    GdkPixbuf *pb;
    int i;
    LauncherIcon icon;

    menu = gtk_menu_new ();
    
    g_signal_connect (menu, "deactivate", 
                      G_CALLBACK (icon_menu_deactivated), ld);
    
    icon.type = LAUNCHER_ICON_TYPE_CATEGORY;
    
    for (i = 1; i < NUM_CATEGORIES; ++i)
    {
        mi = gtk_image_menu_item_new_with_label (_(category_icons[i]));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        g_object_set_data (G_OBJECT (mi), "category", GINT_TO_POINTER (i));

        g_signal_connect (mi, "activate", 
                          G_CALLBACK (icon_menu_activated), ld);

        icon.icon.category = i;
        pb = launcher_icon_load_pixbuf (&icon, MENU_ICON_SIZE);
        img = gtk_image_new_from_pixbuf (pb);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
        g_object_unref (pb);
    }

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_menu_item_new_with_label (_("Other..."));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    g_signal_connect (mi, "activate", 
                      G_CALLBACK (icon_menu_browse), ld);
    
    return menu;
}

static void
add_entry_icon_options (LauncherDialog *ld, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *hbox2, *arrow, *align, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    ld->icon_button = gtk_toggle_button_new ();
    gtk_widget_show (ld->icon_button);
    gtk_box_pack_start (GTK_BOX (hbox), ld->icon_button, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, ld->icon_button);

    ld->icon_category = create_icon_category_menu (ld);

    g_signal_connect (ld->icon_button, "toggled", 
                      G_CALLBACK (popup_icon_menu), ld);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (ld->icon_button), hbox2);

    ld->icon_img = gtk_image_new ();
    gtk_widget_show (ld->icon_img);
    gtk_box_pack_start (GTK_BOX (hbox2), ld->icon_img, TRUE, TRUE, 0);

    arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_IN);
    gtk_widget_show (arrow);
    gtk_box_pack_start (GTK_BOX (hbox2), arrow, TRUE, TRUE, 0);
    
    align = ld->icon_file_align = gtk_alignment_new (0, 0.5, 1, 0);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (align), hbox2);

    ld->icon_file = gtk_entry_new ();
    gtk_widget_show (ld->icon_file);
    gtk_box_pack_start (GTK_BOX (hbox2), ld->icon_file, TRUE, TRUE, 0);
    
    g_signal_connect (ld->icon_file, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ld);
    
    ld->icon_browse = gtk_button_new ();
    gtk_widget_show (ld->icon_browse);
    gtk_box_pack_start (GTK_BOX (hbox2), ld->icon_browse, FALSE, FALSE, 0);

    g_signal_connect (ld->icon_browse, "clicked", G_CALLBACK (icon_browse), 
                      ld);

    img = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (ld->icon_browse), img);
    
    ld->icon_label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (ld->icon_label), 0, 0.5);

    gtk_box_pack_start (GTK_BOX (hbox), ld->icon_label, TRUE, TRUE, 0);
}

/* exec widgets */
static void
add_entry_exec_options (LauncherDialog *ld, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *label, *button, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Command"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ld->exec = gtk_entry_new ();
    gtk_widget_show (ld->exec);
    gtk_box_pack_start (GTK_BOX (hbox), ld->exec, TRUE, TRUE, 0);
    
    g_signal_connect (ld->exec, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ld);

    ld->exec_browse = button = gtk_button_new ();
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    g_signal_connect (ld->exec_browse, "clicked", G_CALLBACK (exec_browse), 
                      ld);

    img = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (button), img);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ld->exec_terminal = 
        gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
    gtk_widget_show (ld->exec_terminal);
    gtk_box_pack_start (GTK_BOX (hbox), ld->exec_terminal, TRUE, TRUE, 0);

    g_signal_connect (ld->exec_terminal, "toggled",
                      G_CALLBACK (check_button_toggled), ld);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ld->exec_startup = 
        gtk_check_button_new_with_mnemonic (_("Use _startup notification"));
    gtk_widget_show (ld->exec_startup);
    gtk_box_pack_start (GTK_BOX (hbox), ld->exec_startup, TRUE, TRUE, 0);

    g_signal_connect (ld->exec_startup, "toggled",
                      G_CALLBACK (check_button_toggled), ld);
}

/* entry properties */
static void
launcher_dialog_update_entry_properties (LauncherDialog *ld)
{
    char *value;
    GdkPixbuf *pb;
    
    ld->updating = TRUE;

    value = ld->entry->name ? ld->entry->name : "";
    gtk_entry_set_text (GTK_ENTRY (ld->exec_name), value);
    
    value = ld->entry->comment ? ld->entry->comment : "";
    gtk_entry_set_text (GTK_ENTRY (ld->exec_comment), value);
    
    value = ld->entry->exec ? ld->entry->exec : "";
    gtk_entry_set_text (GTK_ENTRY (ld->exec), value);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ld->exec_terminal), 
                                  ld->entry->terminal);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ld->exec_startup), 
                                  ld->entry->startup);

    pb = launcher_icon_load_pixbuf (&ld->entry->icon, DLG_ICON_SIZE);
    gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
    g_object_unref (pb);

    if (ld->entry->icon.type != LAUNCHER_ICON_TYPE_CATEGORY)
        gtk_widget_show (ld->icon_file_align);
    else
        gtk_widget_hide (ld->icon_file_align);
    
    if (ld->entry->icon.type == LAUNCHER_ICON_TYPE_NAME && 
        ld->entry->icon.icon.name)
    {
        gtk_entry_set_text (GTK_ENTRY (ld->icon_file), 
                            ld->entry->icon.icon.name);
    }
    else
    {
        gtk_entry_set_text (GTK_ENTRY (ld->icon_file), "");
    }

    if (ld->entry->icon.type == LAUNCHER_ICON_TYPE_CATEGORY)
    {
        gtk_label_set_text (GTK_LABEL (ld->icon_label), 
                _(category_icons [ld->entry->icon.icon.category]));
        gtk_widget_show (ld->icon_label);
    }
    else
    {
        gtk_widget_hide (ld->icon_label);
    }
    
    ld->updating = FALSE;
}

static void
launcher_dialog_add_entry_properties (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *frame, *vbox, *align;
    GtkSizeGroup *sg;

    frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (box), frame, TRUE, TRUE, 0);
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    add_entry_info_options (ld, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_icon_options (ld, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_exec_options (ld, GTK_BOX (vbox), sg);
    
    g_object_unref (sg);
    
    launcher_dialog_update_entry_properties (ld);
    
    launcher_set_drag_dest (frame);
    g_signal_connect (frame, "drag-data-received",
                      G_CALLBACK (entry_dialog_data_received), ld);
}


/* LauncherPlugin Dialog *
 * --------------------- */

/* treeview */
static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

static void
cursor_changed (GtkTreeView * tv, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    LauncherEntry *e;
    
    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (e)
    {
        int i;
        
        ld->entry = e;

	gtk_widget_set_sensitive (ld->up, TRUE);
	gtk_widget_set_sensitive (ld->down, TRUE);
        gtk_widget_set_sensitive (ld->remove, TRUE);

        for (i = 0; i < ld->launcher->entries->len; ++i)
        {
            LauncherEntry *tmp = g_ptr_array_index (ld->launcher->entries, i);

            if (tmp != e)
                continue;
            
            if (i == 0)
            {
                gtk_widget_set_sensitive (ld->remove, FALSE);
                gtk_widget_set_sensitive (ld->up, FALSE);
            }
            
            if (i == ld->launcher->entries->len - 1)
                gtk_widget_set_sensitive (ld->down, FALSE);

            break;
        }
    }
    else
    {
	gtk_widget_set_sensitive (ld->up, FALSE);
	gtk_widget_set_sensitive (ld->down, FALSE);
	gtk_widget_set_sensitive (ld->remove, FALSE);
    }

    launcher_dialog_update_entry_properties (ld);
}

static void
render_icon (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
    LauncherEntry *entry;
    GdkPixbuf *pb;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
    {
        pb = launcher_icon_load_pixbuf (&entry->icon, DLG_ICON_SIZE);
        g_object_set (cell, "pixbuf", pb, NULL);
        g_object_unref (pb);
    }
    else
    {
        g_object_set (cell, "pixbuf", NULL, NULL);
    }
}

static void
render_text (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, GtkWidget * treeview)
{
    LauncherEntry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
        g_object_set (cell, "markup", entry->name, NULL);
    else
        g_object_set (cell, "markup", "", NULL);
}

static void
launcher_dialog_add_item_tree (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *tv;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    LauncherEntry *e;
    int i;

    ld->scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (ld->scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ld->scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (box, ld->scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    ld->tree = tv = gtk_tree_view_new_with_model (model);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), FALSE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (ld->scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

    g_object_unref (G_OBJECT (store));

    launcher_set_drag_dest (tv);
    g_signal_connect (tv, "drag-data-received", 
                      G_CALLBACK (launcher_dialog_data_received), ld);
    
    /* create the view */
    col = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_spacing (col, BORDER);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (col, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func (col, cell,
					     (GtkTreeCellDataFunc)
					     render_icon, NULL, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (col, cell,
					     (GtkTreeCellDataFunc)
					     render_text, tv, NULL);

    /* fill model */
    for (i = 0; i < ld->launcher->entries->len; ++i)
    {
        if (i == 7)
        {
            GtkRequisition req;

            gtk_widget_size_request (tv, &req);

            gtk_widget_set_size_request (tv, -1, req.height);

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_ALWAYS);
        }

	e = g_ptr_array_index (ld->launcher->entries, i);
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, e, -1);
    }

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed), ld);
}

/* treeview buttons */
static void
launcher_dialog_add_entry_after (LauncherDialog *ld, GtkTreeIter *prev_iter, 
                                 LauncherEntry *new_e)
{
    LauncherEntry *e = NULL;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;
    
    g_return_if_fail (new_e != NULL);

    if (ld->launcher->entries->len == 6)
    {
        GtkRequisition req;

        gtk_widget_size_request (ld->tree, &req);

        gtk_widget_set_size_request (ld->tree, -1, req.height);

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                        GTK_POLICY_NEVER, 
                                        GTK_POLICY_ALWAYS);
    }

    model = gtk_tree_view_get_model (GTK_TREE_VIEW (ld->tree));
    
    if (prev_iter)
        gtk_tree_model_get (model, prev_iter, 0, &e, -1);
    
    if (!e)
    {
        g_ptr_array_add (ld->launcher->entries, new_e);
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new_e, -1);
    }
    else
    {
        int i;

        g_ptr_array_add (ld->launcher->entries, NULL);
        
        for (i = ld->launcher->entries->len; i > 0; --i)
        {
            LauncherEntry *tmp = 
                g_ptr_array_index (ld->launcher->entries, i - 1);

            if (tmp == e)
            {
                ld->launcher->entries->pdata[i] = new_e;
                break;
            }
        
            ld->launcher->entries->pdata[i] = tmp;
        }

        gtk_list_store_insert_after (GTK_LIST_STORE (model), &iter, 
                                     prev_iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new_e, -1);
    }

    path = gtk_tree_model_get_path (model, &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, FALSE);
    gtk_tree_path_free (path);

    gtk_widget_show (ld->launcher->arrowbutton);
}

static void
tree_button_clicked (GtkWidget *b, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter, iter2;
    GtkTreePath *path;
    LauncherEntry *e;
    int i;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (b == ld->up)
    {
        if (!e)
            return;

        path = gtk_tree_model_get_path (model, &iter);

        if (gtk_tree_path_prev (path) &&
                gtk_tree_model_get_iter (model, &iter2, path))
        {
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, 
                                      FALSE);

            for (i = 1; i < ld->launcher->entries->len; ++i)
            {
                LauncherEntry *tmp = 
                    g_ptr_array_index (ld->launcher->entries, i);
                
                if (tmp == e)
                {
                    ld->launcher->entries->pdata[i] = 
                        ld->launcher->entries->pdata[i-1];
                    ld->launcher->entries->pdata[i-1] = tmp;

                    if (i == 1)
                        set_panel_icon (ld);

                    break;
                }
            }
        }
            
        gtk_tree_path_free (path);
    }
    else if (b == ld->down)
    {
        if (!e)
            return;

        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_path_next (path);
        
        if (gtk_tree_model_get_iter (model, &iter2, path))
        {
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, 
                                      FALSE);

            for (i = 0; i < ld->launcher->entries->len - 1; ++i)
            {
                LauncherEntry *tmp = 
                    g_ptr_array_index (ld->launcher->entries, i);
                
                if (tmp == e)
                {
                    ld->launcher->entries->pdata[i] = 
                        ld->launcher->entries->pdata[i+1];
                    ld->launcher->entries->pdata[i+1] = tmp;

                    if (i == 0)
                        set_panel_icon (ld);

                    break;
                }
            }
        }

        gtk_tree_path_free (path);
    }
    else if (b == ld->add)
    {
        LauncherEntry *e2;
        
        e2 = launcher_entry_new ();
        e2->name = g_strdup (_("New Item"));
        
        launcher_dialog_add_entry_after (ld, &iter, e2);
    }
    else if (b == ld->remove)
    {
        if (!e || e == g_ptr_array_index (ld->launcher->entries, 0))
            return;

        if (gtk_list_store_remove (GTK_LIST_STORE (model), &iter))
        {
            path = gtk_tree_model_get_path (model, &iter);
        }
        else
        {
            path = gtk_tree_path_new_from_string ("0");
        }
        
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, FALSE);
        
        gtk_tree_path_free (path);
            
        g_ptr_array_remove (ld->launcher->entries, e);
        launcher_entry_free (e);
        
        if (ld->launcher->entries->len == 1)
            gtk_widget_hide (ld->launcher->arrowbutton);
    }

    cursor_changed (GTK_TREE_VIEW (ld->tree), ld);
}

static void
launcher_dialog_add_buttons (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *hbox, *b, *align, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);
    
    ld->up = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);
    img = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    gtk_widget_set_sensitive (b, FALSE);

    ld->down = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);
    img = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_widget_set_size_request (align, 1, 1);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
    
    ld->add = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);
    img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    ld->remove = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);
    img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    gtk_widget_set_sensitive (b, FALSE);
}

/* explanation */
static void
launcher_dialog_add_explanation (GtkBox *box)
{
    GtkWidget *hbox, *img, *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER - 2);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);
    
    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_misc_set_alignment (GTK_MISC (img), 0, 0);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);
    
    label = 
        gtk_label_new (_("The first item in the list is shown on the panel. "
                         "Additional items will appear in a menu."));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
}

/* launcher plugin dialog */
static void
launcher_dialog_response (GtkWidget *dlg, int response, LauncherDialog *ld)
{
    gtk_widget_hide (dlg);
    launcher_update_panel_entry (ld->launcher);
    launcher_recreate_menu (ld->launcher);
    gtk_widget_destroy (dlg);

    xfce_panel_plugin_unblock_menu (ld->plugin);

    launcher_save (ld->plugin, ld->launcher);
    
    g_free (ld);
}

void
launcher_properties_dialog (XfcePanelPlugin *plugin, LauncherPlugin * launcher)
{
    LauncherDialog *ld;
    GtkWidget *header, *vbox, *hbox;

    ld = g_new0 (LauncherDialog, 1);
    
    ld->plugin = plugin;
    ld->launcher = launcher;
    ld->entry = g_ptr_array_index (launcher->entries, 0);
    
    xfce_panel_plugin_block_menu (ld->plugin);
    
    ld->dlg = gtk_dialog_new_with_buttons (_("Properties"), 
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT |
                GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);
    
    gtk_container_set_border_width (GTK_CONTAINER (ld->dlg), 2);
    
    header = xfce_create_header (NULL, _("Program Launcher"));
    gtk_widget_set_size_request (GTK_BIN (header)->child, -1, 32);
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ld->dlg)->vbox), header,
                        FALSE, TRUE, 0);
    
    launcher_dialog_add_explanation (GTK_BOX (GTK_DIALOG (ld->dlg)->vbox));
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER - 2);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ld->dlg)->vbox), hbox,
                        TRUE, TRUE, 0);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    
    launcher_dialog_add_item_tree (ld, GTK_BOX (vbox));

    launcher_dialog_add_buttons (ld, GTK_BOX (vbox));

    launcher_dialog_add_entry_properties (ld, GTK_BOX (hbox));
    
    cursor_changed (GTK_TREE_VIEW (ld->tree), ld);

    g_signal_connect (ld->dlg, "response", 
                      G_CALLBACK (launcher_dialog_response), ld);

    gtk_widget_show (ld->dlg);
}


/* DND *
 * --- */

static const char *dentry_keys [] = {
    "Name",
    "GenericName",
    "Comment",
    "Exec",
    "Icon",
    "Terminal",
    "StartupNotify",
    "OnlyShowIn"
};

static LauncherEntry *
update_entry_from_desktop_file (LauncherEntry *e, const char *path)
{
    XfceDesktopEntry *dentry;

    if ((dentry = xfce_desktop_entry_new (path, dentry_keys, 
                                          G_N_ELEMENTS (dentry_keys))))
    {
        char *value = NULL;

        g_free (e->name);
        g_free (e->comment);
        g_free (e->exec);
        if (e->icon.type == LAUNCHER_ICON_TYPE_NAME)
            g_free (e->icon.icon.name);

        e->name = e->comment = e->exec = NULL;
        e->terminal = e->startup = FALSE;
        e->icon.icon.name = NULL;
        e->icon.type = LAUNCHER_ICON_TYPE_NONE;        
        
        xfce_desktop_entry_get_string (dentry, "OnlyShowIn", FALSE, 
                                       &value);

        if (value && !strcmp ("XFCE", value))
        {
            xfce_desktop_entry_get_string (dentry, "GenericName", FALSE,
                                           &(e->name));            
        }

        g_free (value);

        if (!e->name)
        {
            xfce_desktop_entry_get_string (dentry, "Name", FALSE,
                                           &(e->name));            
        }
        
        xfce_desktop_entry_get_string (dentry, "Comment", FALSE,
                                       &(e->comment));
        
        value = NULL;
        xfce_desktop_entry_get_string (dentry, "Icon", FALSE,
                                       &value);

        if (value)
        {
            e->icon.type = LAUNCHER_ICON_TYPE_NAME;
            e->icon.icon.name = value;
        }

        xfce_desktop_entry_get_string (dentry, "Exec", FALSE,
                                       &(e->exec));

        value = NULL;
        xfce_desktop_entry_get_string (dentry, "Terminal", FALSE,
                                       &value);

        if (value && (!strcmp ("1", value) || !strcmp ("true", value)))
        {
            e->terminal = TRUE;

            g_free (value);
        }
        
        value = NULL;
        xfce_desktop_entry_get_string (dentry, "StartupNotify", FALSE,
                                       &value);

        if (value && (!strcmp ("1", value) || !strcmp ("true", value)))
        {
            e->startup = TRUE;

            g_free (value);
        }
        
        g_object_unref (dentry);

        return e;
    }

    return NULL;
}

static LauncherEntry *
create_entry_from_file (const char *path)
{
    LauncherEntry *e = launcher_entry_new ();
    
    if (g_str_has_suffix (path, ".desktop"))
    {
        update_entry_from_desktop_file (e, path);
    }
    else 
    {
        const char *start, *end;
        char *utf8;
        
        if (!g_utf8_validate (path, -1, NULL))
            utf8 = g_locale_to_utf8 (path, -1, NULL, NULL, NULL);
        else
            utf8 = g_strdup (path);
        
        e->exec = g_strdup (path);
        
        if (!(start = strrchr (utf8, G_DIR_SEPARATOR)))
            start = utf8;
        else
            start++;
        end = strrchr (start, '.');
        e->name = g_strndup (start, end ? end - start : strlen (start));
        e->icon.type = LAUNCHER_ICON_TYPE_NAME;
        e->icon.icon.name = g_strdup (e->name);

        g_free (utf8);
    }

    return e;
}

static void 
entry_dialog_data_received (GtkWidget *w, GdkDragContext *context, 
                            gint x, gint y, GtkSelectionData *data, 
                            guint info, guint time, LauncherDialog *ld)
{
    LauncherEntry *e = NULL;
    GPtrArray *files;
    int i;
    
    if (!data || data->length < 1)
        return;
    
    if (!(files = launcher_get_file_list_from_selection_data (data)))
        return;
    
    if (files->len > 0)
    {
        char *file = g_ptr_array_index (files, 0);
        
        if (g_str_has_suffix (file, ".desktop"))
        {
            e = update_entry_from_desktop_file (ld->entry, file);
        }
        else
        {
            /* do stuff based on the widget the file was dropped on */
        }

        if (e)
        {
            GdkPixbuf *pb;
            
            gtk_entry_set_text (GTK_ENTRY (ld->exec_name), e->name);
            gtk_entry_set_text (GTK_ENTRY (ld->exec_comment), e->comment);
            if (e->icon.type == LAUNCHER_ICON_TYPE_NAME)
                gtk_entry_set_text (GTK_ENTRY (ld->icon_file), 
                                    e->icon.icon.name);
            gtk_entry_set_text (GTK_ENTRY (ld->exec), e->exec);
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (ld->exec_terminal), e->terminal);
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (ld->exec_startup), e->startup);

            pb = launcher_icon_load_pixbuf (&e->icon, DLG_ICON_SIZE);
            gtk_image_set_from_pixbuf (GTK_IMAGE (ld->icon_img), pb);
            g_object_unref (pb);
        }
    }

    for (i = 0; i < files->len; ++i)
        g_free (g_ptr_array_index (files, i));

    g_ptr_array_free (files, TRUE);
}
        
static void 
launcher_dialog_data_received (GtkWidget *w, GdkDragContext *context, 
                               gint x, gint y, GtkSelectionData *data, 
                               guint info, guint time, LauncherDialog *ld)
{
    GPtrArray *files;
    int i;
    
    if (!data || data->length < 1)
        return;
    
    if (!(files = launcher_get_file_list_from_selection_data (data)))
        return;
    
    for (i = 0; i < files->len; ++i)
    {
        char *file = g_ptr_array_index (files, i);
        LauncherEntry *e = create_entry_from_file (file);

        if (e)
            launcher_dialog_add_entry_after (ld, NULL, e);

        g_free (file);
    }

    g_ptr_array_free (files, TRUE);
}
        
/*  File open dialog
 *  ----------------
 */
static void
update_preview_cb (GtkFileChooser *chooser, gpointer data)
{
    GtkImage *preview;
    char *filename;
    GdkPixbuf *pb = NULL;
    
    preview = GTK_IMAGE(data);
    filename = gtk_file_chooser_get_filename(chooser);

    if(filename && g_file_test(filename, G_FILE_TEST_EXISTS)
       && (pb = gdk_pixbuf_new_from_file (filename, NULL)))
    {
        int w, h;
        
        w = gdk_pixbuf_get_width (pb);
        h = gdk_pixbuf_get_height (pb);

        if (h > 120 || w > 120)
        {
            double wratio, hratio;
            GdkPixbuf *tmp;
            
            wratio = (double)120 / w;
            hratio = (double)120 / h;

            if (hratio < wratio)
            {
                w = rint (hratio * w);
                h = 120;
            }
            else
            {
                w = 120;
                h = rint (wratio * h);
            }

            tmp = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
            g_object_unref (pb);
            pb = tmp;
        }
    }
    
    g_free(filename);
    
    gtk_image_set_from_pixbuf(preview, pb);

    if (pb)
        g_object_unref(pb);
}

/* Any of the arguments may be NULL */
static char *
select_file_with_preview (const char *title, const char *path,
			  GtkWidget * parent, gboolean with_preview)
{
    const char *t;
    GtkWidget *fs, *frame, *preview;
    char *name = NULL;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    t = (title) ? title : _("Select file");
    
    fs = gtk_file_chooser_dialog_new (t, GTK_WINDOW(parent), 
                               GTK_FILE_CHOOSER_ACTION_OPEN, 
                               GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
                               GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, 
                               NULL);

    if (path && *path && g_file_test (path, G_FILE_TEST_EXISTS))
    {
        if (!g_path_is_absolute (path))
        {
            char *current, *full;

            current = g_get_current_dir ();
            full = g_build_filename (current, path, NULL);

            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs), full);

            g_free (current);
            g_free (full);
        }
        else
        {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs), path);
        }
    }

    if (with_preview)
    {
        frame = gtk_frame_new (NULL);
        gtk_widget_set_size_request (frame, 130, 130);
        gtk_widget_show (frame);
        
        preview = gtk_image_new();
        gtk_widget_show(preview);
        gtk_container_add (GTK_CONTAINER (frame), preview);
        
        gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(fs), frame);
        
        g_signal_connect (G_OBJECT(fs), "update-preview", G_CALLBACK (update_preview_cb), preview);
        gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER(fs), FALSE);

        if (path)
            update_preview_cb (GTK_FILE_CHOOSER(fs), preview);
    }
    
    if (gtk_dialog_run (GTK_DIALOG (fs)) == GTK_RESPONSE_ACCEPT)
    {
	name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));
    }

    gtk_widget_destroy (fs);

    return name;
}


