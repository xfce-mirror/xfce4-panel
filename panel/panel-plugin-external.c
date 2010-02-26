/* $Id$ */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exo/exo.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-module.h>
#include <panel/panel-plugin-external.h>



static void         panel_plugin_external_class_init          (PanelPluginExternalClass     *klass);
static void         panel_plugin_external_init                (PanelPluginExternal          *external);
static void         panel_plugin_external_provider_init       (XfcePanelPluginProviderIface *iface);
static void         panel_plugin_external_finalize            (GObject                      *object);
static void         panel_plugin_external_realize             (GtkWidget                    *widget);
static void         panel_plugin_external_unrealize           (GtkWidget                    *widget);
static gboolean     panel_plugin_external_client_event        (GtkWidget                    *widget,
                                                               GdkEventClient               *event);
static gboolean     panel_plugin_external_plug_removed        (GtkSocket                    *socket);
static void         panel_plugin_external_send_message        (PanelPluginExternal          *external,
                                                               XfcePanelPluginMessage        message,
                                                               glong                         value);
static void         panel_plugin_external_free_queue          (PanelPluginExternal          *external);
static void         panel_plugin_external_flush_queue         (PanelPluginExternal          *external);
static const gchar *panel_plugin_external_get_name            (XfcePanelPluginProvider      *provider);
static const gchar *panel_plugin_external_get_id              (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_set_size            (XfcePanelPluginProvider      *provider,
                                                               gint                          size);
static void         panel_plugin_external_set_orientation     (XfcePanelPluginProvider      *provider,
                                                               GtkOrientation                orientation);
static void         panel_plugin_external_set_screen_position (XfcePanelPluginProvider      *provider,
                                                               XfceScreenPosition            screen_position);
static void         panel_plugin_external_save                (XfcePanelPluginProvider      *provider);
static void         panel_plugin_external_set_sensitive       (PanelPluginExternal          *external);


struct _PanelPluginExternalClass
{
  GtkSocketClass __parent__;
};

struct _PanelPluginExternal
{
  GtkSocket  __parent__;

  /* plugin information */
  gchar            *id;

  /* startup arguments */
  gchar           **arguments;

  /* the module */
  PanelModule      *module;

  /* the plug window id */
  GdkNativeWindow   plug_window_id;

  /* message queue */
  GSList           *queue;
};

typedef struct
{
  XfcePanelPluginMessage message;
  glong                  value;
}
QueueData;



static GdkAtom message_atom = GDK_NONE;



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
  gtkwidget_class->client_event = panel_plugin_external_client_event;

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_plug_removed;

  /* initialize the global message atom */
  message_atom = gdk_atom_intern_static_string ("XFCE_PANEL_PLUGIN");
}



static void
panel_plugin_external_init (PanelPluginExternal *external)
{
  external->id = NULL;
  external->module = NULL;
  external->plug_window_id = 0;
  external->queue = NULL;
  external->arguments = NULL;

  g_signal_connect (G_OBJECT (external), "notify::sensitive", G_CALLBACK (panel_plugin_external_set_sensitive), NULL);
}



static void
panel_plugin_external_provider_init (XfcePanelPluginProviderIface *iface)
{
  iface->get_name = panel_plugin_external_get_name;
  iface->get_id = panel_plugin_external_get_id;
  iface->set_size = panel_plugin_external_set_size;
  iface->set_orientation = panel_plugin_external_set_orientation;
  iface->set_screen_position = panel_plugin_external_set_screen_position;
  iface->save = panel_plugin_external_save;
}



static void
panel_plugin_external_finalize (GObject *object)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (object);

  /* cleanup */
  g_free (external->id);
  g_strfreev (external->arguments);

  /* free the queue if needed */
  panel_plugin_external_free_queue (external);

  /* release the module */
  g_object_unref (G_OBJECT (external->module));

  (*G_OBJECT_CLASS (panel_plugin_external_parent_class)->finalize) (object);
}



