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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <exo/exo.h>
#include <glib/gstdio.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-window.h>
#include <panel/panel-application.h>
#include <panel/panel-itembar.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-glue.h>
#include <panel/panel-plugin-external.h>

#define AUTOSAVE_INTERVAL (10 * 60)



static void      panel_application_class_init         (PanelApplicationClass  *klass);
static void      panel_application_init               (PanelApplication       *application);
static void      panel_application_finalize           (GObject                *object);
static void      panel_application_load               (PanelApplication       *application);
static void      panel_application_plugin_move        (GtkWidget              *item,
                                                       PanelApplication       *application);
static gboolean  panel_application_plugin_insert      (PanelApplication       *application,
                                                       PanelWindow            *window,
                                                       GdkScreen              *screen,
                                                       const gchar            *name,
                                                       gint                    unique_id,
                                                       gchar                 **arguments,
                                                       gint                    position);
static gboolean  panel_application_save_timeout       (gpointer                user_data);
static void      panel_application_save_reschedule    (PanelApplication       *application);
static void      panel_application_window_destroyed   (GtkWidget              *window,
                                                       PanelApplication       *application);
static void      panel_application_dialog_destroyed   (GtkWindow              *dialog,
                                                       PanelApplication       *application);
static void      panel_application_drag_data_received (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       GtkSelectionData       *selection_data,
                                                       guint                   info,
                                                       guint                   time,
                                                       PanelWindow            *window);
static gboolean  panel_application_drag_drop          (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       guint                   time,
                                                       PanelWindow            *window);



struct _PanelApplicationClass
{
  GObjectClass __parent__;
};

struct _PanelApplication
{
  GObject  __parent__;

  /* the plugin factory */
  PanelModuleFactory *factory;

  /* xfconf channel */
  XfconfChannel      *xfconf;

  /* internal list of all the panel windows */
  GSList  *windows;

  /* internal list of opened dialogs */
  GSList  *dialogs;

  /* autosave timeout */
  guint    autosave_timeout_id;
};

static const GtkTargetEntry drag_targets[] =
{
    { "application/x-xfce-panel-plugin-widget", 0, 0 }
};



G_DEFINE_TYPE (PanelApplication, panel_application, G_TYPE_OBJECT);



static void
panel_application_class_init (PanelApplicationClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_application_finalize;
}



static void
panel_application_init (PanelApplication *application)
{
  /* initialize */
  application->windows = NULL;
  application->dialogs = NULL;
  application->autosave_timeout_id = 0;

  /* get a factory reference so it never unloads */
  application->factory = panel_module_factory_get ();

  /* get the xfconf channel */
  application->xfconf = xfconf_channel_new ("xfce4-panel");

  /* load setup */
  panel_application_load (application);

  /* start the autosave timeout */
  panel_application_save_reschedule (application);
}



static void
panel_application_finalize (GObject *object)
{
  PanelApplication *application = PANEL_APPLICATION (object);
  GSList           *li;

  panel_return_if_fail (application->dialogs == NULL);

  /* stop the autosave timeout */
  g_source_remove (application->autosave_timeout_id);

  /* destroy the windows if they are still opened */
  for (li = application->windows; li != NULL; li = li->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (li->data), G_CALLBACK (panel_application_window_destroyed), application);
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }

  /* cleanup the list of windows */
  g_slist_free (application->windows);

  /* release the xfconf channel */
  g_object_unref (G_OBJECT (application->xfconf));

  /* release the factory */
  g_object_unref (G_OBJECT (application->factory));

  (*G_OBJECT_CLASS (panel_application_parent_class)->finalize) (object);
}



