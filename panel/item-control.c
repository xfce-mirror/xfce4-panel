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

#include "controls.h"
#include "item-control.h"
#include "item.h"
#include "popup.h"
#include "item_dialog.h"
#include "groups.h"

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
item_control_set_theme (Control * control, const char *theme)
{
    ItemControl *ic= control->data;

    item_apply_config (ic->item);

    if (ic->popup)
        panel_popup_set_theme (ic->popup, theme);
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

    cc->set_theme = item_control_set_theme;
    cc->set_size = item_control_set_size;
}
