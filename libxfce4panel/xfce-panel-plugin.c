/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>

#include <libxfce4panel/libxfce4panel-marshal.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#define XFCE_PANEL_PLUGIN_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XFCE_TYPE_PANEL_PLUGIN, XfcePanelPluginPrivate))

typedef const gchar *(*ProviderToPlugin) (XfcePanelPluginProvider *provider);


static void         xfce_panel_plugin_class_init             (XfcePanelPluginClass         *klass);
static void         xfce_panel_plugin_init                   (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_provider_init          (XfcePanelPluginProviderIface *iface);
static void         xfce_panel_plugin_get_property           (GObject                      *object,
                                                              guint                         prop_id,
                                                              GValue                       *value,
                                                              GParamSpec                   *pspec);
static void         xfce_panel_plugin_set_property           (GObject                      *object,
                                                              guint                         prop_id,
                                                              const GValue                 *value,
                                                              GParamSpec                   *pspec);
static void         xfce_panel_plugin_dispose                (GObject                      *object);
static void         xfce_panel_plugin_finalize               (GObject                      *object);
static gboolean     xfce_panel_plugin_button_press_event     (GtkWidget                    *widget,
                                                              GdkEventButton               *event);
static void         xfce_panel_plugin_menu_properties        (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_menu_about             (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_menu_move              (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_menu_remove            (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_menu_add_items         (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_menu_panel_preferences (XfcePanelPlugin              *plugin);
static GtkMenu     *xfce_panel_plugin_menu_get               (XfcePanelPlugin              *plugin);
static gchar       *xfce_panel_plugin_relative_filename      (XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_unregister_menu        (GtkMenu                      *menu,
                                                              XfcePanelPlugin              *plugin);
static void         xfce_panel_plugin_set_size               (XfcePanelPluginProvider      *provider,
                                                              gint                          size);
static void         xfce_panel_plugin_set_orientation        (XfcePanelPluginProvider      *provider,
                                                              GtkOrientation                orientation);
static void         xfce_panel_plugin_set_screen_position    (XfcePanelPluginProvider      *provider,
                                                              XfceScreenPosition            screen_position);
static void         xfce_panel_plugin_save                   (XfcePanelPluginProvider      *provider);
static void         xfce_panel_plugin_take_window_notify     (gpointer                      data,
                                                              GObject                      *where_the_object_was);



enum
{
  PROP_0,
  PROP_NAME,
  PROP_DISPLAY_NAME,
  PROP_ARGUMENTS,
  PROP_ID
};

enum
{
  ABOUT,
  CONFIGURE_PLUGIN,
  FREE_DATA,
  ORIENTATION_CHANGED,
  SAVE,
  SIZE_CHANGED,
  SCREEN_POSITION_CHANGED,
  LAST_SIGNAL
};

struct _XfcePanelPluginPrivate
{
  /* plugin information */
  gchar               *name;
  gchar               *display_name;
  gchar               *id;
  gchar              **arguments;
  gint                 size;
  guint                expand : 1;
  GtkOrientation       orientation;
  XfceScreenPosition   screen_position;

  /* plugin menu */
  GtkMenu             *menu;

  /* menu block counter */
  guint                menu_blocked;

  /* registered menu counter */
  guint                registered_menus;
};



static guint plugin_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (XfcePanelPlugin, xfce_panel_plugin, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN_PROVIDER, xfce_panel_plugin_provider_init));



static void
xfce_panel_plugin_class_init (XfcePanelPluginClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  /* add private data */
  g_type_class_add_private (klass, sizeof (XfcePanelPluginPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_panel_plugin_get_property;
  gobject_class->set_property = xfce_panel_plugin_set_property;
  gobject_class->dispose = xfce_panel_plugin_dispose;
  gobject_class->finalize = xfce_panel_plugin_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = xfce_panel_plugin_button_press_event;

  /**
   * XfcePanelPlugin::about
   * @plugin : an #XfcePanelPlugin.
   *
   * This signal is emmitted when the About entry in the right-click
   * menu is clicked. Plugin writes can use it to show information
   * about the plugin and display credits of the developers, translators
   * and other contributors.
   *
   * See also: xfce_panel_plugin_menu_show_about().
   **/
  plugin_signals[ABOUT] =
    g_signal_new (I_("about"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, about),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfcePanelPlugin::configure-plugin
   * @plugin : an #XfcePanelPlugin.
   *
   * This signal is emmitted when the Properties entry in the right-click
   * menu is clicked. Plugin writes can use this signal to open a
   * plugin settings dialog.
   *
   * See also: xfce_panel_plugin_menu_show_configure() and
   *           xfce_titled_dialog_new ().
   **/
  plugin_signals[CONFIGURE_PLUGIN] =
    g_signal_new (I_("configure-plugin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, configure_plugin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfcePanelPlugin::free-data
   * @plugin : an #XfcePanelPlugin.
   *
   * This signal is emmitted when the plugin is closing. Plugin
   * writers should use this signal to free any allocated resources.
   *
   * See also #XfceHVBox.
   **/
  plugin_signals[FREE_DATA] =
    g_signal_new (I_("free-data"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, free_data),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfcePanelPlugin::orientation-changed
   * @plugin      : an #XfcePanelPlugin.
   * @orientation : new #GtkOrientation of the panel.
   *
   * This signal is emmitted whenever the orientation of the panel
   * the @plugin is on changes. Plugins writers can for example use
   * this signal to change the order of widgets in the plugin.
   *
   * See also: #XfceHVBox.
   **/
  plugin_signals[ORIENTATION_CHANGED] =
    g_signal_new (I_("orientation-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, orientation_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1, GTK_TYPE_ORIENTATION);

  /**
   * XfcePanelPlugin::save
   * @plugin : an #XfcePanelPlugin.
   *
   * This signal is emitted when the plugin should save it's
   * configuration. The signal is always emmitted before the plugin
   * closes (before the "free-data" signal) and also once in 10
   * minutes or so.
   *
   * See also: xfce_panel_plugin_save_location().
   **/
  plugin_signals[SAVE] =
    g_signal_new (I_("save"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, save),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfcePanelPlugin::size-changed
   * @plugin : an #XfcePanelPlugin.
   * @size   : the new size of the panel.
   *
   * This signal is emmitted whenever the size of the panel
   * the @plugin is on changes. Plugins writers can for example use
   * this signal to update their icon size.
   **/
  plugin_signals[SIZE_CHANGED] =
    g_signal_new (I_("size-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, size_changed),
                  g_signal_accumulator_true_handled, NULL,
                  _libxfce4panel_marshal_BOOLEAN__INT,
                  G_TYPE_BOOLEAN, 1, G_TYPE_INT);

  /**
   * XfcePanelPlugin::screen-position-changed
   * @plugin   : an #XfcePanelPlugin.
   * @position : the new screen position of the panel.
   *
   * This signal is emmitted whenever the screen position of the panel
   * the @plugin is on changes. Plugins writers can for example use
   * this signal to change the arrow direction of buttons.
   **/
  plugin_signals[SCREEN_POSITION_CHANGED] =
    g_signal_new (I_("screen-position-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfcePanelPluginClass, screen_position_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE, 1, G_TYPE_INT);

  /**
   * XfcePanelPlugin:name:
   *
   * The internal, unstranslated, name of the #XfcePanelPlugin. Plugin
   * writer can use it to read the plugin name, but
   * xfce_panel_plugin_get_name() is recommended since that
   * returns a const string.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Plugin internal name",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS
                                                        | G_PARAM_CONSTRUCT_ONLY));

  /**
   * XfcePanelPlugin:display-name:
   *
   * The display name of the #XfcePanelPlugin. This property is used during plugin
   * construction and can't be set twice. Plugin writer can use it to read the
   * plugin display name, but xfce_panel_plugin_get_display_name() is recommended
   * since that returns a const string.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_DISPLAY_NAME,
                                   g_param_spec_string ("display-name",
                                                        "Display Name",
                                                        "Plugin display name",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS
                                                        | G_PARAM_CONSTRUCT_ONLY));

  /**
   * XfcePanelPlugin:id:
   *
   * The unique id of the #XfcePanelPlugin. Plugin writer can use it to
   * read the unique id, but xfce_panel_plugin_get_id() is recommended
   * since that returns a const string.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ID,
                                   g_param_spec_string ("id",
                                                        "ID",
                                                        "Unique plugin ID",
                                                        NULL,
                                                        G_PARAM_READWRITE
                                                        | G_PARAM_STATIC_STRINGS
                                                        | G_PARAM_CONSTRUCT_ONLY));

  /**
   * XfcePanelPlugin:arguments:
   *
   * The arguments the plugin was started with. If the plugin was not
   * started with any arguments this value is %NULL. Plugin writer can
   * use it to read the arguments array, but
   * xfce_panel_plugin_get_arguments() is recommended.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ARGUMENTS,
                                   g_param_spec_boxed ("arguments",
                                                       "Arguments",
                                                       "Startup arguments for the plugin",
                                                       G_TYPE_STRV,
                                                       G_PARAM_READWRITE
                                                       | G_PARAM_STATIC_STRINGS
                                                       | G_PARAM_CONSTRUCT_ONLY));

  /** TODO properties for size, expand, orientation, screen-position **/
}



static void
xfce_panel_plugin_init (XfcePanelPlugin *plugin)
{
  /* set private pointer */
  plugin->priv = XFCE_PANEL_PLUGIN_GET_PRIVATE (plugin);

  /* initialize plugin value */
  plugin->priv->name = NULL;
  plugin->priv->display_name = NULL;
  plugin->priv->id = NULL;
  plugin->priv->arguments = NULL;
  plugin->priv->size = 0;
  plugin->priv->expand = FALSE;
  plugin->priv->orientation = GTK_ORIENTATION_HORIZONTAL;
  plugin->priv->screen_position = XFCE_SCREEN_POSITION_NONE;
  plugin->priv->menu = NULL;
  plugin->priv->menu_blocked = 0;
  plugin->priv->registered_menus = 0;

  /* hide the event box window to make the plugin transparent */
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (plugin), FALSE);
}



static void
xfce_panel_plugin_provider_init (XfcePanelPluginProviderIface *iface)
{
  iface->get_name = (ProviderToPlugin) xfce_panel_plugin_get_name;
  iface->get_id = (ProviderToPlugin) xfce_panel_plugin_get_id;
  iface->set_size = xfce_panel_plugin_set_size;
  iface->set_orientation = xfce_panel_plugin_set_orientation;
  iface->set_screen_position = xfce_panel_plugin_set_screen_position;
  iface->save = xfce_panel_plugin_save;
}



static void
xfce_panel_plugin_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  XfcePanelPluginPrivate *private = XFCE_PANEL_PLUGIN (object)->priv;

  switch (prop_id)
    {
      case PROP_NAME:
        g_value_set_static_string (value, private->name);
        break;

      case PROP_DISPLAY_NAME:
        g_value_set_static_string (value, private->display_name);
        break;

      case PROP_ID:
        g_value_set_static_string (value, private->id);
        break;

      case PROP_ARGUMENTS:
        g_value_set_boxed (value, private->arguments);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_panel_plugin_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  XfcePanelPluginPrivate *private = XFCE_PANEL_PLUGIN (object)->priv;

  switch (prop_id)
    {
      case PROP_NAME:
        private->name = g_value_dup_string (value);
        break;

      case PROP_DISPLAY_NAME:
        private->display_name = g_value_dup_string (value);
        break;

      case PROP_ID:
        private->id = g_value_dup_string (value);
        break;

      case PROP_ARGUMENTS:
        private->arguments = g_value_dup_boxed (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_panel_plugin_dispose (GObject *object)
{
  /* allow the plugin to cleanup */
  g_signal_emit (G_OBJECT (object), plugin_signals[FREE_DATA], 0);

  (*G_OBJECT_CLASS (xfce_panel_plugin_parent_class)->dispose) (object);
}



static void
xfce_panel_plugin_finalize (GObject *object)
{
  XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN (object);

  /* destroy the menu */
  if (plugin->priv->menu)
    gtk_widget_destroy (GTK_WIDGET (plugin->priv->menu));

  /* cleanup */
  g_free (plugin->priv->name);
  g_free (plugin->priv->display_name);
  g_free (plugin->priv->id);
  g_strfreev (plugin->priv->arguments);

  (*G_OBJECT_CLASS (xfce_panel_plugin_parent_class)->finalize) (object);
}



static gboolean
xfce_panel_plugin_button_press_event (GtkWidget      *widget,
                                      GdkEventButton *event)
{
  XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN (widget);
  guint            modifiers;
  GtkMenu         *menu;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (widget), FALSE);

  /* get the default accelerator modifier mask */
  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  if (plugin->priv->menu_blocked == 0
      && (event->button == 3 || (event->button == 1 && modifiers == GDK_CONTROL_MASK)))
    {
      /* get the panel menu */
      menu = xfce_panel_plugin_menu_get (plugin);

      /* set the menu screen */
      gtk_menu_set_screen (menu, gtk_widget_get_screen (widget));

      /* popup the menu */
      gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button, event->time);

      return TRUE;
    }

  return FALSE;
}



static void
xfce_panel_plugin_menu_properties (XfcePanelPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* emit configure-plugin signal */
  g_signal_emit (G_OBJECT (plugin), plugin_signals[CONFIGURE_PLUGIN], 0);
}



static void
xfce_panel_plugin_menu_about (XfcePanelPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* emit about signal */
  g_signal_emit (G_OBJECT (plugin), plugin_signals[ABOUT], 0);
}



static void
xfce_panel_plugin_menu_move (XfcePanelPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (plugin));

  /* move the plugin */
  g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", MOVE_PLUGIN);
}



static void
xfce_panel_plugin_menu_remove (XfcePanelPlugin *plugin)
{
  GtkWidget *dialog;
  gchar     *filename;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* create question dialog */
  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE, _("Do you want to remove \"%s\"?"),
                                   xfce_panel_plugin_get_display_name (plugin));

  /* setup */
  gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (plugin)));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("The item will be removed from the panel and "
                                              "its configuration will be lost."));

  /* add hig buttons */
  gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                          GTK_STOCK_REMOVE, GTK_RESPONSE_YES, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);

  /* run the dialog */
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
      /* hide the dialog */
      gtk_widget_hide (dialog);

      /* get the plugin config file */
      filename = xfce_panel_plugin_save_location (plugin, FALSE);
      if (G_LIKELY (filename))
        {
          /* remove the file */
          g_unlink (filename);

          /* cleanup */
          g_free (filename);
        }

      /* ask the panel or wrapper to remove the plugin */
      g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", REMOVE_PLUGIN);
    }

  /* destroy */
  gtk_widget_destroy (dialog);
}



static void
xfce_panel_plugin_menu_add_items (XfcePanelPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (plugin));

  /* open items dialog */
  g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", ADD_NEW_ITEMS);
}



static void
xfce_panel_plugin_menu_panel_preferences (XfcePanelPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (plugin));

  /* open preferences dialog */
  g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", PANEL_PREFERENCES);
}



static GtkMenu *
xfce_panel_plugin_menu_get (XfcePanelPlugin *plugin)
{
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *image;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  if (G_UNLIKELY (plugin->priv->menu == NULL))
    {
      /* create new menu */
      menu = gtk_menu_new ();

      /* attach to the plugin */
      gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (plugin), NULL);

      /* item with plugin name */
      item = gtk_menu_item_new_with_label (xfce_panel_plugin_get_display_name (plugin));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_set_sensitive (item, FALSE);
      gtk_widget_show (item);

      /* separator */
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* properties item */
      item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_properties), plugin);
      g_object_set_data (G_OBJECT (menu), I_("properties-item"), item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

      /* about item */
      item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_about), plugin);
      g_object_set_data (G_OBJECT (menu), I_("about-item"), item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

      /* move item */
      item = gtk_image_menu_item_new_with_mnemonic (_("_Move"));
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_move), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* separator */
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* remove */
      item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_remove), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* separator */
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      /* add new items */
      item = gtk_image_menu_item_new_with_mnemonic (_("Add _New Items..."));
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_add_items), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      image = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* customize panel */
      item = gtk_image_menu_item_new_with_mnemonic (_("Panel Pr_eferences..."));
      g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_panel_plugin_menu_panel_preferences), plugin);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
      gtk_widget_show (image);

      /* set panel menu */
      plugin->priv->menu = GTK_MENU (menu);
    }

  return plugin->priv->menu;
}



static gchar *
xfce_panel_plugin_relative_filename (XfcePanelPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);
  panel_return_val_if_fail (xfce_panel_plugin_get_name (plugin) != NULL, NULL);
  panel_return_val_if_fail (xfce_panel_plugin_get_id (plugin) != NULL, NULL);

  /* return the relative configuration filename */
  return g_strdup_printf (PANEL_PLUGIN_RELATIVE_PATH,
                          xfce_panel_plugin_get_name (plugin),
                          xfce_panel_plugin_get_id (plugin));
}



