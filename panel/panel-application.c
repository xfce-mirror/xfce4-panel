/* $Id$ */
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <exo/exo.h>
#include <glib/gstdio.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>

#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-dbus-service.h>
#include <panel/panel-window.h>
#include <panel/panel-application.h>
#include <panel/panel-itembar.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-plugin-external.h>

#define AUTOSAVE_INTERVAL (10 * 60)



static void      panel_application_finalize           (GObject                *object);
static void      panel_application_load               (PanelApplication       *application,
                                                       GHashTable             *hash_table);
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
                                                       guint                   drag_time,
                                                       PanelWindow            *window);
static gboolean  panel_application_drag_motion        (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       guint                   drag_time,
                                                       PanelApplication       *application);
static gboolean  panel_application_drag_drop          (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       guint                   drag_time,
                                                       PanelApplication       *application);
static void      panel_application_drag_leave         (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       guint                   drag_time,
                                                       PanelApplication       *application);



enum
{
  PROP_0,
  PROP_ITEMS_CHANGED
};

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

  /* drag and drop data */
  guint     drop_data_ready : 1;
  guint     drop_occurred : 1;
  gchar   **drop_uris;
};

enum
{
  TARGET_PLUGIN_NAME,
  TARGET_PLUGIN_WIDGET,
  TARGET_TEXT_URI_LIST
};

static const GtkTargetEntry drag_targets[] =
{
  { (gchar *) "xfce-panel/plugin-widget",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET }
};

static const GtkTargetEntry drop_targets[] =
{
  { (gchar *) "xfce-panel/plugin-name",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_NAME },
  { (gchar *) "xfce-panel/plugin-widget",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET },
  { (gchar *) "text/uri-list",
    GTK_TARGET_OTHER_APP, TARGET_TEXT_URI_LIST }
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
  PanelWindow  *window;
  GHashTable   *hash_table;
  const GValue *value;

  /* initialize */
  application->windows = NULL;
  application->dialogs = NULL;
  application->autosave_timeout_id = 0;
  application->drop_uris = NULL;
  application->drop_data_ready = FALSE;
  application->drop_occurred = FALSE;

  /* get the xfconf channel */
  application->xfconf = panel_properties_get_channel ();

  /* get all the panel properties */
  hash_table = xfconf_channel_get_properties (application->xfconf, NULL);

  if (G_LIKELY (hash_table != NULL))
    {
      /* check if we need to force all plugins to run external */
      value = g_hash_table_lookup (hash_table, "/force-all-external");
      if (value != NULL && g_value_get_boolean (value))
        panel_module_factory_force_all_external ();

      /* set the shared hash table */
      panel_properties_shared_hash_table (hash_table);
    }

  /* get a factory reference so it never unloads */
  application->factory = panel_module_factory_get ();

  /* load setup */
  if (G_LIKELY (hash_table != NULL))
    panel_application_load (application, hash_table);

  /* start the autosave timeout */
  panel_application_save_reschedule (application);

  /* create empty window */
  if (G_UNLIKELY (application->windows == NULL))
    window = panel_application_new_window (application, NULL, TRUE);

  if (G_LIKELY (hash_table != NULL))
    {
      /* unset the shared hash table */
      panel_properties_shared_hash_table (NULL);

      /* cleanup */
      g_hash_table_destroy (hash_table);
    }
}



static void
panel_application_finalize (GObject *object)
{
  PanelApplication *application = PANEL_APPLICATION (object);
  GSList           *li;

  panel_return_if_fail (application->dialogs == NULL);
  panel_return_if_fail (application->drop_uris == NULL);

  /* stop the autosave timeout */
  g_source_remove (application->autosave_timeout_id);

  /* destroy the windows if they are still opened */
  for (li = application->windows; li != NULL; li = li->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (li->data),
          G_CALLBACK (panel_application_window_destroyed), application);
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
                                          gboolean          save_properties)
{
  gchar               *property_base;
  const PanelProperty  properties[] =
  {
    { "locked", G_TYPE_BOOLEAN },
    { "autohide", G_TYPE_BOOLEAN },
    { "span-monitors", G_TYPE_BOOLEAN },
    { "horizontal", G_TYPE_BOOLEAN },
    { "size", G_TYPE_UINT },
    { "length", G_TYPE_UINT },
    { "enter-opacity", G_TYPE_UINT },
    { "leave-opacity", G_TYPE_UINT },
    { "background-alpha", G_TYPE_UINT },
    { "output-name", G_TYPE_STRING },
    { "position", G_TYPE_STRING },
    { NULL, G_TYPE_NONE }
  };

  panel_return_if_fail (XFCONF_IS_CHANNEL (application->xfconf));
  panel_return_if_fail (g_slist_index (application->windows, window) > -1);

  /* create the property base */
  property_base = g_strdup_printf ("/panels/panel-%d",
      g_slist_index (application->windows, window));

  /* bind all the properties */
  panel_properties_bind (application->xfconf, G_OBJECT (window),
                         property_base, properties, save_properties);

  /* cleanup */
  g_free (property_base);
}



