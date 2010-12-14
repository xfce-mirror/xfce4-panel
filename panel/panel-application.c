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
#include <libxfce4ui/libxfce4ui.h>

#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-debug.h>
#include <common/panel-utils.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-dbus-service.h>
#include <panel/panel-base-window.h>
#include <panel/panel-plugin-external-46.h>
#include <panel/panel-window.h>
#include <panel/panel-application.h>
#include <panel/panel-itembar.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-plugin-external.h>

#define AUTOSAVE_INTERVAL (10 * 60)
#define MIGRATE_BIN       LIBDIR G_DIR_SEPARATOR_S "panel" G_DIR_SEPARATOR_S "migrate"



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
static void      panel_application_window_destroyed   (GtkWidget              *window,
                                                       PanelApplication       *application);
static void      panel_application_dialog_destroyed   (GtkWindow              *dialog,
                                                       PanelApplication       *application);
static void      panel_application_drag_data_received (PanelWindow            *window,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       GtkSelectionData       *selection_data,
                                                       guint                   info,
                                                       guint                   drag_time,
                                                       GtkWidget              *itembar);
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
  GSList             *windows;

  /* internal list of opened dialogs */
  GSList             *dialogs;

  /* autosave timeout */
  guint               autosave_timeout_id;

  /* drag and drop data */
  guint               drop_data_ready : 1;
  guint               drop_occurred : 1;
  guint               drop_desktop_files : 1;
  guint               drop_index;
};

enum
{
  TARGET_PLUGIN_NAME,
  TARGET_PLUGIN_WIDGET,
  TARGET_TEXT_URI_LIST
};

static const GtkTargetEntry drag_targets[] =
{
  { "xfce-panel/plugin-widget",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET }
};

static const GtkTargetEntry drop_targets[] =
{
  { "xfce-panel/plugin-name",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_NAME },
  { "xfce-panel/plugin-widget",
    GTK_TARGET_SAME_APP, TARGET_PLUGIN_WIDGET },
  { "text/uri-list", 0, TARGET_TEXT_URI_LIST }
};



G_DEFINE_TYPE (PanelApplication, panel_application, G_TYPE_OBJECT)



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
  PanelWindow *window;
  GError      *error = NULL;

  application->windows = NULL;
  application->dialogs = NULL;
  application->autosave_timeout_id = 0;
  application->drop_desktop_files = FALSE;
  application->drop_data_ready = FALSE;
  application->drop_occurred = FALSE;

  /* get the xfconf channel (singleton) */
  application->xfconf = panel_properties_get_channel (G_OBJECT (application));

  /* check if we need to launch the migration application */
  if (!xfconf_channel_has_property (application->xfconf, "/panels"))
    {
      if (!g_spawn_command_line_sync (MIGRATE_BIN, NULL, NULL, NULL, &error))
        {
          xfce_dialog_show_error (NULL, error, _("Failed to launch the migration application"));
          g_error_free (error);
        }
    }

  /* check if we need to force all plugins to run external */
  if (xfconf_channel_get_bool (application->xfconf, "/force-all-external", FALSE))
    panel_module_factory_force_all_external ();

  /* get a factory reference so it never unloads */
  application->factory = panel_module_factory_get ();

  /* load setup */
  panel_application_load (application);

  /* start the autosave timeout */
  application->autosave_timeout_id =
      g_timeout_add_seconds (AUTOSAVE_INTERVAL,
                             panel_application_save_timeout,
                             application);

  /* create empty window if everything else failed */
  if (G_UNLIKELY (application->windows == NULL))
    window = panel_application_new_window (application, NULL, TRUE);
}