static void
xfce_panel_plugin_unregister_menu (GtkMenu         *menu,
                                   XfcePanelPlugin *plugin)
{
    panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
    panel_return_if_fail (plugin->priv->registered_menus > 0);
    panel_return_if_fail (GTK_IS_MENU (menu));

    if (G_LIKELY (plugin->priv->registered_menus > 0))
      {
        /* disconnect this signal */
        g_signal_handlers_disconnect_by_func (G_OBJECT (menu), G_CALLBACK (xfce_panel_plugin_unregister_menu), plugin);

        /* decrease the counter */
        plugin->priv->registered_menus--;

        /* emit signal to unlock the panel */
        if (G_LIKELY (plugin->priv->registered_menus == 0))
          g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", UNLOCK_PANEL);
      }
}



static void
xfce_panel_plugin_set_size (XfcePanelPluginProvider *provider,
                            gint                     size)
{
  XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN (provider);
  gboolean         handled = FALSE;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (provider));

  /* check if update is required */
  if (G_LIKELY (xfce_panel_plugin_get_size (plugin) != size))
    {
      /* store new size */
      plugin->priv->size = size;

      /* emit signal */
      g_signal_emit (G_OBJECT (plugin), plugin_signals[SIZE_CHANGED], 0, size, &handled);

      /* handle the size when not done by the plugin */
      if (handled == FALSE)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, size);
    }
}



