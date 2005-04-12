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
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/xfce_support.h>

#include "launcher.h"
#include "launcher-dialog.h"


typedef struct
{
    Launcher *launcher;
    
    GtkWidget *dlg;

    GtkWidget *tree;
    GtkWidget *scroll;

    GtkWidget *up;
    GtkWidget *down;
    GtkWidget *edit;
    GtkWidget *add;
    GtkWidget *remove;
}
LauncherDialog;

typedef struct
{
    Entry *entry;
    
    gboolean info_changed;
    gboolean icon_changed;
    gboolean exec_changed;

    GtkWidget *dlg;
    
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
EntryDialog;

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
    N_("Terminal"),
    N_("Development"),
    N_("Settings"),
    N_("System"),
    N_("Windows programs")
};


static void entry_dialog_data_received (GtkWidget *w, GList *data, 
                                        gpointer user_data);
        
static void launcher_dialog_data_received (GtkWidget *tv, GList *data, 
                                           gpointer user_data);


/* Entry dialog *
 * ------------ */

static void
update_entry_info (EntryDialog *ed)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ed->exec_name));

    if (!text || !strlen (text))
    {
        if (ed->entry->name)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->name);
            ed->entry->name = NULL;
        }
    }
    else if (!ed->entry->name || strcmp (text, ed->entry->name) != 0)
    {
        ed->info_changed = TRUE;

        g_free (ed->entry->name);
        ed->entry->name = g_strdup (text);
    }

    text = gtk_entry_get_text (GTK_ENTRY (ed->exec_comment));

    if (!text || !strlen (text))
    {
        if (ed->entry->comment)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->comment);
            ed->entry->comment = NULL;
        }
    }
    else if (!ed->entry->comment || strcmp (text, ed->entry->comment) != 0)
    {
        ed->info_changed = TRUE;

        g_free (ed->entry->comment);
        ed->entry->comment = g_strdup (text);
    }
}

static void
update_entry_icon (EntryDialog *ed)
{
    const char *text;
    GdkPixbuf *pb;

    text = gtk_entry_get_text (GTK_ENTRY (ed->icon_file));
        
    if (!text || !strlen (text))
    {
        ed->icon_changed = TRUE;

        if (ed->entry->icon.type == ICON_TYPE_NAME)
        {
            g_free (ed->entry->icon.icon.name);
            ed->entry->icon.icon.name = NULL;
        }

        ed->entry->icon.type = ICON_TYPE_NONE;

        pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }
    else if (ed->entry->icon.type != ICON_TYPE_NAME || 
             strcmp (text, ed->entry->icon.icon.name) != 0)
    {
        ed->icon_changed = TRUE;

        if (ed->entry->icon.type == ICON_TYPE_NAME)
            g_free (ed->entry->icon.icon.name);

        ed->entry->icon.type = ICON_TYPE_NAME;
        ed->entry->icon.icon.name = g_strdup (text);
        
        pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }
}

static void
update_entry_exec (EntryDialog *ed)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ed->exec));

    if (!text || !strlen (text))
    {
        if (ed->entry->exec)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->exec);
            ed->entry->exec = NULL;
        }
    }
    else if (!ed->entry->exec || strcmp (text, ed->entry->exec) != 0)
    {
        ed->exec_changed = TRUE;

        g_free (ed->entry->exec);
        ed->entry->exec = g_strdup (text);
    }
}

static gboolean
entry_lost_focus (GtkWidget *gentry, GdkEventFocus *ev, EntryDialog *ed)
{
    if (gentry == ed->exec_name || gentry == ed->exec_comment)
    {
        update_entry_info (ed);
    }
    else if (gentry == ed->icon_file)
    {
        update_entry_icon (ed);
    }
    else if (gentry == ed->exec)
    {
        update_entry_exec (ed);
    }

    return FALSE;
}

static void
check_button_toggled (GtkWidget *tb, EntryDialog *ed)
{
    if (tb == ed->exec_terminal)
        ed->entry->terminal = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
    else if (tb == ed->exec_startup)
        ed->entry->startup = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
}

