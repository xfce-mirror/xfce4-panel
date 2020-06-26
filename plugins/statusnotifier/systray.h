/*
 * Copyright (c) 2008-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __SYSTRAY_H__
#define __SYSTRAY_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include "sn-plugin.h"

G_BEGIN_DECLS

typedef struct _SnPluginClass SnPluginClass;
typedef struct _SnPlugin      SnPlugin;
typedef struct _SystrayChild       SystrayChild;
typedef enum   _SystrayChildState  SystrayChildState;

//#define XFCE_TYPE_SYSTRAY_PLUGIN            (systray_plugin_get_type ())
//#define XFCE_SN_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SYSTRAY_PLUGIN, SnPlugin))
//#define XFCE_SN_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SYSTRAY_PLUGIN, SnPluginClass))
//#define XFCE_IS_SN_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SYSTRAY_PLUGIN))
//#define XFCE_IS_SN_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SYSTRAY_PLUGIN))
//#define XFCE_SN_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SYSTRAY_PLUGIN, SnPluginClass))

GType systray_plugin_get_type      (void) G_GNUC_CONST;

//void  systray_plugin_register_type    (XfcePanelTypeModule *type_module);

void  systray_plugin_box_draw            (GtkWidget             *box,
                                          cairo_t               *cr,
                                          gpointer               user_data);
void  systray_plugin_button_toggled      (GtkWidget             *button,
                                          SnPlugin              *plugin);
void  systray_plugin_screen_changed      (GtkWidget             *widget,
                                          GdkScreen             *previous_screen);
void  systray_plugin_composited_changed  (GtkWidget             *widget);
void  systray_plugin_orientation_changed (XfcePanelPlugin       *panel_plugin,
                                          GtkOrientation         orientation);

G_END_DECLS

#endif /* !__SYSTRAY_H__ */