static void
panel_application_xfconf_window_bindings (PanelApplication *application,
                                          PanelWindow      *window,
                                          gboolean          store_settings)
{
  XfconfChannel *channel = application->xfconf;
  gchar          buf[100];
  guint          i;
  guint          index = g_slist_index (application->windows, window);
  GValue         value = { 0, };
  const gchar   *bool_properties[] = { "locked", "autohide", "span-monitors", "horizontal" };
  const gchar   *uint_properties[] = { "size", "length", "x-offset", "y-offset",
                                       "enter-opacity", "leave-opacity", "snap-edge",
                                       "background-alpha" };

  /* connect the boolean properties */
  for (i = 0; i < G_N_ELEMENTS (bool_properties); i++)
    {
      /* create xfconf property name */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/%s", index, bool_properties[i]);

      /* store the window settings in the channel before we create the binding,
       * so we don't loose the panel settings */
      if (store_settings)
        {
          g_value_init(&value, G_TYPE_BOOLEAN);
          g_object_get_property (G_OBJECT (window), bool_properties[i], &value);
          xfconf_channel_set_property (channel, buf, &value);
          g_value_unset (&value);
        }

      /* create binding */
      xfconf_g_property_bind (channel, buf, G_TYPE_BOOLEAN, window, bool_properties[i]);
    }

  /* connect the unsigned intergets */
  for (i = 0; i < G_N_ELEMENTS (uint_properties); i++)
    {
      /* create xfconf property name */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/%s", index, uint_properties[i]);

      /* store the window settings in the channel before we create the binding,
       * so we don't loose the panel settings */
      if (store_settings)
        {
          g_value_init(&value, G_TYPE_UINT);
          g_object_get_property (G_OBJECT (window), uint_properties[i], &value);
          xfconf_channel_set_property (channel, buf, &value);
          g_value_unset (&value);
        }

      /* create binding */
      xfconf_g_property_bind (channel, buf, G_TYPE_UINT, window, uint_properties[i]);
    }
}



static void
panel_application_load (PanelApplication *application)
{
  XfconfChannel *channel = application->xfconf;
  PanelWindow   *window;
  guint          i, n_panels;
  guint          j, n_plugins;
  gchar          buf[100];
  gchar         *name;
  gint           unique_id;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCONF_IS_CHANNEL (application->xfconf));

  /* walk all the panel in the configuration */
  n_panels = xfconf_channel_get_uint (channel, "/panels", 0);
  for (i = 0; i < n_panels; i++)
    {
      /* create a new window */
      window = panel_application_new_window (application, NULL);

      /* walk all the plugins on the panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins", i);
      n_plugins = xfconf_channel_get_uint (channel, buf, 0);
      for (j = 0; j < n_plugins; j++)
        {
          /* get the plugin module name */
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/module", i, j);
          name = xfconf_channel_get_string (channel, buf, NULL);
          if (G_LIKELY (name))
            {
              /* read the plugin id */
              g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/id", i, j);
              unique_id = xfconf_channel_get_int (channel, buf, -1);

              panel_application_plugin_insert (application, window,
                                               gtk_window_get_screen (GTK_WINDOW (window)),
                                               name, unique_id, NULL, -1);

              /* cleanup */
              g_free (name);
            }
        }

      /* show the window */
      gtk_widget_show (GTK_WIDGET (window));
    }
}



static void
panel_application_plugin_move_end (GtkWidget        *item,
                                   GdkDragContext   *context,
                                   PanelApplication *application)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* disconnect this signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (item), G_CALLBACK (panel_application_plugin_move_end), application);

  /* make the window insensitive */
  panel_application_windows_sensitive (application, TRUE);
}



static void
panel_application_plugin_move (GtkWidget        *item,
                               PanelApplication *application)
{
  GtkTargetList  *target_list;
  const gchar    *icon_name;
  GdkDragContext *context;
  PanelModule    *module;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* make the window insensitive */
  panel_application_windows_sensitive (application, FALSE);

  /* create a target list */
  target_list = gtk_target_list_new (drag_targets, G_N_ELEMENTS (drag_targets));

  /* begin a drag */
  context = gtk_drag_begin (item, target_list, GDK_ACTION_MOVE, 1, NULL);

  /* set the drag context icon name */
  module = panel_module_get_from_plugin_provider (XFCE_PANEL_PLUGIN_PROVIDER (item));
  icon_name = panel_module_get_icon_name (module);
  gtk_drag_set_icon_name (context, icon_name ? icon_name : GTK_STOCK_DND, 0, 0);

  /* release the drag list */
  gtk_target_list_unref (target_list);

  /* signal to make the window sensitive again on a drag end */
  g_signal_connect (G_OBJECT (item), "drag-end", G_CALLBACK (panel_application_plugin_move_end), application);
}



