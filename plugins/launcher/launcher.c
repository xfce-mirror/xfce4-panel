/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>

#include "launcher.h"
#include "launcher-dialog.h"

#define ARROW_BUTTON_SIZE              (12)
#define DEFAULT_MENU_ICON_SIZE         (24)
#define MENU_POPUP_DELAY               (225)
#define NO_ARROW_INSIDE_BUTTON(plugin) ((plugin)->arrow_position != LAUNCHER_ARROW_INTERNAL \
                                        || LIST_HAS_ONE_OR_NO_ENTRIES ((plugin)->items))
#define ARROW_INSIDE_BUTTON(plugin)    (!NO_ARROW_INSIDE_BUTTON (plugin))


static void launcher_plugin_style_set (GtkWidget *widget, GtkStyle *previous_style);

static void launcher_plugin_construct (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_free_data (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_orientation_changed (XfcePanelPlugin *panel_plugin, GtkOrientation orientation);
static gboolean launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin, gint size);
static void launcher_plugin_save (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin, gint position);

static void launcher_plugin_property_changed (XfconfChannel *channel, const gchar *property_name, const GValue *value, XfceLauncherPlugin *plugin);
static void launcher_plugin_icon_theme_changed (GtkIconTheme *icon_theme, XfceLauncherPlugin *plugin);

static void launcher_plugin_pack_widgets (XfceLauncherPlugin *plugin);

static void launcher_plugin_menu_deactivate (GtkWidget *menu, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_menu_item_released (GtkMenuItem *widget, GdkEventButton *event, XfceMenuItem *item);
static void launcher_plugin_menu_item_drag_data_received (GtkWidget *widget,GdkDragContext *context, gint x,gint y,GtkSelectionData *data, guint info, guint drag_time, XfceMenuItem *item);
static void launcher_plugin_menu_construct (XfceLauncherPlugin *plugin);
static void launcher_plugin_menu_popup_destroyed (gpointer user_data);
static gboolean launcher_plugin_menu_popup (gpointer user_data);
static void launcher_plugin_menu_destroy (XfceLauncherPlugin *plugin);

static void launcher_plugin_button_set_icon (XfceLauncherPlugin *plugin);
static void launcher_plugin_button_state_changed (GtkWidget *button_a, GtkStateType state, GtkWidget *button_b);
static gboolean launcher_plugin_button_press_event (GtkWidget *button, GdkEventButton *event, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_button_release_event (GtkWidget *button, GdkEventButton *event, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_button_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, XfceLauncherPlugin *plugin);
static void launcher_plugin_button_drag_data_received (GtkWidget *widget,GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint drag_time, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_button_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint drag_time, XfceLauncherPlugin *plugin);
static void launcher_plugin_button_drag_leave (GtkWidget *widget, GdkDragContext *context, guint drag_time, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_button_expose_event (GtkWidget *widget, GdkEventExpose *event, XfceLauncherPlugin *launcher);

static void launcher_plugin_arrow_visibility (XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_arrow_press_event (GtkWidget *button, GdkEventButton *event, XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_arrow_drag_motion(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint drag_time, XfceLauncherPlugin *plugin);
static void launcher_plugin_arrow_drag_leave (GtkWidget *widget, GdkDragContext *context, guint drag_time, XfceLauncherPlugin *plugin);

static void launcher_plugin_items_load (XfceLauncherPlugin *plugin);
static gboolean launcher_plugin_item_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, XfceMenuItem *item);

static gboolean launcher_plugin_item_exec_on_screen (XfceMenuItem *item, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_item_exec (XfceMenuItem *item, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_item_exec_from_clipboard (XfceMenuItem *item, guint32 event_time, GdkScreen *screen);
static void launcher_plugin_exec_append_quoted (GString *string, const gchar *unquoted);
static gboolean launcher_plugin_exec_parse (XfceMenuItem *item, GSList *uri_list, gint *argc, gchar ***argv, GError **error);
static GSList *launcher_plugin_uri_list_extract (GtkSelectionData *data);
static void launcher_plugin_uri_list_free (GSList *uri_list);



struct _XfceLauncherPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _XfceLauncherPlugin
{
  XfcePanelPlugin __parent__;

  XfconfChannel     *channel;

  GtkWidget         *box;
  GtkWidget         *button;
  GtkWidget         *arrow;
  GtkWidget         *image;
  GtkWidget         *menu;

  GSList            *items;

  guint              menu_timeout_id;
  gint               menu_icon_size;

  guint              disable_tooltips : 1;
  guint              move_first : 1;
  LauncherArrowType  arrow_position;
};



G_DEFINE_TYPE (XfceLauncherPlugin, launcher_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_LAUNCHER_PLUGIN);



/* quark to attach the plugin to menu items */
GQuark launcher_plugin_quark = 0;



/* target types for dropping in the launcher plugin */
static const GtkTargetEntry drop_targets[] =
{
    { (gchar *) "text/uri-list", 0, 0, },
    { (gchar *) "STRING", 0, 0 },
    { (gchar *) "UTF8_STRING", 0, 0 },
    { (gchar *) "text/plain", 0, 0 },
};



static void
launcher_plugin_class_init (XfceLauncherPluginClass *klass)
{
  GtkWidgetClass       *gtkwidget_class;
  XfcePanelPluginClass *plugin_class;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->style_set = launcher_plugin_style_set;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = launcher_plugin_construct;
  plugin_class->free_data = launcher_plugin_free_data;
  plugin_class->orientation_changed = launcher_plugin_orientation_changed;
  plugin_class->size_changed = launcher_plugin_size_changed;
  plugin_class->save = launcher_plugin_save;
  plugin_class->configure_plugin = launcher_plugin_configure_plugin;
  plugin_class->screen_position_changed = launcher_plugin_screen_position_changed;

  gtk_widget_class_install_style_property (gtkwidget_class,
                                           g_param_spec_int ("menu-icon-size",
                                                             NULL,
                                                             "Icon size (pixels) used in the menu",
                                                             16, 128,
                                                             DEFAULT_MENU_ICON_SIZE,
                                                             EXO_PARAM_READABLE));

  /* initialize the quark */
  launcher_plugin_quark = g_quark_from_static_string ("xfce-launcher-plugin");
}



static void
launcher_plugin_init (XfceLauncherPlugin *plugin)
{
  GtkIconTheme *icon_theme;

  plugin->disable_tooltips = FALSE;
  plugin->move_first = FALSE;
  plugin->arrow_position = LAUNCHER_ARROW_DEFAULT;
  plugin->menu = NULL;
  plugin->menu_timeout_id = 0;
  plugin->menu_icon_size = DEFAULT_MENU_ICON_SIZE;

  /* initialize xfconf */
  xfconf_init (NULL);

  /* show the configure menu item */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* monitor the default icon theme for changes */
  icon_theme = gtk_icon_theme_get_default ();
  if (G_LIKELY (icon_theme != NULL))
    g_signal_connect (G_OBJECT (icon_theme), "changed",
                      G_CALLBACK (launcher_plugin_icon_theme_changed), plugin);

  /* create the panel widgets */
  plugin->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->box);
  gtk_widget_show (plugin->box);

  plugin->button = xfce_panel_create_button ();
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->button, TRUE, TRUE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
  gtk_widget_set_has_tooltip (plugin->button, TRUE);
  gtk_widget_show (plugin->button);
  gtk_drag_dest_set (plugin->button, GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (plugin->button), "button-press-event",
                    G_CALLBACK (launcher_plugin_button_press_event), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "button-release-event",
                    G_CALLBACK (launcher_plugin_button_release_event), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "query-tooltip",
                    G_CALLBACK (launcher_plugin_button_query_tooltip), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "drag-data-received",
                    G_CALLBACK (launcher_plugin_button_drag_data_received), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "drag-motion",
                    G_CALLBACK (launcher_plugin_button_drag_motion), plugin);
  g_signal_connect (G_OBJECT (plugin->button), "drag-leave",
                    G_CALLBACK (launcher_plugin_button_drag_leave), plugin);
  g_signal_connect_after (G_OBJECT (plugin->button), "expose-event",
                          G_CALLBACK (launcher_plugin_button_expose_event), plugin);

  plugin->image = xfce_scaled_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_widget_show (plugin->image);

  plugin->arrow = xfce_arrow_button_new (GTK_ARROW_UP);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->arrow, FALSE, FALSE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->arrow);
  gtk_button_set_relief (GTK_BUTTON (plugin->arrow), GTK_RELIEF_NONE);
  gtk_drag_dest_set (plugin->arrow, GTK_DEST_DEFAULT_MOTION,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (plugin->arrow), "button-press-event",
                    G_CALLBACK (launcher_plugin_arrow_press_event), plugin);
  g_signal_connect (G_OBJECT (plugin->arrow), "drag-motion",
                    G_CALLBACK (launcher_plugin_arrow_drag_motion), plugin);
  g_signal_connect (G_OBJECT (plugin->arrow), "drag-leave",
                    G_CALLBACK (launcher_plugin_arrow_drag_leave), plugin);

  /* sync button states */
  g_signal_connect (G_OBJECT (plugin->button), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed),
                    plugin->arrow);
  g_signal_connect (G_OBJECT (plugin->arrow), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed),
                    plugin->button);
}



static void
launcher_plugin_style_set (GtkWidget *widget,
                           GtkStyle  *previous_style)
{
  XfceLauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (widget);
  gint                menu_icon_size;

  /* let gtk update the widget style */
  (*GTK_WIDGET_CLASS (launcher_plugin_parent_class)->style_set) (widget, previous_style);

  /* read the style properties */
  gtk_widget_style_get (widget,
                        "menu-icon-size", &menu_icon_size,
                        NULL);

  if (plugin->menu_icon_size != menu_icon_size)
    {
      plugin->menu_icon_size = menu_icon_size;
      launcher_plugin_menu_destroy (plugin);
    }
}



static void
launcher_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  XfceLauncherPlugin  *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  const gchar * const *filenames;
  guint                i;
  XfceMenuItem        *item;
  guint                value;

  /* open the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);
  g_signal_connect (G_OBJECT (plugin->channel), "property-changed",
                    G_CALLBACK (launcher_plugin_property_changed), plugin);

  /* load global settings */
  plugin->disable_tooltips =
      xfconf_channel_get_bool (plugin->channel, "/disable-tooltips", FALSE);
  plugin->move_first =
      xfconf_channel_get_bool (plugin->channel, "/move-first", FALSE);
  value = xfconf_channel_get_uint (plugin->channel, "/arrow-position",
                                   LAUNCHER_ARROW_DEFAULT);
  plugin->arrow_position = MIN (value, LAUNCHER_ARROW_MAX);

  /* load the items */
  launcher_plugin_items_load (plugin);

  /* handle and empty plugin */
  if (G_UNLIKELY (plugin->items == NULL))
    {
      /* get the plugin arguments list */
      filenames = xfce_panel_plugin_get_arguments (panel_plugin);
      if (G_LIKELY (filenames != NULL))
        {
          for (i = 0; filenames[i] != NULL; i++)
            {
              /* create a new item from the file */
              item = xfce_menu_item_new (filenames[i]);
              if (G_LIKELY (item != NULL))
                plugin->items = g_slist_append (plugin->items, item);
            }
        }
    }

  /* update the icon */
  launcher_plugin_button_set_icon (plugin);

  /* update the arrow visibility */
  launcher_plugin_arrow_visibility (plugin);

  /* repack the widgets */
  launcher_plugin_pack_widgets (plugin);
}



static void
launcher_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  XfceLauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);

  /* release the xfconf channel */
  g_object_unref (G_OBJECT (plugin->channel));

  /* destroy the menu and timeout */
  launcher_plugin_menu_destroy (plugin);

  /* shutdown xfconf */
  xfconf_shutdown ();

  /* free items */
  g_slist_foreach (plugin->items, (GFunc) g_object_unref, NULL);
  g_slist_free (plugin->items);
}



