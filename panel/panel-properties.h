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

#ifndef _PANEL_POSITION_H
#define _PANEL_POSITION_H

#include <libxfce4panel/xfce-panel-window.h>
#include <libxfce4panel/xfce-panel-enums.h>

#include "panel.h"

G_BEGIN_DECLS

/* properties */
int panel_get_size (Panel *panel);

void panel_set_size (Panel *panel, int size);


int panel_get_monitor (Panel *panel);

void panel_set_monitor (Panel *panel, int monitor);


XfceScreenPosition panel_get_screen_position (Panel *panel);

void panel_set_screen_position (Panel *panel, XfceScreenPosition position);


int panel_get_xoffset (Panel *panel);

void panel_set_xoffset (Panel *panel, int xoffset);


int panel_get_yoffset (Panel *panel);

void panel_set_yoffset (Panel *panel, int yoffset);


/* positioning */
void panel_init_position    (Panel *panel);

void panel_center           (Panel *panel);

void panel_set_autohide     (Panel *panel, 
                             gboolean autohide);

void panel_block_autohide   (Panel *panel);

void panel_unblock_autohide (Panel *panel);

void panel_set_full_width   (Panel *panel, 
                             gboolean fullwidth);

void panel_set_transparency (Panel *panel,
                             int transparency);

G_END_DECLS

#endif /* _PANEL_POSITION_H */

