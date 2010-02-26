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
#include <panel/panel-plugin-external-46.h>

#define PANEL_PLUGINS_LIB_DIR LIBDIR G_DIR_SEPARATOR_S "panel-plugins"



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

enum _PanelModuleRunMode
{
  UNKNOWN,    /* Unset */
  INTERNAL,   /* plugin library will be loaded in the panel */
  WRAPPER,    /* external library with comunication through PanelPluginExternal */
  EXTERNAL_46 /* external executable with comunication through PanelPluginExternal46 */
};

struct _PanelModule
{
  GTypeModule __parent__;

  /* module type */
  PanelModuleRunMode   mode;

  /* filename to the library or executable
   * for an old 4.6 plugin */
  gchar               *filename;

  /* plugin information from the desktop file */
  gchar               *display_name;
  gchar               *comment;
  gchar               *icon_name;

  /* unique handling */
  guint                use_count;
  guint                is_unique : 1;

  /* module location (null for 4.6 plugins) */
  GModule             *library;

  /* for non-gobject plugin */
  PluginConstructFunc  construct_func;

  /* for gobject plugins */
  GType                plugin_type;
};



static GQuark module_quark = 0;



G_DEFINE_TYPE (PanelModule, panel_module, G_TYPE_TYPE_MODULE)



static void
panel_module_class_init (PanelModuleClass *klass)
{
  GObjectClass     *gobject_class;
  GTypeModuleClass *gtype_module_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = panel_module_dispose;
  gobject_class->finalize = panel_module_finalize;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = panel_module_load;
  gtype_module_class->unload = panel_module_unload;

  module_quark = g_quark_from_static_string ("panel-module");
}



static void
panel_module_init (PanelModule *module)
{
  module->mode = UNKNOWN;
  module->filename = NULL;
  module->display_name = NULL;
  module->comment = NULL;
  module->icon_name = NULL;
  module->use_count = 0;
  module->is_unique = FALSE;
  module->library = NULL;
  module->construct_func = NULL;
  module->plugin_type = G_TYPE_NONE;
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

  g_free (module->filename);
  g_free (module->display_name);
  g_free (module->comment);
  g_free (module->icon_name);

  (*G_OBJECT_CLASS (panel_module_parent_class)->finalize) (object);
}



static gboolean
panel_module_load (GTypeModule *type_module)
{
  PanelModule    *module = PANEL_MODULE (type_module);
  PluginInitFunc  init_func;
  gboolean        make_resident = TRUE;
  gpointer        foo;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), FALSE);
  panel_return_val_if_fail (module->mode == INTERNAL, FALSE);
  panel_return_val_if_fail (module->library == NULL, FALSE);
  panel_return_val_if_fail (module->plugin_type == G_TYPE_NONE, FALSE);
  panel_return_val_if_fail (module->construct_func == NULL, FALSE);

  /* open the module */
  module->library = g_module_open (module->filename, G_MODULE_BIND_LOCAL);
  if (G_UNLIKELY (module->library == NULL))
    {
      g_critical ("Failed to load module \"%s\": %s.",
                  module->filename,
                  g_module_error ());
      return FALSE;
    }

    /* check if there is a preinit function */
  if (g_module_symbol (module->library, "xfce_panel_module_preinit", &foo))
    {
      /* large message, but technically never shown to normal users */
      g_warning ("The plugin \"%s\" is marked as internal in the desktop file, "
                 "but the developer has defined an pre-init function, which is "
                 "not supported for internal plugins. " PACKAGE_NAME " will force "
                 "the plugin to run external.", module->filename);

      panel_module_unload (type_module);

      /* from now on, run this plugin in a wrapper */
      module->mode = WRAPPER;

      return FALSE;
    }

  /* try to link the contruct function */
  if (g_module_symbol (module->library, "xfce_panel_module_init", (gpointer) &init_func))
    {
      /* initialize the plugin */
      module->plugin_type = init_func (type_module, &make_resident);

      /* whether to make this plugin resident or not */
      if (make_resident)
        g_module_make_resident (module->library);
    }
  else if (!g_module_symbol (module->library, "xfce_panel_module_construct",
                             (gpointer) &module->construct_func))
    {
      g_critical ("Module \"%s\" lacks a plugin register function.",
                  module->filename);

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
  panel_return_if_fail (module->mode == INTERNAL);
  panel_return_if_fail (module->library != NULL);
  panel_return_if_fail (module->plugin_type != G_TYPE_NONE
                        || module->construct_func != NULL);

  g_module_close (module->library);

  /* reset plugin state */
  module->library = NULL;
  module->construct_func = NULL;
  module->plugin_type = G_TYPE_NONE;
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
  if (module->mode == INTERNAL)
    g_type_module_unuse (G_TYPE_MODULE (module));

  /* emit signal unique signal in the factory */
  if (module->is_unique)
    panel_module_factory_emit_unique_changed (module);
}



