/* vim: set expandtab ts=8 sw=4: */

/*  $Id: panel-settings.c 19012 2005-12-07 12:13:48Z jasper $
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

typedef struct _PanelManagerDialog PanelManagerDialog;

struct _PanelManagerDialog
{
    GtkWidget *dlg;

    GPtrArray *panels;
    Panel *panel;
    int current;
    
    int contents_id;

    int highlight_id;
    GtkWidget *highlight;
    
    GtkTooltips *tips;

    GtkWidget *mainbox;
    GtkWidget *panelbox;

    /* Panel Selection */
    GtkWidget *panel_selector;
    GtkWidget *add_panel;
    GtkWidget *rm_panel;

    /* item tree */
    GtkWidget *panel_tree;
    GtkWidget *up;
    GtkWidget *down;
    GtkWidget *rm_item;
    GtkWidget *add_new_item;
    
    GtkWidget *notebook;

    gboolean updating;

    /* Panel Properties */
    GtkWidget *properties_box;
    GtkWidget *left_box;
    GtkWidget *right_box;
    
    GPtrArray *monitors;
    GtkWidget *size;
    GtkWidget *transparency;
    GtkWidget *autohide;

    GtkWidget *position_box;
    GtkWidget *fixed;
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
static GdkCursor *cursor = NULL;

static void dialog_opened (Panel *panel);

static void update_item_tree (PanelManagerDialog *pmd);


/*
 * Grippy Cursor
 * =============
 */

/* Cursor inline pixbuf data. Copied from libexo.
 * Copyright (c) 2004 os-cillation e.K.
 * Written by Benedikt Meurer <benny@xfce.org>.
 * License: LGPL
 */
