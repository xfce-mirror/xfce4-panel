/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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
#include <libxfce4panel/xfce-panel-item-iface.h>

#include "panel-settings.h"
#include "panel-properties.h"
#include "panel-private.h"
#include "panel-item-manager.h"
#include "panel-dnd.h"

#define BORDER  8

typedef struct _PanelSettingsDialog PanelSettingsDialog;

struct _PanelSettingsDialog
{
    GtkWidget *dlg;

    GPtrArray *panels;
    Panel *panel;
    
    GtkWidget *vbox;

    gboolean updating;

    /* Panel Properties */
    GtkWidget *properties_box;

    GtkWidget *other_box;
    GPtrArray *monitors;
    GtkWidget *size;
    GtkWidget *transparency;
    GtkWidget *autohide;

    GtkWidget *position_box;
    GtkWidget *floating;
    
    GtkWidget *fixed_box;
    GtkWidget *screen_position[12];
    GtkWidget *fullwidth;

    GtkWidget *floating_box;
    GtkWidget *orientation;
    GtkWidget *handle_style;

    /* Panel Items */
    GtkWidget *items_box;

    GPtrArray *items;
    GtkWidget *tree;
    GtkWidget *add_item;
};


static GtkWidget *dialog_widget = NULL;
static GtkWidget *dialog_widget_items = NULL;

static void properties_dialog_opened (Panel *panel);

/*
 * Panel Properties
 * ================
 */

/* updating */
static void
update_properties_tab (PanelSettingsDialog *psd)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (psd->panel);
    int i;

    psd->updating = TRUE;
    
    /* monitor */
    if (psd->monitors)
    {
        for (i = 0; i < psd->monitors->len; ++i)
        {
            GtkToggleButton *tb = g_ptr_array_index (psd->monitors, i);
            
            gtk_toggle_button_set_active (tb, i == priv->monitor);
        }
    }
    
    /* appearance */
    gtk_range_set_value (GTK_RANGE (psd->size), priv->size);

    gtk_range_set_value (GTK_RANGE (psd->transparency), priv->transparency);

    /* behavior */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psd->autohide),
                                  priv->autohide);
    
    /* position */
    DBG ("position: %d", priv->screen_position);

    if (!xfce_screen_position_is_floating (priv->screen_position))
    {
        gtk_combo_box_set_active (GTK_COMBO_BOX (psd->floating), 0);
        
        gtk_widget_hide (psd->floating_box);
        gtk_widget_show (psd->fixed_box);
        
        for (i = 0; i < 12; ++i)
        {
            gtk_toggle_button_set_active (
                    GTK_TOGGLE_BUTTON (psd->screen_position[i]),
                    (int)priv->screen_position == i+1);
        }

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psd->fullwidth),
                                      priv->full_width);
    }
    else
    {
        XfceHandleStyle style;
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (psd->floating), 1);
        
        gtk_widget_hide (psd->fixed_box);
        gtk_widget_show (psd->floating_box);

        gtk_combo_box_set_active (GTK_COMBO_BOX (psd->orientation), 
                                  panel_is_horizontal (psd->panel) ? 0 : 1);

        style = xfce_panel_window_get_handle_style (
                        XFCE_PANEL_WINDOW (psd->panel));
        if (style == XFCE_HANDLE_STYLE_NONE)
            style = XFCE_HANDLE_STYLE_BOTH;
        
        gtk_combo_box_set_active (GTK_COMBO_BOX (psd->handle_style), 
                                  style - 1);
    }

    psd->updating = FALSE;
}

