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
mcs_plugin_init (McsPlugin * mcs_plugin)
{
    /* 
       This is required for UTF-8 at least - Please don't remove it
       And it needs to be done here for the label to be properly
       localized....
     */
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    mcs_plugin->plugin_name = g_strdup ("xfce4panel");
    mcs_plugin->caption = g_strdup (_("Panel"));
    mcs_plugin->run_dialog = run_dialog;
    mcs_plugin->icon = xfce_themed_icon_load ("xfce4-panel", 48);

    return (MCS_PLUGIN_INIT_OK);
}

/* settings dialog */
static void
run_dialog (McsPlugin * mcs_plugin)
{
    g_spawn_command_line_async("xfce4-panel -c", NULL);
}

