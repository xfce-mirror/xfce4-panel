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

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <common/panel-debug.h>

#include "systray.h"
#include "systray-box.h"
#include "systray-socket.h"
#include "systray-manager.h"

#define BUTTON_SIZE   (16)


static void     systray_plugin_get_property                 (GObject               *object,
                                                             guint                  prop_id,
                                                             GValue                *value,
                                                             GParamSpec            *pspec);
static void     systray_plugin_set_property                 (GObject               *object,
                                                             guint                  prop_id,
                                                             const GValue          *value,
                                                             GParamSpec            *pspec);
static void     systray_plugin_button_set_arrow             (SnPlugin              *plugin);
static void     systray_plugin_names_collect_ordered        (gpointer               data,
                                                             gpointer               user_data);
static void     systray_plugin_names_collect_hidden         (gpointer               key,
                                                             gpointer               value,
                                                             gpointer               user_data);
static void     systray_plugin_names_update                 (SnPlugin              *plugin);
static gboolean systray_plugin_names_get_hidden             (SnPlugin              *plugin,
                                                             const gchar           *name);
static void     systray_plugin_icon_added                   (SystrayManager        *manager,
                                                             GtkWidget             *icon,
                                                             SnPlugin              *plugin);
static void     systray_plugin_icon_removed                 (SystrayManager        *manager,
                                                             GtkWidget             *icon,
                                                             SnPlugin              *plugin);
static void     systray_plugin_lost_selection               (SystrayManager        *manager,
                                                             SnPlugin              *plugin);
static void     systray_plugin_dialog_add_application_names (gpointer               data,
                                                             gpointer               user_data);
static void     systray_plugin_dialog_hidden_toggled        (GtkCellRendererToggle *renderer,
                                                             const gchar           *path_string,
                                                             SnPlugin              *plugin);
static void     systray_plugin_dialog_selection_changed     (GtkTreeSelection      *selection,
                                                             SnPlugin              *plugin);
static void     systray_plugin_dialog_item_move_clicked     (GtkWidget             *button,
                                                             SnPlugin              *plugin);
static void     systray_plugin_dialog_clear_clicked         (GtkWidget             *button,
                                                             SnPlugin              *plugin);
static void     systray_plugin_dialog_cleanup               (SnPlugin              *plugin,
                                                             GtkBuilder            *builder);


enum
{
  PROP_0,
  PROP_ICON_SIZE,
  PROP_SQUARE_ICONS,
  PROP_KNOWN_LEGACY_ITEMS,
  PROP_HIDDEN_LEGACY_ITEMS
};

enum
{
  COLUMN_GICON,
  COLUMN_TITLE,
  COLUMN_HIDDEN,
  COLUMN_INTERNAL_NAME
};



/* known applications to improve the icon and name */
static const gchar *known_applications[][3] =
{
  /* application name, icon-name, understandable name */
  { "networkmanager applet", "network-workgroup", "Network Manager Applet" },
  { "thunar", "Thunar", "Thunar Progress Dialog" },
  { "workrave", NULL, "Workrave" },
  { "workrave tray icon", NULL, "Workrave Applet" },
  { "audacious2", "audacious", "Audacious" },
  { "wicd-client.py", "wicd-gtk", "Wicd" },
  { "drop-down terminal", "utilities-terminal", "Xfce Dropdown Terminal" },
  { "xfce terminal", "utilities-terminal", "Xfce Terminal" },
  { "task manager", "utilities-system-monitor", "Xfce Taskmanager" },
  { "xfce4-power-manager", "xfpm-ac-adapter", "Xfce Power Manager" },
};


