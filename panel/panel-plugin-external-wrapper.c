/*
 * Copyright (C) 2017 Ali Abdallah <ali@xfce.org>
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
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <gdk/gdk.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <common/panel-dbus.h>
#include <common/panel-debug.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-module.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-plugin-external-wrapper.h>
#include <panel/panel-plugin-external-wrapper-x11.h>
#include <panel/panel-plugin-external-wrapper-exported.h>
#include <panel/panel-window.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-marshal.h>



#define WRAPPER_BIN HELPERDIR G_DIR_SEPARATOR_S "wrapper"



#define get_instance_private(instance) \
  panel_plugin_external_wrapper_get_instance_private (PANEL_PLUGIN_EXTERNAL_WRAPPER (instance))

static void       panel_plugin_external_wrapper_constructed              (GObject                        *object);
static void       panel_plugin_external_wrapper_finalize                 (GObject                        *object);
static void       panel_plugin_external_wrapper_set_properties           (PanelPluginExternal            *external,
                                                                          GSList                         *properties);
static gchar    **panel_plugin_external_wrapper_get_argv                 (PanelPluginExternal            *external,
                                                                          gchar                         **arguments);
static gboolean   panel_plugin_external_wrapper_remote_event             (PanelPluginExternal            *external,
                                                                          const gchar                    *name,
                                                                          const GValue                   *value,
                                                                          guint                          *handle);
static gboolean   panel_plugin_external_wrapper_dbus_provider_signal     (XfcePanelPluginWrapperExported *skeleton,
                                                                          GDBusMethodInvocation          *invocation,
                                                                          XfcePanelPluginProviderSignal   provider_signal,
                                                                          PanelPluginExternalWrapper     *wrapper);
static gboolean   panel_plugin_external_wrapper_dbus_remote_event_result (XfcePanelPluginWrapperExported *skeleton,
                                                                          GDBusMethodInvocation          *invocation,
                                                                          guint                           handle,
                                                                          gboolean                        result,
                                                                          PanelPluginExternalWrapper     *wrapper);



typedef struct _PanelPluginExternalWrapperPrivate
{
  XfcePanelPluginWrapperExported *skeleton;
  GDBusConnection                *connection;
} PanelPluginExternalWrapperPrivate;

enum
{
  REMOTE_EVENT_RESULT,
  LAST_SIGNAL
};



static guint external_signals[LAST_SIGNAL];



G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (PanelPluginExternalWrapper, panel_plugin_external_wrapper, PANEL_TYPE_PLUGIN_EXTERNAL)



static void
panel_plugin_external_wrapper_class_init (PanelPluginExternalWrapperClass *klass)
{
  GObjectClass             *gobject_class;
  PanelPluginExternalClass *plugin_external_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_wrapper_finalize;
  gobject_class->constructed = panel_plugin_external_wrapper_constructed;

  plugin_external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  plugin_external_class->get_argv = panel_plugin_external_wrapper_get_argv;
  plugin_external_class->set_properties = panel_plugin_external_wrapper_set_properties;
  plugin_external_class->remote_event = panel_plugin_external_wrapper_remote_event;

  external_signals[REMOTE_EVENT_RESULT] =
    g_signal_new (g_intern_static_string ("remote-event-result"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _panel_marshal_VOID__UINT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT, G_TYPE_BOOLEAN);
}



static void
panel_plugin_external_wrapper_init (PanelPluginExternalWrapper *wrapper)
{
}



static void
panel_plugin_external_wrapper_constructed (GObject *object)
{
  PanelPluginExternalWrapperPrivate *priv = get_instance_private (object);
  gchar *path;
  GError *error = NULL;

  priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL,  &error);
  if (G_LIKELY (priv->connection != NULL))
    {
      priv->skeleton = xfce_panel_plugin_wrapper_exported_skeleton_new ();

      /* register the object in dbus, the wrapper will monitor this object */
      panel_return_if_fail (PANEL_PLUGIN_EXTERNAL (object)->unique_id != -1);
      path = g_strdup_printf (PANEL_DBUS_WRAPPER_PATH, PANEL_PLUGIN_EXTERNAL (object)->unique_id);
      g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->skeleton),
                                        priv->connection,
                                        path,
                                        &error);

      if (G_UNLIKELY (error != NULL))
        {
          g_critical ("Failed to export object at path %s: %s", path, error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_connect (priv->skeleton, "handle_provider_signal",
                            G_CALLBACK (panel_plugin_external_wrapper_dbus_provider_signal), object);
          g_signal_connect (priv->skeleton, "handle_remote_event_result",
                            G_CALLBACK (panel_plugin_external_wrapper_dbus_remote_event_result), object);

          panel_debug (PANEL_DEBUG_EXTERNAL, "Exported object at path %s", path);
        }

      g_free (path);
    }
  else
    {
      g_critical ("Failed to get D-Bus session bus: %s", error->message);
      g_error_free (error);
    }

  G_OBJECT_CLASS (panel_plugin_external_wrapper_parent_class)->constructed (object);
}



