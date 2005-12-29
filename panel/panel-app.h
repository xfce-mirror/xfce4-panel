/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _PANEL_APP_H
#define _PANEL_APP_H

#include <X11/Xlib.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef struct _XfceMonitor XfceMonitor;

struct _XfceMonitor
{
    GdkScreen *screen;
    int num;
    GdkRectangle geometry;
};

/* run control */

int panel_app_init (void);

int panel_app_run (int argc, char **argv);

void panel_app_queue_save (void);


/* actions */

void panel_app_customize (void);

void panel_app_customize_items (GtkWidget *active_item);

void panel_app_save (void);

void panel_app_restart (void);

void panel_app_quit (void);

void panel_app_quit_noconfirm (void);

void panel_app_quit_nosave (void);

void panel_app_add_panel (void);

void panel_app_remove_panel (GtkWidget *panel);

void panel_app_about (void);

Window panel_app_get_ipc_window (void);

XfceMonitor *panel_app_get_monitor (int n);

int panel_app_get_n_monitors (void);

gboolean panel_app_monitors_equal_height (void);

gboolean panel_app_monitors_equal_width (void);

/* keep track of open dialogs */
void panel_app_register_dialog (GtkWidget *dialog);

/* keep track of active panel */
void panel_app_set_current_panel (gpointer *panel);

void panel_app_unset_current_panel (gpointer *panel);

int panel_app_get_current_panel (void);

/* get panel list */
G_CONST_RETURN GPtrArray *panel_app_get_panel_list (void);


G_END_DECLS

#endif /* _PANEL_APP_H */
