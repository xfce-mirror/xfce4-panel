/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright (c) 2003 Andre Lerche <a.lerche@gmx.net>
 *  Copyright (c) 2003 Benedikt Meurer <benedikt.meurer@unix-ag.uni-siegen.de>
 *  Copyright (c) 2006 Jani Monoses <jani@ubuntu.com> 
 *  Copyright (c) 2006 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006 Nick Schermer <nick@xfce.org>
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

#define BORDER 8

#ifndef _WINDOWLIST_H
#define _WINDOWLIST_H

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <gtk/gtk.h>

typedef enum
{
    ICON_BUTTON,
    ARROW_BUTTON,
}
ButtonLayout;

typedef enum
{
    DISABLED,
    OTHER_WORKSPACES,
    ALL_WORKSPACES,
}
UrgencyNotify;

typedef struct
{
    XfcePanelPlugin *plugin;

    /* Widget stuff */
    GtkWidget	*button;
    GtkWidget	*icon;
    GtkArrowType arrowtype;
    GtkTooltips *tooltips;

    NetkScreen	*screen;
    guint	 screen_callback_id;
    
    /* Settings */
    ButtonLayout layout;
    
    gboolean show_all_workspaces;
    gboolean show_window_icons;
    gboolean show_workspace_actions;
    
    UrgencyNotify notify;
    
    /* Blink button stuff */
    guint search_timeout_id;
    guint blink_timeout_id;
    gboolean blink;
    gboolean block_blink;
}
Windowlist;

void
windowlist_start_blink (Windowlist * wl);

void
windowlist_create_button (Windowlist * wl);

#endif /* _WINDOWLIST_H */
