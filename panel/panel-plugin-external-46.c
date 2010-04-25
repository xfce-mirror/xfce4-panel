/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <exo/exo.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <common/panel-debug.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-module.h>
#include <panel/panel-plugin-external-46.h>
#include <panel/panel-window.h>
#include <panel/panel-dialogs.h>



static void         panel_plugin_external_46_provider_init            (XfcePanelPluginProviderInterface *iface);
static void         panel_plugin_external_46_finalize                 (GObject                          *object);
static void         panel_plugin_external_46_get_property             (GObject                          *object,
                                                                       guint                             prop_id,
                                                                       GValue                           *value,
                                                                       GParamSpec                       *pspec);
static void         panel_plugin_external_46_set_property             (GObject                          *object,
                                                                       guint                             prop_id,
                                                                       const GValue                     *value,
                                                                       GParamSpec                       *pspec);
static void         panel_plugin_external_46_realize                  (GtkWidget                        *widget);
static void         panel_plugin_external_46_unrealize                (GtkWidget                        *widget);
static gboolean     panel_plugin_external_46_client_event             (GtkWidget                        *widget,
                                                                       GdkEventClient                   *event);
static gboolean     panel_plugin_external_46_plug_removed             (GtkSocket                        *socket);
static void         panel_plugin_external_46_plug_added               (GtkSocket                        *socket);
static void         panel_plugin_external_46_send_client_event        (PanelPluginExternal46            *external,
                                                                       GdkEventClient                   *event);
static void         panel_plugin_external_46_send_client_event_simple (PanelPluginExternal46            *external,
                                                                       gint                              message,
                                                                       gint                              value);
static void         panel_plugin_external_46_queue_add                (PanelPluginExternal46            *external,
                                                                       gint                              message,
                                                                       gint                              value);
static const gchar *panel_plugin_external_46_get_name                 (XfcePanelPluginProvider          *provider);
static gint         panel_plugin_external_46_get_unique_id            (XfcePanelPluginProvider          *provider);
static void         panel_plugin_external_46_set_size                 (XfcePanelPluginProvider          *provider,
                                                                       gint                              size);
static void         panel_plugin_external_46_set_orientation          (XfcePanelPluginProvider          *provider,
                                                                       GtkOrientation                    orientation);
static void         panel_plugin_external_46_set_screen_position      (XfcePanelPluginProvider          *provider,
                                                                       XfceScreenPosition                screen_position);
static void         panel_plugin_external_46_save                     (XfcePanelPluginProvider          *provider);
static gboolean     panel_plugin_external_46_get_show_configure       (XfcePanelPluginProvider          *provider);
static void         panel_plugin_external_46_show_configure           (XfcePanelPluginProvider          *provider);
static gboolean     panel_plugin_external_46_get_show_about           (XfcePanelPluginProvider          *provider);
static void         panel_plugin_external_46_show_about               (XfcePanelPluginProvider          *provider);
static void         panel_plugin_external_46_removed                  (XfcePanelPluginProvider          *provider);
static gboolean     panel_plugin_external_46_remote_event             (XfcePanelPluginProvider          *provider,
                                                                       const gchar                      *name,
                                                                       const GValue                     *value,
                                                                       guint                            *handler_id);
static void         panel_plugin_external_46_set_locked               (XfcePanelPluginProvider          *provider,
                                                                       gboolean                          locked);
static void         panel_plugin_external_46_set_sensitive            (PanelPluginExternal46            *external);
static void         panel_plugin_external_46_child_watch              (GPid                              pid,
                                                                       gint                              status,
                                                                       gpointer                          user_data);
static void         panel_plugin_external_46_child_watch_destroyed    (gpointer                          user_data);



struct _PanelPluginExternal46Class
{
  GtkSocketClass __parent__;
};

struct _PanelPluginExternal46
{
  GtkSocket  __parent__;

  gint              unique_id;

  /* startup arguments */
  gchar           **arguments;

  PanelModule      *module;

  guint             plug_embedded : 1;

  /* client-event queue */
  GSList           *queue;

  /* auto restart timer */
  GTimer           *restart_timer;

  /* some info we receive on startup */
  guint             show_configure : 1;
  guint             show_about : 1;