static void
panel_application_finalize (GObject *object)
{
  PanelApplication *application = PANEL_APPLICATION (object);
  GSList           *li;

  panel_return_if_fail (application->dialogs == NULL);

  /* stop the autosave timeout */
  g_source_remove (application->autosave_timeout_id);

  /* free all windows */
  for (li = application->windows; li != NULL; li = li->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (li->data),
          G_CALLBACK (panel_application_window_destroyed), application);
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }
  g_slist_free (application->windows);

  g_object_unref (G_OBJECT (application->factory));

  /* this is a good reference if all the objects are released */
  panel_debug (PANEL_DEBUG_DOMAIN_APPLICATION, "finalized");

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
    { "position-locked", G_TYPE_BOOLEAN },
    { "autohide", G_TYPE_BOOLEAN },
    { "span-monitors", G_TYPE_BOOLEAN },
    { "horizontal", G_TYPE_BOOLEAN },
    { "size", G_TYPE_UINT },
    { "length", G_TYPE_UINT },
    { "length-adjust", G_TYPE_BOOLEAN },
    { "enter-opacity", G_TYPE_UINT },
    { "leave-opacity", G_TYPE_UINT },
    { "background-alpha", G_TYPE_UINT },
    { "background-style", G_TYPE_UINT },
    { "background-color", GDK_TYPE_COLOR },
    { "background-image", G_TYPE_STRING },
    { "output-name", G_TYPE_STRING },
    { "position", G_TYPE_STRING },
    { "disable-struts", G_TYPE_BOOLEAN },
    { NULL }
  };

  panel_return_if_fail (XFCONF_IS_CHANNEL (application->xfconf));
  panel_return_if_fail (g_slist_index (application->windows, window) > -1);

  /* create the property base */
  property_base = g_strdup_printf ("/panels/panel-%d",
      g_slist_index (application->windows, window));

  /* bind all the properties */
  panel_properties_bind (application->xfconf, G_OBJECT (window),
                         property_base, properties, save_properties);

  /* set locking for this panel */
  panel_window_set_locked (window,
      xfconf_channel_is_property_locked (application->xfconf, property_base));

  g_free (property_base);
}



