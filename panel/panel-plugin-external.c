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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <exo/exo.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <common/panel-dbus.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-module.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-window.h>
#include <panel/panel-dialogs.h>



#define WRAPPER_BIN LIBEXECDIR G_DIR_SEPARATOR_S "panel" G_DIR_SEPARATOR_S "wrapper"



static void         panel_plugin_external_provider_init         (XfcePanelPluginProviderInterface  *iface);
static GObject     *panel_plugin_external_constructor           (GType                              type,
                                                                 guint                              n_construct_params,
                                                                 GObjectConstructParam             *construct_params);
static void         panel_plugin_external_finalize              (GObject                           *object);
static void         panel_plugin_external_get_property          (GObject                           *object,
                                                                 guint                              prop_id,
                                                                 GValue                            *value,
                                                                 GParamSpec                        *pspec);
static void         panel_plugin_external_set_property          (GObject                           *object,
                                                                 guint                              prop_id,
                                                                 const GValue                      *value,
                                                                 GParamSpec                        *pspec);
static void         panel_plugin_external_destroy               (GtkObject                         *object);
static void         panel_plugin_external_realize               (GtkWidget                         *widget);
static gboolean     panel_plugin_external_plug_removed          (GtkSocket                         *socket);
static void         panel_plugin_external_plug_added            (GtkSocket                         *socket);
static gboolean     panel_plugin_external_dbus_reply            (PanelPluginExternal               *external,
                                                                 guint                              reply_id,
                                                                 const GValue                      *value,
                                                                 GError                           **error);
static gboolean     panel_plugin_external_dbus_provider_signal  (PanelPluginExternal               *external,
                                                                 XfcePanelPluginProviderSignal      provider_signal,
                                                                 GError                           **error);
static void         panel_plugin_external_dbus_set              (PanelPluginExternal               *external,
                                                                 gboolean                           force);
static void         panel_plugin_external_queue_add             (PanelPluginExternal               *external,
                                                                 gboolean                           force,
                                                                 const gchar                       *property,
                                                                 const GValue                      *value);
static void         panel_plugin_external_queue_add_noop        (PanelPluginExternal               *external,
                                                                 gboolean                           force,
                                                                 const gchar                       *property);
static const gchar *panel_plugin_external_get_name              (XfcePanelPluginProvider           *provider);
static gint         panel_plugin_external_get_unique_id         (XfcePanelPluginProvider           *provider);
static void         panel_plugin_external_set_size              (XfcePanelPluginProvider           *provider,
                                                                 gint                               size);
static void         panel_plugin_external_set_orientation       (XfcePanelPluginProvider           *provider,
                                                                 GtkOrientation                     orientation);
static void         panel_plugin_external_set_screen_position   (XfcePanelPluginProvider           *provider,
                                                                 XfceScreenPosition                 screen_position);
static void         panel_plugin_external_save                  (XfcePanelPluginProvider           *provider);
static gboolean     panel_plugin_external_get_show_configure    (XfcePanelPluginProvider           *provider);
static void         panel_plugin_external_show_configure        (XfcePanelPluginProvider           *provider);
static gboolean     panel_plugin_external_get_show_about        (XfcePanelPluginProvider           *provider);
static void         panel_plugin_external_show_about            (XfcePanelPluginProvider           *provider);
static void         panel_plugin_external_removed               (XfcePanelPluginProvider           *provider);
static gboolean     panel_plugin_external_remote_event          (XfcePanelPluginProvider           *provider,
                                                                 const gchar                       *name,
                                                                 const GValue                      *value);
static void         panel_plugin_external_set_sensitive         (PanelPluginExternal               *external);
static void         panel_plugin_external_child_watch           (GPid                               pid,
                                                                 gint                               status,
                                                                 gpointer                           user_data);
static void         panel_plugin_external_child_watch_destroyed (gpointer                           user_data);



/* include the dbus glue generated by dbus-binding-tool */
#include <panel/panel-plugin-external-infos.h>



struct _PanelPluginExternalClass
{
  GtkSocketClass __parent__;
};

struct _PanelPluginExternal
{
  GtkSocket  __parent__;

  gint              unique_id;

  /* startup arguments */
  gchar           **arguments;

  PanelModule      *module;

  guint             plug_embedded : 1;

  /* dbus message queue */
  GPtrArray        *queue;

  /* auto restart timer */
  GTimer           *restart_timer;