static void
xfce_panel_plugin_set_orientation (XfcePanelPluginProvider *provider,
                                   GtkOrientation           orientation)
{
  XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN (provider);

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (provider));

  /* check if update is required */
  if (G_LIKELY (xfce_panel_plugin_get_orientation (plugin) != orientation))
    {
      /* store new size */
      plugin->priv->orientation = orientation;

      /* emit signal */
      g_signal_emit (G_OBJECT (plugin), plugin_signals[ORIENTATION_CHANGED], 0, orientation);
    }
}



static void
xfce_panel_plugin_set_screen_position (XfcePanelPluginProvider *provider,
                                       XfceScreenPosition       screen_position)
{
  XfcePanelPlugin *plugin = XFCE_PANEL_PLUGIN (provider);

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (provider));

  /* check if update is required */
  if (G_LIKELY (xfce_panel_plugin_get_screen_position (plugin) != screen_position))
    {
      /* store new screen position */
      plugin->priv->screen_position = screen_position;

      /* emit signal */
      g_signal_emit (G_OBJECT (plugin), plugin_signals[SCREEN_POSITION_CHANGED], 0, screen_position);
    }
}



static void
xfce_panel_plugin_save (XfcePanelPluginProvider *provider)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (provider));

  /* emit signal */
  g_signal_emit (G_OBJECT (provider), plugin_signals[SAVE], 0);
}



