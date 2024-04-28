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

#include "panel/panel-dialogs.h"
#include "panel/panel-module.h"
#include "panel/panel-plugin-external.h"

#include "common/panel-dbus.h"
#include "common/panel-debug.h"
#include "common/panel-private.h"
#include "common/panel-utils.h"

#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>



#define get_instance_private(instance) \
  ((PanelPluginExternalPrivate *) panel_plugin_external_get_instance_private (PANEL_PLUGIN_EXTERNAL (instance)))

static void
panel_plugin_external_provider_init (XfcePanelPluginProviderInterface *iface);
static void
panel_plugin_external_finalize (GObject *object);
static void
panel_plugin_external_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void
panel_plugin_external_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void
panel_plugin_external_realize (GtkWidget *widget);
static void
panel_plugin_external_unrealize (GtkWidget *widget);
static gboolean
panel_plugin_external_child_ask_restart (PanelPluginExternal *external);
static void
panel_plugin_external_child_spawn (PanelPluginExternal *external);
static void
panel_plugin_external_child_respawn_schedule (PanelPluginExternal *external);
static void
panel_plugin_external_child_watch (GPid pid,
                                   gint status,
                                   gpointer user_data);
static void
panel_plugin_external_child_watch_destroyed (gpointer user_data);
static void
panel_plugin_external_queue_free (PanelPluginExternal *external);
static void
panel_plugin_external_queue_send_to_child (PanelPluginExternal *external);
static const gchar *
panel_plugin_external_get_name (XfcePanelPluginProvider *provider);
static gint
panel_plugin_external_get_unique_id (XfcePanelPluginProvider *provider);
static void
panel_plugin_external_set_size (XfcePanelPluginProvider *provider,
                                gint size);
static void
panel_plugin_external_set_icon_size (XfcePanelPluginProvider *provider,
                                     gint icon_size);
static void
panel_plugin_external_set_dark_mode (XfcePanelPluginProvider *provider,
                                     gboolean dark_mode);
static void
panel_plugin_external_set_mode (XfcePanelPluginProvider *provider,
                                XfcePanelPluginMode mode);
static void
panel_plugin_external_set_nrows (XfcePanelPluginProvider *provider,
                                 guint rows);
static void
panel_plugin_external_set_screen_position (XfcePanelPluginProvider *provider,
                                           XfceScreenPosition screen_position);
static void
panel_plugin_external_save (XfcePanelPluginProvider *provider);
static gboolean
panel_plugin_external_get_show_configure (XfcePanelPluginProvider *provider);
static void
panel_plugin_external_show_configure (XfcePanelPluginProvider *provider);
static gboolean
panel_plugin_external_get_show_about (XfcePanelPluginProvider *provider);
static void
panel_plugin_external_show_about (XfcePanelPluginProvider *provider);
static void
panel_plugin_external_removed (XfcePanelPluginProvider *provider);
static gboolean
panel_plugin_external_remote_event (XfcePanelPluginProvider *provider,
                                    const gchar *name,
                                    const GValue *value,
                                    guint *handler_id);
static void
panel_plugin_external_set_locked (XfcePanelPluginProvider *provider,
                                  gboolean locked);
static void
panel_plugin_external_ask_remove (XfcePanelPluginProvider *provider);
static void
panel_plugin_external_set_sensitive (PanelPluginExternal *external);



typedef struct _PanelPluginExternalPrivate
{
  /* startup arguments */
  PanelModule *module;
  gint unique_id;
  gchar **arguments;

  guint embedded : 1;

  /* dbus message queue */
  GSList *queue;

  /* auto restart timer */
  GTimer *restart_timer;

  /* child watch data */
  GPid pid;
  guint watch_id;

  /* delayed spawning */
  guint spawn_timeout_id;
} PanelPluginExternalPrivate;

enum
{
  PROP_0,
  PROP_MODULE,
  PROP_UNIQUE_ID,
  PROP_ARGUMENTS
};