static void
panel_plugin_external_realize (GtkWidget *widget)
{
  PanelPluginExternal  *external = PANEL_PLUGIN_EXTERNAL (widget);
  gchar               **argv;
  GPid                  pid;
  GError               *error = NULL;
  gboolean              succeed;
  gchar                *socket_id;
  gint                  i, argc = 12;

  /* realize the socket first */
  (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->realize) (widget);

  /* get the socket id in a string */
  socket_id = g_strdup_printf ("%d", gtk_socket_get_id (GTK_SOCKET (widget)));

  /* add the number of arguments to the argv count */
  if (G_UNLIKELY (external->arguments != NULL))
    argc += g_strv_length (external->arguments);

  /* allocate argv */
  argv = g_new0 (gchar *, argc);

  /* setup the basic argv */
  argv[0]  = LIBEXECDIR "/xfce4-panel-wrapper";
  argv[1]  = "-n";
  argv[2]  = (gchar *) panel_module_get_internal_name (external->module);
  argv[3]  = "-i";
  argv[4]  = (gchar *) external->id;
  argv[5]  = "-d";
  argv[6]  = (gchar *) panel_module_get_name (external->module);
  argv[7]  = "-f";
  argv[8]  = (gchar *) panel_module_get_library_filename (external->module);
  argv[9]  = "-s";
  argv[10] = socket_id;

  /* append the arguments */
  if (G_UNLIKELY (external->arguments != NULL))
    for (i = 0; external->arguments[i] != NULL; i++)
      argv[i + 11] = external->arguments[i];

  /* close the argv */
  argv[argc - 1] = NULL;

  /* spawn the proccess */
  succeed = gdk_spawn_on_screen (gdk_screen_get_default (), NULL, argv, NULL, 0, NULL, NULL, &pid, &error);

  /* cleanup */
  g_free (socket_id);
  g_free (argv);

  /* handle problem */
  if (G_UNLIKELY (succeed == FALSE))
    {
      /* show warnings */
      g_critical ("Failed to spawn the xfce4-panel-wrapped: %s", error->message);

      /* cleanup */
      g_error_free (error);
    }
}



static void
panel_plugin_external_unrealize (GtkWidget *widget)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (widget);

  /* send message to quit the wrapper */
  panel_plugin_external_send_message (PANEL_PLUGIN_EXTERNAL (external), MESSAGE_QUIT, 0);

  /* don't send or queue messages */
  external->plug_window_id = -1;

  return (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->unrealize) (widget);
}



static gboolean
panel_plugin_external_client_event (GtkWidget      *widget,
                                    GdkEventClient *event)
{
  PanelPluginExternal    *external = PANEL_PLUGIN_EXTERNAL (widget);
  XfcePanelPluginMessage  message;
  glong                   value;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget), FALSE);

  /* check if this even is for us */
  if (G_LIKELY (event->message_type == message_atom))
    {
      /* get the event message and value */
      message = event->data.l[0];
      value = event->data.l[1];

      /* handle the message */
      switch (message)
        {
          case MESSAGE_EXPAND_CHANGED:
            /* emit the expand changed signal */
            g_signal_emit_by_name (G_OBJECT (external), "expand-changed", !!(value == 1));
            break;

          case MESSAGE_MOVE_ITEM:
            /* start a plugin dnd */
            g_signal_emit_by_name (G_OBJECT (external), "move-item", 0);
            break;

          case MESSAGE_ADD_NEW_ITEMS:
            /* show the add new items dialog */
            g_signal_emit_by_name (G_OBJECT (external), "add-new-items", 0);
            break;

          case MESSAGE_PANEL_PREFERENCES:
            /* show the panel preferences dialog */
            g_signal_emit_by_name (G_OBJECT (external), "panel-preferences", 0, gtk_widget_get_toplevel (widget));
            break;

          case MESSAGE_REMOVE:
            /* plugin properly removed, destroy the socket */
            gtk_widget_destroy (widget);
            break;

          case MESSAGE_SET_PLUG_ID:
            /* set the plug window id */
            external->plug_window_id = value;

            /* flush the message queue */
            panel_plugin_external_flush_queue (external);
            break;

          default:
            g_message ("The panel received an unkown message: %d", message);
            break;
        }

      /* we handled the event */
      return TRUE;
    }

  /* propagate event */
  if (GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->client_event)
    return (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->client_event) (widget, event);
  else
    return FALSE;
}



static gboolean
panel_plugin_external_plug_removed (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);

  /* don't send or queue messages */
  external->plug_window_id = -1;

  /* destroy the socket */
  gtk_widget_destroy (GTK_WIDGET (socket));

  if (GTK_SOCKET_CLASS (panel_plugin_external_parent_class)->plug_removed)
    return (*GTK_SOCKET_CLASS (panel_plugin_external_parent_class)->plug_removed) (socket);
  return FALSE;
}