static void
panel_plugin_external_wrapper_finalize (GObject *object)
{
  PanelPluginExternalWrapperPrivate *priv = get_instance_private (object);

  if (priv->skeleton != NULL)
    g_object_unref (priv->skeleton);
  if (priv->connection != NULL)
    g_object_unref (priv->connection);

  (*G_OBJECT_CLASS (panel_plugin_external_wrapper_parent_class)->finalize) (object);
}



static gchar **
panel_plugin_external_wrapper_get_argv (PanelPluginExternal   *external,
                                        gchar               **arguments)
{
  guint   i, argc = PLUGIN_ARGV_ARGUMENTS;
  gchar **argv;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_WRAPPER (external), NULL);
  panel_return_val_if_fail (PANEL_IS_MODULE (external->module), NULL);

  /* add the number of arguments to the argc count */
  if (G_UNLIKELY (arguments != NULL))
    argc += g_strv_length (arguments);

  /* setup the basic argv */
  argv = g_new0 (gchar *, argc + 1);
  argv[PLUGIN_ARGV_0] = g_strjoin ("-", WRAPPER_BIN, panel_module_get_api (external->module), NULL);
  argv[PLUGIN_ARGV_FILENAME] = g_strdup (panel_module_get_filename (external->module));
  argv[PLUGIN_ARGV_UNIQUE_ID] = g_strdup_printf ("%d", external->unique_id);;
  argv[PLUGIN_ARGV_NAME] = g_strdup (panel_module_get_name (external->module));
  argv[PLUGIN_ARGV_DISPLAY_NAME] = g_strdup (panel_module_get_display_name (external->module));
  argv[PLUGIN_ARGV_COMMENT] = g_strdup (panel_module_get_comment (external->module));

  /* append the arguments */
  if (G_UNLIKELY (arguments != NULL))
    {
      for (i = 0; arguments[i] != NULL; i++)
        argv[i + PLUGIN_ARGV_ARGUMENTS] = g_strdup (arguments[i]);
    }

  return argv;
}



static GVariant *
panel_plugin_external_wrapper_gvalue_prop_to_gvariant (const GValue *value)
{
  const GVariantType *type = NULL;
  GVariant           *variant = NULL;

  switch (G_VALUE_TYPE(value))
    {
    case G_TYPE_INT:
      type = G_VARIANT_TYPE_INT32;
      break;
    case G_TYPE_INT64:
      type = G_VARIANT_TYPE_INT64;
      break;
    case G_TYPE_UINT:
      type = G_VARIANT_TYPE_UINT32;
      break;
    case G_TYPE_UINT64:
      type = G_VARIANT_TYPE_UINT64;
      break;
    case G_TYPE_BOOLEAN:
      type = G_VARIANT_TYPE_BOOLEAN;
      break;
    case G_TYPE_DOUBLE:
      type = G_VARIANT_TYPE_DOUBLE;
      break;
    case G_TYPE_STRING:
      type = G_VARIANT_TYPE_STRING;
      break;
    /* only throw a warning (instead of an assertion) here as otherwise invalid
       types sent to the panel via dbus would let the panel crash */
    default:
      g_warn_if_reached ();
    }

  if (type)
    variant = g_dbus_gvalue_to_gvariant(value, type);

  return variant;
}