#ifdef __SUNPRO_C
#pragma align 4 (drag_cursor_data)
#endif
#ifdef __GNUC__
static const guint8 drag_cursor_data[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 drag_cursor_data[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (2304) */
  "\0\0\11\30"
  /* pixdata_type (0x1010002) */
  "\1\1\0\2"
  /* rowstride (96) */
  "\0\0\0`"
  /* width (24) */
  "\0\0\0\30"
  /* height (24) */
  "\0\0\0\30"
  /* pixel_data: */
  "\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\2\0\0"
  "\0\2\0\0\0\2\0\0\0\3\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\2"
  "\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\11\0\0\0\0\0\0"
  "\0\0\0\0\0\1\0\0\0\2\0\0\0\3\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\2\0\0\0\2"
  "\0\0\0\2\0\0\0\2\0\0\0\1\0\0\0\1\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\1\0\0"
  "\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\0\1\0\0\0\7\0\0\0\4\0\0\0\2\0\0\0\3\0\0\0\3\0\0\0\3\0\0\0\2\0\0"
  "\0\31\0\0\0#\0\0\0\30\0\0\0\2\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1\0\0\0\1"
  "\0\0\0\1\0\0\0\3\0\0\0\2\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\1\0\0\0\2\0\0\0\6\0\0\0\11\0\0\0\14\0\0\0\14\0\0\0\12\0\0\0\206\0"
  "\0\0\303\0\0\0\205\0\0\0\11\0\0\0\6\0\0\0\10\0\0\0\6\0\0\0\3\0\0\0\2"
  "\0\0\0\4\0\0\0\12\0\0\0\7\0\0\0\5\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\4\0\0\0\6\0\0\0V\0\0\0\245\0\0\0\211\0\0\0\4\0\0\0\207\246\246\246"
  "\331\335\335\335\365\245\245\245\333\0\0\0\250\0\0\0\244\0\0\0\243\0"
  "\0\0""9\0\0\0\2\0\0\0\1\0\0\0\1\0\0\0\2\0\0\0\1\0\0\0\1\0\0\0\0\0\0\0"
  "\0\0\0\0\1\0\0\0\6\0\0\0\12\0\0\0\\uuu\271\270\270\270\353\246\246\246"
  "\332\0\0\0\205\0\0\0\267\271\271\271\352\377\377\377\377\270\270\270"
  "\353$$$\312\270\270\270\353\271\271\271\353WWW\246\0\0\0B\0\0\0\4\0\0"
  "\0\13\0\0\0\7\0\0\0\3\0\0\0\4\0\0\0\1\0\0\0\0\0\0\0\2\0\0\0\11\0\0\0"
  "\12\0\0\0\205\220\220\220\341\377\377\377\377\335\335\335\366\0\0\0\307"
  "\0\0\0\304\270\270\270\353\377\377\377\377\271\271\271\352555\314\377"
  "\377\377\377\377\377\377\377eee\327\0\0\0f\0\0\0)\0\0\0[\0\0\0\10\0\0"
  "\0\4\0\0\0\10\0\0\0\3\0\0\0\2\0\0\0\3\0\0\0\11\0\0\0\6\0\0\0[uuu\271"
  "\321\321\321\362\350\350\350\371ddd\332\22\22\22\311\270\270\270\354"
  "\377\377\377\377\271\271\271\353555\314\377\377\377\377\377\377\377\377"
  "eee\327\0\0\0\210;;;{^^^\300\0\0\0E\0\0\0\15\0\0\0\10\0\0\0\6\0\0\0\5"
  "\0\0\0\5\0\0\0\10\0\0\0\4\0\0\0\12\0\0\0iddd\330\377\377\377\377\377"
  "\377\377\377444\320\267\267\267\354\377\377\377\377\270\270\270\3545"
  "55\316\377\377\377\377\377\377\377\377eee\327\33\33\33\311\201\201\201"
  "\337\335\335\335\365\0\0\0\302\0\0\0\40\0\0\0\4\0\0\0\2\0\0\0\3\0\0\0"
  "\5\0\0\0\3\0\0\0\6\0\0\0\13\0\0\0jddd\331\377\377\377\377\377\377\377"
  "\377444\320\270\270\270\354\377\377\377\377\267\267\267\354444\320\377"
  "\377\377\377\377\377\377\377eee\326\221\221\221\340\377\377\377\377\335"
  "\335\335\365\0\0\0\301\0\0\0\40\0\0\0\4\0\0\0\4\0\0\0\5\0\0\0\7\0\0\0"
  "\6\0\0\0\211\0\0\0\242\0\0\0b((([^^^\323\377\377\377\377\343\343\343"
  "\367\364\364\364\374\377\377\377\377\364\364\364\374\342\342\342\367"
  "\377\377\377\377\377\377\377\377ccc\333\217\217\217\344\377\377\377\377"
  "\335\335\335\365\0\0\0\302\0\0\0\40\0\0\0\5\0\0\0\1\0\0\0\3\0\0\0\34"
  "\0\0\0\204\246\246\246\332\271\271\271\352www\266\0\0\0\227666\312\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\320\320\320"
  "\363\334\334\334\366\377\377\377\377\335\335\335\365\0\0\0\302\0\0\0"
  "\40\0\0\0\4\0\0\0\0\0\0\0\2\0\0\0'\0\0\0\303\335\335\335\365\377\377"
  "\377\377\313\313\313\360eee\327555\315\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\335\335"
  "\335\365\205\205\205\313\0\0\0b\0\0\0\24\0\0\0\4\0\0\0\0\0\0\0\3\0\0"
  "\0\35\0\0\0\205\245\245\245\332\351\351\351\371\377\377\377\377\321\321"
  "\321\363\201\201\201\340\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\271\271\271\352\0\0"
  "\0\241\0\0\0\1\0\0\0\5\0\0\0\3\0\0\0\0\0\0\0\3\0\0\0\11\0\0\0\11\0\0"
  "\0\214\245\245\245\333\356\356\356\372\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\271\271\271\352\0\0\0\240\0\0\0\0\0\0\0\1\0"
  "\0\0\2\0\0\0\0\0\0\0\0\0\0\0\2\0\0\0\14\0\0\0\15\0\0\0\212\217\217\217"
  "\344\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\270\270"
  "\270\353\0\0\0\243\0\0\0\0\0\0\0\3\0\0\0\7\0\0\0\0\0\0\0\0\0\0\0\3\0"
  "\0\0\13\0\0\0\5\0\0\0\207\217\217\217\344\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\243\243\243\350...\234\0\0\0+\0\0\0\1\0\0\0\4\0\0\0\5\0"
  "\0\0\0\0\0\0\0\0\0\0\1\0\0\0\4\0\0\0\5\0\0\0""5JJJ\223\235\235\235\346"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\217\217\217\344\0\0\0\210\0\0\0\6\0\0\0\2\0"
  "\0\0\3\0\0\0\3\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\6\0\0\0\11\0\0\0"
  ":JJJ\221\245\245\245\346\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\266\266\266\356^^^\254\0\0\0N\0\0\0\3\0\0\0\1\0\0\0"
  "\1\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\5\0\0\0\7\0\0\0\12\0"
  "\0\0""8---\240\270\270\270\354\364\364\364\374\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377ccc\334\0\0\0o\0\0\0\20\0\0\0\6\0\0\0\0\0\0\0\1\0\0\0\1\0\0\0\0\0"
  "\0\0\0\0\0\0\1\0\0\0\3\0\0\0\4\0\0\0\11\0\0\0\13\0\0\0\25\0\0\0D\0\0"
  "\0\306\335\335\335\365\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377ddd\330\0\0\0b\0\0\0"
  "\4\0\0\0\11\0\0\0\1\0\0\0\2\0\0\0\2\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
  "\0\0\1\0\0\0\5\0\0\0\16\0\0\0\25\0\0\0""2\0\0\0\310\334\334\334\366\377"
  "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377"
  "\377\377\377\377\377\377ccc\332\0\0\0d\0\0\0\10\0\0\0\23\0\0\0\5\0\0"
  "\0\7\0\0\0\10\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\5"
  "\0\0\0\12\0\0\0\24\0\0\0""8111\267444\320444\321444\322444\321444\317"
  "444\320%%%a\0\0\0\26\0\0\0\3\0\0\0\3\0\0\0\1\0\0\0\10\0\0\0\10\0\0\0"
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1\0\0\0\7\0\0\0\10\0\0\0\12\0\0\0\23"
  "\0\0\0\32\0\0\0E\0\0\0Q\0\0\0Q\0\0\0O\0\0\0N\0\0\0R\0\0\0L\0\0\0\35\0"
  "\0\0\4\0\0\0\2\0\0\0\1\0\0\0\2\0\0\0\5\0\0\0\6"
};

static void
create_grippy_cursor (GtkWidget *widget)
{
    GdkPixbuf *pixbuf;

    pixbuf = 
        gdk_pixbuf_new_from_inline (-1, drag_cursor_data, FALSE, NULL);
    cursor = 
        gdk_cursor_new_from_pixbuf (gtk_widget_get_display (widget),
                                    pixbuf, 12, 12);
    g_object_unref (pixbuf);
}

static void
destroy_grippy_cursor (void)
{
    if (cursor)
    {
        gdk_cursor_unref (cursor);
        cursor = NULL;
    }
}

static void
set_grippy_cursor_for_widget (GtkWidget *widget)
{
    if (widget->window)
    {
        gdk_window_set_cursor (widget->window, cursor);
    }
}

static void
set_grippy_cursor (PanelManagerDialog *pmd)
{
    int i;

    if (pmd->tree && pmd->tree->window)
    {
        gdk_window_set_cursor (pmd->tree->window, cursor);
    }

    for (i = 0; i < pmd->panels->len; ++i)
    {
        GtkWidget *widget = g_ptr_array_index (pmd->panels, i);

        gdk_window_set_cursor (widget->window, cursor);
    }
}

static void
unset_grippy_cursor (PanelManagerDialog *pmd)
{
    int i;

    if (pmd->tree && pmd->tree->window)
        gdk_window_set_cursor (pmd->tree->window, NULL);

    for (i = 0; i < pmd->panels->len; ++i)
    {
        GtkWidget *widget = g_ptr_array_index (pmd->panels, i);

        gdk_window_set_cursor (widget->window, NULL);
    }
}


/* Highlight Widget
 * ================
 */

static gboolean
highlight_exposed (GtkWidget *widget, GdkEventExpose *ev, 
                   PanelManagerDialog *pmd)
{
    int w, h;
    
    w = widget->allocation.width - 1;
    h = widget->allocation.height - 1;
    
    /* draw highlight */
    gdk_draw_rectangle (GDK_DRAWABLE (widget->window),
                        widget->style->bg_gc[GTK_STATE_SELECTED],
                        FALSE, 0, 0, w, h);

    return TRUE;
}

static void
blink_widget (GtkWidget *widget)
{
    /* do something clever to draw attention to selected widget */
}

static void
highlight_widget (GtkWidget *widget, PanelManagerDialog *pmd)
{
    if (pmd->highlight_id)
        g_signal_handler_disconnect (pmd->highlight, pmd->highlight_id);

    blink_widget (widget);
    
    pmd->highlight = widget;

    pmd->highlight_id = 
        g_signal_connect_after (widget, "expose-event",
                                G_CALLBACK (highlight_exposed), pmd);

    gtk_widget_queue_draw (widget);
}

static void
unhighlight_widget (GtkWidget *widget, PanelManagerDialog *pmd)
{
    if (pmd->highlight != widget)
        return;
    
    if (pmd->highlight_id)
        g_signal_handler_disconnect (pmd->highlight, pmd->highlight_id);

    pmd->highlight_id = 0;
    pmd->highlight = NULL;
}

/*
 * Update properties
 * =================
 */

static void
update_properties (PanelManagerDialog *pmd)
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
        gtk_range_set_value (GTK_RANGE (pmd->transparency), 
                             priv->transparency);

    /* behavior */
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->autohide),
                                  priv->autohide);
    
    /* position */
    DBG ("position: %d", priv->screen_position);

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

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pmd->fullwidth),
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

