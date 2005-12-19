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

#ifndef _XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H
#define _XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H

#include <libxfce4panel/xfce-panel-plugin-iface.h>

G_BEGIN_DECLS

struct _XfcePanelPluginInterface
{
    GTypeInterface parent;

    /* vtable */
    void (*remove)            (XfcePanelPlugin *plugin);

    void (*set_expand)        (XfcePanelPlugin *plugin,
                               gboolean expand);

    void (*customize_panel)   (XfcePanelPlugin *plugin);
    
    void (*customize_items)   (XfcePanelPlugin *plugin);
    
    void (*move)              (XfcePanelPlugin *plugin);
};

/* menu */
void xfce_panel_plugin_create_menu (XfcePanelPlugin *plugin, 
                                    GCallback deactivate_cb);

void xfce_panel_plugin_popup_menu (XfcePanelPlugin *plugin);

/* emit signals -- to be called by implementors */

void xfce_panel_plugin_signal_screen_position (XfcePanelPlugin *plugin,
                                               XfceScreenPosition position);

void xfce_panel_plugin_signal_orientation (XfcePanelPlugin * plugin,
                                           GtkOrientation orientation);

void xfce_panel_plugin_signal_size (XfcePanelPlugin * plugin,
                                    int size);

void xfce_panel_plugin_signal_free_data (XfcePanelPlugin * plugin);

void xfce_panel_plugin_signal_save (XfcePanelPlugin * plugin);

/* set (in)sensitive */
void xfce_panel_plugin_set_sensitive (XfcePanelPlugin *plugin, 
                                      gboolean sensitive);

/* vtable */

void xfce_panel_plugin_remove_confirm (XfcePanelPlugin *plugin);

void xfce_panel_plugin_remove (XfcePanelPlugin *plugin);

void xfce_panel_plugin_customize_panel (XfcePanelPlugin *plugin);

void xfce_panel_plugin_customize_items (XfcePanelPlugin *plugin);

void xfce_panel_plugin_move (XfcePanelPlugin *plugin);


G_END_DECLS

#endif /* _XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H */