static void
panel_plugin_external_wrapper_set_properties (PanelPluginExternal *external,
                                              GSList              *properties)
{
  PanelPluginExternalWrapperPrivate *priv = get_instance_private (external);
  GVariantBuilder builder;
  PluginProperty *property;
  GSList *li;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);

  /* put properties in a dbus-suitable array for the wrapper */
  for (li = properties; li != NULL; li = li->next)
    {
      GVariant *variant;

      property = li->data;

      variant = panel_plugin_external_wrapper_gvalue_prop_to_gvariant (&property->value);
      if (G_LIKELY(variant))
        {
          g_variant_builder_add (&builder, "(uv)", property->type, variant);
          g_variant_unref (variant);
        }
      else
        {
          g_warning ("Failed to convert wrapper property from gvalue:%s to gvariant", G_VALUE_TYPE_NAME(&property->value));
          return;
        }
    }

  /* send array to the wrapper */
  g_dbus_connection_emit_signal (priv->connection,
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (priv->skeleton)),
                                 PANEL_DBUS_WRAPPER_INTERFACE,
                                 "Set",
                                 g_variant_builder_end (&builder),
                                 NULL);
}



static gboolean
panel_plugin_external_wrapper_remote_event (PanelPluginExternal *external,
                                            const gchar         *name,
                                            const GValue        *value,
                                            guint               *handle)
{
  PanelPluginExternalWrapperPrivate *priv = get_instance_private (external);
  GVariant *variant;
  static guint handle_counter = 0;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_WRAPPER (external), TRUE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external), TRUE);
  panel_return_val_if_fail (value == NULL || G_IS_VALUE (value), FALSE);

  if (G_UNLIKELY (handle_counter > G_MAXUINT - 2))
    handle_counter = 0;
  *handle = ++handle_counter;

  if (value == NULL)
    variant = g_variant_new_variant (g_variant_new_byte ('\0'));
  else
    variant = panel_plugin_external_wrapper_gvalue_prop_to_gvariant (value);
  if (variant == NULL)
    {
      g_warning ("Failed to convert value from gvalue:%s to gvariant", G_VALUE_TYPE_NAME(value));
      variant = g_variant_new_variant (g_variant_new_byte ('\0'));
    }

  g_dbus_connection_emit_signal (priv->connection,
                                 NULL,
                                 g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (priv->skeleton)),
                                 PANEL_DBUS_WRAPPER_INTERFACE,
                                 "RemoteEvent",
                                 g_variant_new ("(svu)",
                                                name,
                                                variant,
                                                *handle),
                                 NULL);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}


static gboolean
panel_plugin_external_wrapper_dbus_provider_signal (XfcePanelPluginWrapperExported *skeleton,
                                                    GDBusMethodInvocation          *invocation,
                                                    XfcePanelPluginProviderSignal   provider_signal,
                                                    PanelPluginExternalWrapper     *wrapper)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (wrapper), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (wrapper), FALSE);

  switch (provider_signal)
    {
    case PROVIDER_SIGNAL_SHOW_CONFIGURE:
      PANEL_PLUGIN_EXTERNAL (wrapper)->show_configure = TRUE;
      break;

    case PROVIDER_SIGNAL_SHOW_ABOUT:
      PANEL_PLUGIN_EXTERNAL (wrapper)->show_about = TRUE;
      break;

    default:
      /* other signals are handled in panel-applications.c */
      xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (wrapper),
                                              provider_signal);
      break;
    }

  xfce_panel_plugin_wrapper_exported_complete_provider_signal (skeleton, invocation);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}



static gboolean
panel_plugin_external_wrapper_dbus_remote_event_result (XfcePanelPluginWrapperExported *skeleton,
                                                        GDBusMethodInvocation          *invocation,
                                                        guint                           handle,
                                                        gboolean                        result,
                                                        PanelPluginExternalWrapper     *wrapper)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (wrapper), FALSE);

  g_signal_emit (G_OBJECT (wrapper), external_signals[REMOTE_EVENT_RESULT], 0,
                 handle, result);

  xfce_panel_plugin_wrapper_exported_complete_remote_event_result (skeleton, invocation);

  return G_DBUS_METHOD_INVOCATION_HANDLED;
}



GtkWidget *
panel_plugin_external_wrapper_new (PanelModule  *module,
                                   gint          unique_id,
                                   gchar       **arguments)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL_WRAPPER_X11,
                         "module", module,
                         "unique-id", unique_id,
                         "arguments", arguments, NULL);
#endif

  g_critical ("Running plugins as external is not supported on this windowing environment");

  return NULL;
}