/* monitor */
static gboolean
monitor_pressed (GtkToggleButton *tb, GdkEvent *ev, PanelSettingsDialog *psd)
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

            for (i = 0; i < psd->monitors->len; ++i)
            {
                GtkToggleButton *mon = g_ptr_array_index (psd->monitors, i);

                if (mon == tb)
                {
                    gtk_toggle_button_set_active (mon, TRUE);
                    panel_set_monitor (psd->panel, i);
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

/* size and transparency */
static void
size_changed (GtkRange *range, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    panel_set_size (psd->panel, (int) gtk_range_get_value (range));
}

static void
transparency_changed (GtkRange *range, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    panel_set_transparency (psd->panel, (int) gtk_range_get_value (range));
}

/* type and position */
static void
type_changed (GtkComboBox *box, PanelSettingsDialog *psd)
{
    PanelPrivate *priv;
    int active;
    
    if (psd->updating)
        return;

    priv = PANEL_GET_PRIVATE (psd->panel);

    active = gtk_combo_box_get_active (box);
    
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
            panel_set_screen_position (psd->panel,
                                       XFCE_SCREEN_POSITION_FLOATING_H);
        }
        else
        {
            panel_set_screen_position (psd->panel, 
                                       XFCE_SCREEN_POSITION_FLOATING_V);
        }

        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (psd->panel),
                                            XFCE_HANDLE_STYLE_BOTH);
    }
    else
    {
        if (xfce_screen_position_is_horizontal (priv->screen_position))
        {
            panel_set_screen_position (psd->panel,
                                       XFCE_SCREEN_POSITION_S);
        }
        else
        {
            panel_set_screen_position (psd->panel, 
                                       XFCE_SCREEN_POSITION_E);
        }

        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (psd->panel),
                                            XFCE_HANDLE_STYLE_NONE);
    }

    gtk_widget_queue_draw (GTK_WIDGET (psd->panel));

    update_properties_tab (psd);
}

