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

#ifndef __PANEL_DND_H__
#define __PANEL_DND_H__

#include <gtk/gtk.h>

enum 
{
    TARGET_PLUGIN_NAME,
    TARGET_PLUGIN_WIDGET
};

void panel_dnd_set_dest_name_and_widget (GtkWidget *widget);

void panel_dnd_set_dest_widget (GtkWidget *widget);

void panel_dnd_set_source_name (GtkWidget *widget);

void panel_dnd_set_source_widget (GtkWidget *widget);

void panel_dnd_begin_drag (GtkWidget *widget);

#endif /* !__PANEL_DND_H__ */
