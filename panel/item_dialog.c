/*  item_dialog.c
 *
 *  Copyright (C) Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#include "global.h"
#include "item_dialog.h"
#include "xfce_support.h"
#include "controls.h"
#include "item.h"
#include "side.h"
#include "popup.h"
#include "icons.h"

/*  item_dialog.c
 *  -------------
 *  There are two types of configuration dialogs: for panel items and for 
 *  menu items.
 *
 *  1) Dialog for changing items on the panel. This is now defined in 
 *  controls_dialog.c. Only icon items use code from this file to 
 *  present their options. This code is partially shared with menu item
 *  dialogs.
 *  
 *  2) Dialogs for changing or adding menu items.
 *  Basically the same as the notebook page for icon panel items. Adds
 *  options for caption and position in menu.
*/

static void menu_item_apply_options(void);
static void panel_item_apply_options(void);

enum
{ RESPONSE_REVERT, RESPONSE_DONE, RESPONSE_REMOVE };

/* the item is a menu item or a panel control */
MenuItem *mitem = NULL;
int num_items = 0;
/*PanelControl *pcontrol = NULL;*/
PanelItem *pitem;

/* important widgets */
static GtkWidget *command_entry;
static GtkWidget *command_browse_button;
static GtkWidget *term_checkbutton;
static GtkWidget *icon_id_menu;
static GtkWidget *icon_entry;
static GtkWidget *icon_browse_button;
static GtkWidget *tip_entry;
static GtkWidget *preview_image;

/* for menu items */
GtkWidget *caption_entry;
GtkWidget *pos_spin;

/* controls on the parent dialog */
GtkWidget *revert_button;
GtkWidget *done_button;

static int id_callback;

/* usefull for (instant) apply */
int icon_id;
static char *icon_path = NULL;
int pos;

/*  Save options for revert 
 *  -----------------------
*/
struct ItemBackup
{
    char *command;
    gboolean in_terminal;

    char *caption;
    char *tooltip;

    int icon_id;
    char *icon_path;

    int pos;
};

struct ItemBackup backup;

void init_backup(void)
{
    backup.command = NULL;
    backup.in_terminal = FALSE;

    backup.caption = NULL;
    backup.tooltip = NULL;

    backup.icon_id = 0;
    backup.icon_path = NULL;

    backup.pos = 0;
}

void clear_backup(void)
{
    g_free(backup.command);

    g_free(backup.caption);
    g_free(backup.tooltip);

    g_free(backup.icon_path);

    /* not really backup, but ... */
    g_free(icon_path);
    icon_path = NULL;

    init_backup();
}

/*  useful in callbacks to make revert button sensitive
*/
void make_sensitive(GtkWidget * widget)
{
    gtk_widget_set_sensitive(widget, TRUE);
}

/*  entry callback
*/
gboolean entry_lost_focus(GtkEntry * entry, GdkEventFocus * event,
                          gpointer data)
{
    if(mitem)
        menu_item_apply_options();
    else
        panel_item_apply_options();

    /* needed to prevent GTK+ crash :( */
    return FALSE;
}

