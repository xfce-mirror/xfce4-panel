/* $Id$ */
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

#ifndef __LIBXFCE4PANEL__
#define __LIBXFCE4PANEL__

G_BEGIN_DECLS

#define LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H

#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-scaled-image.h>

#undef LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H

/* register a resident panel plugin */
#define XFCE_PANEL_DEFINE_PLUGIN_RESIDENT(TypeName, type_name, args...) \
  _XPP_DEFINE_PLUGIN (TypeName, type_name, TRUE, args)

/* register a panel plugin that can unload the module */
#define XFCE_PANEL_DEFINE_PLUGIN(TypeName, type_name, args...) \
  _XPP_DEFINE_PLUGIN (TypeName, type_name, FALSE, args)

#define XFCE_PANEL_DEFINE_TYPE(TypeName, type_name, TYPE_PARENT) \
  static gpointer type_name##_parent_class = NULL; \
  static GType    type_name##_type = 0; \
  \
  static void     type_name##_init              (TypeName        *self); \
  static void     type_name##_class_init        (TypeName##Class *klass); \
  static void     type_name##_class_intern_init (gpointer klass) \
  { \
    type_name##_parent_class = g_type_class_peek_parent (klass); \
    type_name##_class_init ((TypeName##Class*) klass); \
  } \
  \
  GType \
  type_name##_get_type (void) \
  { \
    return type_name##_type; \
  } \
  \
  void \
  type_name##_register_type (GTypeModule *type_module) \
  { \
    GType plugin_define_type_id; \
    static const GTypeInfo plugin_define_type_info = \
    { \
      sizeof (TypeName##Class), \
      NULL, \
      NULL, \
      (GClassInitFunc) type_name##_class_intern_init, \
      NULL, \
      NULL, \
      sizeof (TypeName), \
      0, \
      (GInstanceInitFunc) type_name##_init, \
      NULL, \
    }; \
    \
    g_type_module_register_type (type_module, TYPE_PARENT, \
                                 #TypeName, &plugin_define_type_info, 0); \
    \
    type_name##_type = plugin_define_type_id; \
  }

#define _XPP_DEFINE_PLUGIN(TypeName, type_name, resident, args...) \
  GType __xpp_initialize (GTypeModule *type_module, gboolean *make_resident); \
  \
  XFCE_PANEL_DEFINE_TYPE (TypeName, type_name, XFCE_TYPE_PANEL_PLUGIN) \
  \
  PANEL_SYMBOL_EXPORT GType \
  __xpp_initialize (GTypeModule *type_module, \
                    gboolean    *make_resident) \
  { \
    typedef void (*XppRegFunc) (GTypeModule *module); \
    XppRegFunc reg_funcs[] = { type_name##_register_type, args }; \
    guint      i; \
    \
    /* whether to make this plugin resident */ \
    if (make_resident != NULL) \
      *make_resident = resident; \
    \
    /* register the plugin types */ \
    for (i = 0; i < G_N_ELEMENTS (reg_funcs); i++) \
      (* reg_funcs[i]) (type_module); \
    \
    return type_name##_get_type (); \
  }

#define XFCE_PANEL_PLUGIN_REGISTER(init_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (init_func, {})

#define XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK(construct_func, check_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (construct_func, if (G_LIKELY ((*check_func) (screen) == TRUE)))

#define XFCE_PANEL_PLUGIN_REGISTER_EXTENDED(construct_func, CODE) \
  static void \
  __xpp_realize (XfcePanelPlugin *plugin) \
  { \
    panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin)); \
    \
    g_signal_handlers_disconnect_by_func (G_OBJECT (plugin), G_CALLBACK (__xpp_realize), NULL); \
    \
    (*construct_func) (plugin); \
  } \
  \
  PANEL_SYMBOL_EXPORT G_MODULE_EXPORT XfcePanelPlugin * \
  __xpp_construct (const gchar  *name, \
                   gint          unique_id, \
                   const gchar  *display_name, \
                   gchar       **arguments, \
                   GdkScreen    *screen); \
  PANEL_SYMBOL_EXPORT G_MODULE_EXPORT XfcePanelPlugin * \
  __xpp_construct (const gchar  *name, \
                   gint          unique_id, \
                   const gchar  *display_name, \
                   gchar       **arguments, \
                   GdkScreen    *screen) \
  { \
    XfcePanelPlugin *plugin = NULL; \
    \
    panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL); \
    panel_return_val_if_fail (name != NULL && unique_id != -1, NULL); \
    \
    CODE \
      { \
        plugin = g_object_new (XFCE_TYPE_PANEL_PLUGIN, \
                               "name", name, \
                               "unique-id", unique_id, \
                               "display-name", display_name, \
                               "arguments", arguments, NULL); \
        \
        g_signal_connect_after (G_OBJECT (plugin), "realize", G_CALLBACK (__xpp_realize), NULL); \
      } \
    \
    return plugin; \
  }

G_END_DECLS

#endif /* !__LIBXFCE4PANEL__ */
