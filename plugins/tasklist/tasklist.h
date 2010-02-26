/*  $Id: tasklist.h 26196 2007-10-25 18:23:36Z nick $
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __TASKLIST_H__
#define __TASKLIST_H__

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

typedef struct _TasklistPlugin TasklistPlugin;

struct _TasklistPlugin
{
    /* the panel plugin */
    XfcePanelPlugin          *panel_plugin;

    /* widgets */
    GtkWidget                *box;
    GtkWidget                *handle;
    GtkWidget                *list;

    /* signals */
    gint                      screen_changed_id;

    /* icon theme */
    GtkIconTheme             *icon_theme;

    /* requested width */
    gint                      req_size;

    /* settings */
    gint                      width;
    WnckTasklistGroupingType  grouping;
    guint                     all_workspaces : 1;
    guint                     show_label : 1;
    guint                     expand : 1;
    guint                     flat_buttons : 1;
    guint                     show_handles : 1;
};


void     tasklist_plugin_write   (TasklistPlugin  *tasklist) G_GNUC_INTERNAL;
gboolean tasklist_using_xinerama (XfcePanelPlugin *panel_plugin) G_GNUC_INTERNAL;

#endif /* !__TASKLIST_H__ */
