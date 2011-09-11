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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <common/panel-debug.h>
#include <exo/exo.h>

#include "systray.h"
#include "systray-box.h"
#include "systray-socket.h"
#include "systray-manager.h"
#include "systray-dialog_ui.h"

#define ICON_SIZE     (22)
#define BUTTON_SIZE   (16)
#define FRAME_SPACING (1)


static void     systray_plugin_get_property                 (GObject               *object,
                                                             guint                  prop_id,
                                                             GValue                *value,
                                                             GParamSpec            *pspec);
static void     systray_plugin_set_property                 (GObject               *object,
                                                             guint                  prop_id,
                                                             const GValue          *value,
                                                             GParamSpec            *pspec);
static void     systray_plugin_construct                    (XfcePanelPlugin       *panel_plugin);
static void     systray_plugin_free_data                    (XfcePanelPlugin       *panel_plugin);
static void     systray_plugin_orientation_changed          (XfcePanelPlugin       *panel_plugin,
                                                             GtkOrientation         orientation);
static gboolean systray_plugin_size_changed                 (XfcePanelPlugin       *panel_plugin,
                                                             gint                   size);
static void     systray_plugin_configure_plugin             (XfcePanelPlugin       *panel_plugin);
static void     systray_plugin_box_expose_event             (GtkWidget             *box,
                                                             GdkEventExpose        *event);
static void     systray_plugin_button_toggled               (GtkWidget             *button,
                                                             SystrayPlugin         *plugin);
static void     systray_plugin_button_set_arrow             (SystrayPlugin         *plugin);
static void     systray_plugin_names_collect_visible        (gpointer               key,
                                                             gpointer               value,
                                                             gpointer               user_data);
static void     systray_plugin_names_collect_hidden         (gpointer               key,
                                                             gpointer               value,
                                                             gpointer               user_data);
static gboolean systray_plugin_names_remove                 (gpointer               key,
                                                             gpointer               value,
                                                             gpointer               user_data);
static void     systray_plugin_names_update                 (SystrayPlugin         *plugin);
static gboolean systray_plugin_names_get_hidden             (SystrayPlugin         *plugin,
                                                             const gchar           *name);
static void     systray_plugin_icon_added                   (SystrayManager        *manager,
                                                             GtkWidget             *icon,
                                                             SystrayPlugin         *plugin);
static void     systray_plugin_icon_removed                 (SystrayManager        *manager,
                                                             GtkWidget             *icon,
                                                             SystrayPlugin         *plugin);
static void     systray_plugin_lost_selection               (SystrayManager        *manager,
                                                             SystrayPlugin         *plugin);
static void     systray_plugin_dialog_add_application_names (gpointer               key,
                                                             gpointer               value,
                                                             gpointer               user_data);
static void     systray_plugin_dialog_hidden_toggled        (GtkCellRendererToggle *renderer,
                                                             const gchar           *path_string,
                                                             SystrayPlugin         *plugin);
static void     systray_plugin_dialog_clear_clicked         (GtkWidget             *button,
                                                             SystrayPlugin         *plugin);



struct _SystrayPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _SystrayPlugin
{
  XfcePanelPlugin __parent__;

  /* systray manager */
  SystrayManager *manager;

  guint           idle_startup;

  /* widgets */
  GtkWidget      *frame;
  GtkWidget      *hvbox;
  GtkWidget      *box;
  GtkWidget      *button;

  /* settings */
  guint           show_frame : 1;
  GHashTable     *names;
};

enum
{
  PROP_0,
  PROP_SIZE_MAX,
  PROP_SHOW_FRAME,
  PROP_NAMES_HIDDEN,
  PROP_NAMES_VISIBLE
};

