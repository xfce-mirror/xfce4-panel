/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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
#include "config.h"
#endif

#include "panel-module-factory.h"
#include "panel-module.h"
#include "panel-plugin-external-wrapper.h"

#include "common/panel-debug.h"
#include "common/panel-private.h"

#include <glib/gstdio.h>
#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>

#ifdef HAVE_GTK_LAYER_SHELL
#include <gtk-layer-shell.h>
#else
#define gtk_layer_is_supported() FALSE
#endif



typedef enum _PanelModuleUnique PanelModuleUnique;



static void
panel_module_dispose (GObject *object);
static gboolean
panel_module_load (GTypeModule *type_module);
static void
panel_module_unload (GTypeModule *type_module);
static void
panel_module_plugin_destroyed (gpointer user_data,
                               GObject *where_the_plugin_was);



enum _PanelModuleUnique
{
  UNIQUE_FALSE,
  UNIQUE_TRUE,
  UNIQUE_SCREEN
};

struct _PanelModule
{
  GTypeModule __parent__;

  /* module type */
  PanelModuleRunMode mode;

  /* filename of the library */
  gchar *filename;

  /* plugin information from the desktop file */
  gchar *display_name;
  gchar *comment;
  gchar *icon_name;

  /* unique handling */
  guint use_count;
  PanelModuleUnique unique_mode;

  /* module location */
  GModule *library;

  /* for non-gobject plugin */
  PluginConstructFunc construct_func;

  /* for gobject plugins */
  GType plugin_type;

  /* for wrapper plugins */
  gchar *api;
};



static GQuark module_quark = 0;



G_DEFINE_FINAL_TYPE (PanelModule, panel_module, G_TYPE_TYPE_MODULE)



static void
panel_module_class_init (PanelModuleClass *klass)
{
  GObjectClass *gobject_class;
  GTypeModuleClass *gtype_module_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = panel_module_dispose;

  gtype_module_class = G_TYPE_MODULE_CLASS (klass);
  gtype_module_class->load = panel_module_load;
  gtype_module_class->unload = panel_module_unload;

  module_quark = g_quark_from_static_string ("panel-module");
}



static void
panel_module_init (PanelModule *module)
{
  module->mode = PANEL_MODULE_RUN_MODE_NONE;
  module->filename = NULL;
  module->display_name = NULL;
  module->comment = NULL;
  module->icon_name = NULL;
  module->use_count = 0;
  module->unique_mode = UNIQUE_FALSE;
  module->library = NULL;
  module->construct_func = NULL;
  module->plugin_type = G_TYPE_NONE;
  module->api = g_strdup (LIBXFCE4PANEL_VERSION_API);
}



static void
panel_module_dispose (GObject *object)
{
  PanelModule *module = PANEL_MODULE (object);

  if (module->api != NULL)
    {
      g_free (module->filename);
      g_free (module->display_name);
      g_free (module->comment);
      g_free (module->icon_name);
      g_free (module->api);
      module->api = NULL;
      if (module->plugin_type != G_TYPE_NONE)
        {
          /* a module containing type implementations must exist forever */
          g_object_ref (module);
          return;
        }
    }

  G_OBJECT_CLASS (panel_module_parent_class)->dispose (object);
}



static gboolean
panel_module_load (GTypeModule *type_module)
{
  PanelModule *module = PANEL_MODULE (type_module);
  PluginInitFunc init_func;
  gboolean make_resident = TRUE;
  gpointer foo;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), FALSE);
  panel_return_val_if_fail (module->mode == PANEL_MODULE_RUN_MODE_INTERNAL, FALSE);
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
                 "the plugin to run external.",
                 module->filename);

      panel_module_unload (type_module);

      /* from now on, run this plugin in a wrapper */
      module->mode = PANEL_MODULE_RUN_MODE_EXTERNAL;
      g_free (module->api);
      module->api = g_strdup (LIBXFCE4PANEL_VERSION_API);

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
  panel_return_if_fail (module->mode == PANEL_MODULE_RUN_MODE_INTERNAL);
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
panel_module_plugin_destroyed (gpointer user_data,
                               GObject *where_the_plugin_was)
{
  PanelModule *module = PANEL_MODULE (user_data);

  panel_return_if_fail (PANEL_IS_MODULE (module));
  panel_return_if_fail (G_IS_TYPE_MODULE (module));
  panel_return_if_fail (module->use_count > 0);

  /* decrease counter */
  module->use_count--;

  /* unuse the library if the plugin runs internal */
  if (module->mode == PANEL_MODULE_RUN_MODE_INTERNAL)
    g_type_module_unuse (G_TYPE_MODULE (module));

  /* emit signal unique signal in the factory */
  if (module->unique_mode != UNIQUE_FALSE)
    panel_module_factory_emit_unique_changed (module);
}



