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
#include <panel/panel-dbus-service.h>

/* Number of automatic plugin restarts before the
 * panel asks the users what to do. This code is
 * disabled when debugging is enabled. */
#ifndef NDEBUG
#define N_RESTART_TRIES (0)
#else
#define N_RESTART_TRIES (2)
#endif



static void         panel_plugin_external_provider_init         (XfcePanelPluginProviderIface    *iface);
static void         panel_plugin_external_finalize              (GObject                         *object);
static void         panel_plugin_external_realize               (GtkWidget                       *widget);
static void         panel_plugin_external_unrealize             (GtkWidget                       *widget);
static gboolean     panel_plugin_external_plug_removed          (GtkSocket                       *socket);
static void         panel_plugin_external_plug_added            (GtkSocket                       *socket);
static void         panel_plugin_external_provider_signal       (XfcePanelPluginProvider         *provider,
                                                                 XfcePanelPluginProviderSignal    signal);
static const gchar *panel_plugin_external_get_name              (XfcePanelPluginProvider         *provider);
static gint         panel_plugin_external_get_unique_id         (XfcePanelPluginProvider         *provider);
static void         panel_plugin_external_set_size              (XfcePanelPluginProvider         *provider,
                                                                 gint                             size);
static void         panel_plugin_external_set_orientation       (XfcePanelPluginProvider         *provider,
                                                                 GtkOrientation                   orientation);
static void         panel_plugin_external_set_screen_position   (XfcePanelPluginProvider         *provider,
                                                                 XfceScreenPosition               screen_position);
static void         panel_plugin_external_save                  (XfcePanelPluginProvider         *provider);
static gboolean     panel_plugin_external_get_show_configure    (XfcePanelPluginProvider         *provider);
static void         panel_plugin_external_show_configure        (XfcePanelPluginProvider         *provider);
static gboolean     panel_plugin_external_get_show_about        (XfcePanelPluginProvider         *provider);
static void         panel_plugin_external_show_about            (XfcePanelPluginProvider         *provider);
static void         panel_plugin_external_remove                (XfcePanelPluginProvider         *provider);
static void         panel_plugin_external_set_sensitive         (PanelPluginExternal             *external);
static void         panel_plugin_external_set_property          (PanelPluginExternal             *external,
                                                                 DBusPropertyChanged              property,
                                                                 const GValue                    *value);
static void         panel_plugin_external_set_property_noop     (PanelPluginExternal             *external,
                                                                 DBusPropertyChanged              property);
static void         panel_plugin_external_child_watch           (GPid                             pid,
                                                                 gint                             status,
                                                                 gpointer                         user_data);
static void         panel_plugin_external_child_watch_destroyed (gpointer                         user_data);


struct _PanelPluginExternalClass
{
  GtkSocketClass __parent__;
};

struct _PanelPluginExternal
{
  GtkSocket  __parent__;

  /* plugin information */
  gint              unique_id;

  /* startup arguments */
  gchar           **arguments;

  /* the module */
  PanelModule      *module;

  /* whether the plug is embedded */
  guint             plug_embedded : 1;

  /* dbus message queue */
  GSList           *dbus_queue;

  /* counter to count the number of restarts */
  guint             n_restarts;

  /* some info we store here */
  guint             show_configure : 1;
  guint             show_about : 1;

  /* child watch data */
  GPid              pid;
  guint             watch_id;
};

typedef struct
{
  DBusPropertyChanged property;
  GValue              value;
}
QueuedData;



G_DEFINE_TYPE_WITH_CODE (PanelPluginExternal, panel_plugin_external, GTK_TYPE_SOCKET,
  G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN_PROVIDER, panel_plugin_external_provider_init))



static void
panel_plugin_external_class_init (PanelPluginExternalClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;
  GtkSocketClass *gtksocket_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_plugin_external_realize;
  gtkwidget_class->unrealize = panel_plugin_external_unrealize;

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_plug_removed;
  gtksocket_class->plug_added = panel_plugin_external_plug_added;
}



