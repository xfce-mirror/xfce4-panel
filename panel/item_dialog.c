/*  xfce4
 *
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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
 *  Basically the same as for icon panel items. Addtional options for 
 *  caption and position in menu.
 *
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/debug.h>
#include <libxfce4util/i18n.h>

#include "xfce.h"
#include "groups.h"
#include "popup.h"
#include "item.h"
#include "item_dialog.h"
#include "settings.h"
#include "xfcombo.h"

#define BORDER 6

#define PREVIEW_SIZE 48

typedef struct _ItemDialog ItemDialog;

struct _ItemDialog
{
    Control *control;
    Item *item;

    GtkContainer *container;
    GtkWidget *close;
    
    /* command */
    CommandOptions *cmd_opts;

    /* icon */
    IconOptions *icon_opts;
    
    /* Name and Tooltip */
    GtkWidget *caption_entry;
    GtkWidget *tip_entry;

    /* menu */
    GtkWidget *menu_checkbutton;
};

/* globals */
static GtkWidget *menudialog = NULL;	/* keep track of this for signal 
					   handling */

/* useful widgets */

static void 
add_spacer (GtkBox *box, int size)
{
    GtkWidget *align;

    align = gtk_alignment_new (0,0,0,0);
    gtk_widget_set_size_request (align, size, size);
    gtk_widget_show (align);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}


/* CommandOptions 
 * --------------
*/

static void
combo_completion_cb (xfc_combo_info_t *info, CommandOptions *opts)
{
    const char *cmd;
    GtkToggleButton *tb;
    gboolean in_term;

    cmd = gtk_entry_get_text (GTK_ENTRY(opts->command_entry));

    tb = GTK_TOGGLE_BUTTON (opts->term_checkbutton);
    in_term = history_check_in_terminal (cmd);
    gtk_toggle_button_set_active (tb, in_term);
    
    if (opts->on_change)
    {
	gboolean use_sn = FALSE;

	if (opts->sn_checkbutton)
	{
	    tb = GTK_TOGGLE_BUTTON (opts->sn_checkbutton);
	    use_sn = gtk_toggle_button_get_active (tb);
	}
	
	opts->on_change (cmd, in_term, use_sn, opts->data);
    }
}

static void 
command_browse_cb (GtkWidget *w, CommandOptions *opts)
{
    char *file;
    const char *text;
 
    text = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

    file = select_file_name (_("Select command"), text, 
	    		     gtk_widget_get_toplevel (opts->base));

    if (file)
    {
	if (opts->info)
	    completion_combo_set_text (opts->info, file);
	else
	    gtk_entry_set_text (GTK_ENTRY (opts->command_entry), file);
	
	gtk_editable_set_position (GTK_EDITABLE (opts->command_entry), -1);
    
	if (opts->on_change)
	{
	    GtkToggleButton *tb;
	    gboolean in_term, use_sn = FALSE;
	    
	    tb = GTK_TOGGLE_BUTTON (opts->term_checkbutton);
	    in_term = gtk_toggle_button_get_active (tb);
	    
	    if (opts->sn_checkbutton)
	    {
		tb = GTK_TOGGLE_BUTTON (opts->sn_checkbutton);
		use_sn = gtk_toggle_button_get_active (tb);
	    }
	
	    
	    opts->on_change (file, in_term, use_sn, opts->data);
	}
	
	g_free (file);
    }
}

static void
command_toggle_cb (GtkWidget *w, CommandOptions *opts)
{
    if (opts->on_change)
    {
	GtkToggleButton *tb;
	gboolean in_term, use_sn = FALSE;
	const char *cmd;
	
	cmd = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

	if (!cmd || !strlen(cmd))
	    cmd = NULL;
	
	tb = GTK_TOGGLE_BUTTON (opts->term_checkbutton);
	in_term = gtk_toggle_button_get_active (tb);
	
	if (opts->sn_checkbutton)
	{
	    tb = GTK_TOGGLE_BUTTON (opts->sn_checkbutton);
	    use_sn = gtk_toggle_button_get_active (tb);
	}
	
	opts->on_change (cmd, in_term, use_sn, opts->data);
    }
}