static void
panel_application_load (PanelApplication *application,
                        GHashTable       *hash_table)
{
  const GValue *value;
  PanelWindow  *window;
  guint         i, n_panels;
  guint         j, n_plugins;
  gchar         buf[100];
  const gchar  *name;
  gint          unique_id;
  GdkScreen    *screen;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCONF_IS_CHANNEL (application->xfconf));
  panel_return_if_fail (hash_table != NULL);

  /* walk all the panel in the configuration */
  value = g_hash_table_lookup (hash_table, "/panels");
  n_panels = value != NULL ? g_value_get_uint (value) : 0;
  for (i = 0; i < n_panels; i++)
    {
      /* create a new window */
      window = panel_application_new_window (application, NULL, FALSE);

      /* walk all the plugins on the panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins", i);
      value = g_hash_table_lookup (hash_table, buf);
      n_plugins = value != NULL ? g_value_get_uint (value) : 0;
      for (j = 0; j < n_plugins; j++)
        {
          /* get the plugin module name */
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/module", i, j);
          value = g_hash_table_lookup (hash_table, buf);
          name = value != NULL ? g_value_get_string (value) : NULL;
          if (G_UNLIKELY (name == NULL))
            continue;

          /* read the plugin id */
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u/id", i, j);
          value = g_hash_table_lookup (hash_table, buf);
          unique_id = value != NULL ? g_value_get_int (value) : -1;

          screen = gtk_window_get_screen (GTK_WINDOW (window));
          if (!panel_application_plugin_insert (application, window, screen,
                                                name, unique_id, NULL, -1))
            {
              /* plugin could not be loaded, remove it from the channel */
              g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugins/plugin-%u", i, j);
              xfconf_channel_reset_property (application->xfconf, buf, TRUE);

              g_snprintf (buf, sizeof (buf), "/panels/plugin-%d", unique_id);
              xfconf_channel_reset_property (application->xfconf, buf, TRUE);

              /* show warnings */
              g_critical (_("Plugin \"%s-%d\" was not found and has been "
                          "removed from the configuration"), name, unique_id);
            }
        }
    }
}



static void
panel_application_plugin_move_drag_end (GtkWidget        *item,
                                        GdkDragContext   *context,
                                        PanelApplication *application)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* disconnect this signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (item),
      G_CALLBACK (panel_application_plugin_move_drag_end), application);

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
  g_signal_connect (G_OBJECT (item), "drag-end",
                    G_CALLBACK (panel_application_plugin_move_drag_end), application);
}



static void
panel_application_plugin_delete_config (PanelApplication *application,
                                        const gchar      *name,
                                        gint              unique_id)
{
  gchar *property;
  gchar *filename, *path;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (IS_STRING (name));
  panel_return_if_fail (unique_id != -1);

  /* remove the xfconf property */
  property = g_strdup_printf (PANEL_PLUGIN_PROPERTY_BASE, unique_id);
  xfconf_channel_reset_property (application->xfconf, property, TRUE);
  g_free (property);

  /* lookup the rc file */
  filename = g_strdup_printf (PANEL_PLUGIN_RC_RELATIVE_PATH, name, unique_id);
  path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, filename);
  g_free (filename);

  /* unlink the rc file */
  if (G_LIKELY (path != NULL))
    g_unlink (path);
  g_free (path);
}