  /* background image location */
  gchar            *background_image;

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



static GdkAtom panel_atom = GDK_NONE;



G_DEFINE_TYPE_WITH_CODE (PanelPluginExternal46, panel_plugin_external_46, GTK_TYPE_SOCKET,
  G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN_PROVIDER, panel_plugin_external_46_provider_init))



static void
panel_plugin_external_46_class_init (PanelPluginExternal46Class *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  GtkSocketClass *gtksocket_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_46_finalize;
  gobject_class->set_property = panel_plugin_external_46_set_property;
  gobject_class->get_property = panel_plugin_external_46_get_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_plugin_external_46_realize;
  gtkwidget_class->unrealize = panel_plugin_external_46_unrealize;
  gtkwidget_class->client_event = panel_plugin_external_46_client_event;

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_46_plug_removed;
  gtksocket_class->plug_added = panel_plugin_external_46_plug_added;

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

  panel_atom = gdk_atom_intern_static_string (_PANEL_CLIENT_EVENT_ATOM);
}



static void
panel_plugin_external_46_init (PanelPluginExternal46 *external)
{
  external->unique_id = -1;
  external->module = NULL;
  external->queue = NULL;
  external->arguments = NULL;
  external->plug_embedded = FALSE;
  external->restart_timer = NULL;
  external->show_configure = FALSE;
  external->show_about = FALSE;
  external->background_image = NULL;

  /* signal to pass gtk_widget_set_sensitive() changes to the remote window */
  g_signal_connect (G_OBJECT (external), "notify::sensitive",
      G_CALLBACK (panel_plugin_external_46_set_sensitive), NULL);
}



static void
panel_plugin_external_46_provider_init (XfcePanelPluginProviderInterface *iface)
{
  iface->get_name = panel_plugin_external_46_get_name;
  iface->get_unique_id = panel_plugin_external_46_get_unique_id;
  iface->set_size = panel_plugin_external_46_set_size;
  iface->set_orientation = panel_plugin_external_46_set_orientation;
  iface->set_screen_position = panel_plugin_external_46_set_screen_position;
  iface->save = panel_plugin_external_46_save;
  iface->get_show_configure = panel_plugin_external_46_get_show_configure;
  iface->show_configure = panel_plugin_external_46_show_configure;
  iface->get_show_about = panel_plugin_external_46_get_show_about;
  iface->show_about = panel_plugin_external_46_show_about;
  iface->removed = panel_plugin_external_46_removed;
  iface->remote_event = panel_plugin_external_46_remote_event;
  iface->set_locked = panel_plugin_external_46_set_locked;
}