static void
panel_application_plugin_provider_signal (XfcePanelPluginProvider       *provider,
                                          XfcePanelPluginProviderSignal  signal,
                                          PanelApplication              *application)
{
  GtkWidget   *itembar;
  PanelWindow *window;
  gchar       *property;
  gchar       *path, *filename;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* get the panel of the plugin */
  window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider)));

  /* handle the signal emitted from the plugin provider */
  switch (signal)
    {
      case PROVIDER_SIGNAL_MOVE_PLUGIN:
        /* invoke the move function */
        panel_application_plugin_move (GTK_WIDGET (provider), application);
        break;

      case PROVIDER_SIGNAL_EXPAND_PLUGIN:
      case PROVIDER_SIGNAL_COLLAPSE_PLUGIN:
        /* get the itembar */
        itembar = gtk_bin_get_child (GTK_BIN (window));

        /* set new expand mode */
        panel_itembar_set_child_expand (PANEL_ITEMBAR (itembar), GTK_WIDGET (provider), !!(signal == PROVIDER_SIGNAL_EXPAND_PLUGIN));
        break;

      case PROVIDER_SIGNAL_LOCK_PANEL:
        /* block autohide */
        panel_window_freeze_autohide (window);
        break;

      case PROVIDER_SIGNAL_UNLOCK_PANEL:
        /* unblock autohide */
        panel_window_thaw_autohide (window);
        break;

      case PROVIDER_SIGNAL_REMOVE_PLUGIN:
        /* create the xfconf property base */
        property = g_strdup_printf (PANEL_PLUGIN_PROPERTY_BASE,
                                    xfce_panel_plugin_provider_get_unique_id (provider));

        /* build the plugin rc filename */
        filename = g_strdup_printf (PANEL_PLUGIN_RELATIVE_PATH,
                                    xfce_panel_plugin_provider_get_name (provider),
                                    xfce_panel_plugin_provider_get_unique_id (provider));

        /* destroy the plugin if it's a panel plugin (ie. not external) */
        if (XFCE_IS_PANEL_PLUGIN (provider))
          gtk_widget_destroy (GTK_WIDGET (provider));

        /* remove the xfconf properties */
        xfconf_channel_reset_property (application->xfconf, property, TRUE);
        g_free (property);

        /* get the path of the config file */
        path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, filename);
        g_free (filename);

        /* remove the config file */
        if (G_LIKELY (path))
          g_unlink (path);
        g_free (path);
        break;

      case PROVIDER_SIGNAL_ADD_NEW_ITEMS:
        /* open the items dialog */
        panel_item_dialog_show (window);
        break;

      case PROVIDER_SIGNAL_PANEL_PREFERENCES:
        /* open the panel preferences */
        panel_preferences_dialog_show (window);
        break;

      default:
        g_critical ("Reveived unknown signal %d", signal);
        break;
    }
}



static gboolean
panel_application_plugin_insert (PanelApplication  *application,
                                 PanelWindow       *window,
                                 GdkScreen         *screen,
                                 const gchar       *name,
                                 gint               unique_id,
                                 gchar            **arguments,
                                 gint               position)
{
  GtkWidget               *itembar;
  gboolean                 succeed = FALSE;
  XfcePanelPluginProvider *provider;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (name != NULL, FALSE);

  /* create a new panel plugin */
  provider = panel_module_factory_create_plugin (application->factory, screen, name, unique_id, arguments);

  if (G_LIKELY (provider != NULL))
    {
      /* get the panel itembar */
      itembar = gtk_bin_get_child (GTK_BIN (window));

      /* add signal to monitor provider signals */
      g_signal_connect (G_OBJECT (provider), "provider-signal", G_CALLBACK (panel_application_plugin_provider_signal), application);

      /* add the item to the panel */
      panel_itembar_insert (PANEL_ITEMBAR (itembar), GTK_WIDGET (provider), position);

      /* send all the needed info about the panel to the plugin */
      panel_glue_set_provider_info (provider, window);

      /* show the plugin */
      gtk_widget_show (GTK_WIDGET (provider));

      /* we've succeeded */
      succeed = TRUE;
    }

  return succeed;
}



