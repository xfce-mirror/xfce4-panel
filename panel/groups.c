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

/*  Groups
 *  ------
 *  The panel consist of a collection of panel groups. A panel group defines 
 *  several containers to pack widgets into and to allow easy rearragement of 
 *  these widgets.
 *  
 *  Two widgets are packed into a panel group: a panel control and a popup 
 *  button. 
*/

#include <config.h>
#include <my_gettext.h>

#include "xfce.h"
#include "groups.h"
#include "popup.h"
#include "callbacks.h"

typedef struct _PanelGroup PanelGroup;

static GList *group_list = NULL;
static GList *removed_list = NULL;

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
    Control *control;
};

static void panel_group_arrange(PanelGroup * pg)
{
    int position = settings.popup_position;
    
    if(pg->box)
    {
	if (pg->popup)
	    panel_popup_unpack(pg->popup);
	if (pg->control)
	    control_unpack(pg->control);

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
	    control_pack(pg->control, GTK_BOX(pg->box));
	if (pg->popup)
	    panel_popup_pack(pg->popup, GTK_BOX(pg->box));
    }
    else
    {
	if (pg->popup)
	    panel_popup_pack(pg->popup, GTK_BOX(pg->box));
	if (pg->control)
	    control_pack(pg->control, GTK_BOX(pg->box));
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

static void panel_group_free(PanelGroup *pg)
{
    panel_popup_free(pg->popup);
    control_free(pg->control);

    g_object_unref(pg->base);
    g_free(pg);
}

static void panel_group_pack(PanelGroup * pg, GtkBox * hbox)
{
    gtk_box_pack_start(hbox, pg->base, TRUE, TRUE, 0);
}

static void panel_group_unpack(PanelGroup * pg)
{
    gtk_container_remove(GTK_CONTAINER(pg->base->parent), pg->base);
}

/*  Groups
 *  ------
 *  Mainly housekeeping for the global list of panel groups
*/

/* FIXME: get rid of num_groups global option. 
 * Just add/remove items from config file and dialog. */
void groups_init(GtkBox * box)
{
    int i;
    PanelGroup *group;

    groupbox = box;

    control_class_list_init();

    for(i = 0; i < settings.num_groups; i++)
    {
	group = create_panel_group(i);
	panel_group_pack(group, box);

	group_list = g_list_append(group_list, group);

	group->popup = create_panel_popup();

	/* we create an empty control, because we don't know what to put
	 * here until after we read the configuration file */
	group->control = control_new(i);

	panel_group_arrange(group);
    }
}

void groups_cleanup(void)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_group_free(group);
    }

    control_class_list_cleanup();

    g_list_free(group_list);
    g_list_free(removed_list);
    group_list = NULL;
    removed_list = NULL;
}

void old_groups_set_from_xml(int side, xmlNodePtr node)
{
    static int last_group = 0;
    xmlNodePtr child;
    int i;
    GList *li;
    PanelGroup *group;

    if (side == LEFT)
	last_group = 0;

    li = g_list_nth(group_list, last_group);

    /* children are "Group" nodes */
    if(node)
        node = node->children;

    for(i = last_group; i < settings.num_groups && li; i++, li = li->next)
    {
        gboolean control_created = FALSE;

	group = li->data;
	
	if (side == LEFT && !node)
	    break;

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
                    control_set_from_xml(group->control, child);
                    control_created = TRUE;
                }
            }
        }

        if(!control_created)
            control_set_from_xml(group->control, NULL);

        if(node)
            node = node->next;

	last_group++;
    }
}

void groups_set_from_xml(xmlNodePtr node)
{
    xmlNodePtr child;
    int i;
    GList *li;
    PanelGroup *group;

    li = group_list;
    
    /* children are "Group" nodes */
    if(node)
        node = node->children;

    for(i = 0; i < settings.num_groups && li; i++, li = li->next)
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
                    control_set_from_xml(group->control, child);
                    control_created = TRUE;
                }
            }
        }

        if(!control_created)
            control_set_from_xml(group->control, NULL);

        if(node)
            node = node->next;
    }
}

void groups_write_xml(xmlNodePtr root)
{
    xmlNodePtr node, child;
    GList *li;
    PanelGroup *group;

    node = xmlNewTextChild(root, NULL, "Groups", NULL);

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

        child = xmlNewTextChild(node, NULL, "Group", NULL);

        panel_popup_write_xml(group->popup, child);
        control_write_xml(group->control, child);
    }
}

