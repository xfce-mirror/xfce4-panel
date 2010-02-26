/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
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
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <common/panel-private.h>
#include <libwnck/libwnck.h>
#include <exo/exo.h>

#include "pager.h"
#include "pager-dialog_ui.h"



#define WORKSPACE_SETTINGS_COMMAND "xfwm4-workspace-settings"



static void     pager_plugin_get_property              (GObject           *object,
                                                        guint              prop_id,
                                                        GValue            *value,
                                                        GParamSpec        *pspec);
static void     pager_plugin_set_property              (GObject           *object,
                                                        guint              prop_id,
                                                        const GValue      *value,
                                                        GParamSpec        *pspec);
static gboolean pager_plugin_scroll_event              (GtkWidget         *widget,
                                                        GdkEventScroll    *event);
static void     pager_plugin_screen_changed            (GtkWidget         *widget,
                                                        GdkScreen         *previous_screen);
static void     pager_plugin_construct                 (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_free_data                 (XfcePanelPlugin   *panel_plugin);
static gboolean pager_plugin_size_changed              (XfcePanelPlugin   *panel_plugin,
                                                        gint               size);
static void     pager_plugin_configure_plugin          (XfcePanelPlugin   *panel_plugin);



struct _PagerPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _PagerPlugin
{
  XfcePanelPlugin __parent__;

  GtkWidget     *wnck_pager;

  WnckScreen    *wnck_screen;

  /* settings */
  guint          scrolling : 1;
  guint          show_names : 1;
  gint           rows;
};

enum
{
  PROP_0,
  PROP_WORKSPACE_SCROLLING,
  PROP_SHOW_NAMES,
  PROP_ROWS
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN_RESIDENT (PagerPlugin, pager_plugin)



static void
pager_plugin_class_init (PagerPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;
  GtkWidgetClass       *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = pager_plugin_get_property;
  gobject_class->set_property = pager_plugin_set_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->scroll_event = pager_plugin_scroll_event;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = pager_plugin_construct;
  plugin_class->free_data = pager_plugin_free_data;
  plugin_class->size_changed = pager_plugin_size_changed;
  plugin_class->configure_plugin = pager_plugin_configure_plugin;

  g_object_class_install_property (gobject_class,
                                   PROP_WORKSPACE_SCROLLING,
                                   g_param_spec_boolean ("workspace-scrolling",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_NAMES,
                                   g_param_spec_boolean ("show-names",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ROWS,
                                   g_param_spec_uint ("rows",
                                                      NULL, NULL,
                                                      1, 50, 1,
                                                      EXO_PARAM_READWRITE));


}



static void
pager_plugin_init (PagerPlugin *plugin)
{
  /* init, draw nothing */
  plugin->wnck_screen = NULL;
  plugin->scrolling = TRUE;
  plugin->show_names = FALSE;
  plugin->rows = 1;
  plugin->wnck_pager = NULL;
}



static void
pager_plugin_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_WORKSPACE_SCROLLING:
      g_value_set_boolean (value, plugin->scrolling);
      break;

    case PROP_SHOW_NAMES:
      g_value_set_boolean (value, plugin->show_names);
      break;

    case PROP_ROWS:
      g_value_set_uint (value, plugin->rows);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
pager_plugin_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_WORKSPACE_SCROLLING:
      plugin->scrolling = g_value_get_boolean (value);
      break;

    case PROP_SHOW_NAMES:
      plugin->show_names = g_value_get_boolean (value);

      if (plugin->wnck_pager != NULL)
        wnck_pager_set_display_mode (WNCK_PAGER (plugin->wnck_pager),
                                     plugin->show_names ?
                                         WNCK_PAGER_DISPLAY_NAME :
                                         WNCK_PAGER_DISPLAY_CONTENT);
      break;

    case PROP_ROWS:
      plugin->rows = g_value_get_uint (value);

      if (plugin->wnck_pager != NULL)
        wnck_pager_set_n_rows (WNCK_PAGER (plugin->wnck_pager), plugin->rows);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
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
    case GDK_SCROLL_UP:
      direction = WNCK_MOTION_UP;
      break;

    case GDK_SCROLL_DOWN:
      direction = WNCK_MOTION_DOWN;
      break;

    case GDK_SCROLL_LEFT:
      direction = WNCK_MOTION_LEFT;
      break;

    default:
      direction = WNCK_MOTION_RIGHT;
      break;
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
pager_plugin_screen_layout_changed (PagerPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PAGER_PLUGIN (plugin));
  panel_return_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen));

  if (G_UNLIKELY (plugin->wnck_pager != NULL))
    {
      /* destroy the existing pager */
      gtk_widget_destroy (GTK_WIDGET (plugin->wnck_pager));

      /* force a screen update */
      wnck_screen_force_update (plugin->wnck_screen);
    }

  /* create the wnck pager */
  plugin->wnck_pager = wnck_pager_new (plugin->wnck_screen);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->wnck_pager);
  gtk_widget_show (plugin->wnck_pager);

  /* set the pager properties */
  wnck_pager_set_display_mode (WNCK_PAGER (plugin->wnck_pager),
                               plugin->show_names ?
                                   WNCK_PAGER_DISPLAY_NAME :
                                   WNCK_PAGER_DISPLAY_CONTENT);
  wnck_pager_set_n_rows (WNCK_PAGER (plugin->wnck_pager), plugin->rows);
}