static void
launcher_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                     GtkOrientation   orientation)
{
  /* update the widget order */
  launcher_plugin_pack_widgets (XFCE_LAUNCHER_PLUGIN (panel_plugin));

  /* update the arrow button */
  launcher_plugin_screen_position_changed (panel_plugin,
      xfce_panel_plugin_get_screen_position (panel_plugin));

  /* update the plugin size */
  launcher_plugin_size_changed (panel_plugin,
      xfce_panel_plugin_get_size (panel_plugin));
}



static gboolean
launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint             size)
{
  XfceLauncherPlugin    *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  gint                   p_width, p_height;
  gint                   a_width, a_height;
  gboolean               horizontal;
  LauncherArrowType      arrow_position;

  /* initialize the plugin size */
  p_width = p_height = size;
  a_width = a_height = -1;

  /* add the arrow size */
  if (GTK_WIDGET_VISIBLE (plugin->arrow))
    {
      /* if the panel is horizontal */
      horizontal = !!(xfce_panel_plugin_get_orientation (panel_plugin) ==
          GTK_ORIENTATION_HORIZONTAL);

      /* set tmp arrow position */
      arrow_position = plugin->arrow_position;
      if (arrow_position == LAUNCHER_ARROW_DEFAULT)
        arrow_position = horizontal ? LAUNCHER_ARROW_WEST : LAUNCHER_ARROW_NORTH;

      switch (plugin->arrow_position)
        {
          case LAUNCHER_ARROW_NORTH:
          case LAUNCHER_ARROW_SOUTH:
            a_height = ARROW_BUTTON_SIZE;

            if (horizontal)
              p_width -= ARROW_BUTTON_SIZE;
            else
              p_height += ARROW_BUTTON_SIZE;
            break;

          default:
            a_width = ARROW_BUTTON_SIZE;

            if (horizontal)
              p_width += ARROW_BUTTON_SIZE;
            else
              p_height -= ARROW_BUTTON_SIZE;
            break;
        }

      /* set the arrow size */
      gtk_widget_set_size_request (plugin->arrow, a_width, a_height);
    }

  /* set the panel plugin size */
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), p_width, p_height);

  return TRUE;
}