static gboolean
panel_application_save_timeout (gpointer user_data)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (user_data), FALSE);

  GDK_THREADS_ENTER ();

  /* save */
  panel_application_save (PANEL_APPLICATION (user_data), TRUE);

  GDK_THREADS_LEAVE ();

  return TRUE;
}



static void
panel_application_save_reschedule (PanelApplication *application)
{
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* stop a running timeout */
  if (G_UNLIKELY (application->autosave_timeout_id != 0))
    g_source_remove (application->autosave_timeout_id);

  /* start a new timeout */
  application->autosave_timeout_id = g_timeout_add_seconds (AUTOSAVE_INTERVAL,
                                                            panel_application_save_timeout,
                                                            application);
}



static void
panel_application_window_destroyed (GtkWidget        *window,
                                    PanelApplication *application)
{
  guint     n;
  gchar     buf[100];
  GSList   *li, *lnext;
  gboolean  passed_destroyed_window = FALSE;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (g_slist_find (application->windows, window) != NULL);

  /* we need to update the bindings of all the panels... */
  for (li = application->windows, n = 0; li != NULL; li = lnext, n++)
    {
      lnext = li->next;

      /* skip window if we're not passed the active window */
      if (!passed_destroyed_window && li->data != window)
        continue;

      /* keep updating from now on */
      passed_destroyed_window = TRUE;

      /* disconnect bindings from this panel */
      xfconf_g_property_unbind_all (G_OBJECT (li->data));

      /* remove the properties of this panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u", n);
      xfconf_channel_reset_property (application->xfconf, buf, TRUE);

      /* either remove the window or add the bindings */
      if (li->data == window)
        application->windows = g_slist_delete_link (application->windows, li);
      else
        panel_application_xfconf_window_bindings (application, PANEL_WINDOW (li->data), TRUE);
    }

  /* force a panel save to store plugins and panel count */
  panel_application_save (application, FALSE);

  /* quit if there are no windows opened */
  if (application->windows == NULL)
    gtk_main_quit ();
}



static void
panel_application_dialog_destroyed (GtkWindow        *dialog,
                                    PanelApplication *application)
{
  panel_return_if_fail (GTK_IS_WINDOW (dialog));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (g_slist_find (application->dialogs, dialog) != NULL);

  /* remove the window from the list */
  application->dialogs = g_slist_remove (application->dialogs, dialog);
}



static void
panel_application_drag_data_received (GtkWidget        *itembar,
                                      GdkDragContext   *context,
                                      gint              x,
                                      gint              y,
                                      GtkSelectionData *selection_data,
                                      guint             info,
                                      guint             time,
                                      PanelWindow      *window)
{
  guint             position;
  PanelApplication *application;
  GtkWidget        *provider;
  gboolean          succeed = FALSE;
  GdkScreen        *screen;
  const gchar      *name;
  guint             old_position;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the application */
  application = panel_application_get ();

  /* get the drop index on the itembar */
  position = panel_itembar_get_drop_index (PANEL_ITEMBAR (itembar), x, y);

  /* get the widget screen */
  screen = gtk_widget_get_screen (itembar);

  switch (info)
    {
      case PANEL_ITEMBAR_TARGET_PLUGIN_NAME:
        if (G_LIKELY (selection_data->length > 0))
          {
            /* get the name from the selection data */
            name = (const gchar *) selection_data->data;

            /* create a new item with a unique id */
            succeed = panel_application_plugin_insert (application, window, screen, name,
                                                       -1, NULL, position);
          }
        break;

      case PANEL_ITEMBAR_TARGET_PLUGIN_WIDGET:
        /* get the source widget */
        provider = gtk_drag_get_source_widget (context);

        /* debug check */
        panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

        /* check if we move to another itembar */
        if (gtk_widget_get_parent (provider) == itembar)
          {
            /* get the current position on the itembar */
            old_position = panel_itembar_get_child_index (PANEL_ITEMBAR (itembar), provider);

            /* decrease the counter if we drop after the current position */
            if (position > old_position)
              position--;

            /* reorder the child if needed */
            if (old_position != position)
              panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, position);
          }
        else
          {
            /* reparent the widget, this will also call remove and add for the itembar */
            gtk_widget_reparent (provider, itembar);

            /* move the item to the correct position on the itembar */
            panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, position);

            /* send all the needed panel information to the plugin */
            panel_glue_set_provider_info (XFCE_PANEL_PLUGIN_PROVIDER (provider), window);
          }

        /* everything went fine */
        succeed = TRUE;
        break;

      default:
        panel_assert_not_reached ();
        break;
    }

  /* save the panel configuration if we succeeded */
  if (G_LIKELY (succeed))
    panel_application_save (application, FALSE);

  /* release the application */
  g_object_unref (G_OBJECT (application));

  /* tell the peer that we handled the drop */
  gtk_drag_finish (context, succeed, FALSE, time);
}



