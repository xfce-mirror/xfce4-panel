/*  $Id$
 *  
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#ifndef __XFCE_GROUPS_H__
#define __XFCE_GROUPS_H__

#include <gmodule.h>
#include <panel/global.h>

/* general */
G_MODULE_IMPORT void groups_init (GtkBox * box);
G_MODULE_IMPORT void groups_cleanup (void);

G_MODULE_IMPORT void groups_pack (GtkBox * box);
G_MODULE_IMPORT void groups_unpack (void);

/* configuration */
G_MODULE_IMPORT void old_groups_set_from_xml (int side, xmlNodePtr node);
G_MODULE_IMPORT void groups_set_from_xml (xmlNodePtr node);
G_MODULE_IMPORT void groups_write_xml (xmlNodePtr root);

/* settings */
G_MODULE_IMPORT void groups_set_orientation (int orientation);
G_MODULE_IMPORT void groups_set_layer (int layer);

G_MODULE_IMPORT void groups_set_size (int size);
G_MODULE_IMPORT void groups_set_popup_position (int position);
G_MODULE_IMPORT void groups_set_theme (const char *theme);

/* arrow direction */
G_MODULE_IMPORT void groups_set_arrow_direction (GtkArrowType type);
G_MODULE_IMPORT GtkArrowType groups_get_arrow_direction (void);

/* find or act on specific group */
G_MODULE_IMPORT Control *groups_get_control (int index);
G_MODULE_IMPORT PanelPopup *groups_get_popup (int index);

G_MODULE_IMPORT void groups_move (int from, int to);
G_MODULE_IMPORT void groups_remove (int index);
G_MODULE_IMPORT void groups_show_popup (int index, gboolean show);
G_MODULE_IMPORT void groups_add_control (Control * control, int index);

G_MODULE_IMPORT int groups_get_n_controls (void);

#endif /* __XFCE_GROUPS_H__ */