  /* some info received over dbus on startup */
  guint             show_configure : 1;
  guint             show_about : 1;

  /* child watch data */
  GPid              pid;
  guint             watch_id;
};

enum
{
  PROP_0,
  PROP_MODULE,
  PROP_UNIQUE_ID,
  PROP_ARGUMENTS
};

enum
{
  SET,
  LAST_SIGNAL
};



static guint external_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (PanelPluginExternal, panel_plugin_external, GTK_TYPE_SOCKET,
  G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN_PROVIDER, panel_plugin_external_provider_init))



static void
panel_plugin_external_class_init (PanelPluginExternalClass *klass)
{
  GObjectClass   *gobject_class;
  GtkObjectClass *gtkobject_class;
  GtkWidgetClass *gtkwidget_class;
  GtkSocketClass *gtksocket_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructor = panel_plugin_external_constructor;
  gobject_class->finalize = panel_plugin_external_finalize;
  gobject_class->set_property = panel_plugin_external_set_property;
  gobject_class->get_property = panel_plugin_external_get_property;

  gtkobject_class = GTK_OBJECT_CLASS (klass);
  gtkobject_class->destroy = panel_plugin_external_destroy;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_plugin_external_realize;

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_plug_removed;
  gtksocket_class->plug_added = panel_plugin_external_plug_added;

  external_signals[SET] =
    g_signal_new (g_intern_static_string ("set"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  PANEL_TYPE_DBUS_SET_SIGNAL);

  g_object_class_install_property (gobject_class,
                                   PROP_UNIQUE_ID,
                                   g_param_spec_int ("unique-id",
                                                     NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     EXO_PARAM_READWRITE
                                                     | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_MODULE,
                                   g_param_spec_object ("module",
                                                        NULL, NULL,
                                                        PANEL_TYPE_MODULE,
                                                        EXO_PARAM_READWRITE
                                                        | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_ARGUMENTS,
                                   g_param_spec_boxed ("arguments",
                                                       NULL, NULL,
                                                       G_TYPE_STRV,
                                                       EXO_PARAM_READWRITE
                                                       | G_PARAM_CONSTRUCT_ONLY));

  /* add dbus type info for plugins */
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
      &dbus_glib_panel_plugin_external_object_info);
}



static void
panel_plugin_external_init (PanelPluginExternal *external)
{
  external->unique_id = -1;
  external->module = NULL;
  external->arguments = NULL;
  external->queue = NULL;
  external->restart_timer = NULL;
  external->plug_embedded = FALSE;
  external->show_configure = FALSE;
  external->show_about = FALSE;

  /* signal to pass gtk_widget_set_sensitive() changes to the remote window */
  g_signal_connect (G_OBJECT (external), "notify::sensitive",
      G_CALLBACK (panel_plugin_external_set_sensitive), NULL);
}



static void
panel_plugin_external_provider_init (XfcePanelPluginProviderInterface *iface)
{
  iface->get_name = panel_plugin_external_get_name;
  iface->get_unique_id = panel_plugin_external_get_unique_id;
  iface->set_size = panel_plugin_external_set_size;
  iface->set_orientation = panel_plugin_external_set_orientation;
  iface->set_screen_position = panel_plugin_external_set_screen_position;
  iface->save = panel_plugin_external_save;
  iface->get_show_configure = panel_plugin_external_get_show_configure;
  iface->show_configure = panel_plugin_external_show_configure;
  iface->get_show_about = panel_plugin_external_get_show_about;
  iface->show_about = panel_plugin_external_show_about;
  iface->removed = panel_plugin_external_removed;
  iface->remote_event = panel_plugin_external_remote_event;
}



static GObject *
panel_plugin_external_constructor (GType                  type,
                                   guint                  n_construct_params,
                                   GObjectConstructParam *construct_params)
{
  GObject         *object;
  gchar           *path;
  DBusGConnection *connection;
  GError          *error = NULL;

  object = G_OBJECT_CLASS (panel_plugin_external_parent_class)->constructor (type,
                                                                             n_construct_params,
                                                                             construct_params);

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (G_LIKELY (connection != NULL))
    {
      /* register the object in dbus, the wrapper will monitor this object */
      panel_return_val_if_fail (PANEL_PLUGIN_EXTERNAL (object)->unique_id != -1, NULL);
      path = g_strdup_printf (PANEL_DBUS_WRAPPER_PATH, PANEL_PLUGIN_EXTERNAL (object)->unique_id);
      dbus_g_connection_register_g_object (connection, path, object);
      g_free (path);

      dbus_g_connection_unref (connection);
    }
  else
    {
      g_critical ("Failed to get D-Bus session bus: %s", error->message);
      g_error_free (error);
    }

  return object;
}



static void
panel_plugin_external_finalize (GObject *object)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);

  if (external->watch_id != 0)
    {
      /* remove the child watch and don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }

  if (external->queue != NULL)
    {
      g_ptr_array_foreach (external->queue, (GFunc) g_value_array_free, NULL);
      g_ptr_array_free (external->queue, TRUE);
    }

  g_strfreev (external->arguments);

  if (external->restart_timer != NULL)
    g_timer_destroy (external->restart_timer);

  g_object_unref (G_OBJECT (external->module));

  (*G_OBJECT_CLASS (panel_plugin_external_parent_class)->finalize) (object);
}



static void
panel_plugin_external_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);

  switch (prop_id)
    {
    case PROP_UNIQUE_ID:
      g_value_set_int (value, external->unique_id);
      break;

    case PROP_ARGUMENTS:
      g_value_set_boxed (value, external->arguments);
      break;

    case PROP_MODULE:
      g_value_set_object (value, external->module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_plugin_external_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);

  switch (prop_id)
    {
    case PROP_UNIQUE_ID:
      external->unique_id = g_value_get_int (value);
      break;

    case PROP_ARGUMENTS:
      external->arguments = g_value_dup_boxed (value);
      break;

    case PROP_MODULE:
      external->module = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_plugin_external_destroy (GtkObject *object)
{
  panel_plugin_external_queue_add_noop (PANEL_PLUGIN_EXTERNAL (object),
                                        TRUE, SIGNAL_WRAPPER_QUIT);

  (*GTK_OBJECT_CLASS (panel_plugin_external_parent_class)->destroy) (object);
}



static void
panel_plugin_external_realize (GtkWidget *widget)
{
  PanelPluginExternal  *external = PANEL_PLUGIN_EXTERNAL (widget);
  gchar               **argv;
  GError               *error = NULL;
  gboolean              succeed;
  gchar                *socket_id, *unique_id;
  guint                 i;
  guint                 argc = PLUGIN_ARGV_ARGUMENTS;
  GPid                  pid;

  /* realize the socket first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->realize) (widget);

  /* get the socket id and unique id in a string */
  socket_id = g_strdup_printf ("%d", gtk_socket_get_id (GTK_SOCKET (widget)));
  unique_id = g_strdup_printf ("%d", external->unique_id);

  /* add the number of arguments to the argc count */
  if (G_UNLIKELY (external->arguments != NULL))
    argc += g_strv_length (external->arguments);

  /* setup the basic argv */
  argv = g_new0 (gchar *, argc + 1);
  argv[PLUGIN_ARGV_0] = (gchar *) WRAPPER_BIN;
  argv[PLUGIN_ARGV_FILENAME] = (gchar *) panel_module_get_filename (external->module);
  argv[PLUGIN_ARGV_UNIQUE_ID] = (gchar *) unique_id;
  argv[PLUGIN_ARGV_SOCKET_ID] = (gchar *) socket_id;
  argv[PLUGIN_ARGV_NAME] = (gchar *) panel_module_get_name (external->module);
  argv[PLUGIN_ARGV_DISPLAY_NAME] = (gchar *) panel_module_get_display_name (external->module);
  argv[PLUGIN_ARGV_COMMENT] = (gchar *) panel_module_get_comment (external->module);

  /* append the arguments */
  if (G_UNLIKELY (external->arguments != NULL))
    for (i = 0; external->arguments[i] != NULL; i++)
      argv[i + PLUGIN_ARGV_ARGUMENTS] = external->arguments[i];

  /* spawn the proccess */
  succeed = gdk_spawn_on_screen (gtk_widget_get_screen (widget),
                                 NULL, argv, NULL,
                                 G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                                 NULL, &pid, &error);

  if (G_LIKELY (succeed))
    {
      /* watch the child */
      external->pid = pid;
      external->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                                   panel_plugin_external_child_watch, external,
                                                   panel_plugin_external_child_watch_destroyed);
    }
  else
    {
      g_critical ("Failed to spawn the xfce4-panel-wrapper: %s", error->message);
      g_error_free (error);
    }

  g_free (argv);
  g_free (socket_id);
  g_free (unique_id);
}