static void
xfce_panel_plugin_take_window_notify (gpointer  data,
                                      GObject  *where_the_object_was)
{
  panel_return_if_fail (GTK_IS_WINDOW (data) || XFCE_IS_PANEL_PLUGIN (data));

  /* release the opposite weak ref */
  g_object_weak_unref (G_OBJECT (data), xfce_panel_plugin_take_window_notify, where_the_object_was);

  /* destroy the dialog if the plugin was finalized */
  if (GTK_IS_WINDOW (data))
    gtk_widget_destroy (GTK_WIDGET (data));
}



/**
 * xfce_panel_plugin_get_name:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the name of the panel plugin.
 **/
G_CONST_RETURN gchar *
xfce_panel_plugin_get_name (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  return plugin->priv->name;
}



/**
 * xfce_panel_plugin_get_display_name:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the (translated) display name of the plugin.
 **/
G_CONST_RETURN gchar *
xfce_panel_plugin_get_display_name (XfcePanelPlugin *plugin)
{


  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  if (G_LIKELY (plugin->priv->display_name))
    return plugin->priv->display_name;

  return plugin->priv->name;
}



/**
 * xfce_panel_plugin_get_id:
 * @plugin :
 *
 * Return value: the unique id of the panel plugin.
 **/
G_CONST_RETURN gchar *
xfce_panel_plugin_get_id (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  return plugin->priv->id;
}



