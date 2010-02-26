/* $Id$ */
/*
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef __LIBXFCE4PANEL__
#define __LIBXFCE4PANEL__

G_BEGIN_DECLS

/* typedef for registering types, could be useful if we want
 * a real XfcePanelModule object in the feature */
typedef GTypeModule XfcePanelModule;

#define LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H

#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-macros.h>

#undef LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H



#define XFCE_PANEL_DEFINE_TYPE(TN, t_n, T_P)                         XFCE_PANEL_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, 0, {})
#define XFCE_PANEL_DEFINE_TYPE_WITH_CODE(TN, t_n, T_P, _C_)          XFCE_PANEL_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, 0, _C_)
#define XFCE_PANEL_DEFINE_ABSTRACT_TYPE(TN, t_n, T_P)                XFCE_PANEL_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, G_TYPE_FLAG_ABSTRACT, {})
#define XFCE_PANEL_DEFINE_ABSTRACT_TYPE_WITH_CODE(TN, t_n, T_P, _C_) XFCE_PANEL_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, G_TYPE_FLAG_ABSTRACT, _C_)

#define XFCE_PANEL_DEFINE_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE) \
  static gpointer type_name##_parent_class = NULL; \
  static GType    type_name##_type = G_TYPE_INVALID; \
  static void     type_name##_init              (TypeName        *self); \
  static void     type_name##_class_init        (TypeName##Class *klass); \
  static void     type_name##_class_intern_init (TypeName##Class *klass) \
  { \
    type_name##_parent_class = g_type_class_peek_parent (klass); \
    type_name##_class_init (klass); \
  } \
  \
  GType \
  type_name##_get_type (void) \
  { \
    return type_name##_type; \
  } \
  \
  void \
  type_name##_register_type (XfcePanelModule *xfce_panel_module) \
  { \
    GType xfce_panel_define_type_id; \
    static const GTypeInfo xfce_panel_define_type_info = \
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
    xfce_panel_define_type_id = g_type_module_register_type (G_TYPE_MODULE (xfce_panel_module), TYPE_PARENT, \
                                                             #TypeName, &xfce_panel_define_type_info, flags); \
    { CODE ; } \
    type_name##_type = xfce_panel_define_type_id; \
  }

#define XFCE_PANEL_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init) \
  { \
    static const GInterfaceInfo xfce_panel_implement_interface_info = \
    { \
      (GInterfaceInitFunc) iface_init \
    }; \
    g_type_module_add_interface (G_TYPE_MODULE (xfce_panel_module), xfce_panel_define_type_id, \
                                 TYPE_IFACE, &xfce_panel_implement_interface_info); \
  }



#define XFCE_PANEL_PLUGIN_REGISTER_OBJECT(type) \
  XFCE_PANEL_PLUGIN_REGISTER_OBJECT_EXTENDED (type, {})

#define XFCE_PANEL_PLUGIN_REGISTER_OBJECT_WITH_CHECK(type, check_func) \
  XFCE_PANEL_PLUGIN_REGISTER_OBJECT_EXTENDED (type, {})

#define XFCE_PANEL_PLUGIN_REGISTER_OBJECT_EXTENDED(type, CODE) \
  const gchar *plugin_init_name = NULL; \
  const gchar *plugin_init_id = NULL; \
  const gchar *plugin_init_display_name = NULL; \
  \
  G_MODULE_EXPORT XfcePanelPlugin * \
  xfce_panel_plugin_construct (const gchar  *name, \
                               const gchar  *id, \
                               const gchar  *display_name, \
                               gchar       **arguments, \
                               GdkScreen    *screen) \
  { \
    XfcePanelPlugin     *plugin; \
    extern const gchar  *plugin_init_name; \
    extern const gchar  *plugin_init_id; \
    extern const gchar  *plugin_init_display_name; \
    extern gchar       **plugin_init_arguments; \
    \
    panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL); \
    panel_return_val_if_fail (name != NULL && id != NULL, NULL); \
    panel_return_val_if_fail (g_type_is_a (type, XFCE_TYPE_PANEL_PLUGIN), NULL); \
    \
    /* set the temporarily names for the init function */ \
    plugin_init_name = name; \
    plugin_init_id = id; \
    plugin_init_display_name = display_name; \
    plugin_init_arguments = arguments; \
    \
    CODE \
    \
    plugin = g_object_new (type, \
                           "name", name, \
                           "display-name", display_name, \
                           "id", id, \
                           "arguments", arguments, NULL); \
    \
    return plugin; \
  }



#define XFCE_PANEL_PLUGIN_REGISTER(init_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (init_func, {})

#define XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK(construct_func, check_func) \
  XFCE_PANEL_PLUGIN_REGISTER_EXTENDED (construct_func, if (G_LIKELY ((*check_func) (screen) == TRUE)))

#define XFCE_PANEL_PLUGIN_REGISTER_EXTENDED(construct_func, CODE) \
  static void \
  xfce_panel_plugin_realize (XfcePanelPlugin *plugin) \
  { \
    panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin)); \
    \
    /* disconnect the realize signal */ \
    g_signal_handlers_disconnect_by_func (G_OBJECT (plugin), G_CALLBACK (xfce_panel_plugin_realize), NULL); \
    \
    /* run the plugin construct function */ \
    (*construct_func) (plugin); \
  } \
  \
  G_MODULE_EXPORT XfcePanelPlugin * \
  xfce_panel_plugin_construct (const gchar  *name, \
                               const gchar  *id, \
                               const gchar  *display_name, \
                               gchar       **arguments, \
                               GdkScreen    *screen) \
  { \
    XfcePanelPlugin *plugin = NULL; \
    \
    panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL); \
    panel_return_val_if_fail (name != NULL && id != NULL, NULL); \
    \
    CODE \
      { \
        /* create new internal plugin */ \
        plugin = g_object_new (XFCE_TYPE_PANEL_PLUGIN, \
                               "name", name, \
                               "display-name", display_name, \
                               "id", id, \
                               "arguments", arguments, NULL); \
        \
        /* signal to realize the plugin */ \
        g_signal_connect_after (G_OBJECT (plugin), "realize", G_CALLBACK (xfce_panel_plugin_realize), NULL); \
      } \
    \
    /* return the plugin */ \
    return plugin; \
  }

G_END_DECLS

#endif /* !__LIBXFCE4PANEL__ */
