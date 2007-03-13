/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright © 2006 Benedikt Meurer <benny@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
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

#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include <libxfce4panel/xfce-itembar.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-item-iface.h>

#include "frap-icon-entry.h"
#include "panel-properties.h"
#include "panel-private.h"
#include "panel-item-manager.h"
#include "panel-dnd.h"
#include "panel-dialogs.h"

#define BORDER  8

typedef struct _PanelItemsDialog PanelItemsDialog;
typedef struct _PanelManagerDialog PanelManagerDialog;

struct _PanelItemsDialog
{
    GtkWidget *dlg;

    GPtrArray *panels;
    Panel *panel;
    int current;

    GtkWidget *active;
    
    GPtrArray *items;
    GtkWidget *search_entry;
    GtkWidget *tree;
    GtkWidget *items_box;

    int panel_destroy_id;
};

struct _PanelManagerDialog
{
    GtkWidget *dlg;

    GPtrArray *panels;
    Panel *panel;
    int current;
    
    GtkTooltips *tips;

    gboolean updating;

    /* add/remove/rename panel */
    GtkWidget *panel_selector;
    GtkWidget *add_panel;
    GtkWidget *rm_panel;

    /* appearance */
    GtkWidget *size;
    GtkWidget *transparency;
    GtkWidget *activetrans;

    /* monitors */
    GPtrArray *monitors;

    /* position */
    GtkWidget *fixed;
    GtkWidget *floating;
    
    GtkWidget *fixed_box;
    GtkWidget *screen_position[12];
    GtkWidget *fullwidth;
    int n_width_items;
    GtkWidget *autohide;

    GtkWidget *floating_box;
    GtkWidget *orientation;
    GtkWidget *handle_style;
};


static GtkWidget *panel_dialog_widget = NULL;
static GtkWidget *items_dialog_widget = NULL;


/* 
 * Common Code 
 * ===========
 */

static void
present_dialog (GtkWidget *dialog, GPtrArray *panels)
{
    int n = panel_app_get_current_panel ();
    GdkScreen *screen = 
        gtk_widget_get_screen (g_ptr_array_index (panels, n));

    if (screen != gtk_widget_get_screen (dialog))
        gtk_window_set_screen (GTK_WINDOW (dialog), screen);

    gtk_window_present (GTK_WINDOW (dialog));
}

/* 
 * Add Items Dialog 
 * ================
 */

static gboolean
item_configure_timeout (XfcePanelItem *item)
{
    xfce_panel_item_configure (item);

    return FALSE;
}


static gboolean
add_selected_item (PanelItemsDialog *pid)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;
    GtkWidget *item = NULL;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pid->tree));
    
    if (!sel)
        return FALSE;

    if (!gtk_tree_selection_get_selected (sel, &model, &iter))
        return FALSE;

    gtk_tree_model_get (model, &iter, 0, &info, -1);

    if (!xfce_panel_item_manager_is_available (info->name))
        return FALSE;
   
    if (pid->active)
    {
        PanelPrivate *priv = PANEL_GET_PRIVATE (pid->panel);
        int n;

        n = xfce_itembar_get_item_index (XFCE_ITEMBAR (priv->itembar),
                                         pid->active);

        item = panel_insert_item (pid->panel, info->name, n + 1);
    }
    else
    {
        item = panel_add_item (pid->panel, info->name);
    }

    if (item)
        g_idle_add ((GSourceFunc)item_configure_timeout, item);
    else
        xfce_err (_("Could not open \"%s\" module"), info->name);
    
    return TRUE;
}

static gboolean
treeview_dblclick (GtkWidget * tv, GdkEventButton * evt, 
                   PanelItemsDialog *pid)
{
    if (evt->button == 1 && evt->type == GDK_2BUTTON_PRESS)
    {
	return add_selected_item (pid);
    }

    return FALSE;
}

static void
cursor_changed (GtkTreeView * tv, PanelItemsDialog *pid)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;

    if (!(sel = gtk_tree_view_get_selection (tv)))
        return;

    if (!gtk_tree_selection_get_selected (sel, &model, &iter))
        return;

    gtk_tree_model_get (model, &iter, 0, &info, -1);
}

static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (tv))));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