/*
static void
systray_plugin_class_init (SnPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = systray_plugin_get_property;
  gobject_class->set_property = systray_plugin_set_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);

  g_object_class_install_property (gobject_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_uint ("icon-size",
                                                      NULL, NULL,
                                                      SIZE_MAX_MIN,
                                                      SIZE_MAX_MAX,
                                                      SIZE_MAX_DEFAULT,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SQUARE_ICONS,
                                   g_param_spec_boolean ("square-icons",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_KNOWN_LEGACY_ITEMS,
                                   g_param_spec_boxed ("known-legacy-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_HIDDEN_LEGACY_ITEMS,
                                   g_param_spec_boxed ("hidden-legacy-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}*/



static void
systray_free_array_element (gpointer data)
{
  GValue *value = (GValue *)data;

  g_value_unset (value);
  g_free (value);
}

void  systray_plugin_configuration_changed  (SnConfig           *config,
                                             SnPlugin           *plugin)
{
  gint icon_size = sn_config_get_icon_size (config);
  gboolean square_icons = sn_config_get_square_icons (config);

  systray_box_set_size_max (XFCE_SYSTRAY_BOX (plugin->systray_box),
                            icon_size);

  systray_box_set_squared (XFCE_SYSTRAY_BOX (plugin->systray_box), square_icons);
  systray_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
                               xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
}

static void
systray_plugin_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SnPlugin      *plugin = XFCE_SN_PLUGIN (object);
  GPtrArray     *array;

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      //g_value_set_uint (value,
      //                  systray_box_get_size_max (XFCE_SYSTRAY_BOX (plugin->systray_box)));
      break;

    case PROP_SQUARE_ICONS:
      //g_value_set_boolean (value,
      //                     systray_box_get_squared (XFCE_SYSTRAY_BOX (plugin->systray_box)));
      break;

    case PROP_KNOWN_LEGACY_ITEMS:
      //array = g_ptr_array_new_full (1, (GDestroyNotify) systray_free_array_element);
      //g_slist_foreach (plugin->names_ordered, systray_plugin_names_collect_ordered, array);
      //g_value_set_boxed (value, array);
      //g_ptr_array_unref (array);
      break;

    case PROP_HIDDEN_LEGACY_ITEMS:
      //array = g_ptr_array_new_full (1, (GDestroyNotify) systray_free_array_element);
      //g_hash_table_foreach (plugin->names_hidden, systray_plugin_names_collect_hidden, array);
      //g_value_set_boxed (value, array);
      //g_ptr_array_unref (array);
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
  SnPlugin      *plugin = XFCE_SN_PLUGIN (object);
  gboolean       boolean_val;
  GPtrArray     *array;
  const GValue  *tmp;
  gchar         *name;
  guint          i;

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      //systray_box_set_size_max (XFCE_SYSTRAY_BOX (plugin->systray_box),
      //                          g_value_get_uint (value));
      break;

    case PROP_SQUARE_ICONS:
      //boolean_val = g_value_get_boolean (value);
      //systray_box_set_squared (XFCE_SYSTRAY_BOX (plugin->systray_box), boolean_val);
      //systray_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
      //                             xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
      break;

    case PROP_KNOWN_LEGACY_ITEMS:
      //g_slist_free_full (plugin->names_ordered, g_free);
      //plugin->names_ordered = NULL;

      /* add new values */
      //array = g_value_get_boxed (value);
      //if (G_LIKELY (array != NULL))
      //  {
      //    for (i = 0; i < array->len; i++)
      //      {
      //        tmp = g_ptr_array_index (array, i);
      //        panel_assert (G_VALUE_HOLDS_STRING (tmp));
      //        name = g_value_dup_string (tmp);
      //        plugin->names_ordered = g_slist_prepend (plugin->names_ordered, name);
      //      }

      //    plugin->names_ordered = g_slist_reverse (plugin->names_ordered);
      //  }

      ///* update icons in the box */
      //systray_plugin_names_update (plugin);
      break;

    case PROP_HIDDEN_LEGACY_ITEMS:
      //g_hash_table_remove_all (plugin->names_hidden);

      ///* add new values */
      //array = g_value_get_boxed (value);
      //if (G_LIKELY (array != NULL))
      //  {
      //    for (i = 0; i < array->len; i++)
      //      {
      //        tmp = g_ptr_array_index (array, i);
      //        panel_assert (G_VALUE_HOLDS_STRING (tmp));
      //        name = g_value_dup_string (tmp);
      //        g_hash_table_replace (plugin->names_hidden, name, NULL);
      //      }
      //  }

      ///* update icons in the box */
      //systray_plugin_names_update (plugin);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