static void
panel_application_plugin_provider_signal (XfcePanelPluginProvider       *provider,
                                          XfcePanelPluginProviderSignal  provider_signal,
                                          PanelApplication              *application)
{
  GtkWidget   *itembar;
  PanelWindow *window;
  gint         unique_id;
  gchar       *name;
  gboolean     expand;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  /* get the panel of the plugin */
  window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider)));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* handle the signal emitted from the plugin provider */
  switch (provider_signal)
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
        expand = !!(provider_signal == PROVIDER_SIGNAL_EXPAND_PLUGIN);
        panel_itembar_set_child_expand (PANEL_ITEMBAR (itembar),
                                        GTK_WIDGET (provider),
                                        expand);
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
        /* store the provider's unique id and name */
        unique_id = xfce_panel_plugin_provider_get_unique_id (provider);
        name = g_strdup (xfce_panel_plugin_provider_get_name (provider));

        /* destroy the plugin if it's a panel plugin (ie. not external) */
        if (XFCE_IS_PANEL_PLUGIN (provider))
          gtk_widget_destroy (GTK_WIDGET (provider));

        /* remove the plugin configuration */
        panel_application_plugin_delete_config (application, name, unique_id);
        g_free (name);
        break;

      case PROVIDER_SIGNAL_ADD_NEW_ITEMS:
        /* open the items dialog */
        panel_item_dialog_show (window);
        break;

      case PROVIDER_SIGNAL_PANEL_PREFERENCES:
        /* open the panel preferences */
        panel_preferences_dialog_show (window);
        break;

      case PROVIDER_SIGNAL_PANEL_QUIT:
      case PROVIDER_SIGNAL_PANEL_RESTART:
        /* set the restart boolean */
        dbus_quit_with_restart = !!(provider_signal == PROVIDER_SIGNAL_PANEL_RESTART);

        /* quit the main loop */
        gtk_main_quit ();
        break;

      case PROVIDER_SIGNAL_PANEL_ABOUT:
        /* show the panel about dialog */
        panel_dialogs_show_about ();
        break;

      case PROVIDER_SIGNAL_FOCUS_PLUGIN:
         /* focus the panel window */
         gtk_window_present_with_time (GTK_WINDOW (window), GDK_CURRENT_TIME);
         break;

      case PROVIDER_SIGNAL_SHOW_CONFIGURE:
      case PROVIDER_SIGNAL_SHOW_ABOUT:
        /* signals we can ignore, only for external plugins */
        break;

      default:
        g_critical ("Received unknown provider signal %d", provider_signal);
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
  GtkWidget *itembar, *provider;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (name != NULL, FALSE);

  /* create a new panel plugin */
  provider = panel_module_factory_new_plugin (application->factory,
                                              name, screen, unique_id,
                                              arguments);
  if (G_UNLIKELY (provider == NULL))
    return FALSE;

  /* make sure there is no panel configuration with this unique id when a
   * new plugin is created */
  if (G_UNLIKELY (unique_id == -1))
    {
      unique_id = xfce_panel_plugin_provider_get_unique_id (XFCE_PANEL_PLUGIN_PROVIDER (provider));
      panel_application_plugin_delete_config (application, name, unique_id);
    }

  /* get the panel itembar */
  itembar = gtk_bin_get_child (GTK_BIN (window));

  /* add signal to monitor provider signals */
  g_signal_connect (G_OBJECT (provider), "provider-signal",
                    G_CALLBACK (panel_application_plugin_provider_signal), application);

  /* add the item to the panel */
  panel_itembar_insert (PANEL_ITEMBAR (itembar),
                        GTK_WIDGET (provider), position);

  /* send all the needed info about the panel to the plugin */
  panel_window_set_povider_info (window, provider);

  /* show the plugin */
  gtk_widget_show (provider);

  return TRUE;
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
  application->autosave_timeout_id =
      g_timeout_add_seconds (AUTOSAVE_INTERVAL, panel_application_save_timeout,
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
      panel_properties_unbind (G_OBJECT (li->data));

      /* remove the properties of this panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u", n);
      xfconf_channel_reset_property (application->xfconf, buf, TRUE);

      /* either remove the window or add the bindings */
      if (li->data == window)
        application->windows = g_slist_delete_link (application->windows, li);
      else
        panel_application_xfconf_window_bindings (application,
                                                  PANEL_WINDOW (li->data),
                                                  TRUE);
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
                                      guint             drag_time,
                                      PanelWindow      *window)
{
  guint              position;
  PanelApplication  *application;
  GtkWidget         *provider;
  gboolean           succeed = FALSE;
  GdkScreen         *screen;
  const gchar       *name;
  guint              old_position;
  gchar            **uri_list, *tmp;
  GSList            *uris, *li;
  guint              i;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the application */
  application = panel_application_get ();

  if (!application->drop_data_ready)
    {
      panel_assert (application->drop_uris == NULL);

      if (info == TARGET_TEXT_URI_LIST)
        {
          /* extract the URI list from the selection data */
          uri_list = gtk_selection_data_get_uris (selection_data);
          if (G_LIKELY (uri_list != NULL))
            {
              /* add the valid desktop items to a list */
              for (i = 0, uris = NULL; uri_list[i] != NULL; i++)
                if (g_str_has_suffix (uri_list[i], ".desktop")
                    && (tmp = g_filename_from_uri (uri_list[i], NULL, NULL)) != NULL)
                  uris = g_slist_append (uris, tmp);

              /* cleanup */
              g_strfreev (uri_list);

              /* allocate a string for the valid uris */
              if (uris != NULL)
                {
                  /* allocate a string and add the files */
                  application->drop_uris = g_new0 (gchar *, g_slist_length (uris) + 1);
                  for (li = uris, i = 0; li != NULL; li = li->next, i++)
                    application->drop_uris[i] = li->data;
                  g_slist_free (uris);
                }
            }
        }

      /* reset the state */
      application->drop_data_ready = TRUE;
    }

  /* check if the data was droppped */
  if (!application->drop_occurred)
    {
      gdk_drag_status (context, 0, drag_time);
      goto bailout;
    }

  /* reset the state */
  application->drop_occurred = FALSE;

  /* get the drop index on the itembar */
  position = panel_itembar_get_drop_index (PANEL_ITEMBAR (itembar), x, y);

  /* get the widget screen */
  screen = gtk_widget_get_screen (itembar);

  switch (info)
    {
      case TARGET_PLUGIN_NAME:
        if (G_LIKELY (selection_data->length > 0))
          {
            /* get the name from the selection data */
            name = (const gchar *) selection_data->data;

            /* create a new item with a unique id */
            succeed = panel_application_plugin_insert (application, window,
                                                       screen, name,
                                                       -1, NULL, position);
          }
        break;

      case TARGET_PLUGIN_WIDGET:
        /* get the source widget */
        provider = gtk_drag_get_source_widget (context);

        /* debug check */
        panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

        /* check if we move to another itembar */
        if (gtk_widget_get_parent (provider) == itembar)
          {
            /* get the current position on the itembar */
            old_position = panel_itembar_get_child_index (PANEL_ITEMBAR (itembar),
                                                          provider);

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
            panel_window_set_povider_info (window, provider);
          }

        /* everything went fine */
        succeed = TRUE;
        break;

      case TARGET_TEXT_URI_LIST:
        /* get the uri list fo the selection data for the new launcher */
        if (G_LIKELY (application->drop_uris))
          {
            /* create a new item with a unique id */
            succeed = panel_application_plugin_insert (application, window,
                                                       screen, LAUNCHER_PLUGIN_NAME,
                                                       -1, application->drop_uris,
                                                       position);
          }
        break;

      default:
        panel_assert_not_reached ();
        break;
    }

  /* save the panel configuration if we succeeded */
  if (G_LIKELY (succeed))
    panel_application_save (application, FALSE);

  /* tell the peer that we handled the drop */
  gtk_drag_finish (context, succeed, FALSE, drag_time);

bailout:
  /* release the application */
  g_object_unref (G_OBJECT (application));
}



