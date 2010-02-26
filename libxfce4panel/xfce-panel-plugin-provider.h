/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __XFCE_PANEL_PLUGIN_PROVIDER_H__
#define __XFCE_PANEL_PLUGIN_PROVIDER_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _XfcePanelPluginProviderIface  XfcePanelPluginProviderIface;
typedef struct _XfcePanelPluginProvider       XfcePanelPluginProvider;
typedef enum   _XfcePanelPluginProviderSignal XfcePanelPluginProviderSignal;

#define XFCE_TYPE_PANEL_PLUGIN_PROVIDER           (xfce_panel_plugin_provider_get_type ())
#define XFCE_PANEL_PLUGIN_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER, XfcePanelPluginProvider))
#define XFCE_IS_PANEL_PLUGIN_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER))
#define XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER, XfcePanelPluginProviderIface))

/* provider contruct function */
typedef XfcePanelPluginProvider *(*PluginConstructFunc) (const gchar  *name,
                                                         gint          id,
                                                         const gchar  *display_name,
                                                         gchar       **arguments,
                                                         GdkScreen    *screen);

struct _XfcePanelPluginProviderIface
{
  /*< private >*/
  GTypeInterface __parent__;

  /*< signals >*/
  void         (*provider_signal)     (XfcePanelPluginProvider       *provider,
                                       XfcePanelPluginProviderSignal  signal);

  /*< public >*/
  const gchar *(*get_name)            (XfcePanelPluginProvider       *provider);
  gint         (*get_unique_id)       (XfcePanelPluginProvider       *provider);
  void         (*set_size)            (XfcePanelPluginProvider       *provider,
                                       gint                           size);
  void         (*set_orientation)     (XfcePanelPluginProvider       *provider,
                                       GtkOrientation                 orientation);
  void         (*set_screen_position) (XfcePanelPluginProvider       *provider,
                                       XfceScreenPosition             screen_position);
  void         (*save)                (XfcePanelPluginProvider       *provider);
};

enum _XfcePanelPluginProviderSignal
{
  PROVIDER_SIGNAL_MOVE_PLUGIN,
  PROVIDER_SIGNAL_EXPAND_PLUGIN,
  PROVIDER_SIGNAL_COLLAPSE_PLUGIN,
  PROVIDER_SIGNAL_LOCK_PANEL,
  PROVIDER_SIGNAL_UNLOCK_PANEL,
  PROVIDER_SIGNAL_REMOVE_PLUGIN,
  PROVIDER_SIGNAL_ADD_NEW_ITEMS,
  PROVIDER_SIGNAL_PANEL_PREFERENCES
};



PANEL_SYMBOL_EXPORT
GType        xfce_panel_plugin_provider_get_type            (void) G_GNUC_CONST;

const gchar *xfce_panel_plugin_provider_get_name            (XfcePanelPluginProvider       *provider);

gint         xfce_panel_plugin_provider_get_unique_id       (XfcePanelPluginProvider       *provider);

void         xfce_panel_plugin_provider_set_size            (XfcePanelPluginProvider       *provider,
                                                             gint                           size);

void         xfce_panel_plugin_provider_set_orientation     (XfcePanelPluginProvider       *provider,
                                                             GtkOrientation                 orientation);

void         xfce_panel_plugin_provider_set_screen_position (XfcePanelPluginProvider       *provider,
                                                             XfceScreenPosition             screen_position);

void         xfce_panel_plugin_provider_save                (XfcePanelPluginProvider       *provider);

void         xfce_panel_plugin_provider_send_signal         (XfcePanelPluginProvider       *provider,
                                                             XfcePanelPluginProviderSignal  signal);

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_PROVIDER_H__ */