static void
render_icon (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
    XfcePanelItemInfo *info;

    gtk_tree_model_get (model, iter, 0, &info, -1);

    if (info)
    {
        g_object_set (cell, "pixbuf", info->icon, NULL);
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
    XfcePanelItemInfo *info;

    gtk_tree_model_get (model, iter, 0, &info, -1);

    if (info)
    {
        gboolean insensitive;
        char text[512];

        insensitive = !xfce_panel_item_manager_is_available (info->name);
        
        if (info->comment)
        {
            g_snprintf (text, 512, "<b>%s</b>\n%s", info->display_name, 
                                    info->comment);
        }
        else
        {
            g_snprintf (text, 512, "<b>%s</b>", info->display_name);
        }

        g_object_set (cell, "markup", text, 
                      "foreground-set", insensitive, NULL);
    }
    else
    {
        g_object_set (cell, "markup", "", "foreground-set", TRUE, NULL);
    }
}

static void
treeview_data_received (GtkWidget *widget, GdkDragContext *context, 
                        gint x, gint y, GtkSelectionData *data, 
                        guint info, guint time, gpointer user_data)
{
    gboolean handled = FALSE;

    DBG (" + drag data received: %d", info);
    
    if (data->length && info == TARGET_PLUGIN_WIDGET)
        handled = TRUE;
     
    gtk_drag_finish (context, handled, handled, time);
}

static gboolean
treeview_drag_drop (GtkWidget *widget, GdkDragContext *context, 
                    gint x, gint y, guint time, gpointer user_data)
{
    GdkAtom atom = gtk_drag_dest_find_target (widget, context, NULL);

    if (atom != GDK_NONE)
    {
        gtk_drag_get_data (widget, context, atom, time);
        return TRUE;
    }

    return FALSE;
}

static void
treeview_data_get (GtkWidget *widget, GdkDragContext *drag_context, 
                   GtkSelectionData *data, guint info, 
                   guint time, gpointer user_data)
{
    DBG (" + drag data get: %d", info);
    
    if (info == TARGET_PLUGIN_NAME)
    {
        GtkTreeSelection *sel;
        GtkTreeModel *model;
        GtkTreeIter iter;
        XfcePanelItemInfo *info;

        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

        if (!sel)
        {
            DBG ("No selection!");
            return;
        }
        
        if (!gtk_tree_selection_get_selected (sel, &model, &iter))
            return;

        gtk_tree_model_get (model, &iter, 0, &info, -1);

        if (!xfce_panel_item_manager_is_available (info->name))
            return;
       
        DBG (" + set data: %s", info->name);
        gtk_selection_data_set (data, data->target, 8, 
                                (guchar *)info->name, strlen (info->name));
    }
}

static gboolean
item_visible_func (GtkTreeModel *model,
                   GtkTreeIter  *iter,
                   gpointer      user_data)
{
    XfcePanelItemInfo *info;
    const gchar       *text;
    GtkWidget         *entry = GTK_WIDGET (user_data);
    gboolean           visible;
    gchar             *text_casefolded;
    gchar             *info_casefolded;
    gchar             *normalized;

    text = gtk_entry_get_text (GTK_ENTRY (entry));
    if (G_UNLIKELY (*text == '\0'))
        return TRUE;

    gtk_tree_model_get (model, iter, 0, &info, -1);
    if (G_UNLIKELY (info == NULL))
        return TRUE;

    normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
    text_casefolded = g_utf8_casefold (normalized, -1);
    g_free (normalized);

    normalized = g_utf8_normalize (info->display_name, -1, G_NORMALIZE_ALL);
    info_casefolded = g_utf8_casefold (normalized, -1);
    g_free (normalized);

    visible = (strstr (info_casefolded, text_casefolded) != NULL);

    g_free (info_casefolded);

    if (!visible && info->comment)
    {
        normalized = g_utf8_normalize (info->comment, -1, G_NORMALIZE_ALL);
        info_casefolded = g_utf8_casefold (normalized, -1);
        g_free (normalized);

        visible = (strstr (info_casefolded, text_casefolded) != NULL);

        g_free (info_casefolded);
    }

    g_free (text_casefolded);

    return visible;
}

static void
add_item_treeview (PanelItemsDialog *pid)
{
    GtkWidget *tv, *scroll;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *filter;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    int i;
    GdkColor *color;

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (pid->items_box), scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    filter = gtk_tree_model_filter_new (model, NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), item_visible_func, pid->search_entry, NULL);
    g_signal_connect_swapped (G_OBJECT (pid->search_entry), "changed", G_CALLBACK (gtk_tree_model_filter_refilter), filter);

    pid->tree = tv = gtk_tree_view_new_with_model (filter);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

    g_object_unref (G_OBJECT (filter));
    g_object_unref (G_OBJECT (store));

    /* dnd */
    panel_dnd_set_name_source (tv);

    panel_dnd_set_widget_delete_dest (tv);

    g_signal_connect (tv, "drag-data-get", G_CALLBACK (treeview_data_get), 
                      pid);

    g_signal_connect (tv, "drag-data-received", 
                      G_CALLBACK (treeview_data_received), pid);
    
    g_signal_connect (tv, "drag-drop", 
                      G_CALLBACK (treeview_drag_drop), pid);
    
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

    color = &(tv->style->fg[GTK_STATE_INSENSITIVE]);
    g_object_set (cell, "foreground-gdk", color, NULL);
    
    /* fill model */
    for (i = 0; i < pid->items->len; ++i)
    {
        if (i == 5)
        {
            GtkRequisition req;

            gtk_widget_size_request (tv, &req);
            gtk_widget_set_size_request (tv, -1, req.height);

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_ALWAYS);
        }    

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, 
                            g_ptr_array_index (pid->items, i), -1);
    }

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed),
		      pid);

    g_signal_connect (tv, "button-press-event",
		      G_CALLBACK (treeview_dblclick), pid);

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

static void
item_dialog_opened (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_block_autohide (panel);

    xfce_itembar_raise_event_window (XFCE_ITEMBAR (priv->itembar));
    
    panel_dnd_set_dest (priv->itembar);
    panel_dnd_set_widget_source (priv->itembar);

    panel_set_items_sensitive (panel, FALSE);

    priv->edit_mode = TRUE;
}

