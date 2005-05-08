/*  $Id$
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce.h"
#include "item_dialog.h"
#include "settings.h"

#define BORDER          8
#define PREVIEW_SIZE    32

static GtkTargetEntry target_entry[] = {
    {"text/uri-list", 0, 0},
    {"UTF8_STRING", 0, 1},
    {"STRING", 0, 2}
};

static const char *keys[] = {
    "Name",
    "GenericName",
    "Comment",
    "Icon",
    "Categories",
    "OnlyShowIn",
    "Exec",
    "Terminal"
};

/* CommandOptions 
 * --------------
*/

static void
command_browse_cb (GtkWidget * w, CommandOptions * opts)
{
    char *file;
    const char *text;

    text = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

    file = select_file_name (_("Select command"), text,
			     gtk_widget_get_toplevel (opts->base));

    if (file)
    {
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
command_toggle_cb (GtkWidget * w, CommandOptions * opts)
{
    if (opts->on_change)
    {
	GtkToggleButton *tb;
	gboolean in_term, use_sn = FALSE;
	const char *cmd;

	cmd = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

	if (!cmd || !strlen (cmd))
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

/* Drag and drop URI callback (Appfinder' stuff) */
/* TODO: 
 * - also add icon terminal checkbutton and tooltip (Comment) from 
 *   .desktop file 
 * - allow drop on window (no way to find window currently though...) 
 */
static void
drag_drop_cb (GtkWidget * widget, GdkDragContext * context, gint x,
	      gint y, GtkSelectionData * sd, guint info,
	      guint time, CommandOptions * opts)
{
    if (sd->data)
    {
        char *exec = NULL;
        char *buf, *s;
        XfceDesktopEntry *dentry;

        s = (char *) sd->data;

        if (!strncmp (s, "file", 5))
        {
            s += 5;

            if (!strncmp (s, "//", 2))
                s += 2;
        }

        buf = g_strdup (s);

        if ((s = strchr (buf, '\n')))
            *s = '\0';
        
	if (g_file_test (buf, G_FILE_TEST_EXISTS) &&
	    (dentry = xfce_desktop_entry_new (buf, keys, G_N_ELEMENTS (keys))))
	{ 
            xfce_desktop_entry_get_string (dentry, "Exec", FALSE, &exec);
            g_object_unref (dentry);
            g_free (buf);
	}
        else
        {
            exec = buf;
        }

        if (exec)
        {
            if ((s = g_strrstr (exec, " %")) != NULL)
            {
                s[0] = '\0';
            }
            
            command_options_set_command (opts, exec, FALSE, FALSE);
            g_free (exec);
        }
    }

    gtk_drag_finish (context, TRUE, FALSE, time);
}

G_MODULE_EXPORT /* EXPORT:create_command_options */
CommandOptions *
create_command_options (GtkSizeGroup * sg)
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

    opts->command_entry = w = gtk_entry_new ();

    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);

    w = gtk_button_new ();
    gtk_widget_show (w);
    gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

    image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (w), image);

    g_signal_connect (w, "clicked", G_CALLBACK (command_browse_cb), opts);

    /* xfce4-appfinder support (desktop files / menu spec) */
    gtk_drag_dest_set (opts->command_entry, GTK_DEST_DEFAULT_ALL, 
                       target_entry, G_N_ELEMENTS (target_entry), 
                       GDK_ACTION_COPY);
    g_signal_connect (opts->command_entry, "drag-data-received",
		      G_CALLBACK (drag_drop_cb), opts);

    gtk_drag_dest_set (opts->base, GTK_DEST_DEFAULT_ALL, target_entry, 
                       G_N_ELEMENTS (target_entry), GDK_ACTION_COPY);
    g_signal_connect (opts->base, "drag-data-received",
		      G_CALLBACK (drag_drop_cb), opts);

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

    g_signal_connect (w, "toggled", G_CALLBACK (command_toggle_cb), opts);

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

    g_signal_connect (w, "toggled", G_CALLBACK (command_toggle_cb), opts);
#endif

    g_signal_connect_swapped (opts->base, "destroy",
			      G_CALLBACK (destroy_command_options), opts);

    return opts;
}

G_MODULE_EXPORT /* EXPORT:destroy_command_options */
void
destroy_command_options (CommandOptions * opts)
{
    if (opts->on_change)
    {
	GtkToggleButton *tb;
	gboolean in_term, use_sn = FALSE;
	const char *cmd;

	cmd = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

	if (!cmd || !strlen (cmd))
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

    g_free (opts);
}

G_MODULE_EXPORT /* EXPORT:command_options_set_command */
void
command_options_set_command (CommandOptions * opts, const char *command,
			     gboolean in_term, gboolean use_sn)
{
    const char *cmd = (command != NULL) ? command : "";

    gtk_entry_set_text (GTK_ENTRY (opts->command_entry), cmd);

    gtk_editable_set_position (GTK_EDITABLE (opts->command_entry), -1);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (opts->term_checkbutton),
				  in_term);

    if (opts->sn_checkbutton)
    {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
				      (opts->sn_checkbutton), use_sn);
    }

    if (opts->on_change)
	opts->on_change (command, in_term, use_sn, opts->data);
}