CommandOptions *
create_command_options (GtkSizeGroup *sg)
{
    GtkWidget *w, *vbox, *hbox, *image;
    CommandOptions *opts;

    if (!sg)
	sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    
    opts = g_new0 (CommandOptions, 1);
    
    opts->base = vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
 
    /* entry */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    w = gtk_label_new (_("Command:"));
    gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, w);
    
    /* only available when compiled with libdbh support and 
     * libxfce4_combo module is installed */
    opts->info = 
	create_completion_combo ((ComboCallback)combo_completion_cb, opts);

    if (opts->info)
    {
	DBG ("using completion combo");
	opts->command_entry = GTK_WIDGET (opts->info->entry);
	w = GTK_WIDGET (opts->info->combo);
    }
    else
    {
	opts->command_entry = w = gtk_entry_new ();
    }

    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);

    w = gtk_button_new ();
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (w), image);

    g_signal_connect (w, "clicked", G_CALLBACK (command_browse_cb), 
	    	      opts);

    /* TODO: xfce4-appfinder support (desktop files / menu spec) */
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    w = gtk_label_new ("");
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, w);
    
    /* terminal */
    opts->term_checkbutton = w = 
	gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    g_signal_connect (w, "toggled", G_CALLBACK (command_toggle_cb), 
	    	      opts);
    
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    w = gtk_label_new ("");
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, w);
    
    opts->sn_checkbutton = w = 
	gtk_check_button_new_with_mnemonic (_("Use startup _notification"));
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    g_signal_connect (w, "toggled", G_CALLBACK (command_toggle_cb), 
	    	      opts);
#endif

    g_signal_connect_swapped (opts->base, "destroy", 
	    		      G_CALLBACK (destroy_command_options), opts);
    
    return opts;
}

void 
destroy_command_options (CommandOptions *opts)
{
    if (opts->on_change)
    {
	GtkToggleButton *tb;
	gboolean in_term, use_sn = FALSE;
	const char *cmd;
	
	cmd = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

	if (!cmd || !strlen(cmd))
	    cmd = NULL;
	
	tb = GTK_TOGGLE_BUTTON (opts->term_checkbutton);
	in_term = gtk_toggle_button_get_active (tb);

	if (opts->sn_checkbutton)
	{
	    tb = GTK_TOGGLE_BUTTON (opts->sn_checkbutton);
	    use_sn = gtk_toggle_button_get_active (tb);
	}
	
	opts->on_change (cmd, in_term, use_sn, opts->data);
    }

    if (opts->info)
	destroy_completion_combo (opts->info);

    g_free (opts);
}

void 
command_options_set_command (CommandOptions *opts, const char *command, 
			     gboolean in_term, gboolean use_sn)
{
    const char *cmd = (command != NULL) ? command : "";
    
    if (opts->info)
    {
	completion_combo_set_text (opts->info, cmd);
    }
    else
    {
	gtk_entry_set_text (GTK_ENTRY (opts->command_entry), cmd);
    }
    
    gtk_editable_set_position (GTK_EDITABLE (opts->command_entry), -1);
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (opts->term_checkbutton),
	    			  in_term);

    if (opts->sn_checkbutton)
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (opts->sn_checkbutton),
				      use_sn);
    }
    
    if (opts->on_change)
	opts->on_change (command, in_term, use_sn, opts->data);
}

void 
command_options_set_callback (CommandOptions *opts, 
			      void (*callback)(const char *, gboolean, 
				  	       gboolean, gpointer),
			      gpointer data)
{
    opts->on_change = callback;
    opts->data = data;
}

void 
command_options_get_command  (CommandOptions *opts, char **command, 
			      gboolean *in_term, gboolean *use_sn)
{
    const char *tmp;
    GtkToggleButton *tb;

    tmp = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

    if (tmp && strlen(tmp))
	*command = g_strdup (tmp);
    else
	*command = NULL;

    tb = GTK_TOGGLE_BUTTON (opts->term_checkbutton);
    *in_term = gtk_toggle_button_get_active (tb);

    if (opts->sn_checkbutton)
    {
	tb = GTK_TOGGLE_BUTTON (opts->sn_checkbutton);
	*use_sn = gtk_toggle_button_get_active (tb);
    }
    else
    {
	*use_sn = FALSE;
    }
}


/* IconOptions
 * -----------
*/