/* 
 * Appearance
 * ==========
 */

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
create_appearance_tab (PanelManagerDialog *pmd)
{
    static Atom composite_atom = 0;
    GtkWidget *frame, *box, *vbox2, *hbox, *label, *align;
    GtkSizeGroup *sg, *sg2;

    label = gtk_label_new (_("Appearance"));
    gtk_misc_set_padding (GTK_MISC (label), BORDER, 1);
    gtk_widget_show (label);
    
    pmd->properties_box = box = pmd->right_box = vbox2 = 
        gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (box), BORDER);
    gtk_widget_show (box);
        
    gtk_notebook_append_page (GTK_NOTEBOOK (pmd->notebook), box, label);
    
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
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_size_group_add_widget (sg, label);
    
    pmd->size = gtk_hscale_new_with_range (12, 128, 2);
    gtk_scale_set_value_pos (GTK_SCALE (pmd->size), GTK_POS_BOTTOM);
    gtk_range_set_update_policy (GTK_RANGE (pmd->size),
                                 GTK_UPDATE_DELAYED);
    gtk_widget_show (pmd->size);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->size, TRUE, TRUE, 0);

    label = xfce_create_small_label (_("Large"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    
    gtk_size_group_add_widget (sg2, label);
        
    g_signal_connect (pmd->size, "value-changed", 
                      G_CALLBACK (size_changed), pmd);
    
    /* transparency */
    if (G_UNLIKELY (!composite_atom))
        composite_atom = 
            XInternAtom (GDK_DISPLAY (), "COMPOSITING_MANAGER", False);

    if (XGetSelectionOwner (GDK_DISPLAY (), composite_atom))
    {
        frame = xfce_create_framebox (_("Transparency"), &align);
        gtk_widget_show (frame);
        gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
        
        hbox = gtk_hbox_new (FALSE, BORDER);
        gtk_widget_show (hbox);
        gtk_container_add (GTK_CONTAINER (align), hbox);

        label = xfce_create_small_label (_("None"));
        gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        
        gtk_size_group_add_widget (sg, label);
        
        pmd->transparency = gtk_hscale_new_with_range (0, 100, 5);
        gtk_scale_set_value_pos (GTK_SCALE (pmd->transparency), GTK_POS_BOTTOM);
        gtk_range_set_update_policy (GTK_RANGE (pmd->transparency),
                                     GTK_UPDATE_DELAYED);
        gtk_widget_show (pmd->transparency);
        gtk_box_pack_start (GTK_BOX (hbox), pmd->transparency, TRUE, TRUE, 0);

        label = xfce_create_small_label (_("Full"));
        gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
        gtk_widget_show (label);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        
        gtk_size_group_add_widget (sg2, label);
        
        g_signal_connect (pmd->transparency, "value-changed", 
                          G_CALLBACK (transparency_changed), pmd);
    }
    
    g_object_unref (sg);
    g_object_unref (sg2);
}


/* 
 * Position
 * ========
 */

/* monitor */
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

/* type and position */
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

    update_properties (pmd);
}