static void
add_entry_info_options(EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
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

    ed->exec_name = gtk_entry_new ();
    gtk_widget_show (ed->exec_name);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_name, TRUE, TRUE, 0);
    if (ed->entry->name)
        gtk_entry_set_text (GTK_ENTRY (ed->exec_name), ed->entry->name);

    g_signal_connect (ed->exec_name, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Description"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ed->exec_comment = gtk_entry_new ();
    gtk_widget_show (ed->exec_comment);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_comment, TRUE, TRUE, 0);
    gtk_widget_set_size_request (ed->exec_comment, 300, -1);
    if (ed->entry->comment)
        gtk_entry_set_text (GTK_ENTRY (ed->exec_comment), ed->entry->comment);
    
    g_signal_connect (ed->exec_comment, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
}

static void
icon_menu_deactivated (GtkWidget *menu, EntryDialog *ed)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->icon_button), FALSE);
}

static void
position_icon_menu (GtkMenu * menu, int *x, int *y, gboolean * push_in, 
                    GtkWidget *b)
{
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    /* wtf is this anyway? */
    *push_in = TRUE;

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
popup_icon_menu (GtkWidget *tb, EntryDialog *ed)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
    {
        gtk_menu_popup (GTK_MENU (ed->icon_category), NULL, NULL, 
                        (GtkMenuPositionFunc) position_icon_menu, tb,
                        0, gtk_get_current_event_time ());
    }
}

static void
icon_menu_activated (GtkWidget *mi, EntryDialog *ed)
{
    int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), "category"));

    n = CLAMP (n, 0, NUM_CATEGORIES);

    if (ed->entry->icon.type != ICON_TYPE_CATEGORY 
        || ed->entry->icon.icon.category != n)
    {
        GdkPixbuf *pb;
        
        gtk_label_set_text (GTK_LABEL (ed->icon_label), _(category_icons[n]));
        gtk_widget_hide (ed->icon_file_align);
        gtk_widget_show (ed->icon_label);
        
        ed->icon_changed = TRUE;

        if (ed->entry->icon.type == ICON_TYPE_NAME)
            g_free (ed->entry->icon.icon.name);
        ed->entry->icon.icon.name = NULL;

        ed->entry->icon.type = ICON_TYPE_CATEGORY;
        ed->entry->icon.icon.category = (XfceIconThemeCategory) n;

        pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }
}

static void
icon_browse (GtkWidget *b, EntryDialog *ed)
{
    char *file, *path;
    
    path = 
        (ed->entry->icon.type == ICON_TYPE_NAME 
         && g_path_is_absolute (ed->entry->icon.icon.name)) ?
                ed->entry->icon.icon.name : NULL;
    
    file = select_file_with_preview (_("Select image file"), path, ed->dlg);

    if (file && g_file_test (file, G_FILE_TEST_IS_REGULAR))
    {
        GdkPixbuf *pb;
        
        gtk_entry_set_text (GTK_ENTRY (ed->icon_file), file);
        gtk_editable_set_position (GTK_EDITABLE (ed->icon_file), -1);
        update_entry_icon (ed);

        pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);

        ed->icon_changed = TRUE;
    }

    g_free (file);
}

static void
icon_menu_browse (GtkWidget *mi, EntryDialog *ed)
{
    GdkPixbuf *pb;
    
    ed->icon_changed = TRUE;

    gtk_widget_hide (ed->icon_label);
    gtk_widget_show (ed->icon_file_align);

    update_entry_icon (ed);

    pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
    gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
    g_object_unref (pb);
}