static void
update_icon_preview (int id, const char *path, IconOptions *opts)
{
    int w, h;
    GdkPixbuf *pb = NULL;

    if (id == EXTERN_ICON && path && 
	g_file_test (path, G_FILE_TEST_EXISTS) && 
	!g_file_test (path, G_FILE_TEST_IS_DIR))
    {
	pb = gdk_pixbuf_new_from_file (path, NULL);
    }
    else
    {
	pb = get_pixbuf_by_id (id);
    }

    if (!pb)
	pb = get_pixbuf_by_id (UNKNOWN_ICON);

    w = gdk_pixbuf_get_width (pb);
    h = gdk_pixbuf_get_height (pb);

    if (w > PREVIEW_SIZE || h > PREVIEW_SIZE)
    {
	GdkPixbuf *newpb;
	
	if (w > h)
	{
	    h = (int) (((double) PREVIEW_SIZE / (double) w) * (double) h);
	    w = PREVIEW_SIZE;
	}
	else
	{
	    w = (int) (((double) PREVIEW_SIZE / (double) h) * (double) w);
	    h = PREVIEW_SIZE;
	}

	newpb = gdk_pixbuf_scale_simple (pb, w, h, GDK_INTERP_BILINEAR);
	g_object_unref (pb);
	pb = newpb;
    }
    
    gtk_image_set_from_pixbuf (GTK_IMAGE (opts->image), pb);
    g_object_unref (pb);
}

static void
icon_id_changed (GtkOptionMenu *om, IconOptions *opts)
{
    int id;
    char *path = NULL;

    id = gtk_option_menu_get_history (om);
    
    if (id == 0)
	id = EXTERN_ICON;
    
    if (id == opts->icon_id)
	return;
    
    if (id == EXTERN_ICON && opts->saved_path)
	path = opts->saved_path;

    icon_options_set_icon (opts, id, path);
}

static GtkWidget *
create_icon_option_menu (void)
{
    GtkWidget *om, *menu, *mi;
    int i;

    menu = gtk_menu_new ();
    
    mi = gtk_menu_item_new_with_label (_("Other Icon"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    for (i = 1; i < NUM_ICONS; i++)
    {
	mi = gtk_menu_item_new_with_label (icon_names[i]);
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    om = gtk_option_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (om), menu);

    return om;
}

gboolean
icon_entry_lost_focus (GtkEntry * entry, GdkEventFocus * event, 
		       IconOptions *opts)
{
    const char *temp = gtk_entry_get_text (entry);

    if (temp)
	icon_options_set_icon (opts, EXTERN_ICON, temp);

    /* we must return FALSE or gtk will crash :-( */
    return FALSE;
}

static void 
icon_browse_cb (GtkWidget *w, IconOptions *opts)
{
    char *file;
    const char *text;
    GdkPixbuf *test = NULL;
 
    text = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));

    file = select_file_with_preview (_("Select command"), text, 
	    		             gtk_widget_get_toplevel (opts->base));

    if (file && g_file_test (file, G_FILE_TEST_EXISTS) && 
	!g_file_test (file, G_FILE_TEST_IS_DIR))
    {
	test = gdk_pixbuf_new_from_file (file, NULL);

	if (test)
	{
	    g_object_unref (test);
	    
	    icon_options_set_icon (opts, EXTERN_ICON, file);
	}
    }
    
    g_free (file);
}

static void
xtm_cb (GtkWidget * b, GtkEntry * entry)
{
    gchar *argv[2] = { "xffm_theme_maker", NULL };
    
    g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, 
	    	   NULL, NULL, NULL, NULL);
}

static void
icon_drop_cb (GtkWidget * widget, GdkDragContext * context,
	      gint x, gint y, GtkSelectionData * data,
	      guint info, guint time, IconOptions *opts)
{
    GList *fnames;
    guint count;

    fnames = gnome_uri_list_extract_filenames ((char *) data->data);
    count = g_list_length (fnames);

    if (count > 0)
    {
	char *icon;
	GdkPixbuf *test;

	icon = (char *) fnames->data;

	test = gdk_pixbuf_new_from_file (icon, NULL);

	if (test)
	{
	    icon_options_set_icon (opts, EXTERN_ICON, icon);
	    g_object_unref (test);
	}
    }

    gnome_uri_list_free_strings (fnames);
    gtk_drag_finish (context, (count > 0),
		     (context->action == GDK_ACTION_MOVE), time);
}

