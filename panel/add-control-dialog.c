/*  $Id$
 *  
 *  Copyright (C) 2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>

#include "xfce.h"

#include "add-control-dialog.h"

typedef struct
{
    Panel *panel;
    int position;
    
    GtkWidget *add_button;
    
    GSList *infolist;
    ControlInfo *current;
}
ControlList;

/* allow only one */
static GtkWidget *the_dialog = NULL;

/* prototypes */
static void dialog_response (GtkWidget *dlg, int response, ControlList *list);
static void dialog_destroyed (ControlList * list);
static void add_spacer (GtkBox * box, int size);
static void add_header (GtkBox * box);
static ControlList *add_control_list (GtkBox * box);

/**
 * add_control_dialog
 * @panel    : The #Panel to add item to
 * @position : desired position on the panel; use -1 to put the item
 *             at the end.
 **/
void
add_control_dialog (Panel * panel, int position)
{
    GtkWidget *dlg, *vbox, *cancel, *add;
    ControlList *list;

    if (the_dialog != NULL)
    {
	gtk_window_present (GTK_WINDOW (the_dialog));
	return;
    }

    dlg = gtk_dialog_new_with_buttons (_("Xfce Panel"),
				       GTK_WINDOW (panel->toplevel),
				       GTK_DIALOG_DESTROY_WITH_PARENT |
				       GTK_DIALOG_NO_SEPARATOR, NULL);

    cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_widget_show (cancel);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), cancel, 
	    			  GTK_RESPONSE_CANCEL);

    add = gtk_button_new_from_stock (GTK_STOCK_ADD);
    GTK_WIDGET_SET_FLAGS (add, GTK_CAN_DEFAULT);
    gtk_widget_show (add);
    gtk_dialog_add_action_widget (GTK_DIALOG (dlg), add, GTK_RESPONSE_OK);

    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    the_dialog = dlg;

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);

    vbox = GTK_DIALOG (dlg)->vbox;

    add_header (GTK_BOX (vbox));

    add_spacer (GTK_BOX (vbox), 12);

    list = add_control_list (GTK_BOX (vbox));


    add_spacer (GTK_BOX (vbox), 12);

    list->panel = panel;
    list->position = position;
    list->add_button = add;

    g_signal_connect_swapped (dlg, "destroy-event",
			      G_CALLBACK (dialog_destroyed), list);

    g_signal_connect (dlg, "response", G_CALLBACK (dialog_response), list);
    
    gtk_widget_show (dlg);
}

static void 
dialog_response (GtkWidget *dlg, int response, ControlList *list)
{

    gtk_widget_hide (dlg);
    
    if (response == GTK_RESPONSE_OK)
    {
	insert_control (list->panel, list->current->name, list->position);
    }

    gtk_widget_destroy (dlg);
    the_dialog = NULL;
}

static void
dialog_destroyed (ControlList * list)
{
    GSList *li;

    for (li = list->infolist; li != NULL; li = li->next)
    {
	ControlInfo *info = li->data;

	g_free (info->name);
	g_free (info->caption);

	if (info->icon)
	    g_object_unref (info->icon);

	g_free (info);
    }

    g_slist_free (list->infolist);

    g_free (list);
}

static void
add_spacer (GtkBox *box, int size)
{
    GtkWidget *align;

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_widget_set_size_request (align, size, size);
    gtk_box_pack_start (box, align, FALSE, FALSE, 0);
}    

static void
add_header (GtkBox * box)
{
    GtkWidget *header;
    GdkPixbuf *pb;

    pb = get_panel_pixbuf ();

    header = xfce_create_header (pb, _("Add new item"));
    gtk_widget_show (header);
    gtk_box_pack_start (box, header, FALSE, FALSE, 0);
}

static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

static void
cursor_changed (GtkTreeView * tv, ControlList * list)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    ControlInfo *info;

    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &info, -1);
    
    list->current = info;

    if (info->can_be_added)
	gtk_widget_set_sensitive (list->add_button, TRUE);
    else
	gtk_widget_set_sensitive (list->add_button, FALSE);
}

void
render_icon (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
    ControlInfo *info;

    gtk_tree_model_get (model, iter, 0, &info, -1);

    g_object_set (cell, "pixbuf", info->icon, NULL);
}

void
render_text (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, GtkWidget * treeview)
{
    ControlInfo *info;
    gboolean insensitive;

    gtk_tree_model_get (model, iter, 0, &info, -1);

    insensitive = !(info->can_be_added);

    g_object_set (cell, "text", info->caption, "foreground-set", insensitive, 
	    	  NULL);
}

static ControlList *
add_control_list (GtkBox * box)
{
    GtkWidget *scroll, *tv;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreePath *path;
    ControlList *list;
    GSList *li;
    GdkColor color;

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_container_set_border_width (GTK_CONTAINER (scroll), 6);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (box, scroll, TRUE, TRUE, 0);

    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    tv = gtk_tree_view_new_with_model (model);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), tv);

    g_signal_connect (tv, "destroy-event", G_CALLBACK (treeview_destroyed),
		      NULL);

    g_object_unref (G_OBJECT (store));

    /* create the view */
    col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (col, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func (col, cell, 
	    			             (GtkTreeCellDataFunc) render_icon,
					     NULL, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (col, cell, 
	    			             (GtkTreeCellDataFunc) render_text,
					     tv, NULL);

    color = tv->style->fg[GTK_STATE_INSENSITIVE];
    g_object_set (cell, "foreground-gdk", &color, NULL);
    
    /* fill model */
    list = g_new0 (ControlList, 1);

    list->infolist = get_control_info_list ();
    list->current = list->infolist->data;

    for (li = list->infolist; li != NULL; li = li->next)
    {
	GtkTreeIter iter;
	ControlInfo *info = li->data;

	gtk_list_store_append (store, &iter);

	gtk_list_store_set (store, &iter, 0, info, -1);
    }

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed), list);

    gtk_widget_set_size_request (tv, 300, 300);

    return list;
}