static void
launcher_plugin_save (XfcePanelPlugin *panel_plugin)
{
  XfceLauncherPlugin  *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  gchar              **filenames;
  guint                i, length;
  GSList              *li;
  XfceMenuItem        *item;

  /* save the global settings */
  xfconf_channel_set_bool (plugin->channel, "/disable-tooltips",
                           plugin->disable_tooltips);
  xfconf_channel_set_bool (plugin->channel, "/move-first",
                           plugin->move_first);
  xfconf_channel_set_uint (plugin->channel, "/arrow-position",
                           plugin->arrow_position);

  length = g_slist_length (plugin->items);
  if (G_LIKELY (length > 0))
    {
      /* create the array with the desktop ids */
      filenames = g_new0 (gchar *, length + 1);
      for (li = plugin->items, i = 0; li != NULL; li = li->next)
        if (G_LIKELY ((item = li->data) != NULL))
          filenames[i++] = (gchar *) xfce_menu_item_get_filename (item);

      /* store the list of filenames */
      xfconf_channel_set_string_list (plugin->channel, "/items",
                                      (const gchar **) filenames);

      /* cleanup */
      g_free (filenames);
    }
}



static void
launcher_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  /* run the configure dialog */
  launcher_dialog_show (XFCE_LAUNCHER_PLUGIN (panel_plugin));
}



static void
launcher_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                         gint             position)
{
  XfceLauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);

  /* set the new arrow direction */
  xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow),
      xfce_panel_plugin_arrow_type (panel_plugin));

  /* destroy the menu */
  launcher_plugin_menu_destroy (plugin);
}



static void
launcher_plugin_property_changed (XfconfChannel      *channel,
                                  const gchar        *property_name,
                                  const GValue       *value,
                                  XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->channel == channel);

  /* destroy the menu, all the setting changes need this */
  launcher_plugin_menu_destroy (plugin);

  if (exo_str_is_equal (property_name, "/disable-tooltips"))
    {
      plugin->disable_tooltips = g_value_get_boolean (value);
    }
  else if (exo_str_is_equal (property_name, "/move-first"))
    {
      plugin->move_first = g_value_get_boolean (value);
    }
  else if (exo_str_is_equal (property_name, "/arrow-position"))
    {
      plugin->arrow_position = MIN (g_value_get_uint (value),
                                    LAUNCHER_ARROW_MAX);

      goto update_arrow;

    }
  else if (exo_str_is_equal (property_name, "/items"))
    {
      /* free items */
      g_slist_foreach (plugin->items, (GFunc) g_object_unref, NULL);
      g_slist_free (plugin->items);
      plugin->items = NULL;

      /* load the new items */
      launcher_plugin_items_load (plugin);

      /* update the icon */
      launcher_plugin_button_set_icon (plugin);

      update_arrow:

      /* update the arrow button visibility */
      launcher_plugin_arrow_visibility (plugin);

      /* repack the widgets */
      launcher_plugin_pack_widgets (plugin);

      /* update the plugin size */
      launcher_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
          xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
    }
}



static void
launcher_plugin_icon_theme_changed (GtkIconTheme       *icon_theme,
                                    XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_ICON_THEME (icon_theme));

  /* update the button icon */
  launcher_plugin_button_set_icon (plugin);

  /* destroy the menu */
  launcher_plugin_menu_destroy (plugin);
}