/*  Changing the icon
 *  -----------------
 *  An icon is changed by changing the id or the path in case of an
 *  external icon.
 *  There are several mechamisms by which an external icon can be changed:
 *  - dragging an icon to the preview area
 *  - writing the path in the text entry
 *  - using the file dialog
*/
static void change_icon(int id, const char *path)
{
    GdkPixbuf *pb = NULL;

    if(id == EXTERN_ICON && path)
        pb = gdk_pixbuf_new_from_file(path, NULL);
    else
        pb = get_pixbuf_by_id(id);

    if(!pb || !GDK_IS_PIXBUF(pb))
    {
        g_printerr("xfce4: %s (line %d): couldn't create pixbuf\n",
                   __FILE__, __LINE__);
        return;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(preview_image), pb);
    g_object_unref(pb);

    icon_id = id;

    if(id == EXTERN_ICON || id == UNKNOWN_ICON)
    {
        if(path)
        {
            if(!icon_path || !strequal(path, icon_path))
            {
                g_free(icon_path);
                icon_path = g_strdup(path);
            }

            gtk_entry_set_text(GTK_ENTRY(icon_entry), path);
        }

        gtk_widget_set_sensitive(icon_entry, TRUE);
        gtk_widget_set_sensitive(icon_browse_button, TRUE);
    }
    else
    {
        gtk_entry_set_text(GTK_ENTRY(icon_entry), "");

        gtk_widget_set_sensitive(icon_entry, FALSE);
        gtk_widget_set_sensitive(icon_browse_button, FALSE);
    }

    g_signal_handler_block(icon_id_menu, id_callback);
    gtk_option_menu_set_history(GTK_OPTION_MENU(icon_id_menu),
                                (id == EXTERN_ICON) ? 0 : id);
    g_signal_handler_unblock(icon_id_menu, id_callback);

    if(mitem)
        menu_item_apply_options();
    else
        panel_item_apply_options();

    make_sensitive(revert_button);
}

static void icon_id_changed(void)
{
    int new_id = gtk_option_menu_get_history(GTK_OPTION_MENU(icon_id_menu));

    if(new_id == 0)
    {
        change_icon(EXTERN_ICON, icon_path);
    }
    else
    {
        change_icon(new_id, NULL);
    }
}

static GtkWidget *create_icon_option_menu(void)
{
    GtkWidget *om;
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *mi;
    int i;

    mi = gtk_menu_item_new_with_label(_("External Icon"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    for(i = 1; i < NUM_ICONS; i++)
    {
        mi = gtk_menu_item_new_with_label(icon_names[i]);
        gtk_widget_show(mi);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }

    om = gtk_option_menu_new();
    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    id_callback =
        g_signal_connect_swapped(om, "changed", G_CALLBACK(icon_id_changed),
                                 NULL);

    return om;
}

static void icon_browse_cb(GtkWidget * b, GtkEntry * entry)
{
    char *file =
        select_file_name(_("Select icon"), gtk_entry_get_text(entry), NULL);

    if(file)
    {
        change_icon(EXTERN_ICON, file);
        g_free(file);
    }
}

gboolean icon_entry_lost_focus(GtkEntry * entry, GdkEventFocus * event,
                               gpointer data)
{
    const char *temp = gtk_entry_get_text(entry);

    if(temp)
        change_icon(EXTERN_ICON, temp);

    /* we must return FALSE or gtk will crash :-( */
    return FALSE;
}

static GtkWidget *create_icon_option(GtkSizeGroup * sg)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    /* option menu */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Icon:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    icon_id_menu = create_icon_option_menu();
    gtk_widget_show(icon_id_menu);
    gtk_box_pack_start(GTK_BOX(hbox), icon_id_menu, TRUE, TRUE, 0);

    /* icon entry */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    icon_entry = gtk_entry_new();
    gtk_widget_show(icon_entry);
    gtk_box_pack_start(GTK_BOX(hbox), icon_entry, TRUE, TRUE, 0);

    g_signal_connect(icon_entry, "focus-out-event",
                     G_CALLBACK(icon_entry_lost_focus), NULL);


    icon_browse_button = gtk_button_new_with_label(" ... ");
    gtk_widget_show(icon_browse_button);
    gtk_box_pack_start(GTK_BOX(hbox), icon_browse_button, FALSE, FALSE, 0);

    g_signal_connect(icon_browse_button, "clicked", G_CALLBACK(icon_browse_cb),
                     icon_entry);

    return vbox;
}

/*  Change the command 
 *  ------------------
*/
static void command_browse_cb(GtkWidget * b, GtkEntry * entry)
{
    char *file =
        select_file_name(_("Select command"), gtk_entry_get_text(entry), NULL);

    if(file)
    {
        gtk_entry_set_text(entry, file);
        g_free(file);
    }
}

static GtkWidget *create_command_option(GtkSizeGroup * sg)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    /* entry */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Command:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    command_entry = gtk_entry_new();
    gtk_widget_show(command_entry);
    gtk_box_pack_start(GTK_BOX(hbox), command_entry, TRUE, TRUE, 0);

    command_browse_button = gtk_button_new_with_label("...");
    gtk_widget_show(command_browse_button);
    gtk_box_pack_start(GTK_BOX(hbox), command_browse_button, FALSE, FALSE, 0);

    g_signal_connect(command_browse_button, "clicked",
                     G_CALLBACK(command_browse_cb), command_entry);

    /* terminal */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new("");
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    term_checkbutton =
        gtk_check_button_new_with_mnemonic(_("Run in _terminal"));
    gtk_widget_show(term_checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), term_checkbutton, FALSE, FALSE, 0);

    return vbox;
}