static void
panel_application_load (PanelApplication *application)
{
  PanelWindow  *window;
  guint         i, j, n_panels;
  gchar         buf[50];
  gchar        *name;
  gint          unique_id;
  GdkScreen    *screen;
  GPtrArray    *array;
  const GValue *value;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCONF_IS_CHANNEL (application->xfconf));

  /* walk all the panel in the configuration */
  n_panels = xfconf_channel_get_uint (application->xfconf, "/panels", 0);
  for (i = 0; i < n_panels; i++)
    {
      /* create a new window */
      window = panel_application_new_window (application, NULL, FALSE);
      screen = gtk_window_get_screen (GTK_WINDOW (window));

      /* walk all the plugins on the panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugin-ids", i);
      array = xfconf_channel_get_arrayv (application->xfconf, buf);
      if (array == NULL)
        continue;

      /* walk the array in pairs of two */
      for (j = 0; j < array->len; j++)
        {
          /* get the plugin id */
          value = g_ptr_array_index (array, j);
          panel_assert (value != NULL);
          unique_id = g_value_get_int (value);

          /* get the plugin name */
          g_snprintf (buf, sizeof (buf), "/plugins/plugin-%d", unique_id);
          name = xfconf_channel_get_string (application->xfconf, buf, NULL);

          /* append the plugin to the panel */
          if (unique_id < 1 || name == NULL
              || !panel_application_plugin_insert (application, window, screen,
                                                   name, unique_id, NULL, -1))
            {
              /* plugin could not be loaded, remove it from the channel */
              g_snprintf (buf, sizeof (buf), "/panels/plugin-%d", unique_id);
              if (xfconf_channel_has_property (application->xfconf, buf))
                xfconf_channel_reset_property (application->xfconf, buf, TRUE);

              /* show warnings */
              g_message ("Plugin \"%s-%d\" was not found and has been "
                         "removed from the configuration", name, unique_id);
            }

          g_free (name);
        }

      xfconf_array_free (array);
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

  /* make the window sensitive again */
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
  GtkIconTheme   *theme;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* make the window insensitive */
  panel_application_windows_sensitive (application, FALSE);

  /* create drag context */
  target_list = gtk_target_list_new (drag_targets, G_N_ELEMENTS (drag_targets));
  context = gtk_drag_begin (item, target_list, GDK_ACTION_MOVE, 1, NULL);
  gtk_target_list_unref (target_list);

  /* set the drag context icon name */
  module = panel_module_get_from_plugin_provider (XFCE_PANEL_PLUGIN_PROVIDER (item));
  icon_name = panel_module_get_icon_name (module);
  theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (item));
  if (!exo_str_is_empty (icon_name)
      && gtk_icon_theme_has_icon (theme, icon_name))
    gtk_drag_set_icon_name (context, icon_name, 0, 0);
  else
    gtk_drag_set_icon_default (context);

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
  panel_return_if_fail (!exo_str_is_empty (name));
  panel_return_if_fail (unique_id != -1);

  /* remove the xfconf property */
  property = g_strdup_printf (PANEL_PLUGIN_PROPERTY_BASE, unique_id);
  if (xfconf_channel_has_property (application->xfconf, property))
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
panel_application_plugin_remove (GtkWidget *widget,
                                 gpointer   user_data)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (widget));

  /* ask the plugin to cleanup when we destroy a panel window */
  xfce_panel_plugin_provider_emit_signal (XFCE_PANEL_PLUGIN_PROVIDER (widget),
                                          PROVIDER_SIGNAL_REMOVE_PLUGIN);
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

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

  window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider)));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  switch (provider_signal)
    {
    case PROVIDER_SIGNAL_MOVE_PLUGIN:
      /* check the window locking, not that of the provider, because
       * the users might have worked around that and both should be identical */
      if (!panel_window_get_locked (window))
        panel_application_plugin_move (GTK_WIDGET (provider), application);
      break;

    case PROVIDER_SIGNAL_EXPAND_PLUGIN:
    case PROVIDER_SIGNAL_COLLAPSE_PLUGIN:
      itembar = gtk_bin_get_child (GTK_BIN (window));
      gtk_container_child_set (GTK_CONTAINER (itembar),
                               GTK_WIDGET (provider),
                               "expand",
                               provider_signal == PROVIDER_SIGNAL_EXPAND_PLUGIN,
                               NULL);
      break;

    case PROVIDER_SIGNAL_WRAP_PLUGIN:
    case PROVIDER_SIGNAL_UNWRAP_PLUGIN:
      itembar = gtk_bin_get_child (GTK_BIN (window));
      gtk_container_child_set (GTK_CONTAINER (itembar),
                               GTK_WIDGET (provider),
                               "wrap",
                               provider_signal == PROVIDER_SIGNAL_WRAP_PLUGIN,
                               NULL);
      break;

    case PROVIDER_SIGNAL_LOCK_PANEL:
      /* increase window's autohide counter */
      panel_window_freeze_autohide (window);
      break;

    case PROVIDER_SIGNAL_UNLOCK_PANEL:
      /* decrease window's autohide counter */
      panel_window_thaw_autohide (window);
      break;

    case PROVIDER_SIGNAL_REMOVE_PLUGIN:
      /* check the window locking, not that of the provider, because
       * the users might have worked around that and both should be identical */
      if (!panel_window_get_locked (window))
        {
          /* give plugin the opportunity to cleanup special configuration */
          xfce_panel_plugin_provider_removed (provider);

          /* store the provider's unique id and name (lost after destroy) */
          unique_id = xfce_panel_plugin_provider_get_unique_id (provider);
          name = g_strdup (xfce_panel_plugin_provider_get_name (provider));

          /* destroy the plugin */
          gtk_widget_destroy (GTK_WIDGET (provider));

          /* remove the plugin configuration */
          panel_application_plugin_delete_config (application, name, unique_id);
          g_free (name);
        }
      break;

    case PROVIDER_SIGNAL_ADD_NEW_ITEMS:
      /* open the items dialog, locking is handled in the object */
      panel_item_dialog_show (window);
      break;

    case PROVIDER_SIGNAL_PANEL_PREFERENCES:
      /* open the panel preferences, locking is handled in the object */
      panel_preferences_dialog_show (window);
      break;

    case PROVIDER_SIGNAL_PANEL_LOGOUT:
      /* logout */
      panel_application_logout ();
      break;

    case PROVIDER_SIGNAL_PANEL_ABOUT:
      /* show the about dialog */
      panel_dialogs_show_about ();
      break;

    case PROVIDER_SIGNAL_PANEL_HELP:
      /* try to launch help browser */
      panel_utils_show_help (NULL, NULL, NULL);
      break;

    case PROVIDER_SIGNAL_FOCUS_PLUGIN:
       /* focus the panel window (as part of focusing a widget within the plugin) */
       panel_window_focus (window);
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
  gint       new_unique_id;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (name != NULL, FALSE);

  /* leave if the window is locked */
  if (panel_window_get_locked (window))
    return FALSE;

  /* create a new panel plugin */
  provider = panel_module_factory_new_plugin (application->factory,
                                              name, screen, unique_id,
                                              arguments, &new_unique_id);
  if (G_UNLIKELY (provider == NULL))
    return FALSE;

  /* make sure there is no panel configuration with this unique id when a
   * new plugin is created */
  if (G_UNLIKELY (unique_id == -1))
    panel_application_plugin_delete_config (application, name, new_unique_id);

  /* add signal to monitor provider signals */
  g_signal_connect (G_OBJECT (provider), "provider-signal",
      G_CALLBACK (panel_application_plugin_provider_signal), application);

  /* work around the problem that we need a background before
   * realizing for 4.6 panel plugins */
  if (PANEL_BASE_WINDOW (window)->background_style == PANEL_BG_STYLE_IMAGE
      && PANEL_IS_PLUGIN_EXTERNAL_46 (provider))
    panel_plugin_external_set_background_image (PANEL_PLUGIN_EXTERNAL (provider),
        PANEL_BASE_WINDOW (window)->background_image);

  /* add the item to the panel */
  itembar = gtk_bin_get_child (GTK_BIN (window));
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

  panel_application_save (PANEL_APPLICATION (user_data), TRUE);

  GDK_THREADS_LEAVE ();

  return TRUE;
}



