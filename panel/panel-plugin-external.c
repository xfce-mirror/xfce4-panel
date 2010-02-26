/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#include <exo/exo.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-module.h>
#include <panel/panel-plugin-external.h>
#include <panel/panel-window.h>
#include <panel/panel-glue.h>
#include <panel/panel-dbus-service.h>



static void         panel_plugin_external_class_init          (PanelPluginExternalClass     *klass);
static void         panel_plugin_external_init                (PanelPluginExternal          *external);
static void         panel_plugin_external_provider_init       (XfcePanelPluginProviderIface *iface);
static void         panel_plugin_external_finalize            (GObject                      *object);
static void         panel_plugin_external_realize             (GtkWidget                    *widget);
static void         panel_plugin_external_unrealize           (GtkWidget                    *widget);
static gboolean     panel_plugin_external_plug_removed        (GtkSocket                    *socket);
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

  /* process pid */
  GPid              pid;
};



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

  gtksocket_class = GTK_SOCKET_CLASS (klass);
  gtksocket_class->plug_removed = panel_plugin_external_plug_removed;

  /* initialize the global message atom */
  message_atom = gdk_atom_intern_static_string ("XFCE_PANEL_PLUGIN");
}



static void
panel_plugin_external_init (PanelPluginExternal *external)
{
  /* initialize */
  external->id = NULL;
  external->module = NULL;
  external->arguments = NULL;
  external->pid = 0;

  /* signal to pass gtk_widget_set_sensitive() changes to the remote window */
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
  gchar                *socket_id;
  gint                  i, argc = 12;
  GdkScreen            *screen;

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
  
  /* get the widget screen */
  screen = gtk_widget_get_screen (widget);

  /* spawn the proccess */
  succeed = gdk_spawn_on_screen (screen, NULL, argv, NULL, 0, NULL, NULL, &external->pid, &error);

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
  //PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (widget);

  /* send message to quit the wrapper */
  /* TODO quit */

  return (*GTK_WIDGET_CLASS (panel_plugin_external_parent_class)->unrealize) (widget);
}



static gboolean
panel_plugin_external_plug_removed (GtkSocket *socket)
{
  PanelPluginExternal *external = PANEL_PLUGIN_EXTERNAL (socket);
  GtkWidget           *dialog;
  gint                 response;
  gchar               *filename, *path;
  PanelWindow         *window;

  /* create dialog */
  dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (socket))),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   _("Plugin '%s' unexpectedly left the building, do you want to restart it?"),
                                   panel_module_get_name (external->module));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("If you press Execute "
                                            "the panel will try to restart the plugin otherwise it "
                                            "will be permanently removed from the panel."));
  gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK,
                          GTK_STOCK_REMOVE, GTK_RESPONSE_CLOSE, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  /* reset the pid */
  external->pid = 0;

  /* unrealize and hide the socket */
  gtk_widget_unrealize (GTK_WIDGET (socket));
  gtk_widget_hide (GTK_WIDGET (socket));

  /* run the dialog */
  response = gtk_dialog_run (GTK_DIALOG (dialog));

  /* destroy the dialog */
  gtk_widget_destroy (dialog);

  /* handle the response */
  if (response == GTK_RESPONSE_OK)
    {
      /* get the plugin panel */
      window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (socket)));

      /* debug check */
      panel_return_val_if_fail (PANEL_IS_WINDOW (window), TRUE);

      /* send panel information to the plugin */
      panel_glue_set_provider_info (XFCE_PANEL_PLUGIN_PROVIDER (external));

      /* show the socket again (realize will spawn the plugin) */
      gtk_widget_show (GTK_WIDGET (socket));

      /* don't process other events */
      return TRUE;
    }
  else
    {
      /* build the plugin rc filename (from xfce_panel_plugin_relative_filename() in libxfce4panel) */
      filename = g_strdup_printf ("xfce4" G_DIR_SEPARATOR_S "panel" G_DIR_SEPARATOR_S "%s-%s.rc",
                                  panel_module_get_internal_name (external->module), external->id);

      /* get the path */
      path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, filename);

      /* remove the config file */
      if (G_LIKELY (path))
        g_unlink (path);

      /* cleanup */
      g_free (filename);
      g_free (path);

      /* destroy the socket */
      gtk_widget_destroy (GTK_WIDGET (socket));
    }

  /* process other events */
  if (GTK_SOCKET_CLASS (panel_plugin_external_parent_class)->plug_removed)
    return (*GTK_SOCKET_CLASS (panel_plugin_external_parent_class)->plug_removed) (socket);
  return FALSE;
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
  GValue value = { 0, };
  
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  
  /* set the size */
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, size);
  
  /* send the value */
  panel_dbus_service_set_plugin_property (PANEL_PLUGIN_EXTERNAL (provider)->id, "Size", &value);
  
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

  /* set the orientation */
  g_value_init (&value, G_TYPE_UINT);
  g_value_set_uint (&value, orientation);

  /* send the value */
  panel_dbus_service_set_plugin_property (PANEL_PLUGIN_EXTERNAL (provider)->id, "Orientation", &value);

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
  panel_dbus_service_set_plugin_property (PANEL_PLUGIN_EXTERNAL (provider)->id, "ScreenPosition", &value);

  /* unset */
  g_value_unset (&value);
}



static void
panel_plugin_external_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (provider));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* send signal to wrapper */
  panel_dbus_service_set_plugin_property (PANEL_PLUGIN_EXTERNAL (provider)->id, "Save", NULL);
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
  panel_dbus_service_set_plugin_property (external->id, "Sensitive", &value);
  
  /* unset */
  g_value_unset (&value);
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
  GValue value = { 0, };
  
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* set the boolean */
  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, percentage);

  /* send message */
  panel_dbus_service_set_plugin_property (external->id, "BackgroundAlpha", &value);
  
  /* unset */
  g_value_unset (&value);
}



void
panel_plugin_external_set_active_panel (PanelPluginExternal *external,
                                        gboolean             active)
{
  GValue value = { 0, };
  
  panel_return_if_fail (PANEL_IS_PLUGIN_EXTERNAL (external));

  /* set the boolean */
  g_value_init (&value, G_TYPE_BOOLEAN);
  g_value_set_boolean (&value, active);

  /* send message */
  panel_dbus_service_set_plugin_property (external->id, "ActivePanel", &value);
  
  /* unset */
  g_value_unset (&value);
}