/*  Change caption
*/
static GtkWidget *create_caption_option(GtkSizeGroup * sg)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Caption:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    caption_entry = gtk_entry_new();
    gtk_widget_show(caption_entry);
    gtk_box_pack_start(GTK_BOX(hbox), caption_entry, TRUE, TRUE, 0);

    /* activate revert button when changing the label */
    g_signal_connect_swapped(caption_entry, "insert-at-cursor",
                             G_CALLBACK(make_sensitive), revert_button);
    g_signal_connect_swapped(caption_entry, "delete-from-cursor",
                             G_CALLBACK(make_sensitive), revert_button);

    /* only set label on focus out */
    g_signal_connect(caption_entry, "focus-out-event",
                     G_CALLBACK(entry_lost_focus), NULL);

    return vbox;
}

/*  Change tooltip
*/
static GtkWidget *create_tooltip_option(GtkSizeGroup * sg)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Tooltip:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    tip_entry = gtk_entry_new();
    gtk_widget_show(tip_entry);
    gtk_box_pack_start(GTK_BOX(hbox), tip_entry, TRUE, TRUE, 0);

    return vbox;
}

/* Change menu item position
 * -------------------------
*/
static void pos_changed(GtkSpinButton * spin, gpointer data)
{
    int n = gtk_spin_button_get_value_as_int(spin);

    mitem->pos = n - 1;

    menu_item_apply_options();
    gtk_widget_set_sensitive(revert_button, TRUE);
}

static GtkWidget *create_position_option(GtkSizeGroup * sg)
{
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Position:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg, label);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    pos_spin = gtk_spin_button_new_with_range(1, num_items, 1);
    gtk_widget_show(pos_spin);
    gtk_box_pack_start(GTK_BOX(hbox), pos_spin, FALSE, FALSE, 0);

    g_signal_connect(pos_spin, "value-changed", G_CALLBACK(pos_changed), NULL);

    return vbox;
}

/*  The main options box
 *  --------------------
*/
static GtkWidget *create_item_options_box(void)
{
    GtkWidget *vbox;
    GtkWidget *box;
    GtkWidget *label;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show(vbox);

    /* command */
    box = create_command_option(sg);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);

    /* icon */
    box = create_icon_option(sg);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);

    /* caption (menu item) */
    if(mitem)
    {
        box = create_caption_option(sg);
        gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);
    }

    /* tooltip */
    box = create_tooltip_option(sg);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);

    /* position (menu item) */
    if(mitem && num_items > 1)
    {
        box = create_position_option(sg);
        gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, TRUE, 0);
    }

    return vbox;
}

/*  Icon preview area
 *  -----------------
*/
static void
icon_drop_cb(GtkWidget * widget, GdkDragContext * context,
             gint x, gint y, GtkSelectionData * data,
             guint info, guint time, gpointer user_data)
{
    GList *fnames;
    guint count;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        char *icon;

        icon = (char *)fnames->data;

        change_icon(EXTERN_ICON, icon);
    }

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