/* fixed position */
static gboolean
screen_position_pressed (GtkToggleButton *tb, GdkEvent *ev, 
                         PanelSettingsDialog *psd)
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

            for (i = 0; i < 12; ++i)
            {
                GtkToggleButton *button = 
                    GTK_TOGGLE_BUTTON (psd->screen_position[i]);

                if (button == tb)
                {
                    gtk_toggle_button_set_active (button, TRUE);
                    panel_set_screen_position (psd->panel, i + 1);
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
fullwidth_changed (GtkToggleButton *tb, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    panel_set_full_width (psd->panel, gtk_toggle_button_get_active (tb));
}

static void
autohide_changed (GtkToggleButton *tb, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    panel_set_autohide (psd->panel, gtk_toggle_button_get_active (tb));
}

/* floating panel */
static void
orientation_changed (GtkComboBox *box, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    panel_set_screen_position (psd->panel, 
                               gtk_combo_box_get_active (box) == 0 ? 
                                   XFCE_SCREEN_POSITION_FLOATING_H :
                                   XFCE_SCREEN_POSITION_FLOATING_V);
}

static void
handle_style_changed (GtkComboBox *box, PanelSettingsDialog *psd)
{
    if (psd->updating)
        return;

    xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (psd->panel),
                                        gtk_combo_box_get_active (box) + 1);
}

/* creation */
static void
add_position_box (PanelSettingsDialog *psd)
{
    GtkWidget *hbox, *vbox, *vbox2, *frame, *table, *align, *label;
    GtkSizeGroup *sg;
    int i;
    
    hbox = psd->properties_box;

    psd->position_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    /* floating? */
    frame = xfce_create_framebox (_("Panel Type"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    psd->floating = gtk_combo_box_new_text ();
    gtk_widget_show (psd->floating);
    gtk_container_add (GTK_CONTAINER (align), psd->floating);
    
    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->floating), 
                               _("Fixed Position"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->floating), 
                               _("Freely Moveable"));

    g_signal_connect (psd->floating, "changed", G_CALLBACK (type_changed), 
                      psd);
    
    /* position */
    frame = xfce_create_framebox (_("Position"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (align), vbox2);

    /* fixed */
    psd->fixed_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    table = gtk_table_new (5, 5, FALSE);
    gtk_widget_show (table);
    gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);
    
    for (i = 0; i < 12; ++i)
    {
        psd->screen_position[i] = gtk_toggle_button_new ();
        gtk_widget_show (psd->screen_position[i]);

        if (i <= 2 || i >= 9)
            gtk_widget_set_size_request (psd->screen_position[i], 30, 15);
        else
            gtk_widget_set_size_request (psd->screen_position[i], 15, 30);

        g_signal_connect (psd->screen_position[i], "button-press-event", 
                          G_CALLBACK (screen_position_pressed), psd);
        
        g_signal_connect (psd->screen_position[i], "key-press-event", 
                          G_CALLBACK (screen_position_pressed), psd);
    }

    /* fixed:postion:top */
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[0],
                               1, 2, 0, 1);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[1],
                               2, 3, 0, 1);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[2],
                               3, 4, 0, 1);
    
    /* fixed:postion:left */
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[3],
                               0, 1, 1, 2);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[4],
                               0, 1, 2, 3);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[5],
                               0, 1, 3, 4);
    
    /* fixed:postion:right */
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[6],
                               4, 5, 1, 2);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[7],
                               4, 5, 2, 3);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[8],
                               4, 5, 3, 4);
    
    /* fixed:postion:bottom */
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[9],
                               1, 2, 4, 5);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[10],
                               2, 3, 4, 5);
    
    gtk_table_attach_defaults (GTK_TABLE (table), psd->screen_position[11],
                               3, 4, 4, 5);
    
    /* fixed:full width */
    psd->fullwidth = 
        gtk_check_button_new_with_mnemonic (_("_Full Width"));
    gtk_widget_show (psd->fullwidth);
    gtk_box_pack_start (GTK_BOX (vbox), psd->fullwidth, FALSE, FALSE, 0);

    g_signal_connect (psd->fullwidth, "toggled", 
                      G_CALLBACK (fullwidth_changed), psd);
    
    /* fixed:autohide */
    psd->autohide = 
        gtk_check_button_new_with_mnemonic (_("Auto_hide"));
    gtk_widget_show (psd->autohide);
    gtk_box_pack_start (GTK_BOX (vbox), psd->autohide, FALSE, FALSE, 0);
        
    g_signal_connect (psd->autohide, "toggled", 
                      G_CALLBACK (autohide_changed), psd);
    
    /* floating */
    psd->floating_box = vbox = gtk_vbox_new (FALSE, BORDER);
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
    
    psd->orientation = gtk_combo_box_new_text ();
    gtk_widget_show (psd->orientation);
    gtk_box_pack_start (GTK_BOX (hbox), psd->orientation, TRUE, TRUE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->orientation), 
                               _("Horizontal"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->orientation), 
                               _("Vertical"));
    
    g_signal_connect (psd->orientation, "changed", 
                      G_CALLBACK (orientation_changed), psd);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Handle Style:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);
    
    psd->handle_style = gtk_combo_box_new_text ();
    gtk_widget_show (psd->handle_style);
    gtk_box_pack_start (GTK_BOX (hbox), psd->handle_style, TRUE, TRUE, 0);

    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->handle_style), 
                               _("At both sides"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->handle_style), 
                               _("At the start"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (psd->handle_style), 
                               _("At the end"));

    g_signal_connect (psd->handle_style, "changed", 
                      G_CALLBACK (handle_style_changed), psd);
    
    g_object_unref (sg);
}