systray_plugin_screen_changed_idle (gpointer user_data)
{
  SnPlugin      *plugin = XFCE_SN_PLUGIN (user_data);
  GdkScreen     *screen;
  GError        *error = NULL;

  /* create a new manager and register this screen */
  plugin->manager = systray_manager_new ();
  g_signal_connect (G_OBJECT (plugin->manager), "icon-added",
      G_CALLBACK (systray_plugin_icon_added), plugin);
  g_signal_connect (G_OBJECT (plugin->manager), "icon-removed",
      G_CALLBACK (systray_plugin_icon_removed), plugin);
  g_signal_connect (G_OBJECT (plugin->manager), "lost-selection",
      G_CALLBACK (systray_plugin_lost_selection), plugin);

  /* try to register the systray */
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  if (systray_manager_register (plugin->manager, screen, &error))
    {
      /* send the plugin orientation */
      systray_plugin_orientation_changed (XFCE_PANEL_PLUGIN (plugin),
         xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)));
    }
  else
    {
      xfce_dialog_show_error (NULL, error, _("Unable to start the notification area"));
      g_error_free (error);
    }

  return FALSE;
}



static void
systray_plugin_screen_changed_idle_destroyed (gpointer user_data)
{
  XFCE_SN_PLUGIN (user_data)->idle_startup = 0;
}



void
systray_plugin_screen_changed (GtkWidget *widget,
                               GdkScreen *previous_screen)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (widget);

  if (G_UNLIKELY (plugin->manager != NULL))
    {
      /* unregister this screen screen */
      systray_manager_unregister (plugin->manager);
      g_object_unref (G_OBJECT (plugin->manager));
      plugin->manager = NULL;
    }

  /* schedule a delayed startup */
  if (plugin->idle_startup == 0)
    plugin->idle_startup = gdk_threads_add_idle_full (G_PRIORITY_LOW, systray_plugin_screen_changed_idle,
                                                      plugin, systray_plugin_screen_changed_idle_destroyed);
}


void
systray_plugin_composited_changed (GtkWidget *widget)
{
  /* restart the manager to add the sockets again */
  systray_plugin_screen_changed (widget, gtk_widget_get_screen (widget));
}



static void
systray_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SnPlugin       *plugin = XFCE_SN_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "icon-size", G_TYPE_UINT },
    { "square-icons", G_TYPE_BOOLEAN },
    { "known-legacy-items", G_TYPE_PTR_ARRAY },
    { "hidden-legacy-items", G_TYPE_PTR_ARRAY },
    { NULL }
  };

  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);
}



void
systray_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                    GtkOrientation   orientation)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (panel_plugin);


  gtk_orientable_set_orientation (GTK_ORIENTABLE (plugin->box), orientation);
  systray_box_set_orientation (XFCE_SYSTRAY_BOX (plugin->systray_box), orientation);

  if (G_LIKELY (plugin->manager != NULL))
    systray_manager_set_orientation (plugin->manager, orientation);

  /* apply symbolic colors */
  if (G_LIKELY (plugin->manager != NULL)) {
    GtkStyleContext *context;
    GdkRGBA rgba;
    GdkColor color;
    GdkColor fg;
    GdkColor error;
    GdkColor warning;
    GdkColor success;

    context = gtk_widget_get_style_context (GTK_WIDGET (plugin->systray_box));
    gtk_style_context_get_color (context, GTK_STATE_NORMAL, &rgba);

    color.pixel = 0;
    color.red = rgba.red * G_MAXUSHORT;
    color.green = rgba.green * G_MAXUSHORT;
    color.blue = rgba.blue * G_MAXUSHORT;

    fg = error = warning = success = color;

    systray_manager_set_colors (plugin->manager, &fg, &error, &warning, &success);
  }


  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (plugin->button, BUTTON_SIZE, -1);
  else
    gtk_widget_set_size_request (plugin->button, -1, BUTTON_SIZE);

  systray_plugin_button_set_arrow (plugin);
}