/* fixed position */
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
            int i;

            for (i = 0; i < 12; ++i)
            {
                GtkToggleButton *button = 
                    GTK_TOGGLE_BUTTON (pmd->screen_position[i]);

                if (button == tb)
                {
                    gtk_toggle_button_set_active (button, TRUE);
                    panel_set_screen_position (pmd->panel, i + 1);
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
fullwidth_changed (GtkToggleButton *tb, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_full_width (pmd->panel, gtk_toggle_button_get_active (tb));
}

static void
autohide_changed (GtkToggleButton *tb, PanelManagerDialog *pmd)
{
    if (pmd->updating)
        return;

    panel_set_autohide (pmd->panel, gtk_toggle_button_get_active (tb));
}

/* floating panel */
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

/* creation */
static void
add_position_box (PanelManagerDialog *pmd)
{
    GtkWidget *hbox, *vbox, *vbox2, *frame, *table, *align, *label;
    GtkSizeGroup *sg;
    int i;
    
    hbox = pmd->left_box;

    pmd->position_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

    /* floating? */
    frame = xfce_create_framebox (_("Panel Type"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    vbox2 = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (align), vbox2);

    pmd->fixed = 
        gtk_radio_button_new_with_label (NULL, _("Fixed Position"));
    gtk_widget_show (pmd->fixed);
    gtk_box_pack_start (GTK_BOX (vbox2), pmd->fixed, FALSE, FALSE, 0);
    
    pmd->floating = 
        gtk_radio_button_new_with_label_from_widget (
                GTK_RADIO_BUTTON (pmd->fixed), _("Freely Moveable"));
    gtk_widget_show (pmd->floating);
    gtk_box_pack_start (GTK_BOX (vbox2), pmd->floating, FALSE, FALSE, 0);

    g_signal_connect (pmd->fixed, "toggled", G_CALLBACK (type_changed), 
                      pmd);
    
    g_signal_connect (pmd->floating, "toggled", G_CALLBACK (type_changed), 
                      pmd);
    
    /* position */
    frame = xfce_create_framebox (_("Position"), &align);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    
    vbox2 = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (align), vbox2);

    /* fixed */
    pmd->fixed_box = vbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, TRUE, 0);

    hbox = gtk_vbox_new (FALSE, 0);
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
        pmd->screen_position[i] = gtk_toggle_button_new ();
        gtk_widget_show (pmd->screen_position[i]);

        if (i <= 2 || i >= 9)
            gtk_widget_set_size_request (pmd->screen_position[i], 30, 15);
        else
            gtk_widget_set_size_request (pmd->screen_position[i], 15, 30);

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

    pmd->fullwidth = 
        gtk_check_button_new_with_mnemonic (_("_Full Width"));
    gtk_widget_show (pmd->fullwidth);
    gtk_box_pack_start (GTK_BOX (vbox), pmd->fullwidth, FALSE, FALSE, 0);

    g_signal_connect (pmd->fullwidth, "toggled", 
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

    label = gtk_label_new (_("Handle Position:"));
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
    
    g_object_unref (sg);
}

static void
create_position_tab (PanelManagerDialog *pmd)
{
    GtkWidget *frame, *box, *hbox, *label, *align;
    int n_monitors, i;

    label = gtk_label_new (_("Position"));
    gtk_misc_set_padding (GTK_MISC (label), BORDER, 1);
    gtk_widget_show (label);
    
    pmd->properties_box = pmd->left_box = hbox = box = 
        gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (box), BORDER);
    gtk_widget_show (box);
        
    gtk_notebook_append_page (GTK_NOTEBOOK (pmd->notebook), box, label);
    
    /* position */
    add_position_box (pmd);
    
    /* monitor */
    n_monitors = panel_app_get_n_monitors ();
    
    if (n_monitors > 1)
    {
        GtkWidget *scroll = NULL;
        
        frame = xfce_create_framebox (_("Monitor"), &align);
        gtk_widget_show (frame);
        gtk_box_pack_start (GTK_BOX (box), frame, FALSE, FALSE, 0);

        hbox = gtk_hbox_new (FALSE, BORDER);
        gtk_widget_show (hbox);

        pmd->monitors = g_ptr_array_sized_new (n_monitors);

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


/*
 * Panel Items
 * ===========
 */

static gboolean
add_selected_item (PanelManagerDialog *pmd)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pmd->tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &info, -1);

    if (!xfce_panel_item_manager_is_available (info->name))
        return FALSE;
   
    panel_add_item (pmd->panel, info->name);

    return TRUE;
}

static gboolean
treeview_dblclick (GtkWidget * tv, GdkEventButton * evt, 
                   PanelManagerDialog *pmd)
{
    if (evt->button == 1 && evt->type == GDK_2BUTTON_PRESS)
    {
	return add_selected_item (pmd);
    }

    return FALSE;
}

static void
cursor_changed (GtkTreeView * tv, PanelManagerDialog *pmd)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    XfcePanelItemInfo *info;
    
    if (!pmd->add_item)
        return;

    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &info, -1);

    if (info)
    {
	gtk_widget_set_sensitive (pmd->add_item, 
                xfce_panel_item_manager_is_available (info->name));
        gtk_tooltips_set_tip (pmd->tips, GTK_WIDGET (tv), info->comment, NULL);
    }
    else
    {
	gtk_widget_set_sensitive (pmd->add_item, FALSE);
        gtk_tooltips_set_tip (pmd->tips, GTK_WIDGET (tv), NULL, NULL);
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
    GtkWidget *item;

    gtk_tree_model_get (model, iter, 0, &info, 1, &item, -1);

    if (info)
    {
        gboolean insensitive;

        insensitive = !xfce_panel_item_manager_is_available (info->name);
        
#if 0
        if (info->comment)
        {
            text = g_strdup_printf ("<b>%s</b>\n%s", info->display_name, 
                                    info->comment);
        }
        else
        {
            text = g_strdup_printf ("<b>%s</b>", info->display_name);
        }
#endif
        /* not panel tree */
        if (!item)
            g_object_set (cell, "markup", info->display_name, 
                          "foreground-set", insensitive, NULL);
        else
            g_object_set (cell, "markup", info->display_name, NULL);
    }
    else
    {
        /* not panel tree */
        if (!item)
            g_object_set (cell, "markup", "", "foreground-set", TRUE, NULL);
        else
            g_object_set (cell, "markup", "", NULL);
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
add_item_treeview (PanelManagerDialog *pmd)
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

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (pmd->items_box), scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    pmd->tree = tv = gtk_tree_view_new_with_model (model);
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
                      pmd);

    g_signal_connect (tv, "drag-data-received", 
                      G_CALLBACK (treeview_data_received), pmd);
    
    g_signal_connect (tv, "drag-drop", 
                      G_CALLBACK (treeview_drag_drop), pmd);
    
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
    for (i = 0; i < pmd->items->len; ++i)
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
                            g_ptr_array_index (pmd->items, i), -1);
    }

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed),
		      pmd);

    g_signal_connect (tv, "button-press-event",
		      G_CALLBACK (treeview_dblclick), pmd);

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

