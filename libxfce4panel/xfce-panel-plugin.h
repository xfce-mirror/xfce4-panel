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

/* #if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif */

#ifndef __XFCE_PANEL_PLUGIN_H__
#define __XFCE_PANEL_PLUGIN_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel-enums.h>
#include <libxfce4panel/libxfce4panel-deprecated.h>

G_BEGIN_DECLS

typedef struct _XfcePanelPluginPrivate XfcePanelPluginPrivate;
typedef struct _XfcePanelPluginClass   XfcePanelPluginClass;
typedef struct _XfcePanelPlugin        XfcePanelPlugin;

typedef void (*XfcePanelPluginFunc) (XfcePanelPlugin *plugin);
typedef gboolean (*XfcePanelPluginPreInit) (gint argc, gchar **argv);
typedef gboolean (*XfcePanelPluginCheck) (GdkScreen *screen);

#define XFCE_TYPE_PANEL_PLUGIN            (xfce_panel_plugin_get_type ())
#define XFCE_PANEL_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPlugin))
#define XFCE_PANEL_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPluginClass))
#define XFCE_IS_PANEL_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_PLUGIN))
#define XFCE_IS_PANEL_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_PANEL_PLUGIN))
#define XFCE_PANEL_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPluginClass))

/**
 * XfcePanelPluginClass:
 *
 * TODO
 **/
struct _XfcePanelPluginClass
{
  /*< private >*/
  GtkEventBoxClass __parent__;

  /*< object oriented plugins >*/
  void     (*construct)               (XfcePanelPlugin    *plugin);

  /*< signals >*/
  void     (*screen_position_changed) (XfcePanelPlugin    *plugin,
                                       XfceScreenPosition  position);
  gboolean (*size_changed)            (XfcePanelPlugin    *plugin,
                                       gint                size);
  void     (*orientation_changed)     (XfcePanelPlugin    *plugin,
                                       GtkOrientation      orientation);
  void     (*free_data)               (XfcePanelPlugin    *plugin);
  void     (*save)                    (XfcePanelPlugin    *plugin);
  void     (*about)                   (XfcePanelPlugin    *plugin);
  void     (*configure_plugin)        (XfcePanelPlugin    *plugin);
  void     (*removed)                 (XfcePanelPlugin    *plugin);
  gboolean (*remote_event)            (XfcePanelPlugin    *plugin,
                                       const gchar        *name,
                                       const GValue       *value);

  /*< private >*/
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
};


/**
 * XfcePanelPlugin:
 *
 * TODO
 **/
struct _XfcePanelPlugin
{
  /*< private >*/
  GtkEventBox __parent__;

  /*< private >*/
  XfcePanelPluginPrivate *priv;
};



GType                xfce_panel_plugin_get_type            (void) G_GNUC_CONST;

const gchar         *xfce_panel_plugin_get_name            (XfcePanelPlugin   *plugin) G_GNUC_PURE;

const gchar         *xfce_panel_plugin_get_display_name    (XfcePanelPlugin   *plugin) G_GNUC_PURE;

const gchar         *xfce_panel_plugin_get_comment         (XfcePanelPlugin   *plugin) G_GNUC_PURE;

gint                 xfce_panel_plugin_get_unique_id       (XfcePanelPlugin   *plugin) G_GNUC_PURE;

const gchar         *xfce_panel_plugin_get_property_base   (XfcePanelPlugin   *plugin) G_GNUC_PURE;

const gchar * const *xfce_panel_plugin_get_arguments       (XfcePanelPlugin   *plugin) G_GNUC_PURE;

gint                 xfce_panel_plugin_get_size            (XfcePanelPlugin   *plugin) G_GNUC_PURE;

gboolean             xfce_panel_plugin_get_expand          (XfcePanelPlugin   *plugin) G_GNUC_PURE;

void                 xfce_panel_plugin_set_expand          (XfcePanelPlugin   *plugin,
                                                            gboolean           expand);

GtkOrientation       xfce_panel_plugin_get_orientation     (XfcePanelPlugin   *plugin) G_GNUC_PURE;

XfceScreenPosition   xfce_panel_plugin_get_screen_position (XfcePanelPlugin   *plugin) G_GNUC_PURE;

void                 xfce_panel_plugin_take_window         (XfcePanelPlugin   *plugin,
                                                            GtkWindow         *window);

void                 xfce_panel_plugin_add_action_widget   (XfcePanelPlugin   *plugin,
                                                            GtkWidget         *widget);

void                 xfce_panel_plugin_menu_insert_item    (XfcePanelPlugin   *plugin,
                                                            GtkMenuItem       *item);

void                 xfce_panel_plugin_menu_show_configure (XfcePanelPlugin   *plugin);

void                 xfce_panel_plugin_menu_show_about     (XfcePanelPlugin   *plugin);

void                 xfce_panel_plugin_block_menu          (XfcePanelPlugin   *plugin);

void                 xfce_panel_plugin_unblock_menu        (XfcePanelPlugin   *plugin);

void                 xfce_panel_plugin_register_menu       (XfcePanelPlugin   *plugin,
                                                            GtkMenu           *menu);

GtkArrowType         xfce_panel_plugin_arrow_type          (XfcePanelPlugin   *plugin);

void                 xfce_panel_plugin_position_widget     (XfcePanelPlugin   *plugin,
                                                            GtkWidget         *menu_widget,
                                                            GtkWidget         *attach_widget,
                                                            gint              *x,
                                                            gint              *y);

void                 xfce_panel_plugin_position_menu       (GtkMenu           *menu,
                                                            gint              *x,
                                                            gint              *y,
                                                            gboolean          *push_in,
                                                            gpointer           panel_plugin);

void                 xfce_panel_plugin_focus_widget        (XfcePanelPlugin   *plugin,
                                                            GtkWidget         *widget);

void                 xfce_panel_plugin_block_autohide      (XfcePanelPlugin   *plugin,
                                                            gboolean           blocked);

gchar               *xfce_panel_plugin_lookup_rc_file      (XfcePanelPlugin   *plugin) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

gchar               *xfce_panel_plugin_save_location       (XfcePanelPlugin   *plugin,
                                                            gboolean           create) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__XFCE_PANEL_PLUGIN_H__ */