G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PanelPluginExternal, panel_plugin_external, GTK_TYPE_BOX,
                                  G_ADD_PRIVATE (PanelPluginExternal)
                                  G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN_PROVIDER,
                                                         panel_plugin_external_provider_init))



static void
panel_plugin_external_class_init (PanelPluginExternalClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_finalize;
  gobject_class->set_property = panel_plugin_external_set_property;
  gobject_class->get_property = panel_plugin_external_get_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_plugin_external_realize;
  gtkwidget_class->unrealize = panel_plugin_external_unrealize;

  g_object_class_install_property (gobject_class,
                                   PROP_UNIQUE_ID,
                                   g_param_spec_int ("unique-id", NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                                       | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_MODULE,
                                   g_param_spec_object ("module", NULL, NULL,
                                                        PANEL_TYPE_MODULE,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                                          | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class,
                                   PROP_ARGUMENTS,
                                   g_param_spec_boxed ("arguments", NULL, NULL,
                                                       G_TYPE_STRV,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS
                                                         | G_PARAM_CONSTRUCT_ONLY));
}



static void
panel_plugin_external_init (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  priv->module = NULL;
  priv->unique_id = -1;

  priv->arguments = NULL;
  priv->queue = NULL;
  priv->restart_timer = NULL;
  priv->embedded = FALSE;
  priv->pid = 0;
  priv->spawn_timeout_id = 0;

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
  iface->set_icon_size = panel_plugin_external_set_icon_size;
  iface->set_dark_mode = panel_plugin_external_set_dark_mode;
  iface->set_mode = panel_plugin_external_set_mode;
  iface->set_nrows = panel_plugin_external_set_nrows;
  iface->set_screen_position = panel_plugin_external_set_screen_position;
  iface->save = panel_plugin_external_save;
  iface->get_show_configure = panel_plugin_external_get_show_configure;
  iface->show_configure = panel_plugin_external_show_configure;
  iface->get_show_about = panel_plugin_external_get_show_about;
  iface->show_about = panel_plugin_external_show_about;
  iface->removed = panel_plugin_external_removed;
  iface->remote_event = panel_plugin_external_remote_event;
  iface->set_locked = panel_plugin_external_set_locked;
  iface->ask_remove = panel_plugin_external_ask_remove;
}



static void
panel_plugin_external_finalize (GObject *object)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  panel_debug (PANEL_DEBUG_EXTERNAL,
               "%s-%d: plugin is being finalized",
               panel_module_get_name (priv->module),
               priv->unique_id);

  if (priv->spawn_timeout_id != 0)
    g_source_remove (priv->spawn_timeout_id);

  if (priv->watch_id != 0)
    {
      /* remove the child watch and don't leave zombies */
      g_source_remove (priv->watch_id);
      priv->watch_id = 0;
      if (priv->pid != 0)
        g_child_watch_add (priv->pid,
                           (GChildWatchFunc) (void (*) (void)) g_spawn_close_pid,
                           NULL);
    }

  panel_plugin_external_queue_free (external);

  g_strfreev (priv->arguments);

  if (priv->restart_timer != NULL)
    g_timer_destroy (priv->restart_timer);

  g_object_unref (G_OBJECT (priv->module));

  (*G_OBJECT_CLASS (panel_plugin_external_parent_class)->finalize) (object);
}



static void
panel_plugin_external_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  PanelPluginExternalPrivate *priv = get_instance_private (object);

  switch (prop_id)
    {
    case PROP_UNIQUE_ID:
      g_value_set_int (value, priv->unique_id);
      break;

    case PROP_ARGUMENTS:
      g_value_set_boxed (value, priv->arguments);
      break;

    case PROP_MODULE:
      g_value_set_object (value, priv->module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_plugin_external_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  PanelPluginExternalPrivate *priv = get_instance_private (object);

  switch (prop_id)
    {
    case PROP_UNIQUE_ID:
      priv->unique_id = g_value_get_int (value);
      break;

    case PROP_ARGUMENTS:
      priv->arguments = g_value_dup_boxed (value);
      break;

    case PROP_MODULE:
      priv->module = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
panel_plugin_external_realize (GtkWidget *widget)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (widget);
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  /* realize the widget first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->realize) (widget);

  if (priv->pid == 0)
    {
      if (priv->spawn_timeout_id != 0)
        g_source_remove (priv->spawn_timeout_id);

      panel_plugin_external_child_spawn (external);
    }
  else
    {
      /* the child was asked to quit during unrealize and there is
       * still an pid, so wait for the child to quit and then
       * spawn it again */
      panel_plugin_external_child_respawn_schedule (external);
    }
}



static void
panel_plugin_external_unrealize (GtkWidget *widget)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (widget);
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  /* ask the child to quit */
  if (priv->pid != 0)
    {
      if (priv->embedded)
        panel_plugin_external_queue_add_action (external, PROVIDER_PROP_TYPE_ACTION_QUIT);
      else
        kill (priv->pid, SIGTERM);
    }

  panel_debug (PANEL_DEBUG_EXTERNAL,
               "%s-%d: plugin unrealized; quitting child",
               panel_module_get_name (priv->module),
               priv->unique_id);

  (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->unrealize) (widget);
}



static gboolean
panel_plugin_external_child_ask_restart_dialog (GtkWindow *parent,
                                                const gchar *plugin_name)
{
  GtkWidget *dialog;
  gint response;

  panel_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  panel_return_val_if_fail (plugin_name != NULL, FALSE);

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   _("Plugin \"%s\" unexpectedly left the panel, do you want to restart it?"),
                                   plugin_name);
  gtk_window_set_title (GTK_WINDOW (dialog),
                        _("Plugin Restart"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("The plugin restarted more than once in "
                                            "the last %d seconds. If you press Execute the panel will try to restart "
                                            "the plugin otherwise it will be permanently removed from the panel."),
                                            PANEL_PLUGIN_AUTO_RESTART);
  gtk_dialog_add_buttons (
    GTK_DIALOG (dialog), _("_Execute"), GTK_RESPONSE_OK, _("_Remove"), GTK_RESPONSE_CLOSE, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_OK);
}



static gboolean
panel_plugin_external_remove (gpointer data)
{
  /* cleanup the plugin configuration (in PanelApplication) and destroy 'external' */
  xfce_panel_plugin_provider_emit_signal (data, PROVIDER_SIGNAL_REMOVE_PLUGIN);
  return FALSE;
}



static gboolean
panel_plugin_external_child_ask_restart (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  GtkWidget *toplevel;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (external));
  panel_return_val_if_fail (PANEL_IS_WINDOW (toplevel), FALSE);

  if (priv->restart_timer == NULL
      || g_timer_elapsed (priv->restart_timer, NULL) > PANEL_PLUGIN_AUTO_RESTART)
    {
      g_message ("Plugin %s-%d has been automatically restarted after crash.",
                 panel_module_get_name (priv->module),
                 priv->unique_id);
    }
  else if (!panel_plugin_external_child_ask_restart_dialog (GTK_WINDOW (toplevel),
                                                            panel_module_get_display_name (priv->module)))
    {
      if (priv->watch_id != 0)
        {
          /* remove the child watch and don't leave zombies */
          g_source_remove (priv->watch_id);
          priv->watch_id = 0;
          if (priv->pid != 0)
            g_child_watch_add (priv->pid,
                               (GChildWatchFunc) (void (*) (void)) g_spawn_close_pid,
                               NULL);
        }

      /* delay this until we get out of any other idle func, as this triggers the
       * finalization of 'external' */
      g_idle_add_full (G_PRIORITY_HIGH, panel_plugin_external_remove, external, NULL);

      return FALSE;
    }

  /* create or reset the restart timer */
  if (priv->restart_timer == NULL)
    priv->restart_timer = g_timer_new ();
  else
    g_timer_reset (priv->restart_timer);

  return TRUE;
}



