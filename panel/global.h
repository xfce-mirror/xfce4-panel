/*  global.h
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

#ifndef __XFCE_GLOBAL_H__
#define __XFCE_GLOBAL_H__

#include <gtk/gtk.h>
#include <libxml/tree.h>
#include <string.h>

#include "settings.h"
#include "icons.h"

/* gettext */
#define _(x) x
#define N_(x) x

/* why is this not in the standard C library ? */
#define strequal(s1,s2) !strcmp (s1, s2)
#define strnequal(s1,s2,n) !strncmp (s1, s2, n)

#define MAXSTRLEN 1024

#define RCDIR		".xfce4"
#define RCFILE		"xfce4rc"

/* limits to the panel size */
#define NBSCREENS 12
#define NBGROUPS 16
#define NBITEMS 16

/* minibuttons */
#define MINI_SIZE 24

#define TINY_PANEL_ICONS 25
#define SMALL_PANEL_ICONS 30
#define MEDIUM_PANEL_ICONS 45
#define LARGE_PANEL_ICONS 60

#define SMALL_POPUP_ICONS 24
#define MEDIUM_POPUP_ICONS 32
#define LARGE_POPUP_ICONS 48

#define SCREEN_BUTTON_WIDTH 80

#define TOPHEIGHT 16

#define SMALL_TOPHEIGHT 14
#define MEDIUM_TOPHEIGHT TOPHEIGHT
#define LARGE_TOPHEIGHT TOPHEIGHT

/* panel sides */
enum
{ LEFT, RIGHT };

/* panel styles */
enum
{ OLD_STYLE, NEW_STYLE };

/* panel sizes */
enum
{ TINY, SMALL, MEDIUM, LARGE };

#define DEFAULT_SIZE MEDIUM

/* types of panel items */
enum
{ ICON, MODULE };

/* typedefs */
typedef struct _ScreenButton ScreenButton;
typedef struct _PanelGroup PanelGroup;
typedef struct _PanelPopup PanelPopup;
typedef struct _MenuItem MenuItem;
typedef struct _MoveHandle MoveHandle;
typedef struct _PanelItem PanelItem;
typedef struct _PanelModule PanelModule;

/* global settings */
typedef struct _Settings Settings;

struct _Settings
{
    int x, y;

    int size;
    int popup_size;

    int style;
    char *icon_theme;

    int num_left;
    int num_right;

    int num_screens;
    gboolean show_desktop_buttons;
    int current;

    char *lock_command;
    char *exit_command;
};

/* defined in settings.c */
extern gboolean disable_user_config;

/* defined in xfce.c */
extern GtkWidget *toplevel;
extern Settings settings;

#endif /* __XFCE_GLOBAL_H__ */