static void
launcher_plugin_pack_widgets (XfceLauncherPlugin *plugin)
{
  LauncherArrowType pos = plugin->arrow_position;
  gboolean          rtl;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* leave when the arrow button is not visible */
  if (!GTK_WIDGET_VISIBLE (plugin->arrow)
      || pos == LAUNCHER_ARROW_INTERNAL)
    return;

  if (pos == LAUNCHER_ARROW_DEFAULT)
    {
      /* get the plugin direction */
      rtl = !!(gtk_widget_get_direction (GTK_WIDGET (plugin)) == GTK_TEXT_DIR_RTL);

      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) ==
              GTK_ORIENTATION_HORIZONTAL)
        pos = rtl ? LAUNCHER_ARROW_WEST : LAUNCHER_ARROW_EAST;
      else
        pos = rtl ? LAUNCHER_ARROW_NORTH : LAUNCHER_ARROW_SOUTH;
    }

  /* set the position of the arrow button in the box */
  gtk_box_reorder_child (GTK_BOX (plugin->box), plugin->arrow,
      (pos == LAUNCHER_ARROW_WEST || pos == LAUNCHER_ARROW_NORTH) ? 0 : -1);

  /* set the orientation of the hvbox */
  xfce_hvbox_set_orientation (XFCE_HVBOX (plugin->box),
      !!(pos == LAUNCHER_ARROW_WEST || pos == LAUNCHER_ARROW_EAST) ?
          GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL);
}



static void
launcher_plugin_menu_deactivate (GtkWidget          *menu,
                                 XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == menu);

  /* deactivate the arrow button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), FALSE);
}



static gboolean
launcher_plugin_menu_item_released (GtkMenuItem    *widget,
                                    GdkEventButton *event,
                                    XfceMenuItem   *item)
{
  XfceLauncherPlugin *plugin;
  GdkScreen          *screen;

  panel_return_val_if_fail (GTK_IS_MENU_ITEM (widget), FALSE);
  panel_return_val_if_fail (XFCE_IS_MENU_ITEM (item), FALSE);

  /* get the widget screen */
  screen = gtk_widget_get_screen (GTK_WIDGET (widget));

  /* launch the command */
  if (G_UNLIKELY (event->button == 2))
    launcher_plugin_item_exec_from_clipboard (item, event->time, screen);
  else
    launcher_plugin_item_exec (item, event->time, screen, NULL);

  /* get the plugin */
  plugin = g_object_get_qdata (G_OBJECT (widget), launcher_plugin_quark);
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* move the item to the first position if enabled */
  if (G_UNLIKELY (plugin->move_first))
    {
      /* prepend the item in the list */
      plugin->items = g_slist_remove (plugin->items, item);
      plugin->items = g_slist_prepend (plugin->items, item);

      /* destroy the menu and update the icon */
      launcher_plugin_menu_destroy (plugin);
      launcher_plugin_button_set_icon (plugin);
    }

  return FALSE;
}



static void
launcher_plugin_menu_item_drag_data_received (GtkWidget        *widget,
                                              GdkDragContext   *context,
                                              gint              x,
                                              gint              y,
                                              GtkSelectionData *data,
                                              guint             info,
                                              guint             drag_time,
                                              XfceMenuItem     *item)
{
  XfceLauncherPlugin *plugin;
  GSList             *uri_list;

  panel_return_if_fail (GTK_IS_MENU_ITEM (widget));
  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));

  /* get the plugin */
  plugin = g_object_get_qdata (G_OBJECT (widget), launcher_plugin_quark);
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* extract the uris from the selection data */
  uri_list = launcher_plugin_uri_list_extract (data);
  if (G_LIKELY (uri_list != NULL))
    {
      /* execute the menu item */
      launcher_plugin_item_exec (item, drag_time,
                                 gtk_widget_get_screen (widget),
                                 uri_list);

      /* cleanup */
      launcher_plugin_uri_list_free (uri_list);
    }

  /* hide the menu */
  gtk_widget_hide (GTK_MENU (plugin->menu)->toplevel);
  gtk_widget_hide (plugin->menu);

  /* inactivate the toggle button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), FALSE);

  /* finish the drag */
  gtk_drag_finish (context, TRUE, FALSE, drag_time);
}



static void
launcher_plugin_menu_construct (XfceLauncherPlugin *plugin)
{
  GtkArrowType  arrow_type;
  guint         n;
  XfceMenuItem *item;
  GtkWidget    *mi, *image;
  const gchar  *name, *icon_name;
  GSList       *li;
  GdkPixbuf    *pixbuf;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == NULL);

  /* create a new menu */
  plugin->menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (plugin->menu), GTK_WIDGET (plugin), NULL);
  g_signal_connect (G_OBJECT (plugin->menu), "deactivate",
                    G_CALLBACK (launcher_plugin_menu_deactivate), plugin);

  /* get the arrow type of the plugin */
  arrow_type = xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow));

  /* walk through the menu entries */
  for (li = plugin->items, n = 0; li != NULL; li = li->next, n++)
    {
      /* skip the first entry when the arrow is visible */
      if (n == 0 && plugin->arrow_position != LAUNCHER_ARROW_INTERNAL)
        continue;

      /* get the item data */
      item = XFCE_MENU_ITEM (li->data);

      /* create the menu item */
      name = xfce_menu_item_get_name (item);
      mi = gtk_image_menu_item_new_with_label (IS_STRING (name) ? name :
                                               _("Unnamed Item"));
      g_object_set_qdata (G_OBJECT (mi), launcher_plugin_quark, plugin);
      gtk_widget_set_has_tooltip (mi, TRUE);
      gtk_widget_show (mi);
      gtk_drag_dest_set (mi, GTK_DEST_DEFAULT_ALL,
                         drop_targets, G_N_ELEMENTS (drop_targets),
                         GDK_ACTION_COPY);
      g_signal_connect (G_OBJECT (mi), "button-release-event",
          G_CALLBACK (launcher_plugin_menu_item_released), item);
      g_signal_connect (G_OBJECT (mi), "drag-data-received",
          G_CALLBACK (launcher_plugin_menu_item_drag_data_received), item);
      g_signal_connect (G_OBJECT (mi), "drag-leave",
          G_CALLBACK (launcher_plugin_arrow_drag_leave), plugin);

      /* only connect the tooltip signal if tips are enabled */
      if (plugin->disable_tooltips == FALSE)
        g_signal_connect (G_OBJECT (mi), "query-tooltip",
            G_CALLBACK (launcher_plugin_item_query_tooltip), item);

      /* depending on the menu position we prepend or append */
      if (G_UNLIKELY (arrow_type == GTK_ARROW_DOWN))
        gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), mi);
      else
        gtk_menu_shell_prepend (GTK_MENU_SHELL (plugin->menu), mi);

      /* set the icon if one is set */
      icon_name = xfce_menu_item_get_icon_name (item);
      if (IS_STRING (icon_name))
        {
          pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                             icon_name, plugin->menu_icon_size,
                                             0, NULL);
          if (G_LIKELY (pixbuf != NULL))
            {
              image = gtk_image_new_from_pixbuf (pixbuf);
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              gtk_widget_show (image);

              g_object_unref (G_OBJECT (pixbuf));
            }
        }
    }
}



