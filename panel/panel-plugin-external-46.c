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

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-marshal.h>
#include <panel/panel-module.h>
#include <panel/panel-plugin-external-46.h>
#include <panel/panel-window.h>
#include <panel/panel-dialogs.h>



static void         panel_plugin_external_46_provider_init         (XfcePanelPluginProviderIface *iface);
static void         panel_plugin_external_46_finalize              (GObject                      *object);
static void         panel_plugin_external_46_get_property          (GObject                      *object,
                                                                    guint                         prop_id,
                                                                    GValue                       *value,
                                                                    GParamSpec                   *pspec);
static void         panel_plugin_external_46_set_property          (GObject                      *object,
                                                                    guint                         prop_id,
                                                                    const GValue                 *value,
                                                                    GParamSpec                   *pspec);
static void         panel_plugin_external_46_realize               (GtkWidget                    *widget);
static gboolean     panel_plugin_external_46_client_event          (GtkWidget                    *widget,
                                                                    GdkEventClient               *event);
static gboolean     panel_plugin_external_46_plug_removed          (GtkSocket                    *socket);
static void         panel_plugin_external_46_plug_added            (GtkSocket                    *socket);
static void         panel_plugin_external_46_send_client_event     (PanelPluginExternal46        *external,
                                                                    gint                          message,
                                                                    gint                          value);
static void         panel_plugin_external_46_queue_add             (PanelPluginExternal46        *external,
                                                                    gint                          message,
                                                                    gint                          value);
static const gchar *panel_plugin_external_46_get_name              (XfcePanelPluginProvider      *provider);
static gint         panel_plugin_external_46_get_unique_id         (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_46_set_size              (XfcePanelPluginProvider      *provider,
                                                                    gint                          size);
static void         panel_plugin_external_46_set_orientation       (XfcePanelPluginProvider      *provider,
                                                                    GtkOrientation                orientation);
static void         panel_plugin_external_46_set_screen_position   (XfcePanelPluginProvider      *provider,
                                                                    XfceScreenPosition            screen_position);
static void         panel_plugin_external_46_save                  (XfcePanelPluginProvider      *provider);
static gboolean     panel_plugin_external_46_get_show_configure    (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_46_show_configure        (XfcePanelPluginProvider      *provider);
static gboolean     panel_plugin_external_46_get_show_about        (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_46_show_about            (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_46_remove                (XfcePanelPluginProvider      *provider);
static gboolean     panel_plugin_external_46_remote_event          (XfcePanelPluginProvider      *provider,
                                                                    const gchar                  *name,
                                                                    const GValue                 *value);
static void         panel_plugin_external_46_set_sensitive         (PanelPluginExternal46        *external);
static void         panel_plugin_external_46_child_watch           (GPid                          pid,
                                                                    gint                          status,
                                                                    gpointer                      user_data);
static void         panel_plugin_external_46_child_watch_destroyed (gpointer                      user_data);



struct _PanelPluginExternal46Class
{
  GtkSocketClass __parent__;
};

struct _PanelPluginExternal46
{
  GtkSocket  __parent__;

  /* plugin information */
  gint              unique_id;

  /* the module */
  PanelModule      *module;

  /* whether the plug is embedded */
  guint             plug_embedded : 1;

  /* dbus message queue */
  GSList           *queue;

  /* counter to count the number of restarts */
  guint             n_restarts;

  /* some info we store here */
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
  PROP_UNIQUE_ID
};

typedef struct
{
  gint message;
  gint value;
}
QueueItem;



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
  gtkwidget_class->client_event = panel_plugin_external_46_client_event;

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_46_plug_removed;
  gtksocket_class->plug_added = panel_plugin_external_46_plug_added;

  g_object_class_install_property (gobject_class,
                                   PROP_UNIQUE_ID,
                                   g_param_spec_int ("unique-id", NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE
                                                     | G_PARAM_STATIC_STRINGS
                                                     | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_MODULE,
                                   g_param_spec_object ("module", NULL, NULL,
                                                        PANEL_TYPE_MODULE,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS
                                                        | G_PARAM_CONSTRUCT_ONLY));

  panel_atom = gdk_atom_intern_static_string (PANEL_CLIENT_EVENT_ATOM);
}



static void
panel_plugin_external_46_init (PanelPluginExternal46 *external)
{
  /* initialize */
  external->unique_id = -1;
  external->module = NULL;
  external->queue = NULL;
  external->plug_embedded = FALSE;
  external->n_restarts = 0;
  external->show_configure = FALSE;
  external->show_about = FALSE;

  /* signal to pass gtk_widget_set_sensitive() changes to the remote window */
  g_signal_connect (G_OBJECT (external), "notify::sensitive",
      G_CALLBACK (panel_plugin_external_46_set_sensitive), NULL);
}



static void
panel_plugin_external_46_provider_init (XfcePanelPluginProviderIface *iface)
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
  iface->remove = panel_plugin_external_46_remove;
  iface->remote_event = panel_plugin_external_46_remote_event;
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
        g_slice_free (QueueItem, li->data);
      g_slist_free (external->queue);
    }

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
  gchar               **argv;
  GError               *error = NULL;
  gboolean              succeed;
  gchar                *socket_id, *unique_id;
  GdkScreen            *screen;
  GPid                  pid;

  /* realize the socket first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_46_parent_class)->realize) (widget);

  /* get the socket id and unique id in a string */
  socket_id = g_strdup_printf ("%d", gtk_socket_get_id (GTK_SOCKET (widget)));
  unique_id = g_strdup_printf ("%d", external->unique_id);

  /* setup the basic argv */
  argv = g_new0 (gchar *, 12);
  argv[0]  = (gchar *) panel_module_get_filename (external->module);
  argv[1]  = (gchar *) "-n";
  argv[2]  = (gchar *) panel_module_get_name (external->module);
  argv[3]  = (gchar *) "-i";
  argv[4]  = (gchar *) unique_id;
  argv[5]  = (gchar *) "-d";
  argv[6]  = (gchar *) panel_module_get_display_name (external->module);
  argv[7]  = (gchar *) "-c";
  argv[8]  = (gchar *) panel_module_get_comment (external->module);
  argv[9] = (gchar *) "-s";
  argv[10] = (gchar *) socket_id;

  /* get the widget screen */
  screen = gtk_widget_get_screen (widget);

  /* spawn the proccess */
  succeed = gdk_spawn_on_screen (screen, NULL, argv, NULL,
                                 G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                                 NULL, &pid, &error);

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

  /* cleanup */
  g_free (socket_id);
  g_free (unique_id);
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
          case PROVIDER_SIGNAL_REMOVE_PLUGIN:
            /* we're forced removing the plugin, don't ask for a restart */
            external->plug_embedded = FALSE;

            /* destroy ourselfs, unrealize will close the plugin */
            gtk_widget_destroy (GTK_WIDGET (external));
            break;

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
  if (external->plug_embedded == FALSE)
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

  if (external->n_restarts++ <= PANEL_PLUGIN_AUTOMATIC_RESTARTS)
    {
      g_message ("Automatically restarting plugin %s-%d, try %d",
                 panel_module_get_name (external->module),
                 external->unique_id, external->n_restarts);
    }
  else if (panel_dialogs_restart_plugin (GTK_WINDOW (window),
               panel_module_get_display_name (external->module)))
    {
      /* reset the restart counter */
      external->n_restarts = 0;
    }
  else
    {
      /* cleanup the plugin configuration (in panel-application) */
      xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (external),
                                              PROVIDER_SIGNAL_REMOVE_PLUGIN);

      /* self destruction */
      gtk_widget_destroy (GTK_WIDGET (socket));

      return FALSE;
    }

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
  QueueItem             *item;

  /* plug has been added */
  external->plug_embedded = TRUE;

  if (external->queue != NULL)
    {
      external->queue = g_slist_reverse (external->queue);

      for (li = external->queue; li != NULL; li = li->next)
        {
          item = li->data;

          panel_plugin_external_46_send_client_event (external, item->message, item->value);

          g_slice_free (QueueItem, item);
        }

      g_slist_free (external->queue);
    }
}



static void
panel_plugin_external_46_send_client_event (PanelPluginExternal46 *external,
                                            gint                   message,
                                            gint                   value)
{
  GdkEventClient event;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));
  panel_return_if_fail (panel_atom != GDK_NONE);
  panel_return_if_fail (GDK_IS_WINDOW (GTK_WIDGET (external)->window));

  event.type = GDK_CLIENT_EVENT;
  event.window = GTK_WIDGET (external)->window;
  event.send_event = TRUE;
  event.message_type = panel_atom;
  event.data_format = 16;
  event.data.s[0] = message;
  event.data.s[1] = value;
  event.data.s[2] = 0;

  gdk_error_trap_push ();
  gdk_event_send_client_message ((GdkEvent *) &event,
      GDK_WINDOW_XID (gtk_socket_get_plug_window (GTK_SOCKET (external))));
  gdk_flush ();
  if (gdk_error_trap_pop () != 0)
    g_warning ("Failed to send client event %d", message);
}



