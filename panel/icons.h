/*  icons.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

char *icon_names[NUM_ICONS];

/* system icons */
enum
{
    /* icons for the panel */
    MINILOCK_ICON,
    MINIINFO_ICON,
    MINIPALET_ICON,
    MINIPOWER_ICON,
    HANDLE_ICON,
    ADDICON_ICON,
    CLOSE_ICON,
    ICONIFY_ICON,
    UP_ICON,
    DOWN_ICON,
    /* program icons */
    DIAG_ICON,
    MENU_ICON,
    XFCE_ICON,
    SYS_ICONS
};

/* module icons */
enum
{
    /* thrash module icons */
    TRASH_EMPTY_ICON,
    TRASH_FULL_ICON,
    MODULE_ICONS
    /* ppp module icons 
    PPP_OFF_ICON,
    PPP_ON_ICON,
    PPP_CONNECTING_ICON,*/
};

#define UNKNOWN_ICON DEFAULT_ICON

void create_builtin_pixbufs(void);

GdkPixbuf *get_pixbuf_by_id(int id);
GdkPixbuf *get_pixbuf_from_file(const char *path);

GdkPixbuf *get_system_pixbuf(int id);

GdkPixbuf *get_module_pixbuf(int id);

GdkPixbuf *get_scaled_pixbuf(GdkPixbuf *pb, int size);

/* for plugins */
GdkPixbuf *get_themed_pixbuf(const char *name);

#endif /* __XFCE_ICONS_H__ */