PanelModule *
panel_module_new_from_desktop_file (const gchar *filename,
                                    const gchar *name,
                                    const gchar *libdir,
                                    PanelModuleRunMode forced_mode)
{
  PanelModule *module = NULL;
  XfceRc *rc;
  const gchar *module_name;
  gchar *path;
  const gchar *module_unique;
  gboolean found;

  panel_return_val_if_fail (!xfce_str_is_empty (filename), NULL);
  panel_return_val_if_fail (!xfce_str_is_empty (name), NULL);

  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_UNLIKELY (rc == NULL))
    {
      g_critical ("Plugin %s: Unable to read from desktop file \"%s\"",
                  name, filename);
      return NULL;
    }

  if (!xfce_rc_has_group (rc, "Xfce Panel"))
    {
      g_critical ("Plugin %s: Desktop file \"%s\" has no \"Xfce Panel\" group",
                  name, filename);
      xfce_rc_close (rc);
      return NULL;
    }

  if (g_strcmp0 (xfce_rc_read_entry (rc, "X-XFCE-API", "1.0"), "2.0") != 0)
    {
      g_critical ("Plugin %s: The Desktop file %s requested the Gtk2 API (v1.0), which is "
                  "no longer supported.",
                  name, filename);
      xfce_rc_close (rc);
      return NULL;
    }

  xfce_rc_set_group (rc, "Xfce Panel");

  /* read module location from the desktop file */
  module_name = xfce_rc_read_entry_untranslated (rc, "X-XFCE-Module", NULL);
  if (G_LIKELY (module_name != NULL))
    {
      path = g_module_build_path (libdir, module_name);
      found = g_file_test (path, G_FILE_TEST_EXISTS);

      if (G_LIKELY (found))
        {
          /* create new module */
          module = g_object_new (PANEL_TYPE_MODULE, NULL);
          module->filename = path;

          /* run mode of the module, by default everything runs in
           * the wrapper, unless defined otherwise or unsupported */
          if (forced_mode != PANEL_MODULE_RUN_MODE_INTERNAL
              && (WINDOWING_IS_X11 () || gtk_layer_is_supported ())
              && (forced_mode == PANEL_MODULE_RUN_MODE_EXTERNAL
                  || !xfce_rc_read_bool_entry (rc, "X-XFCE-Internal", FALSE)))
            {
              module->mode = PANEL_MODULE_RUN_MODE_EXTERNAL;
              g_free (module->api);
              module->api = g_strdup (xfce_rc_read_entry (rc, "X-XFCE-API", LIBXFCE4PANEL_VERSION_API));
            }
          else
            module->mode = PANEL_MODULE_RUN_MODE_INTERNAL;
        }
      else
        {
          if (g_strcmp0 (libdir, LIBDIR) == 0)
            g_critical ("Plugin %s: There was no module found at \"%s\"", name, path);
          else
            panel_debug_filtered (PANEL_DEBUG_MODULE, "Plugin %s: There was no module found at \"%s\"", name, path);

          g_free (path);
        }
    }

  if (G_LIKELY (module != NULL))
    {
      g_type_module_set_name (G_TYPE_MODULE (module), name);
      panel_assert (module->mode != PANEL_MODULE_RUN_MODE_NONE);

      /* read the remaining information */
      module->display_name = g_strdup (xfce_rc_read_entry (rc, "Name", name));
      module->comment = g_strdup (xfce_rc_read_entry (rc, "Comment", NULL));
      module->icon_name = g_strdup (xfce_rc_read_entry_untranslated (rc, "Icon", NULL));

      module_unique = xfce_rc_read_entry (rc, "X-XFCE-Unique", NULL);
      if (G_LIKELY (module_unique == NULL))
        module->unique_mode = UNIQUE_FALSE;
      else if (strcasecmp (module_unique, "screen") == 0)
        module->unique_mode = UNIQUE_SCREEN;
      else if (strcasecmp (module_unique, "true") == 0)
        module->unique_mode = UNIQUE_TRUE;
      else
        module->unique_mode = UNIQUE_FALSE;

      panel_debug_filtered (PANEL_DEBUG_MODULE, "new module %s, filename=%s, internal=%s",
                            name, module->filename,
                            PANEL_DEBUG_BOOL (module->mode == PANEL_MODULE_RUN_MODE_INTERNAL));
    }

  xfce_rc_close (rc);

  return module;
}



