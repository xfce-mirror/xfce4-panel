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

#include <gtk/gtk.h>
#include <gmodule.h>
#include <gdk/gdkkeysyms.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_togglebutton.h>
#include <libxfcegui4/xfce_menubutton.h>

#include "icons.h"
#include "controls.h"
#include "item-control.h"
#include "item.h"
#include "item_dialog.h"
#include "panel.h"
#include "settings.h"

#ifdef GDK_MULTIHEAD_SAFE
#undef GDK_MULTIHEAD_SAFE
#endif

#define NBITEMS 32


/* PanelPopup *
 * ---------- */

G_MODULE_EXPORT /* EXPORT:open_popup */
PanelPopup *open_popup = NULL;

/*  Panel popup callbacks
 *  ---------------------
*/
static void
hide_popup (PanelPopup * pp)
{
    if (open_popup == pp)
	open_popup = NULL;

    if (!pp)
	return;

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
position_popup (PanelPopup * pp)
{
    GtkRequisition req;
    GdkWindow *p;
    int xbutton, ybutton, xparent, yparent, x, y;
    int w, h;
    gboolean vertical = settings.orientation == VERTICAL;
    GtkAllocation alloc1 = { 0 }, alloc2 = { 0};
    GtkArrowType at;

    if (!pp)
	return;

    if (pp->detached)
	return;

    alloc1 = pp->button->allocation;
    xbutton = alloc1.x;
    ybutton = alloc1.y;

    p = gtk_widget_get_parent_window (pp->button);
    gdk_window_get_origin (p, &xparent, &yparent);

    w = gdk_screen_width ();
    h = gdk_screen_height ();

    at = xfce_togglebutton_get_arrow_type (XFCE_TOGGLEBUTTON (pp->button));

    if (!GTK_WIDGET_REALIZED (pp->window))
	gtk_widget_realize (pp->window);

    gtk_widget_size_request (pp->window, &req);

    alloc2.width = req.width;
    alloc2.height = req.height;
    gtk_widget_size_allocate (pp->window, &alloc2);

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
	/* left if buttons left or if menu doesn't fit right, 
         * but does fit left */
	if ((at == GTK_ARROW_LEFT || x + req.width + alloc1.width > w)
	    && x - req.width >= 0)
	{
	    x = x - req.width;
	}
	else			/* right */
	{
	    x = x + alloc1.width;
	}
	y = y + alloc1.height - req.height;

	/* check if it fits upwards */
	if (y < 0)
	    y = 0;
    }
    else
    {
	/* down if buttons on bottom or up doesn't fit and down does */
	if ((at == GTK_ARROW_DOWN || y - req.height < 0)
	    && y + alloc1.height + req.height <= h)
	{
	    y = y + alloc1.height;
	}
	else
	{
	    y = y - req.height;
	}
	x = x + alloc1.width / 2 - req.width / 2;

	/* check if it fits to the left and the right */
	if (x < 0)
	    x = 0;
	else if (x + req.width > w)
	    x = w - req.width;
    }

    gtk_window_move (GTK_WINDOW (pp->window), x, y);
    gtk_window_stick (GTK_WINDOW (pp->window));
    gtk_widget_show (pp->window);
}

static void
show_popup (PanelPopup * pp)
{
    if (!pp)
	return;

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

    position_popup (pp);
}

G_MODULE_EXPORT /* EXPORT:hide_current_popup_menu */
void
hide_current_popup_menu (void)
{
    if (open_popup)
	hide_popup (open_popup);
}

G_MODULE_EXPORT /* EXPORT:toggle_popup */
void
toggle_popup (GtkWidget * button, PanelPopup * pp)
{
    hide_current_popup_menu ();

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
	show_popup (pp);
    else
	hide_popup (pp);
}

G_MODULE_EXPORT /* EXPORT:tearoff_popup */
void
tearoff_popup (GtkWidget * button, PanelPopup * pp)
{
    open_popup = NULL;
    pp->detached = TRUE;
    gtk_widget_hide (pp->tearoff_button);
    gtk_window_set_decorated (GTK_WINDOW (pp->window), TRUE);
}