static gboolean
panel_plugin_external_plug_removed (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);
  GtkWidget           *window;

  /* leave when the plugin was already removed */
  if (!external->plug_embedded)
    return FALSE;

  /* plug has been removed */
  external->plug_embedded = FALSE;

  /* unrealize and hide the socket */
  gtk_widget_unrealize (GTK_WIDGET (socket));
  gtk_widget_hide (GTK_WIDGET (socket));

  if (external->watch_id != 0)
    {
      /* remove the child watch so we don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }

  window = gtk_widget_get_toplevel (GTK_WIDGET (socket));
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  if (external->restart_timer == NULL
      || g_timer_elapsed (external->restart_timer, NULL) > PANEL_PLUGIN_AUTO_RESTART)
    {
      g_message ("Plugin %s-%d has been automatically restarted after crash.",
                 panel_module_get_name (external->module),
                 external->unique_id);
    }
  else if (!panel_dialogs_restart_plugin (GTK_WINDOW (window),
               panel_module_get_display_name (external->module)))
    {
      /* cleanup the plugin configuration (in panel-application) */
      xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (external),
                                              PROVIDER_SIGNAL_REMOVE_PLUGIN);

      /* self destruction */
      gtk_widget_destroy (GTK_WIDGET (socket));

      return FALSE;
    }

  /* create or reset the retart timer */
  if (external->restart_timer == NULL)
    external->restart_timer = g_timer_new ();
  else
    g_timer_reset (external->restart_timer);

  /* send panel information to the plugin (queued until realize) */
  panel_window_set_povider_info (PANEL_WINDOW (window), GTK_WIDGET (external));

  /* show the socket again (realize will spawn the plugin) */
  gtk_widget_show (GTK_WIDGET (socket));

  return TRUE;
}