static void
panel_plugin_external_init (PanelPluginExternal *external)
{
  /* initialize */
  external->unique_id = -1;
  external->module = NULL;
  external->arguments = NULL;
  external->dbus_queue = NULL;
  external->plug_embedded = FALSE;
  external->n_restarts = 0;
  external->show_configure = FALSE;
  external->show_about = FALSE;

  /* signal to pass gtk_widget_set_sensitive() changes to the remote window */
  g_signal_connect (G_OBJECT (external), "notify::sensitive", G_CALLBACK (panel_plugin_external_set_sensitive), NULL);
}



static void
panel_plugin_external_provider_init (XfcePanelPluginProviderIface *iface)
{
  iface->provider_signal = panel_plugin_external_provider_signal;
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
  iface->remove = panel_plugin_external_remove;
}



static void
panel_plugin_external_finalize (GObject *object)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);
  GSList              *li;
  QueuedData          *data;

  if (external->watch_id != 0)
    {
      /* remove the child watch and don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }

  /* cleanup */
  g_strfreev (external->arguments);

  /* free the queue */
  for (li = external->dbus_queue; li != NULL; li = li->next)
    {
      data = li->data;
      g_value_unset (&data->value);
      g_slice_free (QueuedData, data);
    }
  g_slist_free (external->dbus_queue);

  /* release the module */
  g_object_unref (G_OBJECT (external->module));

  (*G_OBJECT_CLASS (panel_plugin_external_parent_class)->finalize) (object);
}



static void
panel_plugin_external_realize (GtkWidget *widget)
{
  PanelPluginExternal  *external = PANEL_PLUGIN_EXTERNAL (widget);
  gchar               **argv;
  GError               *error = NULL;
  gboolean              succeed;
  gchar                *socket_id, *unique_id;
  gint                  i, argc = 14;
  GdkScreen            *screen;
  GPid                  pid;

  /* realize the socket first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->realize) (widget);

  /* get the socket id and unique id in a string */
  socket_id = g_strdup_printf ("%d", gtk_socket_get_id (GTK_SOCKET (widget)));
  unique_id = g_strdup_printf ("%d", external->unique_id);

  /* add the number of arguments to the argv count */
  if (G_UNLIKELY (external->arguments != NULL))
    argc += g_strv_length (external->arguments);

  /* allocate argv */
  argv = g_new0 (gchar *, argc);

  /* setup the basic argv */
  argv[0]  = (gchar *) LIBEXECDIR G_DIR_SEPARATOR_S "wrapper";
  argv[1]  = (gchar *) "-n";
  argv[2]  = (gchar *) panel_module_get_name (external->module);
  argv[3]  = (gchar *) "-i";
  argv[4]  = (gchar *) unique_id;
  argv[5]  = (gchar *) "-d";
  argv[6]  = (gchar *) panel_module_get_display_name (external->module);
  argv[7]  = (gchar *) "-c";
  argv[8]  = (gchar *) panel_module_get_comment (external->module);
  argv[9]  = (gchar *) "-f";
  argv[10] = (gchar *) panel_module_get_filename (external->module);
  argv[11] = (gchar *) "-s";
  argv[12] = (gchar *) socket_id;

  /* append the arguments */
  if (G_UNLIKELY (external->arguments != NULL))
    for (i = 0; external->arguments[i] != NULL; i++)
      argv[i + 13] = external->arguments[i];

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
                                                   panel_plugin_external_child_watch, external,
                                                   panel_plugin_external_child_watch_destroyed);
    }
  else
    {
      g_critical ("Failed to spawn the xfce4-panel-wrapper: %s", error->message);
      g_error_free (error);
    }

  /* cleanup */
  g_free (socket_id);
  g_free (unique_id);
}