G_MODULE_EXPORT /* EXPORT:command_options_set_callback */
void
command_options_set_callback (CommandOptions * opts,
			      void (*callback) (const char *, gboolean,
						gboolean, gpointer),
			      gpointer data)
{
    opts->on_change = callback;
    opts->data = data;
}

G_MODULE_EXPORT /* EXPORT:command_options_get_command */
void
command_options_get_command (CommandOptions * opts, char **command,
			     gboolean * in_term, gboolean * use_sn)
{
    const char *tmp;
    GtkToggleButton *tb;

    tmp = gtk_entry_get_text (GTK_ENTRY (opts->command_entry));

    if (tmp && strlen (tmp))
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
update_icon_preview (int id, const char *path, IconOptions * opts)
{
    int w, h;
    GdkPixbuf *pb = NULL;

    if (id == EXTERN_ICON)
    {
        if (path)
            pb = xfce_themed_icon_load (path, PREVIEW_SIZE);
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
icon_id_changed (GtkOptionMenu * om, IconOptions * opts)
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

G_MODULE_EXPORT /* EXPORT:icon_entry_lost_focus */
gboolean
icon_entry_lost_focus (GtkEntry * entry, GdkEventFocus * event,
		       IconOptions * opts)
{
    const char *temp = gtk_entry_get_text (entry);

    if (temp)
	icon_options_set_icon (opts, EXTERN_ICON, temp);

    /* we must return FALSE or gtk will crash :-( */
    return FALSE;
}

static void
icon_browse_cb (GtkWidget * w, IconOptions * opts)
{
    char *file;
    const char *text;
    GdkPixbuf *test = NULL;

    text = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));

    file = select_file_with_preview (NULL, text,
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
    gchar *argv[2] = { "xfmime-edit", NULL };

    g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
		   NULL, NULL, NULL, NULL);
}

static void
icon_drop_cb (GtkWidget * widget, GdkDragContext * context,
	      gint x, gint y, GtkSelectionData * data,
	      guint info, guint time, IconOptions * opts)
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
create_icon_preview_frame (IconOptions * opts)
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

G_MODULE_EXPORT /* EXPORT:create_icon_options */
IconOptions *
create_icon_options (GtkSizeGroup * sg, gboolean use_builtins)
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

    /* use xfmime-edit when available */
    {
	gchar *g = g_find_program_in_path ("xfmime-edit");

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

G_MODULE_EXPORT /* EXPORT:destroy_icon_options */
void
destroy_icon_options (IconOptions * opts)
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

G_MODULE_EXPORT /* EXPORT:icon_options_set_icon */
void
icon_options_set_icon (IconOptions * opts, int id, const char *path)
{
    const char *icon_path = NULL;

    if (opts->icon_id == EXTERN_ICON)
        icon_path = gtk_entry_get_text (GTK_ENTRY (opts->icon_entry));

    if (id == opts->icon_id)
    {
        if (id != EXTERN_ICON)
        {
            return;
        }
        else if (path && icon_path && 
                 strlen(icon_path) && !strcmp(path,icon_path))
        {
            return;
        }
    }
    
    g_signal_handler_block (opts->icon_menu, opts->id_sig);
    gtk_option_menu_set_history (GTK_OPTION_MENU (opts->icon_menu),
				 (id == EXTERN_ICON) ? 0 : id);
    g_signal_handler_unblock (opts->icon_menu, opts->id_sig);

    if (id == EXTERN_ICON || id == UNKNOWN_ICON)
    {
        if (path)
        {
            gtk_entry_set_text (GTK_ENTRY (opts->icon_entry), path);
            gtk_editable_set_position (GTK_EDITABLE (opts->icon_entry), -1);
        }

	gtk_widget_set_sensitive (opts->icon_entry, TRUE);
    }
    else
    {
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

G_MODULE_EXPORT /* EXPORT:icon_options_set_callback */
void
icon_options_set_callback (IconOptions * opts,
			   void (*callback) (int, const char *, gpointer),
			   gpointer data)
{
    opts->on_change = callback;
    opts->data = data;
}

G_MODULE_EXPORT /* EXPORT:icon_options_get_icon */
void
icon_options_get_icon (IconOptions * opts, int *id, char **path)
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

