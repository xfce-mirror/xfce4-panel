/*  icons.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
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

#include "global.h"

/* launcher and menu icons */
enum
{
    STOCK_ICON = -2,            /* used in popup menu */
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

const char *icon_names[NUM_ICONS];

/* system icons */
enum
{
    MINILOCK_ICON,
    MINIINFO_ICON,
    MINIPALET_ICON,
    MINIPOWER_ICON,
    MINIBUTTONS
};

enum
{
    DIAG_ICON,
    MENU_ICON,
    XFCE_ICON,
};

#define UNKNOWN_ICON DEFAULT_ICON

void icons_init (void);

GdkPixbuf *get_pixbuf_by_id (int id);
GdkPixbuf *get_minibutton_pixbuf (int id);
GdkPixbuf *get_system_pixbuf (int id);

GdkPixbuf *get_pixbuf_from_file (const char *path);

GdkPixbuf *get_scaled_pixbuf (GdkPixbuf * pb, int size);

/* for plugins */
GdkPixbuf *get_themed_pixbuf (const char *name);

#endif /* __XFCE_ICONS_H__ */