static void
panel_plugin_external_child_spawn (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  gchar **argv, **dbg_argv, **tmp_argv;
  GError *error = NULL;
  gboolean succeed;
  GPid pid;
  gchar *program, *cmd_line;
  guint i;
  gint tmp_argc;
  gint64 timestamp;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (gtk_widget_get_realized (GTK_WIDGET (external)));

  /* set plugin specific arguments */
  argv = (*PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->get_argv) (external, priv->arguments);
  panel_return_if_fail (argv != NULL);

  /* check debugging state */
  if (panel_debug_has_domain (PANEL_DEBUG_GDB)
      || panel_debug_has_domain (PANEL_DEBUG_VALGRIND))
    {
      timestamp = g_get_real_time ();
      cmd_line = NULL;
      program = NULL;

      /* note that if the program was not found in PATH, we already warned
       * for it in panel_debug_notify_proxy, so no need to do that again */
      if (panel_debug_has_domain (PANEL_DEBUG_GDB))
        {
          program = g_find_program_in_path ("gdb");
          if (G_LIKELY (program != NULL))
            {
              cmd_line = g_strdup_printf ("%s -batch "
                                          "-ex 'set logging file %s" G_DIR_SEPARATOR_S "%li_gdb_%s_%s.log' "
                                          "-ex 'set logging on' "
                                          "-ex 'set pagination off' "
                                          "-ex 'set logging redirect on' "
                                          "-ex 'run' "
                                          "-ex 'backtrace full' "
                                          "-ex 'info registers' "
                                          "-args",
                                          program, g_get_tmp_dir (), timestamp / G_USEC_PER_SEC,
                                          panel_module_get_name (priv->module),
                                          argv[PLUGIN_ARGV_UNIQUE_ID]);
            }
        }
      else if (panel_debug_has_domain (PANEL_DEBUG_VALGRIND))
        {
          program = g_find_program_in_path ("valgrind");
          if (G_LIKELY (program != NULL))
            {
              cmd_line = g_strdup_printf ("%s "
                                          "--log-file='%s" G_DIR_SEPARATOR_S "%li_valgrind_%s_%s.log' "
                                          "--leak-check=full --show-reachable=yes -v ",
                                          program, g_get_tmp_dir (), timestamp / G_USEC_PER_SEC,
                                          panel_module_get_name (priv->module),
                                          argv[PLUGIN_ARGV_UNIQUE_ID]);
            }
        }

      if (cmd_line != NULL
          && g_shell_parse_argv (cmd_line, &tmp_argc, &tmp_argv, &error))
        {
          dbg_argv = g_new0 (gchar *, tmp_argc + g_strv_length (argv) + 1);

          for (i = 0; tmp_argv[i] != NULL; i++)
            dbg_argv[i] = tmp_argv[i];
          g_free (tmp_argv);

          for (i = 0; argv[i] != NULL; i++)
            dbg_argv[i + tmp_argc] = argv[i];
          g_free (argv);

          argv = dbg_argv;
        }
      else
        {
          panel_debug (PANEL_DEBUG_EXTERNAL,
                       "%s-%d: Failed to run the plugin in %s: %s",
                       panel_module_get_name (priv->module),
                       priv->unique_id, program,
                       cmd_line != NULL ? error->message : "debugger not found");
          g_error_free (error);

          return;
        }

      g_free (program);
      g_free (cmd_line);
    }

  /* spawn the proccess */
  succeed = PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->spawn (external, argv, &pid, &error);

  panel_debug (PANEL_DEBUG_EXTERNAL,
               "%s-%d: child spawned; pid=%d, argc=%d",
               panel_module_get_name (priv->module),
               priv->unique_id, pid, g_strv_length (argv));

  if (G_LIKELY (succeed))
    {
      /* watch the child */
      priv->pid = pid;
      priv->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                               panel_plugin_external_child_watch, external,
                                               panel_plugin_external_child_watch_destroyed);
    }
  else
    {
      g_critical ("Failed to spawn the xfce4-panel-wrapper: %s", error->message);
      g_error_free (error);
    }

  g_strfreev (argv);
}