static void
item_dialog_response (GtkWidget *dlg, int response, 
                      PanelManagerDialog *pmd)
{
    pmd->tree = NULL;
    gtk_widget_destroy (dlg);

    unset_grippy_cursor (pmd);

    pmd->updating = FALSE;
    
    update_item_tree (pmd);

    gtk_widget_set_sensitive (pmd->dlg, TRUE);
}

static void
add_items_dialog (PanelManagerDialog *pmd)
{
    GtkWidget *dlg;
    GtkWidget *vbox, *hbox, *img, *label, *align;
    char *markup;

    gtk_widget_set_sensitive (pmd->dlg, FALSE);

    pmd->updating = TRUE;
    
    dlg = gtk_dialog_new_with_buttons (_("Add Items"), GTK_WINDOW (pmd->dlg),
                                       GTK_DIALOG_NO_SEPARATOR,
                                       GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                       NULL);

    g_signal_connect (dlg, "response", G_CALLBACK (item_dialog_response), pmd);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    pmd->items_box = vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER - 2);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox, TRUE, TRUE, 0);

    /* info */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment (GTK_MISC (img), 0, 0);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);

    label = gtk_label_new (_("Drag items from the list to a panel or remove "
                             "them by dragging them back to the list."));
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

    add_item_treeview (pmd);

    gtk_widget_realize (pmd->tree);

    set_grippy_cursor_for_widget (pmd->tree);
    set_grippy_cursor (pmd);

    /* button(s) */
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    pmd->add_item = xfce_create_mixed_button (GTK_STOCK_ADD, 
                                              _("Add To Selected Panel"));
    gtk_button_set_focus_on_click (GTK_BUTTON (pmd->add_item), FALSE);
    gtk_widget_show (pmd->add_item);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->add_item, FALSE, FALSE, 0);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, TRUE, TRUE, 0);

    g_signal_connect_swapped (pmd->add_item, "clicked", 
                              G_CALLBACK (add_selected_item), pmd);

    gtk_widget_show (dlg);
}

