/*  popup.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#ifdef GDK_MULTIHEAD_SAFE
#undef GDK_MULTIHEAD_SAFE
#endif

#include <gdk/gdkkeysyms.h>

#include <libxfce4util/debug.h>
#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_togglebutton.h>
#include <libxfcegui4/xfce_menubutton.h>

#include "xfce.h"
#include "popup.h"
#include "item.h"
#include "settings.h"

static PanelPopup *open_popup = NULL;

/*  Panel popup callbacks
 *  ---------------------
*/
static void
hide_popup (PanelPopup * pp)
{
    if (open_popup == pp)
        open_popup = NULL;

    xfce_togglebutton_set_active (XFCE_TOGGLEBUTTON (pp->button), FALSE);

    gtk_widget_hide (pp->window);

    if (pp->detached)
    {
        pp->detached = FALSE;
        gtk_widget_show (pp->tearoff_button);
        gtk_window_set_decorated (GTK_WINDOW (pp->window), FALSE);
    }
}

static void
show_popup (PanelPopup * pp)
{
    GtkRequisition req;
    GdkWindow *p;
    int xbutton, ybutton, xparent, yparent, x, y;
    int w, h;
    gboolean vertical = settings.orientation == VERTICAL;
    int pos = settings.popup_position;
    GtkAllocation alloc1 = { 0, }, alloc2 =
    {
    0,};

    if (open_popup)
        hide_popup (open_popup);

    if (!pp->detached)
        open_popup = pp;

    if (pp->items && !pp->detached)
        gtk_widget_show (pp->tearoff_button);
    else
        gtk_widget_hide (pp->tearoff_button);

    if (disable_user_config || g_list_length (pp->items) >= NBITEMS)
    {
        gtk_widget_hide (pp->addtomenu_item->button);
        gtk_widget_hide (pp->separator);
    }
    else
    {
        gtk_widget_show (pp->addtomenu_item->button);
        gtk_widget_show (pp->separator);
    }

    alloc1 = pp->button->allocation;
    xbutton = alloc1.x;
    ybutton = alloc1.y;

    p = gtk_widget_get_parent_window (pp->button);
    gdk_window_get_root_origin (p, &xparent, &yparent);

    w = gdk_screen_width ();
    h = gdk_screen_height ();

    if (!GTK_WIDGET_REALIZED (pp->window))
        gtk_widget_realize (pp->window);

    gtk_widget_size_request (pp->window, &req);

    alloc2.width = req.width;
    alloc2.height = req.height;
    gtk_widget_size_allocate (pp->window, &alloc2);

    gtk_widget_realize (pp->window);

    /* this is necessary, because the icons are resized on allocation */
    gtk_widget_size_request (pp->window, &req);

    /*  positioning logic (well ...)
     *  ----------------------------
     *  1) vertical panel
     *  - to the left or the right
     *    - if buttons left the left  else right
     *  - fit the screen
     *  2) horizontal
     *  - up or down
     *    - if buttons down -> down else up
     *  - fit the screen
     */

    /* set x and y to topleft corner of the button */
    x = xbutton + xparent;
    y = ybutton + yparent;

    if (vertical)
    {
        /* left if buttons left ... */
        if (pos == LEFT && x - req.width >= 0)
        {
            x = x - req.width;
            y = y + alloc1.height - req.height;
        }
        /* .. or if menu doesn't fit right, but does fit left */
        else if (x + req.width + alloc1.width > w && x - req.width >= 0)
        {
            x = x - req.width;
            y = y + alloc1.height - req.height;
        }
        else                    /* right */
        {
            x = x + alloc1.width;
            y = y + alloc1.height - req.height;
        }

        /* check if it fits upwards */
        if (y < 0)
            y = 0;
    }
    else
    {
        /* down if buttons on bottom ... */
        if (pos == BOTTOM && y + alloc1.height + req.height <= h)
        {
            x = x + alloc1.width / 2 - req.width / 2;
            y = y + alloc1.height;
        }
        /* ... or up doesn't fit and down does */
        else if (y - req.height < 0 && y + alloc1.height + req.height <= h)
        {
            x = x + alloc1.width / 2 - req.width / 2;
            y = y + alloc1.height;
        }
        else
        {
            x = x + alloc1.width / 2 - req.width / 2;
            y = y - req.height;
        }

        /* check if it fits to the left and the right */
        if (x < 0)
            x = 0;
        else if (x + req.width > w)
            x = w - req.width;
    }

    gtk_window_move (GTK_WINDOW (pp->window), x, y);
    gtk_window_stick (pp->window);
    gtk_widget_show (pp->window);
}

