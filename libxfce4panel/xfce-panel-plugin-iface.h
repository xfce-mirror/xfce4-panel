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

#ifndef _XFCE_PANEL_PLUGIN_IFACE_H
#define _XFCE_PANEL_PLUGIN_IFACE_H

#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-enums.h>

#define XFCE_TYPE_PANEL_PLUGIN                (xfce_panel_plugin_get_type ())
#define XFCE_PANEL_PLUGIN(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPlugin))
#define XFCE_IS_PANEL_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_PLUGIN))
#define XFCE_PANEL_PLUGIN_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPluginInterface))


G_BEGIN_DECLS

typedef struct _XfcePanelPlugin XfcePanelPlugin;    /* dummy object */
typedef struct _XfcePanelPluginInterface XfcePanelPluginInterface;

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


GType xfce_panel_plugin_get_type (void) G_GNUC_CONST;


/* properties */

G_CONST_RETURN char *xfce_panel_plugin_get_name (XfcePanelPlugin *plugin);

G_CONST_RETURN char *xfce_panel_plugin_get_id (XfcePanelPlugin *plugin);

G_CONST_RETURN char * xfce_panel_plugin_get_display_name (XfcePanelPlugin *plugin);

int xfce_panel_plugin_get_size (XfcePanelPlugin *plugin);

XfceScreenPosition xfce_panel_plugin_get_screen_position (XfcePanelPlugin *plugin);

void xfce_panel_plugin_set_expand (XfcePanelPlugin *plugin, 
                                   gboolean expand);

gboolean xfce_panel_plugin_get_expand (XfcePanelPlugin *plugin);


/* convenience */
GtkOrientation xfce_panel_plugin_get_orientation (XfcePanelPlugin *plugin);

    
/* menu */
void xfce_panel_plugin_add_action_widget (XfcePanelPlugin *plugin, 
                                          GtkWidget *widget);

void xfce_panel_plugin_menu_insert_item (XfcePanelPlugin *plugin,
                                         GtkMenuItem *item);

void xfce_panel_plugin_menu_show_about (XfcePanelPlugin *plugin);

void xfce_panel_plugin_menu_show_configure (XfcePanelPlugin *plugin);

void xfce_panel_plugin_block_menu (XfcePanelPlugin *plugin);

void xfce_panel_plugin_unblock_menu (XfcePanelPlugin *plugin);

void xfce_panel_plugin_register_menu (XfcePanelPlugin *plugin,
                                      GtkMenu *menu);


/* config */
char *xfce_panel_plugin_lookup_rc_file (XfcePanelPlugin *plugin);

char *xfce_panel_plugin_save_location (XfcePanelPlugin *plugin,
                                       gboolean create);


G_END_DECLS

#endif /* _XFCE_PANEL_PLUGIN_IFACE_H */