static gboolean
panel_plugin_external_child_respawn (gpointer user_data)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (user_data);
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  GtkWidget *window;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);

  /* abort startup if the plugin is not realized */
  if (!gtk_widget_get_realized (GTK_WIDGET (external)))
    return FALSE;

  /* delay startup if the old child is still embedded */
  if (priv->embedded
      || priv->pid != 0)
    {
      panel_debug (PANEL_DEBUG_EXTERNAL,
                   "%s-%d: still a child embedded, respawn delayed",
                   panel_module_get_name (priv->module), priv->unique_id);

      return TRUE;
    }

  panel_plugin_external_queue_free (external);

  window = gtk_widget_get_toplevel (GTK_WIDGET (external));
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_window_set_provider_info (PANEL_WINDOW (window), GTK_WIDGET (external), FALSE);

  panel_plugin_external_child_spawn (external);

  /* stop timeout */
  return FALSE;
}



static void
panel_plugin_external_child_respawn_destroyed (gpointer user_data)
{
  get_instance_private (user_data)->spawn_timeout_id = 0;
}



static void
panel_plugin_external_child_respawn_schedule (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (priv->spawn_timeout_id == 0)
    {
      panel_debug (PANEL_DEBUG_EXTERNAL,
                   "%s-%d: scheduled a respawn of the child",
                   panel_module_get_name (priv->module), priv->unique_id);

      /* schedule a restart timeout */
      priv->spawn_timeout_id = g_timeout_add_full (G_PRIORITY_LOW, 100, panel_plugin_external_child_respawn,
                                                   external, panel_plugin_external_child_respawn_destroyed);
    }
}



