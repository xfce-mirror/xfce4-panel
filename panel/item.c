/*  xfce4
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

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_togglebutton.h>
#include <libxfcegui4/xfce_menubutton.h>

#include "xfce.h"
#include "item.h"
#include "popup.h"
#include "item_dialog.h"
#include "settings.h"

/*  Common item callbacks
 *  ---------------------
*/
static void item_drop_cb(GtkWidget * widget, GdkDragContext * context, gint x, gint y,
		  GtkSelectionData * data, guint info, guint time, 
		  Item * item)
{
    GList *fnames, *fnp;
    guint count;
    char *execute;

    if (!item || !(item->command))
	return;
    
    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        execute = g_new0(char, MAXSTRLEN);

        strcpy(execute, item->command);

        for(fnp = fnames; fnp; fnp = fnp->next, count--)
        {
            strcat(execute, " \'");
            strncat(execute, (char *)(fnp->data), MAXSTRLEN - strlen(execute));
            strcat(execute, "\' ");
        }

        exec_cmd(execute, item->in_terminal, item->use_sn);
        g_free(execute);

	hide_current_popup_menu();
    }
    
    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

static void item_click_cb(GtkButton * b, Item * item)
{
    hide_current_popup_menu();
    
    exec_cmd(item->command, item->in_terminal, item->use_sn);
}

/*  Menu item callbacks
 *  -------------------
*/
static void addtomenu_item_drop_cb(GtkWidget * widget, 
				   GdkDragContext * context,
				   gint x, gint y, 
				   GtkSelectionData * data,
				   guint info, guint time, 
				   PanelPopup * pp)
{
    GList *fnames, *fnp;
    guint count;

    fnames = gnome_uri_list_extract_filenames((char *)data->data);
    count = g_list_length(fnames);

    if(count > 0)
    {
        for(fnp = fnames; fnp; fnp = fnp->next, count--)
        {
            Item *mi;

	    mi = menu_item_new(pp);

            mi->command = g_strdup((char *)fnp->data);

            mi->caption = g_path_get_basename(mi->command);
            mi->caption[0] = g_ascii_toupper(mi->caption[0]);

            create_menu_item(mi);
	    panel_popup_add_item(pp, mi);

	    if (!pp->detached)
	    {
		xfce_togglebutton_toggled(XFCE_TOGGLEBUTTON(pp->button));
		xfce_togglebutton_toggled(XFCE_TOGGLEBUTTON(pp->button));
	    }
        }
    }

    gnome_uri_list_free_strings(fnames);
    gtk_drag_finish(context, (count > 0), (context->action == GDK_ACTION_MOVE),
                    time);
}

static void addtomenu_item_click_cb(GtkButton * b, PanelPopup * pp)
{
    add_menu_item_dialog(pp);
}

