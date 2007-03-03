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

#ifndef __PANEL_PRIVATE_H__
#define __PANEL_PRIVATE_H__

#include <gtk/gtkwidget.h>
#include <libxfce4panel/xfce-panel-enums.h>

#include "panel-app.h"

#define DEFAULT_SIZE             48
#define MIN_SIZE                 16
#define MAX_SIZE                 128
#define DEFAULT_MONITOR          0
#define DEFAULT_SCREEN_POSITION  XFCE_SCREEN_POSITION_NONE
#define DEFAULT_XOFFSET          0
#define DEFAULT_YOFFSET          0
#define DEFAULT_AUTOHIDE         FALSE
#define DEFAULT_FULL_WIDTH       XFCE_PANEL_NORMAL_WIDTH
#define DEFAULT_TRANSPARENCY     20
#define DEFAULT_ACTIVE_TRANS     FALSE

#define PANEL_GET_PRIVATE(o)     (PANEL(o)->priv)

typedef struct _PanelPrivate PanelPrivate;

typedef enum
{
    XFCE_PANEL_NORMAL_WIDTH,
    XFCE_PANEL_FULL_WIDTH,
    XFCE_PANEL_SPAN_MONITORS,
}
XfcePanelWidthType;

struct _PanelPrivate
{
    GtkWidget          *itembar;
    GtkWidget          *menu;
    GtkWidget          *drag_widget;

    gint                size;
    gint                monitor;
    XfceScreenPosition  screen_position;
    gint                xoffset;
    gint                yoffset;
    XfcePanelWidthType  full_width;
    gint                transparency;

    guint               opacity;
    guint               saved_opacity;

    gint                block_autohide;
    gint                hide_timeout;
    gint                unhide_timeout;

    /* booleans */
    guint               autohide : 1;
    guint               activetrans : 1;
    guint               hidden : 1;
    guint               edit_mode : 1;

    gulong              struts[12];
};

#endif /* !__PANEL_PRIVATE_H__ */