static gboolean
panel_application_drag_motion (GtkWidget        *itembar,
                               GdkDragContext   *context,
                               gint              x,
                               gint              y,
                               guint             drag_time,
                               PanelApplication *application)
{
  GdkAtom       target;
  GdkDragAction action;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (application->drop_occurred == FALSE, FALSE);

  /* determine the drag target */
  target = gtk_drag_dest_find_target (itembar, context, NULL);
  if (target == gdk_atom_intern_static_string ("text/uri-list"))
    {
      /* request the drop data on-demand (if we don't have it already) */
      if (!application->drop_data_ready)
        {
          /* request the drop data from the source */
          gtk_drag_get_data (itembar, context, target, drag_time);

          /* we cannot drop here (yet!) */
          return TRUE;
        }
      else if (application->drop_uris != NULL)
        {
          /* there are valid uris in the drop data */
          gdk_drag_status (context, GDK_ACTION_COPY, drag_time);
        }
      else
        {
          goto invalid_drop;
        }
    }
  else if (target != GDK_NONE)
    {
      if (target == gdk_atom_intern_static_string ("xfce-panel/plugin-name"))
        action = GDK_ACTION_COPY;
      else
        action = GDK_ACTION_MOVE;

      /* plugin-widget or -name */
      gdk_drag_status (context, action, drag_time);
    }
  else
    {
      invalid_drop:

      /* not a valid drop */
      gdk_drag_status (context, 0, drag_time);

      return TRUE;
    }

  return FALSE;
}