static void
panel_application_window_destroyed (GtkWidget        *window,
                                    PanelApplication *application)
{
  guint      n;
  gchar     *property;
  GSList    *li, *lnext;
  gboolean   passed_destroyed_window = FALSE;
  GtkWidget *itembar;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (g_slist_find (application->windows, window) != NULL);

  /* leave if the application is locked */
  if (panel_application_get_locked (application))
    return;

  /* we need to update the bindings of all the panels... */
  for (li = application->windows, n = 0; li != NULL; li = lnext, n++)
    {
      lnext = li->next;

      /* TODO, this might go wrong when only 1 window is locked */
      if (panel_window_get_locked (li->data))
        continue;

      if (passed_destroyed_window)
        {
          /* save this panel again at it's new position */
          panel_properties_unbind (G_OBJECT (li->data));
          panel_application_xfconf_window_bindings (application,
                                                    PANEL_WINDOW (li->data),
                                                    TRUE);
        }
      else if (li->data == window)
        {
          /* disconnect bindings from this panel */
          panel_properties_unbind (G_OBJECT (window));

          /* remove all the plugins from the itembar */
          itembar = gtk_bin_get_child (GTK_BIN (window));
          gtk_container_foreach (GTK_CONTAINER (itembar),
              panel_application_plugin_remove, NULL);

          /* remove from the internal list */
          application->windows = g_slist_delete_link (application->windows, li);

          /* keep updating the bindings for the remaining windows */
          passed_destroyed_window = TRUE;
        }
    }

  /* remove the last property from the channel */
  property = g_strdup_printf ("/panels/panel-%u", n - 1);
  panel_assert (n - 1 == g_slist_length (application->windows));
  xfconf_channel_reset_property (application->xfconf, property, TRUE);
  g_free (property);

  /* schedule a save to store the new number of panels */
  panel_application_save (application, FALSE);

  /* quit if there are no windows */
  /* TODO, allow removing all windows and ask user what to do */
  if (application->windows == NULL)
    gtk_main_quit ();
}



