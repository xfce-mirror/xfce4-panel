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

#define INSENSITIVE_TIMEOUT     800
#define BORDER                  6
#define W_ARROW                 18
#define MENU_ICON_SIZE          24
#define DLG_ICON_SIZE           32
#define PANEL_ICON_SIZE         48
/* Don't use icons without xfce- default */
#define NUM_CATEGORIES          (XFCE_N_BUILTIN_ICON_CATEGORIES - 4)

typedef struct _Icon Icon;
typedef struct _Entry Entry;
typedef struct _Launcher Launcher;
typedef enum _IconType IconType;

enum _IconType
{
    ICON_TYPE_NONE,
    ICON_TYPE_NAME,
    ICON_TYPE_CATEGORY
};

struct _Icon
{
    IconType type;
    union 
    {
        XfceIconThemeCategory category;
        char *name;
    }
    icon;
};

struct _Entry
{
    char *name;
    char *comment;
    Icon icon;
    char *exec;

    guint terminal:1;
    guint startup:1;
};

struct _Launcher
{
    GtkTooltips *tips;

    Entry *entry;
    
    GList *items;

    GtkWidget *menu;

    GtkWidget *box;
    GtkWidget *arrowbutton;
    GtkWidget *iconbutton;
};


extern XfceIconTheme *launcher_theme;


/* prototypes */
GdkPixbuf *launcher_load_pixbuf (Icon *icon, int size);

char *file_uri_to_local (const char *uri);
    

Entry *entry_new (void);

void entry_free (Entry *entry);


void launcher_update_panel_entry (Launcher *launcher);

void launcher_recreate_menu (Launcher *launcher);


#endif /* _XFCE_PANEL_LAUNCHER_H */