static GtkWidget *
create_icon_preview_frame (IconOptions *opts)
{
    GtkWidget *frame;
    GtkWidget *eventbox;

    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    gtk_widget_show (frame);

    eventbox = gtk_event_box_new ();
    add_tooltip (eventbox, _("Drag file onto this frame to change the icon"));
    gtk_widget_show (eventbox);
    gtk_container_add (GTK_CONTAINER (frame), eventbox);

    opts->image = gtk_image_new ();
    gtk_widget_show (opts->image);
    gtk_container_add (GTK_CONTAINER (eventbox), opts->image);

    /* signals */
    dnd_set_drag_dest (eventbox);

    g_signal_connect (eventbox, "drag_data_received",
		      G_CALLBACK (icon_drop_cb), opts);

    return frame;
}

IconOptions *
create_icon_options (GtkSizeGroup *sg, gboolean use_builtins)
{
    GtkWidget *w, *vbox, *hbox, *image;
    IconOptions *opts;

    opts = g_new0 (IconOptions, 1);
    
    opts->base = hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    /* image preview */
    w = create_icon_preview_frame (opts);
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, TRUE, 0);
    
    if (sg)
	gtk_size_group_add_widget (sg, w);
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

    /* icon option menu */
    if (use_builtins)
    {
	opts->icon_menu = w = create_icon_option_menu ();
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);

	opts->id_sig = g_signal_connect (w, "changed", 
					 G_CALLBACK (icon_id_changed), opts);
    }
    
    /* icon entry */
    opts->icon_entry = w = gtk_entry_new ();
    gtk_widget_show (w);

    if (use_builtins)
    {
	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);
    }
    else
    {
	gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, BORDER);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
    }
    
    g_signal_connect (w, "focus-out-event",
		      G_CALLBACK (icon_entry_lost_focus), opts);

    w = gtk_button_new ();
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (w), image);

    g_signal_connect (w, "clicked", G_CALLBACK (icon_browse_cb), opts);

    /* use xffm_theme_maker when available */
    {
	gchar *g = g_find_program_in_path ("xffm_theme_maker");

	if (g)
	{
	    GtkWidget *xtm_button = gtk_button_new ();

	    gtk_box_pack_start (GTK_BOX (hbox), xtm_button, FALSE, FALSE, 0);
	    gtk_widget_show (xtm_button);

	    image =
		gtk_image_new_from_stock (GTK_STOCK_SELECT_COLOR,
					  GTK_ICON_SIZE_BUTTON);
	    gtk_widget_show (image);
	    gtk_container_add (GTK_CONTAINER (xtm_button), image);

	    g_signal_connect (xtm_button, "clicked",
			      G_CALLBACK (xtm_cb), opts);
	    g_free (g);
	}
    }

    g_signal_connect_swapped (opts->base, "destroy", 
	    		      G_CALLBACK (destroy_icon_options), opts);
    
    return opts;
}

void 
destroy_icon_options (IconOptions *opts)
{
    if (opts->on_change)
    {
	int id;
	const char *icon_path = NULL;

	id = gtk_option_menu_get_history (GTK_OPTION_MENU (opts->icon_menu));

	if (id == 0)
	{
	    id = EXTERN_ICON;
	    
	    icon_path = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));
	    
	    if (!icon_path || !strlen (icon_path))
		icon_path = NULL;
	}

	opts->on_change (id, icon_path, opts->data);
    }

    g_free (opts->saved_path);

    g_free (opts);
}

void 
icon_options_set_icon (IconOptions *opts, int id, const char *path)
{
    g_signal_handler_block (opts->icon_menu, opts->id_sig);
    gtk_option_menu_set_history (GTK_OPTION_MENU (opts->icon_menu), 
	    			 (id == EXTERN_ICON) ? 0 : id);
    g_signal_handler_unblock (opts->icon_menu, opts->id_sig);

    if (id == EXTERN_ICON)
    {
	gtk_entry_set_text (GTK_ENTRY (opts->icon_entry), path);
	gtk_editable_set_position (GTK_EDITABLE (opts->icon_entry), -1);
    
	gtk_widget_set_sensitive (opts->icon_entry, TRUE);
    }
    else 
    {
	const char *icon_path = NULL;
    
	if (opts->icon_id == EXTERN_ICON)
	    icon_path = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));

	if (icon_path && strlen (icon_path))
	{
	    g_free (opts->saved_path);
	    opts->saved_path = g_strdup (icon_path);
	}

	gtk_entry_set_text (GTK_ENTRY (opts->icon_entry), "");
	gtk_widget_set_sensitive (opts->icon_entry, FALSE);
    }

    opts->icon_id = id;
    
    update_icon_preview (id, path, opts);

    if (opts->on_change)
	opts->on_change (id, path, opts->data);
}