/* current items */
static void
move_item (GtkWidget *button, PanelManagerDialog *pmd)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter, iter2;
    GtkTreePath *path;
    GtkWidget *item;
    int new_pos = -1;
    
    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pmd->panel_tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 1, &item, -1);

    if (item)
    {
        path = gtk_tree_model_get_path (model, &iter);

        if (button == pmd->up)
        {
            if (gtk_tree_path_prev (path) &&
                    gtk_tree_model_get_iter (model, &iter2, path))
            {
                int *n;
                
                n = gtk_tree_path_get_indices (path);

                new_pos = n[0];
            }
        }
        else if (button == pmd->down)
        {
            gtk_tree_path_next (path);

            if (gtk_tree_model_get_iter (model, &iter2, path))
            {
                int *n;
                
                n = gtk_tree_path_get_indices (path);

                new_pos = n[0];
            }
        }

        if (new_pos > -1)
        {
            pmd->updating = TRUE;
        
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);
            gtk_tree_view_set_cursor (GTK_TREE_VIEW (pmd->panel_tree), 
                                      path, NULL, FALSE);

            xfce_itembar_reorder_child (
                    XFCE_ITEMBAR (GTK_BIN (pmd->panel)->child), 
                    item, new_pos);

            pmd->updating = FALSE;
        }
        
        gtk_tree_path_free (path);
    }
}

static void
item_cursor_changed (GtkTreeView * tv, PanelManagerDialog *pmd)
{
    /*
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    GtkWidget *item;
    
    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 1, &item, -1);

    if (item)
    {
        gtk_tree_view_get_cursor (tv, &path, NULL);
        gtk_tree_path_free (path);
    }
    else
    {
    }
    */
}

static void
fill_item_tree (PanelManagerDialog *pmd)
{
    int i;
    GList *items, *l;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreePath *path;

    items = gtk_container_get_children (
                GTK_CONTAINER (GTK_BIN (pmd->panel)->child));

    store = GTK_LIST_STORE (gtk_tree_view_get_model (
                GTK_TREE_VIEW (pmd->panel_tree)));
    
    /* fill model */
    for (l = items; l != NULL; l = l->next)
    {
        XfcePanelItem *item = l->data;
        XfcePanelItemInfo *info = NULL;
        
        for (i = 0; i < pmd->items->len; ++i)
        {
            info = g_ptr_array_index (pmd->items, i);

            if (!strcmp (info->name, xfce_panel_item_get_name (item)))
                break;
        }

        if (!info)
            g_critical ("No matching plugin found: %s\n",
                        xfce_panel_item_get_name (item));

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, info, 1, item, -1);
    }

    g_list_free (items);

    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (pmd->panel_tree), path, NULL, 
                              FALSE);
    gtk_tree_path_free (path);
}

static void
update_item_tree (PanelManagerDialog *pmd)
{
    gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (
                GTK_TREE_VIEW (pmd->panel_tree))));
    
    fill_item_tree (pmd);
}

static void
create_panel_item_tree (PanelManagerDialog *pmd)
{
    GtkWidget *tv, *scroll;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *model;
    
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (pmd->panelbox), scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (2, G_TYPE_POINTER, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    pmd->panel_tree = tv = gtk_tree_view_new_with_model (model);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

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

    g_object_unref (G_OBJECT (store));

    fill_item_tree (pmd);

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (item_cursor_changed),
		      pmd);
}