static void
panel_application_windows_block_autohide (PanelApplication *application,
                                          gboolean          blocked)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  for (li = application->windows; li != NULL; li = li->next)
    {
      if (blocked)
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
    }
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

  /* unblock autohide if there are no open windows anymore */
  if (application->dialogs == NULL)
    panel_application_windows_block_autohide (application, FALSE);
}



static void
panel_application_drag_data_received (PanelWindow      *window,
                                      GdkDragContext   *context,
                                      gint              x,
                                      gint              y,
                                      GtkSelectionData *selection_data,
                                      guint             info,
                                      guint             drag_time,
                                      GtkWidget        *itembar)
{
  PanelApplication  *application;
  GtkWidget         *provider;
  gboolean           succeed = FALSE;
  GdkScreen         *screen;
  const gchar       *name;
  guint              old_position;
  gchar            **uris;
  guint              i;
  gboolean           found;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));

  /* we don't allow any kind of drops here when the panel is locked */
  if (panel_window_get_locked (window))
    {
      gdk_drag_status (context, 0, drag_time);
      return;
    }

  /* get the application */
  application = panel_application_get ();

  if (!application->drop_data_ready)
    {
      panel_assert (!application->drop_desktop_files);

      if (info == TARGET_TEXT_URI_LIST)
        {
          /* look if the selection data contains atleast 1 desktop file */
          uris = gtk_selection_data_get_uris (selection_data);
          if (G_LIKELY (uris != NULL))
            {
              for (i = 0, found = FALSE; !found && uris[i] != NULL; i++)
                found = g_str_has_suffix (uris[i], ".desktop");
              g_strfreev (uris);
              application->drop_desktop_files = found;
            }
        }

      /* reset the state */
      application->drop_data_ready = TRUE;
    }

  /* check if the data was droppped */
  if (application->drop_occurred)
    {
      /* reset the state */
      application->drop_occurred = FALSE;

      /* get the widget screen */
      screen = gtk_window_get_screen (GTK_WINDOW (window));

      switch (info)
        {
        case TARGET_PLUGIN_NAME:
          if (G_LIKELY (selection_data->length > 0))
            {
              /* create a new item with a unique id */
              name = (const gchar *) selection_data->data;
              succeed = panel_application_plugin_insert (application, window,
                                                         screen, name,
                                                         -1, NULL, application->drop_index);
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
              if (application->drop_index > old_position)
                application->drop_index--;

              /* reorder the child if needed */
              if (old_position != application->drop_index)
                panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, application->drop_index);
            }
          else
            {
              /* reparent the widget, this will also call remove and add for the itembar */
              gtk_widget_reparent (provider, itembar);

              /* move the item to the correct position on the itembar */
              panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, application->drop_index);

              /* send all the needed panel information to the plugin */
              panel_window_set_povider_info (window, provider);
            }

          /* everything went fine */
          succeed = TRUE;
          break;

        case TARGET_TEXT_URI_LIST:
          if (G_LIKELY (application->drop_desktop_files))
            {
              /* pass all the uris to the launcher, it will filter out
               * the desktop files on it's own */
              uris = gtk_selection_data_get_uris (selection_data);
              if (G_LIKELY (uris != NULL))
                {
                  /* create a new item with a unique id */
                  succeed = panel_application_plugin_insert (application, window,
                                                             screen, LAUNCHER_PLUGIN_NAME,
                                                             -1, uris, application->drop_index);
                  g_strfreev (uris);
                }

              application->drop_desktop_files = FALSE;
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
    }
  else
    {
      gdk_drag_status (context, 0, drag_time);
    }

  g_object_unref (G_OBJECT (application));
}



