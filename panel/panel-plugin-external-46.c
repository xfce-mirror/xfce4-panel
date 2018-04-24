/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <common/panel-debug.h>

#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-module.h>
#include <panel/panel-plugin-external-46.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-window.h>
#include <panel/panel-dialogs.h>



static void              panel_plugin_external_46_finalize       (GObject              *object);
static void              panel_plugin_external_46_realize        (GtkWidget            *widget);
static void              panel_plugin_external_46_unrealize      (GtkWidget            *widget);
static GdkFilterReturn   panel_plugin_external_46_event_filter   (GdkXEvent            *xevent,
                                                                  GdkEvent             *event,
                                                                  gpointer              data);
static gchar           **panel_plugin_external_46_get_argv       (PanelPluginExternal  *external,
                                                                  gchar               **arguments);
static void              panel_plugin_external_46_set_properties (PanelPluginExternal  *external,
                                                                  GSList               *properties);
static gboolean          panel_plugin_external_46_remote_event   (PanelPluginExternal  *external,
                                                                  const gchar          *name,
                                                                  const GValue         *value,
                                                                  guint                *handle);



struct _PanelPluginExternal46Class
{
  PanelPluginExternalClass __parent__;
};

struct _PanelPluginExternal46
{
  PanelPluginExternal __parent__;

  gchar *background_image;
};



static Atom panel_atom = None;



G_DEFINE_TYPE (PanelPluginExternal46, panel_plugin_external_46, PANEL_TYPE_PLUGIN_EXTERNAL)



static void
panel_plugin_external_46_class_init (PanelPluginExternal46Class *klass)
{
  GObjectClass             *gobject_class;
  GtkWidgetClass           *gtkwidget_class;
  PanelPluginExternalClass *plugin_external_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_plugin_external_46_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_plugin_external_46_realize;
  gtkwidget_class->unrealize = panel_plugin_external_46_unrealize;

  plugin_external_class = PANEL_PLUGIN_EXTERNAL_CLASS (klass);
  plugin_external_class->get_argv = panel_plugin_external_46_get_argv;
  plugin_external_class->set_properties = panel_plugin_external_46_set_properties;
  plugin_external_class->remote_event = panel_plugin_external_46_remote_event;

  panel_atom = XInternAtom( gdk_x11_get_default_xdisplay (), _PANEL_CLIENT_EVENT_ATOM, False);
}



static void
panel_plugin_external_46_init (PanelPluginExternal46 *external)
{
  external->background_image = NULL;
}



static void
panel_plugin_external_46_finalize (GObject *object)
{
  PanelPluginExternal46 *external = PANEL_PLUGIN_EXTERNAL_46 (object);

  g_free (external->background_image);

  (*G_OBJECT_CLASS (panel_plugin_external_46_parent_class)->finalize) (object);
}



static void
panel_plugin_external_46_realize (GtkWidget *widget)
{
  GdkWindow *window;

  GTK_WIDGET_CLASS (panel_plugin_external_46_parent_class)->realize (widget);

  window = gtk_widget_get_window (widget);
  if (G_UNLIKELY (window == NULL))
    return;

  gdk_window_add_filter (window, panel_plugin_external_46_event_filter, widget);
}



static void
panel_plugin_external_46_unrealize (GtkWidget *widget)
{
  GdkWindow *window;

  window = gtk_widget_get_window (widget);
  if (G_LIKELY (window != NULL))
    {
      gdk_window_remove_filter (window, panel_plugin_external_46_event_filter, NULL);
    }

  GTK_WIDGET_CLASS (panel_plugin_external_46_parent_class)->unrealize (widget);
}



static GdkFilterReturn
panel_plugin_external_46_event_filter (GdkXEvent *xevent,
                                       GdkEvent  *event,
                                       gpointer   data)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (data);
  gint                 provider_signal;
  XClientMessageEvent *client_event;

  if (((XEvent*)xevent)->type != ClientMessage)
    return GDK_FILTER_CONTINUE;

  client_event = (XClientMessageEvent*)xevent;

  if (client_event->message_type == panel_atom)
    {
      provider_signal = client_event->data.s[0];

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

      return GDK_FILTER_REMOVE;
    }

  return GDK_FILTER_CONTINUE;
}



static gchar **
panel_plugin_external_46_get_argv (PanelPluginExternal  *external,
                                   gchar               **arguments)
{
  PanelPluginExternal46  *external46 = PANEL_PLUGIN_EXTERNAL_46 (external);
  guint                  i, argc = PLUGIN_ARGV_ARGUMENTS;
  gchar                 **argv;

  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external), NULL);
  panel_return_val_if_fail (PANEL_IS_MODULE (external->module), NULL);
  panel_return_val_if_fail (GTK_IS_SOCKET (external), NULL);

  /* add the number of arguments to the argc count */
  if (G_UNLIKELY (arguments != NULL))
    argc += g_strv_length (arguments);

  /* setup the basic argv */
  argv = g_new0 (gchar *, argc + 1);
  argv[PLUGIN_ARGV_0] = g_strdup (panel_module_get_filename (external->module));
  argv[PLUGIN_ARGV_FILENAME] = g_strdup (""); /* unused, for wrapper only */
  argv[PLUGIN_ARGV_UNIQUE_ID] = g_strdup_printf ("%d", external->unique_id);;
  argv[PLUGIN_ARGV_SOCKET_ID] = g_strdup_printf ("%lu", gtk_socket_get_id (GTK_SOCKET (external)));;
  argv[PLUGIN_ARGV_NAME] = g_strdup (panel_module_get_name (external->module));
  argv[PLUGIN_ARGV_DISPLAY_NAME] = g_strdup (panel_module_get_display_name (external->module));
  argv[PLUGIN_ARGV_COMMENT] = g_strdup (panel_module_get_comment (external->module));

  if (external46->background_image != NULL)
    argv[PLUGIN_ARGV_BACKGROUND_IMAGE] = g_strdup (external46->background_image);
  else
    argv[PLUGIN_ARGV_BACKGROUND_IMAGE] = g_strdup ("");

  /* append the arguments */
  if (G_UNLIKELY (arguments != NULL))
    {
      for (i = 0; arguments[i] != NULL; i++)
        argv[i + PLUGIN_ARGV_ARGUMENTS] = g_strdup (arguments[i]);
    }

  return argv;
}