G_MODULE_EXPORT /* EXPORT:delete_popup */
gboolean
delete_popup (GtkWidget * window, GdkEvent * ev, PanelPopup * pp)
{
    hide_popup (pp);

    return TRUE;
}

G_MODULE_EXPORT /* EXPORT:popup_key_pressed */
gboolean
popup_key_pressed (GtkWidget * window, GdkEventKey * ev, PanelPopup * pp)
{
    if (ev->keyval == GDK_Escape)
    {
	hide_popup (pp);
	return TRUE;
    }
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
    gtk_window_set_transient_for (window, GTK_WINDOW (panel.toplevel));
    gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_MENU);

    pb = get_panel_pixbuf ();
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

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

G_MODULE_EXPORT /* EXPORT:create_panel_popup */
PanelPopup *
create_panel_popup (void)
{
    PanelPopup *pp = g_new (PanelPopup, 1);
    GtkWidget *sep;
    GtkArrowType at;
    gboolean vertical = settings.orientation == VERTICAL;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

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
    g_signal_connect (pp->button, "drag-motion", G_CALLBACK (drag_motion),
		      pp);

    /* the menu */
    pp->hgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    pp->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    set_popup_window_properties (pp->window);

    g_signal_connect_swapped (pp->window, "size-allocate",
			      G_CALLBACK (position_popup), pp);

    pp->frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (pp->frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (pp->window), pp->frame);

    pp->vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (pp->frame), pp->vbox);

    pp->addtomenu_item = menu_item_new (pp);
    create_addtomenu_item (pp->addtomenu_item);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->addtomenu_item->button, FALSE,
			FALSE, 0);

    pp->separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->separator, FALSE, FALSE, 0);

    /* we don't know the items until we read the config file */
    pp->items = NULL;
    pp->item_vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (pp->item_vbox);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->item_vbox, FALSE, FALSE, 0);

    pp->tearoff_button = gtk_button_new ();
    gtk_button_set_relief (GTK_BUTTON (pp->tearoff_button), GTK_RELIEF_NONE);
    gtk_box_pack_start (GTK_BOX (pp->vbox), pp->tearoff_button, FALSE, FALSE,
			0);
    sep = gtk_hseparator_new ();
    gtk_container_add (GTK_CONTAINER (pp->tearoff_button), sep);

    /* signals */
    g_signal_connect (pp->tearoff_button, "clicked",
		      G_CALLBACK (tearoff_popup), pp);

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

G_MODULE_EXPORT /* EXPORT:panel_popup_pack */
void
panel_popup_pack (PanelPopup * pp, GtkBox * box)
{
    if (!pp)
	return;

    gtk_box_pack_start (box, pp->button, FALSE, FALSE, 0);
}

G_MODULE_EXPORT /* EXPORT:panel_popup_unpack */
void
panel_popup_unpack (PanelPopup * pp)
{
    GtkWidget *container;

    if (!pp)
	return;

    container = pp->button->parent;

    gtk_container_remove (GTK_CONTAINER (container), pp->button);
}

G_MODULE_EXPORT /* EXPORT:panel_popup_add_item */
void
panel_popup_add_item (PanelPopup * pp, Item * mi)
{
    GList *li;
    int i;

    gtk_box_pack_start (GTK_BOX (pp->item_vbox), mi->button, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (pp->item_vbox), mi->button, mi->pos);

    pp->items = g_list_insert (pp->items, mi, mi->pos);

    for (i = 0, li = pp->items; li && li->data; i++, li = li->next)
    {
	Item *item = li->data;

	item->pos = i;
    }
}

G_MODULE_EXPORT /* EXPORT:panel_popup_remove_item */
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

G_MODULE_EXPORT /* EXPORT:panel_popup_set_from_xml */
void
panel_popup_set_from_xml (PanelPopup * pp, xmlNodePtr node)
{
    xmlNodePtr child;
    int i;

    if (!pp)
	return;

    for (i = 0, child = node->children; child; i++, child = child->next)
    {
	Item *mi;

	if (!xmlStrEqual (child->name, (const xmlChar *) "Item"))
	    continue;

	mi = menu_item_new (pp);
	item_read_config (mi, child);
	create_menu_item (mi);

	mi->pos = i;

	panel_popup_add_item (pp, mi);
    }
}

