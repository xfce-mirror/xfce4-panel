/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>
#include <exo/exo.h>
#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-module.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-plugin-external.h>



static void      panel_module_class_init     (PanelModuleClass *klass);
static void      panel_module_init           (PanelModule      *plugin);
static void      panel_module_dispose        (GObject          *object);
static void      panel_module_finalize       (GObject          *object);
static gboolean  panel_module_load           (GTypeModule      *type_module);
static void      panel_module_unload         (GTypeModule      *type_module);
static void      panel_module_item_finalized (gpointer          user_data,
                                              GObject          *item);



struct _PanelModuleClass
{
  GTypeModuleClass __parent__;
};



struct _PanelModule
{
  GTypeModule __parent__;

  /* to plugin library */
  GModule             *library;

  /* plugin init function */
  PluginConstructFunc  construct_func;

  /* whether to run the plugin in the wrapper */
  guint                run_in_wrapper : 1;

  /* the library location */
  gchar               *filename;

  /* plugin information from the desktop file */
  gchar               *name;
  gchar               *comment;
  gchar               *icon_name;

  /* whether this plugin is unique (only 1 running instance) */
  guint                is_unique : 1;

  /* use count */
  guint                use_count;
};



G_DEFINE_TYPE (PanelModule, panel_module, G_TYPE_TYPE_MODULE);



static void
panel_module_class_init (PanelModuleClass *klass)
{
  GObjectClass *gobject_class;
  GTypeModuleClass *gtype_module_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = panel_module_dispose;
  gobject_class->finalize = panel_module_finalize;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = panel_module_load;
  gtype_module_class->unload = panel_module_unload;
}



static void
panel_module_init (PanelModule *module)
{
  /* initialize */
  module->library = NULL;
  module->construct_func = NULL;
  module->filename = NULL;
  module->run_in_wrapper = TRUE;
  module->name = NULL;
  module->comment = NULL;
  module->icon_name = NULL;
  module->is_unique = FALSE;
  module->use_count = 0;
}



static void
panel_module_dispose (GObject *object)
{
  /* Do nothing to avoid problems with dispose in GTypeModule when
   * types are registered.
   *
   * For us this is not a problem since the modules are released when
   * everything is destroyed. So we really want that last unref before
   * closing the application. */
}



static void
panel_module_finalize (GObject *object)
{
  PanelModule *module = PANEL_MODULE (object);

  /* cleanup */
  g_free (module->filename);
  g_free (module->name);
  g_free (module->comment);
  g_free (module->icon_name);

  (*G_OBJECT_CLASS (panel_module_parent_class)->finalize) (object);
}



static gboolean
panel_module_load (GTypeModule *type_module)
{
  PanelModule             *module = PANEL_MODULE (type_module);
  PluginRegisterTypesFunc  register_func;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), FALSE);

  /* load the module */
  module->library = g_module_open (module->filename, G_MODULE_BIND_LOCAL);
  if (G_UNLIKELY (module->library == NULL))
    {
      g_critical ("Failed to load plugin '%s': %s", panel_module_get_name (module), g_module_error ());

      return FALSE;
    }

  /* link the required construct function */
  if (!g_module_symbol (module->library, "xfce_panel_plugin_construct", (gpointer) &module->construct_func))
    {
      g_critical ("Plugin '%s' lacks required symbol: %s", panel_module_get_name (module), g_module_error ());

      /* unload */
      panel_module_unload (type_module);

      return FALSE;
    }

  /* run the type register function if available */
  if (g_module_symbol (module->library, "xfce_panel_plugin_register_types", (gpointer) &register_func))
    (*register_func) (type_module);

  return TRUE;
}



static void
panel_module_unload (GTypeModule *type_module)
{
  PanelModule *module = PANEL_MODULE (type_module);

  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));

  /* unload the library */
  g_module_close (module->library);

  /* reset plugin state */
  module->library = NULL;
  module->construct_func = NULL;
}



static void
panel_module_item_finalized (gpointer  user_data,
                             GObject  *item)
{
  PanelModule *module = PANEL_MODULE (user_data);

  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));
  panel_return_if_fail (module->use_count > 0);
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));

  /* decrease counter */
  module->use_count--;

  /* unuse the library if the plugin runs internal */
  if (module->run_in_wrapper == FALSE)
    g_type_module_unuse (G_TYPE_MODULE (module));

  /* emit signal unique signal in the factory */
  if (module->is_unique)
    panel_module_factory_emit_unique_changed (module);
}