static void
panel_plugin_external_46_finalize (GObject *object)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (object);
  GSList                *li;

  if (external->watch_id != 0)
    {
      /* remove the child watch and don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }

  if (external->queue != NULL)
    {
      for (li = external->queue; li != NULL; li = li->next)
        g_slice_free (GdkEventClient, li->data);
      g_slist_free (external->queue);
    }

  g_strfreev (external->arguments);
  g_free (external->background_image);

  if (external->restart_timer != NULL)
    g_timer_destroy (external->restart_timer);

  g_object_unref (G_OBJECT (external->module));

  (*G_OBJECT_CLASS (panel_plugin_external_46_parent_class)->finalize) (object);
}



static void
panel_plugin_external_46_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (object);

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
panel_plugin_external_46_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (object);

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
panel_plugin_external_46_realize (GtkWidget *widget)
{
  PanelPluginExternal46  *external = PANEL_PLUGIN_EXTERNAL_46 (widget);
  gchar                 **argv;
  GError                 *error = NULL;
  gboolean                succeed;
  gchar                  *socket_id, *unique_id;
  GPid                    pid;
  guint                   argc = PLUGIN_ARGV_ARGUMENTS;
  guint                   i;

  panel_return_if_fail (external->pid == 0);
  panel_return_if_fail (!external->plug_embedded);

  /* realize the socket first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_46_parent_class)->realize) (widget);

  /* get the socket id and unique id in a string */
  socket_id = g_strdup_printf ("%u", gtk_socket_get_id (GTK_SOCKET (widget)));
  unique_id = g_strdup_printf ("%d", external->unique_id);

  /* add the number of arguments to the argc count */
  if (G_UNLIKELY (external->arguments != NULL))
    argc += g_strv_length (external->arguments);

  /* setup the basic argv */
  argv = g_new0 (gchar *, argc + 1);
  argv[PLUGIN_ARGV_0] = (gchar *) panel_module_get_filename (external->module);
  argv[PLUGIN_ARGV_FILENAME] = (gchar *) ""; /* unused, for wrapper only */
  argv[PLUGIN_ARGV_UNIQUE_ID] = unique_id;
  argv[PLUGIN_ARGV_SOCKET_ID] = socket_id;
  argv[PLUGIN_ARGV_NAME] = (gchar *) panel_module_get_name (external->module);
  argv[PLUGIN_ARGV_DISPLAY_NAME] = (gchar *) panel_module_get_display_name (external->module);
  argv[PLUGIN_ARGV_COMMENT] = (gchar *) panel_module_get_comment (external->module);

  if (external->background_image != NULL)
    argv[PLUGIN_ARGV_BACKGROUND_IMAGE] = (gchar *) external->background_image;
  else
    argv[PLUGIN_ARGV_BACKGROUND_IMAGE] = (gchar *) "";

  /* append the arguments */
  if (G_UNLIKELY (external->arguments != NULL))
    for (i = 0; external->arguments[i] != NULL; i++)
      argv[i + PLUGIN_ARGV_ARGUMENTS] = external->arguments[i];

  /* spawn the proccess */
  succeed = gdk_spawn_on_screen (gtk_widget_get_screen (widget),
                                 NULL, argv, NULL,
                                 G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                                 NULL, &pid, &error);

  panel_debug (PANEL_DEBUG_DOMAIN_EXTERNAL46,
               "plugin=%s, unique-id=%s. socked-id=%s, argc=%d, pid=%d",
               argv[PLUGIN_ARGV_0], unique_id,
               socket_id, argc, pid);

  if (G_LIKELY (succeed))
    {
      /* watch the child */
      external->pid = pid;
      external->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                                   panel_plugin_external_46_child_watch, external,
                                                   panel_plugin_external_46_child_watch_destroyed);
    }
  else
    {
      g_critical ("Failed to spawn plugin: %s", error->message);
      g_error_free (error);
    }

  g_free (argv);
  g_free (socket_id);
  g_free (unique_id);
}



static void
panel_plugin_external_46_unrealize (GtkWidget *widget)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (widget);

  /* the plug is not embedded anymore */
  external->plug_embedded = FALSE;

  panel_debug (PANEL_DEBUG_DOMAIN_EXTERNAL46,
               "plugin %s-%d unrealized, quiting GtkPlug",
               panel_module_get_name (external->module),
               external->unique_id);

  if (external->watch_id != 0)
    {
      /* remove the child watch so we don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
      external->pid = 0;
    }

  /* quit the plugin */
  panel_plugin_external_46_send_client_event_simple (external, PANEL_CLIENT_EVENT_QUIT, FALSE);

  (*GTK_WIDGET_CLASS (panel_plugin_external_46_parent_class)->unrealize) (widget);
}



static gboolean
panel_plugin_external_46_client_event (GtkWidget      *widget,
                                       GdkEventClient *event)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (widget);
  gint                   provider_signal;

  if (event->message_type == panel_atom)
    {
      provider_signal = event->data.s[0];

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

      return FALSE;
    }

  return TRUE;
}



static gboolean
panel_plugin_external_46_plug_removed (GtkSocket *socket)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (socket);
  GtkWidget           *window;

  panel_return_val_if_fail (PANEL_IS_MODULE (external->module), FALSE);

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
panel_plugin_external_46_plug_added (GtkSocket *socket)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (socket);
  GSList                *li;
  GdkEventClient        *event;

  /* plug has been added */
  external->plug_embedded = TRUE;

  panel_debug (PANEL_DEBUG_DOMAIN_EXTERNAL46,
               "plugin %d has been embedded, %d events in queue",
               external->unique_id, g_slist_length (external->queue));

  /* send all the messages in the queue */
  if (external->queue != NULL)
    {
      external->queue = g_slist_reverse (external->queue);
      for (li = external->queue; li != NULL; li = li->next)
        {
          event = li->data;
          panel_plugin_external_46_send_client_event (external, event);
          g_slice_free (GdkEventClient, event);
        }

      g_slist_free (external->queue);
      external->queue = NULL;
    }
}



