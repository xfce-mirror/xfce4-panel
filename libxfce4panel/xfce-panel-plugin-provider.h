/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#define XFCE_TYPE_PANEL_PLUGIN_PROVIDER           (xfce_panel_plugin_provider_get_type ())
#define XFCE_PANEL_PLUGIN_PROVIDER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER, XfcePanelPluginProvider))
#define XFCE_IS_PANEL_PLUGIN_PROVIDER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER))
#define XFCE_PANEL_PLUGIN_PROVIDER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), XFCE_TYPE_PANEL_PLUGIN_PROVIDER, XfcePanelPluginProviderIface))

/* plugin module functions */
typedef GtkWidget *(*PluginConstructFunc) (const gchar  *name,
                                           gint          unique_id,
                                           const gchar  *display_name,
                                           const gchar  *comment,
                                           gchar       **arguments,
                                           GdkScreen    *screen);
typedef GType      (*PluginInitFunc)      (GTypeModule  *module,
                                           gboolean     *make_resident);

struct _XfcePanelPluginProviderIface
{
  /*< private >*/
  GTypeInterface __parent__;

  /*<public >*/
  const gchar *(*get_name)            (XfcePanelPluginProvider       *provider);
  gint         (*get_unique_id)       (XfcePanelPluginProvider       *provider);
  void         (*set_size)            (XfcePanelPluginProvider       *provider,
                                       gint                           size);
  void         (*set_orientation)     (XfcePanelPluginProvider       *provider,
                                       GtkOrientation                 orientation);
  void         (*set_screen_position) (XfcePanelPluginProvider       *provider,
                                       XfceScreenPosition             screen_position);
  void         (*save)                (XfcePanelPluginProvider       *provider);
  gboolean     (*get_show_configure)  (XfcePanelPluginProvider       *provider);
  void         (*show_configure)      (XfcePanelPluginProvider       *provider);
  gboolean     (*get_show_about)      (XfcePanelPluginProvider       *provider);
  void         (*show_about)          (XfcePanelPluginProvider       *provider);
  void         (*remove)              (XfcePanelPluginProvider       *provider);
  gboolean     (*remote_event)        (XfcePanelPluginProvider       *provider,
                                       const gchar                   *name,
                                       const GValue                  *value);
};

/* signals send from the plugin to the panel (possibly through the wrapper) */
typedef enum /*< skip >*/
{
  PROVIDER_SIGNAL_MOVE_PLUGIN = 0,
  PROVIDER_SIGNAL_EXPAND_PLUGIN,
  PROVIDER_SIGNAL_COLLAPSE_PLUGIN,
  PROVIDER_SIGNAL_WRAP_PLUGIN,
  PROVIDER_SIGNAL_UNWRAP_PLUGIN,
  PROVIDER_SIGNAL_LOCK_PANEL,
  PROVIDER_SIGNAL_UNLOCK_PANEL,
  PROVIDER_SIGNAL_REMOVE_PLUGIN,
  PROVIDER_SIGNAL_ADD_NEW_ITEMS,
  PROVIDER_SIGNAL_PANEL_PREFERENCES,
  PROVIDER_SIGNAL_PANEL_QUIT,
  PROVIDER_SIGNAL_PANEL_RESTART,
  PROVIDER_SIGNAL_PANEL_ABOUT,
  PROVIDER_SIGNAL_SHOW_CONFIGURE,
  PROVIDER_SIGNAL_SHOW_ABOUT,
  PROVIDER_SIGNAL_FOCUS_PLUGIN
}
XfcePanelPluginProviderSignal;

/* plugin exit values */
enum
{
  PLUGIN_EXIT_SUCCESS = 0,
  PLUGIN_EXIT_FAILURE,
  PLUGIN_EXIT_PREINIT_FAILED,
  PLUGIN_EXIT_CHECK_FAILED,
  PLUGIN_EXIT_NO_PROVIDER
};

/* argument handling in plugin and wrapper */
enum
{
  PLUGIN_ARGV_0 = 0,
  PLUGIN_ARGV_FILENAME,
  PLUGIN_ARGV_UNIQUE_ID,
  PLUGIN_ARGV_SOCKET_ID,
  PLUGIN_ARGV_NAME,
  PLUGIN_ARGV_DISPLAY_NAME,
  PLUGIN_ARGV_COMMENT,
  PLUGIN_ARGV_ARGUMENTS
};



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

void         xfce_panel_plugin_provider_emit_signal         (XfcePanelPluginProvider       *provider,
                                                             XfcePanelPluginProviderSignal  provider_signal);

gboolean     xfce_panel_plugin_provider_get_show_configure  (XfcePanelPluginProvider       *provider);

void         xfce_panel_plugin_provider_show_configure      (XfcePanelPluginProvider       *provider);

gboolean     xfce_panel_plugin_provider_get_show_about      (XfcePanelPluginProvider       *provider);

void         xfce_panel_plugin_provider_show_about          (XfcePanelPluginProvider       *provider);

void         xfce_panel_plugin_provider_remove              (XfcePanelPluginProvider       *provider);

gboolean     xfce_panel_plugin_provider_remote_event        (XfcePanelPluginProvider       *provider,
                                                             const gchar                   *name,
                                                             const GValue                  *value);

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_PROVIDER_H__ */
