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

/* control classes */
void control_class_list_init (void);

void control_class_list_cleanup (void);

/* NOTE:
 * Use next two function only on classes that have at least 
 * their name field initialized */
void control_class_set_icon (ControlClass *cclass, GdkPixbuf *icon);

void control_class_set_unique (ControlClass *cclass, gboolean unique);

/* add controls menu */
GtkWidget *get_controls_submenu (void);

/* controls */
Control *control_new (int index);

gboolean create_control (Control * control, int id, const char *filename);

void control_free (Control * control);

gboolean control_set_from_xml (Control * control, xmlNodePtr node);

void control_write_xml (Control * control, xmlNodePtr parent);

void control_pack (Control * control, GtkBox * box);

void control_unpack (Control * control);

void control_attach_callbacks (Control * control);

void control_create_options (Control * control, GtkContainer * container,
			     GtkWidget * done);

/* global settings */
void control_set_settings (Control * control);

void control_set_orientation (Control * control, int orientation);

void control_set_size (Control * control, int size);

void control_set_theme (Control * control, const char *theme);

#endif /* __XFCE_CONTROLS_H__ */