static void
panel_plugin_external_46_queue_add (PanelPluginExternal46 *external,
                                    gint                   message,
                                    gint                   value)
{
  QueueItem *item;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));

  if (external->plug_embedded)
    {
      /* directly send the message */
      panel_plugin_external_46_send_client_event (external, message, value);
    }
  else
    {
      /* queue the message until the plug is embedded */
      item = g_slice_new0 (QueueItem);
      item->message = message;
      item->value = value;

      external->queue = g_slist_prepend (external->queue, item);
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
panel_plugin_external_46_remove (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_46_queue_add (PANEL_PLUGIN_EXTERNAL_46 (provider),
                                      PANEL_CLIENT_EVENT_REMOVE,
                                      FALSE);
}



static gboolean
panel_plugin_external_46_remote_event (XfcePanelPluginProvider *provider,
                                       const gchar             *name,
                                       const GValue            *value)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (provider), TRUE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), TRUE);

  g_warning ("Plugin %s is compiled as an Xfce 4.6 binary. It needs to be "
             "ported to the new library plugin framework to be able to use "
             "remote events.", panel_plugin_external_46_get_name (provider));

  return TRUE;
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

  switch (WEXITSTATUS (status))
    {
      case WRAPPER_EXIT_FAILURE:
      case WRAPPER_EXIT_NO_PROVIDER:
      case WRAPPER_EXIT_PREINIT:
        /* wait until everything is settled, then destroy the
         * external plugin so it is removed from the configuration */
        exo_gtk_object_destroy_later (GTK_OBJECT (external));
        break;
    }

  /* close the pid */
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
/* TODO arguments */
  return g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL_46,
                       "module", module,
                       "unique-id", unique_id, NULL);
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
