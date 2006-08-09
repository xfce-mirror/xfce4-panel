/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_PANEL_PLUGIN_H
#define _XFCE_PANEL_PLUGIN_H

#include <stdlib.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-plugin-iface.h>
#include <libxfce4panel/xfce-panel-internal-plugin.h> 
#include <libxfce4panel/xfce-panel-external-plugin.h>

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL
 * @construct : name of a function that can be cast to an #XfcePanelPluginFunc
 *
 * Registers and initializes the plugin. This is the only thing that is 
 * required to create a panel plugin.
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(construct) \
    int \
    main (int argc, char **argv) \
    { \
        GtkWidget *plugin; \
        gtk_init (&argc, &argv); \
        plugin = xfce_external_panel_plugin_new (argc, argv, \
                     (XfcePanelPluginFunc)construct); \
        if (!plugin) return 1; \
        g_signal_connect_after (plugin, "destroy", \
                                G_CALLBACK (exit), NULL); \
        gtk_widget_show (plugin); \
        gtk_main (); \
        return 0; \
    }

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK
 * @construct : name of a function that can be cast to an 
 *              #XfcePanelPluginFunc
 * @check     : name of a function that can be cast to an
 *              #XfcePanelPluginCheck
 *
 * Registers and initializes the plugin. This is the only thing that is 
 * required to create a panel plugin. The @check functions is run before
 * creating the plugin, and should return FALSE if plugin creation is not
 * possible.
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK(construct,check) \
    int \
    main (int argc, char **argv) \
    { \
        GtkWidget *plugin; \
        XfcePanelPluginCheck test = (XfcePanelPluginCheck)check; \
        gtk_init (&argc, &argv); \
        if (!test(gdk_screen_get_default())) return 2; \
        plugin = xfce_external_panel_plugin_new (argc, argv, \
                     (XfcePanelPluginFunc)construct); \
        if (!plugin) return 1; \
        g_signal_connect_after (plugin, "destroy", \
                                G_CALLBACK (exit), NULL); \
        gtk_widget_show (plugin); \
        gtk_main (); \
        return 0; \
    }

/**
 * XFCE_PANEL_PLUGIN_REGISTER_INTERNAL
 * @construct : name of a function that can be cast to an #XfcePanelPluginFunc
 *
 * Registers and initializes the plugin. This is the only thing that is 
 * required to create a panel plugin.
 *
 * This macro is for plugins implemented as a loadable module. Generally it is
 * preferred to create an external plugin, for which you have to use
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL() .
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(construct) \
    XfcePanelPluginFunc \
    xfce_panel_plugin_get_construct (void) \
    { \
        return (XfcePanelPluginFunc)construct; \
    }

/**
 * XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK
 * @construct : name of a function that can be cast to an #XfcePanelPluginFunc
 * @check     : name of a function that can be cast to an
 *              #XfcePanelPluginCheck
 *
 * Registers and initializes the plugin. This is the only thing that is 
 * required to create a panel plugin. The @check function is run before
 * creating the plugin, and should return FALSE if plugin creation is not
 * possible.
 *
 * This macro is for plugins implemented as a loadable module. Generally it is
 * preferred to create an external plugin, for which you have to use
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL() .
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK(construct,check) \
    XfcePanelPluginFunc \
    xfce_panel_plugin_get_construct (void) \
    { \
        return (XfcePanelPluginFunc)construct; \
    } \
    XfcePanelPluginCheck \
    xfce_panel_plugin_get_check (void) \
    { \
        return (XfcePanelPluginCheck)check; \
    }

#endif /* _XFCE_PANEL_PLUGIN_H */
