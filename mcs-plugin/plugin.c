/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2006 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <xfce-mcs-manager/manager-plugin.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>

static void run_dialog (McsPlugin * mcs_plugin);

/* plugin init */
McsPluginInitResult
mcs_plugin_init (McsPlugin * plugin)
{
    GtkIconTheme *icon_theme;

    /* setup i18n support */
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    /* initialize plugin structure */
    plugin->plugin_name = g_strdup ("xfce4panel");
    plugin->caption = g_strdup (_("Panel"));
    plugin->run_dialog = run_dialog;

    /* lookup the icon (on the default icon theme, *sigh*) */
    icon_theme = gtk_icon_theme_get_default ();
    plugin->icon = gtk_icon_theme_load_icon (icon_theme, "xfce4-panel", 48, 0, NULL);

    /* if that didn't work, we know where we installed the icon, so load it directly */
    if (G_UNLIKELY (plugin->icon == NULL))
        plugin->icon = gdk_pixbuf_new_from_file (DATADIR "/icons/hicolor/48x48/apps/xfce4-panel.png", NULL);

    /* attach icon name to the plugin icon (if any) */
    if (G_LIKELY (plugin->icon != NULL))
        g_object_set_data_full (G_OBJECT (plugin->icon), "mcs-plugin-icon-name", g_strdup ("xfce4-panel"), g_free);

    return MCS_PLUGIN_INIT_OK;
}

/* settings dialog */
static void
run_dialog (McsPlugin * mcs_plugin)
{
    g_spawn_command_line_async ("xfce4-panel -c", NULL);
}

