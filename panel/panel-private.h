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

#ifndef _PANEL_PRIVATE_H
#define _PANEL_PRIVATE_H

#include <gtk/gtkwidget.h>
#include <libxfce4panel/xfce-panel-enums.h>

#include "panel-app.h"

#define DEFAULT_SIZE             48
#define DEFAULT_MONITOR          0
#define DEFAULT_SCREEN_POSITION  XFCE_SCREEN_POSITION_NONE
#define DEFAULT_XOFFSET          0
#define DEFAULT_YOFFSET          0
#define DEFAULT_AUTOHIDE         FALSE
#define DEFAULT_FULL_WIDTH       FALSE
#define DEFAULT_TRANSPARENCY     20

#define PANEL_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), PANEL_TYPE_PANEL, PanelPrivate))

typedef struct _PanelPrivate PanelPrivate;

struct _PanelPrivate
{
    GtkWidget *itembar;
    GtkWidget *menu;

    int size;
    int monitor;
    XfceScreenPosition screen_position;
    int xoffset;
    int yoffset;
    guint autohide:1;
    guint full_width:1;
    int transparency;

    guint opacity;

    guint hidden:1;
    int block_autohide;
    int hide_timeout;
    int unhide_timeout;

    GtkWidget *drag_widget;

    gboolean edit_mode;
};

#endif /* _PANEL_PRIVATE_H */