static void
item_dialog_closed (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_unblock_autohide (panel);
    
    xfce_itembar_lower_event_window (XFCE_ITEMBAR (priv->itembar));

    panel_set_items_sensitive (panel, TRUE);
    
    panel_dnd_unset_dest (priv->itembar);
    panel_dnd_unset_source (priv->itembar);

    priv->edit_mode = FALSE;
}

static void
item_dialog_response (GtkWidget *dlg, int response, PanelItemsDialog *pid)
{
    if (response != GTK_RESPONSE_HELP)
    {
        if (response == GTK_RESPONSE_OK)
        {
            add_selected_item (pid);
        }

        items_dialog_widget = NULL;
        g_ptr_array_foreach (pid->panels, (GFunc)item_dialog_closed, NULL);

        xfce_panel_item_manager_free_item_info_list (pid->items);

        gtk_widget_destroy (dlg);
        
        g_signal_handler_disconnect (pid->panel, pid->panel_destroy_id);
        panel_slice_free (PanelItemsDialog, pid);

        panel_app_save ();
    }
    else
    {
        xfce_exec_on_screen (gtk_widget_get_screen (dlg), 
                             "xfhelp4 panel.html", FALSE, FALSE, NULL);
    }
}

static void
items_dialog_panel_destroyed (PanelItemsDialog *pid)
{
    gtk_dialog_response (GTK_DIALOG (pid->dlg), GTK_RESPONSE_CANCEL);
}

void
add_items_dialog (GPtrArray *panels, GtkWidget *active_item)
{
    PanelItemsDialog *pid;
    Panel *panel;
    GtkWidget *dlg, *vbox, *img, *hbox, *label;
    char *markup;
    
    if (items_dialog_widget)
    {
        present_dialog (items_dialog_widget, panels);
        return;
    }
    
    pid = panel_slice_new0 (PanelItemsDialog);

    /* panels */
    pid->panels = panels;
    pid->current = panel_app_get_current_panel();
    panel = pid->panel = 
        g_ptr_array_index (panels, pid->current);
    pid->active = active_item;
    
    /* available items */
    pid->items = xfce_panel_item_manager_get_item_info_list ();
    
    /* main dialog widget */
    items_dialog_widget = pid->dlg = dlg = 
        xfce_titled_dialog_new_with_buttons (_("Add Items to the Panel"), NULL,
                                             GTK_DIALOG_NO_SEPARATOR,
                                             GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                             GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_ADD, GTK_RESPONSE_OK,
                                             NULL);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-panel");
    
    g_signal_connect (dlg, "response", G_CALLBACK (item_dialog_response), pid);
    pid->panel_destroy_id = 
        g_signal_connect_swapped (panel, "destroy", 
                                  G_CALLBACK (items_dialog_panel_destroyed), 
                                  pid);

    pid->items_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER - 2);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);

    /* info */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_misc_set_alignment (GTK_MISC (img), 0.0f, 0.5f);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);

    label = gtk_label_new (_("Drag items from the list to a panel or remove "
                             "them by dragging them back to the list."));
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    /* treeview */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 1.0f);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

    markup = g_strdup_printf ("<b>%s</b>", _("Available Items"));
    gtk_label_set_markup (GTK_LABEL (label), markup);
    g_free (markup);

    /* the list filter entry (FIXME: Add tooltip? Jasper?) */
    pid->search_entry = frap_icon_entry_new ();
    frap_icon_entry_set_stock_id (FRAP_ICON_ENTRY (pid->search_entry), GTK_STOCK_FIND);
    gtk_widget_show (pid->search_entry);
    gtk_box_pack_end (GTK_BOX (hbox), pid->search_entry, FALSE, FALSE, 0);
    
    add_item_treeview (pid);

     /* make panels insensitive and set up dnd */
    g_ptr_array_foreach (panels, (GFunc)item_dialog_opened, NULL);

    gtk_window_stick(GTK_WINDOW (dlg));

    gtk_widget_realize (dlg);
    gdk_x11_window_set_user_time (GTK_WIDGET (dlg)->window,
        gdk_x11_get_server_time (GTK_WIDGET (dlg)->window));
    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dlg));
    gtk_widget_show (dlg);

    panel_app_register_dialog (dlg);

    gtk_window_present (GTK_WINDOW (dlg));
}

/* 
 * Manage Panels Dialog 
 * ====================
 */

static gboolean
can_span_monitors (Panel *panel)
{
    return ( (panel_app_monitors_equal_height () && 
             panel_is_horizontal (panel))
             || (panel_app_monitors_equal_width () && 
                 !panel_is_horizontal (panel)) );
}

/* Update widgets */