static void
panel_plugin_external_unrealize (GtkWidget *widget)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (widget);
  GValue               value = { 0, };

  /* create dummy value */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, FALSE);

  /* send directly (don't queue here) */
  panel_dbus_service_plugin_property_changed (external->unique_id,
                                              PROPERTY_CHANGED_WRAPPER_QUIT,
                                              &value);

  /* unset */
  g_value_unset (&value);

  return (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->unrealize) (widget);
}



static gboolean
panel_plugin_external_plug_removed (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);
  GtkWidget           *dialog;
  gint                 response;
  PanelWindow         *window;

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
      /* remove the child watch and don't leave zomies */
      g_source_remove (external->watch_id);
      g_child_watch_add (external->pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }

  /* increase the restart counter */
  external->n_restarts++;

  /* check if we ask the user what to do */
  if (external->n_restarts > N_RESTART_TRIES)
    {
      /* create dialog */
      dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (socket))),
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                       _("Plugin '%s' unexpectedly left the building, do you want to restart it?"),
                                       panel_module_get_display_name (external->module));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("If you press Execute "
                                                "the panel will try to restart the plugin otherwise it "
                                                "will be permanently removed from the panel."));
      gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK,
                              GTK_STOCK_REMOVE, GTK_RESPONSE_CLOSE, NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

      /* wait for the user's choise */
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      /* reset the restart counter */
      external->n_restarts = 0;
    }
  else
    {
      /* pretend the user clicked the yes button */
      response = GTK_RESPONSE_OK;

      /* print a message we did an autorestart */
      g_message ("Automatically restarting plugin %s-%d, try %d",
                 panel_module_get_name (external->module),
                 external->unique_id, external->n_restarts);
  }

  /* handle the response */
  if (response == GTK_RESPONSE_OK)
    {
      /* get the plugin panel */
      window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (socket)));
      panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

      /* send panel information to the plugin */
      panel_window_set_povider_info (window, GTK_WIDGET (external));

      /* show the socket again (realize will spawn the plugin) */
      gtk_widget_show (GTK_WIDGET (socket));

      /* don't process other events */
      return TRUE;
    }
  else
    {
      /* emit a remove signal, so we can cleanup the plugin configuration (in panel-application) */
      xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (external),
                                              PROVIDER_SIGNAL_REMOVE_PLUGIN);

      /* destroy the socket */
      gtk_widget_destroy (GTK_WIDGET (socket));
    }

  return FALSE;
}



static void
panel_plugin_external_plug_added (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);
  GSList              *li;
  QueuedData          *data;

  /* plug has been added */
  external->plug_embedded = TRUE;

  if (G_LIKELY (external->dbus_queue))
    {
      /* reverse the order fo the queue, since we prepended all the time */
      external->dbus_queue = g_slist_reverse (external->dbus_queue);

      /* flush the queue */
      for (li = external->dbus_queue; li != NULL; li = li->next)
        {
          data = li->data;
          panel_dbus_service_plugin_property_changed (external->unique_id,
                                                      data->property,
                                                      &data->value);
          g_value_unset (&data->value);
          g_slice_free (QueuedData, data);
        }

      /* free the list */
      g_slist_free (external->dbus_queue);
      external->dbus_queue = NULL;
    }
}



static void
panel_plugin_external_provider_signal (XfcePanelPluginProvider       *provider,
                                       XfcePanelPluginProviderSignal  provider_signal)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (provider);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* we handle some signals here, everything else is handled in
   * panel-application */
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
        break;
    }
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
panel_plugin_external_set_property (PanelPluginExternal *external,
                                    DBusPropertyChanged  property,
                                    const GValue        *value)
{
  QueuedData *data;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (value && G_TYPE_CHECK_VALUE (value));

  if (G_LIKELY (external->plug_embedded))
    {
      /* directly send the new property */
      panel_dbus_service_plugin_property_changed (external->unique_id,
                                                  property, value);
    }
  else
    {
      /* queue the property */
      data = g_slice_new (QueuedData);
      data->property = property;
      g_value_init (&data->value, G_VALUE_TYPE (value));
      g_value_copy (value, &data->value);

      /* add to the queue (still in reversed order here) */
      external->dbus_queue = g_slist_prepend (external->dbus_queue, data);
    }
}