gboolean
systray_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                             gint             size)
{
  SnPlugin         *plugin = XFCE_SN_PLUGIN (panel_plugin);
  GtkStyleContext  *context;
  GtkBorder         padding;
  gint              border = 0;

  /* because the allocated size, used in size_requested is always 1 step
   * behind the allocated size when resizing and during startup, we
   * correct the maximum size set by the user with the size the panel
   * will most likely allocated */
  context = gtk_widget_get_style_context (plugin->box);
  gtk_style_context_get_padding (context, gtk_widget_get_state_flags (plugin->box), &padding);

  border += MAX (padding.left + padding.right, padding.top + padding.bottom);
  systray_box_set_size_alloc (XFCE_SYSTRAY_BOX (plugin->systray_box), size - 2 * border,
                              xfce_panel_plugin_get_nrows (panel_plugin));

  return TRUE;
}



static void
systray_plugin_box_draw_icon (GtkWidget *child,
                              gpointer   user_data)
{
  cairo_t       *cr = user_data;
  GtkAllocation  alloc;

  if (systray_socket_is_composited (XFCE_SYSTRAY_SOCKET (child)))
    {
      gtk_widget_get_allocation (child, &alloc);

      /* skip hidden (see offscreen in box widget) icons */
      if (alloc.x > -1 && alloc.y > -1)
        {
          // FIXME
          gdk_cairo_set_source_window (cr, gtk_widget_get_window (child),
                                       alloc.x, alloc.y);
          cairo_paint (cr);
        }
    }
}



void
systray_plugin_box_draw (GtkWidget *box,
                         cairo_t   *cr,
                         gpointer   user_data)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (user_data);
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (cr != NULL);

  /* separately draw all the composed tray icons after gtk
   * handled the draw event */
  gtk_container_foreach (GTK_CONTAINER (box),
                         (GtkCallback) (void (*)(void)) systray_plugin_box_draw_icon, cr);
}



void
systray_plugin_button_toggled (GtkWidget     *button,
                               SnPlugin      *plugin)
{
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (plugin->button == button);

  systray_box_set_show_hidden (XFCE_SYSTRAY_BOX (plugin->systray_box),
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
  systray_plugin_button_set_arrow (plugin);
}



static void
systray_plugin_button_set_arrow (SnPlugin *plugin)
{
  GtkArrowType   arrow_type;
  gboolean       show_hidden;
  GtkOrientation orientation;

  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));

  show_hidden = systray_box_get_show_hidden (XFCE_SYSTRAY_BOX (plugin->systray_box));
  orientation = xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin));
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    arrow_type = show_hidden ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT;
  else
    arrow_type = show_hidden ? GTK_ARROW_UP : GTK_ARROW_DOWN;

  xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (plugin->button), arrow_type);
}



static void
systray_plugin_names_collect (GPtrArray   *array,
                              const gchar *name)
{
  GValue *tmp;

  tmp = g_new0 (GValue, 1);
  g_value_init (tmp, G_TYPE_STRING);
  g_value_set_string (tmp, name);
  g_ptr_array_add (array, tmp);
}



static void
systray_plugin_names_collect_ordered (gpointer data,
                                      gpointer user_data)
{
  systray_plugin_names_collect (user_data, data);
}



static void
systray_plugin_names_collect_hidden (gpointer key,
                                     gpointer value,
                                     gpointer user_data)
{
  systray_plugin_names_collect (user_data, key);
}