static gboolean
panel_application_drag_drop (GtkWidget        *itembar,
                             GdkDragContext   *context,
                             gint              x,
                             gint              y,
                             guint             drag_time,
                             PanelApplication *application)
{
  GdkAtom target;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);

  target = gtk_drag_dest_find_target (itembar, context, NULL);

  /* we cannot handle the drag data */
  if (G_UNLIKELY (target == GDK_NONE))
    return FALSE;

  /* set state so the drag-data-received handler
   * knows that this is really a drop this time. */
  application->drop_occurred = TRUE;

  /* request the drag data from the source. */
  gtk_drag_get_data (itembar, context, target, drag_time);

  /* we call gtk_drag_finish() later */
  return TRUE;
}



static void
panel_application_drag_leave (GtkWidget        *itembar,
                              GdkDragContext   *context,
                              guint             drag_time,
                              PanelApplication *application)
{
  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* cleanup the drop data */
  if (application->drop_uris != NULL)
    {
      g_strfreev (application->drop_uris);
      application->drop_uris = NULL;
    }

  /* reset the state */
  application->drop_data_ready = FALSE;
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
  g_signal_connect (G_OBJECT (dialog), "destroy",
                    G_CALLBACK (panel_application_dialog_destroyed), application);

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
      panel_application_plugin_insert (application, window,
                                       gtk_widget_get_screen (GTK_WIDGET (window)),
                                       plugin_name, -1, arguments, -1);
    }
  else
    {
      /* print warning */
      g_warning (_("The plugin (%s) you want to add is not "
                   "recognized by the panel."), plugin_name);
    }
}



PanelWindow *
panel_application_new_window (PanelApplication *application,
                              GdkScreen        *screen,
                              gboolean          new_window)
{
  GtkWidget *window;
  GtkWidget *itembar;
  gchar     *property;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (XFCONF_IS_CHANNEL (application->xfconf), NULL);

  /* create panel window */
  window = panel_window_new ();

  /* realize */
  gtk_widget_realize (window);

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (panel_application_window_destroyed), application);

  /* put on the correct screen */
  gtk_window_set_screen (GTK_WINDOW (window), screen ? screen : gdk_screen_get_default ());

  /* add the window to internal list */
  application->windows = g_slist_append (application->windows, window);

  /* flush the window properties */
  if (new_window)
    {
      /* remove the xfconf properties */
      property = g_strdup_printf ("/panels/panel-%d", g_slist_index (application->windows, window));
      xfconf_channel_reset_property (application->xfconf, property, TRUE);
      g_free (property);

      /* set default position */
      g_object_set (G_OBJECT (window), "position", "p=0;x=100;y=100", NULL);
    }

  /* add the itembar */
  itembar = panel_itembar_new ();
  exo_binding_new (G_OBJECT (window), "horizontal", G_OBJECT (itembar), "horizontal");
  gtk_container_add (GTK_CONTAINER (window), itembar);
  gtk_widget_show (itembar);

  /* set the itembar drag destination targets */
  gtk_drag_dest_set (GTK_WIDGET (itembar), 0,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  /* signals for drag and drop */
  g_signal_connect (G_OBJECT (itembar), "drag-data-received",
                    G_CALLBACK (panel_application_drag_data_received), window);
  g_signal_connect (G_OBJECT (itembar), "drag-motion",
                    G_CALLBACK (panel_application_drag_motion), application);
  g_signal_connect (G_OBJECT (itembar), "drag-drop",
                    G_CALLBACK (panel_application_drag_drop), application);
  g_signal_connect (G_OBJECT (itembar), "drag-leave",
                    G_CALLBACK (panel_application_drag_leave), application);

  /* add the xfconf bindings */
  panel_application_xfconf_window_bindings (application, PANEL_WINDOW (window), FALSE);

  return PANEL_WINDOW (window);
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
    g_object_set (G_OBJECT (li->data), "active", li->data == window, NULL);
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