static void
create_items_tab (PanelManagerDialog *pmd)
{
    GtkWidget *label, *hbox, *img, *box, *align;
    
    label = gtk_label_new (_("Panel Items"));
    gtk_misc_set_padding (GTK_MISC (label), BORDER, 1);
    gtk_widget_show (label);
    
    pmd->panelbox = box = gtk_vbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (pmd->panelbox), BORDER);
    gtk_widget_show (pmd->panelbox);

    gtk_notebook_append_page (GTK_NOTEBOOK (pmd->notebook), pmd->panelbox, 
                              label);
    
    create_panel_item_tree (pmd);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

    pmd->up = gtk_button_new ();
    gtk_widget_show (pmd->up);
    gtk_button_set_focus_on_click (GTK_BUTTON (pmd->up), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->up, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->up), img);

    g_signal_connect (pmd->up, "clicked", G_CALLBACK (move_item), pmd);
    
    pmd->down = gtk_button_new ();
    gtk_widget_show (pmd->down);
    gtk_button_set_focus_on_click (GTK_BUTTON (pmd->down), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->down, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->down), img);

    g_signal_connect (pmd->down, "clicked", G_CALLBACK (move_item), pmd);
    
    align = gtk_alignment_new (0,0,0,0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
    
    pmd->rm_item = gtk_button_new ();
    gtk_widget_show (pmd->rm_item);
    gtk_button_set_focus_on_click (GTK_BUTTON (pmd->rm_item), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->rm_item, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->rm_item), img);
    
    pmd->add_new_item = gtk_button_new ();
    gtk_widget_show (pmd->add_new_item);
    gtk_button_set_focus_on_click (GTK_BUTTON (pmd->add_new_item), FALSE);
    gtk_box_pack_start (GTK_BOX (hbox), pmd->add_new_item, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->add_new_item), img);

    g_signal_connect_swapped (pmd->add_new_item, "clicked", 
                              G_CALLBACK (add_items_dialog), pmd);

    /* dnd */
    panel_dnd_set_widget_delete_dest (pmd->rm_item);

    g_signal_connect (pmd->rm_item, "drag-data-received", 
                      G_CALLBACK (treeview_data_received), pmd);
    
    g_signal_connect (pmd->rm_item, "drag-drop", 
                      G_CALLBACK (treeview_drag_drop), pmd);

}

/*
 * Panel Selector
 * ===============
 */

static void
panel_contents_changed (GtkWidget *itembar, PanelManagerDialog *pmd)
{
    if (!pmd->updating)
        update_item_tree (pmd);
}

static void
panel_selected (GtkComboBox * combo, PanelManagerDialog * pmd)
{
    int n = gtk_combo_box_get_active (combo);

    if (n == pmd->current)
        return;

    pmd->current = n;

    if (pmd->panel)
        unhighlight_widget (GTK_WIDGET (pmd->panel), pmd);

    if (pmd->contents_id)
        g_signal_handler_disconnect (GTK_BIN (pmd->panel)->child, 
                                     pmd->contents_id);
    
    pmd->panel = g_ptr_array_index (pmd->panels, n);

    update_properties (pmd);

    update_item_tree (pmd);
    
    gtk_notebook_set_current_page (GTK_NOTEBOOK (pmd->notebook), 0);

    highlight_widget (GTK_WIDGET (pmd->panel), pmd);

    pmd->contents_id = 
        g_signal_connect (GTK_BIN (pmd->panel)->child, "contents-changed",
                          G_CALLBACK (panel_contents_changed), pmd);
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

    dialog_opened (g_ptr_array_index (pmd->panels, n));
    set_grippy_cursor_for_widget (
            GTK_WIDGET (g_ptr_array_index (pmd->panels, n)));

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

static void
create_panel_selector (PanelManagerDialog * pmd)
{
    GtkWidget *box, *img;
    int i;

    box = gtk_hbox_new (FALSE, BORDER);
    gtk_container_set_border_width (GTK_CONTAINER (box), BORDER - 2);
    gtk_widget_show (box);
    gtk_box_pack_start (GTK_BOX (pmd->mainbox), box, FALSE, FALSE, 0);

    pmd->panel_selector = gtk_combo_box_new_text ();
    gtk_widget_show (pmd->panel_selector);
    gtk_box_pack_start (GTK_BOX (box), pmd->panel_selector, TRUE, TRUE, 0);

    for (i = 0; i < pmd->panels->len; ++i)
    {
        char name[20];

        g_snprintf (name, 20, _("Panel %d"), i + 1);

        gtk_combo_box_append_text (GTK_COMBO_BOX (pmd->panel_selector), name);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (pmd->panel_selector),
                              pmd->current);

    highlight_widget (GTK_WIDGET (pmd->panel), pmd);

    g_signal_connect (pmd->panel_selector, "changed",
                      G_CALLBACK (panel_selected), pmd);
    
    pmd->rm_panel = gtk_button_new ();
    gtk_widget_show (pmd->rm_panel);
    gtk_box_pack_start (GTK_BOX (box), pmd->rm_panel, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->rm_panel), img);

    gtk_tooltips_set_tip (pmd->tips, pmd->rm_panel, _("Remove Panel"), NULL);

    g_signal_connect (pmd->rm_panel, "clicked",
                      G_CALLBACK (remove_panel), pmd);
    
    pmd->add_panel = gtk_button_new ();
    gtk_widget_show (pmd->add_panel);
    gtk_box_pack_start (GTK_BOX (box), pmd->add_panel, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (pmd->add_panel), img);

    gtk_tooltips_set_tip (pmd->tips, pmd->add_panel, _("New Panel"), NULL);

    g_signal_connect (pmd->add_panel, "clicked", G_CALLBACK (add_panel), pmd);
}

/*
 * Settings Dialog
 * ===============
 */

static void
dialog_opened (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_block_autohide (panel);

    xfce_itembar_raise_event_window (XFCE_ITEMBAR (priv->itembar));
    
    panel_dnd_set_dest (priv->itembar);
    panel_dnd_set_widget_source (priv->itembar);

    panel_set_items_sensitive (panel, FALSE);
}