void groups_pack(GtkBox *box)
{
    GList *li;
    PanelGroup *group;

    groupbox = box;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;
	
	panel_group_pack(group, box);
	panel_group_arrange(group);
    }
}

void groups_unpack(void)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;
	
        panel_group_unpack(group);
    }
}

void groups_register_control(Control * control)
{
    PanelGroup *group;

    group = g_list_nth(group_list, control->index)->data;

    group->control = control;

    if (!control->with_popup) 
	groups_show_popup(control->index, FALSE);
    else
	groups_show_popup(control->index, TRUE);
    
    panel_group_arrange(group);
}

Control *groups_get_control(int index)
{
    GList *li;
    PanelGroup *group;
    int n, len;
    
    len = g_list_length(group_list);

    if (index < 0)
	n = 0;
    else if (index >= len)
	n = len -1;
    else
	n = index;
    
    li = g_list_nth(group_list, n);
    
    group = li->data;

    return group->control;
}

void groups_move(int from, int to)
{
    int i;
    GList *li;
    PanelGroup *group;

    if (from < 0 || from >= g_list_length(group_list))
	return;
    
    li = g_list_nth(group_list, from);
    group = li->data;

    gtk_box_reorder_child(groupbox, group->base, to);

    group_list = g_list_delete_link(group_list, li);
    group_list = g_list_insert(group_list, group, to);

    for (i = 0, li = group_list; li; i++, li = li->next)
    {
	group = li->data;

	group->index = group->control->index = i;
    }
}

void groups_remove(int index)
{
    int i;
    GList *li;
    PanelGroup *group;

    li = g_list_nth(group_list, index);
    group = li->data;

    panel_group_unpack(group);

    group_list = g_list_delete_link(group_list, li);

    panel_group_free(group);
    
    settings.num_groups--;

    for (i = 0, li = group_list; li; i++, li = li->next)
    {
	group = li->data;

	group->index = group->control->index = i;
    }
}

void groups_show_popup(int index, gboolean show)
{
    GList *li;
    PanelGroup *group;

    li = g_list_nth(group_list, index);
    group = li->data;

    if (show)
	gtk_widget_show(group->popup->button);
    else
	gtk_widget_hide(group->popup->button);
}

/* settings */
void groups_set_orientation(int orientation)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

	control_set_orientation(group->control, orientation);
    }
}

void groups_set_layer(int layer)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_popup_set_layer(group->popup, layer);
    }
}

void groups_set_size(int size)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_popup_set_size(group->popup, size);
        control_set_size(group->control, size);
    }
}

void groups_set_popup_position(int position)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;
	
        panel_group_arrange(group);
        panel_popup_set_popup_position(group->popup, position);
    }
}

void groups_set_style(int style)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;
        
	panel_popup_set_style(group->popup, style);
        control_set_style(group->control, style);
    }
}

void groups_set_theme(const char *theme)
{
    GList *li;
    PanelGroup *group;

    for(li = group_list; li; li = li->next)
    {
	group = li->data;
        
	panel_popup_set_theme(group->popup, theme);
        control_set_theme(group->control, theme);
    }
}

void groups_set_num_groups(int n)
{
    int i, len, max;
    GList *li;
    PanelGroup *group;

    len = g_list_length(group_list);
    max = (n > len) ? n : len;
    
    li = group_list;
    for(i = 0; i < max; i++)
    {
	group = li ? li->data : NULL;
	
        if(i < n)
        {
            if(!group)
            {
		if (removed_list)
		{
		    group = removed_list->data;
		    removed_list = g_list_delete_link(removed_list, removed_list);
		    
		    panel_group_pack(group, groupbox);
		    group_list = g_list_append(group_list, group);
		    
		    panel_group_arrange(group);
		}
		else
		{
		    group = create_panel_group(i);
		    panel_group_pack(group, groupbox);
		    group_list = g_list_append(group_list, group);

		    group->popup = create_panel_popup();

		    group->control = control_new(i);
		    create_control(group->control, ICON, NULL);

		    panel_group_arrange(group);
		}
            }

	    gtk_widget_show(group->base);
        }
        else if(group)
        {
	    GList *li2;
	    
            panel_group_unpack(group);

	    li2 = li;
	    li = li->prev;

	    group_list = g_list_remove_link(group_list, li2);
	    removed_list = g_list_prepend(removed_list, group);
        }

	if (li)
	    li = li->next;
    }
}