static void
create_properties_tab (PanelSettingsDialog *psd)
{
    GtkWidget *frame, *vbox2, *hbox, *label, *align;
    GtkSizeGroup *sg, *sg2;
    int n_monitors, i;

    psd->properties_box = hbox = gtk_hbox_new (FALSE, 2*BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (psd->vbox), hbox, TRUE, TRUE, 0);

    /* position */
    add_position_box (psd);
    
    /* other settings */
    psd->other_box = vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox2);
    gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
    
    /* monitor */
    n_monitors = panel_app_get_n_monitors ();
    
    if (n_monitors > 1)
    {
        GtkWidget *scroll = NULL;
        
        frame = xfce_create_framebox (_("Monitor"), &align);
        gtk_widget_show (frame);
        gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, BORDER);
        gtk_widget_show (hbox);

        psd->monitors = g_ptr_array_sized_new (n_monitors);

        for (i = 0; i < n_monitors; ++i)
        {
            GtkWidget *ebox, *ebox2, *b, *label;
            GtkStyle *style;
            char markup[10];
            
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
                              G_CALLBACK (monitor_pressed), psd);
            
            g_signal_connect (b, "key-press-event", 
                              G_CALLBACK (monitor_pressed), psd);
            
            g_ptr_array_add (psd->monitors, b);
        }

        if (scroll)
            gtk_container_add (GTK_CONTAINER (align), scroll);
        else
            gtk_container_add (GTK_CONTAINER (align), hbox);
    }

    /* size */
    frame = xfce_create_framebox (_("Size"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (align), hbox);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    sg2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    label = xfce_create_small_label (_("Small"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_size_group_add_widget (sg, label);
    
    psd->size = gtk_hscale_new_with_range (12, 128, 2);
    gtk_widget_set_size_request (psd->size, 150, -1);
    /*gtk_scale_set_draw_value (GTK_SCALE (psd->size), FALSE);*/
    gtk_scale_set_value_pos (GTK_SCALE (psd->size), GTK_POS_BOTTOM);
    gtk_range_set_update_policy (GTK_RANGE (psd->size),
                                 GTK_UPDATE_DELAYED);
    gtk_widget_show (psd->size);
    gtk_box_pack_start (GTK_BOX (hbox), psd->size, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg2, psd->size);
    
    label = xfce_create_small_label (_("Large"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    g_signal_connect (psd->size, "value-changed", 
                      G_CALLBACK (size_changed), psd);
    
    /* transparency */
    frame = xfce_create_framebox (_("Transparency"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (align), hbox);

    label = xfce_create_small_label (_("None"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_size_group_add_widget (sg, label);
    
    psd->transparency = gtk_hscale_new_with_range (0, 95, 5);
    gtk_widget_set_size_request (psd->transparency, 150, -1);
    /*gtk_scale_set_draw_value (GTK_SCALE (psd->transparency), FALSE);*/
    gtk_scale_set_value_pos (GTK_SCALE (psd->transparency), GTK_POS_BOTTOM);
    gtk_range_set_update_policy (GTK_RANGE (psd->transparency),
                                 GTK_UPDATE_DELAYED);
    gtk_widget_show (psd->transparency);
    gtk_box_pack_start (GTK_BOX (hbox), psd->transparency, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg2, psd->transparency);
    
    label = xfce_create_small_label (_("Full"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    g_signal_connect (psd->transparency, "value-changed", 
                      G_CALLBACK (transparency_changed), psd);
    
    g_object_unref (sg);
    g_object_unref (sg2);
    
    /* fill in values */
    update_properties_tab (psd);
}


/*
 * Panel Items
 * ===========
 */
static gboolean
add_selected_item (PanelSettingsDialog *psd)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (psd->tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &info, -1);

    if (!xfce_panel_item_manager_is_available (info->name))
        return FALSE;
   
    panel_add_item (psd->panel, info->name);

    return TRUE;
}

static gboolean
treeview_dblclick (GtkWidget * tv, GdkEventButton * evt, 
                   PanelSettingsDialog *psd)
{
    if (evt->button == 1 && evt->type == GDK_2BUTTON_PRESS)
    {
	return add_selected_item (psd);
    }

    return FALSE;
}

static void
cursor_changed (GtkTreeView * tv, PanelSettingsDialog *psd)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;
    
    if (!psd->add_item)
        return;

    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &info, -1);

    if (info)
    {
	gtk_widget_set_sensitive (psd->add_item, 
                xfce_panel_item_manager_is_available (info->name));
    }
    else
    {
	gtk_widget_set_sensitive (psd->add_item, FALSE);
    }
}

static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
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
        char *text;
        gboolean insensitive;

        insensitive = !xfce_panel_item_manager_is_available (info->name);
        
        if (info->comment)
        {
            text = g_strdup_printf ("<b>%s</b>\n%s", info->display_name, 
                                    info->comment);
        }
        else
        {
            text = g_strdup_printf ("<b>%s</b>", info->display_name);
        }
        
        g_object_set (cell, "markup", text, 
                      "foreground-set", insensitive, NULL);

        g_free (text);
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
        
        gtk_tree_selection_get_selected (sel, &model, &iter);

        gtk_tree_model_get (model, &iter, 0, &info, -1);

        if (!xfce_panel_item_manager_is_available (info->name))
            return;
       
        DBG (" + set data: %s", info->name);
        gtk_selection_data_set (data, data->target, 8, 
                                (guchar *)info->name, strlen (info->name));
    }
}

static void
add_item_treeview (PanelSettingsDialog *psd)
{
    GtkWidget *tv, *scroll;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    int i;
    GdkColor *color;

    psd->items = xfce_panel_item_manager_get_item_info_list ();
    
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (psd->items_box), scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    psd->tree = tv = gtk_tree_view_new_with_model (model);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

    g_object_unref (G_OBJECT (store));

    /* dnd */
    panel_dnd_set_name_source (tv);

    panel_dnd_set_widget_delete_dest (tv);

    g_signal_connect (tv, "drag-data-get", G_CALLBACK (treeview_data_get), 
                      psd);

    g_signal_connect (tv, "drag-data-received", 
                      G_CALLBACK (treeview_data_received), psd);
    
    g_signal_connect (tv, "drag-drop", 
                      G_CALLBACK (treeview_drag_drop), psd);
    
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
    for (i = 0; i < psd->items->len; ++i)
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
                            g_ptr_array_index (psd->items, i), -1);
    }

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed),
		      psd);

    g_signal_connect (tv, "button-press-event",
		      G_CALLBACK (treeview_dblclick), psd);

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

static void
create_items_tab (PanelSettingsDialog *psd)
{
    GtkWidget *vbox, *hbox, *img, *label, *align;
    char *markup;

    psd->items_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (psd->vbox), vbox, TRUE, TRUE, 0);

    /* info */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment (GTK_MISC (img), 0, 0);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);

    label = gtk_label_new (_("Drag items from the list to a panel or move "
                             "existing items to a new location."));
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
    
    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

    /* treeview */
    label = gtk_label_new (NULL);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    markup = g_strdup_printf ("<b>%s</b>", 
                              _("Avalaible Items"));
    gtk_label_set_markup (GTK_LABEL (label), markup);
    g_free (markup);

    add_item_treeview (psd);