G_MODULE_EXPORT /* EXPORT:panel_popup_write_xml */
void
panel_popup_write_xml (PanelPopup * pp, xmlNodePtr root)
{
    xmlNodePtr node, child;
    GList *li;

    if (!pp || !pp->items)
	return;

    node = xmlNewTextChild (root, NULL, "Popup", NULL);

    for (li = pp->items; li; li = li->next)
    {
	Item *mi = li->data;

	child = xmlNewTextChild (node, NULL, "Item", NULL);

	item_write_config (mi, child);
    }
}

G_MODULE_EXPORT /* EXPORT:panel_popup_free */
void
panel_popup_free (PanelPopup * pp)
{
    /* only items contain non-gtk elements to be freed */
    GList *li;

    if (!pp)
	return;

    gtk_widget_destroy (pp->window);

    for (li = pp->items; li && li->data; li = li->next)
    {
	Item *mi = li->data;

	item_free (mi);
    }

    g_free (pp);
}

G_MODULE_EXPORT /* EXPORT:panel_popup_set_size */
void
panel_popup_set_size (PanelPopup * pp, int size)
{
    int pos = settings.popup_position;
    int w, h;
    GList *li;

    if (!pp)
	return;

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

G_MODULE_EXPORT /* EXPORT:panel_popup_set_popup_position */
void
panel_popup_set_popup_position (PanelPopup * pp, int position)
{
    settings.popup_position = position;

    if (!pp)
	return;

    panel_popup_set_size (pp, settings.size);
}

G_MODULE_EXPORT /* EXPORT:panel_popup_set_theme */
void
panel_popup_set_theme (PanelPopup * pp, const char *theme)
{
    GList *li;

    if (!pp)
	return;

    for (li = pp->items; li && li->data; li = li->next)
    {
	Item *mi = li->data;

	item_set_theme (mi, theme);
    }
}

G_MODULE_EXPORT /* EXPORT:panel_popup_set_arrow_type */
void
panel_popup_set_arrow_type (PanelPopup * pp, GtkArrowType type)
{
    if (!pp || !pp->button)
	return;

    xfce_togglebutton_set_arrow_type (XFCE_TOGGLEBUTTON (pp->button), type);
}

/* ItemControl *
 * ----------- */

/* PanelGroup replacement */

static void
item_control_arrange (ItemControl *ic)
{
    int position = settings.popup_position;

    if (ic->box)
    {
	if (ic->popup && ic->popup->button->parent)
	    gtk_container_remove (GTK_CONTAINER (ic->box), ic->popup->button);
	if (ic->item)
	    gtk_container_remove (GTK_CONTAINER (ic->box), ic->item->button);

	gtk_widget_destroy (ic->box);
    }

    if (position == TOP || position == BOTTOM)
	ic->box = gtk_vbox_new (FALSE, 0);
    else
	ic->box = gtk_hbox_new (FALSE, 0);

    gtk_widget_show (ic->box);
    gtk_container_add (GTK_CONTAINER (ic->base), ic->box);

    if (position == RIGHT || position == BOTTOM)
    {
	if (ic->item)
	    gtk_box_pack_start (GTK_BOX (ic->box), ic->item->button,
                                TRUE, TRUE, 0);
	if (ic->popup)
	    gtk_box_pack_start (GTK_BOX (ic->box), ic->popup->button,
                                FALSE, FALSE, 0);
    }
    else
    {
	if (ic->popup)
	    gtk_box_pack_start (GTK_BOX (ic->box), ic->popup->button,
                                FALSE, FALSE, 0);
	if (ic->item)
	    gtk_box_pack_start (GTK_BOX (ic->box), ic->item->button,
                                TRUE, TRUE, 0);
    }
}

void 
item_control_add_popup_from_xml (Control *control, xmlNodePtr node)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return;
        
    ic = control->data;

    ic->popup = create_panel_popup ();
    item_control_arrange (ic);

    panel_popup_set_from_xml (ic->popup, node);

    control->with_popup = ic->item->with_popup = TRUE;

    item_control_show_popup (control, TRUE);
}