static void
update_widgets (PanelManagerDialog *pmd)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (pmd->panel);
    int i;

    pmd->updating = TRUE;
    
    /* monitor */
    if (pmd->monitors)
    {
        for (i = 0; i < pmd->monitors->len; ++i)
        {
            GtkToggleButton *tb = g_ptr_array_index (pmd->monitors, i);
            
            gtk_toggle_button_set_active (tb, i == priv->monitor);
        }
    }
    
    /* appearance */
    gtk_range_set_value (GTK_RANGE (pmd->size), priv->size);

    if (pmd->transparency)
    {
        gtk_range_set_value (GTK_RANGE (pmd->transparency), 
                             priv->transparency);
    
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->activetrans),
                                      !priv->activetrans);
    }

    /* behavior */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->autohide),
                                  priv->autohide);
    
    /* position */
    if (!xfce_screen_position_is_floating (priv->screen_position))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->fixed), TRUE);
        
        gtk_widget_hide (pmd->floating_box);
        gtk_widget_show (pmd->fixed_box);
        
        for (i = 0; i < 12; ++i)
        {
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (pmd->screen_position[i]),
                    (int)priv->screen_position == i+1);
        }

        gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->fullwidth), 
                                  priv->full_width);
    }
    else
    {
        XfceHandleStyle style;
        
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->floating), TRUE);
        
        gtk_widget_hide (pmd->fixed_box);
        gtk_widget_show (pmd->floating_box);

        gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->orientation), 
                                  panel_is_horizontal (pmd->panel) ? 0 : 1);

        style = xfce_panel_window_get_handle_style (
                        XFCE_PANEL_WINDOW (pmd->panel));
        if (style == XFCE_HANDLE_STYLE_NONE)
            style = XFCE_HANDLE_STYLE_BOTH;
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->handle_style), 
                                  style - 1);
    }

    pmd->updating = FALSE;
}


/* position */
static void
type_changed (GtkToggleButton *tb, PanelManagerDialog *pmd)
{
    PanelPrivate *priv;
    int active;
    
    if (pmd->updating)
        return;

    if (!gtk_toggle_button_get_active (tb))
        return;
    
    priv = PANEL_GET_PRIVATE (pmd->panel);

    /* 0 for fixed, 1 for floating */
    active = GTK_WIDGET (tb) == pmd->fixed ? 0 : 1;
    
    if ((active == 0) == 
        (priv->screen_position > XFCE_SCREEN_POSITION_NONE &&
         priv->screen_position < XFCE_SCREEN_POSITION_FLOATING_H))
    {
        return;
    }

    if (active == 1)
    {
        if (xfce_screen_position_is_horizontal (priv->screen_position))
        {
            panel_set_screen_position (pmd->panel,
                                       XFCE_SCREEN_POSITION_FLOATING_H);
        }
        else
        {
            panel_set_screen_position (pmd->panel, 
                                       XFCE_SCREEN_POSITION_FLOATING_V);
        }

        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (pmd->panel),
                                            XFCE_HANDLE_STYLE_BOTH);
    }
    else
    {
        if (xfce_screen_position_is_horizontal (priv->screen_position))
        {
            panel_set_screen_position (pmd->panel,
                                       XFCE_SCREEN_POSITION_S);
        }
        else
        {
            panel_set_screen_position (pmd->panel, 
                                       XFCE_SCREEN_POSITION_E);
        }

        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (pmd->panel),
                                            XFCE_HANDLE_STYLE_NONE);
    }

    gtk_widget_queue_draw (GTK_WIDGET (pmd->panel));

    update_widgets (pmd);
}

static gboolean
screen_position_pressed (GtkToggleButton *tb, GdkEvent *ev, 
                         PanelManagerDialog *pmd)
{
    if (ev->type == GDK_KEY_PRESS && 
             ((GdkEventKey *)ev)->keyval == GDK_Tab)
    {
        return FALSE;
    }
    
    if (!gtk_toggle_button_get_active (tb))
    {
        if (ev->type == GDK_BUTTON_PRESS ||
            (ev->type == GDK_KEY_PRESS && 
                (((GdkEventKey *)ev)->keyval == GDK_space ||
                        ((GdkEventKey *)ev)->keyval == GDK_Return)))
        {
            int i, full_width;

            for (i = 0; i < 12; ++i)
            {
                GtkToggleButton *button = 
                    GTK_TOGGLE_BUTTON (pmd->screen_position[i]);

                if (button == tb)
                {
                    gtk_toggle_button_set_active (button, TRUE);
                    panel_set_screen_position (pmd->panel, i + 1);
                    
                    /* fix up full width setting */
                    full_width = gtk_combo_box_get_active (
                                        GTK_COMBO_BOX (pmd->fullwidth));
                    
                    for ( ; pmd->n_width_items > 0; pmd->n_width_items--)
                    {
                        gtk_combo_box_remove_text (
                                GTK_COMBO_BOX (pmd->fullwidth), 
                                pmd->n_width_items - 1);
                    }
                    
                    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->fullwidth),
                                               _("Normal Width"));
                    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->fullwidth),
                                               _("Full Width")); 
                    pmd->n_width_items = 2;
                    if (can_span_monitors(pmd->panel))
                    {
                        gtk_combo_box_append_text (
                                GTK_COMBO_BOX (pmd->fullwidth), 
                                _("Span Monitors"));
                        pmd->n_width_items = 3;
                    }

                    full_width = MIN(pmd->n_width_items - 1, full_width);
                    panel_set_full_width (pmd->panel, full_width);
                    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->fullwidth),
                                              full_width);
                }                                          
                else
                {
                    gtk_toggle_button_set_active (button, FALSE);
                }
            }
        }
    }

    return TRUE;
}

static void
fullwidth_changed (GtkComboBox *box, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_full_width (pmd->panel, gtk_combo_box_get_active (box));
}

static void
autohide_changed (GtkToggleButton *tb, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_autohide (pmd->panel, gtk_toggle_button_get_active (tb));
}