static gboolean
panel_application_drag_motion (GtkWidget        *window,
                               GdkDragContext   *context,
                               gint              x,
                               gint              y,
                               guint             drag_time,
                               PanelApplication *application)
{
  GdkAtom        target;
  GtkWidget     *itembar;
  GdkDragAction  drag_action = 0;

  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (!application->drop_occurred, FALSE);

  /* don't allow anything when the window is locked */
  if (panel_window_get_locked (PANEL_WINDOW (window)))
    goto not_a_drop_zone;

  /* determine the drag target */
  target = gtk_drag_dest_find_target (window, context, NULL);

  if (target == gdk_atom_intern_static_string ("text/uri-list"))
    {
      /* request the drop data on-demand (if we don't have it already) */
      if (!application->drop_data_ready)
        {
          /* request the drop data from the source */
          gtk_drag_get_data (window, context, target, drag_time);

          /* we cannot drop here (yet!) */
          return TRUE;
        }
      else if (application->drop_desktop_files)
        {
          /* there are valid uris in the drop data */
          drag_action = GDK_ACTION_COPY;
        }
    }
  else if (target == gdk_atom_intern_static_string ("xfce-panel/plugin-name"))
    {
      /* insert a new plugin */
      drag_action = GDK_ACTION_COPY;
    }
  else if (target == gdk_atom_intern_static_string ("xfce-panel/plugin-widget"))
    {
      /* move an existing plugin */
      drag_action = GDK_ACTION_MOVE;
    }

  if (drag_action != 0)
    {
      /* highlight the drop zone */
      itembar = gtk_bin_get_child (GTK_BIN (window));
      application->drop_index = panel_itembar_get_drop_index (PANEL_ITEMBAR (itembar), x, y);
      panel_itembar_set_drop_highlight_item (PANEL_ITEMBAR (itembar), application->drop_index);
    }

not_a_drop_zone:
  gdk_drag_status (context, drag_action, drag_time);

  return (drag_action == 0);
}



static gboolean
panel_application_drag_drop (GtkWidget        *window,
                             GdkDragContext   *context,
                             gint              x,
                             gint              y,
                             guint             drag_time,
                             PanelApplication *application)
{
  GdkAtom target;

  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);

  target = gtk_drag_dest_find_target (window, context, NULL);

  /* we cannot handle the drop */
  if (G_UNLIKELY (target == GDK_NONE))
    return FALSE;

  /* set state so the drag-data-received handler
   * knows that this is really a drop this time. */
  application->drop_occurred = TRUE;

  /* request the drag data from the source. */
  gtk_drag_get_data (window, context, target, drag_time);

  /* we call gtk_drag_finish() later */
  return TRUE;
}