/**
 * xfce_panel_plugin_get_arguments:
 * @plugin    : an #XfcePanelPlugin.
 * @arguments : return location for the argument vector.
 *
 * Return value: %TRUE @arguments was set, %FALSE otherwise.
 *
 * Since: 4.8.0
 **/
gboolean
xfce_panel_plugin_get_arguments (XfcePanelPlugin   *plugin,
                                 gchar           ***arguments)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), FALSE);

  /* dupplicate the arguments */
  if (G_LIKELY (arguments))
    *arguments = g_strdupv (plugin->priv->arguments);

  return !!(plugin->priv->arguments != NULL);
}



/**
 * xfce_panel_plugin_get_size:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the current size of the panel.
 **/
gint
xfce_panel_plugin_get_size (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), -1);

  return plugin->priv->size;
}



/**
 * xfce_panel_plugin_get_expand:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: %TRUE when the plugin should expand,
 *               %FALSE otherwise.
 **/
gboolean
xfce_panel_plugin_get_expand (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), FALSE);

  return plugin->priv->expand;
}



/**
 * xfce_panel_plugin_set_expand:
 * @plugin : an #XfcePanelPlugin.
 **/
void
xfce_panel_plugin_set_expand (XfcePanelPlugin *plugin,
                              gboolean         expand)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* normalize the value */
  expand = !!expand;

  /* check if update is required */
  if (G_LIKELY (xfce_panel_plugin_get_expand (plugin) != expand))
    {
      /* store internal value */
      plugin->priv->expand = expand;

      /* emit signal (in provider) */
      g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", expand ? EXPAND_PLUGIN : COLLAPSE_PLUGIN);
    }
}