static void
orientation_changed (GtkComboBox *box, PanelManagerDialog *pmd)
{
    XfceScreenPosition position;
    int n;
    gboolean tmp_updating;
    
    position = gtk_combo_box_get_active (box) == 0 ? 
                                   XFCE_SCREEN_POSITION_FLOATING_H :
                                   XFCE_SCREEN_POSITION_FLOATING_V;
    
    tmp_updating = pmd->updating;
    
    pmd->updating = TRUE;
    n = gtk_combo_box_get_active (GTK_COMBO_BOX (pmd->handle_style));

    gtk_combo_box_remove_text (GTK_COMBO_BOX (pmd->handle_style), 2);
    gtk_combo_box_remove_text (GTK_COMBO_BOX (pmd->handle_style), 1);

    if (position == XFCE_SCREEN_POSITION_FLOATING_H)
    {
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Left"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Right"));
    }
    else
    {
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Top"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Bottom"));
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->handle_style), n);
    pmd->updating = tmp_updating;

    if (!pmd->updating)
        panel_set_screen_position (pmd->panel, position);
}

static void
handle_style_changed (GtkComboBox *box, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (pmd->panel),
                                        gtk_combo_box_get_active (box) + 1);
}

static void
add_position_options (GtkBox *box, PanelManagerDialog *pmd)
{
    GtkWidget *frame, *vbox, *vbox2, *hbox, *table, *align, *label, *sep;
    GtkSizeGroup *sg;
    int i;
    
    /* position */
    frame = xfce_create_framebox (_("Position"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, 0);
    
    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (align), vbox2);

    /* type */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);

    pmd->fixed = 
        gtk_radio_button_new_with_label (NULL, _("Fixed Position"));
    gtk_widget_show (pmd->fixed);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->fixed, FALSE, FALSE, 0);
    
    pmd->floating = 
        gtk_radio_button_new_with_label_from_widget (
                GTK_RADIO_BUTTON (pmd->fixed), _("Freely Moveable"));
    gtk_widget_show (pmd->floating);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->floating, FALSE, FALSE, 0);

    g_signal_connect (pmd->fixed, "toggled", G_CALLBACK (type_changed), 
                      pmd);
    
    g_signal_connect (pmd->floating, "toggled", G_CALLBACK (type_changed), 
                      pmd);
    
    sep = gtk_hseparator_new ();
    gtk_widget_show (sep);
    gtk_box_pack_start (GTK_BOX (vbox2), sep, FALSE, FALSE, 0);
    
    /* fixed */
    pmd->fixed_box = hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);

    /* fixed: position */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    table = gtk_table_new (5, 5, FALSE);
    gtk_widget_show (table);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);
    
    for (i = 0; i < 12; ++i)
    {
        pmd->screen_position[i] = gtk_toggle_button_new ();
        gtk_widget_show (pmd->screen_position[i]);

        if (i <= 2 || i >= 9)
            gtk_widget_set_size_request (pmd->screen_position[i], 30, 15);
        else
            gtk_widget_set_size_request (pmd->screen_position[i], 15, 25);

        g_signal_connect (pmd->screen_position[i], "button-press-event", 
                          G_CALLBACK (screen_position_pressed), pmd);
        
        g_signal_connect (pmd->screen_position[i], "key-press-event", 
                          G_CALLBACK (screen_position_pressed), pmd);
    }

    /* fixed:postion:top */
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[0],
                               1, 2, 0, 1);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[1],
                               2, 3, 0, 1);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[2],
                               3, 4, 0, 1);
    
    /* fixed:postion:left */
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[3],
                               0, 1, 1, 2);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[4],
                               0, 1, 2, 3);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[5],
                               0, 1, 3, 4);
    
    /* fixed:postion:right */
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[6],
                               4, 5, 1, 2);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[7],
                               4, 5, 2, 3);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[8],
                               4, 5, 3, 4);
    
    /* fixed:postion:bottom */
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[9],
                               1, 2, 4, 5);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[10],
                               2, 3, 4, 5);
    
    gtk_table_attach_defaults (GTK_TABLE (table), pmd->screen_position[11],
                               3, 4, 4, 5);
    
    /* fixed:full width */
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (pmd->fixed_box), vbox, TRUE, TRUE, 0);

    pmd->fullwidth = gtk_combo_box_new_text ();
    gtk_widget_show (pmd->fullwidth);
    gtk_box_pack_start (GTK_BOX (vbox), pmd->fullwidth, FALSE, FALSE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->fullwidth),
                               _("Normal Width"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->fullwidth),
                               _("Full Width")); 
    pmd->n_width_items = 2;
    if (can_span_monitors (pmd->panel))
    {
        pmd->n_width_items = 3;
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->fullwidth), 
                                   _("Span Monitors"));
    }

    g_signal_connect (pmd->fullwidth, "changed", 
                      G_CALLBACK (fullwidth_changed), pmd);
    
    /* fixed:autohide */
    pmd->autohide = 
        gtk_check_button_new_with_mnemonic (_("Auto_hide"));
    gtk_widget_show (pmd->autohide);
    gtk_box_pack_start (GTK_BOX (vbox), pmd->autohide, FALSE, FALSE, 0);
        
    g_signal_connect (pmd->autohide, "toggled", 
                      G_CALLBACK (autohide_changed), pmd);
    
    /* floating */
    pmd->floating_box = vbox = gtk_vbox_new (FALSE, BORDER);
    /* don't show by default */
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);    
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Orientation:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);
    
    pmd->orientation = gtk_combo_box_new_text ();
    gtk_widget_show (pmd->orientation);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->orientation, TRUE, TRUE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->orientation), 
                               _("Horizontal"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->orientation), 
                               _("Vertical"));
    
    g_signal_connect (pmd->orientation, "changed", 
                      G_CALLBACK (orientation_changed), pmd);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Handle:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);
    
    pmd->handle_style = gtk_combo_box_new_text ();
    gtk_widget_show (pmd->handle_style);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->handle_style, TRUE, TRUE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                               _("At both sides"));
    if (panel_is_horizontal (pmd->panel))
    {
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Left"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Right"));
    }
    else
    {
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Top"));
        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->handle_style), 
                                   _("Bottom"));
    }

    g_signal_connect (pmd->handle_style, "changed", 
                      G_CALLBACK (handle_style_changed), pmd);
    
    g_object_unref (G_OBJECT (sg));
}

