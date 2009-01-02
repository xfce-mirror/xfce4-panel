/* $Id$
 *
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __XFCE_PANEL_PLUGIN_H__
#define __XFCE_PANEL_PLUGIN_H__

#include <gtk/gtk.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-plugin-iface.h>
#include <libxfce4panel/xfce-panel-internal-plugin.h>
#include <libxfce4panel/xfce-panel-external-plugin.h>

G_BEGIN_DECLS

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL
 * @construct : name of a function that can be cast to an #XfcePanelPluginFunc
 *
 * Registers and initializes the plugin. This is the only thing that is
 * required to create a panel plugin.
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(construct)  \
                XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL(construct,NULL,NULL)

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
                XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL(construct,NULL,check)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL
 * @construct : name of a function that can be cast to #XfcePanelPluginFunc
 * @init      : name of a function that can be case to #XfcePanelPluginPreInit
 *              or NULL
 * @check     : name of a function that can be cast to #XfcePanelPluginCheck
 *              or NULL
 *
 * Registers and initializes the plugin. This is the only thing that is
 * required to create a panel plugin.
 *
 * The @init argument should be a function that takes two parameters:
 *
 *   gboolean init( int argc, char **argv );
 *
 * The @check functions is run aftern gtk_init() and before creating the
 * plugin; it takes one argument and should return FALSE if plugin creation
 * is not possible:
 *
 *   gboolean check( GdkScreen *screen );
 *
 * See also: <link linkend="XfcePanelPlugin">Panel Plugin interface</link>
 * Since: 4.5
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL(construct,init,check)          \
    gint                                                                        \
    main (gint argc, gchar **argv)                                              \
    {                                                                           \
        GtkWidget              *plugin;                                         \
        XfcePanelPluginFunc     create  = (XfcePanelPluginFunc) construct;      \
        XfcePanelPluginPreInit  preinit = (XfcePanelPluginPreInit) init;        \
        XfcePanelPluginCheck    test    = (XfcePanelPluginCheck) check;         \
                                                                                \
        if ( preinit )                                                          \
        {                                                                       \
            if (G_UNLIKELY (preinit(argc,argv) == FALSE))                       \
                return 3;                                                       \
        }                                                                       \
                                                                                \
        gtk_init (&argc, &argv);                                                \
                                                                                \
        if ( test )                                                             \
        {                                                                       \
            if (G_UNLIKELY (test(gdk_screen_get_default()) == FALSE))           \
                return 2;                                                       \
        }                                                                       \
                                                                                \
        plugin = xfce_external_panel_plugin_new (argc, argv, create);           \
                                                                                \
        if (G_UNLIKELY (plugin == NULL))                                        \
            return 1;                                                           \
                                                                                \
        g_signal_connect_after (G_OBJECT (plugin), "destroy",                   \
                                G_CALLBACK (gtk_main_quit), NULL);              \
                                                                                \
        gtk_widget_show (plugin);                                               \
        gtk_main ();                                                            \
                                                                                \
        return 0;                                                               \
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
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(construct)          \
    XfcePanelPluginFunc xfce_panel_plugin_get_construct (void); \
                                                                \
    XfcePanelPluginFunc                                         \
    xfce_panel_plugin_get_construct (void)                      \
    {                                                           \
        return (XfcePanelPluginFunc) construct;                 \
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
    XfcePanelPluginFunc  xfce_panel_plugin_get_construct (void);        \
    XfcePanelPluginCheck xfce_panel_plugin_get_check     (void);        \
                                                                        \
    XfcePanelPluginFunc                                                 \
    xfce_panel_plugin_get_construct (void)                              \
    {                                                                   \
        return (XfcePanelPluginFunc) construct;                         \
    }                                                                   \
    XfcePanelPluginCheck                                                \
    xfce_panel_plugin_get_check (void)                                  \
    {                                                                   \
        return (XfcePanelPluginCheck) check;                            \
    }

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_H__ */