/**
 * xfce_panel_plugin_get_orientation:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the current #GtkOrientation of the panel.
 **/
GtkOrientation
xfce_panel_plugin_get_orientation (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), GTK_ORIENTATION_HORIZONTAL);

  return plugin->priv->orientation;
}



/**
 * xfce_panel_plugin_get_screen_position:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the current #XfceScreenPosition of the panel.
 **/
XfceScreenPosition
xfce_panel_plugin_get_screen_position (XfcePanelPlugin *plugin)
{
  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), XFCE_SCREEN_POSITION_NONE);

  return plugin->priv->screen_position;
}



/**
 * xfce_panel_plugin_take_window:
 * @plugin : an #XfcePanelPlugin.
 * @window : a #GtkWindow.
 *
 * Since: 4.8.0
 **/
void
xfce_panel_plugin_take_window (XfcePanelPlugin *plugin,
                               GtkWindow       *window)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_WINDOW (window));

  /* monitor both objects */
  g_object_weak_ref (G_OBJECT (plugin), xfce_panel_plugin_take_window_notify, window);
  g_object_weak_ref (G_OBJECT (window), xfce_panel_plugin_take_window_notify, plugin);
}



/**
 * xfce_panel_plugin_add_action_widget:
 * @plugin : an #XfcePanelPlugin.
 * @widget : a #GtkWidget.
 **/
void
xfce_panel_plugin_add_action_widget (XfcePanelPlugin *plugin,
                                     GtkWidget       *widget)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* connect button-press-event signal */
  g_signal_connect_swapped (G_OBJECT (widget), "button-press-event", G_CALLBACK (xfce_panel_plugin_button_press_event), plugin);
}



/**
 * xfce_panel_plugin_menu_insert_item:
 * @plugin : an #XfcePanelPlugin.
 * @item   : a #GtkMenuItem.
 **/
void
xfce_panel_plugin_menu_insert_item (XfcePanelPlugin *plugin,
                                    GtkMenuItem     *item)
{
  GtkMenu *menu;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_MENU_ITEM (item));

  /* get the panel menu */
  menu = xfce_panel_plugin_menu_get (plugin);

  /* insert the new item below the move entry */
  gtk_menu_shell_insert (GTK_MENU_SHELL (menu), GTK_WIDGET (item), 5);
}



/**
 * xfce_panel_plugin_menu_show_configure:
 * @plugin : an #XfcePanelPlugin.
 **/
void
xfce_panel_plugin_menu_show_configure (XfcePanelPlugin *plugin)
{
  GtkWidget *item;
  GtkMenu   *menu;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* get the panel menu */
  menu = xfce_panel_plugin_menu_get (plugin);

  /* get and show the properties item */
  item = g_object_get_data (G_OBJECT (menu), I_("properties-item"));
  if (G_LIKELY (item))
    gtk_widget_show (item);
}



/**
 * xfce_panel_plugin_menu_show_about:
 * @plugin : an #XfcePanelPlugin.
 **/
void
xfce_panel_plugin_menu_show_about (XfcePanelPlugin *plugin)
{
  GtkWidget *item;
  GtkMenu   *menu;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* get the panel menu */
  menu = xfce_panel_plugin_menu_get (plugin);

  /* get and show the properties item */
  item = g_object_get_data (G_OBJECT (menu), I_("about-item"));
  if (G_LIKELY (item))
    gtk_widget_show (item);
}



/**
 * xfce_panel_plugin_block_menu:
 * plugin : an #XfcePanelPlugin.
 **/
void
xfce_panel_plugin_block_menu (XfcePanelPlugin *plugin)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

  /* increase block counter */
  plugin->priv->menu_blocked++;
}



/**
 * xfce_panel_plugin_unblock_menu:
 * plugin : an #XfcePanelPlugin.
 **/
