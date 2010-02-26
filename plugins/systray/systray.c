/* $Id$ */
/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>
#include <xfconf/xfconf.h>
#include <exo/exo.h>

#include "systray.h"
#include "systray-box.h"
#include "systray-manager.h"
#include "systray-dialog_glade.h"



static void     systray_plugin_get_property            (GObject         *object,
                                                        guint            prop_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);
static void     systray_plugin_set_property            (GObject         *object,
                                                        guint            prop_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void     systray_plugin_construct               (XfcePanelPlugin *panel_plugin);
static void     systray_plugin_free_data               (XfcePanelPlugin *panel_plugin);
static void     systray_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                                        gint             screen_position);
static void     systray_plugin_orientation_changed     (XfcePanelPlugin *panel_plugin,
                                                        GtkOrientation   orientation);
static gboolean systray_plugin_size_changed            (XfcePanelPlugin *panel_plugin,
                                                        gint size);
static void     systray_plugin_configure_plugin        (XfcePanelPlugin *panel_plugin);
static void     systray_plugin_icon_added              (SystrayManager  *manager,
                                                        GtkWidget       *icon,
                                                        SystrayPlugin   *plugin);
static void     systray_plugin_icon_removed            (SystrayManager  *manager,
                                                        GtkWidget       *icon,
                                                        SystrayPlugin   *plugin);
static void     systray_plugin_lost_selection          (SystrayManager  *manager,
                                                        SystrayPlugin   *plugin);



struct _SystrayPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _SystrayPlugin
{
  XfcePanelPlugin __parent__;

  /* systray manager */
  SystrayManager *manager;

  /* xfconf channnel */
  XfconfChannel  *channel;

  /* widgets */
  GtkWidget      *frame;
  GtkWidget      *box;

  /* settings */
  guint           show_frame : 1;
};

enum
{
  PROP_0,
  PROP_ROWS,
  PROP_SHOW_FRAME
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (SystrayPlugin, systray_plugin,
    systray_box_register_type,
    systray_manager_register_type)



static void
systray_plugin_class_init (SystrayPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = systray_plugin_get_property;
  gobject_class->set_property = systray_plugin_set_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = systray_plugin_construct;
  plugin_class->free_data = systray_plugin_free_data;
  plugin_class->size_changed = systray_plugin_size_changed;
  plugin_class->screen_position_changed = systray_plugin_screen_position_changed;
  plugin_class->configure_plugin = systray_plugin_configure_plugin;
  plugin_class->orientation_changed = systray_plugin_orientation_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_ROWS,
                                   g_param_spec_int ("rows",
                                                     NULL, NULL,
                                                     1, 10, 1,
                                                     EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_FRAME,
                                   g_param_spec_boolean ("show-frame",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
}



static void
systray_plugin_init (SystrayPlugin *plugin)
{
  plugin->manager = NULL;
  plugin->show_frame = FALSE;

  /* initialize xfconf */
  xfconf_init (NULL);

  /* show configure */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* plugin widgets */
  plugin->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->frame);
  gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), GTK_SHADOW_NONE);
  gtk_widget_show (plugin->frame);

  plugin->box = systray_box_new ();
  gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->box);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->box);
  gtk_widget_show (plugin->box);
}



