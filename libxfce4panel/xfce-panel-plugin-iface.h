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

#ifndef __XFCE_PANEL_PLUGIN_IFACE_H__
#define __XFCE_PANEL_PLUGIN_IFACE_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>
#include <libxfce4panel/xfce-panel-enums.h>

G_BEGIN_DECLS

typedef struct _XfcePanelPlugin          XfcePanelPlugin;
typedef struct _XfcePanelPluginInterface XfcePanelPluginInterface;

#define XFCE_TYPE_PANEL_PLUGIN                (xfce_panel_plugin_get_type ())
#define XFCE_PANEL_PLUGIN(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPlugin))
#define XFCE_IS_PANEL_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_PLUGIN))
#define XFCE_PANEL_PLUGIN_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPluginInterface))

/**
 * XfcePanelPluginFunc:
 * @plugin : The #XfcePanelPlugin
 *
 * Callback function to create the plugin contents. It should be given as
 * the argument to the registration macros.
 *
 * See also: XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL() and
 *           XFCE_PANEL_PLUGIN_REGISTER_INTERNAL()
 **/
typedef void (*XfcePanelPluginFunc) (XfcePanelPlugin *plugin);

/**
 * XfcePanelPluginPreInit:
 * @argc : number of arguments to the plugin
 * @argv : argument array
 *
 * Callback function that is run in an external plugin before gtk_init(). It
 * should return %FALSE if the plugin is not available for whatever reason.
 * The function can be given as argument to one of the registration macros.
 *
 * The main purpose of this callback is to allow multithreaded plugins to call
 * g_thread_init().
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * See also: XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL()
 * Since: 4.5
 **/
typedef gboolean (*XfcePanelPluginPreInit) (int argc, char **argv);

/**
 * XfcePanelPluginCheck:
 * @screen : the #GdkScreen the panel is running on
 *
 * Callback function that is run before creating a plugin. It should return
 * %FALSE if the plugin is not available for whatever reason. The function
 * can be given as argument to one of the registration macros.
 *
 * Returns: %TRUE if the plugin can be started, %FALSE otherwise.
 *
 * See also: XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK(),
 *           XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK() and
 *           XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL()
 **/
typedef gboolean (*XfcePanelPluginCheck) (GdkScreen *screen);

GType                xfce_panel_plugin_get_type             (void) G_GNUC_CONST;


gchar               *xfce_panel_plugin_get_name             (XfcePanelPlugin  *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gchar               *xfce_panel_plugin_get_id               (XfcePanelPlugin  *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gchar               *xfce_panel_plugin_get_display_name     (XfcePanelPlugin  *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gint                 xfce_panel_plugin_get_size             (XfcePanelPlugin  *plugin);
XfceScreenPosition   xfce_panel_plugin_get_screen_position  (XfcePanelPlugin  *plugin);
void                 xfce_panel_plugin_set_expand           (XfcePanelPlugin  *plugin,
                                                             gboolean          expand);
gboolean             xfce_panel_plugin_get_expand           (XfcePanelPlugin  *plugin);
GtkOrientation       xfce_panel_plugin_get_orientation      (XfcePanelPlugin  *plugin);


void                 xfce_panel_plugin_add_action_widget    (XfcePanelPlugin  *plugin,
                                                             GtkWidget        *widget);
void                 xfce_panel_plugin_menu_insert_item     (XfcePanelPlugin  *plugin,
                                                             GtkMenuItem      *item);
void                 xfce_panel_plugin_menu_show_about      (XfcePanelPlugin  *plugin);
void                 xfce_panel_plugin_menu_show_configure  (XfcePanelPlugin  *plugin);
void                 xfce_panel_plugin_block_menu           (XfcePanelPlugin  *plugin);
void                 xfce_panel_plugin_unblock_menu         (XfcePanelPlugin  *plugin);


void                 xfce_panel_plugin_register_menu        (XfcePanelPlugin  *plugin,
                                                             GtkMenu          *menu);
GtkArrowType         xfce_panel_plugin_arrow_type           (XfcePanelPlugin  *plugin);
void                 xfce_panel_plugin_position_widget      (XfcePanelPlugin  *plugin,
                                                             GtkWidget        *menu_widget,
                                                             GtkWidget        *attach_widget,
                                                             gint             *x,
                                                             gint             *y);
void                 xfce_panel_plugin_position_menu        (GtkMenu          *menu,
                                                             gint             *x,
                                                             gint             *y,
                                                             gboolean         *push_in,
                                                             gpointer          panel_plugin);


gchar               *xfce_panel_plugin_lookup_rc_file       (XfcePanelPlugin  *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gchar               *xfce_panel_plugin_save_location        (XfcePanelPlugin  *plugin,
                                                             gboolean          create) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;


void                 xfce_panel_plugin_focus_widget         (XfcePanelPlugin  *plugin,
                                                             GtkWidget        *widget);
void                 xfce_panel_plugin_set_panel_hidden     (XfcePanelPlugin  *plugin,
                                                             gboolean          hidden);

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_IFACE_H__ */

