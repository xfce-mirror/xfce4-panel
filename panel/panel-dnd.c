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
 * Copyright © 2005 Ryan McDougall
 * Licensed under the GNU GPL
 */

static const GtkTargetEntry dest_target_list[] =
{
    { "application/x-xfce-panel-plugin-name", 0, TARGET_PLUGIN_NAME },
    { "application/x-xfce-panel-plugin-widget", 0, TARGET_PLUGIN_WIDGET },
    { "text/plain", 0, TARGET_FILE },
    { "text/uri-list", 0, TARGET_FILE },
    { "UTF8_STRING", 0, TARGET_FILE }
};

static const GtkTargetEntry name_target_list[] =
{
    { "application/x-xfce-panel-plugin-name", 0, TARGET_PLUGIN_NAME }
};

static const GtkTargetEntry widget_target_list[] =
{
    { "application/x-xfce-panel-plugin-widget", 0, TARGET_PLUGIN_WIDGET }
};

/* public API */

void
panel_dnd_set_dest (GtkWidget *widget)
{
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION,
                       dest_target_list, G_N_ELEMENTS (dest_target_list), GDK_ACTION_COPY);
}

void
panel_dnd_set_widget_delete_dest (GtkWidget *widget)
{
    gtk_drag_dest_set (widget,
                       GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_MOTION,
                       widget_target_list, G_N_ELEMENTS (widget_target_list),
                       GDK_ACTION_MOVE);
}

void
panel_dnd_unset_dest (GtkWidget *widget)
{
    gtk_drag_dest_unset (widget);
}

GtkWidget *
panel_dnd_get_plugin_from_data (GtkSelectionData *data)
{
    glong *n;

    n = (glong *)data->data;
    DBG (" + get pointer: %ld", *n);

    return GTK_WIDGET (GINT_TO_POINTER (*n));
}

void
panel_dnd_set_name_source (GtkWidget *widget)
{
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK,
                         name_target_list, G_N_ELEMENTS (name_target_list),
                         GDK_ACTION_COPY);
}

void
panel_dnd_set_widget_source (GtkWidget *widget)
{
    gtk_drag_source_set (widget, GDK_BUTTON1_MASK,
                         widget_target_list, G_N_ELEMENTS (widget_target_list),
                         GDK_ACTION_COPY|GDK_ACTION_MOVE);
}

void panel_dnd_unset_source (GtkWidget *widget)
{
    gtk_drag_source_unset (widget);
}

void
panel_dnd_set_widget_data (GtkSelectionData *data, GtkWidget *widget)
{
    glong n = GPOINTER_TO_INT (widget);

    DBG (" + set pointer: %ld", n);

    gtk_selection_data_set (data, data->target, 32, (guchar *) &n, sizeof (n));
}

void
panel_dnd_begin_drag (GtkWidget *widget)
{
    static GtkTargetList *list = NULL;
    GdkEvent             *ev;

    if (G_UNLIKELY (list == NULL))
    {
        list = gtk_target_list_new (widget_target_list, G_N_ELEMENTS (widget_target_list));
    }

    ev = gtk_get_current_event();
    gtk_drag_begin (widget, list, GDK_ACTION_COPY, 1, ev);

    gdk_event_free (ev);
}