static void
panel_plugin_external_child_watch (GPid pid,
                                   gint status,
                                   gpointer user_data)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (user_data);
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  gboolean auto_restart = FALSE;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (priv->pid == pid);

  /* reset the pid, it can't be embedded as well */
  priv->pid = 0;
  panel_plugin_external_set_embedded (external, FALSE);

  panel_debug (PANEL_DEBUG_EXTERNAL,
               "%s-%d: child exited with status %d",
               panel_module_get_name (priv->module),
               priv->unique_id, status);

  if (WIFEXITED (status))
    {
      /* extract our return value from the status */
      switch (WEXITSTATUS (status))
        {
        case PLUGIN_EXIT_SUCCESS:
          /* normal exit, do not try to restart */
          goto close_pid;

        case PLUGIN_EXIT_SUCCESS_AND_RESTART:
          /* the panel asked for a restart, so do not bother the user */
          auto_restart = TRUE;
          break;

        case PLUGIN_EXIT_FAILURE:
          /* do nothing, maybe we try to restart */
          break;

        case PLUGIN_EXIT_ARGUMENTS_FAILED:
        case PLUGIN_EXIT_PREINIT_FAILED:
        case PLUGIN_EXIT_CHECK_FAILED:
        case PLUGIN_EXIT_NO_PROVIDER:
          g_warning ("Plugin %s-%d exited with status %d, removing from panel configuration",
                     panel_module_get_name (priv->module),
                     priv->unique_id, WEXITSTATUS (status));

          /* delay this until we get out of any other idle func, as this triggers the
           * finalization of 'external' */
          g_idle_add_full (G_PRIORITY_HIGH, panel_plugin_external_remove, external, NULL);

          goto close_pid;
        }
    }
  else if (WIFSIGNALED (status))
    {
      switch (WTERMSIG (status))
        {
        case SIGUSR1:
          /* the panel asked for a restart, so do not bother the user */
          auto_restart = TRUE;
          break;
        }
    }

  if (gtk_widget_get_realized (GTK_WIDGET (external))
      && (auto_restart || panel_plugin_external_child_ask_restart (external)))
    {
      panel_plugin_external_child_respawn_schedule (external);
    }

close_pid:
  g_spawn_close_pid (pid);
}



static void
panel_plugin_external_child_watch_destroyed (gpointer user_data)
{
  get_instance_private (user_data)->watch_id = 0;
}



static void
panel_plugin_external_queue_free (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  PluginProperty *property;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  for (GSList *li = priv->queue; li != NULL; li = li->next)
    {
      property = li->data;
      g_value_unset (&property->value);
      g_slice_free (PluginProperty, property);
    }

  g_slist_free (priv->queue);
  priv->queue = NULL;
}



