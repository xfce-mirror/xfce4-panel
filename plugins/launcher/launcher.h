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

#define STARTUP_TIMEOUT   (30 * 1000)
#define ARROW_WIDTH       16
#define MENU_ICON_SIZE    24
#define MENU_POPUP_DELAY  225
#define BORDER            8
#define TREE_ICON_SIZE    24
#define CHOOSER_ICON_SIZE 48

#define g_free_null(mem)          g_free (mem); mem = NULL
#define g_slist_free_all(list)    g_slist_foreach (list, (GFunc) g_free, NULL); g_slist_free (list)

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
};

struct _LauncherPlugin
{
    /* globals */
    XfcePanelPlugin *plugin;
    GtkTooltips     *tips;
    GList           *entries;

    /* cpu saver */
    guint            icon_update_required : 1;

    /* panel widgets */
    GtkWidget       *arrowbutton;
    GtkWidget       *iconbutton;
    GtkWidget       *image;
    GtkWidget       *menu;

    /* global settings */
    guint            move_first : 1;

    /* timeouts */
    guint            popup_timeout_id;
    guint            theme_timeout_id;
};

/* target types for dropping in the launcher plugin */
static const GtkTargetEntry drop_targets[] =
{
    { "text/uri-list", 0, 0, },
};

void                     launcher_g_list_swap                (GList              *li_a,
                                                              GList              *li_b)           G_GNUC_INTERNAL;
GdkPixbuf               *launcher_load_pixbuf                (GtkWidget          *widget,
                                                              const gchar        *icon_name,
                                                              guint               size,
                                                              gboolean            fallback)       G_GNUC_INTERNAL G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void                     launcher_set_move_first             (gboolean            activate)       G_GNUC_INTERNAL;
GSList                  *launcher_file_list_from_selection   (GtkSelectionData   *selection_data) G_GNUC_INTERNAL G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean                 launcher_button_update              (LauncherPlugin     *launcher)       G_GNUC_INTERNAL;
gboolean                 launcher_menu_prepare               (LauncherPlugin     *launcher)       G_GNUC_INTERNAL;
void                     launcher_read                       (LauncherPlugin     *launcher)       G_GNUC_INTERNAL;
void                     launcher_write                      (LauncherPlugin     *launcher)       G_GNUC_INTERNAL;
void                     launcher_free_entry                 (LauncherEntry      *entry,
                                                              LauncherPlugin     *launcher)       G_GNUC_INTERNAL;
LauncherEntry           *launcher_new_entry                  (void)                               G_GNUC_INTERNAL G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

#endif /* !__XFCE_PANEL_LAUNCHER_H__ */