static GtkWidget *
create_icon_category_menu (EntryDialog *ed)
{
    GtkWidget *menu, *mi, *img;
    GdkPixbuf *pb;
    int i;

    menu = gtk_menu_new ();
    
    g_signal_connect (menu, "deactivate", 
                      G_CALLBACK (icon_menu_deactivated), ed);
    
    for (i = 1; i < NUM_CATEGORIES; ++i)
    {
        mi = gtk_image_menu_item_new_with_label (_(category_icons[i]));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        g_object_set_data (G_OBJECT (mi), "category", GINT_TO_POINTER (i));

        g_signal_connect (mi, "activate", 
                          G_CALLBACK (icon_menu_activated), ed);

        pb = xfce_icon_theme_load_category (launcher_theme, i, MENU_ICON_SIZE);
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
                      G_CALLBACK (icon_menu_browse), ed);
    
    return menu;
}

static void
add_entry_icon_options (EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *hbox2, *arrow, *align, *img;
    GdkPixbuf *pb;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    ed->icon_button = gtk_toggle_button_new ();
    gtk_widget_show (ed->icon_button);
    gtk_box_pack_start (GTK_BOX (hbox), ed->icon_button, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, ed->icon_button);

    ed->icon_category = create_icon_category_menu (ed);

    g_signal_connect (ed->icon_button, "toggled", 
                      G_CALLBACK (popup_icon_menu), ed);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (ed->icon_button), hbox2);

    ed->icon_img = gtk_image_new ();
    gtk_widget_show (ed->icon_img);
    gtk_box_pack_start (GTK_BOX (hbox2), ed->icon_img, TRUE, TRUE, 0);

    pb = launcher_load_pixbuf (&ed->entry->icon, DLG_ICON_SIZE);
        
    gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
    g_object_unref (pb);

    arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
    gtk_widget_show (arrow);
    gtk_box_pack_start (GTK_BOX (hbox2), arrow, TRUE, TRUE, 0);
    
    align = ed->icon_file_align = gtk_alignment_new (0, 0.5, 1, 0);
    if (ed->entry->icon.type != ICON_TYPE_CATEGORY)
        gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (align), hbox2);

    ed->icon_file = gtk_entry_new ();
    gtk_widget_show (ed->icon_file);
    gtk_box_pack_start (GTK_BOX (hbox2), ed->icon_file, TRUE, TRUE, 0);
    if (ed->entry->icon.type == ICON_TYPE_NAME && ed->entry->icon.icon.name)
        gtk_entry_set_text (GTK_ENTRY (ed->icon_file), 
                            ed->entry->icon.icon.name);
    
    g_signal_connect (ed->icon_file, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
    
    ed->icon_browse = gtk_button_new ();
    gtk_widget_show (ed->icon_browse);
    gtk_box_pack_start (GTK_BOX (hbox2), ed->icon_browse, FALSE, FALSE, 0);

    g_signal_connect (ed->icon_browse, "clicked", G_CALLBACK (icon_browse), 
                      ed);

    img = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (ed->icon_browse), img);
    
    ed->icon_label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (ed->icon_label), 0, 0.5);
    if (ed->entry->icon.type == ICON_TYPE_CATEGORY)
    {
        gtk_label_set_text (GTK_LABEL (ed->icon_label), 
                            _(category_icons [ed->entry->icon.icon.category]));
        gtk_widget_show (ed->icon_label);
    }

    gtk_box_pack_start (GTK_BOX (hbox), ed->icon_label, TRUE, TRUE, 0);
}

static void
add_entry_exec_options (EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
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

    ed->exec = gtk_entry_new ();
    gtk_widget_show (ed->exec);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec, TRUE, TRUE, 0);
    if (ed->entry->exec)
        gtk_entry_set_text (GTK_ENTRY (ed->exec), ed->entry->exec);
    
    g_signal_connect (ed->exec, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);

    button = gtk_button_new ();
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (button), img);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ed->exec_terminal = 
        gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
    gtk_widget_show (ed->exec_terminal);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_terminal, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->exec_terminal), 
                                  ed->entry->terminal);

    g_signal_connect (ed->exec_terminal, "toggled",
                      G_CALLBACK (check_button_toggled), ed);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ed->exec_startup = 
        gtk_check_button_new_with_mnemonic (_("Use _startup notification"));
    gtk_widget_show (ed->exec_startup);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_startup, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->exec_startup), 
                                  ed->entry->startup);

    g_signal_connect (ed->exec_startup, "toggled",
                      G_CALLBACK (check_button_toggled), ed);
}