void
hide_current_popup_menu (void)
{
    if (open_popup)
        hide_popup (open_popup);
}

void
toggle_popup (GtkWidget * button, PanelPopup * pp)
{
    hide_current_popup_menu ();

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        show_popup (pp);
    else
        hide_popup (pp);
}

void
tearoff_popup (GtkWidget * button, PanelPopup * pp)
{
    open_popup = NULL;
    pp->detached = TRUE;
    gtk_widget_hide (pp->window);
    gtk_widget_hide (pp->tearoff_button);
    gtk_window_set_decorated (GTK_WINDOW (pp->window), TRUE);
    show_popup (pp);
}

gboolean
delete_popup (GtkWidget * window, GdkEvent * ev, PanelPopup * pp)
{
    hide_popup (pp);

    return TRUE;
}

gboolean
popup_key_pressed (GtkWidget * window, GdkEventKey * ev, PanelPopup * pp)
{
    if (ev->keyval == GDK_Escape)
    {
        hide_popup (pp);
        return TRUE;
    }
    else
        return FALSE;
}

/*  Popup menus 
 *  -----------
*/
static void
set_popup_window_properties (GtkWidget * win)
{
    GtkWindow *window = GTK_WINDOW (win);
    GdkPixbuf *pb;

    gtk_window_set_decorated (window, FALSE);
    gtk_window_set_resizable (window, FALSE);
    gtk_window_stick (window);
    gtk_window_set_title (window, _("Menu"));
    gtk_window_set_transient_for (window, GTK_WINDOW (toplevel));
    gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_MENU);

    pb = get_system_pixbuf (MENU_ICON);
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    set_window_layer (win, settings.layer);

    /* don't care about decorations when calculating position */
    gtk_window_set_gravity (window, GDK_GRAVITY_STATIC);
}

static GtkTargetEntry entry[] = {
    {"text/uri-list", 0, 0},
    {"STRING", 0, 1}
};

static gboolean
drag_motion (GtkWidget * widget, GdkDragContext * context,
             int x, int y, guint time, PanelPopup * pp)
{
    if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pp->button)))
    {
        DBG ("open popup\n");
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pp->button), TRUE);
    }
    else
        DBG ("already open\n");

    return TRUE;
}

PanelPopup *
create_panel_popup (void)
{
    PanelPopup *pp = g_new (PanelPopup, 1);
    GtkWidget *sep;
    GtkArrowType at;
    gboolean vertical = settings.orientation == VERTICAL;

    /* the button */
    if (vertical)
    {
        if (settings.popup_position == LEFT)
            at = GTK_ARROW_LEFT;
        else
            at = GTK_ARROW_RIGHT;
    }
    else
    {
        if (settings.popup_position == BOTTOM)
            at = GTK_ARROW_DOWN;
        else
            at = GTK_ARROW_UP;
    }

    pp->button = xfce_togglebutton_new (at);
    gtk_widget_show (pp->button);
    g_object_ref (pp->button);
    gtk_widget_set_name (pp->button, "xfce_popup_button");

    xfce_togglebutton_set_relief (XFCE_TOGGLEBUTTON (pp->button),
                                  GTK_RELIEF_NONE);

    g_signal_connect (pp->button, "toggled", G_CALLBACK (toggle_popup), pp);

    gtk_drag_dest_set (pp->button, GTK_DEST_DEFAULT_ALL, entry, 2,
                       GDK_ACTION_COPY);
    g_signal_connect (pp->button, "drag-motion", G_CALLBACK (drag_motion), pp);

    /* the menu */
    pp->hgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    pp->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    set_popup_window_properties (pp->window);

    pp->frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (pp->frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (pp->window), pp->frame);

    pp->vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (pp->frame), pp->vbox);

    pp->addtomenu_item = menu_item_new (pp);
    create_addtomenu_item (pp->addtomenu_item);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->addtomenu_item->button, TRUE,
                        TRUE, 0);

    pp->separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->separator, FALSE, FALSE, 0);

    /* we don't know the items until we read the config file */
    pp->items = NULL;
    pp->item_vbox = gtk_vbox_new (TRUE, 0);
    gtk_widget_show (pp->item_vbox);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->item_vbox, FALSE, FALSE, 0);

    pp->tearoff_button = gtk_button_new ();
    gtk_button_set_relief (GTK_BUTTON (pp->tearoff_button), GTK_RELIEF_NONE);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->tearoff_button, FALSE, TRUE, 0);
    sep = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (pp->tearoff_button), sep);

    /* signals */
    g_signal_connect (pp->tearoff_button, "clicked", G_CALLBACK (tearoff_popup),
                      pp);

    g_signal_connect (pp->window, "delete-event", G_CALLBACK (delete_popup),
                      pp);

    g_signal_connect (pp->window, "key-press-event",
                      G_CALLBACK (popup_key_pressed), pp);

    /* apparently this is necessary to make the popup show correctly */
    pp->detached = FALSE;

    gtk_widget_show_all (pp->frame);

    panel_popup_set_size (pp, settings.size);

    return pp;
}

