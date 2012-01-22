/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2010 Nick Schermer <nick@xfce.org>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
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
#include "pager-buttons.h"
#include "pager-dialog_ui.h"



#define WORKSPACE_SETTINGS_COMMAND "xfwm4-workspace-settings"



static void     pager_plugin_get_property                 (GObject           *object,
                                                           guint              prop_id,
                                                           GValue            *value,
                                                           GParamSpec        *pspec);
static void     pager_plugin_set_property                 (GObject           *object,
                                                           guint              prop_id,
                                                           const GValue      *value,
                                                           GParamSpec        *pspec);
static gboolean pager_plugin_scroll_event                 (GtkWidget         *widget,
                                                           GdkEventScroll    *event);
static void     pager_plugin_screen_changed               (GtkWidget         *widget,
                                                           GdkScreen         *previous_screen);
static void     pager_plugin_construct                    (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_free_data                    (XfcePanelPlugin   *panel_plugin);
static gboolean pager_plugin_size_changed                 (XfcePanelPlugin   *panel_plugin,
                                                           gint               size);
static void     pager_plugin_mode_changed                 (XfcePanelPlugin     *panel_plugin,
                                                           XfcePanelPluginMode  mode);
static void     pager_plugin_configure_workspace_settings (GtkWidget         *button);
static void     pager_plugin_configure_plugin             (XfcePanelPlugin   *panel_plugin);
static void     pager_plugin_screen_layout_changed        (PagerPlugin       *plugin);
static void     pager_plugin_size_request                 (GtkWidget         *widget,
                                                           GtkRequisition    *requisition);
static void     pager_plugin_size_allocate                (GtkWidget         *widget,
                                                           GtkAllocation     *allocation);



struct _PagerPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _PagerPlugin
{
  XfcePanelPlugin __parent__;

  GtkWidget     *pager;

  WnckScreen    *wnck_screen;

  /* settings */
  guint          scrolling : 1;
  guint          miniature_view : 1;
  gint           rows;

  /* panel size */
  gint           size;

  /* WNCK pager aspect ratio */
  gdouble        aspect;
};

enum
{
  PROP_0,
  PROP_WORKSPACE_SCROLLING,
  PROP_MINIATURE_VIEW,
  PROP_ROWS
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN_RESIDENT (PagerPlugin, pager_plugin,
    pager_buttons_register_type)



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
  widget_class->size_request = pager_plugin_size_request;
  widget_class->size_allocate = pager_plugin_size_allocate;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = pager_plugin_construct;
  plugin_class->free_data = pager_plugin_free_data;
  plugin_class->size_changed = pager_plugin_size_changed;
  plugin_class->mode_changed = pager_plugin_mode_changed;
  plugin_class->configure_plugin = pager_plugin_configure_plugin;

  g_object_class_install_property (gobject_class,
                                   PROP_WORKSPACE_SCROLLING,
                                   g_param_spec_boolean ("workspace-scrolling",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_MINIATURE_VIEW,
                                   g_param_spec_boolean ("miniature-view",
                                                         NULL, NULL,
                                                         TRUE,
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
  plugin->wnck_screen = NULL;
  plugin->scrolling = TRUE;
  plugin->miniature_view = TRUE;
  plugin->rows = 1;
  plugin->pager = NULL;
  plugin->size = 0;
  plugin->aspect = 1.;
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

    case PROP_MINIATURE_VIEW:
      g_value_set_boolean (value, plugin->miniature_view);

      pager_plugin_screen_layout_changed (plugin);
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

    case PROP_MINIATURE_VIEW:
      plugin->miniature_view = g_value_get_boolean (value);
      break;

    case PROP_ROWS:
      plugin->rows = g_value_get_uint (value);

      if (plugin->pager != NULL)
        {
          if (plugin->miniature_view)
            {
              if (!wnck_pager_set_n_rows (WNCK_PAGER (plugin->pager), plugin->rows))
                g_message ("Failed to set the number of pager rows. You probably "
                           "have more than 1 pager in your panel setup.");
            }
          else
            pager_buttons_set_n_rows (XFCE_PAGER_BUTTONS (plugin->pager), plugin->rows);
        }
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
  PagerPlugin   *plugin = XFCE_PAGER_PLUGIN (widget);
  WnckWorkspace *active_ws;
  WnckWorkspace *new_ws;
  gint           active_n;
  gint           n_workspaces;

  panel_return_val_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen), FALSE);

  /* leave when scrolling is not enabled */
  if (plugin->scrolling == FALSE)
    return TRUE;

  active_ws = wnck_screen_get_active_workspace (plugin->wnck_screen);
  active_n = wnck_workspace_get_number (active_ws);

  if (event->direction == GDK_SCROLL_UP
      || event->direction == GDK_SCROLL_LEFT)
    active_n--;
  else
    active_n++;

  /* wrap around */
  n_workspaces = wnck_screen_get_workspace_count (plugin->wnck_screen) - 1;
  if (active_n < 0)
    active_n = n_workspaces;
  else if (active_n > n_workspaces)
    active_n = 0;

  new_ws = wnck_screen_get_workspace (plugin->wnck_screen, active_n);
  if (new_ws != NULL && active_ws != new_ws)
    wnck_workspace_activate (new_ws, event->time);

  return TRUE;
}



static void
pager_plugin_screen_layout_changed (PagerPlugin *plugin)
{
  GtkOrientation orientation;

  panel_return_if_fail (XFCE_IS_PAGER_PLUGIN (plugin));
  panel_return_if_fail (WNCK_IS_SCREEN (plugin->wnck_screen));

  if (G_UNLIKELY (plugin->pager != NULL))
    {
      gtk_widget_destroy (GTK_WIDGET (plugin->pager));
      wnck_screen_force_update (plugin->wnck_screen);
    }

  orientation =
    (xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (plugin)) != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
    GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

  if (plugin->miniature_view)
    {
      plugin->pager = wnck_pager_new (plugin->wnck_screen);
      wnck_pager_set_display_mode (WNCK_PAGER (plugin->pager), WNCK_PAGER_DISPLAY_CONTENT);
      if (!wnck_pager_set_n_rows (WNCK_PAGER (plugin->pager), plugin->rows))
        g_message ("Setting the pager rows returned false. Maybe the setting is not applied.");
      wnck_pager_set_orientation (WNCK_PAGER (plugin->pager), orientation);
    }
  else
    {
      plugin->pager = pager_buttons_new (plugin->wnck_screen);
      pager_buttons_set_n_rows (XFCE_PAGER_BUTTONS (plugin->pager), plugin->rows);
      pager_buttons_set_orientation (XFCE_PAGER_BUTTONS (plugin->pager), orientation);
    }

  gtk_container_add (GTK_CONTAINER (plugin), plugin->pager);
  gtk_widget_show (plugin->pager);
}



static void
pager_plugin_screen_changed (GtkWidget *widget,
                             GdkScreen *previous_screen)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (widget);
  GdkScreen   *screen;
  WnckScreen  *wnck_screen;

  screen = gtk_widget_get_screen (widget);
  wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));

  if (plugin->wnck_screen != wnck_screen)
    {
      plugin->wnck_screen = wnck_screen;

      pager_plugin_screen_layout_changed (plugin);

      g_signal_connect_swapped (G_OBJECT (screen), "monitors-changed",
         G_CALLBACK (pager_plugin_screen_layout_changed), plugin);
      g_signal_connect_swapped (G_OBJECT (screen), "size-changed",
         G_CALLBACK (pager_plugin_screen_layout_changed), plugin);
    }
}