PanelModule *
panel_module_new_from_desktop_file (const gchar *filename,
                                    const gchar *name)
{
  PanelModule *module = NULL;
  XfceRc      *rc;
  const gchar *module_name;
  const gchar *directory;
  const gchar *value;
  gchar       *path;

  panel_return_val_if_fail (filename != NULL && *filename != '\0', NULL);

  /* open the desktop file */
  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_LIKELY (rc != NULL && xfce_rc_has_group (rc, "Xfce Panel")))
    {
      /* set the xfce panel group */
      xfce_rc_set_group (rc, "Xfce Panel");

      /* read library location from the desktop file */
      module_name = xfce_rc_read_entry (rc, "X-XFCE-Module", NULL);
      directory = xfce_rc_read_entry (rc, "X-XFCE-Module-Path", NULL);

      if (G_LIKELY (module_name != NULL && directory != NULL))
        {
          /* build the module path */
          path = g_module_build_path (directory, module_name);

          /* test if the library exists */
          if (G_LIKELY (g_file_test (path, G_FILE_TEST_EXISTS)))
            {
              /* create new module */
              module = g_object_new (PANEL_TYPE_MODULE, NULL);

              /* set library location */
              module->filename = path;
            }
          else
            {
              /* cleanup */
              g_free (path);
            }
        }

      /* read the remaining information */
      if (G_LIKELY (module != NULL))
        {
          /* set the module name */
          g_type_module_set_name (G_TYPE_MODULE (module), name);

          /* read the plugin name */
          value = xfce_rc_read_entry (rc, "Name", NULL);
          module->name = g_strdup (value);

          /* read the plugin comment */
          value = xfce_rc_read_entry (rc, "Comment", NULL);
          module->comment = g_strdup (value);

          /* read the plugin icon */
          value = xfce_rc_read_entry (rc, "Icon", NULL);
          module->icon_name = g_strdup (value);

          /* whether the plugin is unique */
          module->is_unique = xfce_rc_read_bool_entry (rc, "X-XFCE-Unique", FALSE);

          /* whether to run the plugin external */
          module->run_in_wrapper = xfce_rc_read_bool_entry (rc, "X-XFCE-External", TRUE);
        }
      else if (xfce_rc_has_entry (rc, "X-XFCE-Exec"))
        {
          /* old external plugin, not usable anymore */
          //g_message ("The plugin from desktop file \"%s\" should be ported to an internal plugin", filename);
        }
      else
        {
          /* print warning */
          g_warning ("Failed to create a plugin from desktop file \"%s\"", filename);
        }

      /* close rc file */
      xfce_rc_close (rc);
    }

  return module;
}



XfcePanelPluginProvider *
panel_module_create_plugin (PanelModule  *module,
                            GdkScreen    *screen,
                            const gchar  *name,
                            const gchar  *id,
                            gchar       **arguments)
{
  XfcePanelPluginProvider *plugin = NULL;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (name != NULL && *name != '\0', NULL);
  panel_return_val_if_fail (id != NULL && *id != '\0', NULL);
  panel_return_val_if_fail (exo_str_is_equal (name, G_TYPE_MODULE (module)->name), NULL);

  /* return null if the module is not usable (unique and already used) */
  if (G_UNLIKELY (panel_module_is_usable (module) == FALSE))
    return NULL;

  if (module->run_in_wrapper)
    {
      /* create external plugin */
      plugin = panel_plugin_external_new (module, name, id, arguments);
    }
  else
    {
      /* increase the module use count */
      g_type_module_use (G_TYPE_MODULE (module));

      if (G_LIKELY (module->library))
        {
          /* debug check */
          panel_return_val_if_fail (module->construct_func != NULL, NULL);

          /* create a new panel plugin */
          plugin = (*module->construct_func) (name, id, module->name, arguments, screen);
        }
      else
        {
          /* this should never happen */
          panel_assert_not_reached ();
        }
    }

  if (G_LIKELY (plugin))
    {
      /* increase count */
      module->use_count++;

      /* handle module use count and unloading */
      g_object_weak_ref (G_OBJECT (plugin), panel_module_item_finalized, module);

      /* emit unique-changed if the plugin is unique */
      if (module->is_unique)
        panel_module_factory_emit_unique_changed (module);
    }
  else if (module->run_in_wrapper == FALSE)
    {
      /* decrease the use count since loading failed somehow */
      g_type_module_unuse (G_TYPE_MODULE (module));
    }

  return plugin;
}



const gchar *
panel_module_get_internal_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);

  return G_TYPE_MODULE (module)->name;
}



const gchar *
panel_module_get_library_filename (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);

  return module->filename;
}



const gchar *
panel_module_get_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (module->name == NULL || g_utf8_validate (module->name, -1, NULL), NULL);
  panel_return_val_if_fail (module->name != NULL || g_utf8_validate (G_TYPE_MODULE (module)->name, -1, NULL), NULL);

  return module->name ? module->name : G_TYPE_MODULE (module)->name;
}



const gchar *
panel_module_get_comment (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->comment == NULL || g_utf8_validate (module->comment, -1, NULL), NULL);

  return module->comment;
}



const gchar *
panel_module_get_icon_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->icon_name == NULL || g_utf8_validate (module->icon_name, -1, NULL), NULL);

  return module->icon_name;
}



gboolean
panel_module_is_valid (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);

  return g_file_test (module->filename, G_FILE_TEST_EXISTS);
}



gboolean
panel_module_is_usable (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);

  /* whether the module is usable */
  return module->is_unique ? module->use_count == 0 : TRUE;
}
