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

#ifndef __XFCE_CONTROLS_H__
#define __XFCE_CONTROLS_H__

#include <gmodule.h>
#include <libxml/tree.h>

#include <panel/global.h>

typedef gboolean (*CreateControlFunc) (Control * control);

struct _ControlClass
{
    int id;
    const char *name;		/* unique name */
    const char *caption;	/* translated, human readable */

    /* for plugins */
    GModule *gmodule;
    char *filename;

    CreateControlFunc create_control;

    /* module interface */
    void (*free) (Control * control);
    void (*read_config) (Control * control, xmlNodePtr node);
    void (*write_config) (Control * control, xmlNodePtr node);
    void (*attach_callback) (Control * control, const char *signal,
			     GCallback callback, gpointer data);

    void (*create_options) (Control * control, GtkContainer * container,
			    GtkWidget * done);

    /* global preferences */
    void (*set_orientation) (Control * control, int orientation);
    void (*set_size) (Control * control, int size);
    void (*set_theme) (Control * control, const char *theme);

    /* open about dialog */
    void (*about) (void);
};

struct _Control
{
    ControlClass *cclass;

    /* base widget (supplied by xfce) */
    GtkWidget *base;

    int index;
    gpointer data;
    gboolean with_popup;
};

#define CONTROL_CLASS(cc) ((ControlClass*) cc)

/* for add-control-dialog.c */
typedef struct
{
    char *name;
    char *caption;
    GdkPixbuf *icon;
    gboolean can_be_added;	/* not unique or not already added */
}
ControlInfo;

G_MODULE_IMPORT GSList *get_control_info_list (void);

G_MODULE_IMPORT void insert_control (Panel * panel, const char *name, int position);

/* control classes */
G_MODULE_IMPORT void control_class_list_init (void);

G_MODULE_IMPORT void control_class_list_cleanup (void);

/* NOTE:
 * Use next functions only on classes that have at least 
 * their name field initialized */
G_MODULE_IMPORT void control_class_set_icon (ControlClass * cclass, GdkPixbuf * icon);

G_MODULE_IMPORT void control_class_set_unique (ControlClass * cclass, gboolean unique);

G_MODULE_IMPORT void control_class_set_unloadable (ControlClass * cclass,
				   gboolean unloadable);

/* controls */
G_MODULE_IMPORT Control *control_new (int index);

G_MODULE_IMPORT gboolean create_control (Control * control, int id, const char *filename);

G_MODULE_IMPORT void control_free (Control * control);

G_MODULE_IMPORT gboolean control_set_from_xml (Control * control, xmlNodePtr node);

G_MODULE_IMPORT void control_write_xml (Control * control, xmlNodePtr parent);

G_MODULE_IMPORT void control_pack (Control * control, GtkBox * box);

G_MODULE_IMPORT void control_unpack (Control * control);

G_MODULE_IMPORT void control_attach_callbacks (Control * control);

G_MODULE_IMPORT void control_create_options (Control * control, GtkContainer * container,
			     GtkWidget * done);

/* global settings */
G_MODULE_IMPORT void control_set_settings (Control * control);

G_MODULE_IMPORT void control_set_orientation (Control * control, int orientation);

G_MODULE_IMPORT void control_set_size (Control * control, int size);

G_MODULE_IMPORT void control_set_theme (Control * control, const char *theme);

#endif /* __XFCE_CONTROLS_H__ */