static void
panel_application_drag_leave (GtkWidget        *window,
                              GdkDragContext   *context,
                              guint             drag_time,
                              PanelApplication *application)
{
  GtkWidget *itembar;

  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* reset the state */
  application->drop_data_ready = FALSE;
  application->drop_desktop_files = FALSE;
  application->drop_occurred = FALSE;

  /* unset the highlight position */
  itembar = gtk_bin_get_child (GTK_BIN (window));
  panel_itembar_set_drop_highlight_item (PANEL_ITEMBAR (itembar), -1);
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
  guint                    i;
  gchar                    buf[50];
  XfconfChannel           *channel = application->xfconf;
  GPtrArray               *array;
  GValue                  *value;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));

  for (li = application->windows, i = 0; li != NULL; li = li->next, i++)
    {
      /* skip this window if it is locked */
      if (panel_window_get_locked (li->data))
        continue;

      panel_debug (PANEL_DEBUG_DOMAIN_APPLICATION,
                   "saving /panels/panel-%u, save-plugins=%s",
                   i, PANEL_DEBUG_BOOL (save_plugin_providers));

      /* get the itembar children */
      itembar = gtk_bin_get_child (GTK_BIN (li->data));
      children = gtk_container_get_children (GTK_CONTAINER (itembar));

      /* only cleanup and continue if there are no children */
      if (G_UNLIKELY (children == NULL))
        {
          g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugin-ids", i);
          if (xfconf_channel_has_property (channel, buf))
            xfconf_channel_reset_property (channel, buf, FALSE);
          continue;
        }

      /* create empty pointer array */
      array = g_ptr_array_new ();

      /* walk all the plugin children */
      for (lp = children; lp != NULL; lp = lp->next)
        {
          provider = XFCE_PANEL_PLUGIN_PROVIDER (lp->data);

          /* add plugin id to the array */
          value = g_new0 (GValue, 1);
          g_value_init (value, G_TYPE_INT);
          g_value_set_int (value,
              xfce_panel_plugin_provider_get_unique_id (provider));
          g_ptr_array_add (array, value);

          /* make sure the plugin type is store in the plugin item */
          g_snprintf (buf, sizeof (buf), "/plugins/plugin-%d", g_value_get_int (value));
          xfconf_channel_set_string (channel, buf, xfce_panel_plugin_provider_get_name (provider));

          /* ask the plugin to save */
          if (save_plugin_providers)
            xfce_panel_plugin_provider_save (provider);
        }

      /* store the plugins for this panel */
      g_snprintf (buf, sizeof (buf), "/panels/panel-%u/plugin-ids", i);
      xfconf_channel_set_arrayv (channel, buf, array);

      g_list_free (children);
      xfconf_array_free (array);
    }

  /* store the number of panels */
  if (!xfconf_channel_is_property_locked (channel, "/panels"))
    xfconf_channel_set_uint (channel, "/panels", i);
}



void
panel_application_take_dialog (PanelApplication *application,
                               GtkWindow        *dialog)
{
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (GTK_IS_WINDOW (dialog));

  /* block autohide if this will be the first dialog */
  if (application->dialogs == NULL)
    panel_application_windows_block_autohide (application, TRUE);

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (dialog), "destroy",
      G_CALLBACK (panel_application_dialog_destroyed), application);
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
      lnext = li->next;
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }

  panel_return_if_fail (application->dialogs == NULL);
}



