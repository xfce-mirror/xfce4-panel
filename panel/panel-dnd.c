/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include "panel-dnd.h"


/* Partly based on the example on
 * http://live.gnome.org/GnomeLove/DragNDropTutorial
 * Copyright (c) 2005 Ryan McDougall
 * Licensed under the GNU GPL
 */

static const GtkTargetEntry dest_target_list[] = 
{
    { (gchar *) "application/x-xfce-panel-plugin-name", 0, TARGET_PLUGIN_NAME },
    { (gchar *) "application/x-xfce-panel-plugin-widget", GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET }
};

static const GtkTargetEntry name_target_list[] = 
{
    { (gchar *) "application/x-xfce-panel-plugin-name", 0, TARGET_PLUGIN_NAME }
};

static const GtkTargetEntry widget_target_list[] = 
{
    { (gchar *) "application/x-xfce-panel-plugin-widget", GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET }
};

/* public API */

void 
panel_dnd_set_dest_name_and_widget (GtkWidget *widget)
{
    gtk_drag_dest_set (widget, 
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION,
                       dest_target_list, G_N_ELEMENTS (dest_target_list),
                       GDK_ACTION_MOVE | GDK_ACTION_COPY);
}

void 
panel_dnd_set_dest_widget (GtkWidget *widget)
{
    gtk_drag_dest_set (widget, 
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION,
                       widget_target_list, G_N_ELEMENTS (widget_target_list), GDK_ACTION_MOVE);
}

void 
panel_dnd_set_source_name (GtkWidget *widget)
{
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK, 
                         name_target_list, G_N_ELEMENTS (name_target_list), GDK_ACTION_COPY);
}

void 
panel_dnd_set_source_widget (GtkWidget *widget)
{
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK, 
                         widget_target_list, G_N_ELEMENTS (widget_target_list), GDK_ACTION_COPY);
}

void
panel_dnd_begin_drag (GtkWidget *widget)
{
    GtkTargetList *target_list ;
    GdkEvent      *event;
    
    event = gtk_get_current_event();
    if (G_LIKELY (event))
    {
        /* create a new target list */
        target_list = gtk_target_list_new (widget_target_list, G_N_ELEMENTS (widget_target_list));

        /* begin the drag */
        gtk_drag_begin (widget, target_list, GDK_ACTION_MOVE, 1, event);
    
        /* release the target list */
        gtk_target_list_unref (target_list);

        /* free the event */
        gdk_event_free (event);
    }
}
