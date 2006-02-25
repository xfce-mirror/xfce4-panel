/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#ifndef _XFCE_PANEL_LAUNCHER_H
#define _XFCE_PANEL_LAUNCHER_H

#define MENU_TIMEOUT               250
#define BORDER                     8
#define W_ARROW                    16
#define MENU_ICON_SIZE             24
#define DLG_ICON_SIZE              32
#define PANEL_ICON_SIZE            48
#define MIN_ICON_SIZE              12 

/* A bit of a hack: Don't use icons without xfce- default */
#define NUM_CATEGORIES             (XFCE_N_BUILTIN_ICON_CATEGORIES - 4)

typedef enum    _LauncherIconType  LauncherIconType;
typedef struct  _LauncherIcon      LauncherIcon;
typedef struct  _LauncherEntry     LauncherEntry;
typedef struct  _LauncherPlugin    LauncherPlugin;

enum _LauncherIconType
{
    LAUNCHER_ICON_TYPE_NONE,
    LAUNCHER_ICON_TYPE_NAME,
    LAUNCHER_ICON_TYPE_CATEGORY
};

struct _LauncherIcon
{
    LauncherIconType type;
    union {
        XfceIconThemeCategory category;
        char *name;
    } icon;
};

struct _LauncherEntry
{
    char *name;
    char *comment;
    char *exec;
    char *real_exec;

    LauncherIcon icon; 

    guint terminal:1;
    guint startup:1;
};

struct _LauncherPlugin
{
    GPtrArray *entries;

    GtkWidget *plugin;
    GtkTooltips *tips;

    /* button + menu */
    GtkWidget *box;
    GtkWidget *arrowbutton;
    GtkWidget *iconbutton;
    GtkWidget *image;
    GtkWidget *menu;

    int popup_timeout;
    guint from_timeout:1;
};


/* launcher */
void launcher_update_panel_entry (LauncherPlugin *launcher);

void launcher_recreate_menu (LauncherPlugin *launcher);

void launcher_save (XfcePanelPlugin *plugin, LauncherPlugin *launcher);


/* entry */
LauncherEntry *launcher_entry_new (void);

void launcher_entry_free (LauncherEntry *entry);


/* icon */
GdkPixbuf * launcher_icon_load_pixbuf (GtkWidget *w, 
                                       LauncherIcon *icon, 
                                       int size);


/* DND */
void launcher_set_drag_dest (GtkWidget *widget);

GPtrArray *launcher_get_file_list_from_selection_data (GtkSelectionData *data);

#endif /* _XFCE_PANEL_LAUNCHER_H */