static void
panel_plugin_external_46_send_client_event (PanelPluginExternal46 *external,
                                            GdkEventClient        *event)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));
  panel_return_if_fail (panel_atom != GDK_NONE);
  panel_return_if_fail (GDK_IS_WINDOW (GTK_WIDGET (external)->window));

  /* complete event information */
  event->type = GDK_CLIENT_EVENT;
  event->window = GTK_WIDGET (external)->window;
  event->send_event = TRUE;
  event->message_type = panel_atom;

  gdk_error_trap_push ();
  gdk_event_send_client_message ((GdkEvent *) event,
      GDK_WINDOW_XID (gtk_socket_get_plug_window (GTK_SOCKET (external))));
  gdk_flush ();
  if (gdk_error_trap_pop () != 0)
    g_warning ("Failed to send client event %d", event->data.s[0]);
}



static void
panel_plugin_external_46_send_client_event_simple (PanelPluginExternal46 *external,
                                                   gint                   message,
                                                   gint                   value)
{
  GdkEventClient event;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  /* set event data */
  event.data_format = 16;
  event.data.s[0] = message;
  event.data.s[1] = value;
  event.data.s[2] = 0;

  panel_plugin_external_46_send_client_event (external, &event);
}



static void
panel_plugin_external_46_queue_add (PanelPluginExternal46 *external,
                                    gint                   message,
                                    gint                   value)
{
  GdkEventClient *event;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  if (external->plug_embedded)
    {
      /* directly send the message */
      panel_plugin_external_46_send_client_event_simple (external, message, value);
    }
  else
    {
      /* queue the message until the plug is embedded */
      event = g_slice_new0 (GdkEventClient);
      event->data_format = 16;
      event->data.s[0] = message;
      event->data.s[1] = value;

      external->queue = g_slist_prepend (external->queue, event);
    }
}



static const gchar *
panel_plugin_external_46_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), NULL);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return panel_module_get_name (PANEL_PLUGIN_EXTERNAL_46 (provider)->module);
}



static gint
panel_plugin_external_46_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), -1);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return PANEL_PLUGIN_EXTERNAL_46 (provider)->unique_id;
}



static void
panel_plugin_external_46_set_size (XfcePanelPluginProvider *provider,
                                   gint                     size)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SET_SIZE,
                                      size);
}



static void
panel_plugin_external_46_set_orientation (XfcePanelPluginProvider *provider,
                                          GtkOrientation           orientation)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SET_ORIENTATION,
                                      orientation);
}



static void
panel_plugin_external_46_set_screen_position (XfcePanelPluginProvider *provider,
                                              XfceScreenPosition       screen_position)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SET_SCREEN_POSITION,
                                      screen_position);
}



static void
panel_plugin_external_46_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SAVE,
                                      FALSE);
}



static gboolean
panel_plugin_external_46_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL_46 (provider)->show_configure;
}



static void
panel_plugin_external_46_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SHOW_CONFIGURE,
                                      FALSE);
}



static gboolean
panel_plugin_external_46_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL_46 (provider)->show_about;
}



static void
panel_plugin_external_46_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SHOW_ABOUT,
                                      FALSE);
}



static void
panel_plugin_external_46_removed (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_REMOVED,
                                      FALSE);
}



static gboolean
panel_plugin_external_46_remote_event (XfcePanelPluginProvider *provider,
                                       const gchar             *name,
                                       const GValue            *value,
                                       guint                   *handle)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), TRUE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), TRUE);

  g_warning ("Plugin %s is compiled as an Xfce 4.6 binary. It needs to be "
             "ported to the new library plugin framework to be able to use "
             "remote events.", panel_plugin_external_46_get_name (provider));

  return TRUE;
}



static void
panel_plugin_external_46_set_locked (XfcePanelPluginProvider *provider,
                                     gboolean                 locked)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_SET_LOCKED,
                                      locked);
}



