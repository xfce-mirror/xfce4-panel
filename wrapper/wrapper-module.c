/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdk.h>
#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <wrapper/wrapper-module.h>



static void      wrapper_module_class_init (WrapperModuleClass *klass);
static void      wrapper_module_init       (WrapperModule      *module);
static gboolean  wrapper_module_load       (GTypeModule        *type_module);
static void      wrapper_module_unload     (GTypeModule        *type_module);



struct _WrapperModuleClass
{
  GTypeModuleClass __parent__;
};

struct _WrapperModule
{
  GTypeModule __parent__;

  /* plugin library */
  GModule             *library;

  /* plugin init function */
  PluginConstructFunc  construct_func;

  /* the library location */
  const gchar         *filename;
};



G_DEFINE_TYPE (WrapperModule, wrapper_module, G_TYPE_TYPE_MODULE);



static void
wrapper_module_class_init (WrapperModuleClass *klass)
{
  GTypeModuleClass *gtype_module_class;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = wrapper_module_load;
  gtype_module_class->unload = wrapper_module_unload;
}



static void
wrapper_module_init (WrapperModule *module)
{
  /* initialize */
  module->library = NULL;
  module->construct_func = NULL;
  module->filename = NULL;
}



static gboolean
wrapper_module_load (GTypeModule *type_module)
{
  WrapperModule           *module = WRAPPER_MODULE (type_module);
  PluginRegisterTypesFunc  register_func;

  panel_return_val_if_fail (WRAPPER_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), FALSE);

  /* load the module */
  module->library = g_module_open (module->filename, G_MODULE_BIND_LOCAL);
  if (G_UNLIKELY (module->library == NULL))
    {
      g_critical ("Failed to load plugin '%s': %s", type_module->name, g_module_error ());

      return FALSE;
    }

  /* link the required construct function */
  if (!g_module_symbol (module->library, "xfce_panel_plugin_construct", (gpointer) &module->construct_func))
    {
      g_critical ("Plugin '%s' lacks required symbol: %s", type_module->name, g_module_error ());

      /* unload */
      wrapper_module_unload (type_module);

      return FALSE;
    }

  /* run the type register function if available */
  if (g_module_symbol (module->library, "xfce_panel_plugin_register_types", (gpointer) &register_func))
    (*register_func) (type_module);

  return TRUE;
}



static void
wrapper_module_unload (GTypeModule *type_module)
{
  WrapperModule *module = WRAPPER_MODULE (type_module);

  panel_return_if_fail (WRAPPER_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));

  /* unload the library */
  g_module_close (module->library);

  /* reset plugin state */
  module->library = NULL;
  module->construct_func = NULL;
}



WrapperModule *
wrapper_module_new (const gchar *filename,
                    const gchar *name)
{
  WrapperModule *module = NULL;

  panel_return_val_if_fail (filename != NULL && *filename != '\0', NULL);

  /* test if the library exists */
  if (G_LIKELY (g_file_test (filename, G_FILE_TEST_EXISTS)))
    {
      /* create new module */
      module = g_object_new (WRAPPER_TYPE_MODULE, NULL);

      /* set the module name */
      g_type_module_set_name (G_TYPE_MODULE (module), name);

      /* set library location */
      module->filename = filename;
    }

  return module;
}



XfcePanelPluginProvider *
wrapper_module_create_plugin (WrapperModule  *module,
                              const gchar    *name,
                              const gchar    *id,
                              const gchar    *display_name,
                              gchar         **arguments)
{
  XfcePanelPluginProvider *provider = NULL;

  panel_return_val_if_fail (WRAPPER_IS_MODULE (module), NULL);
  panel_return_val_if_fail (name != NULL && *name != '\0', NULL);
  panel_return_val_if_fail (id != NULL && *id != '\0', NULL);

  /* increase the module use count */
  g_type_module_use (G_TYPE_MODULE (module));

  if (G_LIKELY (module->library))
    {
      /* debug check */
      panel_return_val_if_fail (module->construct_func != NULL, NULL);

      /* create a new panel plugin */
      provider = (*module->construct_func) (name, id, display_name, arguments, gdk_screen_get_default ());
    }
  else
    {
      /* decrease the module use count */
      g_type_module_unuse (G_TYPE_MODULE (module));

      /* this should never happen */
      panel_assert_not_reached ();
    }

  return provider;
}
