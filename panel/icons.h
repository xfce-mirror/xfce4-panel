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

#ifndef __XFCE_ICONS_H__
#define __XFCE_ICONS_H__

#include <gmodule.h>
#include <panel/global.h>

/* launcher and menu icons */
enum
{
    STOCK_ICON = -2,		/* used in popup menu */
    EXTERN_ICON = -1,
    /* general icons */
    DEFAULT_ICON,
    EDIT_ICON,
    FILE1_ICON,
    FILE2_ICON,
    GAMES_ICON,
    MAN_ICON,
    MULTIMEDIA_ICON,
    NETWORK_ICON,
    PAINT_ICON,
    PRINT_ICON,
    SCHEDULE_ICON,
    SOUND_ICON,
    TERMINAL_ICON,
    NUM_ICONS
};

G_MODULE_IMPORT const char *icon_names[NUM_ICONS];

#define UNKNOWN_ICON DEFAULT_ICON

G_MODULE_IMPORT void icons_init (void);

G_MODULE_IMPORT void icon_theme_init (void);

G_MODULE_IMPORT GdkPixbuf *get_pixbuf_by_id (int id);

G_MODULE_IMPORT GdkPixbuf *get_panel_pixbuf (void);

G_MODULE_IMPORT GdkPixbuf *get_pixbuf_from_file (const char *path);

G_MODULE_IMPORT GdkPixbuf *get_scaled_pixbuf (GdkPixbuf * pb, int size);

/* for plugins */
G_MODULE_IMPORT GdkPixbuf *get_themed_pixbuf (const char *name);

G_MODULE_IMPORT GdkPixbuf *themed_pixbuf_from_name_list (char **namelist, int size);

#endif /* __XFCE_ICONS_H__ */