static void
panel_plugin_external_46_set_sensitive (PanelPluginExternal46 *external)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  panel_plugin_external_46_queue_add (external,
                                      PANEL_CLIENT_EVENT_SET_SENSITIVE,
                                      GTK_WIDGET_IS_SENSITIVE (external));
}



static void
panel_plugin_external_46_child_watch (GPid     pid,
                                      gint     status,
                                      gpointer user_data)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (user_data);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));
  panel_return_if_fail (external->pid == pid);

  /* only handle our exit status if the plugin exited normally */
  if (WIFEXITED (status))
    {
      /* extract our return value from the status */
      switch (WEXITSTATUS (status))
        {
        case PLUGIN_EXIT_SUCCESS:
          break;

        case PLUGIN_EXIT_FAILURE:
        case PLUGIN_EXIT_PREINIT_FAILED:
        case PLUGIN_EXIT_CHECK_FAILED:
        case PLUGIN_EXIT_NO_PROVIDER:
          panel_debug (PANEL_DEBUG_DOMAIN_EXTERNAL46,
                       "plugin exited with status %d. Removing from "
                       "configuration.", WEXITSTATUS (status));

          /* cleanup the plugin configuration (in panel-application) */
          xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (external),
                                                  PROVIDER_SIGNAL_REMOVE_PLUGIN);

          /* wait until everything is settled */
          exo_gtk_object_destroy_later (GTK_OBJECT (external));
          break;

        }
    }

  g_spawn_close_pid (pid);
}



static void
panel_plugin_external_46_child_watch_destroyed (gpointer user_data)
{
  PANEL_PLUGIN_EXTERNAL_46 (user_data)->watch_id = 0;
}



GtkWidget *
panel_plugin_external_46_new (PanelModule  *module,
                              gint          unique_id,
                              gchar       **arguments)
{
  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);

  return g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL_46,
                       "module", module,
                       "unique-id", unique_id,
                       "arguments", arguments, NULL);
}



void
panel_plugin_external_46_set_background_alpha (PanelPluginExternal46 *external,
                                               gdouble              alpha)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  panel_plugin_external_46_queue_add (external,
                                      PANEL_CLIENT_EVENT_SET_BACKGROUND_ALPHA,
                                      rint (alpha * 100.0));
}



void
panel_plugin_external_46_set_background_color (PanelPluginExternal46 *external,
                                               const GdkColor        *color)
{
  GdkEventClient *event;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  event = g_slice_new0 (GdkEventClient);

  if (color != NULL)
    {
      event->data_format = 16;
      event->data.s[0] = PANEL_CLIENT_EVENT_SET_BG_COLOR;
      event->data.s[1] = color->red;
      event->data.s[2] = color->green;
      event->data.s[3] = color->blue;
      event->data.s[4] = 0;
    }
  else
    {
      event->data_format = 16;
      event->data.s[0] = PANEL_CLIENT_EVENT_UNSET_BG;
    }

  if (external->plug_embedded)
    {
      /* directly send the event */
      panel_plugin_external_46_send_client_event (external, event);
      g_slice_free (GdkEventClient, event);
    }
  else
    {
      /* queue the event until the plug is embedded */
      external->queue = g_slist_prepend (external->queue, event);
    }
}


void
panel_plugin_external_46_set_background_image (PanelPluginExternal46 *external,
                                               const gchar           *image)
{
  GtkWidget *window;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  /* store new file location */
  g_free (external->background_image);
  external->background_image = g_strdup (image);

  /* restart the plugin if a process is already running */
  if (external->plug_embedded)
    {
      panel_debug (PANEL_DEBUG_DOMAIN_EXTERNAL46,
                   "going to restart plugin %d for background image change",
                   external->unique_id);

      gtk_widget_unrealize (GTK_WIDGET (external));
      gtk_widget_hide (GTK_WIDGET (external));

      window = gtk_widget_get_toplevel (GTK_WIDGET (external));
      panel_return_if_fail (PANEL_IS_WINDOW (window));
      panel_window_set_povider_info (PANEL_WINDOW (window), GTK_WIDGET (external));

      gtk_widget_show (GTK_WIDGET (external));
    }
}