void 
item_control_set_popup_position (Control *control, int position)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return;
        
    ic = control->data;

    item_control_arrange (ic);

    if (ic->popup)
        panel_popup_set_popup_position (ic->popup, position);
}

void 
item_control_set_arrow_direction (Control *control, GtkArrowType type)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return;
        
    ic = control->data;

    if (ic->popup)
        panel_popup_set_arrow_type (ic->popup, type);
}

PanelPopup *
item_control_get_popup (Control *control)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return NULL;
        
    ic = control->data;

    return ic->popup;
}

void 
item_control_show_popup (Control *control, gboolean show)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return;
        
    ic = control->data;

    /* make sure these match */
    ic->item->with_popup = control->with_popup;

    if (show && control->with_popup && !ic->popup)
    {
	ic->popup = create_panel_popup ();
        panel_popup_set_arrow_type (ic->popup, groups_get_arrow_direction());
	item_control_arrange (ic);
    }

    if (ic->popup)
    {
	if (show)
	    gtk_widget_show (ic->popup->button);
	else
	    gtk_widget_hide (ic->popup->button);
    }
}

void 
item_control_write_popup_xml (Control *control, xmlNodePtr node)
{
    ItemControl *ic;

    if (control->cclass->id != ICON)
        return;
        
    ic = control->data;

    if (ic->popup && control->with_popup)
        panel_popup_write_xml (ic->popup, node);
}

/* Control interface */

static ItemControl *
item_control_new (void)
{
    ItemControl *ic = g_new0 (ItemControl, 1);
    
    ic->base = gtk_alignment_new (0.5, 0.5, 1, 1);
    gtk_widget_show (ic->base);

    ic->item = panel_item_new ();
    item_control_arrange (ic);

    return ic;
}

static void
item_control_free (Control * control)
{
    ItemControl *ic= control->data;

    item_free (ic->item);
    
    if (ic->popup)
        panel_popup_free (ic->popup);
    
    g_free (ic);
}

static void
item_control_set_size (Control * control, int size)
{
    ItemControl *ic= control->data;
    int s = icon_size[size] + border_width;
    
    if (ic->item)
        gtk_widget_set_size_request (ic->item->button, s, s);
    if (ic->popup)
        panel_popup_set_size (ic->popup, size);
}

static void
item_control_set_theme (Control * control, const char *theme)
{
    ItemControl *ic= control->data;

    item_apply_config (ic->item);

    if (ic->popup)
        panel_popup_set_theme (ic->popup, theme);
}

static void
item_control_read_config (Control * control, xmlNodePtr node)
{
    ItemControl *ic= control->data;

    item_read_config (ic->item, node);
    item_apply_config (ic->item);
}

static void
item_control_write_config (Control * control, xmlNodePtr root)
{
    ItemControl *ic = control->data;

    item_write_config (ic->item, root);
}

static void
item_control_create_options (Control * control, GtkContainer * container,
                             GtkWidget * done)
{
    ItemControl *ic = control->data;

    create_item_dialog (control, ic->item, container, done);
}

static void
item_control_attach_callback (Control * control, const char *signal,
			      GCallback callback, gpointer data)
{
    ItemControl *ic = control->data;

    g_signal_connect (ic->item->button, signal, callback, data);

    if (ic->popup)
        g_signal_connect (ic->popup->button, signal, callback, data);
}

G_MODULE_EXPORT /* EXPORT:create_item_control */
void
create_item_control (Control * control)
{
    ItemControl *ic = item_control_new ();

    control->with_popup = FALSE;
    gtk_container_add (GTK_CONTAINER (control->base), ic->base);

    control->data = (gpointer) ic;
}

G_MODULE_EXPORT /* EXPORT:item_control_class_init */
void
item_control_class_init (ControlClass * cc)
{
    cc->id = ICON;
    cc->name = "icon";
    cc->caption = _("Launcher");

    cc->create_control = (CreateControlFunc) create_item_control;

    cc->free = item_control_free;
    cc->read_config = item_control_read_config;
    cc->write_config = item_control_write_config;
    cc->attach_callback = item_control_attach_callback;

    cc->create_options = item_control_create_options;

    cc->set_size = item_control_set_size;
    cc->set_theme = item_control_set_theme;
}