GtkWidget *
panel_module_new_plugin (PanelModule *module,
                         GdkScreen *screen,
                         gint unique_id,
                         gchar **arguments)
{
  GtkWidget *plugin = NULL;
  const gchar *debug_type = NULL;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);
  panel_return_val_if_fail (module->mode != PANEL_MODULE_RUN_MODE_NONE, NULL);

  /* return null if the module is not usable (unique and already used) */
  if (G_UNLIKELY (!panel_module_is_usable (module, screen)))
    return NULL;

  switch (module->mode)
    {
    case PANEL_MODULE_RUN_MODE_INTERNAL:
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

              debug_type = "object-type";
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

              debug_type = "construct-func";
            }

          if (G_LIKELY (plugin != NULL))
            break;
          else
            g_type_module_unuse (G_TYPE_MODULE (module));
        }

      /* fall-through (make wrapper plugin), probably a plugin with
       * preinit_func which is not supported for internal plugins
       * note: next comment tells GCC7 to ignore the fallthrough */
      /* fall through */

    case PANEL_MODULE_RUN_MODE_EXTERNAL:
      plugin = panel_plugin_external_wrapper_new (module, unique_id, arguments);
      debug_type = "external-wrapper";
      break;

    default:
      panel_assert_not_reached ();
      break;
    }

  if (G_LIKELY (plugin != NULL))
    {
      /* increase count */
      module->use_count++;

      panel_debug (PANEL_DEBUG_MODULE, "new item (type=%s, name=%s, id=%d)",
                   debug_type, panel_module_get_name (module), unique_id);

      /* handle module use count and unloading */
      g_object_weak_ref (G_OBJECT (plugin), panel_module_plugin_destroyed, module);

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
                              || g_utf8_validate (module->display_name, -1, NULL),
                            NULL);

  return module->display_name;
}



const gchar *
panel_module_get_comment (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->comment == NULL
                              || g_utf8_validate (module->comment, -1, NULL),
                            NULL);

  return module->comment;
}



const gchar *
panel_module_get_icon_name (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (module->icon_name == NULL
                              || g_utf8_validate (module->icon_name, -1, NULL),
                            NULL);

  return module->icon_name;
}



const gchar *
panel_module_get_api (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (G_IS_TYPE_MODULE (module), NULL);

  return module->api;
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
panel_module_is_unique (PanelModule *module)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);

  return module->unique_mode != UNIQUE_FALSE;
}



gboolean
panel_module_is_usable (PanelModule *module,
                        GdkScreen *screen)
{
  PanelModuleFactory *factory;
  GSList *plugins, *li;
  gboolean usable = TRUE;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  if (module->use_count > 0
      && module->unique_mode == UNIQUE_TRUE)
    return FALSE;

  if (module->use_count > 0
      && module->unique_mode == UNIQUE_SCREEN)
    {
      factory = panel_module_factory_get ();
      plugins = panel_module_factory_get_plugins (factory, panel_module_get_name (module));

      /* search existing plugins if one of them runs on this screen */
      for (li = plugins; usable && li != NULL; li = li->next)
        if (screen == gtk_widget_get_screen (GTK_WIDGET (li->data)))
          usable = FALSE;

      g_slist_free (plugins);
      g_object_unref (G_OBJECT (factory));
    }

  return usable;
}
