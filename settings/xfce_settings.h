/*  xfce4
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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

#ifndef __XFCE_SETTINGS_H
#define __XFCE_SETTINGS_H

#define CHANNEL "xfce"

/* IMPORTANT: keep this in sync with mcs_client.c */
enum {
    XFCE_ORIENTATION,
    XFCE_LAYER,
    XFCE_SIZE,
    XFCE_POPUPPOSITION,
    XFCE_STYLE,
    XFCE_THEME,
    XFCE_POSITION,
    XFCE_OPTIONS
};

enum {
    XFCE_POSITION_BOTTOM,
    XFCE_POSITION_TOP,
    XFCE_POSITION_LEFT,
    XFCE_POSITION_RIGHT,
    XFCE_POSITION_SAVE,
    XFCE_POSITION_RESTORE,
    XFCE_POSITION_NONE,
};

static char *xfce_settings_names [] = {
    "orientation",
    "layer",
    "size",
    "popupposition",
    "style",
    "theme",
    "position"
};

#endif /* __XFCE_SETTINGS_H */