static GtkWidget *create_icon_preview_frame()
{
    GtkWidget *frame;
    GtkWidget *eventbox;

    frame = gtk_frame_new(_("Icon Preview"));
    gtk_widget_show(frame);

    eventbox = gtk_event_box_new();
    add_tooltip(eventbox, _("Drag file onto this frame to change the icon"));
    gtk_widget_show(eventbox);
    gtk_container_add(GTK_CONTAINER(frame), eventbox);

    preview_image = gtk_image_new();
    gtk_widget_show(preview_image);
    gtk_container_add(GTK_CONTAINER(eventbox), preview_image);

    /* signals */
    dnd_set_drag_dest(eventbox);

    g_signal_connect(eventbox, "drag_data_received", G_CALLBACK(icon_drop_cb),
                     NULL);

    return frame;
}

/*  Panel item options box
 *  ----------------------
*/
void panel_item_revert_options(PanelControl * pc)
{
    g_free(pitem->command);
    pitem->command = g_strdup(backup.command);

    pitem->in_terminal = backup.in_terminal;

    g_free(pitem->tooltip);
    pitem->tooltip = g_strdup(backup.tooltip);

    pitem->icon_id = backup.icon_id;

    g_free(pitem->icon_path);
    pitem->icon_path = g_strdup(backup.icon_path);

    panel_item_apply_config(pitem);

    /* revert the widgets */
    if(pitem->command)
        gtk_entry_set_text(GTK_ENTRY(command_entry), pitem->command);
    else
        gtk_entry_set_text(GTK_ENTRY(command_entry), "");

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 pitem->in_terminal);

    change_icon(pitem->icon_id, pitem->icon_path);

    if(pitem->tooltip)
        gtk_entry_set_text(GTK_ENTRY(tip_entry), pitem->tooltip);
    else
        gtk_entry_set_text(GTK_ENTRY(tip_entry), "");
}

void panel_item_apply_options(void)
{
    const char *temp;

    g_free(pitem->command);
    temp = gtk_entry_get_text(GTK_ENTRY(command_entry));

    if(temp && *temp)
        pitem->command = g_strdup(temp);
    else
        pitem->command = NULL;

    pitem->in_terminal =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));

    g_free(pitem->tooltip);
    temp = gtk_entry_get_text(GTK_ENTRY(tip_entry));

    if(temp && *temp)
        pitem->tooltip = g_strdup(temp);
    else
        pitem->tooltip = NULL;

    pitem->icon_id = icon_id;

    g_free(pitem->icon_path);

    if(icon_path && pitem->icon_id == EXTERN_ICON)
        pitem->icon_path = g_strdup(icon_path);
    else
        pitem->icon_path = NULL;

    panel_item_apply_config(pitem);
}

void panel_item_add_options(PanelControl * pc, GtkContainer * container,
                            GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *main_hbox;
    GtkWidget *options_box;
    GtkWidget *preview_frame;

    pitem = (PanelItem *) pc->data;
    mitem = NULL;

    revert_button = revert;

    g_signal_connect_swapped(revert, "clicked",
                             G_CALLBACK(panel_item_revert_options), pitem);

    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(panel_item_apply_options), pitem);

    /* create backup for revert */
    init_backup();

    if(pitem->command)
        backup.command = g_strdup(pitem->command);

    backup.in_terminal = pitem->in_terminal;

    if(pitem->tooltip)
        backup.tooltip = g_strdup(pitem->tooltip);

    backup.icon_id = pitem->icon_id;

    if(pitem->icon_path)
        backup.icon_path = g_strdup(pitem->icon_path);

    main_hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(main_hbox);
    gtk_container_add(container, main_hbox);

    /* clean backup when dialog is destroyed */
    g_signal_connect(main_hbox, "destroy-event",
                     G_CALLBACK(clear_backup), NULL);

    options_box = create_item_options_box();
    gtk_box_pack_start(GTK_BOX(main_hbox), options_box, TRUE, TRUE, 0);

    preview_frame = create_icon_preview_frame();
    gtk_box_pack_start(GTK_BOX(main_hbox), preview_frame, TRUE, FALSE, 0);

    /* fill in the structures 
     * use the backup values because the item may have changed
     * after connecting callbacks */
    if(backup.command)
        gtk_entry_set_text(GTK_ENTRY(command_entry), backup.command);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 backup.in_terminal);

    change_icon(backup.icon_id, backup.icon_path);

    if(backup.tooltip)
        gtk_entry_set_text(GTK_ENTRY(tip_entry), backup.tooltip);
}