static void
launcher_plugin_menu_popup_destroyed (gpointer user_data)
{
   XFCE_LAUNCHER_PLUGIN (user_data)->menu_timeout_id = 0;
}



static gboolean
launcher_plugin_menu_popup (gpointer user_data)
{
  XfceLauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (user_data);
  gint                x, y;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  GDK_THREADS_ENTER ();

  /* construct the menu if needed */
  if (plugin->menu == NULL)
    launcher_plugin_menu_construct (plugin);

  /* toggle the arrow button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), TRUE);

  /* popup the menu */
  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  xfce_panel_plugin_position_menu,
                  XFCE_PANEL_PLUGIN (plugin), 1,
                  gtk_get_current_event_time ());

  /* fallback to manual positioning, this is used with
   * drag motion over the arrow button */
  if (!GTK_WIDGET_VISIBLE (plugin->menu))
    {
      /* make sure the size is allocated */
      if (!GTK_WIDGET_REALIZED (plugin->menu))
        gtk_widget_realize (plugin->menu);

      /* use the widget position function to get the coordinates */
      xfce_panel_plugin_position_widget (XFCE_PANEL_PLUGIN (plugin),
                                         plugin->menu, NULL, &x, &y);

      /* bit ugly... but show the menu */
      gtk_widget_show (plugin->menu);
      gtk_window_move (GTK_WINDOW (GTK_MENU (plugin->menu)->toplevel), x, y);
      gtk_widget_show (GTK_MENU (plugin->menu)->toplevel);
    }

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
launcher_plugin_menu_destroy (XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* stop pending timeout */
  if (plugin->menu_timeout_id != 0)
    g_source_remove (plugin->menu_timeout_id);

  if (plugin->menu != NULL)
    {
      /* destroy the menu */
      gtk_widget_destroy (plugin->menu);
      plugin->menu = NULL;

      /* deactivate the toggle button */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), FALSE);
    }
}



static void
launcher_plugin_button_set_icon (XfceLauncherPlugin *plugin)
{
  XfceMenuItem *item;
  const gchar  *icon_name;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SCALED_IMAGE (plugin->image));

  if (G_LIKELY (plugin->items != NULL))
    {
      item = XFCE_MENU_ITEM (plugin->items->data);

      icon_name = xfce_menu_item_get_icon_name (item);
      if (IS_STRING (icon_name))
        {
          if (g_path_is_absolute (icon_name))
            xfce_scaled_image_set_from_file (XFCE_SCALED_IMAGE (plugin->image),
                                             icon_name);
          else
            xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image),
                                                  icon_name);
          /* TODO some more name checking */
          return;
        }
    }

  /* set missing image icon */
  xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image),
                                        GTK_STOCK_MISSING_IMAGE);
}



static void
launcher_plugin_button_state_changed (GtkWidget    *button_a,
                                      GtkStateType  state,
                                      GtkWidget    *button_b)
{
  if (GTK_WIDGET_STATE (button_a) != GTK_WIDGET_STATE (button_b)
      && GTK_WIDGET_STATE (button_a) != GTK_STATE_INSENSITIVE)
    gtk_widget_set_state (button_b, GTK_WIDGET_STATE (button_a));
}



static gboolean
launcher_plugin_button_press_event (GtkWidget          *button,
                                    GdkEventButton     *event,
                                    XfceLauncherPlugin *plugin)
{
  guint modifiers;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* do nothing on anything else then a single click */
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  /* get the default accelerator modifier mask */
  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  /* leave when button 1 is not pressed or shift is pressed */
  if (event->button != 1 || modifiers == GDK_CONTROL_MASK)
    return FALSE;

  if (ARROW_INSIDE_BUTTON (plugin))
    {
      /* directly popup the menu */
      launcher_plugin_menu_popup (plugin);
    }
  else if (plugin->menu_timeout_id == 0
           && LIST_HAS_TWO_OR_MORE_ENTRIES (plugin->items))
    {
      /* start the popup timeout */
      plugin->menu_timeout_id =
          g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
                              MENU_POPUP_DELAY,
                              launcher_plugin_menu_popup, plugin,
                              launcher_plugin_menu_popup_destroyed);
    }

  return FALSE;
}



static gboolean
launcher_plugin_button_release_event (GtkWidget          *button,
                                      GdkEventButton     *event,
                                      XfceLauncherPlugin *plugin)
{
  XfceMenuItem *item;
  GdkScreen    *screen;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* remove a delayed popup timeout */
  if (plugin->menu_timeout_id != 0)
    g_source_remove (plugin->menu_timeout_id);

  /* leave when there are no menu items or there is an internal arrow */
  if (plugin->items == NULL
      || GTK_BUTTON (button)->in_button == FALSE
      || ARROW_INSIDE_BUTTON (plugin))
    return FALSE;

  /* get the menu item and the screen */
  item = XFCE_MENU_ITEM (plugin->items->data);
  screen = gtk_widget_get_screen (button);

  /* launcher the entry */
  if (event->button == 1)
    launcher_plugin_item_exec (item, event->time, screen, NULL);
  else if (event->button == 2)
    launcher_plugin_item_exec_from_clipboard (item, event->time, screen);
  else
    return TRUE;

  return FALSE;
}