enum
{
  COLUMN_PIXBUF,
  COLUMN_TITLE,
  COLUMN_HIDDEN,
  COLUMN_INTERNAL_NAME
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (SystrayPlugin, systray_plugin,
    systray_box_register_type,
    systray_manager_register_type,
    systray_socket_register_type)



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
  { "xfce4-power-manager", "xfpm-ac-adapter", "Xfce Power Manager" },
};



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
  plugin_class->configure_plugin = systray_plugin_configure_plugin;
  plugin_class->orientation_changed = systray_plugin_orientation_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE_MAX,
                                   g_param_spec_uint ("size-max",
                                                      NULL, NULL,
                                                      SIZE_MAX_MIN,
                                                      SIZE_MAX_MAX,
                                                      SIZE_MAX_DEFAULT,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_FRAME,
                                   g_param_spec_boolean ("show-frame",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_NAMES_HIDDEN,
                                   g_param_spec_boxed ("names-hidden",
                                                       NULL, NULL,
                                                       PANEL_PROPERTIES_TYPE_VALUE_ARRAY,
                                                       EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_NAMES_VISIBLE,
                                   g_param_spec_boxed ("names-visible",
                                                       NULL, NULL,
                                                       PANEL_PROPERTIES_TYPE_VALUE_ARRAY,
                                                       EXO_PARAM_READWRITE));
}



static void
systray_plugin_init (SystrayPlugin *plugin)
{
  GtkRcStyle *style;

  plugin->manager = NULL;
  plugin->show_frame = TRUE;
  plugin->idle_startup = 0;
  plugin->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  plugin->frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->frame);
  gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), GTK_SHADOW_ETCHED_IN);
  gtk_widget_show (plugin->frame);

  style = gtk_rc_style_new ();
  style->xthickness = style->ythickness = 1;
  gtk_widget_modify_style (plugin->frame, style);
  g_object_unref (G_OBJECT (style));

  plugin->hvbox = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 2);
  gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->hvbox);
  gtk_widget_show (plugin->hvbox);

  plugin->box = systray_box_new ();
  gtk_box_pack_start (GTK_BOX (plugin->hvbox), plugin->box, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (plugin->box), "expose-event",
      G_CALLBACK (systray_plugin_box_expose_event), NULL);
  gtk_container_set_border_width (GTK_CONTAINER (plugin->box), FRAME_SPACING);
  gtk_widget_show (plugin->box);

  plugin->button = xfce_arrow_button_new (GTK_ARROW_RIGHT);
  gtk_box_pack_start (GTK_BOX (plugin->hvbox), plugin->button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (plugin->button), "toggled",
      G_CALLBACK (systray_plugin_button_toggled), plugin);
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  exo_binding_new (G_OBJECT (plugin->box), "has-hidden", G_OBJECT (plugin->button), "visible");
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
}



static void
systray_plugin_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (object);
  GPtrArray     *array;

  switch (prop_id)
    {
    case PROP_SIZE_MAX:
      g_value_set_uint (value,
          systray_box_get_size_max (XFCE_SYSTRAY_BOX (plugin->box)));
      break;

    case PROP_SHOW_FRAME:
      g_value_set_boolean (value, plugin->show_frame);
      break;

   case PROP_NAMES_VISIBLE:
      array = g_ptr_array_new ();
      g_hash_table_foreach (plugin->names, systray_plugin_names_collect_visible, array);
      g_value_set_boxed (value, array);
      xfconf_array_free (array);
      break;

    case PROP_NAMES_HIDDEN:
      array = g_ptr_array_new ();
      g_hash_table_foreach (plugin->names, systray_plugin_names_collect_hidden, array);
      g_value_set_boxed (value, array);
      xfconf_array_free (array);
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
  gboolean       show_frame;
  gboolean       hidden = TRUE;
  GPtrArray     *array;
  const GValue  *tmp;
  gchar         *name;
  guint          i;
  GtkRcStyle    *style;

  switch (prop_id)
    {
    case PROP_SIZE_MAX:
      systray_box_set_size_max (XFCE_SYSTRAY_BOX (plugin->box),
                                g_value_get_uint (value));
      break;

    case PROP_SHOW_FRAME:
      show_frame = g_value_get_boolean (value);
      if (plugin->show_frame != show_frame)
        {
          plugin->show_frame = show_frame;
          gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame),
              show_frame ? GTK_SHADOW_ETCHED_IN : GTK_SHADOW_NONE);

          style = gtk_rc_style_new ();
          style->xthickness = style->ythickness = show_frame ? 1 : 0;
          gtk_widget_modify_style (plugin->frame, style);
          g_object_unref (G_OBJECT (style));

          systray_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
              xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
        }
      break;

    case PROP_NAMES_VISIBLE:
      hidden = FALSE;
      /* fall-though */

    case PROP_NAMES_HIDDEN:
      /* remove old names with this state */
      g_hash_table_foreach_remove (plugin->names,
                                   systray_plugin_names_remove,
                                   GUINT_TO_POINTER (hidden));

      /* add new values */
      array = g_value_get_boxed (value);
      if (G_LIKELY (array != NULL))
        {
          for (i = 0; i < array->len; i++)
            {
              tmp = g_ptr_array_index (array, i);
              panel_assert (G_VALUE_HOLDS_STRING (tmp));
              name = g_value_dup_string (tmp);
              g_hash_table_replace (plugin->names, name, GUINT_TO_POINTER (hidden));
            }
        }

      /* update icons in the box */
      systray_plugin_names_update (plugin);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
