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
#include "popup.h"
#include "item.h"
#include "controls.h"
#include "callbacks.h"
#include "move.h"
#include "icons.h"

typedef struct _PanelGroup PanelGroup;
typedef struct _MoveHandle MoveHandle;

/* left and right versions of all important data are represented in a
 * single two dimensional array. This allows for the use of the enum
 * members LEFT (== 0) and RIGHT (== 1) as index.*/

static PanelGroup *groups[2][NBGROUPS];
static PanelPopup *popups[2][NBGROUPS];
static PanelControl *controls[2][NBGROUPS];
static MoveHandle *handles[2];
static GtkBox *boxes[2];

/*
static GPtrArray *groups[2];
static GPtrArray *popups[2];
static GPtrArray *controls[2];
static MoveHandle *handles[2];
static GtkBox *boxes[2];
*/

/*  Move handle
 *  -----------
*/
struct _MoveHandle
{
    int side;

    GtkWidget *base;            /* provided by the panel group */
    GtkWidget *box;             /* depends on popup position */

/*    GdkPixbuf *iconify_pb;
    GtkWidget *iconify_image;*/
    GtkWidget *button;

/*    GdkPixbuf *handle_pb;
    GtkWidget *handle_image;*/
    GtkWidget *eventbox;
    GtkWidget *frame;
};

static void move_handle_arrange(MoveHandle * mh, int position)
{
    gboolean horizontal;

    if(mh->box)
    {
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->button);
        gtk_container_remove(GTK_CONTAINER(mh->box), mh->eventbox);

        /* removing the box will destroy it */
        gtk_container_remove(GTK_CONTAINER(mh->base), mh->box);
    }

    /* create new box */
    if(position == TOP || position == BOTTOM)
    {
        horizontal = TRUE;
        mh->box = gtk_hbox_new(FALSE, 0);
    }
    else
    {
        horizontal = FALSE;
        mh->box = gtk_vbox_new(FALSE, 0);
    }

    gtk_widget_show(mh->box);
    gtk_container_add(GTK_CONTAINER(mh->base), mh->box);

    if(horizontal && mh->side == RIGHT)
    {
        gtk_box_pack_end(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
    }
    else
    {
        gtk_box_pack_start(GTK_BOX(mh->box), mh->button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(mh->box), mh->eventbox, TRUE, TRUE, 0);
    }
}

move_handle_pack(MoveHandle * mh, GtkContainer * container)
{
    gtk_container_add(container, mh->base);
}

move_handle_unpack(MoveHandle * mh, GtkContainer * container)
{
    gtk_container_add(container, mh->base);
}

void move_handle_free(MoveHandle * mh)
{
    if(mh->button && GTK_IS_WIDGET(mh->button))
    {
        gtk_widget_destroy(mh->button);
        gtk_widget_destroy(mh->eventbox);
    }

    g_free(mh);
}

void move_handle_set_size(MoveHandle * mh, int size)
{
    int h = top_height[size];
    int w = icon_size[size] + border_width - h;
    int p = settings.popup_position;

    if(p == TOP || p == BOTTOM)
        gtk_widget_set_size_request(mh->eventbox, w + border_width, h);
    else
        gtk_widget_set_size_request(mh->eventbox, h, w);

    gtk_widget_set_size_request(mh->button, h, h);
}

void move_handle_set_style(MoveHandle * mh, int style)
{
    if(style == OLD_STYLE)
    {
        gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_OUT);
        gtk_widget_set_name(mh->frame, "gxfce_color1");

        gtk_widget_set_name(mh->eventbox, "gxfce_color1");

        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NORMAL);
        gtk_widget_set_name(mh->button, "gxfce_color1");
    }
    else
    {
        gtk_frame_set_shadow_type(GTK_FRAME(mh->frame), GTK_SHADOW_NONE);
        gtk_widget_set_name(mh->frame, "gxfce_color7");

        gtk_widget_set_name(mh->eventbox, "gxfce_color7");

        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
        gtk_widget_set_name(mh->button, "gxfce_color7");
    }
}

