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
/* left and right versions of all important data are represented in a
 * single two dimensional array. This allows for the use of the enum
 * members LEFT (== 0) and RIGHT (== 1) as index.*/

static PanelGroup *groups[2][NBGROUPS];
static PanelPopup *popups[2][NBGROUPS];
static PanelControl *controls[2][NBGROUPS];
static GtkBox *boxes[2];

/*
static GPtrArray *groups[2];
static GPtrArray *popups[2];
static GPtrArray *controls[2];
static GtkBox *boxes[2];
*/

/*  Panel group
 *  -----------
*/
struct _PanelGroup
{
    int index;
    int side;

    GtkWidget *base;            /* container to pack into panel */
    GtkWidget *box;             /* hbox or vbox */
    GtkWidget *top;             /* container for popup button or move handle */
    GtkWidget *bottom;          /* container for panel control */
};

void panel_group_arrange(PanelGroup * pg, int position)
{
    gboolean vertical = settings.orientation == VERTICAL;
    
    if(pg->box)
    {
        gtk_container_remove(GTK_CONTAINER(pg->box), pg->top);
        gtk_container_remove(GTK_CONTAINER(pg->box), pg->bottom);

        /* removing the box will destroy it */
        gtk_container_remove(GTK_CONTAINER(pg->base), pg->box);
    }

    if(position == TOP || position == BOTTOM)
        pg->box = gtk_vbox_new(FALSE, 0);
    else
        pg->box = gtk_hbox_new(FALSE, 0);

    gtk_widget_show(pg->box);
    gtk_container_add(GTK_CONTAINER(pg->base), pg->box);

    /* find all cases for which we must use .._pack_end() */
    if (position == RIGHT || position == BOTTOM)
    {
	gtk_box_pack_end(GTK_BOX(pg->box), pg->top, FALSE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
    }
    else
    {
        gtk_box_pack_start(GTK_BOX(pg->box), pg->top, FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
    }
}

PanelGroup *create_panel_group(int side, int index)
{
    PanelGroup *pg = g_new(PanelGroup, 1);

    pg->index = index;
    pg->side = side;

    pg->base = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(pg->base);

    /* protect against destruction when unpacking */
    g_object_ref(pg->base);

    pg->top = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(pg->top);

    pg->bottom = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(pg->bottom);

    /* protect against destruction when removed from box */
    g_object_ref(pg->top);
    g_object_ref(pg->bottom);

    /* create box and arrange containers */
    pg->box = NULL;
    panel_group_arrange(pg, settings.popup_position);

    return pg;
}

void panel_group_pack(PanelGroup * pg, GtkBox * hbox)
{
    if(pg->side == LEFT)
        gtk_box_pack_start(hbox, pg->base, TRUE, TRUE, 0);
    else
        gtk_box_pack_end(hbox, pg->base, TRUE, TRUE, 0);
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
    int i;
    int num = (side == LEFT) ? settings.num_left : settings.num_right;

    boxes[side] = box;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(i < num)
        {
            groups[side][i] = create_panel_group(side, i);
	    panel_group_pack(groups[side][i], box);

	    popups[side][i] = create_panel_popup();
	    panel_popup_pack(popups[side][i],
			     GTK_CONTAINER(groups[side][i]->top));

            /* we create an empty control, because we don't know what to put
             * here until after we read the configuration file */
            controls[side][i] = panel_control_new(side, i);
	    panel_control_pack(controls[side][i],
			       GTK_CONTAINER(groups[side][i]->bottom));
        }
        else
        {
            groups[side][i] = NULL;
            popups[side][i] = NULL;
            controls[side][i] = NULL;
        }
    }
}

void side_panel_pack(int side, GtkBox *box)
{
    int i;
    int num = (side == LEFT) ? settings.num_left : settings.num_right;

    boxes[side] = box;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(groups[side][i])
	{
	    panel_group_pack(groups[side][i], box);
	    panel_group_arrange(groups[side][i], settings.popup_position);
	}
    }
}