#if 0
    /* button(s) */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    psd->add_item = xfce_create_mixed_button (GTK_STOCK_ADD, 
                                              _("Add To Selected Panel"));
    gtk_button_set_focus_on_click (GTK_BUTTON (psd->add_item), FALSE);
    gtk_widget_show (psd->add_item);
    gtk_box_pack_start (GTK_BOX (hbox), psd->add_item, FALSE, FALSE, 0);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

    g_signal_connect_swapped (psd->add_item, "clicked", 
                              G_CALLBACK (add_selected_item), psd);
#endif
}


/*
 * Settings Dialog
 * ===============
 */

static void
properties_dialog_opened (Panel *panel)
{
    panel_block_autohide (panel);

    gtk_drag_highlight (GTK_WIDGET (panel));
}

static void
items_dialog_opened (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_block_autohide (panel);

    xfce_itembar_raise_event_window (XFCE_ITEMBAR (priv->itembar));
    
    panel_dnd_set_dest (priv->itembar);
    panel_dnd_set_widget_source (priv->itembar);

    panel_set_items_sensitive (panel, FALSE);
}

static void
properties_dialog_closed (Panel *panel)
{
    panel_unblock_autohide (panel);
    gtk_drag_unhighlight (GTK_WIDGET (panel));
}

static void
items_dialog_closed (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_unblock_autohide (panel);
    
    xfce_itembar_lower_event_window (XFCE_ITEMBAR (priv->itembar));

    panel_set_items_sensitive (panel, TRUE);
    
    panel_dnd_unset_dest (priv->itembar);
    panel_dnd_unset_source (priv->itembar);
}