static void
panel_plugin_external_send_message (PanelPluginExternal    *external,
                                    XfcePanelPluginMessage  message,
                                    glong                   value)
{
  GdkEventClient  event;
  QueueData      *data;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (G_LIKELY (external->plug_window_id > 0))
    {
      /* setup the event */
      event.type = GDK_CLIENT_EVENT;
      event.window = GTK_WIDGET (external)->window;
      event.send_event = TRUE;
      event.message_type = message_atom;
      event.data_format = 32;
      event.data.l[0] = message;
      event.data.l[1] = value;
      event.data.l[2] = 0;

      /* don't crash on x errors */
      gdk_error_trap_push ();

      /* send the event to the wrapper */
      gdk_event_send_client_message_for_display (gdk_display_get_default (), (GdkEvent *) &event, external->plug_window_id);

      /* flush the x output buffer */
      gdk_flush ();

      /* pop the push */
      gdk_error_trap_pop ();
    }
  else if (external->plug_window_id == 0)
    {
      /* setup queue data */
      data = g_slice_new0 (QueueData);
      data->message = message;
      data->value = value;

      /* add the message to the list */
      external->queue = g_slist_append (external->queue, data);
    }
}



static void
panel_plugin_external_free_queue (PanelPluginExternal *external)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  if (external->queue != NULL)
    {
      /* cleanup all the queue data */
      for (li = external->queue; li != NULL; li = li->next)
        g_slice_free (QueueData, li->data);

      /* free the list */
      g_slist_free (external->queue);

      /* set to null */
      external->queue = NULL;
    }
}



static void
panel_plugin_external_flush_queue (PanelPluginExternal *external)
{
  GSList    *li;
  QueueData *data;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (external->plug_window_id != 0);

  if (G_LIKELY (external->queue != NULL))
    {
      /* send all messages in the queue */
      for (li = external->queue; li != NULL; li = li->next)
        {
          data = li->data;
          panel_plugin_external_send_message (external, data->message, data->value);
        }

      /* free the queue */
      panel_plugin_external_free_queue (external);
    }
}



static const gchar *
panel_plugin_external_get_name (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), NULL);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return panel_module_get_internal_name (PANEL_PLUGIN_EXTERNAL (provider)->module);
}



static const gchar *
panel_plugin_external_get_id (XfcePanelPluginProvider *provider)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider), NULL);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider), NULL);

  return PANEL_PLUGIN_EXTERNAL (provider)->id;
}



static void
panel_plugin_external_set_size (XfcePanelPluginProvider *provider,
                                gint                     size)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (PANEL_PLUGIN_EXTERNAL (provider), MESSAGE_SET_SIZE, size);
}



static void
panel_plugin_external_set_orientation (XfcePanelPluginProvider *provider,
                                       GtkOrientation           orientation)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (PANEL_PLUGIN_EXTERNAL (provider), MESSAGE_SET_ORIENTATION, orientation);
}



static void
panel_plugin_external_set_screen_position (XfcePanelPluginProvider *provider,
                                           XfceScreenPosition       screen_position)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (PANEL_PLUGIN_EXTERNAL (provider), MESSAGE_SET_SCREEN_POSITION, screen_position);
}



static void
panel_plugin_external_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (PANEL_PLUGIN_EXTERNAL (provider), MESSAGE_SAVE, 0);
}



static void
panel_plugin_external_set_sensitive (PanelPluginExternal *external)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* send message */
  panel_plugin_external_send_message (external, MESSAGE_SET_SENSITIVE, GTK_WIDGET_IS_SENSITIVE (external) ? 1 : 0);
}



XfcePanelPluginProvider *
panel_plugin_external_new (PanelModule  *module,
                           const gchar  *name,
                           const gchar  *id,
                           gchar       **arguments)
{
  PanelPluginExternal *external;

  panel_return_val_if_fail (PANEL_IS_MODULE (module), NULL);
  panel_return_val_if_fail (name != NULL, NULL);
  panel_return_val_if_fail (id != NULL, NULL);

  /* create new object */
  external = g_object_new (PANEL_TYPE_PLUGIN_EXTERNAL, NULL);

  /* set name, id and module */
  external->id = g_strdup (id);
  external->module = g_object_ref (G_OBJECT (module));
  external->arguments = g_strdupv (arguments);

  return XFCE_PANEL_PLUGIN_PROVIDER (external);
}



void
panel_plugin_external_set_background_alpha (PanelPluginExternal *external,
                                            gint                 percentage)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (external, MESSAGE_SET_BACKGROUND_ALPHA, percentage);
}



void
panel_plugin_external_set_active_panel (PanelPluginExternal *external,
                                        gboolean             active)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external));

  /* send the signal to the wrapper */
  panel_plugin_external_send_message (external, MESSAGE_SET_ACTIVE_PANEL, active ? 1 : 0);
}