static void
panel_plugin_external_queue_send_to_child (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (priv->queue != NULL)
    {
      priv->queue = g_slist_reverse (priv->queue);

      (*PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->set_properties) (external, priv->queue);

      panel_plugin_external_queue_free (external);
    }
}



static const gchar *
panel_plugin_external_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), NULL);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return panel_module_get_name (get_instance_private (provider)->module);
}



static gint
panel_plugin_external_get_unique_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), -1);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), -1);

  return get_instance_private (provider)->unique_id;
}



static void
panel_plugin_external_set_size (XfcePanelPluginProvider *provider,
                                gint size)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, size);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_SIZE, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_icon_size (XfcePanelPluginProvider *provider,
                                     gint icon_size)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, icon_size);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_ICON_SIZE, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_dark_mode (XfcePanelPluginProvider *provider,
                                     gboolean dark_mode)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, dark_mode);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_DARK_MODE, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_mode (XfcePanelPluginProvider *provider,
                                XfcePanelPluginMode mode)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* PluginExternal is a GtkBox since 4.19.0 so it must be oriented with the panel to not
   * allow the remote plug to expand in the wrong direction */
  gtk_orientable_set_orientation (GTK_ORIENTABLE (provider),
                                  mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL ? GTK_ORIENTATION_HORIZONTAL
                                                                            : GTK_ORIENTATION_VERTICAL);

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, mode);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_MODE, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_nrows (XfcePanelPluginProvider *provider,
                                 guint rows)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, rows);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_NROWS, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_set_screen_position (XfcePanelPluginProvider *provider,
                                           XfceScreenPosition screen_position)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, screen_position);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_SCREEN_POSITION, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_queue_add_action (PANEL_PLUGIN_EXTERNAL (provider),
                                          PROVIDER_PROP_TYPE_ACTION_SAVE);
}



static gboolean
panel_plugin_external_get_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL_GET_CLASS (provider)->get_show_configure (PANEL_PLUGIN_EXTERNAL (provider));
}



static void
panel_plugin_external_show_configure (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_queue_add_action (PANEL_PLUGIN_EXTERNAL (provider),
                                          PROVIDER_PROP_TYPE_ACTION_SHOW_CONFIGURE);
}



static gboolean
panel_plugin_external_get_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), FALSE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), FALSE);

  return PANEL_PLUGIN_EXTERNAL_GET_CLASS (provider)->get_show_about (PANEL_PLUGIN_EXTERNAL (provider));
}



static void
panel_plugin_external_show_about (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_queue_add_action (PANEL_PLUGIN_EXTERNAL (provider),
                                          PROVIDER_PROP_TYPE_ACTION_SHOW_ABOUT);
}



static void
panel_plugin_external_removed (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_queue_add_action (PANEL_PLUGIN_EXTERNAL (provider),
                                          PROVIDER_PROP_TYPE_ACTION_REMOVED);
}



static gboolean
panel_plugin_external_remote_event (XfcePanelPluginProvider *provider,
                                    const gchar *name,
                                    const GValue *value,
                                    guint *handle)
{
  return (*PANEL_PLUGIN_EXTERNAL_GET_CLASS (provider)->remote_event) (PANEL_PLUGIN_EXTERNAL (provider),
                                                                      name, value, handle);
}



static void
panel_plugin_external_set_locked (XfcePanelPluginProvider *provider,
                                  gboolean locked)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, locked);

  panel_plugin_external_queue_add (PANEL_PLUGIN_EXTERNAL (provider),
                                   PROVIDER_PROP_TYPE_SET_LOCKED, &value);

  g_value_unset (&value);
}



static void
panel_plugin_external_ask_remove (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  panel_plugin_external_queue_add_action (PANEL_PLUGIN_EXTERNAL (provider),
                                          PROVIDER_PROP_TYPE_ACTION_ASK_REMOVE);
}