static gboolean
launcher_plugin_button_query_tooltip (GtkWidget          *widget,
                                      gint                x,
                                      gint                y,
                                      gboolean            keyboard_mode,
                                      GtkTooltip         *tooltip,
                                      XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* check if we show tooltips */
  if (plugin->disable_tooltips
      || plugin->arrow_position
      || plugin->items == NULL
      || plugin->items->data == NULL)
    return FALSE;

  return launcher_plugin_item_query_tooltip (widget, x, y, keyboard_mode,
                                             tooltip,
                                             XFCE_MENU_ITEM (plugin->items->data));
}



static void
launcher_plugin_button_drag_data_received (GtkWidget            *widget,
                                           GdkDragContext       *context,
                                           gint                  x,
                                           gint                  y,
                                           GtkSelectionData     *selection_data,
                                           guint                 info,
                                           guint                 drag_time,
                                           XfceLauncherPlugin   *plugin)
{
  GSList *uri_list;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* leave when there are not items or the arrow is internal */
  if (ARROW_INSIDE_BUTTON (plugin) || plugin->items == NULL)
    return;

  /* get the list of uris from the selection data */
  uri_list = launcher_plugin_uri_list_extract (selection_data);
  if (G_LIKELY (uri_list != NULL))
    {
      /* execute */
      launcher_plugin_item_exec (XFCE_MENU_ITEM (plugin->items->data),
                                 gtk_get_current_event_time (),
                                 gtk_widget_get_screen (widget),
                                 uri_list);

      /* cleanup */
      launcher_plugin_uri_list_free (uri_list);
    }

  /* finish the drag */
  gtk_drag_finish (context, TRUE, FALSE, drag_time);
}



static gboolean
launcher_plugin_button_drag_motion (GtkWidget          *widget,
                                    GdkDragContext     *context,
                                    gint                x,
                                    gint                y,
                                    guint               drag_time,
                                    XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* do nothing if the plugin is empty */
  if (plugin->items == NULL)
    {
      /* not a drop zone */
      gdk_drag_status (context, 0, drag_time);
      return FALSE;
    }

  /* highlight the button if this is a launcher button */
  if (NO_ARROW_INSIDE_BUTTON (plugin))
    {
      gtk_drag_highlight (widget);
      return TRUE;
    }

  /* handle the popup menu */
  return launcher_plugin_arrow_drag_motion (widget, context, x, y,
                                            drag_time, plugin);
}



static void
launcher_plugin_button_drag_leave (GtkWidget          *widget,
                                   GdkDragContext     *context,
                                   guint               drag_time,
                                   XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* unhighlight the widget or make sure the menu is deactivated */
  if (NO_ARROW_INSIDE_BUTTON (plugin))
    gtk_drag_unhighlight (widget);
  else
    launcher_plugin_arrow_drag_leave (widget, context, drag_time, plugin);
}



static gboolean
launcher_plugin_button_expose_event (GtkWidget          *widget,
                                     GdkEventExpose     *event,
                                     XfceLauncherPlugin *plugin)
{
  GtkArrowType arrow_type;
  gint         size, x, y, thickness, offset;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* leave when the arrow is not shown inside the button */
  if (NO_ARROW_INSIDE_BUTTON (plugin))
    return FALSE;

  /* get the arrow type */
  arrow_type = xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow));

  /* style thickness */
  thickness = MAX (widget->style->xthickness, widget->style->ythickness);

  /* size of the arrow and the start coordinates */
  size = widget->allocation.width / 3;
  x = widget->allocation.x + thickness;
  y = widget->allocation.y + thickness;
  offset = size + 2 * thickness;

  /* calculate the position based on the arrow type */
  switch (arrow_type)
    {
      case GTK_ARROW_UP:
        /* north east */
        x += widget->allocation.width - offset;
        break;

      case GTK_ARROW_DOWN:
        /* south west */
        y += widget->allocation.height - offset;
        break;

      case GTK_ARROW_RIGHT:
        /* south east */
        x += widget->allocation.width - offset;
        y += widget->allocation.height - offset;
        break;

      default:
        /* north west */
        break;
    }

  /* paint the arrow */
  gtk_paint_arrow (widget->style, widget->window,
                   GTK_WIDGET_STATE (widget), GTK_SHADOW_IN,
                   &(event->area), widget, "launcher_button",
                   arrow_type, TRUE, x, y, size, size);

  return FALSE;
}



static void
launcher_plugin_arrow_visibility (XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  if (plugin->arrow_position != LAUNCHER_ARROW_INTERNAL
       && LIST_HAS_TWO_OR_MORE_ENTRIES (plugin->items))
    gtk_widget_show (plugin->arrow);
  else
    gtk_widget_hide (plugin->arrow);
}



static gboolean
launcher_plugin_arrow_press_event (GtkWidget          *button,
                                   GdkEventButton     *event,
                                   XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* only popup when button 1 is pressed */
  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      launcher_plugin_menu_popup (plugin);
      return FALSE;
    }

  return TRUE;
}




static gboolean
launcher_plugin_arrow_drag_motion (GtkWidget          *widget,
                                   GdkDragContext     *context,
                                   gint                x,
                                   gint                y,
                                   guint               drag_time,
                                   XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* the arrow is not a drop zone */
  gdk_drag_status (context, 0, drag_time);

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->arrow)))
    {
      /* make the toggle button active */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), TRUE);

      /* start the popup timeout */
      plugin->menu_timeout_id =
          g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, MENU_POPUP_DELAY,
                              launcher_plugin_menu_popup, plugin,
                              launcher_plugin_menu_popup_destroyed);

      return TRUE;
    }

  return FALSE;
}