void side_panel_unpack(int side)
{
    int i;
    int num = (side == LEFT) ? settings.num_left : settings.num_right;

    for(i = 0; i < num; i++)
    {
        panel_group_unpack(groups[side][i], GTK_CONTAINER(boxes[side]));
    }
}

void side_panel_register_control(PanelControl * pc)
{
    controls[pc->side][pc->index] = pc;
}

void side_panel_cleanup(int side)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_free(popups[side][i]);

        if(controls[side][i])
            panel_control_free(controls[side][i]);
    }
}

void side_panel_set_size(int side, int size)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
	if(popups[side][i])
            panel_popup_set_size(popups[side][i], size);

        if(controls[side][i])
            panel_control_set_size(controls[side][i], size);
    }
}

#if 0
void side_panel_set_popup_size(int side, int size)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_set_popup_size(popups[side][i], size);
    }
}
#endif

void side_panel_set_popup_position(int side, int position)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(groups[side][i])
            panel_group_arrange(groups[side][i], position);

        if(popups[side][i])
            panel_popup_set_popup_position(popups[side][i], position);
    }
}

void side_panel_set_on_top(int side, gboolean on_top)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_set_on_top(popups[side][i], on_top);
    }
}

void side_panel_set_style(int side, int style)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_set_style(popups[side][i], style);

        if(controls[side][i])
            panel_control_set_style(controls[side][i], style);
    }
}

void side_panel_set_theme(int side, const char *theme)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_set_theme(popups[side][i], theme);

        if(controls[side][i])
            panel_control_set_theme(controls[side][i], theme);
    }
}

void side_panel_set_from_xml(int side, xmlNodePtr node)
{
    xmlNodePtr child;
    int i;
    int num = (side == LEFT) ? settings.num_left : settings.num_right;

    /* children are "Group" nodes */
    if(node)
        node = node->children;

    for(i = 0; i < num; i++)
    {
        gboolean control_created = FALSE;

        if(node)
        {
            for(child = node->children; child; child = child->next)
            {
                /* create popup items and panel control */
                if(xmlStrEqual(child->name, (const xmlChar *)"Popup"))
                {
                    panel_popup_set_from_xml(popups[side][i], child);
                }
                else if(xmlStrEqual(child->name, (const xmlChar *)"Control"))
                {
                    panel_control_set_from_xml(controls[side][i], child);
                    control_created = TRUE;
                }
            }
        }

        if(!control_created)
            panel_control_set_from_xml(controls[side][i], NULL);

        if(node)
            node = node->next;
    }
}

void side_panel_set_num_groups(int side, int n)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(i < n)
        {
            if(!groups[side][i])
            {
                groups[side][i] = create_panel_group(side, i);
                panel_group_pack(groups[side][i], boxes[side]);

                popups[side][i] = create_panel_popup();
                panel_popup_pack(popups[side][i],
                                 GTK_CONTAINER(groups[side][i]->top));

                /* we create an empty control, because we don't know what to 
                 * put here until after we read the configuration file */
                controls[side][i] = panel_control_new(side, i);
                panel_control_pack(controls[side][i],
                                   GTK_CONTAINER(groups[side][i]->bottom));

                create_panel_control(controls[side][i]);
            }

            gtk_widget_show(groups[side][i]->base);
        }
        else if(groups[side][i])
        {
            gtk_widget_hide(groups[side][i]->base);
        }
    }
}

void side_panel_write_xml(int side, xmlNodePtr root)
{
    xmlNodePtr node, child;
    int i;
    int num = (side == LEFT) ? settings.num_left : settings.num_right;

    node = xmlNewTextChild(root, NULL, side == LEFT ? "Left" : "Right", NULL);

    for(i = 0; i < num; i++)
    {
        child = xmlNewTextChild(node, NULL, "Group", NULL);

        panel_popup_write_xml(popups[side][i], child);

        panel_control_write_xml(controls[side][i], child);
    }
}