static void
dialog_closed (Panel *panel)
{
    PanelPrivate *priv = PANEL_GET_PRIVATE (panel);
    
    panel_unblock_autohide (panel);
    
    xfce_itembar_lower_event_window (XFCE_ITEMBAR (priv->itembar));

    panel_set_items_sensitive (panel, TRUE);
    
    panel_dnd_unset_dest (priv->itembar);
    panel_dnd_unset_source (priv->itembar);
}

static void
dialog_response (GtkWidget *dlg, int response, PanelManagerDialog *pmd)
{
    if (response != GTK_RESPONSE_HELP)
    {
        dialog_widget = NULL;
        unset_grippy_cursor (pmd);
        destroy_grippy_cursor ();
        g_ptr_array_foreach (pmd->panels, (GFunc)dialog_closed, NULL);

        xfce_panel_item_manager_free_item_info_list (pmd->items);

        unhighlight_widget (pmd->highlight, pmd);

        gtk_widget_destroy (dlg);
        
        if (pmd->monitors)
            g_ptr_array_free (pmd->monitors, TRUE);

        if (pmd->contents_id)
            g_signal_handler_disconnect (GTK_BIN (pmd->panel)->child, 
                                         pmd->contents_id);
        
        g_object_unref (pmd->tips);
        g_free (pmd);

        panel_app_save ();
    }
    else
    {
        xfce_exec_on_screen (gtk_widget_get_screen (dlg), 
                             "xfhelp4 panel.html", FALSE, FALSE, NULL);
    }
}

/* public API */
void 
panel_manager_dialog (GPtrArray *panels, gboolean show_items)
{
    PanelManagerDialog *pmd;
    GtkWidget *header, *vbox, *img;
    Panel *panel;

    if (dialog_widget)
    {
        int n = panel_app_get_current_panel ();
        GdkScreen *screen = 
            gtk_widget_get_screen (g_ptr_array_index (panels, n));

        if (screen != gtk_widget_get_screen (dialog_widget))
            gtk_window_set_screen (GTK_WINDOW (dialog_widget), screen);

        gtk_window_present (GTK_WINDOW (dialog_widget));
        return;
    }
    
    pmd = g_new0 (PanelManagerDialog, 1);

    /* panels */
    pmd->panels = panels;
    pmd->current = panel_app_get_current_panel();
    panel = pmd->panel = 
        g_ptr_array_index (panels, pmd->current);

    /* available items */
    pmd->items = xfce_panel_item_manager_get_item_info_list ();
    
    /* main dialog widget */
    dialog_widget = pmd->dlg = 
        gtk_dialog_new_with_buttons (_("Xfce Panel"), NULL, 
                                     GTK_DIALOG_NO_SEPARATOR,
                                     GTK_STOCK_HELP, GTK_RESPONSE_HELP,
                                     GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                     NULL);
    
    gtk_dialog_set_default_response (GTK_DIALOG (pmd->dlg), GTK_RESPONSE_OK);

    gtk_container_set_border_width (GTK_CONTAINER (pmd->dlg), 2);

    gtk_window_stick (GTK_WINDOW (pmd->dlg));
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (pmd->dlg), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (pmd->dlg), TRUE);
    gtk_window_set_position (GTK_WINDOW (pmd->dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_screen (GTK_WINDOW (pmd->dlg), 
                           gtk_widget_get_screen (GTK_WIDGET (panel)));

    create_grippy_cursor (GTK_WIDGET (panel));
    
    pmd->tips = gtk_tooltips_new ();
    g_object_ref (pmd->tips);
    gtk_object_sink (GTK_OBJECT (pmd->tips));
    
    pmd->mainbox = vbox = GTK_DIALOG (pmd->dlg)->vbox;

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

    header = xfce_create_header_with_image (img, _("Customize Panels"));
    gtk_container_set_border_width (GTK_CONTAINER (header), BORDER - 2);
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, FALSE, 0);

    pmd->updating = TRUE;
    
    /* select panel */
    create_panel_selector (pmd);

    /* notebook with panel options */
    pmd->notebook = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (pmd->notebook), BORDER - 2);
    gtk_widget_show (pmd->notebook);
    gtk_box_pack_start (GTK_BOX (vbox), pmd->notebook, TRUE, TRUE, 0);

    create_items_tab (pmd);
    
    create_position_tab (pmd);

    create_appearance_tab (pmd);

    pmd->updating = FALSE;

    /* fill in values */
    update_properties (pmd);
    
    /* make panels insensitive, set up dnd and highlight current panel */
    g_ptr_array_foreach (panels, (GFunc)dialog_opened, NULL);
    highlight_widget (GTK_WIDGET (panel), pmd);

    /* watch for outside changes */
    pmd->contents_id = 
        g_signal_connect (GTK_BIN (pmd->panel)->child, "contents-changed",
                          G_CALLBACK (panel_contents_changed), pmd);

    g_signal_connect (pmd->dlg, "response", G_CALLBACK (dialog_response), pmd);
    gtk_widget_show (pmd->dlg);

    panel_app_register_dialog (pmd->dlg);
}