/* monitors */
static gboolean
monitor_pressed (GtkToggleButton *tb, GdkEvent *ev, PanelManagerDialog *pmd)
{
    if (ev->type == GDK_KEY_PRESS && 
             ((GdkEventKey *)ev)->keyval == GDK_Tab)
    {
        return FALSE;
    }
    
    if (!gtk_toggle_button_get_active (tb))
    {
        if (ev->type == GDK_BUTTON_PRESS ||
            (ev->type == GDK_KEY_PRESS && 
                (((GdkEventKey *)ev)->keyval == GDK_space ||
                        ((GdkEventKey *)ev)->keyval == GDK_Return)))
        {
            int i;

            for (i = 0; i < pmd->monitors->len; ++i)
            {
                GtkToggleButton *mon = g_ptr_array_index (pmd->monitors, i);

                if (mon == tb)
                {
                    gtk_toggle_button_set_active (mon, TRUE);
                    panel_set_monitor (pmd->panel, i);
                }
                else
                {
                    gtk_toggle_button_set_active (mon, FALSE);
                }
            }
        }
    }

    return TRUE;
}

static void
add_monitor_selector (GtkBox *box, PanelManagerDialog *pmd)
{
    int n_monitors, i;

    n_monitors = panel_app_get_n_monitors ();
    
    if (n_monitors > 1)
    {
        GtkWidget *frame, *align, *hbox;
        GtkWidget *scroll = NULL;
        
        frame = xfce_create_framebox (_("Select Monitor"), &align);
        gtk_widget_show (frame);
        gtk_box_pack_start (box, frame, FALSE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, BORDER);
        gtk_widget_show (hbox);
        /* don't add it to align yet */

        pmd->monitors = g_ptr_array_sized_new (n_monitors);

        for (i = 0; i < n_monitors; ++i)
        {
            GtkWidget *ebox, *ebox2, *b, *label;
            GtkStyle *style;
            char markup[10];
            
            /* use a scroll window if more than 4 monitors */
            if (i == 5)
            {
                GtkRequisition req;

                scroll = gtk_scrolled_window_new (NULL, NULL);
                gtk_widget_show (scroll);
                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                                GTK_POLICY_NEVER, 
                                                GTK_POLICY_NEVER);
                gtk_scrolled_window_set_shadow_type (
                        GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);
        
                gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

                gtk_scrolled_window_add_with_viewport (
                        GTK_SCROLLED_WINDOW (scroll), hbox);
                
                gtk_widget_size_request (scroll, &req);

                gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                                GTK_POLICY_ALWAYS, 
                                                GTK_POLICY_NEVER);

                gtk_widget_set_size_request (scroll, req.width, -1);
            }

            g_snprintf (markup, 10, "<b>%d</b>", i + 1);
            
            ebox = gtk_event_box_new ();
            style = gtk_widget_get_style (ebox);
            gtk_widget_modify_bg (ebox, GTK_STATE_NORMAL,
                                  &(style->bg[GTK_STATE_SELECTED]));
            gtk_widget_show (ebox);
            gtk_box_pack_start (GTK_BOX (hbox), ebox, FALSE, FALSE, 0);
            
            ebox2 = gtk_event_box_new ();
            gtk_container_set_border_width (GTK_CONTAINER (ebox2), 3);
            gtk_widget_show (ebox2);
            gtk_container_add (GTK_CONTAINER (ebox), ebox2);
            
            b = gtk_toggle_button_new();
            gtk_button_set_relief (GTK_BUTTON (b), GTK_RELIEF_NONE);
            gtk_widget_set_size_request (b, 40, 30);
            gtk_widget_show (b);
            gtk_container_add (GTK_CONTAINER (ebox2), b);

            label = gtk_label_new (NULL);
            gtk_label_set_markup (GTK_LABEL (label), markup);
            gtk_widget_show (label);
            gtk_container_add (GTK_CONTAINER (b), label);

            g_signal_connect (b, "button-press-event", 
                              G_CALLBACK (monitor_pressed), pmd);
            
            g_signal_connect (b, "key-press-event", 
                              G_CALLBACK (monitor_pressed), pmd);
            
            g_ptr_array_add (pmd->monitors, b);
        }

        if (scroll)
            gtk_container_add (GTK_CONTAINER (align), scroll);
        else
            gtk_container_add (GTK_CONTAINER (align), hbox);
    }
}