static gboolean
entry_properties_dialog (Entry *entry, GtkWindow *parent)
{
    EntryDialog *ed;
    GtkWidget *header, *vbox, *align;
    GtkSizeGroup *sg;
    gboolean changed = FALSE;

    ed = g_new0 (EntryDialog, 1);
    
    ed->entry = entry;
    
    ed->dlg = gtk_dialog_new_with_buttons (_("Edit launcher"), parent,
                                           GTK_DIALOG_NO_SEPARATOR,
                                           GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                           NULL);
    
    header = xfce_create_header (NULL, _("Edit launcher"));
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ed->dlg)->vbox), header,
                        FALSE, TRUE, 0);
    gtk_widget_set_size_request (header, -1, DLG_ICON_SIZE);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ed->dlg)->vbox), vbox,
                        FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    add_entry_info_options (ed, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_icon_options (ed, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_exec_options (ed, GTK_BOX (vbox), sg);
    
    g_object_unref (sg);
    
    dnd_set_drag_dest (ed->dlg);
    dnd_set_callback (ed->dlg, DROP_CALLBACK (entry_dialog_data_received), 
                      ed);
    
    gtk_dialog_run (GTK_DIALOG (ed->dlg));

    gtk_widget_hide (ed->dlg);
   
    update_entry_info (ed);
   
    if (ed->entry->icon.type == ICON_TYPE_NAME)
        update_entry_icon (ed);
    
    update_entry_exec (ed);
    
    changed = ed->info_changed || ed->icon_changed || ed->exec_changed;
    
    gtk_widget_destroy (ed->dlg);
    
    g_free (ed);

    return changed;
}


/* Launcher dialog *
 * --------------- */

static void
set_panel_icon (LauncherDialog *ld)
{
    GdkPixbuf *pb;

    pb = launcher_load_pixbuf (&ld->launcher->entry->icon, PANEL_ICON_SIZE);
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (ld->launcher->iconbutton),
                                pb);
    g_object_unref (pb);
}

static void
launcher_dialog_destroyed (LauncherDialog *ld)
{
    if (ld->launcher->iconbutton)
    {
        launcher_recreate_menu (ld->launcher);
        launcher_update_panel_entry (ld->launcher);
    }

    g_free (ld);
}

static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

