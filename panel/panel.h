/*  $Id$
 *
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifndef __XFCE_PANEL_H
#define __XFCE_PANEL_H

#include <gmodule.h>
#include <libxml/tree.h>
#include <panel/global.h>

typedef struct _PanelPrivate PanelPrivate;

typedef enum
{
    XFCE_POS_STATE_NONE,
    XFCE_POS_STATE_CENTER,
    XFCE_POS_STATE_START,
    XFCE_POS_STATE_END
}
XfcePositionState;

struct _Position
{
    int x, y;
};

struct _Panel
{
    gboolean hidden;
    int hide_timeout;
    int unhide_timeout;

    Position position;

    GtkWidget *toplevel;

    GtkWidget *group_box;

    PanelPrivate *priv;
};

/* panel functions */
G_MODULE_IMPORT void create_panel (void);
G_MODULE_IMPORT void panel_cleanup (void);

/* global settings */
G_MODULE_IMPORT void panel_set_settings (void);

G_MODULE_IMPORT void panel_center (int side);

G_MODULE_IMPORT void panel_set_orientation (int orientation);
G_MODULE_IMPORT void panel_set_layer (int layer);

G_MODULE_IMPORT void panel_set_size (int size);
G_MODULE_IMPORT void panel_set_popup_position (int position);
G_MODULE_IMPORT void panel_set_theme (const char *theme);

G_MODULE_IMPORT void panel_set_autohide (gboolean hide);

/* panel data */
G_MODULE_IMPORT void panel_parse_xml (xmlNodePtr node);
G_MODULE_IMPORT void panel_write_xml (xmlNodePtr root);

/* for menus, to prevent problems with autohide */
G_MODULE_IMPORT void panel_register_open_menu (GtkWidget * menu);

G_MODULE_IMPORT void panel_block_autohide (Panel * panel);

G_MODULE_IMPORT void panel_unblock_autohide (Panel * panel);

G_MODULE_IMPORT int panel_get_side (void);


#endif /* __XFCE_PANEL_H */