MoveHandle *create_move_handle(int side)
{
    GdkPixbuf *pb;
    GtkWidget *im;
    MoveHandle *mh = g_new(MoveHandle, 1);

    mh->side = side;

    mh->base = gtk_alignment_new(0, 0, 1, 1);
    gtk_widget_show(mh->base);

    /* protect against destruction when unpacking */
    g_object_ref(mh->base);

    mh->button = gtk_button_new();
    if(settings.style == NEW_STYLE)
        gtk_button_set_relief(GTK_BUTTON(mh->button), GTK_RELIEF_NONE);
    add_tooltip(mh->button, _("Iconify panel"));
    gtk_widget_show(mh->button);

    pb = get_system_pixbuf(ICONIFY_ICON);
    im = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(mh->button), im);

    if(settings.style == NEW_STYLE)
        gtk_widget_set_name(im, "gxfce_color1");
    else
        gtk_widget_set_name(im, "gxfce_color7");

    mh->eventbox = gtk_event_box_new();
    add_tooltip(mh->eventbox, _("Move panel"));
    gtk_widget_show(mh->eventbox);

    mh->frame = gtk_frame_new(NULL);
    gtk_widget_show(mh->frame);
    gtk_container_add(GTK_CONTAINER(mh->eventbox), mh->frame);

    pb = get_system_pixbuf(HANDLE_ICON);
    im = gtk_image_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(mh->frame), im);

    if(settings.style == NEW_STYLE)
        gtk_widget_set_name(im, "gxfce_color1");
    else
        gtk_widget_set_name(im, "gxfce_color7");

    /* protect against destruction when removed from box */
    g_object_ref(mh->button);
    g_object_ref(mh->eventbox);

    mh->box = NULL;

    move_handle_set_size(mh, settings.size);
    move_handle_set_style(mh, settings.style);
    move_handle_arrange(mh, settings.popup_position);

    /* signals */
    g_signal_connect(mh->button, "clicked", G_CALLBACK(iconify_cb), NULL);
    attach_move_callbacks(mh->eventbox);

    return mh;
}

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

    if(position == LEFT || position == TOP)
    {
        if(pg->index == 0 && position == LEFT && pg->side == RIGHT)
        {
            gtk_box_pack_end(GTK_BOX(pg->box), pg->top, FALSE, FALSE, 0);
            gtk_box_pack_end(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
        }
        else
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->top, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
        }
    }
    else
    {
        if(pg->index == 0 && position == RIGHT && pg->side == LEFT)
        {
            gtk_box_pack_start(GTK_BOX(pg->box), pg->top, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
        }
        else
        {
            gtk_box_pack_end(GTK_BOX(pg->box), pg->top, TRUE, TRUE, 0);
            gtk_box_pack_end(GTK_BOX(pg->box), pg->bottom, TRUE, TRUE, 0);
        }
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

            if(i == 0)
            {
                handles[side] = create_move_handle(side);
		move_handle_pack(handles[side],
				 GTK_CONTAINER(groups[side][i]->top));
                /* Theoretically the popups array can be 1 item shorter
                   than the others, because this on will always be NULL.
                   However, that would be really confusing. */
                popups[side][i] = NULL;
            }
            else
            {
                popups[side][i] = create_panel_popup();
		panel_popup_pack(popups[side][i],
				 GTK_CONTAINER(groups[side][i]->top));
            }

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

    for(i = 0; i < num; i++)
    {
        panel_group_pack(groups[side][i], box);
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
        if(i == 0)
            move_handle_free(handles[side]);
        else if(popups[side][i])
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
        if(i == 0)
            move_handle_set_size(handles[side], size);
        else if(popups[side][i])
            panel_popup_set_size(popups[side][i], size);

        if(controls[side][i])
            panel_control_set_size(controls[side][i], size);
    }
}

void side_panel_set_popup_size(int side, int size)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(popups[side][i])
            panel_popup_set_popup_size(popups[side][i], size);
    }
}

void side_panel_set_popup_position(int side, int position)
{
    int i;

    for(i = 0; i < NBGROUPS; i++)
    {
        if(groups[side][i])
            panel_group_arrange(groups[side][i], position);

        if(i == 0)
            move_handle_arrange(handles[side], position);
        else if(popups[side][i])
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
        if(i == 0)
            move_handle_set_style(handles[side], style);
        else if(popups[side][i])
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
        if(i != 0 && popups[side][i])
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

        if(i != 0)
            panel_popup_write_xml(popups[side][i], child);

        panel_control_write_xml(controls[side][i], child);
    }
}
