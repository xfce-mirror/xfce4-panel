/*  side.c
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

/*  Side panels
 *  -----------
 *  Side panels (left and right) consist of a collection of panel groups. A 
 *  panel group defines several containers to pack widgets into and to allow
 *  easy rearragement of these widgets.
 *  
 *  Two widgets are packed into a panel group: a panel control and a popup 
 *  button. The first group on each side contains a move handle instead of
 *  a popup button.
 *
 *  For easy access to the structure important data is stored in global 
 *  variable arrays, one for each side per major data structure.
*/

#include "side.h"

#include "xfce.h"
#include "xfce_support.h"
#include "popup.h"
#include "item.h"
#include "controls.h"
#include "callbacks.h"
#include "icons.h"

typedef struct _PanelGroup PanelGroup;

static GList *group_list = NULL;
static GtkBox *groupbox;

/*  Panel group
 *  -----------
*/
struct _PanelGroup
{
    int index;

    GtkWidget *base;            /* container to pack into panel */
    GtkWidget *box;             /* hbox or vbox */

    PanelPopup *popup;
    PanelControl *control;
};

void panel_group_arrange(PanelGroup * pg, int position)
{
    gboolean vertical = settings.orientation == VERTICAL;
    
    if(pg->box)
    {
	if (pg->popup)
	    panel_popup_unpack(pg->popup, GTK_CONTAINER(pg->box));
	if (pg->control)
	    panel_control_unpack(pg->control);

        gtk_widget_destroy(pg->box);
    }

    if(position == TOP || position == BOTTOM)
        pg->box = gtk_vbox_new(FALSE, 0);
    else
        pg->box = gtk_hbox_new(FALSE, 0);

    gtk_widget_show(pg->box);
    gtk_container_add(GTK_CONTAINER(pg->base), pg->box);

    if (position == RIGHT || position == BOTTOM)
    {
	if (pg->control)
	    panel_control_pack(pg->control, GTK_BOX(pg->box));
	if (pg->popup)
	    panel_popup_pack(pg->popup, GTK_BOX(pg->box));
    }
    else
    {
	if (pg->popup)
	    panel_popup_pack(pg->popup, GTK_BOX(pg->box));
	if (pg->control)
	    panel_control_pack(pg->control, GTK_BOX(pg->box));
    }
}

PanelGroup *create_panel_group(int index)
{
    PanelGroup *pg = g_new(PanelGroup, 1);

    pg->index = index;

    pg->base = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(pg->base);

    /* protect against destruction when unpacking */
    g_object_ref(pg->base);
    gtk_object_sink(GTK_OBJECT(pg->base));

    pg->box = NULL;
    pg->popup = NULL;
    pg->control = NULL;

    return pg;
}

void panel_group_pack(PanelGroup * pg, GtkBox * hbox)
{
    gtk_box_pack_start(hbox, pg->base, TRUE, TRUE, 0);
}

void panel_group_unpack(PanelGroup * pg, GtkContainer *container)
{
    gtk_container_remove(container, pg->base);
}

/*  Side panel
 *  ----------
*/
void side_panel_init(int side, GtkBox * box)
{
    int i = (side == LEFT) ? 0 : 
		settings.central_index >= 0 ? settings.central_index : 0;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    PanelGroup *group;

    groupbox = box;

    for(; i < num; i++)
    {
	group = create_panel_group(i);
	panel_group_pack(group, box);

	group_list = g_list_append(group_list, group);

	group->popup = create_panel_popup();

	/* we create an empty control, because we don't know what to put
	 * here until after we read the configuration file */
	group->control = panel_control_new(i);

	panel_group_arrange(group, settings.popup_position);
    }
}

void side_panel_pack(int side, GtkBox *box)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    groupbox = box;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;
	
        if(group)
	{
	    panel_group_pack(group, box);
	    panel_group_arrange(group, settings.popup_position);
	}
    }
}

void side_panel_unpack(int side)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;
	
        panel_group_unpack(group, GTK_CONTAINER(groupbox));
    }
}

void side_panel_register_control(PanelControl * pc)
{
    PanelGroup *group;

    group = g_list_nth(group_list, pc->index)->data;

    group->control = pc;

    panel_group_arrange(group, settings.popup_position);
}