static void
panel_plugin_external_plug_added (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);

  /* plug has been added */
  external->plug_embedded = TRUE;

  if (external->queue != NULL)
    panel_plugin_external_dbus_set (external, FALSE);
}



static gboolean
panel_plugin_external_dbus_reply (PanelPluginExternal  *external,
                                  guint                 reply_id,
                                  const GValue         *value,
                                  GError              **error)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external), FALSE);

  return TRUE;
}



static gboolean
panel_plugin_external_dbus_provider_signal (PanelPluginExternal            *external,
                                            XfcePanelPluginProviderSignal   provider_signal,
                                            GError                        **error)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external), FALSE);

  switch (provider_signal)
    {
    case PROVIDER_SIGNAL_SHOW_CONFIGURE:
      external->show_configure = TRUE;
      break;

    case PROVIDER_SIGNAL_SHOW_ABOUT:
      external->show_about = TRUE;
      break;

    default:
      /* other signals are handled in panel-applications.c */
      xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (external),
                                              provider_signal);
      break;
    }

  return TRUE;
}



static void
panel_plugin_external_dbus_set (PanelPluginExternal *external,
                                gboolean             force)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (external->queue != NULL);

  if (force || external->plug_embedded)
    {
      g_signal_emit (G_OBJECT (external), external_signals[SET], 0,
                     external->queue);

      g_ptr_array_foreach (external->queue, (GFunc) g_value_array_free, NULL);
      g_ptr_array_free (external->queue, TRUE);
      external->queue = NULL;
    }
}



static void
panel_plugin_external_queue_add (PanelPluginExternal *external,
                                 gboolean             force,
                                 const gchar         *property,
                                 const GValue        *value)
{
  GValue message = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (G_TYPE_CHECK_VALUE (value));
  panel_return_if_fail (!force || external->queue == NULL);

  if (external->queue == NULL)
    external->queue = g_ptr_array_sized_new (1);

  g_value_init (&message, PANEL_TYPE_DBUS_SET_MESSAGE);
  g_value_take_boxed (&message, dbus_g_type_specialized_construct (G_VALUE_TYPE (&message)));

  dbus_g_type_struct_set (&message,
                          DBUS_SET_PROPERTY, property,
                          DBUS_SET_VALUE, value,
                          DBUS_SET_REPLY_ID, 0,
                          G_MAXUINT);

  g_ptr_array_add (external->queue, g_value_get_boxed (&message));

  panel_plugin_external_dbus_set (external, force);
}



