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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <xfce4/libxfcegui4/libxfcegui4.h>
#include <libxml/tree.h>
#include <string.h>

/* Debug purpose */
#include "debug.h"

/* gettext */
#define _(x) x
#define N_(x) x

/* why is this not in the standard C library ? */
#define strequal(s1,s2) !strcmp (s1, s2)
#define strnequal(s1,s2,n) !strncmp (s1, s2, n)

#define MAXSTRLEN 1024

/* files and directories */
#ifndef SYSCONFDIR
#define SYSCONFDIR "/usr/local/etc/"
#endif

#ifndef DATADIR
#define DATADIR    "/usr/local/share/xfce4"
#endif

#ifndef LIBDIR
#define LIBDIR    "/usr/local/lib/xfce4"
#endif

#define HOMERCDIR  ".xfce4"
#define SYSRCDIR   "xfce4"
#define XFCERC     "xfce4rc"

#define PLUGINDIR  "plugins"
#define THEMEDIR   "themes"

/* limits to the panel size */
#define NBSCREENS 12
#define NBGROUPS  16
#define NBITEMS   16

/* panel sides / popup orientation */
enum
{ LEFT, RIGHT, TOP, BOTTOM };

/* panel styles */
enum
{ OLD_STYLE, NEW_STYLE };

/* panel orientation */
enum
{ HORIZONTAL, VERTICAL };

/*  Panel sizes
 *  -----------
 *  settings.size is a symbolic size given by an enum. The actual sizes
 *  are put in an array so you can use the symbolic size as index,
 *  e.g. icon_size[SMALL], top_height[LARGE]
*/
enum
{ TINY, SMALL, MEDIUM, LARGE, PANEL_SIZES };

extern int minibutton_size[PANEL_SIZES];

extern int icon_size[PANEL_SIZES];

extern int border_width;

extern int popup_icon_size[PANEL_SIZES];

extern int top_height[PANEL_SIZES];

extern int screen_button_width[PANEL_SIZES];

/* panel controls */

enum
{
    ICON = -2,
    PLUGIN = -1,
    DUMMY_CLOCK, 
    DUMMY_TRASH,
    EXIT,
    CONFIG,
    NUM_BUILTINS
};

/* typedefs */
typedef struct _ScreenButton ScreenButton;
typedef struct _PanelControl PanelControl;
typedef struct _IconButton IconButton;
typedef struct _PanelItem PanelItem;
typedef struct _PanelPopup PanelPopup;
typedef struct _MenuItem MenuItem;

/* global settings */
typedef struct _Settings Settings;

struct _Settings
{
    int x, y;

    int orientation;
    gboolean on_top;

    int size;
    int popup_position;

    int style;
    char *theme;

    int num_left;
    int num_right;
    int num_screens;

    gboolean show_central;
    gboolean show_desktop_buttons;
    gboolean show_minibuttons;

    char *lock_command;
    char *exit_command;
};

/* defined in settings.c */
extern gboolean disable_user_config;

/* defined in xfce.c */
extern GtkWidget *toplevel;
extern Settings settings;

#endif /* __XFCE_GLOBAL_H__ */