void 
icon_options_set_callback (IconOptions *opts, 
			   void (*callback)(int, const char *, gpointer),
			   gpointer data)
{
    opts->on_change = callback;
    opts->data = data;
}

void 
icon_options_get_icon  (IconOptions *opts, int *id, char **path)
{
    *id = gtk_option_menu_get_history (GTK_OPTION_MENU (opts->icon_menu));

    *path = NULL;

    if (*id == 0)
    {
	const char *icon_path;

	*id = EXTERN_ICON;
	
	icon_path = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));
	
	if (icon_path && strlen (icon_path))
	    *path = g_strdup (icon_path);
    }
}

/* ItemDialog 
 * ----------
*/

static void
icon_options_changed (int id, const char *path, gpointer data)
{
    ItemDialog *idlg = data;
    Item *item = idlg->item;

    item->icon_id = id;

    g_free (item->icon_path);

    if (path)
	item->icon_path = g_strdup (path);
    else
	item->icon_path = NULL;
    
    item_apply_config (item);
}

static inline void
add_command_options (GtkBox *box, ItemDialog *idlg, GtkSizeGroup *sg)
{
    idlg->cmd_opts = create_command_options (sg);
    
    command_options_set_command (idlg->cmd_opts, idlg->item->command, 
	    			 idlg->item->in_terminal, idlg->item->use_sn);

    gtk_box_pack_start (box, idlg->cmd_opts->base, FALSE, TRUE, 0);
}

static inline void
add_icon_options (GtkBox *box, ItemDialog *idlg, GtkSizeGroup *sg)
{
    idlg->icon_opts = create_icon_options (sg, TRUE);

    icon_options_set_icon (idlg->icon_opts, idlg->item->icon_id, 
	    		   idlg->item->icon_path);

    icon_options_set_callback (idlg->icon_opts, icon_options_changed, idlg);

    gtk_box_pack_start (box, idlg->icon_opts->base, FALSE, TRUE, 0);
}

static gboolean
caption_entry_lost_focus (GtkWidget *entry, GdkEventFocus *event, Item *item)
{
    const char *tmp;

    tmp = gtk_entry_get_text (GTK_ENTRY(entry));

    if (tmp && strlen (tmp))
    {
	g_free (item->caption);
	item->caption = g_strdup (tmp);

	item_apply_config (item);
    }

    /* we must return FALSE or gtk will crash :-( */
    return FALSE;
}

static inline void
add_caption_option (GtkBox *box, ItemDialog *idlg, GtkSizeGroup *sg)
{
    GtkWidget *hbox;
    GtkWidget *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Caption:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    idlg->caption_entry = gtk_entry_new ();
    gtk_widget_show (idlg->caption_entry);
    gtk_box_pack_start (GTK_BOX (hbox), idlg->caption_entry, TRUE, TRUE, 0);

    if (idlg->item->caption)
	gtk_entry_set_text (GTK_ENTRY (idlg->caption_entry), 
			    idlg->item->caption);

    /* only set label on focus out */
    g_signal_connect (idlg->caption_entry, "focus-out-event",
		      G_CALLBACK (caption_entry_lost_focus), idlg->item);
}

static inline void
add_tooltip_option (GtkBox *box, ItemDialog *idlg, GtkSizeGroup *sg)
{
    GtkWidget *hbox;
    GtkWidget *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Tooltip:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    idlg->tip_entry = gtk_entry_new ();
    gtk_widget_show (idlg->tip_entry);
    gtk_box_pack_start (GTK_BOX (hbox), idlg->tip_entry, TRUE, TRUE, 0);

    if (idlg->item->tooltip)
	gtk_entry_set_text (GTK_ENTRY (idlg->tip_entry), idlg->item->tooltip);
}

