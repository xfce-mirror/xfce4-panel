/*  global.h
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

#ifndef __XFCE_GLOBAL_H__
#define __XFCE_GLOBAL_H__

#include <string.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

/* Debug purpose */
#include "debug.h"

/* why is this not in the standard C library ? */
#define strequal(s1,s2) !strcmp (s1, s2)
#define strnequal(s1,s2,n) !strncmp (s1, s2, n)

#define MAXSTRLEN 1024

/* files and directories */
#define HOMERCDIR  ".xfce4"
#define SYSRCDIR   "xfce4"
#define XFCERC     "xfce4rc"

#define PLUGINDIR  "plugins"
#define THEMEDIR   "themes"

/* limits to the panel size */
#define NBSCREENS 12
#define NBGROUPS  16
#define NBITEMS   16

/* don't change default here, this is the fallback option
 * default config is set in the settings manager plugin */
#define DEFAULT_THEME "XFce"

/* panel sides / popup orientation */
enum
{ LEFT, RIGHT, TOP, BOTTOM };

/* panel orientation */
enum
{ HORIZONTAL, VERTICAL };

/* panel layer */
enum
{ ABOVE, NORMAL, BELOW };

/*  Panel sizes
 *  -----------
 *  `settings.size' is a symbolic size given by an enum. The actual sizes
 *  are put in an array so you can use the symbolic size as index, e.g. 
 *  icon_size[SMALL]. The sizes are defined in 'panel.c'.
*/
enum
{ TINY, SMALL, MEDIUM, LARGE, PANEL_SIZES };

extern int icon_size[PANEL_SIZES];

extern int border_width;

extern int top_height[PANEL_SIZES];

extern int popup_icon_size[PANEL_SIZES];

/* types for panel controls */
enum
{
    ICON = -2,   /* special case: the traditional laucher item */
    PLUGIN = -1, /* external plugin */
    NUM_BUILTINS /* no more builtins! yay! */
};

/* typedefs */
typedef struct _ControlClass ControlClass;
typedef struct _Control Control;
typedef struct _PanelPopup PanelPopup;
typedef struct _PanelItem PanelItem;
typedef struct _MenuItem MenuItem;

/* global settings */
typedef struct _Settings Settings;
typedef struct _Position Position;

struct _Position
{
    int x, y;
};

struct _Settings
{
    int orientation;
    int layer;

    int size;
    int popup_position;

    char *theme;

    int num_groups;
};

/* defined in settings.c */
extern gboolean disable_user_config;

/* defined in panel.c */
extern GtkWidget *toplevel;
extern Settings settings;
extern Position position;

#endif /* __XFCE_GLOBAL_H__ */