static void
panel_plugin_external_46_set_properties (PanelPluginExternal *external,
                                         GSList              *properties)
{
  PanelPluginExternal46 *external46 = PANEL_PLUGIN_EXTERNAL_46 (external);
  GSList                *li;
  XEvent                 event;
  PluginProperty        *property;
  GdkRGBA                color;
  GdkWindow             *window;

  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external));
  panel_return_if_fail (panel_atom != None);
  panel_return_if_fail (PANEL_IS_MODULE (external->module));

  if (!gtk_widget_get_realized (GTK_WIDGET (external)))
    return;

  event.xclient.type = ClientMessage;
  panel_return_if_fail (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET (external))));
  event.xclient.window = gdk_x11_window_get_xid (gtk_widget_get_window (GTK_WIDGET (external)));
  event.xclient.message_type = panel_atom;
  event.xclient.format = 16;

  gdk_error_trap_push ();

  for (li = properties; li != NULL; li = li->next)
    {
      property = li->data;

      event.xclient.data.s[0] = property->type;

      switch (property->type)
        {
        case PROVIDER_PROP_TYPE_SET_SIZE:
        case PROVIDER_PROP_TYPE_SET_MODE:
        case PROVIDER_PROP_TYPE_SET_NROWS:
        case PROVIDER_PROP_TYPE_SET_SCREEN_POSITION:
          event.xclient.data.s[1] = g_value_get_int (&property->value);
          break;

        case PROVIDER_PROP_TYPE_SET_LOCKED:
        case PROVIDER_PROP_TYPE_SET_SENSITIVE:
          event.xclient.data.s[1] = g_value_get_boolean (&property->value);
          break;

        case PROVIDER_PROP_TYPE_SET_BACKGROUND_COLOR:
          if (gdk_rgba_parse (&color, g_value_get_string (&property->value)))
            {
              event.xclient.data.s[1] = color.red;
              event.xclient.data.s[2] = color.green;
              event.xclient.data.s[3] = color.blue;
              event.xclient.data.s[4] = 0;
            }
          break;

        case PROVIDER_PROP_TYPE_SET_BACKGROUND_IMAGE:
          panel_plugin_external_46_set_background_image (external46, g_value_get_string (&property->value));

          /* ask to restart the child */
          panel_plugin_external_restart (external);

          /* stop handling other properties */
          goto bailout;

        case PROVIDER_PROP_TYPE_ACTION_REMOVED:
        case PROVIDER_PROP_TYPE_ACTION_SAVE:
        case PROVIDER_PROP_TYPE_ACTION_QUIT:
        case PROVIDER_PROP_TYPE_ACTION_QUIT_FOR_RESTART:
        case PROVIDER_PROP_TYPE_ACTION_BACKGROUND_UNSET:
        case PROVIDER_PROP_TYPE_ACTION_SHOW_CONFIGURE:
        case PROVIDER_PROP_TYPE_ACTION_SHOW_ABOUT:
          event.xclient.data.s[1] = 0;
          break;

        default:
          g_critical ("Received unknown plugin property %u for %s-%d",
                      property->type, panel_module_get_name (external->module),
                      external->unique_id);
          goto bailout;
        }

      window = gtk_socket_get_plug_window (GTK_SOCKET (external));
      panel_return_if_fail (GDK_IS_WINDOW (window));
      XSendEvent (gdk_x11_display_get_xdisplay (gdk_window_get_display (window)),
                  gdk_x11_window_get_xid (window),
                  False,
                  NoEventMask,
                  &event);
    }

  bailout:

  gdk_flush ();

  if (gdk_error_trap_pop () != 0)
    {
      g_critical ("Failed to send client messages for %s-%d",
                  panel_module_get_name (external->module),
                  external->unique_id);
    }
}



static gboolean
panel_plugin_external_46_remote_event (PanelPluginExternal *external,
                                       const gchar         *name,
                                       const GValue        *value,
                                       guint               *handle)
{
  panel_return_val_if_fail (PANEL_IS_PLUGIN_EXTERNAL_46 (external), TRUE);
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (external), TRUE);

  g_warning ("Plugin %s is compiled as an Xfce 4.6 binary. It needs to be "
             "ported to the new library plugin framework to be able to use "
             "remote events.", panel_module_get_name (external->module));

  return TRUE;
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
panel_plugin_external_46_set_background_image (PanelPluginExternal46 *external,
                                               const gchar           *image)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  g_free (external->background_image);
  external->background_image = g_strdup (image);
}