void
xfce_panel_plugin_unblock_menu (XfcePanelPlugin *plugin)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (plugin->priv->menu_blocked > 0);

  /* decrease block counter */
  if (G_LIKELY (plugin->priv->menu_blocked > 0))
    plugin->priv->menu_blocked--;
}



/**
 * xfce_panel_plugin_register_menu:
 * @plugin : an #XfcePanelPlugin.
 * @menu   : a #GtkMenu.
 **/
void
xfce_panel_plugin_register_menu (XfcePanelPlugin *plugin,
                                 GtkMenu         *menu)
{
  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_MENU (menu));

  /* increase the counter */
  plugin->priv->registered_menus++;

  /* connect signal to menu to decrease counter */
  g_signal_connect (G_OBJECT (menu), "deactivate", G_CALLBACK (xfce_panel_plugin_unregister_menu), plugin);

  /* tell panel it needs to lock */
  if (G_LIKELY (plugin->priv->registered_menus == 1))
    g_signal_emit_by_name (G_OBJECT (plugin), "provider-signal", LOCK_PANEL);
}



/**
 * xfce_panel_plugin_arrow_type:
 * @plugin : an #XfcePanelPlugin.
 *
 * Return value: the #GtkArrowType to use.
 *
 * Since: 4.6.0
 **/
GtkArrowType
xfce_panel_plugin_arrow_type (XfcePanelPlugin *plugin)
{
  XfceScreenPosition  screen_position;
  GdkScreen          *screen;
  gint                monitor_num;
  GdkRectangle        monitor;
  gint                x, y;

  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), GTK_ARROW_UP);

  /* get the plugin screen position */
  screen_position = xfce_panel_plugin_get_screen_position (plugin);

  /* detect the arrow type */
  if (xfce_screen_position_is_top (screen_position))
    return GTK_ARROW_DOWN;
  else if (xfce_screen_position_is_bottom (screen_position))
    return GTK_ARROW_UP;
  else if (xfce_screen_position_is_left (screen_position))
    return GTK_ARROW_RIGHT;
  else if (xfce_screen_position_is_right (screen_position))
    return GTK_ARROW_LEFT;
  else /* floating */
    {
      /* get the monitor geometry */
      screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
      monitor_num = gdk_screen_get_monitor_at_window (screen, GTK_WIDGET (plugin)->window);
      gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

      /* get the plugin root origin */
      gdk_window_get_root_origin (GTK_WIDGET (plugin)->window, &x, &y);

      /* detect arrow type */
      if (screen_position == XFCE_SCREEN_POSITION_FLOATING_H)
        return (y < (monitor.y + monitor.height / 2)) ? GTK_ARROW_DOWN : GTK_ARROW_UP;
      else
        return (x < (monitor.x + monitor.width / 2)) ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT;
    }
}



/**
 * xfce_panel_plugin_position_widget:
 * @plugin        : an #XfcePanelPlugin.
 * @menu_widget   : a #GtkWidget that will be used as popup menu.
 * @attach_widget : a #GtkWidget relative to which the menu should be positioned.
 * @x             : return location for the x coordinate.
 * @y             : return location for the x coordinate.
 *
 * Since: 4.6.0
 **/