/* appearance */
static void
size_changed (GtkRange *range, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_size (pmd->panel, (int) gtk_range_get_value (range));
}

static void
transparency_changed (GtkRange *range, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_transparency (pmd->panel, (int) gtk_range_get_value (range));
}

static void
activetrans_toggled (GtkToggleButton *tb, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_activetrans (pmd->panel, !gtk_toggle_button_get_active (tb));
}

static void
add_appearance_options (GtkBox *box, PanelManagerDialog *pmd)
{
    static Atom composite_atom = 0;
    GtkWidget *frame, *table, *label, *align;

    frame = xfce_create_framebox (_("Appearance"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (box, frame, FALSE, FALSE, BORDER);

    table = gtk_table_new (3, 2, FALSE);
    gtk_table_set_row_spacings (GTK_TABLE (table), BORDER);
    gtk_table_set_col_spacings (GTK_TABLE (table), BORDER);
    gtk_widget_show (table);
    gtk_container_add (GTK_CONTAINER (align), table);
    
    /* size */
    label = gtk_label_new (_("Size (pixels):"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                      GTK_FILL, 0, 0, 0);
    
    pmd->size = gtk_hscale_new_with_range (MIN_SIZE, MAX_SIZE, 2);
    gtk_scale_set_value_pos (GTK_SCALE (pmd->size), GTK_POS_LEFT);
    gtk_range_set_update_policy (GTK_RANGE (pmd->size), GTK_UPDATE_DELAYED);
    gtk_widget_show (pmd->size);
    gtk_table_attach (GTK_TABLE (table), pmd->size, 1, 2, 0, 1,
                      GTK_FILL|GTK_EXPAND, 0, 0, 0);

    g_signal_connect (pmd->size, "value-changed", 
                      G_CALLBACK (size_changed), pmd);

    /* transparency */
    if (G_UNLIKELY (!composite_atom))
    {
        char text[16];
        g_snprintf (text, 16, "_NET_WM_CM_S%d", 
                    GDK_SCREEN_XNUMBER(gdk_screen_get_default()));
        composite_atom = 
            XInternAtom (GDK_DISPLAY (), text, False);
    }

    if (XGetSelectionOwner (GDK_DISPLAY (), composite_atom))
    {
        label = gtk_label_new (_("Transparency (%):"));
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
        gtk_widget_show (label);
        gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                          GTK_FILL, 0, 0, 0);
        
        pmd->transparency = gtk_hscale_new_with_range (0, 100, 5);
        gtk_scale_set_value_pos (GTK_SCALE (pmd->transparency), GTK_POS_LEFT);
        gtk_range_set_update_policy (GTK_RANGE (pmd->transparency),
                                     GTK_UPDATE_DELAYED);
        gtk_widget_show (pmd->transparency);
        gtk_table_attach (GTK_TABLE (table), pmd->transparency, 1, 2, 1, 2,
                          GTK_FILL|GTK_EXPAND, 0, 0, 0);

        g_signal_connect (pmd->transparency, "value-changed", 
                          G_CALLBACK (transparency_changed), pmd);

        align = gtk_alignment_new (0, 0, 0, 0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
        gtk_widget_show (align);
        gtk_table_attach (GTK_TABLE (table), align, 0, 3, 2, 3,
                          GTK_FILL|GTK_EXPAND, 0, 0, 0);
    
        pmd->activetrans = 
            gtk_check_button_new_with_label (_("Make active panel opaque"));
        gtk_widget_show (pmd->activetrans);
        gtk_container_add (GTK_CONTAINER (align), pmd->activetrans);

        g_signal_connect (pmd->activetrans, "toggled", 
                          G_CALLBACK (activetrans_toggled), pmd);
    }
}

/* panel selector: add/remove/rename panel */
static void
panel_selected (GtkComboBox * combo, PanelManagerDialog * pmd)
{
    int n = gtk_combo_box_get_active (combo);

    if (n == pmd->current)
        return;

    pmd->current = n;
    pmd->panel = g_ptr_array_index (pmd->panels, n);

    update_widgets (pmd);
}

static void
add_panel (GtkWidget * w, PanelManagerDialog * pmd)
{
    char name[20];
    int n, x, y;

    n = pmd->panels->len;

    panel_app_add_panel ();

    if (n == pmd->panels->len)
        return;

    panel_block_autohide (PANEL (g_ptr_array_index (pmd->panels, n)));

    g_snprintf (name, 20, _("Panel %d"), pmd->panels->len);

    gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->panel_selector), name);

    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->panel_selector), n);

    gtk_window_get_position (GTK_WINDOW (pmd->dlg), &x, &y);

    x += (pmd->dlg->allocation.width - 
            GTK_WIDGET (pmd->panel)->allocation.width) / 2;
    y += pmd->dlg->allocation.height + BORDER;

    gtk_window_move (GTK_WINDOW (pmd->panel), x, y);
    gtk_widget_queue_resize (GTK_WIDGET (pmd->panel));
}

