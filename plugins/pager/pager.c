/* $Id$ */
/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2008 Nick Schermer <nick@xfce.org>
 *
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

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>
#include <libwnck/libwnck.h>

#include "pager.h"
#include "pager-dialog_glade.h"



static gboolean pager_plugin_scroll_event              (GtkWidget         *widget,
                                                        GdkEventScroll    *event);
static void     pager_plugin_screen_changed            (GtkWidget         *widget,
                                                        GdkScreen         *previous_screen);
static void     pager_plugin_construct                 (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_free_data                 (XfcePanelPlugin   *panel_plugin);
static gboolean pager_plugin_size_changed              (XfcePanelPlugin   *panel_plugin,
                                                        gint               size);
static void     pager_plugin_save                      (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_configure_plugin          (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_property_changed          (XfconfChannel     *channel,
                                                        const gchar       *property_name,
                                                        const GValue      *value,
                                                        PagerPlugin       *plugin);



struct _PagerPluginClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;
};

struct _PagerPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* the wnck pager */
  GtkWidget     *wnck_pager;

  /* xfconf channel */
  XfconfChannel *channel;

  /* the active wnck screen */
  WnckScreen    *wnck_screen;

  /* settings */
  guint          scrolling : 1;
  guint          show_names : 1;
  gint           rows;
};



G_DEFINE_TYPE (PagerPlugin, pager_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_PAGER_PLUGIN);



static void
pager_plugin_class_init (PagerPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GtkWidgetClass       *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->scroll_event = pager_plugin_scroll_event;
  widget_class->screen_changed = pager_plugin_screen_changed;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = pager_plugin_construct;
  plugin_class->free_data = pager_plugin_free_data;
  plugin_class->save = pager_plugin_save;
  plugin_class->size_changed = pager_plugin_size_changed;
  plugin_class->configure_plugin = pager_plugin_configure_plugin;
}



static void
pager_plugin_init (PagerPlugin *plugin)
{
  /* init, draw nothing */
  plugin->wnck_screen = NULL;
  plugin->scrolling = FALSE;
  plugin->show_names = FALSE;
  plugin->rows = 1;
  plugin->wnck_pager = NULL;
  plugin->channel = NULL;

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* initialize xfconf */
  xfconf_init (NULL);
}



static gboolean
pager_plugin_scroll_event (GtkWidget      *widget,
                           GdkEventScroll *event)
{
  PagerPlugin         *plugin = XFCE_PAGER_PLUGIN (widget);
  WnckWorkspace       *active, *neighbor;
  WnckMotionDirection  direction;

  panel_return_val_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen), FALSE);

  /* leave when scrolling is not enabled */
  if (plugin->scrolling == FALSE)
    return TRUE;

  /* translate the gdk scroll direction into a wnck motion direction */
  switch (event->direction)
    {
      case GDK_SCROLL_UP:   direction = WNCK_MOTION_UP;    break;
      case GDK_SCROLL_DOWN: direction = WNCK_MOTION_DOWN;  break;
      case GDK_SCROLL_LEFT: direction = WNCK_MOTION_LEFT;  break;
      default:              direction = WNCK_MOTION_RIGHT; break;
    }

  /* get the active workspace's neighbor */
  active = wnck_screen_get_active_workspace (plugin->wnck_screen);
  neighbor = wnck_workspace_get_neighbor (active, direction);

  /* if there is a neighbor, move to it */
  if (neighbor != NULL)
    wnck_workspace_activate (neighbor, event->time);

  return TRUE;
}



static void
pager_plugin_screen_changed (GtkWidget *widget,
                             GdkScreen *previous_screen)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (widget);
  GdkScreen   *screen;
  WnckScreen  *wnck_screen;

  /* do nothing until the xfconf properties are initialized */
  if (plugin->channel == NULL)
    return;

  /* get the active screen */
  screen = gtk_widget_get_screen (widget);
  wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));

  /* only update if the screen changed */
  if (plugin->wnck_screen != wnck_screen)
    {
      /* destroy the existing pager */
      if (G_UNLIKELY (plugin->wnck_pager != NULL))
        gtk_widget_destroy (GTK_WIDGET (plugin->wnck_pager));

      /* set the new screen */
      plugin->wnck_screen = wnck_screen;

      /* create the wnck pager */
      plugin->wnck_pager = wnck_pager_new (wnck_screen);
      gtk_container_add (GTK_CONTAINER (widget), plugin->wnck_pager);
      gtk_widget_show (plugin->wnck_pager);

      /* set the pager properties */
      wnck_pager_set_display_mode (WNCK_PAGER (plugin->wnck_pager),
                                   plugin->show_names ?
                                   WNCK_PAGER_DISPLAY_NAME :
                                   WNCK_PAGER_DISPLAY_CONTENT);
      wnck_pager_set_n_rows (WNCK_PAGER (plugin->wnck_pager), plugin->rows);
    }
}



static void
pager_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  /* set the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);
  g_signal_connect (G_OBJECT (plugin->channel), "property-changed",
                    G_CALLBACK (pager_plugin_property_changed), plugin);

  /* read xfconf settings */
  plugin->scrolling = xfconf_channel_get_bool (plugin->channel, "/workspace-scrolling", FALSE);
  plugin->show_names = xfconf_channel_get_bool (plugin->channel, "/show-names", FALSE);
  plugin->rows = CLAMP (xfconf_channel_get_int (plugin->channel, "/rows", 1), 1, 50);\

  /* create the pager */
  pager_plugin_screen_changed (GTK_WIDGET (panel_plugin), NULL);
}