static void
systray_plugin_names_update_icon (GtkWidget *icon,
                                  gpointer   data)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (data);
  SystraySocket *socket = XFCE_SYSTRAY_SOCKET (icon);
  const gchar   *name;

  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_SOCKET (icon));

  name = systray_socket_get_name (socket);
  systray_socket_set_hidden (socket,
      systray_plugin_names_get_hidden (plugin, name));
}



static void
systray_plugin_names_update (SnPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));

  gtk_container_foreach (GTK_CONTAINER (plugin->systray_box),
    systray_plugin_names_update_icon, plugin);
  systray_box_update (XFCE_SYSTRAY_BOX (plugin->systray_box),
    plugin->names_ordered);
}



static void
systray_plugin_names_set_hidden (SnPlugin      *plugin,
                                 const gchar   *name,
                                 gboolean       hidden)
{
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (!panel_str_is_empty (name));

  if (g_hash_table_contains (plugin->names_hidden, name))
    g_hash_table_remove (plugin->names_hidden, name);
  else
    g_hash_table_replace (plugin->names_hidden, g_strdup (name), NULL);

  systray_plugin_names_update (plugin);

  g_object_notify (G_OBJECT (plugin), "hidden-legacy-items");
}



static gboolean
systray_plugin_names_get_hidden (SnPlugin      *plugin,
                                 const gchar   *name)
{
  if (panel_str_is_empty (name))
    return FALSE;

  /* lookup the name in the list */
  if (g_slist_find_custom (plugin->names_ordered, name, (GCompareFunc)g_strcmp0) == NULL)
    {
      /* add the new name */
      plugin->names_ordered = g_slist_prepend (plugin->names_ordered, g_strdup (name));
      sn_plugin_legacy_item_added (plugin, name);

      /* do not hide the icon */
      return FALSE;
    }
  else
    {
      return g_hash_table_contains (plugin->names_hidden, name);
    }
}



static void
systray_plugin_names_clear (SnPlugin *plugin)
{
  g_slist_free_full (plugin->names_ordered, g_free);
  plugin->names_ordered = NULL;
  g_hash_table_remove_all (plugin->names_hidden);

  g_object_notify (G_OBJECT (plugin), "known-legacy-items");
  g_object_notify (G_OBJECT (plugin), "hidden-legacy-items");

  systray_plugin_names_update (plugin);
}



static void
systray_plugin_icon_added (SystrayManager *manager,
                           GtkWidget      *icon,
                           SnPlugin       *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_SOCKET (icon));
  panel_return_if_fail (plugin->manager == manager);
  panel_return_if_fail (GTK_IS_WIDGET (icon));

  systray_plugin_names_update_icon (icon, plugin);
  gtk_container_add (GTK_CONTAINER (plugin->systray_box), icon);
  gtk_widget_show (icon);

  panel_debug_filtered (PANEL_DEBUG_SYSTRAY, "added %s[%p] icon",
      systray_socket_get_name (XFCE_SYSTRAY_SOCKET (icon)), icon);
}



static void
systray_plugin_icon_removed (SystrayManager *manager,
                             GtkWidget      *icon,
                             SnPlugin       *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (plugin->manager == manager);
  panel_return_if_fail (GTK_IS_WIDGET (icon));

  /* remove the icon from the box */
  gtk_container_remove (GTK_CONTAINER (plugin->systray_box), icon);

  panel_debug_filtered (PANEL_DEBUG_SYSTRAY, "removed %s[%p] icon",
      systray_socket_get_name (XFCE_SYSTRAY_SOCKET (icon)), icon);
}



static void
systray_plugin_lost_selection (SystrayManager *manager,
                               SnPlugin       *plugin)
{
  GError error;

  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (plugin->manager == manager);

  /* create fake error and show it */
  error.message = _("Most likely another widget took over the function "
                    "of a notification area. This area will be unused.");
  xfce_dialog_show_error (NULL, &error, _("The notification area lost selection"));
}