/*  Menu item options box
 *  ---------------------
 *  Much of the code is shared with the panel item option box
*/
static void menu_item_apply_options(void)
{
    const char *temp;
    PanelPopup *pp = mitem->parent;

    g_free(mitem->command);
    temp = gtk_entry_get_text(GTK_ENTRY(command_entry));

    if(temp && *temp)
        mitem->command = g_strdup(temp);
    else
        mitem->command = NULL;

    mitem->in_terminal =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term_checkbutton));

    g_free(mitem->tooltip);
    temp = gtk_entry_get_text(GTK_ENTRY(tip_entry));

    if(temp && *temp)
        mitem->tooltip = g_strdup(temp);
    else
        mitem->tooltip = NULL;

    mitem->icon_id = icon_id;

    g_free(mitem->icon_path);

    if(icon_path && mitem->icon_id == EXTERN_ICON)
        mitem->icon_path = g_strdup(icon_path);
    else
        mitem->icon_path = NULL;

    g_free(mitem->caption);
    temp = gtk_entry_get_text(GTK_ENTRY(caption_entry));

    if(temp && *temp)
        mitem->caption = g_strdup(temp);
    else
        mitem->caption = NULL;

    gtk_box_reorder_child(GTK_BOX(pp->vbox), mitem->button, mitem->pos + 2);

    menu_item_apply_config(mitem);
}

void menu_item_revert_options(void)
{
    g_free(mitem->command);
    mitem->command = g_strdup(backup.command);

    mitem->in_terminal = backup.in_terminal;

    g_free(mitem->tooltip);
    mitem->tooltip = g_strdup(backup.tooltip);

    g_free(mitem->caption);
    mitem->tooltip = g_strdup(backup.caption);

    mitem->icon_id = backup.icon_id;

    g_free(mitem->icon_path);
    mitem->icon_path = g_strdup(backup.icon_path);

    mitem->pos = backup.pos;

    menu_item_apply_config(mitem);

    gtk_box_reorder_child(GTK_BOX(mitem->parent->vbox), mitem->button,
                          mitem->pos + 2);

    /* revert the widgets */
    if(mitem->command)
        gtk_entry_set_text(GTK_ENTRY(command_entry), mitem->command);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 mitem->in_terminal);

    change_icon(mitem->icon_id, mitem->icon_path);

    if(mitem->tooltip)
        gtk_entry_set_text(GTK_ENTRY(tip_entry), mitem->tooltip);

    if(mitem->caption)
        gtk_entry_set_text(GTK_ENTRY(caption_entry), mitem->caption);

    if(num_items > 1)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(pos_spin),
                                  (gfloat) mitem->pos + 1);

    gtk_widget_set_sensitive(revert_button, FALSE);
}

void menu_item_add_options(MenuItem * item, GtkContainer * container)
{
    GtkWidget *main_hbox;
    GtkWidget *options_box;
    GtkWidget *preview_frame;

    pitem = NULL;
    mitem = item;

    init_backup();

    backup.command = g_strdup(mitem->command);
    backup.in_terminal = mitem->in_terminal;
    backup.tooltip = g_strdup(mitem->tooltip);
    backup.caption = g_strdup(mitem->caption);
    backup.icon_id = mitem->icon_id;
    backup.icon_path = g_strdup(mitem->icon_path);
    backup.pos = mitem->pos;

    main_hbox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(main_hbox);
    gtk_container_add(container, main_hbox);

    options_box = create_item_options_box();
    gtk_box_pack_start(GTK_BOX(main_hbox), options_box, TRUE, TRUE, 0);

    preview_frame = create_icon_preview_frame();
    gtk_box_pack_start(GTK_BOX(main_hbox), preview_frame, TRUE, FALSE, 0);

    /* fill in the structures use the backup values 
     * because the item values may have changed when connecting signals */
    if(backup.command)
        gtk_entry_set_text(GTK_ENTRY(command_entry), backup.command);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(term_checkbutton),
                                 backup.in_terminal);

    change_icon(backup.icon_id, backup.icon_path);

    if(backup.tooltip)
        gtk_entry_set_text(GTK_ENTRY(tip_entry), backup.tooltip);

    if(backup.caption)
        gtk_entry_set_text(GTK_ENTRY(caption_entry), backup.caption);

    if(num_items > 1)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(pos_spin),
                                  (gfloat) backup.pos + 1);

    menu_item_apply_options();
}