static void
panel_plugin_external_queue_add_noop (PanelPluginExternal *external,
                                      gboolean             force,
                                      const gchar         *property)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* add to queue with noop boolean */
  g_value_init (&value, G_TYPE_BOOLEAN);
  panel_plugin_external_queue_add (external, force, property, &value);
  g_value_unset (&value);
}



static const gchar *
panel_plugin_external_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), NULL);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return panel_module_get_name (PANEL_PLUGIN_EXTERNAL (provider)->module);
}



static gint
panel_plugin_external_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), -1);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return PANEL_PLUGIN_EXTERNAL (provider)->unique_id;
}



static void
panel_plugin_external_set_size (XfcePanelPluginProvider *provider,
                                gint                     size)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, size);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   FALSE, SIGNAL_SET_SIZE,
                                   &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_orientation (XfcePanelPluginProvider *provider,
                                       GtkOrientation           orientation)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, orientation);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   FALSE, SIGNAL_SET_ORIENTATION,
                                   &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_screen_position (XfcePanelPluginProvider *provider,
                                           XfceScreenPosition       screen_position)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, screen_position);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   FALSE, SIGNAL_SET_SCREEN_POSITION,
                                   &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_queue_add_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                        FALSE, SIGNAL_SAVE);
}



static gboolean
panel_plugin_external_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL (provider)->show_configure;
}



static void
panel_plugin_external_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_queue_add_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                        FALSE, SIGNAL_SHOW_CONFIGURE);
}



static gboolean
panel_plugin_external_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL (provider)->show_about;
}



static void
panel_plugin_external_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_queue_add_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                        FALSE, SIGNAL_SHOW_ABOUT);
}



static void
panel_plugin_external_removed (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_queue_add_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                        FALSE, SIGNAL_REMOVED);
}



static gboolean
panel_plugin_external_remote_event (XfcePanelPluginProvider *provider,
                                    const gchar             *name,
                                    const GValue            *value)
{
  GValue        noop_value = { 0, };
  const GValue *real_value = value;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), TRUE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), TRUE);
  panel_return_val_if_fail (value == NULL || G_IS_VALUE (value), FALSE);

  /* TODO handle the return value */

  if (value == NULL)
    {
      g_value_init (&noop_value, G_TYPE_UCHAR);
      g_value_set_uchar (&noop_value, '\0');
      real_value = &noop_value;
    }

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   FALSE, name, real_value);

  return TRUE;
}



static void
panel_plugin_external_set_sensitive (PanelPluginExternal *external)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, GTK_WIDGET_IS_SENSITIVE (external));

  panel_plugin_external_queue_add (external, FALSE,
                                   SIGNAL_WRAPPER_SET_SENSITIVE,
                                   &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_child_watch (GPid     pid,
                                   gint     status,
                                   gpointer user_data)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (user_data);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (external->pid == pid);

  /* only handle our exit status if the plugin exited normally */
  if (WIFEXITED (status))
    {
      /* extract our return value from the status */
      switch (WEXITSTATUS (status))
        {
        case PLUGIN_EXIT_SUCCESS:
        case PLUGIN_EXIT_FAILURE:
        case PLUGIN_EXIT_PREINIT_FAILED:
        case PLUGIN_EXIT_CHECK_FAILED:
        case PLUGIN_EXIT_NO_PROVIDER:
          /* wait until everything is settled, then destroy the
           * external plugin so it is removed from the configuration */
          exo_gtk_object_destroy_later (GTK_OBJECT (external));
          break;
        }
    }

  g_spawn_close_pid (pid);
}



static void
panel_plugin_external_child_watch_destroyed (gpointer user_data)
{
  PANEL_PLUGIN_EXTERNAL (user_data)->watch_id = 0;
}



GtkWidget *
panel_plugin_external_new (PanelModule  *module,
                           gint          unique_id,
                           gchar       **arguments)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);

  return g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL,
                       "module", module,
                       "unique-id", unique_id,
                       "arguments", arguments, NULL);
}



void
panel_plugin_external_set_background_alpha (PanelPluginExternal *external,
                                            gdouble              alpha)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  g_value_init (&value, G_TYPE_DOUBLE);
  g_value_set_double (&value, alpha);

  panel_plugin_external_queue_add (external, FALSE,
                                   SIGNAL_WRAPPER_BACKGROUND_ALPHA,
                                   &value);

  g_value_unset (&value);
}