static void
dialog_response (GtkWidget *dlg, int response, PanelSettingsDialog *psd)
{
    if (response != GTK_RESPONSE_HELP)
    {
        if (dlg == dialog_widget)
        {
            dialog_widget = NULL;
            properties_dialog_closed (psd->panel);
        }
        else
        {
            dialog_widget_items = NULL;
            g_ptr_array_foreach (psd->panels, (GFunc)items_dialog_closed, 
                                 NULL);

            xfce_panel_item_manager_free_item_info_list (psd->items);

        }

        gtk_widget_destroy (dlg);
        
        if (psd->monitors)
            g_ptr_array_free (psd->monitors, TRUE);
        
        g_free (psd);

        panel_app_save ();
    }
    else
    {
        xfce_exec ("xfhelp4 panel.html", FALSE, FALSE, NULL);
    }
}

/* public API */
void 
panel_settings_dialog (GPtrArray *panels, gboolean show_items)
{
    PanelSettingsDialog *psd;
    GtkWidget *header, *vbox, *img;
    Panel *panel;

    if (!show_items)
    {
        if (dialog_widget)
        {
            gtk_window_present (GTK_WINDOW (dialog_widget));
            return;
        }
    }
    else
    {
        if (dialog_widget_items)
        {
            gtk_window_present (GTK_WINDOW (dialog_widget));
            return;
        }
    }
    
    psd = g_new0 (PanelSettingsDialog, 1);

    psd->panels = panels;
    panel = psd->panel = 
        g_ptr_array_index (panels, panel_app_get_current_panel());

    psd->updating = TRUE;
    
    psd->dlg = 
        gtk_dialog_new_with_buttons (_("Xfce Panel"), GTK_WINDOW (psd->panel), 
                                     GTK_DIALOG_NO_SEPARATOR,
                                     GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                     GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                     NULL);
    
    if (!show_items)
    {
        dialog_widget = psd->dlg;
        
        properties_dialog_opened (panel);
    }
    else
    {
        dialog_widget_items = psd->dlg;
        
        g_ptr_array_foreach (panels, (GFunc)items_dialog_opened, NULL);
    }
    
    gtk_dialog_set_default_response (GTK_DIALOG (psd->dlg), GTK_RESPONSE_OK);

    gtk_container_set_border_width (GTK_CONTAINER (psd->dlg), 2);

    gtk_window_stick (GTK_WINDOW (psd->dlg));
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (psd->dlg), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (psd->dlg), TRUE);
    gtk_window_set_position (GTK_WINDOW (psd->dlg), GTK_WIN_POS_CENTER);

    vbox = GTK_DIALOG (psd->dlg)->vbox;
    gtk_box_set_spacing (GTK_BOX (vbox), BORDER);

    img = gtk_image_new();
    gtk_widget_show (img);
    
    if ((gtk_major_version == 2 && gtk_minor_version >= 6) || 
         gtk_major_version > 2)
    {
        g_object_set (G_OBJECT (img), 
                      "icon-name", "xfce4-panel", 
                      "icon-size", GTK_ICON_SIZE_DIALOG, 
                      NULL);
    }
    else
    {
        GdkPixbuf *pb;

        pb = xfce_themed_icon_load ("xfce4-panel", 48);

        gtk_image_set_from_pixbuf (GTK_IMAGE (img), pb);

        g_object_unref (pb);
    }

    if (!show_items)
        header = xfce_create_header_with_image (img, _("Panel Preferences"));
    else
        header = xfce_create_header_with_image (img, _("Add Items"));
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, FALSE, 0);

    psd->vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (psd->vbox), BORDER - 2);
    gtk_widget_show (psd->vbox);
    gtk_box_pack_start (GTK_BOX (vbox), psd->vbox, TRUE, TRUE, 0);

    if (!show_items)
        create_properties_tab (psd);
    else
        create_items_tab (psd);

    g_signal_connect (psd->dlg, "response", G_CALLBACK (dialog_response), psd);

    psd->updating = FALSE;
    
    gtk_widget_show (psd->dlg);

    panel_app_register_dialog (psd->dlg);
}