systray_plugin_screen_changed_idle (gpointer user_data)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (user_data);
  GdkScreen     *screen;
  GError        *error = NULL;

  GDK_THREADS_ENTER ();

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

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
systray_plugin_screen_changed_idle_destroyed (gpointer user_data)
{
  XFCE_SYSTRAY_PLUGIN (user_data)->idle_startup = 0;
}



static void
systray_plugin_screen_changed (GtkWidget *widget,
                               GdkScreen *previous_screen)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (widget);

  if (G_UNLIKELY (plugin->manager != NULL))
    {
      /* unregister this screen screen */
      systray_manager_unregister (plugin->manager);
      g_object_unref (G_OBJECT (plugin->manager));
      plugin->manager = NULL;
    }

  /* schedule a delayed startup */
  if (plugin->idle_startup == 0)
    plugin->idle_startup = g_idle_add_full (G_PRIORITY_LOW, systray_plugin_screen_changed_idle,
                                            plugin, systray_plugin_screen_changed_idle_destroyed);
}


static void
systray_plugin_composited_changed (GtkWidget *widget)
{
  /* restart the manager to add the sockets again */
  systray_plugin_screen_changed (widget, gtk_widget_get_screen (widget));
}



static void
systray_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin       *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "size-max", G_TYPE_UINT },
    { "show-frame", G_TYPE_BOOLEAN },
    { "names-visible", PANEL_PROPERTIES_TYPE_VALUE_ARRAY },
    { "names-hidden", PANEL_PROPERTIES_TYPE_VALUE_ARRAY },
    { NULL }
  };

  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* monitor screen changes */
  g_signal_connect (G_OBJECT (plugin), "screen-changed",
      G_CALLBACK (systray_plugin_screen_changed), NULL);
  systray_plugin_screen_changed (GTK_WIDGET (plugin), NULL);

  /* restart internally if compositing changed */
  g_signal_connect (G_OBJECT (plugin), "composited-changed",
     G_CALLBACK (systray_plugin_composited_changed), NULL);
}



static void
systray_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  /* stop pending idle startup */
  if (plugin->idle_startup != 0)
    g_source_remove (plugin->idle_startup);

  /* disconnect screen changed signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
      systray_plugin_screen_changed, NULL);

  g_hash_table_destroy (plugin->names);

  if (G_LIKELY (plugin->manager != NULL))
    {
      systray_manager_unregister (plugin->manager);
      g_object_unref (G_OBJECT (plugin->manager));
    }
}



static void
systray_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                    GtkOrientation   orientation)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);

  xfce_hvbox_set_orientation (XFCE_HVBOX (plugin->hvbox), orientation);
  systray_box_set_orientation (XFCE_SYSTRAY_BOX (plugin->box), orientation);

  if (G_LIKELY (plugin->manager != NULL))
    systray_manager_set_orientation (plugin->manager, orientation);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (plugin->button, BUTTON_SIZE, -1);
  else
    gtk_widget_set_size_request (plugin->button, -1, BUTTON_SIZE);

  systray_plugin_button_set_arrow (plugin);
}



static gboolean
systray_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                             gint             size)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);
  GtkWidget     *frame = plugin->frame;
  gint           border = 0;

  /* set the frame border */
  if (plugin->show_frame && size > 26)
    border = 1;
  gtk_container_set_border_width (GTK_CONTAINER (frame), border);

  /* because the allocated size, used in size_requested is always 1 step
   * behind the allocated size when resizing and during startup, we
   * correct the maximum size set by the user with the size the panel
   * will most likely allocated */
  border += MAX (frame->style->xthickness, frame->style->ythickness);
  systray_box_set_size_alloc (XFCE_SYSTRAY_BOX (plugin->box), size - 2 * border);

  return TRUE;
}