static void
remove_panel (GtkWidget * w, PanelManagerDialog * pmd)
{
    int n = pmd->panels->len;
    int i;

    panel_app_remove_panel (GTK_WIDGET (pmd->panel));

    if (pmd->panels->len == n)
        return;

    pmd->panel = g_ptr_array_index (pmd->panels, 0);

    for (i = pmd->panels->len; i >= 0; --i)
        gtk_combo_box_remove_text (GTK_COMBO_BOX (pmd->panel_selector), i);

    for (i = 0; i < pmd->panels->len; ++i)
    {
        char name[20];

        g_snprintf (name, 20, _("Panel %d"), i + 1);

        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->panel_selector), name);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->panel_selector), 0);
}

static GtkWidget *
create_panel_selector (PanelManagerDialog *pmd)
{
    GtkWidget *hbox, *img;
    int i;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);

    pmd->panel_selector = gtk_combo_box_new_text ();
    gtk_widget_show (pmd->panel_selector);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->panel_selector, TRUE, TRUE, 0);

    for (i = 0; i < pmd->panels->len; ++i)
    {
        char name[20];

        g_snprintf (name, 20, _("Panel %d"), i + 1);

        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->panel_selector), name);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->panel_selector),
                              pmd->current);

    g_signal_connect (pmd->panel_selector, "changed",
                      G_CALLBACK (panel_selected), pmd);
    
    pmd->rm_panel = gtk_button_new ();
    gtk_widget_show (pmd->rm_panel);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->rm_panel, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->rm_panel), img);

    gtk_tooltips_set_tip (pmd->tips, pmd->rm_panel, _("Remove Panel"), NULL);

    g_signal_connect (pmd->rm_panel, "clicked",
                      G_CALLBACK (remove_panel), pmd);
    
    pmd->add_panel = gtk_button_new ();
    gtk_widget_show (pmd->add_panel);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->add_panel, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->add_panel), img);

    gtk_tooltips_set_tip (pmd->tips, pmd->add_panel, _("New Panel"), NULL);

    g_signal_connect (pmd->add_panel, "clicked", G_CALLBACK (add_panel), pmd);

    return hbox;
}

/* main dialog */
static void
panel_dialog_response (GtkWidget *dlg, int response, PanelManagerDialog *pmd)
{
    if (response != GTK_RESPONSE_HELP)
    {
        panel_dialog_widget = NULL;
        
        if (pmd->monitors)
            g_ptr_array_free (pmd->monitors, TRUE);
        
        g_ptr_array_foreach (pmd->panels, (GFunc)panel_unblock_autohide, NULL);
        
        gtk_widget_destroy (dlg);
        
        g_object_unref (G_OBJECT (pmd->tips));
        panel_slice_free (PanelManagerDialog, pmd);

        panel_app_save ();
    }
    else
    {
        xfce_exec_on_screen (gtk_widget_get_screen (dlg), 
                             "xfhelp4 panel.html", FALSE, FALSE, NULL);
    }
}

void
panel_manager_dialog (GPtrArray *panels)
{
    PanelManagerDialog *pmd;
    GtkWidget *vbox, *sel;
    Panel *panel;

    if (panel_dialog_widget)
    {
        present_dialog (panel_dialog_widget, panels);
        return;
    }
    
    pmd = panel_slice_new0 (PanelManagerDialog);

    /* panels */
    pmd->panels = panels;
    pmd->current = panel_app_get_current_panel();
    panel = pmd->panel = 
        g_ptr_array_index (panels, pmd->current);

    /* main dialog widget */
    panel_dialog_widget = pmd->dlg = 
        xfce_titled_dialog_new_with_buttons (_("Panel Manager"), NULL, 
                                             GTK_DIALOG_NO_SEPARATOR,
                                             GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                             GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                             NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (pmd->dlg), GTK_RESPONSE_OK);
    gtk_window_set_icon_name (GTK_WINDOW (pmd->dlg), "xfce4-panel");

    pmd->tips = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (pmd->tips));
    gtk_object_sink (GTK_OBJECT (pmd->tips));

    /* main container */
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER - 2);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pmd->dlg)->vbox), vbox, 
                        TRUE, TRUE, 0);

    pmd->updating = TRUE;
    
    /* add/remove/rename panel */
    sel = create_panel_selector (pmd);
    gtk_widget_show (sel);
    gtk_box_pack_start (GTK_BOX (vbox), sel, FALSE, FALSE, 0);

    /* appearance */
    add_appearance_options (GTK_BOX (vbox), pmd);

    /* position */
    add_position_options (GTK_BOX (vbox), pmd);

    /* monitors */
    add_monitor_selector (GTK_BOX (vbox), pmd);

    pmd->updating = FALSE;

    /* fill in values */
    update_widgets (pmd);

    /* setup panels */
    g_ptr_array_foreach (pmd->panels, (GFunc)panel_block_autohide, NULL);

    /* setup and show dialog */
    g_signal_connect (pmd->dlg, "response", 
                      G_CALLBACK (panel_dialog_response), pmd);
    
    gtk_window_stick(GTK_WINDOW (pmd->dlg));
    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (pmd->dlg));
    gtk_widget_realize (pmd->dlg);

    /* window needs to be realized */
    gdk_x11_window_set_user_time (GTK_WIDGET (pmd->dlg)->window,
        gdk_x11_get_server_time (GTK_WIDGET (pmd->dlg)->window));
    gtk_widget_show (pmd->dlg);

    panel_app_register_dialog (pmd->dlg);
}

