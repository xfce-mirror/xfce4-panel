/*  $Id$
 *  
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

/**
 * Groups
 * ------
 * The panel consist of a collection of panel groups. 
 *
 * A PanelGroup contains two items: a panel control and a popup button. 
 *
 * groups_* functions are for general houskeeping and to perform actions on
 * all panel groups.
 *
 * panel_group_* funtions act on a single PanelGroup and handle things like
 * creation destruction and layout.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce.h"
#include "groups.h"
#include "popup.h"
#include "item-control.h"

/* defined in controls.c */
extern void control_class_unref (ControlClass * cclass);

static GSList *group_list = NULL;
static GtkArrowType popup_arrow_type = GTK_ARROW_UP;
static GtkBox *groupbox;

/*  Panel group
 *  -----------
*/
typedef struct _PanelGroup PanelGroup;

struct _PanelGroup
{
    int index;
    Control *control;
};

static PanelGroup *
create_panel_group (int index)
{
    PanelGroup *pg = g_new0 (PanelGroup, 1);
    pg->index = index;

    return pg;
}

static void
panel_group_free (PanelGroup * pg)
{
    control_free (pg->control);
    g_free (pg);
}

static void
panel_group_pack (PanelGroup * pg, GtkBox * hbox)
{
    gtk_box_pack_start (hbox, pg->control->base, TRUE, TRUE, 0);
}

static void
panel_group_unpack (PanelGroup * pg)
{
    gtk_container_remove (GTK_CONTAINER (pg->control->base->parent), 
                          pg->control->base);
}


/*  Groups
 *  ------
 *  Mainly housekeeping for the global list of panel groups
*/

G_MODULE_EXPORT /* EXPORT:groups_init */
void
groups_init (GtkBox * box)
{
    groupbox = box;

    /* we need this later on, so we may just as well initialize it here */
    control_class_list_init ();
}

G_MODULE_EXPORT /* EXPORT:groups_cleanup */
void
groups_cleanup (void)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_group_free (group);
    }

    control_class_list_cleanup ();

    g_slist_free (group_list);
    group_list = NULL;
}

G_MODULE_EXPORT /* EXPORT:groups_pack */
void
groups_pack (GtkBox * box)
{
    GSList *li;
    PanelGroup *group;

    groupbox = box;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_group_pack (group, box);
    }
}

G_MODULE_EXPORT /* EXPORT:groups_unpack */
void
groups_unpack (void)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	panel_group_unpack (group);
    }
}

/* configuration */

G_MODULE_EXPORT /* EXPORT:old_groups_set_from_xml */
void
old_groups_set_from_xml (int side, xmlNodePtr node)
{
    static int last_group = 0;
    xmlNodePtr child;
    int i;
    GSList *li;
    PanelGroup *group;

    if (side == LEFT)
	last_group = 0;

    li = g_slist_nth (group_list, last_group);

    /* children are "Group" nodes */
    if (node)
	node = node->children;

    for (i = last_group; /*i < settings.num_groups || */ li || node; i++)
    {
	gboolean control_created = FALSE;

	if (side == LEFT && !node)
	    break;

	if (li)
	{
	    group = li->data;
	}
	else
	{
	    group = create_panel_group (i);
	    group_list = g_slist_append (group_list, group);

	    group->control = control_new (i);
	    panel_group_pack (group, groupbox);
	}

	if (node)
	{
            xmlNodePtr popup_node = NULL;
            
	    for (child = node->children; child; child = child->next)
	    {
		/* create popup items and panel control */
		if (xmlStrEqual (child->name, (const xmlChar *) "Popup"))
		{
		    popup_node = child;
		}
		else if (xmlStrEqual
			 (child->name, (const xmlChar *) "Control"))
		{
		    control_set_from_xml (group->control, child);
		    control_created = TRUE;
		}
	    }

            if (control_created && popup_node)
                item_control_add_popup_from_xml (group->control, popup_node);
	}

	if (!control_created)
	    control_set_from_xml (group->control, NULL);

	if (node)
	    node = node->next;

	if (li)
	    li = li->next;

	last_group++;
    }
}

G_MODULE_EXPORT /* EXPORT:groups_set_from_xml */
void
groups_set_from_xml (xmlNodePtr node)
{
    int i;

    /* children are "Group" nodes */
    if (node)
	node = node->children;

    for (i = 0; node; i++, node = node->next)
    {
	gboolean control_created = FALSE;
	xmlNodePtr child, popup_node = NULL;
	PanelGroup *group;

	group = create_panel_group (i);
	group->control = control_new (i);
	panel_group_pack (group, groupbox);
	group_list = g_slist_append (group_list, group);

	for (child = node->children; child; child = child->next)
	{
	    /* create popup items and panel control */
	    if (xmlStrEqual (child->name, (const xmlChar *) "Popup"))
	    {
		popup_node = child;
	    }
	    else if (xmlStrEqual (child->name, (const xmlChar *) "Control"))
	    {
		control_created =
		    control_set_from_xml (group->control, child);

                if (control_created && popup_node)
                    item_control_add_popup_from_xml (group->control, 
                                                     popup_node);
		break;
	    }
	}

	if (!control_created)
	{
	    group_list = g_slist_remove (group_list, group);
	    panel_group_unpack (group);
	    panel_group_free (group);
	}
    }
}