static gboolean
panel_application_drag_drop (GtkWidget      *itembar,
                             GdkDragContext *context,
                             gint            x,
                             gint            y,
                             guint           time,
                             PanelWindow    *window)
{
  GdkAtom target;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  target = gtk_drag_dest_find_target (itembar, context, NULL);

  /* we cannot handle the drag data */
  if (G_UNLIKELY (target == GDK_NONE))
    return FALSE;

  /* request the drag data */
  gtk_drag_get_data (itembar, context, target, time);

  /* we call gtk_drag_finish later */
  return TRUE;
}



PanelApplication *
panel_application_get (void)
{
  static PanelApplication *application = NULL;

  if (G_LIKELY (application))
    {
      g_object_ref (G_OBJECT (application));
    }
  else
    {
      application = g_object_new (PANEL_TYPE_APPLICATION, NULL);
      g_object_add_weak_pointer (G_OBJECT (application), (gpointer) &application);
    }

  return application;
}



void
panel_application_save (PanelApplication *application,
                        gboolean          save_plugin_providers)
{
  GSList                  *li;
  GList                   *children, *lp;
  GtkWidget               *itembar;
  XfcePanelPluginProvider *provider;
  guint                    i, j;
  gchar                    buf[100];
  XfconfChannel           *channel = application->xfconf;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));

  /* save the settings of all plugins */
  for (li = application->windows, i = 0; li != NULL; li = li->next, i++)
    {
      /* get the itembar children */
      itembar = gtk_bin_get_child (GTK_BIN (li->data));
      children = gtk_container_get_children (GTK_CONTAINER (itembar));

      /* walk all the plugin children */
      for (lp = children, j = 0; lp != NULL; lp = lp->next, j++)
        {
          provider = XFCE_PANEL_PLUGIN_PROVIDER (lp->data);

          /* save the plugin name */
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/module", i, j);
          xfconf_channel_set_string (channel, buf, xfce_panel_plugin_provider_get_name (provider));

          /* save the plugin id */
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/id", i, j);
          xfconf_channel_set_int (channel, buf, xfce_panel_plugin_provider_get_unique_id (provider));

          /* ask the plugin to save */
          if (save_plugin_providers)
            xfce_panel_plugin_provider_save (provider);
        }

      /* cleanup */
      g_list_free (children);

      /* store the number of plugins in this panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins", i);
      xfconf_channel_set_uint (channel, buf, j);
    }

  /* store the number of panels */
  xfconf_channel_set_uint (channel, "/panels", i);
}



void
panel_application_take_dialog (PanelApplication *application,
                               GtkWindow        *dialog)
{
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (GTK_IS_WINDOW (dialog));

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (panel_application_dialog_destroyed), application);

  /* add the window to internal list */
  application->dialogs = g_slist_prepend (application->dialogs, dialog);
}



void
panel_application_destroy_dialogs (PanelApplication *application)
{
  GSList *li, *lnext;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* destroy all dialogs */
  for (li = application->dialogs; li != NULL; li = lnext)
    {
      /* get next element */
      lnext = li->next;

      /* destroy the window */
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }

  /* check */
  panel_return_if_fail (application->dialogs == NULL);
}



