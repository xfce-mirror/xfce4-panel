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

#ifndef __XFCE_GLOBAL_H__
#define __XFCE_GLOBAL_H__

#include <gmodule.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

/* why is this not in the standard C library ? */
#define strequal(s1,s2) !strcmp (s1, s2)
#define strnequal(s1,s2,n) !strncmp (s1, s2, n)

#define MAXSTRLEN 1024

/* files and directories */
#define HOMERCDIR  ".xfce4"
#define SYSRCDIR   "xfce4"
#define XFCERC     "xfce4rc"

#define PLUGINDIR  "panel-plugins"
#define THEMEDIR   "icons"

/* don't change default here, this is the fallback option
 * default config is set in the settings manager plugin */
#define DEFAULT_THEME "Curve"

/* typedefs */
typedef struct _Panel Panel;
typedef struct _ControlClass ControlClass;
typedef struct _Control Control;
typedef struct _PanelPopup PanelPopup;
typedef struct _Item Item;
typedef struct _Settings Settings;
typedef struct _Position Position;

/* panel sides / popup orientation */
enum
{ LEFT, RIGHT, TOP, BOTTOM };

/* panel orientation */
enum
{ HORIZONTAL, VERTICAL };

/* panel sizes */
enum
{ TINY, SMALL, MEDIUM, LARGE, PANEL_SIZES };

/* types for panel controls */
enum
{
    ICON = -2,			/* special case: the traditional laucher item */
    PLUGIN = -1,		/* external plugin */
    NUM_BUILTINS		/* no more builtins! yay! */
};

/* global settings */

struct _Settings
{
    int orientation;
    int layer;

    int size;
    int popup_position;

    int autohide;

    char *theme;

    int num_groups;
};

/* global variables, from globals.c
 *
 * FIXME: should be changed to use accessor functions 
 * for more flexibility (e.g. multple panels) */

G_MODULE_IMPORT gboolean disable_user_config;

G_MODULE_IMPORT Settings settings;
G_MODULE_IMPORT Panel panel;

G_MODULE_IMPORT int icon_size[PANEL_SIZES];

G_MODULE_IMPORT int border_width;

G_MODULE_IMPORT int top_height[PANEL_SIZES];

G_MODULE_IMPORT int popup_icon_size[PANEL_SIZES];


#endif /* __XFCE_GLOBAL_H__ */
