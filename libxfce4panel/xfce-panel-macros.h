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

#ifndef __XFCE_PANEL_MACROS_H__
#define __XFCE_PANEL_MACROS_H__

#include <glib.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/libxfce4panel-deprecated.h>

G_BEGIN_DECLS

typedef GTypeModule XfcePanelTypeModule;

/* xfconf channel for plugins */
#define XFCE_PANEL_PLUGIN_CHANNEL_NAME ("xfce4-panel")

/* macro for opening an XfconfChannel for plugins */
#define xfce_panel_plugin_xfconf_channel_new(plugin) \
  xfconf_channel_new_with_property_base (XFCE_PANEL_PLUGIN_CHANNEL_NAME, \
    xfce_panel_plugin_get_property_base (XFCE_PANEL_PLUGIN (plugin)))

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
  type_name##_register_type (XfcePanelTypeModule *type_module) \
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
    plugin_define_type_id = \
        g_type_module_register_type (G_TYPE_MODULE (type_module), TYPE_PARENT, \
                                     "Xfce" #TypeName, &plugin_define_type_info, 0); \
    \
    type_name##_type = plugin_define_type_id; \
  }

#define _XPP_DEFINE_PLUGIN(TypeName, type_name, resident, args...) \
  GType xfce_panel_module_init (GTypeModule *type_module, gboolean *make_resident); \
  \
  XFCE_PANEL_DEFINE_TYPE (TypeName, type_name, XFCE_TYPE_PANEL_PLUGIN) \
  \
  G_MODULE_EXPORT GType \
  xfce_panel_module_init (GTypeModule *type_module, \
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

/**
 * XFCE_PANEL_PLUGIN_REGISTER:
 * construct_func : name of the function that points to an
 *                  #XfcePanelPluginFunc function.
 *
 * Since: 4.8
 **/
#define XFCE_PANEL_PLUGIN_REGISTER(construct_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (construct_func, /* foo */, /* foo */)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK:
 * construct_func : name of the function that points to an
 *                  #XfcePanelPluginFunc function.
 * check_func     : name of the function that points to an
 *                  #XfcePanelPluginCheck function.
 *
 * Since: 4.8
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK(construct_func, check_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (construct_func, /* foo */, \
    if (G_LIKELY ((*check_func) (xpp_screen) == TRUE)))

/**
 * XFCE_PANEL_PLUGIN_REGISTER_FULL:
 * construct_func : name of the function that points to an
 *                  #XfcePanelPluginFunc function.
 * preinit_func   : name of the function that points to an
 *                  #XfcePanelPluginPreInit function.
 * check_func     : name of the function that points to an
 *                  #XfcePanelPluginCheck function.
 *
 * Since: 4.8
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_FULL(construct_func, preinit_func, check_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (construct_func, \
    G_MODULE_EXPORT gboolean xfce_panel_module_preinit (gint argc, gchar **argv); \
    \
    G_MODULE_EXPORT gboolean \
    xfce_panel_module_preinit (gint    argc, \
                               gchar **argv) \
    { \
      return (*preinit_func) (argc, argv); \
    }, \
    \
    if (G_LIKELY ((*check_func) (xpp_screen) == TRUE)))

/* <private> */
#define XFCE_PANEL_PLUGIN_REGISTER_EXTENDED(construct_func, PREINIT_CODE, CHECK_CODE) \
  static void \
  xfce_panel_module_realize (XfcePanelPlugin *xpp) \
  { \
    panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (xpp)); \
    \
    g_signal_handlers_disconnect_by_func (G_OBJECT (xpp), \
        G_CALLBACK (xfce_panel_module_realize), NULL); \
    \
    (*construct_func) (xpp); \
  } \
  \
  PREINIT_CODE \
  \
  G_MODULE_EXPORT XfcePanelPlugin * \
  xfce_panel_module_construct (const gchar  *xpp_name, \
                               gint          xpp_unique_id, \
                               const gchar  *xpp_display_name, \
                               const gchar  *xpp_comment, \
                               gchar       **xpp_arguments, \
                               GdkScreen    *xpp_screen); \
  G_MODULE_EXPORT XfcePanelPlugin * \
  xfce_panel_module_construct (const gchar  *xpp_name, \
                               gint          xpp_unique_id, \
                               const gchar  *xpp_display_name, \
                               const gchar  *xpp_comment, \
                               gchar       **xpp_arguments, \
                               GdkScreen    *xpp_screen) \
  { \
    XfcePanelPlugin *xpp = NULL; \
    \
    panel_return_val_if_fail (GDK_IS_SCREEN (xpp_screen), NULL); \
    panel_return_val_if_fail (xpp_name != NULL && xpp_unique_id != -1, NULL); \
    \
    CHECK_CODE \
      { \
        xpp = g_object_new (XFCE_TYPE_PANEL_PLUGIN, \
                            "name", xpp_name, \
                            "unique-id", xpp_unique_id, \
                            "display-name", xpp_display_name, \
                            "comment", xpp_comment, \
                            "arguments", xpp_arguments, NULL); \
        \
        g_signal_connect_after (G_OBJECT (xpp), "realize", \
            G_CALLBACK (xfce_panel_module_realize), NULL); \
      } \
    \
    return xpp; \
  }

G_END_DECLS

#endif /* !__XFCE_PANEL_MACROS_H__ */