void
panel_application_add_new_item (PanelApplication  *application,
                                const gchar       *plugin_name,
                                gchar            **arguments)
{
  PanelWindow *window;
  gint         nth = 0;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (plugin_name != NULL);
  panel_return_if_fail (g_slist_length (application->windows) > 0);

  if (panel_module_factory_has_module (application->factory, plugin_name))
    {
      /* ask the user what panel to use if there is more then one */
      if (g_slist_length (application->windows) > 1)
        if ((nth = panel_dialogs_choose_panel (application)) == -1)
          return;

      /* get the window */
      window = g_slist_nth_data (application->windows, nth);

      /* add the plugin to the end of the choosen window */
      panel_application_plugin_insert (application, window, gtk_widget_get_screen (GTK_WIDGET (window)),
                                       plugin_name, -1, arguments, -1);
    }
  else
    {
      /* print warning */
      g_warning (_("The plugin (%s) you want to add is not recognized by the panel."), plugin_name);
    }
}



PanelWindow *
panel_application_new_window (PanelApplication *application,
                              GdkScreen        *screen)
{
  GtkWidget *window;
  GtkWidget *itembar;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), NULL);

  /* create panel window */
  window = g_object_new (PANEL_TYPE_WINDOW, NULL);

  /* realize */
  gtk_widget_realize (window);

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (panel_application_window_destroyed), application);

  /* put on the correct screen */
  gtk_window_set_screen (GTK_WINDOW (window), screen ? screen : gdk_screen_get_default ());

  /* add the window to internal list */
  application->windows = g_slist_append (application->windows, window);

  /* add the itembar */
  itembar = panel_itembar_new ();
  exo_binding_new (G_OBJECT (window), "horizontal", G_OBJECT (itembar), "horizontal");
  gtk_container_add (GTK_CONTAINER (window), itembar);
  gtk_widget_show (itembar);

  /* signals for drag and drop */
  g_signal_connect (G_OBJECT (itembar), "drag-data-received", G_CALLBACK (panel_application_drag_data_received), window);
  g_signal_connect (G_OBJECT (itembar), "drag-drop", G_CALLBACK (panel_application_drag_drop), window);

  /* add the xfconf bindings */
  panel_application_xfconf_window_bindings (application, PANEL_WINDOW (window), FALSE);

  return PANEL_WINDOW (window);
}



GSList *
panel_application_get_windows (PanelApplication *application)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);

  return application->windows;
}



guint
panel_application_get_n_windows (PanelApplication *application)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);

  return g_slist_length (application->windows);
}



gint
panel_application_get_window_index (PanelApplication *application,
                                    PanelWindow      *window)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), 0);

  return g_slist_index (application->windows, window);
}



PanelWindow *
panel_application_get_window (PanelApplication *application,
                              guint             idx)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);

  return g_slist_nth_data (application->windows, idx);
}



void
panel_application_window_select (PanelApplication *application,
                                 PanelWindow      *window)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (window == NULL || PANEL_IS_WINDOW (window));

  /* update state for all windows */
  for (li = application->windows; li != NULL; li = li->next)
    panel_window_set_active_panel (PANEL_WINDOW (li->data), !!(li->data == window));
}



void
panel_application_windows_sensitive (PanelApplication *application,
                                     gboolean          sensitive)
{
  GtkWidget *itembar;
  GSList    *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* walk the windows */
  for (li = application->windows; li != NULL; li = li->next)
    {
      /* get the window itembar */
      itembar = gtk_bin_get_child (GTK_BIN (li->data));

      /* set sensitivity of the itembar (and the plugins) */
      panel_itembar_set_sensitive (PANEL_ITEMBAR (itembar), sensitive);

      /* block autohide for all windows */
      if (sensitive)
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
    }
}



void
panel_application_windows_autohide (PanelApplication *application,
                                    gboolean          freeze)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  for (li = application->windows; li != NULL; li = li->next)
    {
      if (freeze)
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
    }
}