static gchar *
systray_plugin_dialog_camel_case (const gchar *text)
{
  const gchar *t;
  gboolean     upper = TRUE;
  gunichar     c;
  GString     *result;

  panel_return_val_if_fail (!panel_str_is_empty (text), NULL);

  /* allocate a new string for the result */
  result = g_string_sized_new (32);

  /* convert the input text */
  for (t = text; *t != '\0'; t = g_utf8_next_char (t))
    {
      /* check the next char */
      c = g_utf8_get_char (t);
      if (g_unichar_isspace (c))
        {
          upper = TRUE;
        }
      else if (upper)
        {
          c = g_unichar_toupper (c);
          upper = FALSE;
        }
      else
        {
          c = g_unichar_tolower (c);
        }

      /* append the char to the result */
      g_string_append_unichar (result, c);
    }

  return g_string_free (result, FALSE);
}



static void
systray_plugin_dialog_add_application_names (gpointer data,
                                             gpointer user_data)
{
  gpointer      **user_data_array = user_data;
  SnPlugin       *plugin = (SnPlugin *)user_data_array[0];
  GtkListStore   *store = (GtkListStore *)user_data_array[1];
  const gchar    *name = data;
  gboolean        hidden = g_hash_table_contains (plugin->names_hidden, name);
  const gchar    *title = NULL;
  gchar          *camelcase = NULL;
  const gchar    *icon_name = name;
  GdkPixbuf      *pixbuf;
  GIcon          *gicon;
  guint           i;
  GtkTreeIter     iter;

  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  panel_return_if_fail (name == NULL || g_utf8_validate (name, -1, NULL));

  /* skip invalid names */
  if (panel_str_is_empty (name))
     return;

  /* check if we have a better name for the application */
  for (i = 0; i < G_N_ELEMENTS (known_applications); i++)
    {
      if (strcmp (name, known_applications[i][0]) == 0)
        {
          icon_name = known_applications[i][1];
          title = known_applications[i][2];
          break;
        }
    }

  /* create fallback title if the application was not found */
  if (title == NULL)
    {
      camelcase = systray_plugin_dialog_camel_case (name);
      title = camelcase;
    }

  /* try to load the icon name */
  if (G_LIKELY (icon_name != NULL))
    gicon = xfce_gicon_from_name (icon_name);
  else
    gicon = xfce_gicon_from_name (name);

  /* insert in the store */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COLUMN_GICON, gicon,
                      COLUMN_TITLE, title,
                      COLUMN_HIDDEN, hidden,
                      COLUMN_INTERNAL_NAME, name,
                      -1);

  g_free (camelcase);
  if (pixbuf != NULL)
    g_object_unref (G_OBJECT (pixbuf));
}



static void
systray_plugin_dialog_hidden_toggled (GtkCellRendererToggle *renderer,
                                      const gchar           *path_string,
                                      SnPlugin              *plugin)
{
  GtkTreeIter   iter;
  gboolean      hidden;
  GtkTreeModel *model;
  gchar        *name;

  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (plugin->systray_box));

  model = GTK_TREE_MODEL (gtk_builder_get_object (plugin->configure_builder, "applications-store"));
  panel_return_if_fail (GTK_IS_LIST_STORE (model));
  if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
    {
      gtk_tree_model_get (model, &iter,
                          COLUMN_HIDDEN, &hidden,
                          COLUMN_INTERNAL_NAME, &name, -1);

      /* insert value (we need to update it) */
      hidden = !hidden;

      /* update box and store with new state */
      systray_plugin_names_set_hidden (plugin, name, hidden);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 2, hidden, -1);

      g_free (name);
    }
}



