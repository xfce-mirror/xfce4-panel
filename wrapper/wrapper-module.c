/* $Id$ */
/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <wrapper/wrapper-module.h>



static void     wrapper_module_dispose  (GObject     *object);
static void     wrapper_module_finalize (GObject     *object);
static gboolean wrapper_module_load     (GTypeModule *type_module);
static void     wrapper_module_unload   (GTypeModule *type_module);



struct _WrapperModuleClass
{
  GTypeModuleClass __parent__;
};

struct _WrapperModule
{
  GTypeModule __parent__;

  /* module filename */
  gchar               *filename;

  /* module library */
  GModule             *library;

  /* construct function */
  PluginConstructFunc  construct_func;

  /* module type */
  GType                type;
};



G_DEFINE_TYPE (WrapperModule, wrapper_module, G_TYPE_TYPE_MODULE)



static void
wrapper_module_class_init (WrapperModuleClass *klass)
{
  GObjectClass     *gobject_class;
  GTypeModuleClass *gtype_module_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = wrapper_module_dispose;
  gobject_class->finalize = wrapper_module_finalize;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = wrapper_module_load;
  gtype_module_class->unload = wrapper_module_unload;
}



static void
wrapper_module_init (WrapperModule *module)
{
  module->filename = NULL;
  module->library = NULL;
  module->construct_func = NULL;
  module->type = G_TYPE_NONE;
}



static void
wrapper_module_dispose (GObject *object)
{
  /* do nothing */
}



static void
wrapper_module_finalize (GObject *object)
{
  WrapperModule *module = WRAPPER_MODULE (object);

  /* cleanup */
  g_free (module->filename);

  (*G_OBJECT_CLASS (wrapper_module_parent_class)->finalize) (object);
}



static gboolean
wrapper_module_load (GTypeModule *type_module)
{
  WrapperModule        *module = WRAPPER_MODULE (type_module);
  PluginInitializeFunc  init_func;

  /* open the module */
  module->library = g_module_open (module->filename, G_MODULE_BIND_LOCAL);
  if (G_UNLIKELY (module->library == NULL))
    {
      /* print error and leave */
      g_critical ("Failed to load module \"%s\": %s.", module->filename,
                  g_module_error ());
      return FALSE;
    }

  /* try to link the contruct function */
  if (g_module_symbol (module->library, "__xpp_initialize",
                       (gpointer) &init_func))
    {
      /* initialize the plugin */
      module->type = init_func (type_module, NULL);
    }
  else if (!g_module_symbol (module->library, "__xpp_construct",
                             (gpointer) &module->construct_func))
    {
      /* print critical warning */
      g_critical ("Module \"%s\" lacks a plugin register function.",
                  module->filename);

      /* unload */
      wrapper_module_unload (type_module);

      return FALSE;
    }

  /* no need to unload modules in the wrapper, can only cause problems */
  g_module_make_resident (module->library);

  return TRUE;
}



static void
wrapper_module_unload (GTypeModule *type_module)
{
  WrapperModule *module = WRAPPER_MODULE (type_module);

  /* close the module */
  g_module_close (module->library);

  /* reset plugin state */
  module->library = NULL;
  module->construct_func = NULL;
  module->type = G_TYPE_NONE;
}



static void
wrapper_module_plugin_destroyed (gpointer  user_data,
                                 GObject  *where_the_plugin_was)
{
  /* unused the module */
  g_type_module_unuse (G_TYPE_MODULE (user_data));
}



WrapperModule *
wrapper_module_new (const gchar *filename)
{
  WrapperModule *module;
  
  module = g_object_new (WRAPPER_TYPE_MODULE, NULL);
  module->filename = g_strdup (filename);
  
  return module;
}



GtkWidget *
wrapper_module_new_provider (WrapperModule  *module,
                             GdkScreen      *screen,
                             const gchar    *name,
                             gint            unique_id,
                             const gchar    *display_name,
                             gchar         **arguments)
{
  GtkWidget *plugin = NULL;

  /* increase the module use count */
  if (g_type_module_use (G_TYPE_MODULE (module)))
    {
      if (module->type != G_TYPE_NONE)
        {g_message ("New plugin from object");
          /* create the object */
          plugin = g_object_new (module->type,
                                 "name", name,
                                 "unique-id", unique_id,
                                 "display-name", display_name,
                                 "arguments", arguments, NULL);
        }
      else if (module->construct_func != NULL)
        {g_message ("New plugin from construct function");
          /* create a new panel plugin */
          plugin = GTK_WIDGET ((*module->construct_func) (name, unique_id,
                                                          display_name,
                                                          arguments, screen));
        }

      if (G_LIKELY (plugin != NULL))
        {
          /* weak ref the plugin to unload the module */
          g_object_weak_ref (G_OBJECT (plugin),
                             wrapper_module_plugin_destroyed,
                             module);
        }
      else
        {
          /* unuse the module */
          g_type_module_unuse (G_TYPE_MODULE (module));
        }
    }

  return plugin;
}