static void
launcher_dialog_add_entry_after (LauncherDialog *ld, GtkTreeIter *prev_iter, 
                                 Entry *new_e)
{
    Entry *e = NULL;
    GList *l;
    int n;
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreePath *path;
    
    g_return_if_fail (new_e != NULL);

    if (g_list_length (ld->launcher->items) == 6)
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
        ld->launcher->items = g_list_append (ld->launcher->items, new_e);
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new_e, -1);
    }
    else
    {
        if (e == ld->launcher->entry)
        {
            ld->launcher->items = g_list_prepend (ld->launcher->items, new_e);
        }
        else
        {
            for (n = 1, l = ld->launcher->items; l != NULL; l = l->next, ++n)
            {
                if (e == (Entry *)l->data)
                {
                    ld->launcher->items = 
                        g_list_insert (ld->launcher->items, new_e, n);

                    break;
                }
            }
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

static gboolean
treeview_dblclick (GtkWidget * tv, GdkEventButton * evt, LauncherDialog *ld)
{
    if (evt->type == GDK_2BUTTON_PRESS)
    {
        GtkTreeSelection *sel;
        GtkTreeModel *model;
        GtkTreeIter iter;
        Entry *e;

        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
        gtk_tree_selection_get_selected (sel, &model, &iter);

        gtk_tree_model_get (model, &iter, 0, &e, -1);

        if (!e)
            return FALSE;
        
        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        if (entry_properties_dialog (e, GTK_WINDOW (ld->dlg)) && 
            e == ld->launcher->entry)
        {
            set_panel_icon (ld);
        }
        
        return TRUE;
    }

    return FALSE;
}

static void
cursor_changed (GtkTreeView * tv, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Entry *e;

    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (e)
    {
        GList *l;
        
	gtk_widget_set_sensitive (ld->up, TRUE);
	gtk_widget_set_sensitive (ld->down, TRUE);
	gtk_widget_set_sensitive (ld->edit, TRUE);
        gtk_widget_set_sensitive (ld->remove, TRUE);

        if (e == ld->launcher->entry)
        {
            gtk_widget_set_sensitive (ld->remove, FALSE);
            gtk_widget_set_sensitive (ld->up, FALSE);

            if (!ld->launcher->items)
                gtk_widget_set_sensitive (ld->down, FALSE);
        }
        else
        {
            for (l = ld->launcher->items; l != NULL; l = l->next)
            {
                if (e == (Entry *)l->data)
                {
                    if (!l->next)
                        gtk_widget_set_sensitive (ld->down, FALSE);

                    break;
                }
            }
        }
    }
    else
    {
	gtk_widget_set_sensitive (ld->up, FALSE);
	gtk_widget_set_sensitive (ld->down, FALSE);
	gtk_widget_set_sensitive (ld->edit, FALSE);
	gtk_widget_set_sensitive (ld->remove, FALSE);
    }
}

static void
render_icon (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
    Entry *entry;
    GdkPixbuf *pb;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
    {
        pb = launcher_load_pixbuf (&entry->icon, DLG_ICON_SIZE);
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
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
    {
        char *text;

        if (entry->comment)
        {
            text = g_strdup_printf ("<b>%s</b>\n%s", entry->name, 
                                    entry->comment);
        }
        else
        {
            text = g_strdup_printf ("<b>%s</b>", entry->name);
        }
        
        g_object_set (cell, "markup", text, NULL);
        g_free (text);
    }
    else
    {
        g_object_set (cell, "markup", "", NULL);
    }
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
    Entry *e;
    GList *l;
    int n;

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
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (ld->scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

    g_object_unref (G_OBJECT (store));

    dnd_set_drag_dest (tv);
    dnd_set_callback (tv, DROP_CALLBACK (launcher_dialog_data_received), ld);
    
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
    e = ld->launcher->entry;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, e, -1);
    
    for (l = ld->launcher->items, n = 0; l != NULL; l = l->next, n++)
    {
        if (n == 7)
        {
            GtkRequisition req;

            gtk_widget_size_request (tv, &req);

            gtk_widget_set_size_request (tv, -1, req.height);

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_ALWAYS);
        }

	e = l->data;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, e, -1);
    }

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed),
		      ld);

    g_signal_connect (tv, "button-press-event",
		      G_CALLBACK (treeview_dblclick), ld);
    
    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