/*  Menu item dialogs
 *  -----------------
 *  Edit or add menu items
*/
GtkWidget *create_menu_item_dialog(MenuItem * mi)
{
    char *title;
    GtkWidget *dlg;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *main_vbox;
    GtkWidget *frame;
    GtkWidget *main_hbox;
    GtkWidget *options_box;
    GtkWidget *preview_frame;
    GtkWidget *remove_button;

    /* create dialog */
    title = _("Change menu item");

    dlg = gtk_dialog_new_with_buttons(title, GTK_WINDOW(toplevel),
                                      GTK_DIALOG_MODAL, NULL);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);

    /* add buttons */
    remove_button = mixed_button_new(GTK_STOCK_REMOVE, _("Remove"));
    gtk_widget_show(remove_button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), remove_button,
                                 RESPONSE_REMOVE);

    revert_button = mixed_button_new(GTK_STOCK_UNDO, _("Revert"));
    gtk_widget_show(revert_button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), revert_button,
                                 RESPONSE_REVERT);

    done_button = mixed_button_new(GTK_STOCK_OK, _("Done"));
    gtk_widget_show(done_button);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), done_button, RESPONSE_DONE);

    /* the options */
    main_vbox = GTK_DIALOG(dlg)->vbox;

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

    menu_item_add_options(mi, GTK_CONTAINER(frame));

    /* signals */
    g_signal_connect(revert_button, "clicked",
                     G_CALLBACK(menu_item_revert_options), NULL);

    g_signal_connect(done_button, "clicked",
                     G_CALLBACK(menu_item_apply_options), NULL);

    return dlg;
}

static void reposition_popup(PanelPopup * pp)
{
    if(pp->detached)
        return;

    gtk_button_clicked(GTK_BUTTON(pp->button));
    gtk_button_clicked(GTK_BUTTON(pp->button));
}

void edit_menu_item_dialog(MenuItem * mi)
{
    GtkWidget *dlg;

    mitem = mi;
    pitem = NULL;

    num_items = g_list_length(mi->parent->items);

    dlg = create_menu_item_dialog(mi);

    /* run dialog until 'Done' */
    while(1)
    {
        int response = GTK_RESPONSE_NONE;

        gtk_widget_set_sensitive(revert_button, FALSE);

        response = gtk_dialog_run(GTK_DIALOG(dlg));

        if(response == RESPONSE_REVERT)
            continue;

        /* the options are already applied, so we only have to deal
         * with removal */
        if(response == RESPONSE_REMOVE)
        {
            PanelPopup *pp = mi->parent;

            gtk_container_remove(GTK_CONTAINER(pp->vbox), mi->button);
            pp->items = g_list_remove(pp->items, mi);
            menu_item_free(mi);

            reposition_popup(pp);
        }

        break;
    }

    gtk_widget_destroy(dlg);
    num_items = 0;

    clear_backup();

    if(icon_path)
    {
        g_free(icon_path);
        icon_path = NULL;
    }
}

void add_menu_item_dialog(PanelPopup * pp)
{
    MenuItem *mi = menu_item_new(pp);

    create_menu_item(mi);

    panel_popup_add_item(pp, mi);

    reposition_popup(pp);

    edit_menu_item_dialog(mi);
}
