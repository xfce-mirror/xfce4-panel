/*  controls.h
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

#ifndef __XFCE_CONTROLS_H__
#define __XFCE_CONTROLS_H__

#include <gmodule.h>

#include "global.h"

struct _PanelControl
{
    /* info */
    int index;
    int id;
    char *filename;
    char *dirname;
    GModule *gmodule;

    /* base widget (supplied by xfce) */
    GtkWidget *base;
    
    /* module data */
    char *caption;
    gpointer data;
    gboolean with_popup;

    /* module interface */
    void (*free)(PanelControl *pc);
    void (*read_config)(PanelControl *pc, xmlNodePtr node);
    void (*write_config)(PanelControl *pc, xmlNodePtr node);
    void (*attach_callback)(PanelControl *pc, const char *signal, 
	    		    GCallback callback, gpointer data);

    void (*add_options)(PanelControl *pc, GtkContainer *container,
	    		GtkWidget *revert, GtkWidget *done);

    /* global preferences */
    void (*set_orientation)(PanelControl *pc, int orientation);
    void (*set_size)(PanelControl *pc, int size);
    void (*set_style)(PanelControl *pc, int style);
    void (*set_theme)(PanelControl *pc, const char *theme);
};

PanelControl *panel_control_new(int index);

void create_panel_control(PanelControl * pc);

void panel_control_set_from_xml(PanelControl * pc, xmlNodePtr node);

void panel_control_write_xml(PanelControl * pc, xmlNodePtr parent);

void panel_control_free(PanelControl * pc);

void panel_control_pack(PanelControl * pc, GtkBox * box);

void panel_control_unpack(PanelControl * pc);

/* config dialog */
void panel_control_add_options(PanelControl * pc, GtkContainer * container,
                               GtkWidget * revert, GtkWidget * done);

/* settings */
void panel_control_set_settings(PanelControl *pc);

void panel_control_set_orientation(PanelControl * pc, int orientation);

void panel_control_set_size(PanelControl * pc, int size);

void panel_control_set_style(PanelControl * pc, int style);

void panel_control_set_theme(PanelControl * pc, const char *theme);

#endif /* __XFCE_CONTROLS_H__ */