static void
popup_menu_changed (GtkToggleButton * tb, Control *control)
{
    Item *item = control->data;
    
    item->with_popup = gtk_toggle_button_get_active (tb);

    groups_show_popup (control->index, item->with_popup);
}

static inline void
add_menu_option (GtkBox *box, ItemDialog *idlg, GtkSizeGroup *sg)
{
#if 0
    GtkWidget *hbox;
    GtkWidget *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Menu:"));
    gtk_size_group_add_widget (sg, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
#endif
    idlg->menu_checkbutton =
	gtk_check_button_new_with_label (_("Attach menu to launcher"));
    gtk_widget_show (idlg->menu_checkbutton);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (idlg->menu_checkbutton),
				  idlg->item->with_popup);
#if 0
    gtk_box_pack_start (GTK_BOX (hbox), idlg->menu_checkbutton, 
	    		FALSE, FALSE, 0);
#else
    gtk_box_pack_start (box, idlg->menu_checkbutton, FALSE, FALSE, 0);
#endif    

    g_signal_connect (idlg->menu_checkbutton, "toggled",
		      G_CALLBACK (popup_menu_changed), idlg->control);
}

static void
item_dialog_closed (GtkWidget *w, ItemDialog *idlg)
{
    Item *item;
    const char *temp;

    item = idlg->item;
    
    /* command */
    g_free (item->command);
    item->command = NULL;

    command_options_get_command (idlg->cmd_opts, &(item->command),
	    			 &(item->in_terminal), &(item->use_sn));

    /* icon */
    g_free (item->icon_path);
    item->icon_path = NULL;

    icon_options_get_icon (idlg->icon_opts, &(item->icon_id), 
	    		   &(item->icon_path));
    
    /* tooltip */
    g_free (item->tooltip);
    item->tooltip = NULL;
    
    temp = gtk_entry_get_text (GTK_ENTRY (idlg->tip_entry));

    if (temp && *temp)
	item->tooltip = g_strdup (temp);

    if (item->type == MENUITEM)
    {
	/* caption */
	g_free (item->caption);
	item->caption = NULL;

	temp = gtk_entry_get_text (GTK_ENTRY (idlg->caption_entry));
	
	if (temp && *temp)
	    item->caption = g_strdup (temp);
    }

    item_apply_config (item);
}

static void
destroy_item_dialog (ItemDialog *idlg)
{
    /* TODO: check if these are still here */
    command_options_set_callback (idlg->cmd_opts, NULL, NULL);
    icon_options_set_callback (idlg->icon_opts, NULL, NULL);

    g_free (idlg);
}

static void
create_item_dialog (Control *control, Item *item, 
		    GtkContainer *container, GtkWidget *close)
{
    ItemDialog *idlg;
    GtkWidget *vbox;
    GtkSizeGroup *sg;

    idlg = g_new0 (ItemDialog, 1);

    if (control)
    {
	idlg->control = control;
	idlg->item = (Item *)control->data;
    }
    else
    {
	idlg->item = item;
    }
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    
    add_icon_options (GTK_BOX (vbox), idlg, sg);
    add_spacer (GTK_BOX (vbox), BORDER);

    add_command_options (GTK_BOX (vbox), idlg, sg);
    add_spacer (GTK_BOX (vbox), BORDER);
    
    if (!control)
    {
	add_caption_option (GTK_BOX (vbox), idlg, sg);
    }

    add_tooltip_option (GTK_BOX (vbox), idlg, sg);

    if (control)
    {
	add_spacer (GTK_BOX (vbox), BORDER);
	add_menu_option (GTK_BOX (vbox), idlg, sg);
    }

    add_spacer (GTK_BOX (vbox), BORDER);
    
    g_signal_connect (close, "clicked", 
	    	      G_CALLBACK (item_dialog_closed), idlg);

    g_signal_connect_swapped (container, "destroy", 
	    	      	      G_CALLBACK (destroy_item_dialog), idlg);
}

/* panel and menu item API 
 * -----------------------
*/

/* panel control */

void 
panel_item_create_options (Control * control, GtkContainer * container,
			   GtkWidget * done)
{
    create_item_dialog (control, NULL, container, done);
}