void
panel_application_add_new_item (PanelApplication  *application,
                                const gchar       *plugin_name,
                                gchar            **arguments)
{
  gint         nth = 0;
  GSList      *li;
  gboolean     active;
  PanelWindow *window;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (plugin_name != NULL);
  panel_return_if_fail (g_slist_length (application->windows) > 0);

  if (panel_module_factory_has_module (application->factory, plugin_name))
    {
      /* find a suitable window if there are 2 or more windows */
      if (LIST_HAS_TWO_OR_MORE_ENTRIES (application->windows))
        {
          /* try to find an avtive panel */
          for (li = application->windows, nth = 0; li != NULL; li = li->next, nth++)
            {
              g_object_get (G_OBJECT (li->data), "active", &active, NULL);
              if (active)
                break;
            }

          /* no active panel found, ask user to select a panel, leave when
           * the cancel button is pressed */
          if (li == NULL
              && (nth = panel_dialogs_choose_panel (application)) == -1)
            return;
        }

      /* add the plugin to the end of the choosen window */
      window = g_slist_nth_data (application->windows, nth);
      panel_application_plugin_insert (application, window,
                                       gtk_widget_get_screen (GTK_WIDGET (window)),
                                       plugin_name, -1, arguments, -1);
    }
  else
    {
      g_warning ("The plugin \"%s\" you want to add is not "
                 "known by the panel", plugin_name);
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
  gint       idx;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), NULL);
  panel_return_val_if_fail (XFCONF_IS_CHANNEL (application->xfconf), NULL);

  /* create panel window */
  window = panel_window_new ();

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
      idx = g_slist_index (application->windows, window);
      property = g_strdup_printf ("/panels/panel-%d", idx);
      xfconf_channel_reset_property (application->xfconf, property, TRUE);
      g_free (property);

      /* set default position */
      g_object_set (G_OBJECT (window), "position", "p=0;x=100;y=100", NULL);
    }

  /* add the itembar */
  itembar = panel_itembar_new ();
  exo_binding_new (G_OBJECT (window), "horizontal", G_OBJECT (itembar), "horizontal");
  exo_binding_new (G_OBJECT (window), "size", G_OBJECT (itembar), "size");
  gtk_container_add (GTK_CONTAINER (window), itembar);
  gtk_widget_show (itembar);

  /* set the itembar drag destination targets */
  gtk_drag_dest_set (GTK_WIDGET (window), 0,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  /* signals for drag and drop */
  g_signal_connect (G_OBJECT (window), "drag-data-received",
                    G_CALLBACK (panel_application_drag_data_received), itembar);
  g_signal_connect (G_OBJECT (window), "drag-motion",
                    G_CALLBACK (panel_application_drag_motion), application);
  g_signal_connect (G_OBJECT (window), "drag-drop",
                    G_CALLBACK (panel_application_drag_drop), application);
  g_signal_connect (G_OBJECT (window), "drag-leave",
                    G_CALLBACK (panel_application_drag_leave), application);

  /* add the xfconf bindings */
  panel_application_xfconf_window_bindings (application, PANEL_WINDOW (window), FALSE);

  /* make sure the position of the panel is always saved else
   * the new window won't be visible on restart */
  if (new_window)
    g_object_notify (G_OBJECT (window), "position");

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
panel_application_get_nth_window (PanelApplication *application,
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

  /* update state for all windows */
  for (li = application->windows; li != NULL; li = li->next)
    g_object_set (G_OBJECT (li->data), "active", window == li->data, NULL);
}



void
panel_application_windows_sensitive (PanelApplication *application,
                                     gboolean          sensitive)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* walk the windows */
  for (li = application->windows; li != NULL; li = li->next)
    {
      /* block autohide for all windows */
      if (sensitive)
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
    }
}



gboolean
panel_application_get_locked (PanelApplication *application)
{
  GSList *li;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), TRUE);
  panel_return_val_if_fail (XFCONF_IS_CHANNEL (application->xfconf), TRUE);

  /* don't even look for the individual window if the
   * entire channel is locked */
  if (xfconf_channel_is_property_locked (application->xfconf, "/"))
    return TRUE;

  /* if one of the windows is not locked, the user can still modify
   * some settings, so then we return %FALSE */
  for (li = application->windows; li != NULL; li = li->next)
    if (!panel_window_get_locked (li->data))
      return FALSE;

  /* TODO we could extend this to a plugin basis (ie. panels are
   * locked but maybe not all the plugins) */
  return TRUE;
}



void
panel_application_logout (void)
{
  XfceSMClient *sm_client;
  GError       *error = NULL;
  const gchar  *command = "xfce4-session-logout";

  /* first try to session client to logout else fallback and spawn xfce4-session-logout */
  sm_client = xfce_sm_client_get ();
  if (xfce_sm_client_is_connected (sm_client))
    {
      xfce_sm_client_request_shutdown (sm_client, XFCE_SM_CLIENT_SHUTDOWN_HINT_ASK);

      return;
    }
  else if (g_getenv ("SESSION_MANAGER") == NULL)
    {
      if (xfce_dialog_confirm (NULL, GTK_STOCK_QUIT, NULL,
          _("You have started X without session manager. Clicking Quit will close the X server."),
          _("Are you sure you want to quit the panel?")))
        command = "xfce4-panel --quit";
      else
        return;
    }

  if (!g_spawn_command_line_async (command, &error))
    {
      xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\""),
                              command);
      g_error_free (error);
    }
}