static void
systray_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (panel_plugin);
  GtkBuilder    *builder;
  GObject       *dialog, *object, *store;

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, systray_dialog_ui,
                                     systray_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  object = gtk_builder_get_object (builder, "size-max");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "size-max",
                          G_OBJECT (object), "value");

  object = gtk_builder_get_object (builder, "show-frame");
  panel_return_if_fail (GTK_IS_WIDGET (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "show-frame",
                          G_OBJECT (object), "active");

  store = gtk_builder_get_object (builder, "applications-store");
  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  g_hash_table_foreach (plugin->names,
      systray_plugin_dialog_add_application_names, store);
  g_object_set_data (G_OBJECT (plugin), "applications-store", store);

  object = gtk_builder_get_object (builder, "hidden-toggle");
  panel_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (object));
  g_signal_connect (G_OBJECT (object), "toggled",
      G_CALLBACK (systray_plugin_dialog_hidden_toggled), plugin);

  object = gtk_builder_get_object (builder, "applications-clear");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  g_signal_connect (G_OBJECT (object), "clicked",
      G_CALLBACK (systray_plugin_dialog_clear_clicked), plugin);

  gtk_widget_show (GTK_WIDGET (dialog));
}



static void
systray_plugin_box_expose_event_icon (GtkWidget *child,
                                      gpointer   user_data)
{
  cairo_t       *cr = user_data;
  GtkAllocation *alloc;

  if (systray_socket_is_composited (XFCE_SYSTRAY_SOCKET (child)))
    {
      alloc = &child->allocation;

      /* skip hidden (see offscreen in box widget) icons */
      if (alloc->x > -1 && alloc->y > -1)
        {
          gdk_cairo_set_source_pixmap (cr, gtk_widget_get_window (child),
                                       alloc->x, alloc->y);
          cairo_paint (cr);
        }
    }
}



static void
systray_plugin_box_expose_event (GtkWidget      *box,
                                 GdkEventExpose *event)
{
  cairo_t *cr;

  if (!gtk_widget_is_composited (box))
    return;

  cr = gdk_cairo_create (gtk_widget_get_window (box));
  if (G_LIKELY (cr != NULL))
    {
      gdk_cairo_rectangle (cr, &event->area);
      cairo_clip (cr);

      /* separately draw all the composed tray icons after gtk
       * handled the expose event */
      gtk_container_foreach (GTK_CONTAINER (box),
          systray_plugin_box_expose_event_icon, cr);

      cairo_destroy (cr);
    }
}



static void
systray_plugin_button_toggled (GtkWidget     *button,
                               SystrayPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (plugin->button == button);

  systray_box_set_show_hidden (XFCE_SYSTRAY_BOX (plugin->box),
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
  systray_plugin_button_set_arrow (plugin);
}



static void
systray_plugin_button_set_arrow (SystrayPlugin *plugin)
{
  GtkArrowType   arrow_type;
  gboolean       show_hidden;
  GtkOrientation orientation;

  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));

  show_hidden = systray_box_get_show_hidden (XFCE_SYSTRAY_BOX (plugin->box));
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
systray_plugin_names_collect_visible (gpointer key,
                                      gpointer value,
                                      gpointer user_data)
{
  /* add all the visible names */
  if (!GPOINTER_TO_UINT (value))
    systray_plugin_names_collect (user_data, key);
}



static void
systray_plugin_names_collect_hidden (gpointer key,
                                     gpointer value,
                                     gpointer user_data)
{
  /* add all the hidden names */
  if (GPOINTER_TO_UINT (value))
    systray_plugin_names_collect (user_data, key);
}



static gboolean
systray_plugin_names_remove (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  return GPOINTER_TO_UINT (value) == GPOINTER_TO_UINT (user_data);
}



static void
systray_plugin_names_update_icon (GtkWidget *icon,
                                  gpointer   data)
{
  SystrayPlugin *plugin = XFCE_SYSTRAY_PLUGIN (data);
  SystraySocket *socket = XFCE_SYSTRAY_SOCKET (icon);
  const gchar   *name;

  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_SOCKET (icon));

  name = systray_socket_get_name (socket);
  systray_socket_set_hidden (socket,
      systray_plugin_names_get_hidden (plugin, name));
}



static void
systray_plugin_names_update (SystrayPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));

  gtk_container_foreach (GTK_CONTAINER (plugin->box),
     systray_plugin_names_update_icon, plugin);
  systray_box_update (XFCE_SYSTRAY_BOX (plugin->box));
}



