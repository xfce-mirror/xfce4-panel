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

#ifndef __XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H__
#define __XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H__

#include <libxfce4panel/xfce-panel-plugin-iface.h>

G_BEGIN_DECLS

struct _XfcePanelPluginInterface
{
    GTypeInterface parent;

    /* signals */
    void (*remove)           (XfcePanelPlugin *plugin);
    void (*set_expand)       (XfcePanelPlugin *plugin,
                              gboolean         expand);
    void (*customize_panel)  (XfcePanelPlugin *plugin);
    void (*customize_items)  (XfcePanelPlugin *plugin);
    void (*move)             (XfcePanelPlugin *plugin);
    void (*register_menu)    (XfcePanelPlugin *plugin,
                              GtkMenu         *menu);
    void (*focus_panel)      (XfcePanelPlugin *plugin);
    void (*set_panel_hidden) (XfcePanelPlugin *plugin,
                              gboolean         hidden);

    /* reserved for future expansion */
    void (*reserved1) (void);
    void (*reserved2) (void);
};

void  _xfce_panel_plugin_create_menu            (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_popup_menu             (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_screen_position (XfcePanelPlugin     *plugin,
                                                 XfceScreenPosition   position) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_orientation     (XfcePanelPlugin     *plugin,
                                                 GtkOrientation       orientation) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_size            (XfcePanelPlugin     *plugin,
                                                 gint                 size) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_free_data       (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_save            (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_signal_configure       (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_set_sensitive          (XfcePanelPlugin     *plugin,
                                                 gboolean             sensitive) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_remove_confirm         (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_remove                 (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_customize_panel        (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_customize_items        (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;
void  _xfce_panel_plugin_move                   (XfcePanelPlugin     *plugin) G_GNUC_INTERNAL;

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_IFACE_PRIVATE_H__ */