static gboolean menu_item_press(GtkButton * b, GdkEventButton * ev, Item * mi)
{
    if(disable_user_config)
        return FALSE;

    if(ev->button == 3 || 
	    (ev->button == 1 && (ev->state & GDK_SHIFT_MASK)))
    {
	edit_menu_item_dialog(mi);

	return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*  Common item interface
 *  ---------------------
*/
void item_read_config(Item * item, xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar *value;

    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Caption"))
        {
            value = DATA(child);

            if(value)
                item->caption = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Command"))
        {
            int n = -1;

            value = DATA(child);

            if(value)
                item->command = (char *)value;

            value = xmlGetProp(child, "term");

            if(value)
	    {
                n = atoi(value);
		g_free(value);
            }
	    
            if(n == 1)
	    {
                item->in_terminal = TRUE;
            }
	    else
	    {
                item->in_terminal = FALSE;
            }

            n = -1;
	    value = xmlGetProp(child, "sn");

            if(value)
	    {
                n = atoi(value);
		g_free(value);
            }

            if(n == 1)
	    {
                item->use_sn = TRUE;
            }
	    else
	    {
	        item->use_sn = FALSE;
            }

        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Tooltip"))
        {
            value = DATA(child);

            if(value)
                item->tooltip = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Icon"))
        {
            value = xmlGetProp(child, (const xmlChar *)"id");

            if(value)
                item->icon_id = atoi(value);

            if(!value || item->icon_id < EXTERN_ICON || item->icon_id >= NUM_ICONS)
                item->icon_id = UNKNOWN_ICON;

            g_free(value);

            if(item->icon_id == EXTERN_ICON)
            {
                value = DATA(child);

                if(value)
                    item->icon_path = (char *)value;
                else
                    item->icon_id = UNKNOWN_ICON;
            }
        }
    }
}

void item_write_config(Item * item, xmlNodePtr node)
{
    xmlNodePtr child;
    char value[3];

    if (item->type == MENUITEM)
	xmlNewTextChild(node, NULL, "Caption", item->caption);

    child = xmlNewTextChild(node, NULL, "Command", item->command);

    snprintf(value, 2, "%d", item->in_terminal);
    xmlSetProp(child, "term", value);

    snprintf(value, 2, "%d", item->use_sn);
    xmlSetProp(child, "sn", value);

    if(item->tooltip)
        xmlNewTextChild(node, NULL, "Tooltip", item->tooltip);

    child = xmlNewTextChild(node, NULL, "Icon", item->icon_path);

    snprintf(value, 3, "%d", item->icon_id);
    xmlSetProp(child, "id", value);
}

void item_free(Item * item)
{
    g_free(item->command);
    g_free(item->caption);
    g_free(item->tooltip);
    g_free(item->icon_path);

    g_free(item);
}

void item_set_theme(Item * item, const char *theme)
{
    GdkPixbuf *pb;
    
    if(item->icon_id <= EXTERN_ICON)
        return;

    pb = get_pixbuf_by_id(item->icon_id);

    if (item->type == MENUITEM)
	xfce_menubutton_set_pixbuf(XFCE_MENUBUTTON(item->button), pb);
    else
	xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(item->button), pb);
    
    g_object_unref(pb);
}

void item_apply_config(Item * item)
{
    GdkPixbuf *pb = NULL;

    if(item->icon_id == EXTERN_ICON && item->icon_path)
        pb = get_pixbuf_from_file(item->icon_path);
    else if (item->icon_id != STOCK_ICON)
        pb = get_pixbuf_by_id(item->icon_id);

    if (pb)
    {
	if (item->type == MENUITEM)
	    xfce_menubutton_set_pixbuf(XFCE_MENUBUTTON(item->button), pb);
	else
	    xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(item->button), pb);
	
	g_object_unref(pb);
    }
    
    if (item->type == MENUITEM)
    {
	xfce_menubutton_set_text(XFCE_MENUBUTTON(item->button), item->caption);
	menu_item_set_popup_size(item, settings.size);
    }

    if(item->tooltip && strlen(item->tooltip))
        add_tooltip(item->button, item->tooltip);
    else if(item->command && strlen(item->command))
        add_tooltip(item->button, item->command);
    else
        add_tooltip(item->button, _("Click mouse button 3 to change item"));
}

/*  Menu items  
 *  ----------
*/
Item *menu_item_new(PanelPopup * pp)
{
    Item *mi = g_new0(Item, 1);

    mi->type = MENUITEM;

    mi->parent = pp;

    return mi;
}

void create_addtomenu_item(Item * mi)
{
    mi->button = xfce_menubutton_new_with_stock_icon(_("Add launcher"), GTK_STOCK_ADD);
    gtk_widget_show(mi->button);
    gtk_button_set_relief(GTK_BUTTON(mi->button), GTK_RELIEF_NONE);

    add_tooltip(mi->button, _("Add new item"));

    /* signals */
    dnd_set_drag_dest(mi->button);

    g_signal_connect(mi->button, "drag_data_received",
                     G_CALLBACK(addtomenu_item_drop_cb), mi->parent);

    g_signal_connect(mi->button, "clicked", G_CALLBACK(addtomenu_item_click_cb),
                     mi->parent);

    menu_item_set_popup_size(mi, settings.size);
}

void create_menu_item(Item * mi)
{
    mi->button = xfce_menubutton_new(NULL);
    gtk_widget_show(mi->button);
    gtk_button_set_relief(GTK_BUTTON(mi->button), GTK_RELIEF_NONE);

    item_apply_config(mi);
    
    /* signals */
    g_signal_connect(mi->button, "button-press-event",
                     G_CALLBACK(menu_item_press), mi);

    g_signal_connect(mi->button, "clicked", G_CALLBACK(item_click_cb), mi);

    dnd_set_drag_dest(mi->button);

    g_signal_connect(mi->button, "drag_data_received",
                     G_CALLBACK(item_drop_cb), mi);

    menu_item_set_popup_size(mi, settings.size);
}

void menu_item_set_popup_size(Item * mi, int size)
{
    int s = popup_icon_size[size];

    gtk_widget_set_size_request(mi->button, -1, s + border_width);
}

/*  Panel item
 *  ----------
*/
static Item *panel_item_new(void)
{
    Item *pi = g_new0(Item, 1);

    pi->type = PANELITEM;
    pi->button = xfce_iconbutton_new();
    gtk_widget_show(pi->button);
    gtk_button_set_relief(GTK_BUTTON(pi->button), GTK_RELIEF_NONE);

    item_apply_config(pi);

    g_signal_connect(pi->button, "clicked", G_CALLBACK(item_click_cb), pi);

    dnd_set_drag_dest(pi->button);
    g_signal_connect(pi->button, "drag-data-received", G_CALLBACK(item_drop_cb),
                     pi);

    return pi;
}

static void panel_item_free(Control * control)
{
    Item *pi = control->data;

    item_free(pi);
}

static void panel_item_set_theme(Control * control, const char *theme)
{
    Item *pi = control->data;

    item_apply_config(pi);
}

static void panel_item_read_config(Control * control, xmlNodePtr node)
{
    Item *pi = control->data;

    item_read_config(pi, node);
    
    item_apply_config(pi);
}

static void panel_item_write_config(Control * control, xmlNodePtr root)
{
    Item *pi = control->data;

    item_write_config(pi, root);
}

static void panel_item_attach_callback(Control *control, const char *signal,
				       GCallback callback, gpointer data)
{
    Item *pi = control->data;

    g_signal_connect(pi->button, signal, callback,data);
}

void create_panel_item(Control * control)
{
    Item *pi = panel_item_new();

    gtk_container_add(GTK_CONTAINER(control->base), pi->button);

    control->data = (gpointer) pi;
}

void panel_item_class_init(ControlClass *cc)
{
    cc->id = ICON;
    cc->name = "icon";
    cc->caption = _("Launcher");

    cc->create_control = (CreateControlFunc) create_panel_item;

    cc->free = panel_item_free;
    cc->read_config = panel_item_read_config;
    cc->write_config = panel_item_write_config;
    cc->attach_callback = panel_item_attach_callback;

    cc->add_options = panel_item_add_options;

    cc->set_theme = panel_item_set_theme;
}

