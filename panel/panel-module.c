/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-module.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-plugin-external.h>



static void      panel_module_dispose          (GObject          *object);
static void      panel_module_finalize         (GObject          *object);
static gboolean  panel_module_load             (GTypeModule      *type_module);
static void      panel_module_unload           (GTypeModule      *type_module);
static void      panel_module_plugin_destroyed (gpointer          user_data,
                                                GObject          *where_the_plugin_was);



struct _PanelModuleClass
{
  GTypeModuleClass __parent__;
};



struct _PanelModule
{
  GTypeModule __parent__;

  /* to plugin library */
  GModule             *library;

  PluginConstructFunc  construct_func;

  /* plugin type */
  GType                type;

  /* whether to run the plugin in the wrapper */
  guint                run_in_wrapper : 1;

  /* the library location */
  gchar               *filename;

  /* plugin information from the desktop file */
  gchar               *display_name;
  gchar               *comment;
  gchar               *icon_name;

  /* whether this plugin is unique (only 1 running instance) */
  guint                is_unique : 1;

  /* use count */
  guint                use_count;
};


static GQuark module_quark = 0;



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

  /* initialize the quark */
  module_quark = g_quark_from_static_string ("panel-module");
}



static void
panel_module_init (PanelModule *module)
{
  /* initialize */
  module->library = NULL;
  module->construct_func = NULL;
  module->type = G_TYPE_NONE;
  module->filename = NULL;
  module->run_in_wrapper = TRUE;
  module->display_name = NULL;
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
  g_free (module->display_name);
  g_free (module->comment);
  g_free (module->icon_name);

  (*G_OBJECT_CLASS (panel_module_parent_class)->finalize) (object);
}



static gboolean
panel_module_load (GTypeModule *type_module)
{
  PanelModule          *module = PANEL_MODULE (type_module);
  PluginInitializeFunc  init_func;
  gboolean              make_resident = TRUE;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), FALSE);

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
  if (g_module_symbol (module->library, "__xpp_initialize", (gpointer) &init_func))
    {
      /* initialize the plugin */
      module->type = init_func (type_module, &make_resident);

      /* whether to make this plugin resident or not */
      if (make_resident)
        g_module_make_resident (module->library);
    }
  else if (!g_module_symbol (module->library, "__xpp_construct",
                             (gpointer) &module->construct_func))
    {
      /* print critical warning */
      g_critical ("Module \"%s\" lacks a plugin register function.",
                  module->filename);

      /* unload */
      panel_module_unload (type_module);

      return FALSE;
    }

  return TRUE;
}



static void
panel_module_unload (GTypeModule *type_module)
{
  PanelModule *module = PANEL_MODULE (type_module);

  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));

  /* close the module */
  g_module_close (module->library);

  /* reset plugin state */
  module->library = NULL;
  module->construct_func = NULL;
  module->type = G_TYPE_NONE;
}



static void
panel_module_plugin_destroyed (gpointer  user_data,
                               GObject  *where_the_plugin_was)
{
  PanelModule *module = PANEL_MODULE (user_data);

  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));
  panel_return_if_fail (module->use_count > 0);

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

  panel_return_val_if_fail (IS_STRING (filename), NULL);

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
          value = xfce_rc_read_entry (rc, "Name", name);
          module->display_name = g_strdup (value);

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
          g_message ("The plugin from desktop file \"%s\" should "
                     "be ported to an internal plugin", filename);
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



GtkWidget *
panel_module_new_plugin (PanelModule  *module,
                         GdkScreen    *screen,
                         gint          unique_id,
                         gchar       **arguments)
{
  GtkWidget   *plugin = NULL;
  const gchar *name;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);

  /* return null if the module is not usable (unique and already used) */
  if (G_UNLIKELY (panel_module_is_usable (module) == FALSE))
    return NULL;

  /* get the internal plugin name */
  name = panel_module_get_name (module);

  if (module->run_in_wrapper)
    {
      /* create external plugin */
      plugin = panel_plugin_external_new (module, unique_id, arguments);
    }
  else
    {
      /* increase the module use count */
      if (g_type_module_use (G_TYPE_MODULE (module)))
        {
          if (module->type != G_TYPE_NONE)
            {
              plugin = g_object_new (module->type,
                                     "name", name,
                                     "unique-id", unique_id,
                                     "display-name", module->display_name,
                                     "comment", module->comment,
                                     "arguments", arguments,
                                     NULL);
            }
          else if (module->construct_func != NULL)
            {
              /* create a new panel plugin */
              plugin = (*module->construct_func) (name,
                                                  unique_id,
                                                  module->display_name,
                                                  module->comment,
                                                  arguments,
                                                  screen);
            }
        }
    }

  if (G_LIKELY (plugin != NULL))
    {
      /* increase count */
      module->use_count++;

      /* handle module use count and unloading */
      g_object_weak_ref (G_OBJECT (plugin), panel_module_plugin_destroyed, module);

      /* emit unique-changed if the plugin is unique */
      if (module->is_unique)
        panel_module_factory_emit_unique_changed (module);

      /* add link to the module */
      g_object_set_qdata (G_OBJECT (plugin), module_quark, module);
    }
  else if (module->run_in_wrapper == FALSE)
    {
      /* decrease the use count since loading failed somehow */
      g_type_module_unuse (G_TYPE_MODULE (module));
    }

  return plugin;
}



const gchar *
panel_module_get_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);

  return G_TYPE_MODULE (module)->name;
}



const gchar *
panel_module_get_filename (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);

  return module->filename;
}



const gchar *
panel_module_get_display_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (g_utf8_validate (module->display_name, -1, NULL), NULL);

  return module->display_name;
}



const gchar *
panel_module_get_comment (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (g_utf8_validate (module->comment, -1, NULL), NULL);

  return module->comment;
}



const gchar *
panel_module_get_icon_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (g_utf8_validate (module->icon_name, -1, NULL), NULL);

  return module->icon_name;
}



PanelModule *
panel_module_get_from_plugin_provider (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  /* return the panel module */
  return g_object_get_qdata (G_OBJECT (provider), module_quark);
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