static void
systray_plugin_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (object);
  gint           rows;

  switch (prop_id)
    {
      case PROP_ROWS:
        rows = systray_box_get_rows (XFCE_SYSTRAY_BOX (plugin->box));
        g_value_set_int (value, rows);
        break;

      case PROP_SHOW_FRAME:
        g_value_set_boolean (value, plugin->show_frame);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
systray_plugin_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (object);
  gint           rows;

  switch (prop_id)
    {
      case PROP_ROWS:
        rows = g_value_get_int (value);
        systray_box_set_rows (XFCE_SYSTRAY_BOX (plugin->box), rows);
        break;

      case PROP_SHOW_FRAME:
        plugin->show_frame = g_value_get_boolean (value);

        /* set the frame shadow */
        gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame),
            plugin->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
systray_plugin_screen_changed (GtkWidget *widget,
                               GdkScreen *previous_screen)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (widget);
  GdkScreen     *screen;
  GError        *error = NULL;

  if (G_UNLIKELY (plugin->manager != NULL))
    {
      /* unregister from this screen */
      systray_manager_unregister (plugin->manager);
      g_object_unref (G_OBJECT (plugin->manager));
      plugin->manager = NULL;
    }

  /* get the new screen */
  screen = gtk_widget_get_screen (widget);

  /* check if not another systray is running on this screen */
  if (G_LIKELY (systray_manager_check_running (screen) == FALSE))
    {
      /* create a new manager and register this screen */
      plugin->manager = systray_manager_new ();

      /* hookup signals */
      g_signal_connect (G_OBJECT (plugin->manager), "icon-added",
          G_CALLBACK (systray_plugin_icon_added), plugin);
      g_signal_connect (G_OBJECT (plugin->manager), "icon-removed",
          G_CALLBACK (systray_plugin_icon_removed), plugin);
      g_signal_connect (G_OBJECT (plugin->manager), "lost-selection",
          G_CALLBACK (systray_plugin_lost_selection), plugin);

      if (systray_manager_register (plugin->manager, screen, &error))
        {
          /* send the plugin orientation */
          systray_plugin_orientation_changed (XFCE_PANEL_PLUGIN (plugin),
              xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)));
        }
      else
        {
          /* TODO handle error and leave the plugin */
          g_message ("Failed to register the systray manager %s", error->message);
          g_error_free (error);
        }
    }
  else
    {
      /* TODO, error and leave the plugin */
      g_message ("already a notification area running");
    }
}



static void
systray_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  /* open the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);

  /* bind the properties */
  xfconf_g_property_bind (plugin->channel, "/rows",
                          G_TYPE_UINT, plugin, "rows");
  xfconf_g_property_bind (plugin->channel, "/show-frame",
                          G_TYPE_BOOLEAN, plugin, "show-frame");

  /* monitor screen changes */
  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (systray_plugin_screen_changed), NULL);

  /* initialize the screen */
  systray_plugin_screen_changed (GTK_WIDGET (plugin), NULL);
}



static void
systray_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
      systray_plugin_screen_changed, NULL);

  /* release the manager */
  if (G_LIKELY (plugin->manager))
    g_object_unref (G_OBJECT (plugin->manager));

  /* release the xfconf channel */
  if (G_LIKELY (plugin->channel))
    g_object_unref (G_OBJECT (plugin->channel));

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static void
systray_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                        gint             screen_position)
{
  SystrayPlugin      *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);
  XfceScreenPosition  position;
  GdkScreen          *screen;
  GdkRectangle        geom;
  gint                mon, x, y;
  GtkArrowType        arrow_type;

  panel_return_if_fail (GTK_WIDGET_REALIZED (panel_plugin));

  /* get the plugin position */
  position = xfce_panel_plugin_get_screen_position (panel_plugin);

  /* get the button position */
  switch (position)
    {
      /*    horizontal west */
      case XFCE_SCREEN_POSITION_NW_H:
      case XFCE_SCREEN_POSITION_SW_H:
        arrow_type = GTK_ARROW_RIGHT;
        break;

      /* horizontal east */
      case XFCE_SCREEN_POSITION_N:
      case XFCE_SCREEN_POSITION_NE_H:
      case XFCE_SCREEN_POSITION_S:
      case XFCE_SCREEN_POSITION_SE_H:
        arrow_type = GTK_ARROW_LEFT;
        break;

      /* vertical north */
      case XFCE_SCREEN_POSITION_NW_V:
      case XFCE_SCREEN_POSITION_NE_V:
        arrow_type = GTK_ARROW_DOWN;
        break;

      /* vertical south */
      case XFCE_SCREEN_POSITION_W:
      case XFCE_SCREEN_POSITION_SW_V:
      case XFCE_SCREEN_POSITION_E:
      case XFCE_SCREEN_POSITION_SE_V:
        arrow_type = GTK_ARROW_UP;
        break;

      /* floating */
      default:
        /* get the screen information */
        screen = gtk_widget_get_screen (GTK_WIDGET (panel_plugin));
        mon = gdk_screen_get_monitor_at_window (screen, GTK_WIDGET (panel_plugin)->window);
        gdk_screen_get_monitor_geometry (screen, mon, &geom);
        gdk_window_get_root_origin (GTK_WIDGET (panel_plugin)->window, &x, &y);

        /* get the position based on the screen position */
        if (position == XFCE_SCREEN_POSITION_FLOATING_H)
            arrow_type = ((x < (geom.x + geom.width / 2)) ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
        else
            arrow_type = ((y < (geom.y + geom.height / 2)) ? GTK_ARROW_DOWN : GTK_ARROW_UP);
        break;
    }

  /* set the arrow type of the tray widget */
  systray_box_set_arrow_type (XFCE_SYSTRAY_BOX (plugin->box), arrow_type);

  /* update the manager orientation */
  systray_plugin_orientation_changed (panel_plugin,
      xfce_panel_plugin_get_orientation (panel_plugin));
}