static void
pager_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin         *plugin = XFCE_PAGER_PLUGIN (panel_plugin);
  GtkWidget           *mi, *image;
  const PanelProperty  properties[] =
  {
    { "workspace-scrolling", G_TYPE_BOOLEAN },
    { "miniature-view", G_TYPE_BOOLEAN },
    { "rows", G_TYPE_UINT },
    { NULL }
  };

  xfce_panel_plugin_menu_show_configure (panel_plugin);

  mi = gtk_image_menu_item_new_with_mnemonic (_("Workspace _Settings..."));
  xfce_panel_plugin_menu_insert_item (panel_plugin, GTK_MENU_ITEM (mi));
  g_signal_connect (G_OBJECT (mi), "activate",
      G_CALLBACK (pager_plugin_configure_workspace_settings), NULL);
  gtk_widget_show (mi);

  image = gtk_image_new_from_icon_name ("xfce4-workspaces", GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
  gtk_widget_show (image);

  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (pager_plugin_screen_changed), NULL);
  pager_plugin_screen_changed (GTK_WIDGET (plugin), NULL);
}



static void
pager_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
      pager_plugin_screen_changed, NULL);
}



static void
pager_plugin_size_request (GtkWidget      *widget,
                           GtkRequisition *requisition)
{
  PagerPlugin        *plugin = XFCE_PAGER_PLUGIN (widget);
  XfcePanelPlugin    *panel_plugin = XFCE_PANEL_PLUGIN (widget);
  gdouble             aspect = 1.0;

  /* propagate the size request as normal */
  gtk_widget_size_request (GTK_WIDGET (plugin->pager), requisition);

  /* if in deskbar mode, memorize the aspect ratio of the WNCK pager     */
  /* WNCK pager assumes that orientation of the panel is always same     */
  /* as orientation of the pager, which is not true in the deskbar mode. */
  if (plugin->miniature_view &&
      xfce_panel_plugin_get_mode (panel_plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
      if (requisition->width > 1)
        aspect = (gdouble) requisition->height / (gdouble) requisition->width;

      /* Update the memorized aspect value only if change is big enough */
      /* WNCK pager requests resizing if the allocated vertical dimension */
      /* is different from the previously allocated vertical dimension. */
      /* Therefore, we only want to change the aspect ratio (and allocation) */
      /* when it corresponds to a "real" aspect ratio change. */
      /* The threshold is arbitrary - small value may cause infinite looping, */
      /* large value reduces accuracy of pager scaling. */
      if (fabs ((plugin->aspect - aspect) * plugin->size) > 3.0)
        plugin->aspect = aspect;

      requisition->width = plugin->size;
      requisition->height = rint (requisition->width * plugin->aspect);
    }
}



static void
pager_plugin_size_allocate (GtkWidget      *widget,
                            GtkAllocation  *allocation)
{
  PagerPlugin        *plugin = XFCE_PAGER_PLUGIN (widget);
  XfcePanelPlugin    *panel_plugin = XFCE_PANEL_PLUGIN (widget);

  /* if in deskbar mode, force the allocation based on memorized aspect ratio. */
  /* WNCK pager will request resizing if it differs from previous allocation.  */
  if (plugin->miniature_view &&
      xfce_panel_plugin_get_mode (panel_plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
      allocation->width = plugin->size;
      allocation->height = rint (allocation->width * plugin->aspect);
    }
  gtk_widget_size_allocate (plugin->pager, allocation);
}



static gboolean
pager_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                           gint             size)
{
  PagerPlugin        *plugin = XFCE_PAGER_PLUGIN (panel_plugin);

  if (plugin->size != size)
    {
      plugin->size = size;

      /* request resize when using WNCK pager */
      /* (its height depends on width)        */
      if (plugin->miniature_view &&
          xfce_panel_plugin_get_mode (panel_plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        {
          gtk_widget_queue_resize (GTK_WIDGET (panel_plugin));
        }
    }

  /* do not set fixed size */
  return TRUE;
}



static void
pager_plugin_mode_changed (XfcePanelPlugin     *panel_plugin,
                           XfcePanelPluginMode  mode)
{
  PagerPlugin       *plugin = XFCE_PAGER_PLUGIN (panel_plugin);
  GtkOrientation     orientation;

  orientation =
    (mode != XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
    GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

  if (plugin->miniature_view)
    wnck_pager_set_orientation (WNCK_PAGER (plugin->pager), orientation);
  else
    pager_buttons_set_orientation (XFCE_PAGER_BUTTONS (plugin->pager), orientation);
}



static void
pager_plugin_configure_workspace_settings (GtkWidget *button)
{
  GdkScreen *screen;
  GError    *error = NULL;
  GtkWidget *toplevel;

  panel_return_if_fail (GTK_IS_WIDGET (button));

  screen = gtk_widget_get_screen (button);
  if (G_UNLIKELY (screen == NULL))
    screen = gdk_screen_get_default ();

  /* try to start the settings dialog */
  if (!xfce_spawn_command_line_on_screen (screen, WORKSPACE_SETTINGS_COMMAND,
                                          FALSE, FALSE, &error))
    {
      /* show an error dialog */
      toplevel = gtk_widget_get_toplevel (button);
      xfce_dialog_show_error (GTK_WINDOW (toplevel), error,
          _("Unable to open the workspace settings"));
      g_error_free (error);
    }
}



static void
pager_plugin_configure_n_workspaces_changed (WnckScreen    *wnck_screen,
                                             WnckWorkspace *workspace,
                                             GtkBuilder    *builder)
{
  GObject       *object;
  gdouble        upper, value;
  WnckWorkspace *active_ws;

  panel_return_if_fail (WNCK_IS_SCREEN (wnck_screen));
  panel_return_if_fail (GTK_IS_BUILDER (builder));

  object = gtk_builder_get_object (builder, "rows");

  upper = wnck_screen_get_workspace_count (wnck_screen);
  if (upper == 1)
    {
      /* check if we ware in viewport mode */
      active_ws = wnck_screen_get_active_workspace (wnck_screen);
      if (wnck_workspace_is_virtual (active_ws))
        {
          /* number of rows * number of columns */
          upper = (wnck_workspace_get_width (active_ws) / wnck_screen_get_width (wnck_screen))
                  * (wnck_workspace_get_height (active_ws) / wnck_screen_get_height (wnck_screen));
        }
    }

  value = MIN (gtk_adjustment_get_value (GTK_ADJUSTMENT (object)), upper);

  g_object_set (G_OBJECT (object), "upper", upper, "value", value, NULL);
}



static void
pager_plugin_configure_destroyed (gpointer  data,
                                  GObject  *where_the_object_was)
{
  PagerPlugin *plugin = XFCE_PAGER_PLUGIN (data);

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

  object = gtk_builder_get_object (builder, "workspace-scrolling");
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "workspace-scrolling",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "miniature-view");
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "miniature-view",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "rows");
  panel_return_if_fail (GTK_IS_ADJUSTMENT (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "rows",
                          G_OBJECT (object), "value");

  /* update the rows limit */
  pager_plugin_configure_n_workspaces_changed (plugin->wnck_screen, NULL, builder);

  gtk_widget_show (GTK_WIDGET (dialog));
}
