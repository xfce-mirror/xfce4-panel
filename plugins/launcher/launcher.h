/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
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

#ifndef __XFCE_PANEL_LAUNCHER_H__
#define __XFCE_PANEL_LAUNCHER_H__

#include <gtk/gtk.h>
#include <exo/exo.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#define BORDER                     (8)
#define LAUNCHER_NEW_TOOLTIP_API   (GTK_CHECK_VERSION (2,11,6))
#define LAUNCHER_ARROW_SIZE        (16)
#define LAUNCHER_POPUP_DELAY       (225)
#define LAUNCHER_TOOLTIP_SIZE      (32)
#define LAUNCHER_MENU_SIZE         (24)
#define LAUNCHER_STARTUP_TIMEOUT   (30 * 1000)
#define LAUNCHER_TREE_ICON_SIZE    (24)
#define LAUNCHER_CHOOSER_ICON_SIZE (48)


/* frequently used code */
#define launcher_free_filenames(list) G_STMT_START{ \
    g_slist_foreach (list, (GFunc) g_free, NULL); \
    g_slist_free (list); \
    }G_STMT_END


typedef struct _LauncherEntry  LauncherEntry;
typedef struct _LauncherPlugin LauncherPlugin;

struct _LauncherEntry
{
    gchar    *name;
    gchar    *comment;
    gchar    *exec;
    gchar    *path;
    gchar    *icon;

    guint     terminal : 1;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
    guint     startup : 1;
#endif

#if LAUNCHER_NEW_TOOLTIP_API
    GdkPixbuf *tooltip_cache;
#endif
};

struct _LauncherPlugin
{
    /* panel plugin */
    XfcePanelPlugin *panel_plugin;

    /* whether saving is allowed */
    guint            plugin_can_save : 1;
    gint             image_size;

    /* list of launcher entries */
    GList           *entries;

    /* panel widgets */
    GtkWidget       *box;
    GtkWidget       *icon_button;
    GtkWidget       *arrow_button;
    GtkWidget       *image;
    GtkWidget       *menu;
#if !LAUNCHER_NEW_TOOLTIP_API
    GtkTooltips     *tips;
#endif

    /* event source ids */
    guint            popup_timeout_id;

    /* settings */
    guint            move_first : 1;
    guint            arrow_position;
};

enum
{
    LAUNCHER_ARROW_DEFAULT = 0,
    LAUNCHER_ARROW_LEFT,
    LAUNCHER_ARROW_RIGHT,
    LAUNCHER_ARROW_TOP,
    LAUNCHER_ARROW_BOTTOM,
    LAUNCHER_ARROW_INSIDE_BUTTON
};

/* target types for dropping in the launcher plugin */
static const GtkTargetEntry drop_targets[] =
{
    { (gchar *) "text/uri-list", 0, 0, },
    { (gchar *) "STRING",	       0, 0 },
    { (gchar *) "UTF8_STRING",   0, 0 },
    { (gchar *) "text/plain",    0, 0 },
};



GSList        *launcher_utility_filenames_from_selection_data (GtkSelectionData *selection_data) G_GNUC_MALLOC G_GNUC_INTERNAL;
GdkPixbuf     *launcher_utility_load_pixbuf                   (GdkScreen        *screen,
                                                               const gchar      *name,
                                                               guint             size) G_GNUC_MALLOC G_GNUC_INTERNAL;
LauncherEntry *launcher_entry_new                             (void) G_GNUC_MALLOC G_GNUC_INTERNAL;
void           launcher_entry_free                            (LauncherEntry    *entry,
                                                               LauncherPlugin   *launcher) G_GNUC_INTERNAL;
void           launcher_plugin_rebuild                        (LauncherPlugin   *launcher,
                                                               gboolean          update_icon) G_GNUC_INTERNAL;
void           launcher_plugin_read                           (LauncherPlugin   *launcher) G_GNUC_INTERNAL;
void           launcher_plugin_save                           (LauncherPlugin   *launcher) G_GNUC_INTERNAL;

#endif /* !__XFCE_PANEL_LAUNCHER_H__ */