static void
panel_plugin_external_set_sensitive (PanelPluginExternal *external)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, gtk_widget_is_sensitive (GTK_WIDGET (external)));

  panel_plugin_external_queue_add (external, PROVIDER_PROP_TYPE_SET_SENSITIVE,
                                   &value);

  g_value_unset (&value);
}



void
panel_plugin_external_queue_add (PanelPluginExternal *external,
                                 XfcePanelPluginProviderPropType type,
                                 const GValue *value)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);
  PluginProperty *prop;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (G_TYPE_CHECK_VALUE (value));

  prop = g_slice_new0 (PluginProperty);
  prop->type = type;
  g_value_init (&prop->value, G_VALUE_TYPE (value));
  g_value_copy (value, &prop->value);

  priv->queue = g_slist_prepend (priv->queue, prop);

  if (priv->embedded)
    panel_plugin_external_queue_send_to_child (external);
}



void
panel_plugin_external_queue_add_action (PanelPluginExternal *external,
                                        XfcePanelPluginProviderPropType type)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* add to queue with noop boolean */
  g_value_init (&value, G_TYPE_BOOLEAN);
  panel_plugin_external_queue_add (external, type, &value);
  g_value_unset (&value);
}



void
panel_plugin_external_restart (PanelPluginExternal *external)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (priv->pid != 0)
    {
      panel_debug (PANEL_DEBUG_EXTERNAL,
                   "%s-%d: child asked to restart; pid=%d",
                   panel_module_get_name (priv->module),
                   priv->unique_id, priv->pid);

      panel_plugin_external_queue_free (external);

      if (priv->embedded)
        panel_plugin_external_queue_add_action (external, PROVIDER_PROP_TYPE_ACTION_QUIT_FOR_RESTART);
      else
        kill (priv->pid, SIGUSR1);
    }
}



void
panel_plugin_external_set_opacity (PanelPluginExternal *external,
                                   gdouble opacity)
{
  GValue value = G_VALUE_INIT;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  g_value_init (&value, G_TYPE_DOUBLE);
  g_value_set_double (&value, opacity);

  panel_plugin_external_queue_add (external,
                                   PROVIDER_PROP_TYPE_SET_OPACITY,
                                   &value);

  g_value_unset (&value);
}



void
panel_plugin_external_set_background_color (PanelPluginExternal *external,
                                            const GdkRGBA *color)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->set_background_color (external, color);
}



void
panel_plugin_external_set_background_image (PanelPluginExternal *external,
                                            const gchar *image)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->set_background_image (external, image);
}



void
panel_plugin_external_set_geometry (PanelPluginExternal *external,
                                    PanelWindow *window)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->set_geometry (external, window);
}



gboolean
panel_plugin_external_pointer_is_outside (PanelPluginExternal *external)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);

  if (!get_instance_private (external)->embedded)
    return TRUE;

  return PANEL_PLUGIN_EXTERNAL_GET_CLASS (external)->pointer_is_outside (external);
}



gboolean
panel_plugin_external_get_embedded (PanelPluginExternal *external)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), FALSE);

  return get_instance_private (external)->embedded;
}



void
panel_plugin_external_set_embedded (PanelPluginExternal *external,
                                    gboolean embedded)
{
  PanelPluginExternalPrivate *priv = get_instance_private (external);

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (priv->embedded == !!embedded)
    return;

  priv->embedded = embedded;
  if (embedded)
    {
      panel_debug (PANEL_DEBUG_EXTERNAL,
                   "%s-%d: child is embedded; %d properties in queue",
                   panel_module_get_name (priv->module),
                   priv->unique_id,
                   g_slist_length (priv->queue));

      /* send queue to wrapper */
      panel_plugin_external_queue_send_to_child (external);
    }
  else
    {
      panel_debug (PANEL_DEBUG_EXTERNAL,
                   "%s-%d: child is unembedded",
                   panel_module_get_name (priv->module),
                   priv->unique_id);
    }
}



GPid
panel_plugin_external_get_pid (PanelPluginExternal *external)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external), 0);

  return get_instance_private (external)->pid;
}