void
panel_popup_pack (PanelPopup * pp, GtkBox * box)
{
    gtk_box_pack_start (box, pp->button, FALSE, FALSE, 0);
}

void
panel_popup_unpack (PanelPopup * pp)
{
    GtkWidget *container = pp->button->parent;

    gtk_container_remove (GTK_CONTAINER (container), pp->button);
}

void
panel_popup_add_item (PanelPopup * pp, Item * mi)
{
    GList *li;
    int i;

    gtk_box_pack_start (GTK_BOX (pp->item_vbox), mi->button, TRUE, TRUE, 0);
    gtk_box_reorder_child (GTK_BOX (pp->item_vbox), mi->button, mi->pos);

    pp->items = g_list_insert (pp->items, mi, mi->pos);

    for (i = 0, li = pp->items; li && li->data; i++, li = li->next)
    {
        Item *item = li->data;

        item->pos = i;
    }
}

void
panel_popup_remove_item (PanelPopup * pp, Item * mi)
{
    GList *li;
    int i;

    gtk_container_remove (GTK_CONTAINER (pp->vbox), mi->button);

    pp->items = g_list_remove (pp->items, mi);

    item_free (mi);

    for (i = 0, li = pp->items; li && li->data; i++, li = li->next)
    {
        Item *item = li->data;

        item->pos = i;
    }
}

void
panel_popup_set_from_xml (PanelPopup * pp, xmlNodePtr node)
{
    xmlNodePtr child;
    int i;

    for (i = 0, child = node->children; child; i++, child = child->next)
    {
        Item *mi;

        if (!xmlStrEqual (child->name, (const xmlChar *)"Item"))
            continue;

        mi = menu_item_new (pp);
        item_read_config (mi, child);
        create_menu_item (mi);

        mi->pos = i;

        panel_popup_add_item (pp, mi);
    }
}

void
panel_popup_write_xml (PanelPopup * pp, xmlNodePtr root)
{
    xmlNodePtr node, child;
    GList *li;

    node = xmlNewTextChild (root, NULL, "Popup", NULL);

    for (li = pp->items; li; li = li->next)
    {
        Item *mi = li->data;

        child = xmlNewTextChild (node, NULL, "Item", NULL);

        item_write_config (mi, child);
    }
}

void
panel_popup_free (PanelPopup * pp)
{
    /* only items contain non-gtk elements to be freed */
    GList *li;

    for (li = pp->items; li && li->data; li = li->next)
    {
        Item *mi = li->data;

        item_free (mi);
    }

    g_free (pp);
}

void
panel_popup_set_size (PanelPopup * pp, int size)
{
    int pos = settings.popup_position;
    int w, h;
    GList *li;

    w = icon_size[size] + border_width;
    h = top_height[size];

    if (pos == LEFT || pos == RIGHT)
        gtk_widget_set_size_request (pp->button, h, w);
    else
        gtk_widget_set_size_request (pp->button, w, h);

    /* decide on popup size based on panel size */
    menu_item_set_popup_size (pp->addtomenu_item, size);

    for (li = pp->items; li && li->data; li = li->next)
    {
        Item *mi = li->data;

        menu_item_set_popup_size (mi, size);
    }

}

void
panel_popup_set_popup_position (PanelPopup * pp, int position)
{
    GtkArrowType at;
    gboolean vertical = settings.orientation == VERTICAL;

    settings.popup_position = position;

    if (vertical)
    {
        if (settings.popup_position == LEFT)
            at = GTK_ARROW_LEFT;
        else
            at = GTK_ARROW_RIGHT;
    }
    else
    {
        if (settings.popup_position == BOTTOM)
            at = GTK_ARROW_DOWN;
        else
            at = GTK_ARROW_UP;
    }

    xfce_togglebutton_set_arrow_type (XFCE_TOGGLEBUTTON (pp->button), at);
    panel_popup_set_size (pp, settings.size);
}

void
panel_popup_set_layer (PanelPopup * pp, int layer)
{
    set_window_layer (pp->window, layer);
}

void
panel_popup_set_theme (PanelPopup * pp, const char *theme)
{
    GList *li;

    for (li = pp->items; li && li->data; li = li->next)
    {
        Item *mi = li->data;

        item_set_theme (mi, theme);
    }
}