/* menu item */

static void
reindex_items (GList * items)
{
    Item *item;
    GList *li;
    int i;

    for (i = 0, li = items; li; i++, li = li->next)
    {
	item = li->data;

	item->pos = i;
    }
}

static void
pos_changed (GtkSpinButton * spin, Item *item)
{
    int n = gtk_spin_button_get_value_as_int (spin) - 1;
    PanelPopup *pp = item->parent;

    if (n == item->pos)
	return;

    pp->items = g_list_remove (pp->items, item);
    pp->items = g_list_insert (pp->items, item, n);
    reindex_items (pp->items);

    gtk_box_reorder_child (GTK_BOX (pp->item_vbox), item->button,
			   item->pos);
}

static void
add_position_option (GtkBox *box, Item *item, int num_items)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *pos_spin;

    g_return_if_fail (num_items > 1);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Position:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    pos_spin = gtk_spin_button_new_with_range (1, num_items, 1);
    gtk_widget_show (pos_spin);
    gtk_box_pack_start (GTK_BOX (hbox), pos_spin, FALSE, FALSE, 0);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (pos_spin), 
	    		       (gfloat) item->pos + 1);
    
    g_signal_connect (pos_spin, "value-changed", G_CALLBACK (pos_changed),
		      item);
}

static gboolean
menu_dialog_delete (GtkWidget *dlg)
{
    gtk_dialog_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    return TRUE;
}

void 
edit_menu_item_dialog (Item * mi)
{
    GtkWidget *remove, *close, *header;
    GtkDialog *dlg;
    int response, num_items;

    menudialog = gtk_dialog_new ();
    dlg = GTK_DIALOG (menudialog);
    
    gtk_window_set_title (GTK_WINDOW (dlg), _("Change menu item"));
    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

    gtk_dialog_set_has_separator (dlg, FALSE);
    
    /* add buttons */
    remove = mixed_button_new (GTK_STOCK_REMOVE, _("Remove"));
    gtk_widget_show (remove);
    gtk_dialog_add_action_widget (dlg, remove, GTK_RESPONSE_CANCEL);

    close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
    gtk_widget_show (close);
    gtk_dialog_add_action_widget (dlg, close, GTK_RESPONSE_OK);

    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX(dlg->action_area),
					remove, TRUE);

    header = create_header (NULL, _("Launcher"));
    gtk_container_set_border_width (GTK_CONTAINER (GTK_BIN (header)->child), 
	    			    BORDER);
    gtk_widget_set_size_request (header, -1, 32);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (dlg->vbox), header, FALSE, TRUE, 0);

    add_spacer (GTK_BOX (dlg->vbox), BORDER);
    
    /* position */
    num_items = g_list_length (mi->parent->items);

    if (num_items > 1)
    {
	add_position_option (GTK_BOX (dlg->vbox), mi, num_items);

	add_spacer (GTK_BOX (dlg->vbox), BORDER);
    }

    /* add the other options */
    create_item_dialog (NULL, mi, GTK_CONTAINER (dlg->vbox), close);

    add_spacer (GTK_BOX (dlg->vbox), BORDER);

    g_signal_connect (dlg, "delete-event", G_CALLBACK (menu_dialog_delete), 
	    	      NULL);
    
    gtk_widget_grab_focus (close);
    gtk_widget_grab_default (close);

    response = GTK_RESPONSE_NONE;
    response = gtk_dialog_run (dlg);

    if (response == GTK_RESPONSE_CANCEL)
    {
	PanelPopup *pp = mi->parent;
	
	gtk_container_remove (GTK_CONTAINER (pp->item_vbox), mi->button);
	pp->items = g_list_remove (pp->items, mi);
	item_free (mi);
	reindex_items (pp->items);
    }
    
    gtk_widget_destroy (menudialog);

    write_panel_config ();
}

void 
add_menu_item_dialog (PanelPopup * pp)
{
    Item *mi = menu_item_new (pp);

    create_menu_item (mi);
    mi->pos = 0;

    panel_popup_add_item (pp, mi);

    edit_menu_item_dialog (mi);
}

void
destroy_menu_dialog (void)
{
    if (menudialog)
	gtk_dialog_response (GTK_DIALOG (menudialog), GTK_RESPONSE_OK);
}

