/*  $Id$
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
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct _Tasklist Tasklist;

struct _Tasklist
{
    XfcePanelPlugin          *plugin;

    /* widgets */
    GtkWidget                *box;
    GtkWidget                *handle;
    GtkWidget                *list;

    /* signals */
    gint                      screen_changed_id;

    /* settings */
    gint                      width;
    NetkTasklistGroupingType  grouping;
    guint                     all_workspaces : 1;
    guint                     show_label : 1;
    guint                     expand : 1;
    guint                     flat_buttons : 1;
    guint                     show_handles : 1;
};

void        tasklist_write_rc_file           (Tasklist        *tasklist) G_GNUC_INTERNAL;
gboolean    tasklist_set_size                (Tasklist        *tasklist,
                                              gint             size)     G_GNUC_INTERNAL;
gboolean    tasklist_using_xinerama          (XfcePanelPlugin *plugin)   G_GNUC_INTERNAL;

#endif /* !__TASKLIST_H__ */