PanelModule *
panel_module_new_from_desktop_file (const gchar *filename,
                                    const gchar *name,
                                    gboolean     force_external)
{
  PanelModule *module = NULL;
  XfceRc      *rc;
  const gchar *module_name;
  gchar       *path;
  const gchar *module_exec;

  panel_return_val_if_fail (IS_STRING (filename), NULL);
  panel_return_val_if_fail (IS_STRING (name), NULL);

  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_critical ("Plugin %s: Unable to read from desktop file \"%s\"",
                  name, filename);
      return NULL;
    }

  if (!xfce_rc_has_group (rc, "Xfce Panel"))
    {
      g_critical ("Plugin %s: Desktop file \"%s\" has no "
                  "\"Xfce Panel\" group", name, filename);
      xfce_rc_close (rc);
      return NULL;
    }

  xfce_rc_set_group (rc, "Xfce Panel");

  /* read module location from the desktop file */
  module_name = xfce_rc_read_entry_untranslated (rc, "X-XFCE-Module", NULL);
  if (G_LIKELY (module_name != NULL))
    {
#ifndef NDEBUG
      if (xfce_rc_has_entry (rc, "X-XFCE-Module-Path"))
        {
          /* show a messsage if the old module path key still exists */
          g_message ("Plugin %s: The \"X-XFCE-Module-Path\" key is "
                     "ignored in \"%s\", the panel will look for the "
                     "module in " PANEL_PLUGINS_LIB_DIR ". See bug "
                     "#5455 why this decision was made", name, filename);
        }
#endif

      path = g_module_build_path (PANEL_PLUGINS_LIB_DIR, module_name);
      if (G_LIKELY (g_file_test (path, G_FILE_TEST_EXISTS)))
        {
          /* create new module */
          module = g_object_new (PANEL_TYPE_MODULE, NULL);
          module->filename = path;

          /* run mode of the module, by default everything runs in
           * the wrapper, unless defined otherwise */
          if (force_external || !xfce_rc_read_bool_entry (rc, "X-XFCE-Internal", FALSE))
            module->mode = WRAPPER;
          else
            module->mode = INTERNAL;
        }
      else
        {
          g_critical ("Plugin %s: There was no module found at \"%s\"",
                      name, path);
          g_free (path);
        }
    }
  else
    {
      /* yeah, we support ancient shizzle too... */
      module_exec = xfce_rc_read_entry_untranslated (rc, "X-XFCE-Exec", NULL);
      if (module_exec != NULL
          && g_path_is_absolute (module_exec)
          && g_file_test (module_exec, G_FILE_TEST_EXISTS))
        {
          module = g_object_new (PANEL_TYPE_MODULE, NULL);
          module->filename = g_strdup (module_exec);
          module->mode = EXTERNAL_46;
        }
      else
        {
          g_critical ("Plugin %s: There was no executable found at \"%s\"",
                      name, module_exec);
        }
    }

  if (G_LIKELY (module != NULL))
    {
      g_type_module_set_name (G_TYPE_MODULE (module), name);
      panel_assert (module->mode != UNKNOWN);

      /* read the remaining information */
      module->display_name = g_strdup (xfce_rc_read_entry (rc, "Name", name));
      module->comment = g_strdup (xfce_rc_read_entry (rc, "Comment", NULL));
      module->icon_name = g_strdup (xfce_rc_read_entry_untranslated (rc, "Icon", NULL));
      module->is_unique = xfce_rc_read_bool_entry (rc, "X-XFCE-Unique", FALSE);
    }

  xfce_rc_close (rc);

  return module;
}



GtkWidget *
panel_module_new_plugin (PanelModule  *module,
                         GdkScreen    *screen,
                         gint          unique_id,
                         gchar       **arguments)
{
  GtkWidget *plugin = NULL;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);
  panel_return_val_if_fail (module->mode != UNKNOWN, NULL);

  /* return null if the module is not usable (unique and already used) */
  if (G_UNLIKELY (!panel_module_is_usable (module)))
    return NULL;

  switch (module->mode)
    {
      case INTERNAL:
        if (g_type_module_use (G_TYPE_MODULE (module)))
          {
            if (module->plugin_type != G_TYPE_NONE)
              {
                /* plugin is build as an object, to use its gtype */
                plugin = g_object_new (module->plugin_type,
                                       "name", panel_module_get_name (module),
                                       "unique-id", unique_id,
                                       "display-name", module->display_name,
                                       "comment", module->comment,
                                       "arguments", arguments,
                                       NULL);
              }
            else if (module->construct_func != NULL)
              {
                /* create plugin using the 'old style' construct function */
                plugin = (*module->construct_func) (panel_module_get_name (module),
                                                    unique_id,
                                                    module->display_name,
                                                    module->comment,
                                                    arguments,
                                                    screen);
              }

            if (G_LIKELY (plugin != NULL))
              break;
            else
              g_type_module_unuse (G_TYPE_MODULE (module));
          }

        /* fall-through (make wrapper plugin), probably a plugin with
         * preinit_func which is not supported for internal plugins */

      case WRAPPER:
        plugin = panel_plugin_external_new (module, unique_id, arguments);
        break;

      case EXTERNAL_46:
        plugin = panel_plugin_external_46_new (module, unique_id, arguments);
        break;

      default:
        panel_assert_not_reached ();
        break;
    }

  if (G_LIKELY (plugin != NULL))
    {
      /* increase count */
      module->use_count++;

      /* handle module use count and unloading */
      g_object_weak_ref (G_OBJECT (plugin),
          panel_module_plugin_destroyed, module);

      /* emit unique-changed if the plugin is unique */
      if (module->is_unique)
        panel_module_factory_emit_unique_changed (module);

      /* add link to the module */
      g_object_set_qdata (G_OBJECT (plugin), module_quark, module);
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
  panel_return_val_if_fail (module->display_name == NULL
                            || g_utf8_validate (module->display_name, -1, NULL), NULL);

  return module->display_name;
}



const gchar *
panel_module_get_comment (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->comment == NULL
                            || g_utf8_validate (module->comment, -1, NULL), NULL);

  return module->comment;
}



const gchar *
panel_module_get_icon_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->icon_name == NULL
                            || g_utf8_validate (module->icon_name, -1, NULL), NULL);

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