static void
pager_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* release the xfonf channel */
  g_object_unref (G_OBJECT (plugin->channel));

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static gboolean
pager_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint             size)
{
  if (xfce_panel_plugin_get_orientation (panel_plugin) ==
      GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, -1);

  return TRUE;
}



static void
pager_plugin_save (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* store settings */
  xfconf_channel_set_bool (plugin->channel, "/workspace-scrolling", plugin->scrolling);
  xfconf_channel_set_bool (plugin->channel, "/show-names", plugin->show_names);
  xfconf_channel_set_uint (plugin->channel, "/rows", plugin->rows);
}



static void
pager_plugin_configure_workspace_settings (GtkWidget *button)
{
  GdkScreen *screen;
  GError    *error = NULL;

  panel_return_if_fail (GTK_IS_WIDGET (button));

  /* get the screen */
  screen = gtk_widget_get_screen (button);
  if (G_UNLIKELY (screen == NULL))
    screen = gdk_screen_get_default ();

  /* try to start the settings dialog */
  if (!gdk_spawn_command_line_on_screen (screen, "xfwm4-workspace-settings", &error))
    {
      /* show an error dialog */
      xfce_dialog_show_error (button, error, _("Unable to open the Xfce workspace settings"));
      g_error_free (error);
    }
}



static void
pager_plugin_configure_n_workspaces_changed (WnckScreen    *wnck_screen,
                                             WnckWorkspace *workspace,
                                             GtkBuilder    *builder)
{
  GObject *object;
  gdouble  n_worspaces, value;

  panel_return_if_fail (WNCK_IS_SCREEN (wnck_screen));
  panel_return_if_fail (GTK_IS_BUILDER (builder));

  /* get the rows adjustment */
  object = gtk_builder_get_object (builder, "rows");

  /* get the number of workspaces and clamp the current value */
  n_worspaces = wnck_screen_get_workspace_count (wnck_screen);
  value = MIN (gtk_adjustment_get_value (GTK_ADJUSTMENT (object)), n_worspaces);

  /* update the adjustment */
  g_object_set (G_OBJECT (object), "upper", n_worspaces, "value", value, NULL);
}



static void
pager_plugin_configure_destroyed (gpointer  data,
                                  GObject  *where_the_object_was)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (data);

  /* disconnect signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin->wnck_screen),
                                        pager_plugin_configure_n_workspaces_changed,
                                        where_the_object_was);

  /* unblock the menu */
  xfce_panel_plugin_unblock_menu (XFCE_PANEL_PLUGIN (plugin));
}



static void
pager_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);
  GtkBuilder  *builder;
  GObject     *dialog, *object;

  panel_return_if_fail (XFCE_IS_PAGER_PLUGIN (plugin));
  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* save before we opend the dialog, so all properties exist in xfonf */
  pager_plugin_save (panel_plugin);

  /* load the dialog from the glade file */
  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, pager_dialog_glade, pager_dialog_glade_length, NULL))
    {
      /* signals to monitor number of workspace changes */
      g_signal_connect (G_OBJECT (plugin->wnck_screen), "workspace-created",
                        G_CALLBACK (pager_plugin_configure_n_workspaces_changed), builder);
      g_signal_connect (G_OBJECT (plugin->wnck_screen), "workspace-destroyed",
                        G_CALLBACK (pager_plugin_configure_n_workspaces_changed), builder);

      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (builder), pager_plugin_configure_destroyed, plugin);

      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);

      object = gtk_builder_get_object (builder, "settings-button");
      g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (pager_plugin_configure_workspace_settings), dialog);

      object = gtk_builder_get_object (builder, "workspace-scrolling");
      xfconf_g_property_bind (plugin->channel, "/workspace-scrolling", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "show-names");
      xfconf_g_property_bind (plugin->channel, "/show-names", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "rows");
      xfconf_g_property_bind (plugin->channel, "/rows", G_TYPE_UINT, object, "value");

      /* update the rows limit */
      pager_plugin_configure_n_workspaces_changed (plugin->wnck_screen, NULL, builder);

      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}



static void
pager_plugin_property_changed (XfconfChannel *channel,
                               const gchar   *property_name,
                               const GValue  *value,
                               PagerPlugin   *plugin)
{
  guint n_workspaces;

  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (XFCE_IS_PAGER_PLUGIN (plugin));
  panel_return_if_fail (plugin->channel == channel);
  panel_return_if_fail (WNCK_IS_PAGER (plugin->wnck_pager));
  panel_return_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen));

  /* update the changed property */
  if (strcmp (property_name, "/workspace-scrolling") == 0)
    {
      /* store new setting */
      plugin->scrolling = g_value_get_boolean (value);
    }
  else if (strcmp (property_name, "/show-names") == 0)
    {
      /* store new value and set wnck pager display mode */
      plugin->show_names = g_value_get_boolean (value);
      wnck_pager_set_display_mode (WNCK_PAGER (plugin->wnck_pager),
                                   plugin->show_names ?
                                   WNCK_PAGER_DISPLAY_NAME :
                                   WNCK_PAGER_DISPLAY_CONTENT);
    }
  else if (strcmp (property_name, "/rows") == 0)
    {
      /* store new value and set wnck pager rows */
      n_workspaces = MIN (wnck_screen_get_workspace_count (plugin->wnck_screen), 1);
      plugin->rows = CLAMP (g_value_get_uint (value), 1, n_workspaces);
      wnck_pager_set_n_rows (WNCK_PAGER (plugin->wnck_pager), plugin->rows);
    }
}