static void
tree_button_clicked (GtkWidget *b, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter, iter2;
    GtkTreePath *path;
    Entry *e;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (b == ld->up)
    {
        if (!e)
            return;

        path = gtk_tree_model_get_path (model, &iter);

        if (gtk_tree_path_prev (path))
        {
            gtk_tree_model_get_iter (model, &iter2, path);

            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, 
                                      FALSE);

            if (!gtk_tree_path_prev (path))
            {
                Entry *tmp = ld->launcher->entry;
                
                ld->launcher->items = g_list_remove (ld->launcher->items, e);

                ld->launcher->entry = e;

                ld->launcher->items = g_list_prepend (ld->launcher->items, 
                                                      tmp);

                set_panel_icon (ld);
            }
            else
            {
                GList *l;

                for (l = ld->launcher->items; l != NULL; l = l->next)
                {
                    if ((Entry *)l->data == e)
                    {
                        GList *l2 = l->prev;                    
                        
                        l->prev = l2->prev;
                        l2->prev = l;
                        l2->next = l->next;
                        l->next = l2;
                        
                        if (l2 == ld->launcher->items)
                            ld->launcher->items = l;
                        
                        break;
                    }
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

            if (e == ld->launcher->entry)
            {
                Entry *tmp;

                tmp = ld->launcher->items->data;

                ld->launcher->items = g_list_remove (ld->launcher->items, tmp);

                ld->launcher->items = g_list_prepend (ld->launcher->items, 
                                                      ld->launcher->entry);
                
                ld->launcher->entry = tmp;

                set_panel_icon (ld);
            }
            else
            {
                GList *l;

                for (l = ld->launcher->items; l != NULL; l = l->next)
                {
                    if ((Entry *)l->data == e)
                    {
                        GList *l2 = l->next;                    
                        
                        l2->prev = l->prev;
                        l->prev = l2;
                        l->next = l2->next;
                        l2->next = l;

                        if (l == ld->launcher->items)
                            ld->launcher->items = l2;
                        
                        break;
                    }
                }
            }
        }

        gtk_tree_path_free (path);
    }
    else if (b == ld->edit)
    {
        if (!e)
            return;
        
        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        if (entry_properties_dialog (e, GTK_WINDOW (ld->dlg)) && 
            e == ld->launcher->entry)
        {
            set_panel_icon (ld);
        }
    }
    else if (b == ld->add)
    {
        Entry *e2;
        
        e2 = g_new0 (Entry, 1);
        e2->name = g_strdup (_("New item"));
        e2->comment = g_strdup (_("This item has not yet been configured"));
        
        launcher_dialog_add_entry_after (ld, &iter, e2);

        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        entry_properties_dialog (e2, GTK_WINDOW (ld->dlg));
    }
    else if (b == ld->remove)
    {
        if (!e || e == ld->launcher->entry)
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
            
        ld->launcher->items = g_list_remove (ld->launcher->items, e);

        entry_free (e);

        if (g_list_length (ld->launcher->items) == 7)
        {
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_NEVER);

            gtk_widget_set_size_request (ld->tree, -1, -1);
        }

        if (!ld->launcher->items)
            gtk_widget_hide (ld->launcher->arrowbutton);
    }

    cursor_changed (GTK_TREE_VIEW (ld->tree), ld);
}

static void
launcher_dialog_add_buttons (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *hbox, *b, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    ld->up = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    ld->down = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    ld->edit = b = gtk_button_new_with_mnemonic (_("_Edit"));
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, TRUE, TRUE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    ld->add = b = gtk_button_new_from_stock (GTK_STOCK_ADD);
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, TRUE, TRUE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    ld->remove = b = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, TRUE, TRUE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    gtk_widget_set_sensitive (b, FALSE);
}

static void
launcher_dialog_add_explanation (GtkBox *box)
{
    GtkWidget *hbox, *img, *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);
    
    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);
    
    label = 
        gtk_label_new (_("The first item in the list is shown on the panel. "
                         "Additional items will appear in a menu."));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
}

void
launcher_properties_dialog (Launcher * launcher, GtkContainer * container,
                            GtkWidget * close)
{
    LauncherDialog *ld;
    GtkWidget *vbox;

    ld = g_new0 (LauncherDialog, 1);
    
    ld->launcher = launcher;
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    g_signal_connect_swapped (vbox, "destroy", 
                              G_CALLBACK (launcher_dialog_destroyed), ld);
    
    launcher_dialog_add_buttons (ld, GTK_BOX (vbox));
 
    launcher_dialog_add_item_tree (ld, GTK_BOX (vbox));

    launcher_dialog_add_explanation (GTK_BOX (vbox));
}

/* dnd */
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

