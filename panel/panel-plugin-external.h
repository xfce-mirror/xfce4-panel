/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_PLUGIN_EXTERNAL_H__
#define __PANEL_PLUGIN_EXTERNAL_H__

#include "panel-window.h"

#include "libxfce4panel/libxfce4panel.h"
#include "libxfce4panel/xfce-panel-plugin-provider.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PANEL_TYPE_PLUGIN_EXTERNAL (panel_plugin_external_get_type ())
G_DECLARE_DERIVABLE_TYPE (PanelPluginExternal, panel_plugin_external, PANEL, PLUGIN_EXTERNAL, GtkBox)

struct _PanelPluginExternalClass
{
  GtkBoxClass __parent__;

  /* send panel values to the plugin or wrapper */
  void (*set_properties) (PanelPluginExternal *external,
                          GSList *properties);

  /* complete startup array for the plugin */
  gchar **(*get_argv) (PanelPluginExternal *external,
                       gchar **arguments);

  /* spawn wrapper process according to windowing environment */
  gboolean (*spawn) (PanelPluginExternal *external,
                     gchar **argv,
                     GPid *pid,
                     GError **error);

  /* handling of remote events */
  gboolean (*remote_event) (PanelPluginExternal *external,
                            const gchar *name,
                            const GValue *value,
                            guint *handle);

  /* some info received on plugin startup by the
   * implementations of the abstract object */
  gboolean (*get_show_configure) (PanelPluginExternal *external);
  gboolean (*get_show_about) (PanelPluginExternal *external);

  /* X11 only */
  void (*set_background_color) (PanelPluginExternal *external,
                                const GdkRGBA *color);
  void (*set_background_image) (PanelPluginExternal *external,
                                const gchar *image);

  /* Wayland only */
  void (*set_geometry) (PanelPluginExternal *external,
                        PanelWindow *window);
  gboolean (*pointer_is_outside) (PanelPluginExternal *external);
};

typedef struct
{
  XfcePanelPluginProviderPropType type;
  GValue value;
} PluginProperty;

void
panel_plugin_external_queue_add (PanelPluginExternal *external,
                                 XfcePanelPluginProviderPropType type,
                                 const GValue *value);

void
panel_plugin_external_queue_add_action (PanelPluginExternal *external,
                                        XfcePanelPluginProviderPropType type);

void
panel_plugin_external_restart (PanelPluginExternal *external);

void
panel_plugin_external_set_opacity (PanelPluginExternal *external,
                                   gdouble opacity);

void
panel_plugin_external_set_background_color (PanelPluginExternal *external,
                                            const GdkRGBA *color);

void
panel_plugin_external_set_background_image (PanelPluginExternal *external,
                                            const gchar *image);

void
panel_plugin_external_set_geometry (PanelPluginExternal *external,
                                    PanelWindow *window);

gboolean
panel_plugin_external_pointer_is_outside (PanelPluginExternal *external);

gboolean
panel_plugin_external_get_embedded (PanelPluginExternal *external);

void
panel_plugin_external_set_embedded (PanelPluginExternal *external,
                                    gboolean embedded);

GPid
panel_plugin_external_get_pid (PanelPluginExternal *external);

G_END_DECLS

#endif /* !__PANEL_PLUGIN_EXTERNAL_H__ */