void
xfce_panel_plugin_position_widget (XfcePanelPlugin *plugin,
                                   GtkWidget       *menu_widget,
                                   GtkWidget       *attach_widget,
                                   gint            *x,
                                   gint            *y)
{
  GtkRequisition  requisition;
  GdkScreen      *screen;
  GdkRectangle    monitor;
  gint            monitor_num;
  GtkWidget      *toplevel;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));
  g_return_if_fail (GTK_IS_WIDGET (menu_widget));
  g_return_if_fail (attach_widget == NULL || GTK_IS_WIDGET (attach_widget));

  /* if the attach widget is null, use the panel plugin */
  if (attach_widget == NULL)
    attach_widget = GTK_WIDGET (plugin);

  /* make sure the menu is realized to get valid rectangle sizes */
  if (!GTK_WIDGET_REALIZED (menu_widget))
    gtk_widget_realize (menu_widget);

  /* make sure the attach widget is realized for the gdkwindow */
  if (!GTK_WIDGET_REALIZED (attach_widget))
    gtk_widget_realize (attach_widget);

  /* get the menu/widget size request */
  gtk_widget_size_request (menu_widget, &requisition);

  /* get the root position of the attach widget (the panel) */
  toplevel = gtk_widget_get_toplevel (attach_widget);
  gtk_window_get_position (GTK_WINDOW (toplevel), x, y);

  /* add the widgets allocation */
  *x += attach_widget->allocation.x;
  *y += attach_widget->allocation.y;

  switch (xfce_panel_plugin_arrow_type (plugin))
    {
      case GTK_ARROW_UP:
        *y -= requisition.height;
        break;

      case GTK_ARROW_DOWN:
        *y += attach_widget->allocation.height;
        break;

      case GTK_ARROW_LEFT:
        *x -= requisition.width;
        break;

      default: /* GTK_ARROW_RIGHT and GTK_ARROW_NONE */
        *x += attach_widget->allocation.width;
        break;
    }

  /* get the monitor geometry */
  screen = gtk_widget_get_screen (attach_widget);
  monitor_num = gdk_screen_get_monitor_at_window (screen, attach_widget->window);
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  /* keep the menu inside the screen */
  if (*x > monitor.x + monitor.width - requisition.width)
    *x = monitor.x + monitor.width - requisition.width;
  if (*x < monitor.x)
    *x = monitor.x;
  if (*y > monitor.y + monitor.height - requisition.height)
    *y = monitor.y + monitor.height - requisition.height;
  if (*y < monitor.y)
    *y = monitor.y;

  /* popup on the correct screen */
  if (G_LIKELY (GTK_IS_MENU (menu_widget)))
    gtk_menu_set_screen (GTK_MENU (menu_widget), screen);
  else if (GTK_IS_WINDOW (menu_widget))
    gtk_window_set_screen (GTK_WINDOW (menu_widget), screen);
}



/**
 * xfce_panel_plugin_position_menu:
 * @menu         : a #GtkMenu.
 * @x            : return location for the x coordinate.
 * @x            : return location for the y coordinate.
 * @push_in      : keep inside the screen (see #GtkMenuPositionFunc)
 * @panel_plugin : an #XfcePanelPlugin.
 *
 * Since: 4.6.0
 **/
void
xfce_panel_plugin_position_menu (GtkMenu  *menu,
                                 gint     *x,
                                 gint     *y,
                                 gboolean *push_in,
                                 gpointer  panel_plugin)
{
  GtkWidget *attach_widget;

  g_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));
  g_return_if_fail (GTK_IS_MENU (menu));

  /* get the attach widget */
  attach_widget = gtk_menu_get_attach_widget (menu);

  /* calculate the coordinates */
  xfce_panel_plugin_position_widget (XFCE_PANEL_PLUGIN (panel_plugin), GTK_WIDGET (menu),
                                     attach_widget, x, y);

  /* keep the menu inside screen */
  *push_in = TRUE;

  /* register the menu */
  xfce_panel_plugin_register_menu (XFCE_PANEL_PLUGIN (panel_plugin), menu);
}



/**
 * xfce_panel_plugin_lookup_rc_file:
 * @plugin : an #XfcePanelPlugin.
 *
 * Looks for the plugin resource file. This should be used to get the
 * plugin read location of the config file. You should only use the
 * returned path to read information from, since it might point to a
 * not-writable file (in kiosk mode for example).
 *
 * See also: xfce_panel_plugin_save_location() and xfce_resource_lookup()
 *
 * Returns: The path to a config file or %NULL if no file was found.
 *          The returned string must be freed using g_free()
 **/
gchar *
xfce_panel_plugin_lookup_rc_file (XfcePanelPlugin *plugin)
{
  gchar *filename, *path;

  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  /* get the relative filename */
  filename = xfce_panel_plugin_relative_filename (plugin);

  /* get the absolute path */
  path = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, filename);

  /* cleanup */
  g_free (filename);

  return path;
}



/**
 * xfce_panel_plugin_save_location:
 * @plugin : an #XfcePanelPlugin.
 * @create : whether to create missing directories.
 *
 * Returns the path that can be used to store configuration information.
 * Don't use this function if you want to read from the config file, but
 * use xfce_panel_plugin_rc_location() instead.
 *
 * See also: xfce_panel_plugin_rc_location() and xfce_resource_save_location()
 *
 * Returns: The path to a config file or %NULL if no file was found.
 *          The returned string must be freed u sing g_free().
 **/
gchar *
xfce_panel_plugin_save_location (XfcePanelPlugin *plugin,
                                 gboolean         create)
{
  gchar *filename, *path;

  g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

  /* get the relative filename */
  filename = xfce_panel_plugin_relative_filename (plugin);

  /* get the absolute path */
  path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, filename, create);

  /* cleanup */
  g_free (filename);

  return path;
}