static gboolean
launcher_plugin_arrow_drag_leave_timeout (gpointer user_data)
{
  XfceLauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (user_data);
  gint                pointer_x, pointer_y;
  GtkWidget          *menu = plugin->menu;
  gint                menu_x, menu_y, menu_w, menu_h;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (menu == NULL || GDK_IS_WINDOW (menu->window), FALSE);

  /* leave when the menu is destroyed */
  if (G_UNLIKELY (plugin->menu == NULL))
    return FALSE;

  /* get the pointer position */
  gdk_display_get_pointer (gtk_widget_get_display (menu),
                           NULL, &pointer_x, &pointer_y, NULL);

  /* get the menu position */
  gdk_window_get_root_origin (menu->window, &menu_x, &menu_y);
  gdk_drawable_get_size (GDK_DRAWABLE (menu->window), &menu_w, &menu_h);

  /* check if we should hide the menu */
  if (pointer_x < menu_x || pointer_x > menu_x + menu_w
      || pointer_y < menu_y || pointer_y > menu_y + menu_h)
    {
      /* hide the menu */
      gtk_widget_hide (GTK_MENU (menu)->toplevel);
      gtk_widget_hide (menu);

      /* inactive the toggle button */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), FALSE);
    }

  return FALSE;
}



static void
launcher_plugin_arrow_drag_leave (GtkWidget          *widget,
                                  GdkDragContext     *context,
                                  guint               drag_time,
                                  XfceLauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  if (plugin->menu_timeout_id != 0)
    {
      /* stop the popup timeout */
      g_source_remove (plugin->menu_timeout_id);

      /* inactive the toggle button */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow), FALSE);
    }
  else
    {
      /* start a timeout to give the user some time to drag to the menu */
      g_timeout_add (MENU_POPUP_DELAY, launcher_plugin_arrow_drag_leave_timeout, plugin);
    }
}



static void
launcher_plugin_items_load (XfceLauncherPlugin *plugin)
{
  gchar        **filenames;
  guint          i;
  XfceMenuItem  *item;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->items == NULL);

  /* get the list of launcher filenames */
  filenames = xfconf_channel_get_string_list (plugin->channel, "/items");
  if (G_LIKELY (filenames != NULL))
    {
      /* try to load all the items */
      for (i = 0; filenames[i] != NULL; i++)
        {
          /* create a new item from the file */
          item = xfce_menu_item_new (filenames[i]);
          if (G_LIKELY (item != NULL))
            plugin->items = g_slist_append (plugin->items, item);
          else
            /* TODO */
            g_message ("Lookup the item in the pool...");
        }

      /* cleanup */
      g_strfreev (filenames);
    }
}



static gboolean
launcher_plugin_item_query_tooltip (GtkWidget    *widget,
                                    gint          x,
                                    gint          y,
                                    gboolean      keyboard_mode,
                                    GtkTooltip   *tooltip,
                                    XfceMenuItem *item)
{
  gchar       *markup;
  const gchar *name, *comment, *icon_name;

  panel_return_val_if_fail (XFCE_IS_MENU_ITEM (item), FALSE);

  /* require atleast an item name */
  name = xfce_menu_item_get_name (item);
  if (!IS_STRING (name))
    return FALSE;

  comment = xfce_menu_item_get_comment (item);
  if (IS_STRING (comment))
    {
      markup = g_strdup_printf ("<b>%s</b>\n%s", name, comment);
      gtk_tooltip_set_markup (tooltip, markup);
      g_free (markup);
    }
  else
    {
      gtk_tooltip_set_text (tooltip, name);
    }

  icon_name = xfce_menu_item_get_icon_name (item);
  if (G_LIKELY (icon_name))
    {
      /* TODO pixbuf with cache */
    }

  return TRUE;
}



static gboolean
launcher_plugin_item_exec_on_screen (XfceMenuItem *item,
                                     guint32       event_time,
                                     GdkScreen    *screen,
                                     GSList       *uri_list)
{
  GError    *error = NULL;
  gchar    **argv;
  gboolean   succeed = FALSE;

  panel_return_val_if_fail (XFCE_IS_MENU_ITEM (item), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  /* parse the execute command */
  if (launcher_plugin_exec_parse (item, uri_list, NULL, &argv, &error))
    {
      /* launch the command on the screen */
      succeed = xfce_execute_argv_on_screen (screen,
                                             xfce_menu_item_get_path (item),
                                             argv, NULL, G_SPAWN_SEARCH_PATH,
                                             xfce_menu_item_supports_startup_notification (item),
                                             event_time,
                                             xfce_menu_item_get_icon_name (item),
                                             &error);

      /* cleanup */
      g_strfreev (argv);
    }

  if (G_UNLIKELY (succeed == FALSE))
    {
      /* show an error dialog */
      xfce_dialog_show_error (screen, error,
                              _("Failed to execute command \"%s\"."),
                              xfce_menu_item_get_command (item));

      /* cleanup */
      g_error_free (error);
    }

  return succeed;
}



static void
launcher_plugin_item_exec (XfceMenuItem *item,
                           guint32       event_time,
                           GdkScreen    *screen,
                           GSList       *uri_list)
{
  GSList      *li, fake;
  gboolean     proceed = TRUE;
  const gchar *command;

  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));
  panel_return_if_fail (GDK_IS_SCREEN (screen));

  /* leave when there is nothing to execute */
  command = xfce_menu_item_get_command (item);
  if (!IS_STRING (command))
    return;

  if (G_UNLIKELY (uri_list != NULL
      && strstr (command, "%F") == NULL
      && strstr (command, "%U") == NULL))
    {
      fake.next = NULL;

      /* run an instance for each file, break on the first error */
      for (li = uri_list; li != NULL && proceed; li = li->next)
        {
          fake.data = li->data;
          proceed = launcher_plugin_item_exec_on_screen (item, event_time,
                                                         screen, &fake);
        }
    }
  else
    {
      launcher_plugin_item_exec_on_screen (item, event_time,
                                           screen, uri_list);
    }
}



