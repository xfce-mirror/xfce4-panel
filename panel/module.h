/*  module.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#ifndef __XFCE_MODULE_H__
#define __XFCE_MODULE_H__

#include <gmodule.h>
#include "global.h"

enum
{
    EXTERN_MODULE = -1,
    CLOCK_MODULE,
    TRASH_MODULE,
    BUILTIN_MODULES
};

struct _PanelModule
{
    PanelGroup *parent;

    int id;
    char *name;                 /* if id==EXTERN_MODULE */
    char *dir;
    GModule *gmodule;           /* for PLUGIN */

    /* module interface */
    char *caption;              /* name to show in the configuration dialog */
    GtkWidget *main;            /* widget to connect callback to */
    gpointer data;              /* usually the module structure */
    
    /* update display */
    int interval;		/* interval for the g_timeout */
    int timeout_id;
    gboolean (*update) (PanelModule *);

    /* add to and remove from panel */
    void (*pack) (PanelModule *, GtkBox *);
    void (*unpack) (PanelModule *, GtkContainer *);

    /* clean up */
    /* This should also destroy the gtk widgets */
    void (*free) (PanelModule *);

    /* settings */
    void (*set_size) (PanelModule *, int);
    void (*set_style) (PanelModule *, int);
    void (*set_icon_theme) (PanelModule *, const char *);

    /* configure */
	void (*add_options)(PanelModule *, GtkContainer *);
    void (*apply_configuration) (PanelModule *);
};

/*****************************************************************************/

PanelModule *panel_module_new(PanelGroup * pg);
gboolean create_panel_module(PanelModule * pm);
void panel_module_pack(PanelModule * pm, GtkBox * box);
void panel_module_unpack(PanelModule * pm, GtkContainer * container);
void panel_module_free(PanelModule * pm);

void panel_module_start(PanelModule * pm);
void panel_module_stop(PanelModule * pm);

void panel_module_set_size(PanelModule * pm, int size);
void panel_module_set_style(PanelModule * pm, int style);
void panel_module_set_icon_theme(PanelModule * pm, const char *theme);

void panel_module_add_options(PanelModule * pm, GtkContainer * container);
void panel_module_apply_configuration(PanelModule * pm);

void panel_module_parse_xml(xmlNodePtr node, PanelModule * pm);
void panel_module_write_xml(xmlNodePtr root, PanelModule *pm);

#endif /* __XFCE_MODULE_H__ */