static Entry *
create_entry_from_desktop_file (const char *path)
{
    XfceDesktopEntry *dentry;
    Entry *e = g_new0 (Entry, 1);

    if ((dentry = xfce_desktop_entry_new (path, dentry_keys, 
                                          G_N_ELEMENTS (dentry_keys))))
    {
        char *value;

        xfce_desktop_entry_get_string (dentry, "OnlyShowIn", FALSE, 
                                       &value);

        if (value && !strcmp ("XFCE", value))
        {
            g_free (value);
            xfce_desktop_entry_get_string (dentry, "GenericName", FALSE,
                                           &(e->name));            
        }

        if (!e->name)
        {
            xfce_desktop_entry_get_string (dentry, "Name", FALSE,
                                           &(e->name));            
        }
        
        xfce_desktop_entry_get_string (dentry, "Comment", FALSE,
                                       &(e->comment));
        
        xfce_desktop_entry_get_string (dentry, "Icon", FALSE,
                                       &value);

        if (value)
        {
            e->icon.type = ICON_TYPE_NAME;
            e->icon.icon.name = value;
        }

        xfce_desktop_entry_get_string (dentry, "Exec", FALSE,
                                       &(e->exec));

        xfce_desktop_entry_get_string (dentry, "Terminal", FALSE,
                                       &value);

        if (value && 
            (!strcmp ("1", value) || !strcmp ("true", value)))
        {
            e->terminal = TRUE;
        }
        
        g_free (value);

        xfce_desktop_entry_get_string (dentry, "StartupNotify", FALSE,
                                       &value);

        if (value && 
            (!strcmp ("1", value) || !strcmp ("true", value)))
        {
            e->startup = TRUE;
        }
        
        g_free (value);

        g_object_unref (dentry);
    }
    else
    {
        g_free (e);
        e = NULL;
    }

    return e;
}

static Entry *
create_entry_from_file (const char *path)
{
    Entry *e = NULL;
    
    if (g_str_has_suffix (path, ".desktop"))
    {
        e = create_entry_from_desktop_file (path);
    }
    else 
    {
        const char *start, *end;
        char *utf8;
        
        
        if (!g_utf8_validate (path, -1, NULL))
            utf8 = g_locale_to_utf8 (path, -1, NULL, NULL, NULL);
        else
            utf8 = g_strdup (path);
    
        
        e = g_new0 (Entry, 1);
        
        e->exec = g_strdup (path);
        
        if (!(start = strrchr (utf8, G_DIR_SEPARATOR)))
            start = utf8;
        else
            start++;
        end = strrchr (start, '.');
        e->name = g_strndup (start, end ? end - start : strlen (start));
        e->icon.type = ICON_TYPE_NAME;
        e->icon.icon.name = g_strup (e->name);

        g_free (utf8);
    }

    return e;
}

static void 
entry_dialog_data_received (GtkWidget *widget, GList *data, gpointer user_data)
{
    EntryDialog *ed = user_data;
    Entry *e = NULL;

    if (data && data->data)
    {
        if (g_str_has_suffix ((char *)data->data, ".desktop"))
        {
            e = create_entry_from_desktop_file ((char *)data->data);
        }
        else
        {
            /* do stuff based on the widget the file was dropped on */
        }

        if (e)
        {
            GdkPixbuf *pb;
            
            gtk_entry_set_text (GTK_ENTRY (ed->exec_name), e->name);
            gtk_entry_set_text (GTK_ENTRY (ed->exec_comment), e->comment);
            if (e->icon.type == ICON_TYPE_NAME)
                gtk_entry_set_text (GTK_ENTRY (ed->icon_file), 
                                    e->icon.icon.name);
            gtk_entry_set_text (GTK_ENTRY (ed->exec), e->exec);
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (ed->exec_terminal), e->terminal);
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (ed->exec_startup), e->startup);

            pb = launcher_load_pixbuf (&e->icon, DLG_ICON_SIZE);
            gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
            g_object_unref (pb);
        }
    }
}
        
static void 
launcher_dialog_data_received (GtkWidget *tv, GList *data, gpointer user_data)
{
    LauncherDialog *ld = user_data;
    GList *l;

    for (l = data; l != NULL; l = l->next)
    {
        char *path = file_uri_to_local ((char *)l->data);
        Entry *e = create_entry_from_file (path);

        g_free (path);
        
        if (e)
            launcher_dialog_add_entry_after (ld, NULL, e);
    }
}
        