static void
systray_plugin_dialog_selection_changed (GtkTreeSelection *selection,
                                         SnPlugin         *plugin)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GtkTreePath  *path;
  gint         *indices;
  gint          count = 0, position = -1, depth;
  gboolean      item_up_sensitive, item_down_sensitive;
  GObject      *object;

  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      path = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices_with_depth (path, &depth);

      if (indices != NULL && depth > 0)
        position = indices[0];

      count = gtk_tree_model_iter_n_children (model, NULL);

      gtk_tree_path_free (path);
    }

  item_up_sensitive = position > 0;
  item_down_sensitive = position + 1 < count;

  object = gtk_builder_get_object (plugin->configure_builder, "item-up");
  if (GTK_IS_BUTTON (object))
    gtk_widget_set_sensitive (GTK_WIDGET (object), item_up_sensitive);

  object = gtk_builder_get_object (plugin->configure_builder, "item-down");
  if (GTK_IS_BUTTON (object))
    gtk_widget_set_sensitive (GTK_WIDGET (object), item_down_sensitive);
}



static gboolean
systray_plugin_dialog_tree_iter_insert (GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        gpointer      user_data)
{
  SnPlugin      *plugin = user_data;
  gchar         *name;

  gtk_tree_model_get (model, iter, COLUMN_INTERNAL_NAME, &name, -1);
  plugin->names_ordered = g_slist_prepend (plugin->names_ordered, g_strdup (name));

  return FALSE;
}



static void
systray_plugin_dialog_item_move_clicked (GtkWidget     *button,
                                         SnPlugin      *plugin)
{
  GObject          *object;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter, from_iter;
  GtkTreePath      *path;
  gint              direction;

  object = gtk_builder_get_object (plugin->configure_builder, "applications-treeview");
  panel_return_if_fail (GTK_IS_TREE_VIEW (object));

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (object));

  object = gtk_builder_get_object (plugin->configure_builder, "item-up");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  direction = object == G_OBJECT (button) ? -1 : 1;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      path = gtk_tree_model_get_path (model, &iter);

      if (direction > 0)
        gtk_tree_path_next (path);
      else
        gtk_tree_path_prev (path);

      if (gtk_tree_model_get_iter (model, &from_iter, path))
        {
          if (direction > 0)
            gtk_list_store_move_after (GTK_LIST_STORE (model), &iter, &from_iter);
          else
            gtk_list_store_move_before (GTK_LIST_STORE (model), &iter, &from_iter);

          /* update buttons state */
          systray_plugin_dialog_selection_changed (selection, plugin);

          /* update order */
          g_slist_free_full (plugin->names_ordered, g_free);
          plugin->names_ordered = NULL;
          gtk_tree_model_foreach (model, systray_plugin_dialog_tree_iter_insert, plugin);
          plugin->names_ordered = g_slist_reverse (plugin->names_ordered);
          g_object_notify (G_OBJECT (plugin), "known-legacy-items");
        }

      gtk_tree_path_free (path);
    }
}



static void
systray_plugin_dialog_clear_clicked (GtkWidget     *button,
                                     SnPlugin      *plugin)
{
  GtkListStore *store;

  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (plugin->systray_box));

  if (xfce_dialog_confirm (GTK_WINDOW (gtk_widget_get_toplevel (button)),
                           "edit-clear", _("Clear"), NULL,
                           _("Are you sure you want to clear the list of "
                             "known applications?")))
    {
      store = GTK_LIST_STORE (gtk_builder_get_object (plugin->configure_builder, "applications-store"));
      panel_return_if_fail (GTK_IS_LIST_STORE (store));
      gtk_list_store_clear (store);

      systray_plugin_names_clear (plugin);
    }
}



static void
systray_plugin_dialog_cleanup (SnPlugin      *plugin,
                               GtkBuilder    *builder)
{
  panel_return_if_fail (XFCE_IS_SN_PLUGIN (plugin));

  if (plugin->configure_builder == builder)
    plugin->configure_builder = NULL;
}