static void
launcher_plugin_item_exec_from_clipboard (XfceMenuItem *item,
                                          guint32       event_time,
                                          GdkScreen    *screen)
{
  GtkClipboard     *clipboard;
  gchar            *text = NULL;
  GSList           *uri_list;
  GtkSelectionData  data;

  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));
  panel_return_if_fail (GDK_IS_SCREEN (screen));

  /* get the primary clipboard text */
  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  if (G_LIKELY (clipboard))
    text = gtk_clipboard_wait_for_text (clipboard);

  /* try the secondary keayboard if the text is empty */
  if (IS_STRING (text))
    {
      /* get the secondary clipboard text */
      clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      if (G_LIKELY (clipboard))
        text = gtk_clipboard_wait_for_text (clipboard);
    }

  if (IS_STRING (text))
    {
      /* create fake selection data */
      data.data = (guchar *) text;
      data.length = strlen (text);
      data.target = GDK_NONE;

      /* extract the uris from the selection data */
      uri_list = launcher_plugin_uri_list_extract (&data);

      /* launch with the uri list */
      launcher_plugin_item_exec (item, event_time,
                                 screen, uri_list);

      /* cleanup */
      launcher_plugin_uri_list_free (uri_list);
    }

  /* cleanup */
  g_free (text);
}



static void
launcher_plugin_exec_append_quoted (GString     *string,
                                    const gchar *unquoted)
{
  gchar *quoted;

  quoted = g_shell_quote (unquoted);
  g_string_append (string, quoted);
  g_free (quoted);
}



static gboolean
launcher_plugin_exec_parse (XfceMenuItem   *item,
                            GSList         *uri_list,
                            gint           *argc,
                            gchar        ***argv,
                            GError        **error)
{
  GString     *string;
  const gchar *p;
  gboolean     result;
  GSList      *li;
  gchar       *filename;
  const gchar *command, *tmp;

  panel_return_val_if_fail (XFCE_IS_MENU_ITEM (item), FALSE);

  /* get the command */
  command = xfce_menu_item_get_command (item);
  panel_return_val_if_fail (IS_STRING (command), FALSE);

  /* allocate an empty string */
  string = g_string_sized_new (100);

  /* prepend terminal command if required */
  if (xfce_menu_item_requires_terminal (item))
    g_string_append (string, "exo-open --launch TerminalEmulator ");

  for (p = command; *p != '\0'; ++p)
    {
      if (p[0] == '%' && p[1] != '\0')
        {
          switch (*++p)
            {
              case 'f':
              case 'F':
                for (li = uri_list; li != NULL; li = li->next)
                  {
                    filename = g_filename_from_uri ((const gchar *) li->data,
                                                    NULL, NULL);
                    if (G_LIKELY (filename != NULL))
                      launcher_plugin_exec_append_quoted (string, filename);
                    g_free (filename);

                    if (*p == 'f')
                      break;
                    if (li->next != NULL)
                      g_string_append_c (string, ' ');
                  }
                break;

              case 'u':
              case 'U':
                for (li = uri_list; li != NULL; li = li->next)
                  {
                    launcher_plugin_exec_append_quoted (string, (const gchar *)
                                                        li->data);

                    if (*p == 'u')
                      break;
                    if (li->next != NULL)
                      g_string_append_c (string, ' ');
                  }
                break;

              case 'i':
                tmp = xfce_menu_item_get_icon_name (item);
                if (IS_STRING (tmp))
                  {
                    g_string_append (string, "--icon ");
                    launcher_plugin_exec_append_quoted (string, tmp);
                  }
                break;

              case 'c':
                tmp = xfce_menu_item_get_name (item);
                if (IS_STRING (tmp))
                  launcher_plugin_exec_append_quoted (string, tmp);
                break;

              case 'k':
                tmp = xfce_menu_item_get_filename (item);
                if (IS_STRING (tmp))
                  launcher_plugin_exec_append_quoted (string, tmp);
                break;

              case '%':
                g_string_append_c (string, '%');
                break;
            }
        }
      else
        {
          g_string_append_c (string, *p);
        }
    }

  result = g_shell_parse_argv (string->str, argc, argv, error);
  g_string_free (string, TRUE);

  return result;
}



static GSList *
launcher_plugin_uri_list_extract (GtkSelectionData *data)
{
  GSList  *list = NULL;
  gchar  **array;
  guint    i;
  gchar   *uri;
  gint     j;

  /* leave if there is no data */
  if (data->length <= 0)
    return NULL;

  /* extract the files */
  if (data->target == gdk_atom_intern_static_string ("text/uri-list"))
    {
      /* extract the list of uris */
      array = g_uri_list_extract_uris ((gchar *) data->data);
      if (G_UNLIKELY (array == NULL))
        return NULL;

      /* create the list of uris */
      for (i = 0; array[i] != NULL; i++)
        {
          if (IS_STRING (array[i]))
            list = g_slist_prepend (list, array[i]);
          else
            g_free (array[i]);
        }

      /* cleanup */
      g_free (array);
    }
  else
    {
      /* split the data on new lines */
      array = g_strsplit_set ((const gchar *) data->data, "\n\r", -1);
      if (G_UNLIKELY (array == NULL))
        return NULL;

      /* create the list of uris */
      for (i = 0; array[i] != NULL; i++)
        {
          /* skip empty strings */
          if (!IS_STRING (array[i]))
            continue;

          uri = NULL;

          if (g_path_is_absolute (array[i]))
            {
              /* convert the filename to an uri */
              uri = g_filename_to_uri (array[i], NULL, NULL);
            }
          else if (data->length > 6)
            {
              /* check if this looks like an other uri, if it does, use it */
              for (j = 3; uri == NULL && j <= 5; j++)
                if (g_str_has_prefix (array[i] + j, "://"))
                  uri = g_strdup (array[i]);
            }

          /* append the uri if we extracted one */
          if (G_LIKELY (uri != NULL))
            list = g_slist_prepend (list, uri);
        }

      /* cleanup */
      g_strfreev (array);
    }

  return g_slist_reverse (list);
}



static void
launcher_plugin_uri_list_free (GSList *uri_list)
{
  if (uri_list != NULL)
    {
      g_slist_foreach (uri_list, (GFunc) g_free, NULL);
      g_slist_free (uri_list);
    }
}



XfconfChannel *
launcher_plugin_get_channel (XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), NULL);
  return plugin->channel;
}



GSList *
launcher_plugin_get_items (XfceLauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), NULL);
  return plugin->items;
}