static void
systray_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                    GtkOrientation   orientation)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  /* send the new orientation to the manager */
  if (G_LIKELY (plugin->manager != NULL))
    systray_manager_set_orientation (plugin->manager, orientation);
}



static gboolean
systray_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                             gint             size)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  /* set border sizes */
  gtk_container_set_border_width (GTK_CONTAINER (plugin->frame),
    (size > 26 && plugin->show_frame) ? 1 : 0);

  return TRUE;
}



static void
systray_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);
  GtkBuilder    *builder;
  GObject       *dialog, *object;

  /* fix gtk builder problem: "Invalid object type XfceTitledDialog" */
  if (xfce_titled_dialog_get_type () == 0)
    return;

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, systray_dialog_glade,
                                   systray_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify)
                         xfce_panel_plugin_unblock_menu, panel_plugin);

      object = gtk_builder_get_object (builder, "close-button");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      g_signal_connect_swapped (G_OBJECT (object), "clicked",
                                G_CALLBACK (gtk_widget_destroy), dialog);

      object = gtk_builder_get_object (builder, "rows");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (plugin), "rows",
                              G_OBJECT (object), "value");

      object = gtk_builder_get_object (builder, "show-frame");
      panel_return_if_fail (GTK_IS_WIDGET (object));
      exo_mutual_binding_new (G_OBJECT (plugin), "show-frame",
                              G_OBJECT (object), "active");

      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}



static void
systray_plugin_icon_added (SystrayManager *manager,
                           GtkWidget      *icon,
                           SystrayPlugin  *plugin)
{
  gchar *name;

  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (plugin->manager == manager);
  panel_return_if_fail (GTK_IS_WIDGET (icon));

  /* get the application name */
  name = systray_manager_get_application_name (icon);

  /* add the icon to the widget */
  systray_box_add_with_name (XFCE_SYSTRAY_BOX (plugin->box), icon, name);

  /* cleanup */
  g_free (name);

  /* show icon */
  gtk_widget_show (icon);
}



static void
systray_plugin_icon_removed (SystrayManager *manager,
                             GtkWidget      *icon,
                             SystrayPlugin  *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (plugin->manager == manager);
  panel_return_if_fail (GTK_IS_WIDGET (icon));

  /* remove the icon from the box */
  gtk_container_remove (GTK_CONTAINER (plugin->box), icon);
}



static void
systray_plugin_lost_selection (SystrayManager *manager,
                               SystrayPlugin  *plugin)
{
  GError error;

  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (plugin->manager == manager);

  /* create fake error and show it */
  error.message = _("Most likely another widget took over the function "
                    "of a notification area. This plugin will close.");
  xfce_dialog_show_error (gtk_widget_get_screen (GTK_WIDGET (plugin)), &error,
                          _("The notification area lost selection"));
}
