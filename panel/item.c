/*  item.c
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

#include "item.h"

#include "item_dialog.h"
#include "controls.h"
#include "iconbutton.h"
#include "xfce.h"
#include "xfce_support.h"
#include "callbacks.h"
#include "popup.h"
#include "icons.h"
#include "settings.h"

/*  (re)apply the settings to the icon button
*/
void panel_item_apply_config(PanelItem * pi)
{
    GdkPixbuf *tmp;

    if(pi->icon_id == EXTERN_ICON)
        tmp = get_pixbuf_from_file(pi->icon_path);
    else
        tmp = get_pixbuf_by_id(pi->icon_id);

    icon_button_set_pixbuf(pi->button, tmp);
    g_object_unref(tmp);

    if(pi->tooltip)
        add_tooltip(icon_button_get_button(pi->button), pi->tooltip);
}

/*  global settings
*/
static void panel_item_set_size(PanelControl * pc, int size)
{
    PanelItem *pi = (PanelItem *) pc->data;

    icon_button_set_size(pi->button, size);
}

static void panel_item_set_theme(PanelControl * pc, const char *theme)
{
    PanelItem *pi = (PanelItem *) pc->data;

    panel_item_apply_config(pi);
}

/*  creation
*/
static PanelItem *panel_item_new(void)
{
    PanelItem *pi = g_new(PanelItem, 1);
    GtkWidget *w;

    pi->command = NULL;
    pi->in_terminal = FALSE;
    pi->tooltip = NULL;

    pi->icon_id = UNKNOWN_ICON;
    pi->icon_path = NULL;

    pi->button = icon_button_new(NULL);

    w = icon_button_get_button(pi->button);

    add_tooltip(w, _("Click Mouse 3 to change item"));

    g_signal_connect(w, "clicked", G_CALLBACK(panel_item_click_cb), pi);

    dnd_set_drag_dest(w);
    g_signal_connect(w, "drag-data-received", G_CALLBACK(panel_item_drop_cb),
                     pi);

    return pi;
}

static void panel_item_read_config(PanelControl * pc, xmlNodePtr node)
{
    PanelItem *pi = (PanelItem *) pc->data;
    xmlNodePtr child;
    xmlChar *value;

    if(!node)
    {
        panel_item_apply_config(pi);
        return;
    }

    for(child = node->children; child; child = child->next)
    {
        if(xmlStrEqual(child->name, (const xmlChar *)"Command"))
        {
            int n = -1;

            value = DATA(child);

            if(value)
                pi->command = (char *)value;

            value = xmlGetProp(child, "term");

            if(value)
                n = atoi(value);

            if(n == 1)
                pi->in_terminal = TRUE;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Tooltip"))
        {
            value = DATA(child);

            if(value)
                pi->tooltip = (char *)value;
        }
        else if(xmlStrEqual(child->name, (const xmlChar *)"Icon"))
        {
            value = xmlGetProp(child, (const xmlChar *)"id");

            if(value)
                pi->icon_id = atoi(value);

            if(!value || pi->icon_id < EXTERN_ICON || pi->icon_id >= NUM_ICONS)
                pi->icon_id = UNKNOWN_ICON;

            g_free(value);

            if(pi->icon_id == EXTERN_ICON)
            {
                value = DATA(child);

                if(value)
                    pi->icon_path = (char *)value;
                else
                    pi->icon_id = UNKNOWN_ICON;
            }
        }
    }

    panel_item_apply_config(pi);
}

/* destruction
*/
static void panel_item_free(PanelControl * pc)
{
    PanelItem *pi = (PanelItem *) pc->data;

    g_free(pi->command);
    g_free(pi->icon_path);
    g_free(pi->tooltip);

    icon_button_free(pi->button);

    g_free(pi);
}

/*  exit
*/
static void panel_item_write_config(PanelControl * pc, xmlNodePtr root)
{
    PanelItem *pi = (PanelItem *) pc->data;
    xmlNodePtr child;
    char value[3];

    child = xmlNewChild(root, NULL, "Command", pi->command);

    snprintf(value, 2, "%d", pi->in_terminal);
    xmlSetProp(child, "term", value);

    if(pi->tooltip)
        xmlNewTextChild(root, NULL, "Tooltip", pi->tooltip);

    child = xmlNewTextChild(root, NULL, "Icon", pi->icon_path);

    snprintf(value, 3, "%d", pi->icon_id);
    xmlSetProp(child, "id", value);
}

/*  create a default panel item 
*/
void create_panel_item(PanelControl * pc)
{
    PanelItem *pi = panel_item_new();
    GtkWidget *w = icon_button_get_button(pi->button);

    gtk_container_add(GTK_CONTAINER(pc->base), w);

    /* fill the PanelControl structure */
    pc->id = ICON;

    pc->caption = g_strdup(_("Icon"));
    pc->main = w;
    pc->data = (gpointer) pi;

    pc->read_config = panel_item_read_config;
    pc->write_config = panel_item_write_config;

    pc->free = panel_item_free;

    pc->set_size = panel_item_set_size;
    pc->set_theme = panel_item_set_theme;

    pc->add_options = panel_item_add_options;
}