static void
pager_plugin_screen_changed (GtkWidget *widget,
                             GdkScreen *previous_screen)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (widget);
  GdkScreen   *screen;
  WnckScreen  *wnck_screen;

  /* get the active screen */
  screen = gtk_widget_get_screen (widget);
  wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));

  /* only update if the screen changed */
  if (plugin->wnck_screen != wnck_screen)
    {
      /* set the new screen */
      plugin->wnck_screen = wnck_screen;

      /* rebuild the pager */
      pager_plugin_screen_layout_changed (plugin);

      /* watch the screen for changes */
      g_signal_connect_swapped (G_OBJECT (screen), "monitors-changed",
         G_CALLBACK (pager_plugin_screen_layout_changed), plugin);
      g_signal_connect_swapped (G_OBJECT (screen), "size-changed",
         G_CALLBACK (pager_plugin_screen_layout_changed), plugin);
    }
}



static void
pager_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "workspace-scrolling", G_TYPE_BOOLEAN },
    { "show-names", G_TYPE_BOOLEAN },
    { "rows", G_TYPE_UINT },
    { NULL }
  };

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* bind all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* create the pager */
  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (pager_plugin_screen_changed), NULL);
  pager_plugin_screen_changed (GTK_WIDGET (plugin), NULL);
}



static void
pager_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
      pager_plugin_screen_changed, NULL);
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
pager_plugin_configure_workspace_settings (GtkWidget *button)
{
  GdkScreen *screen;
  GError    *error = NULL;
  GtkWidget *toplevel;

  panel_return_if_fail (GTK_IS_WIDGET (button));

  /* get the screen */
  screen = gtk_widget_get_screen (button);
  if (G_UNLIKELY (screen == NULL))
    screen = gdk_screen_get_default ();

  /* try to start the settings dialog */
  if (!gdk_spawn_command_line_on_screen (screen, WORKSPACE_SETTINGS_COMMAND, &error))
    {
      /* show an error dialog */
      toplevel = gtk_widget_get_toplevel (button);
      xfce_dialog_show_error (GTK_WINDOW (toplevel), error,
          _("Unable to open the Xfce workspace settings"));
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
}



static void
pager_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);
  GtkBuilder  *builder;
  GObject     *dialog, *object;
  gchar       *path;

  panel_return_if_fail (XFCE_IS_PAGER_PLUGIN (plugin));

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, pager_dialog_ui,
                                     pager_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  /* signals to monitor number of workspace changes */
  g_signal_connect (G_OBJECT (plugin->wnck_screen), "workspace-created",
      G_CALLBACK (pager_plugin_configure_n_workspaces_changed), builder);
  g_signal_connect (G_OBJECT (plugin->wnck_screen), "workspace-destroyed",
      G_CALLBACK (pager_plugin_configure_n_workspaces_changed), builder);
  g_object_weak_ref (G_OBJECT (builder), pager_plugin_configure_destroyed, plugin);

  object = gtk_builder_get_object (builder, "settings-button");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  g_signal_connect (G_OBJECT (object), "clicked",
      G_CALLBACK (pager_plugin_configure_workspace_settings), dialog);

  /* don't show button if xfwm4 is not installed */
  path = g_find_program_in_path (WORKSPACE_SETTINGS_COMMAND);
  g_object_set (G_OBJECT (object), "visible", path != NULL, NULL);
  g_free (path);

  object = gtk_builder_get_object (builder, "workspace-scrolling");
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "workspace-scrolling",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "show-names");
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "show-names",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "rows");
  panel_return_if_fail (GTK_IS_ADJUSTMENT (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "rows",
                          G_OBJECT (object), "value");

  /* update the rows limit */
  pager_plugin_configure_n_workspaces_changed (plugin->wnck_screen, NULL, builder);

  gtk_widget_show (GTK_WIDGET (dialog));
}