static void
systray_plugin_names_set_hidden (SystrayPlugin *plugin,
                                 const gchar   *name,
                                 gboolean       hidden)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (!exo_str_is_empty (name));

  g_hash_table_replace (plugin->names, g_strdup (name),
                        GUINT_TO_POINTER (hidden ? 1 : 0));

  systray_plugin_names_update (plugin);

  g_object_notify (G_OBJECT (plugin), "names-visible");
  g_object_notify (G_OBJECT (plugin), "names-hidden");
}



static gboolean
systray_plugin_names_get_hidden (SystrayPlugin *plugin,
                                 const gchar   *name)
{
  gpointer p;

  if (exo_str_is_empty (name))
    return FALSE;

  /* lookup the name in the table */
  p = g_hash_table_lookup (plugin->names, name);
  if (G_UNLIKELY (p == NULL))
    {
      /* add the new name */
      g_hash_table_insert (plugin->names, g_strdup (name), GUINT_TO_POINTER (0));
      g_object_notify (G_OBJECT (plugin), "names-visible");

      /* do not hide the icon */
      return FALSE;
    }
  else
    {
      return (GPOINTER_TO_UINT (p) == 1 ? TRUE : FALSE);
    }
}



static void
systray_plugin_names_clear (SystrayPlugin *plugin)
{
  g_hash_table_remove_all (plugin->names);

  g_object_notify (G_OBJECT (plugin), "names-hidden");
  g_object_notify (G_OBJECT (plugin), "names-visible");

  systray_plugin_names_update (plugin);
}



static void
systray_plugin_icon_added (SystrayManager *manager,
                           GtkWidget      *icon,
                           SystrayPlugin  *plugin)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_MANAGER (manager));
  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_SOCKET (icon));
  panel_return_if_fail (plugin->manager == manager);
  panel_return_if_fail (GTK_IS_WIDGET (icon));

  systray_plugin_names_update_icon (icon, plugin);
  gtk_container_add (GTK_CONTAINER (plugin->box), icon);
  gtk_widget_show (icon);

  panel_debug_filtered (PANEL_DEBUG_SYSTRAY, "added %s[%p] icon",
      systray_socket_get_name (XFCE_SYSTRAY_SOCKET (icon)), icon);
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

  panel_debug_filtered (PANEL_DEBUG_SYSTRAY, "removed %s[%p] icon",
      systray_socket_get_name (XFCE_SYSTRAY_SOCKET (icon)), icon);
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

  panel_return_val_if_fail (!exo_str_is_empty (text), NULL);

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
systray_plugin_dialog_add_application_names (gpointer key,
                                             gpointer value,
                                             gpointer user_data)
{
  GtkListStore *store = GTK_LIST_STORE (user_data);
  const gchar  *name = key;
  gboolean      hidden = GPOINTER_TO_UINT (value);
  const gchar  *title = NULL;
  gchar        *camelcase = NULL;
  const gchar  *icon_name = name;
  GdkPixbuf    *pixbuf;
  guint         i;
  GtkTreeIter   iter;

  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  panel_return_if_fail (name == NULL || g_utf8_validate (name, -1, NULL));

  /* skip invalid names */
  if (exo_str_is_empty (name))
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
    pixbuf = xfce_panel_pixbuf_from_source (icon_name, NULL, ICON_SIZE);
  else
    pixbuf = NULL;

  /* insert in the store */
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      COLUMN_PIXBUF, pixbuf,
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
                                      SystrayPlugin         *plugin)
{
  GtkTreeIter   iter;
  gboolean      hidden;
  GtkTreeModel *model;
  gchar        *name;

  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (plugin->box));

  model = g_object_get_data (G_OBJECT (plugin), "applications-store");
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
systray_plugin_dialog_clear_clicked (GtkWidget     *button,
                                     SystrayPlugin *plugin)
{
  GtkListStore *store;

  panel_return_if_fail (XFCE_IS_SYSTRAY_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (plugin->box));

  if (xfce_dialog_confirm (GTK_WINDOW (gtk_widget_get_toplevel (button)),
                           GTK_STOCK_CLEAR, NULL, NULL,
                           _("Are you sure you want to clear the list of "
                             "known applications?")))
    {
      store = g_object_get_data (G_OBJECT (plugin), "applications-store");
      panel_return_if_fail (GTK_IS_LIST_STORE (store));
      gtk_list_store_clear (store);

      systray_plugin_names_clear (plugin);
    }
}