void side_panel_cleanup(int side)
{
    GList *li;
    PanelGroup *group;

    /* ok, here I cheat. I just clean all groups when side == RIGHT */
    if (side == LEFT)
	return;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_popup_free(group->popup);
	panel_control_free(group->control);

	g_object_unref(group->base);
	g_free(group);
    }

    g_list_free(group_list);
    group_list = NULL;
}

void side_panel_set_size(int side, int size)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;

	panel_popup_set_size(group->popup, size);

        panel_control_set_size(group->control, size);
    }
}

void side_panel_set_popup_position(int side, int position)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;
	
        panel_group_arrange(group, position);

        panel_popup_set_popup_position(group->popup, position);
    }
}

void side_panel_set_on_top(int side, gboolean on_top)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;

	panel_popup_set_on_top(group->popup, on_top);
    }
}

void side_panel_set_style(int side, int style)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;
        
	panel_popup_set_style(group->popup, style);

        panel_control_set_style(group->control, style);
    }
}

void side_panel_set_theme(int side, const char *theme)
{
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;
        
	panel_popup_set_theme(group->popup, theme);

        panel_control_set_theme(group->control, theme);
    }
}

void side_panel_move(int from, int to)
{
    int i;
    GList *li;
    PanelGroup *group;

    if (from < 0 || from >= settings.num_groups)
	return;
    
    li = g_list_nth(group_list, from);
    group = li->data;

    /* FIXME: This isn't right 
     * We need to incorporate the desktop switcher in the group list */
    if (to >= settings.central_index && settings.show_central)
	gtk_box_reorder_child(groupbox, group->base, to + 1);
    else
	gtk_box_reorder_child(groupbox, group->base, to);

    group_list = g_list_delete_link(group_list, li);
    group_list = g_list_insert(group_list, group, to);

    for (i = 0, li = group_list; li; i++, li = li->next)
    {
	group = li->data;

	group->index = group->control->index = i;
    }
}

void side_panel_set_from_xml(int side, xmlNodePtr node)
{
    xmlNodePtr child;
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    /* children are "Group" nodes */
    if(node)
        node = node->children;

    for(i = 0; i < num && li; i++, li = li->next)
    {
        gboolean control_created = FALSE;

	group = li->data;
	
        if(node)
        {
            for(child = node->children; child; child = child->next)
            {
                /* create popup items and panel control */
                if(xmlStrEqual(child->name, (const xmlChar *)"Popup"))
                {
                    panel_popup_set_from_xml(group->popup, child);
                }
                else if(xmlStrEqual(child->name, (const xmlChar *)"Control"))
                {
                    panel_control_set_from_xml(group->control, child);
                    control_created = TRUE;
                }
            }
        }

        if(!control_created)
            panel_control_set_from_xml(group->control, NULL);

        if(node)
            node = node->next;
    }
}

void side_panel_set_num_groups(int n)
{
    int i;
    GList *li;
    PanelGroup *group;

    for(i = 0, li = group_list; i < n ||li; i++)
    {
	group = li ? li->data : NULL;
	
        if(i < n)
        {
            if(!group)
            {
                group = create_panel_group(i);
                panel_group_pack(group, groupbox);

		group_list = g_list_append(group_list, group);

                group->popup = create_panel_popup();

                group->control = panel_control_new(i);
                create_panel_control(group->control);

		panel_group_arrange(group, settings.popup_position);
            }

	    gtk_widget_show(group->base);
        }
        else if(group)
        {
            gtk_widget_hide(group->base);
        }

	if (li)
	    li = li->next;
    }
}

void side_panel_write_xml(int side, xmlNodePtr root)
{
    xmlNodePtr node, child;
    int i;
    int num = (side == RIGHT) ? settings.num_groups :
		settings.central_index >= 0 ? settings.central_index : 0;
    GList *li;
    PanelGroup *group;

    if (side == RIGHT && settings.central_index >= 0)
	li = g_list_nth(group_list, settings.central_index);
    else
	li = group_list;
    
    node = xmlNewTextChild(root, NULL, side == LEFT ? "Left" : "Right", NULL);

    for(i = 0; i < num && li; i++, li = li->next)
    {
	group = li->data;

        child = xmlNewTextChild(node, NULL, "Group", NULL);

        panel_popup_write_xml(group->popup, child);

        panel_control_write_xml(group->control, child);
    }
}