static void
panel_plugin_external_set_property_noop (PanelPluginExternal *external,
                                         DBusPropertyChanged  property)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* create a noop value */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, FALSE);

  /* send the value */
  panel_plugin_external_set_property (external, property, &value);

  /* unset */
  g_value_unset (&value);
}



static void
panel_plugin_external_set_size (XfcePanelPluginProvider *provider,
                                gint                     size)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* create a new value */
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, size);

  /* send the value */
  panel_plugin_external_set_property (PANEL_PLUGIN_EXTERNAL (provider),
                                      PROPERTY_CHANGED_PROVIDER_SIZE,
                                      &value);

  /* unset */
  g_value_unset (&value);
}



static void
panel_plugin_external_set_orientation (XfcePanelPluginProvider *provider,
                                       GtkOrientation           orientation)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* create a new value */
  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, orientation);

  /* send the value */
  panel_plugin_external_set_property (PANEL_PLUGIN_EXTERNAL (provider),
                                      PROPERTY_CHANGED_PROVIDER_ORIENTATION,
                                      &value);

  /* unset */
  g_value_unset (&value);
}



static void
panel_plugin_external_set_screen_position (XfcePanelPluginProvider *provider,
                                           XfceScreenPosition       screen_position)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* set the orientation */
  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, screen_position);

  /* send the value */
  panel_plugin_external_set_property (PANEL_PLUGIN_EXTERNAL (provider),
                                      PROPERTY_CHANGED_PROVIDER_SCREEN_POSITION,
                                      &value);

  /* unset */
  g_value_unset (&value);
}



static void
panel_plugin_external_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_set_property_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                           PROPERTY_CHANGED_PROVIDER_EMIT_SAVE);
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
  panel_plugin_external_set_property_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                           PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_CONFIGURE);
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
  panel_plugin_external_set_property_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                           PROPERTY_CHANGED_PROVIDER_EMIT_SHOW_ABOUT);
}



static void
panel_plugin_external_remove (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_plugin_external_set_property_noop (PANEL_PLUGIN_EXTERNAL (provider),
                                           PROPERTY_CHANGED_PROVIDER_REMOVE);
}



static void
panel_plugin_external_set_sensitive (PanelPluginExternal *external)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* set the boolean */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, GTK_WIDGET_IS_SENSITIVE (external));

  /* send message */
  panel_plugin_external_set_property (external,
                                      PROPERTY_CHANGED_WRAPPER_SET_SENSITIVE,
                                      &value);

  /* unset */
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
panel_plugin_external_child_watch_destroyed (gpointer user_data)
{
  PANEL_PLUGIN_EXTERNAL (user_data)->watch_id = 0;
}



GtkWidget *
panel_plugin_external_new (PanelModule  *module,
                           gint          unique_id,
                           gchar       **arguments)
{
  PanelPluginExternal *external;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (unique_id != -1, NULL);

  /* create new object */
  external = g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL, NULL);

  /* set name, id and module */
  external->unique_id = unique_id;
  external->module = g_object_ref (G_OBJECT (module));
  external->arguments = g_strdupv (arguments);

  return GTK_WIDGET (external);
}



void
panel_plugin_external_set_background_alpha (PanelPluginExternal *external,
                                            gint                 percentage)
{
  GValue value = { 0, };

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* set the boolean */
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, percentage);

  /* send message */
  panel_plugin_external_set_property (external,
                                      PROPERTY_CHANGED_WRAPPER_BACKGROUND_ALPHA,
                                      &value);

  /* unset */
  g_value_unset (&value);
}