G_MODULE_EXPORT /* EXPORT:groups_write_xml */
void
groups_write_xml (xmlNodePtr root)
{
    xmlNodePtr node, child;
    GSList *li;
    PanelGroup *group;

    node = xmlNewTextChild (root, NULL, "Groups", NULL);

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	child = xmlNewTextChild (node, NULL, "Group", NULL);

        item_control_write_popup_xml (group->control, child);
	control_write_xml (group->control, child);
    }
}

/* settings */
G_MODULE_EXPORT /* EXPORT:groups_set_orientation */
void
groups_set_orientation (int orientation)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	control_set_orientation (group->control, orientation);
    }
}

G_MODULE_EXPORT /* EXPORT:groups_set_size */
void
groups_set_size (int size)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	control_set_size (group->control, size);
    }
}

G_MODULE_EXPORT /* EXPORT:groups_set_popup_position */
void
groups_set_popup_position (int position)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

        item_control_set_popup_position (group->control, position);
    }
}

G_MODULE_EXPORT /* EXPORT:groups_set_theme */
void
groups_set_theme (const char *theme)
{
    GSList *li;
    PanelGroup *group;

    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	control_set_theme (group->control, theme);
    }
}

/* arrow direction */

G_MODULE_EXPORT /* EXPORT:groups_set_arrow_direction */
void
groups_set_arrow_direction (GtkArrowType type)
{
    GSList *li;
    PanelGroup *group;

    popup_arrow_type = type;
    
    for (li = group_list; li; li = li->next)
    {
	group = li->data;

	item_control_set_arrow_direction (group->control, type);
    }
}

G_MODULE_EXPORT /* EXPORT:groups_get_arrow_direction */
GtkArrowType
groups_get_arrow_direction (void)
{
    return popup_arrow_type;
}

/* find or act on specific groups */

static PanelGroup *
groups_get_nth (int n)
{
    PanelGroup *group;
    GSList *li;
    int index, len;

    len = g_slist_length (group_list);

    index = CLAMP (n, 0, len);

    li = g_slist_nth (group_list, index);

    group = li->data;

    return group;
}

G_MODULE_EXPORT /* EXPORT:groups_get_control */
Control *
groups_get_control (int index)
{
    PanelGroup *group;

    group = groups_get_nth (index);
    return group->control;
}

G_MODULE_EXPORT /* EXPORT:groups_move */
void
groups_move (int from, int to)
{
    int i;
    GSList *li;
    PanelGroup *group;

    if (from < 0 || from >= g_slist_length (group_list))
	return;

    li = g_slist_nth (group_list, from);
    group = li->data;

    gtk_box_reorder_child (groupbox, group->control->base, to);

    group_list = g_slist_delete_link (group_list, li);
    group_list = g_slist_insert (group_list, group, to);

    for (i = 0, li = group_list; li; i++, li = li->next)
    {
	group = li->data;

	group->index = group->control->index = i;
    }
}

G_MODULE_EXPORT /* EXPORT:groups_remove */
void
groups_remove (int index)
{
    int i;
    GSList *li;
    PanelGroup *group;

    li = g_slist_nth (group_list, index);
    /* Paranoid, should not happen here */
    if (!li)
	return;

    group = li->data;

    DBG ("unref class %s", group->control->cclass->caption);
    control_class_unref (group->control->cclass);

    panel_group_unpack (group);

    group_list = g_slist_delete_link (group_list, li);
    panel_group_free (group);

    settings.num_groups--;

    for (i = 0, li = group_list; li; i++, li = li->next)
    {
	group = li->data;

	group->index = group->control->index = i;
    }
}

G_MODULE_EXPORT /* EXPORT:groups_add_control */
void
groups_add_control (Control * control, int index)
{
    int len;
    PanelGroup *group = NULL;

    len = g_slist_length (group_list);

    if (index < 0 || index > len)
	index = len;

    group = create_panel_group (index);
    group->control = control;
    control->index = index;
    panel_group_pack (group, groupbox);
    group_list = g_slist_append (group_list, group);

    if (index >= 0 && index < len)
	groups_move (len, index);

    if (control->with_popup)
    {
	item_control_show_popup (control, TRUE);
    }

    settings.num_groups++;
}

G_MODULE_EXPORT /* EXPORT:groups_get_n_controls */
int
groups_get_n_controls (void)
{
    return g_slist_length (group_list);
}
