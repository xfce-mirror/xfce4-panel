/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#include <exo/exo.h>
#include <garcon/garcon.h>
#include <garcon-gtk/garcon-gtk.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <common/panel-private.h>
#include <common/panel-debug.h>

#include "applicationsmenu.h"
#include "applicationsmenu-dialog_ui.h"


/* I18N: default tooltip of the application menu */
#define DEFAULT_TITLE     _("Applications")
#define DEFAULT_ICON_NAME "org.xfce.panel.applicationsmenu"
#define DEFAULT_ICON_SIZE (16)
#define DEFAULT_EDITOR    "menulibre"


struct _ApplicationsMenuPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _ApplicationsMenuPlugin
{
  XfcePanelPlugin __parent__;

  GtkWidget       *button;
  GtkWidget       *box;
  GtkWidget       *icon;
  GtkWidget       *label;
  GtkWidget       *menu;

  guint            is_constructed : 1;

  guint            show_button_title : 1;
  gchar           *button_title;
  gchar           *button_icon;
  gboolean         custom_menu;
  gchar           *custom_menu_file;
  gchar           *menu_editor;

  /* temp item we store here when the
   * properties dialog is opened */
  GtkWidget       *dialog_icon;

  gulong           style_set_id;
  gulong           screen_changed_id;
  gulong           theme_changed_id;
};

enum
{
  PROP_0,
  PROP_SHOW_GENERIC_NAMES,
  PROP_SHOW_MENU_ICONS,
  PROP_SHOW_TOOLTIPS,
  PROP_SHOW_BUTTON_TITLE,
  PROP_BUTTON_TITLE,
  PROP_BUTTON_ICON,
  PROP_CUSTOM_MENU,
  PROP_CUSTOM_MENU_FILE,
  PROP_MENU_EDITOR
};



static void      applications_menu_plugin_get_property         (GObject                *object,
                                                                guint                   prop_id,
                                                                GValue                 *value,
                                                                GParamSpec             *pspec);
static void      applications_menu_plugin_set_property         (GObject                *object,
                                                                guint                   prop_id,
                                                                const GValue           *value,
                                                                GParamSpec             *pspec);
static void      applications_menu_plugin_construct            (XfcePanelPlugin        *panel_plugin);
static void      applications_menu_plugin_free_data            (XfcePanelPlugin        *panel_plugin);
static gboolean  applications_menu_plugin_size_changed         (XfcePanelPlugin        *panel_plugin,
                                                                gint                    size);
static void      applications_menu_plugin_mode_changed         (XfcePanelPlugin        *panel_plugin,
                                                                XfcePanelPluginMode     mode);
static void      applications_menu_plugin_configure_plugin     (XfcePanelPlugin        *panel_plugin);
static gboolean  applications_menu_plugin_remote_event         (XfcePanelPlugin        *panel_plugin,
                                                                const gchar            *name,
                                                                const GValue           *value);
static gboolean  applications_menu_plugin_menu                 (GtkWidget              *button,
                                                                GdkEventButton         *event,
                                                                ApplicationsMenuPlugin *plugin);
static void      applications_menu_plugin_menu_deactivate      (GtkWidget              *menu,
                                                                ApplicationsMenuPlugin *plugin);
static void      applications_menu_plugin_set_garcon_menu      (ApplicationsMenuPlugin *plugin);
static void      applications_menu_button_theme_changed        (ApplicationsMenuPlugin *plugin);



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ApplicationsMenuPlugin, applications_menu_plugin)



static void
applications_menu_plugin_class_init (ApplicationsMenuPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = applications_menu_plugin_get_property;
  gobject_class->set_property = applications_menu_plugin_set_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = applications_menu_plugin_construct;
  plugin_class->free_data = applications_menu_plugin_free_data;
  plugin_class->size_changed = applications_menu_plugin_size_changed;
  plugin_class->mode_changed = applications_menu_plugin_mode_changed;
  plugin_class->configure_plugin = applications_menu_plugin_configure_plugin;
  plugin_class->remote_event = applications_menu_plugin_remote_event;

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_GENERIC_NAMES,
                                   g_param_spec_boolean ("show-generic-names",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_MENU_ICONS,
                                   g_param_spec_boolean ("show-menu-icons",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_TOOLTIPS,
                                   g_param_spec_boolean ("show-tooltips",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_BUTTON_TITLE,
                                   g_param_spec_boolean ("show-button-title",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BUTTON_TITLE,
                                   g_param_spec_string ("button-title",
                                                        NULL, NULL,
                                                        DEFAULT_TITLE,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BUTTON_ICON,
                                   g_param_spec_string ("button-icon",
                                                        NULL, NULL,
                                                        DEFAULT_ICON_NAME,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_MENU,
                                   g_param_spec_boolean ("custom-menu",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_MENU_FILE,
                                   g_param_spec_string ("custom-menu-file",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_MENU_EDITOR,
                                   g_param_spec_string ("menu-editor",
                                                        NULL, NULL,
                                                        DEFAULT_EDITOR,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static void
applications_menu_plugin_init (ApplicationsMenuPlugin *plugin)
{
  GtkIconTheme *icon_theme;

  /* init garcon environment */
  garcon_set_environment_xdg (GARCON_ENVIRONMENT_XFCE);

  icon_theme = gtk_icon_theme_get_default ();

  plugin->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->button);
  gtk_widget_set_name (plugin->button, "applicationmenu-button");
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (plugin->button, DEFAULT_TITLE);
  g_signal_connect (G_OBJECT (plugin->button), "button-press-event",
      G_CALLBACK (applications_menu_plugin_menu), plugin);

  plugin->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (plugin->box), 0);
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->box);
  gtk_widget_show (plugin->box);

  plugin->icon = gtk_image_new_from_icon_name (DEFAULT_ICON_NAME, DEFAULT_ICON_SIZE);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->icon, FALSE, FALSE, 0);
  gtk_widget_show (plugin->icon);

  plugin->label = gtk_label_new (DEFAULT_TITLE);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->label, FALSE, FALSE, 0);
  plugin->show_button_title = TRUE;
  gtk_widget_show (plugin->label);

  /* prepare the menu */
  plugin->menu = garcon_gtk_menu_new (NULL);
  g_signal_connect (G_OBJECT (plugin->menu), "selection-done",
      G_CALLBACK (applications_menu_plugin_menu_deactivate), plugin);

  plugin->style_set_id = g_signal_connect_swapped (G_OBJECT (plugin->button), "style-set",
                                                   G_CALLBACK (applications_menu_button_theme_changed), plugin);
  plugin->screen_changed_id = g_signal_connect_swapped (G_OBJECT (plugin->button), "screen-changed",
                                                        G_CALLBACK (applications_menu_button_theme_changed), plugin);
  plugin->theme_changed_id = g_signal_connect_swapped (G_OBJECT (icon_theme), "changed",
                                                       G_CALLBACK (applications_menu_plugin_set_garcon_menu), plugin);
}



static void
applications_menu_plugin_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (object);

  switch (prop_id)
    {
    case PROP_SHOW_GENERIC_NAMES:
      g_value_set_boolean (value,
          garcon_gtk_menu_get_show_generic_names (GARCON_GTK_MENU (plugin->menu)));
      break;

    case PROP_SHOW_MENU_ICONS:
      g_value_set_boolean (value,
          garcon_gtk_menu_get_show_menu_icons (GARCON_GTK_MENU (plugin->menu)));
      break;

    case PROP_SHOW_TOOLTIPS:
      g_value_set_boolean (value,
          garcon_gtk_menu_get_show_tooltips (GARCON_GTK_MENU (plugin->menu)));
      break;

    case PROP_SHOW_BUTTON_TITLE:
      g_value_set_boolean (value, plugin->show_button_title);
      break;

    case PROP_BUTTON_TITLE:
      g_value_set_string (value, plugin->button_title == NULL ?
          DEFAULT_TITLE : plugin->button_title);
      break;

    case PROP_BUTTON_ICON:
      g_value_set_string (value, panel_str_is_empty (plugin->button_icon) ?
          DEFAULT_ICON_NAME : plugin->button_icon);
      break;

    case PROP_CUSTOM_MENU:
      g_value_set_boolean (value, plugin->custom_menu);
      break;

    case PROP_CUSTOM_MENU_FILE:
      g_value_set_string (value, plugin->custom_menu_file);
      break;

    case PROP_MENU_EDITOR:
      g_value_set_string (value, plugin->menu_editor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
applications_menu_plugin_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (object);
  gboolean                force_a_resize = FALSE;

  switch (prop_id)
    {
    case PROP_SHOW_GENERIC_NAMES:
      garcon_gtk_menu_set_show_generic_names (GARCON_GTK_MENU (plugin->menu),
                                              g_value_get_boolean (value));
      break;

    case PROP_SHOW_MENU_ICONS:
      garcon_gtk_menu_set_show_menu_icons (GARCON_GTK_MENU (plugin->menu),
                                           g_value_get_boolean (value));
       break;

    case PROP_SHOW_TOOLTIPS:
      garcon_gtk_menu_set_show_tooltips (GARCON_GTK_MENU (plugin->menu),
                                         g_value_get_boolean (value));
      break;

    case PROP_SHOW_BUTTON_TITLE:
      plugin->show_button_title = g_value_get_boolean (value);
      if (plugin->show_button_title)
        gtk_widget_show (plugin->label);
      else
        gtk_widget_hide (plugin->label);
      applications_menu_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
          xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
      return;

    case PROP_BUTTON_TITLE:
      g_free (plugin->button_title);
      plugin->button_title = g_value_dup_string (value);
      gtk_label_set_text (GTK_LABEL (plugin->label),
          plugin->button_title != NULL ? plugin->button_title : "");
      gtk_widget_set_tooltip_text (plugin->button,
          panel_str_is_empty (plugin->button_title) ? NULL : plugin->button_title);

      /* check if the label still fits */
      if (xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (plugin)) == XFCE_PANEL_PLUGIN_MODE_DESKBAR
          && plugin->show_button_title)
        {
          force_a_resize = TRUE;
        }
      break;

    case PROP_BUTTON_ICON:
      g_free (plugin->button_icon);
      plugin->button_icon = g_value_dup_string (value);

      force_a_resize = TRUE;
      break;

    case PROP_CUSTOM_MENU:
      plugin->custom_menu = g_value_get_boolean (value);

      if (plugin->is_constructed)
        applications_menu_plugin_set_garcon_menu (plugin);
      break;

    case PROP_CUSTOM_MENU_FILE:
      g_free (plugin->custom_menu_file);
      plugin->custom_menu_file = g_value_dup_string (value);

      if (plugin->is_constructed)
        applications_menu_plugin_set_garcon_menu (plugin);
      break;

    case PROP_MENU_EDITOR:
      plugin->menu_editor = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  if (force_a_resize)
    {
      applications_menu_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
          xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
    }
}



static void
applications_menu_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "show-generic-names", G_TYPE_BOOLEAN },
    { "show-menu-icons", G_TYPE_BOOLEAN },
    { "show-button-title", G_TYPE_BOOLEAN },
    { "show-tooltips", G_TYPE_BOOLEAN },
    { "button-title", G_TYPE_STRING },
    { "button-icon", G_TYPE_STRING },
    { "custom-menu", G_TYPE_BOOLEAN },
    { "custom-menu-file", G_TYPE_STRING },
    { "menu-editor", G_TYPE_STRING },
    { NULL }
  };

  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* bind all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* make sure the menu is set */
  applications_menu_plugin_set_garcon_menu (plugin);

  if (!plugin->menu_editor)
      plugin->menu_editor = DEFAULT_EDITOR;

  gtk_widget_show (plugin->button);

  applications_menu_plugin_size_changed (panel_plugin,
      xfce_panel_plugin_get_size (panel_plugin));
  plugin->is_constructed = TRUE;
}



static void
applications_menu_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  GtkIconTheme           *icon_theme;

  if (plugin->menu != NULL)
    gtk_widget_destroy (plugin->menu);

  if (plugin->style_set_id != 0)
    {
      g_signal_handler_disconnect (plugin->button, plugin->style_set_id);
      plugin->style_set_id = 0;
    }

  if (plugin->screen_changed_id != 0)
    {
      g_signal_handler_disconnect (plugin->button, plugin->screen_changed_id);
      plugin->screen_changed_id = 0;
    }

  if (plugin->theme_changed_id != 0)
    {
      icon_theme = gtk_icon_theme_get_default ();
      g_signal_handler_disconnect (G_OBJECT (icon_theme),
                                   plugin->theme_changed_id);
      plugin->theme_changed_id = 0;
    }

  g_free (plugin->button_title);
  g_free (plugin->button_icon);
  g_free (plugin->custom_menu_file);
}



static gboolean
applications_menu_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                       gint             size)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  XfcePanelPluginMode     mode;
  GtkRequisition          label_size;
  GtkOrientation          orientation;
  gint                    border_thickness;
  GdkPixbuf              *icon;
  gint                    icon_size;
  GdkScreen              *screen;
  GtkIconTheme           *icon_theme = NULL;
  gchar                  *icon_name;
  GtkStyleContext        *ctx;
  GtkBorder               padding, border;

  gtk_box_set_child_packing (GTK_BOX (plugin->box), plugin->icon,
                             !plugin->show_button_title,
                             !plugin->show_button_title,
                             0, GTK_PACK_START);

  mode = xfce_panel_plugin_get_mode (panel_plugin);

  if (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
    orientation = GTK_ORIENTATION_HORIZONTAL;
  else
    orientation = GTK_ORIENTATION_VERTICAL;

  /* style thickness */
  ctx = gtk_widget_get_style_context (plugin->button);
  gtk_style_context_get_padding (ctx, gtk_widget_get_state_flags (plugin->button), &padding);
  gtk_style_context_get_border (ctx, gtk_widget_get_state_flags (plugin->button), &border);
  border_thickness = MAX (padding.left + padding.right + border.left + border.right,
                          padding.top + padding.bottom + border.top + border.bottom);

  icon_size = xfce_panel_plugin_get_icon_size (panel_plugin);

  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  if (G_LIKELY (screen != NULL))
    icon_theme = gtk_icon_theme_get_for_screen (screen);

  icon_name = panel_str_is_empty (plugin->button_icon) ?
    DEFAULT_ICON_NAME : plugin->button_icon;

  icon = xfce_panel_pixbuf_from_source_at_size (icon_name,
                                                icon_theme,
                                                icon_size,
                                                icon_size);

  if (G_LIKELY (icon != NULL))
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (plugin->icon), icon);
      g_object_unref (G_OBJECT (icon));
    }

  if (plugin->show_button_title &&
      mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
    {
      /* check if the label (minimum size) fits next to the icon */
      gtk_widget_get_preferred_size (GTK_WIDGET (plugin->label), &label_size, NULL);
      if (label_size.width <= size - icon_size - 2 - border_thickness)
        orientation = GTK_ORIENTATION_HORIZONTAL;
    }

  gtk_orientable_set_orientation (GTK_ORIENTABLE (plugin->box), orientation);

  return TRUE;
}



static void
applications_menu_plugin_mode_changed (XfcePanelPlugin     *panel_plugin,
                                       XfcePanelPluginMode  mode)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  gint                    angle;

  angle = (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? 270 : 0;
  gtk_label_set_angle (GTK_LABEL (plugin->label), angle);

  applications_menu_plugin_size_changed (panel_plugin,
      xfce_panel_plugin_get_size (panel_plugin));
}



static void
applications_menu_plugin_configure_plugin_file_set (GtkFileChooserButton   *button,
                                                    ApplicationsMenuPlugin *plugin)
{
  gchar *filename;

  panel_return_if_fail (GTK_IS_FILE_CHOOSER_BUTTON (button));
  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (button));
  g_object_set (G_OBJECT (plugin), "custom-menu-file", filename, NULL);
  g_free (filename);
}



static void
applications_menu_plugin_configure_plugin_icon_chooser (GtkWidget              *button,
                                                        ApplicationsMenuPlugin *plugin)
{
#ifdef EXO_CHECK_VERSION
  GtkWidget *chooser;
  gchar     *icon;

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (GTK_IMAGE (plugin->dialog_icon));

  chooser = exo_icon_chooser_dialog_new (_("Select An Icon"),
                                         GTK_WINDOW (gtk_widget_get_toplevel (button)),
                                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                                         _("_OK"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);

  exo_icon_chooser_dialog_set_icon (EXO_ICON_CHOOSER_DIALOG (chooser),
      panel_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon);

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      icon = exo_icon_chooser_dialog_get_icon (EXO_ICON_CHOOSER_DIALOG (chooser));
      g_object_set (G_OBJECT (plugin), "button-icon", icon, NULL);
      gtk_image_set_from_icon_name (GTK_IMAGE (plugin->dialog_icon),
                                    exo_str_is_empty (plugin->button_icon) ?
                                    DEFAULT_ICON_NAME : plugin->button_icon,
                                    GTK_ICON_SIZE_DIALOG);
      g_free (icon);
    }

  gtk_widget_destroy (chooser);
#endif
}



static void
applications_menu_plugin_configure_plugin_edit (GtkWidget              *button,
                                                ApplicationsMenuPlugin *plugin)
{
  GError      *error = NULL;

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_WIDGET (button));

  if (!xfce_spawn_command_line (gtk_widget_get_screen (button), plugin->menu_editor,
                                FALSE, FALSE, TRUE, &error))
    {
      xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\"."), plugin->menu_editor);
      g_error_free (error);
    }
}



static void
applications_menu_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  GtkBuilder             *builder;
  GObject                *dialog, *object, *object2;
  guint                   i;
  gchar                  *path;
  const gchar            *check_names[] = { "show-generic-names", "show-menu-icons",
                                            "show-tooltips", "show-button-title" };

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, applicationsmenu_dialog_ui,
                                     applicationsmenu_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  for (i = 0; i < G_N_ELEMENTS (check_names); ++i)
    {
      object = gtk_builder_get_object (builder, check_names[i]);
      panel_return_if_fail (GTK_IS_CHECK_BUTTON (object));
      g_object_bind_property (G_OBJECT (plugin), check_names[i],
                              G_OBJECT (object), "active",
                              G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
    }

  object = gtk_builder_get_object (builder, "button-title");
  panel_return_if_fail (GTK_IS_ENTRY (object));
  g_object_bind_property (G_OBJECT (plugin), "button-title",
                          G_OBJECT (object), "text",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  object = gtk_builder_get_object (builder, "icon-button");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  g_signal_connect (G_OBJECT (object), "clicked",
     G_CALLBACK (applications_menu_plugin_configure_plugin_icon_chooser), plugin);

  plugin->dialog_icon = gtk_image_new_from_icon_name (
      panel_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon,
      GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (object), plugin->dialog_icon);
  g_object_add_weak_pointer (G_OBJECT (plugin->dialog_icon), (gpointer) &plugin->dialog_icon);
  gtk_widget_show (plugin->dialog_icon);

  /* whether we show the edit menu button */
  object = gtk_builder_get_object (builder, "edit-menu-button");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  path = g_find_program_in_path (plugin->menu_editor);
  if (path != NULL)
    {
      object2 = gtk_builder_get_object (builder, "use-default-menu");
      panel_return_if_fail (GTK_IS_RADIO_BUTTON (object2));
      g_object_bind_property (G_OBJECT (object2), "active",
                              G_OBJECT (object), "sensitive",
                              G_BINDING_SYNC_CREATE);
      g_signal_connect (G_OBJECT (object), "clicked",
          G_CALLBACK (applications_menu_plugin_configure_plugin_edit), plugin);
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET (object));
    }
  g_free (path);

  object = gtk_builder_get_object (builder, "use-custom-menu");
  panel_return_if_fail (GTK_IS_RADIO_BUTTON (object));
  g_object_bind_property (G_OBJECT (plugin), "custom-menu",
                          G_OBJECT (object), "active",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);

  /* sensitivity of custom file selector */
  object2 = gtk_builder_get_object (builder, "custom-box");
  panel_return_if_fail (GTK_IS_WIDGET (object2));
  g_object_bind_property (G_OBJECT (object), "active",
                          G_OBJECT (object2), "sensitive",
                          G_BINDING_SYNC_CREATE);

  object = gtk_builder_get_object (builder, "custom-file");
  panel_return_if_fail (GTK_IS_FILE_CHOOSER_BUTTON (object));
  if (!panel_str_is_empty (plugin->custom_menu_file))
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (object), plugin->custom_menu_file);
  g_signal_connect (G_OBJECT (object), "file-set",
     G_CALLBACK (applications_menu_plugin_configure_plugin_file_set), plugin);

  gtk_widget_show (GTK_WIDGET (dialog));
}



static gboolean
applications_menu_plugin_remote_event (XfcePanelPlugin *panel_plugin,
                                       const gchar     *name,
                                       const GValue    *value)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);

  panel_return_val_if_fail (value == NULL || G_IS_VALUE (value), FALSE);

  if (strcmp (name, "popup") == 0
      && gtk_widget_get_visible (GTK_WIDGET (panel_plugin))
      && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->button))
      && panel_utils_grab_available ())
    {
      if (value != NULL
          && G_VALUE_HOLDS_BOOLEAN (value)
          && g_value_get_boolean (value))
        {
          /* show menu under cursor */
          applications_menu_plugin_menu (NULL, NULL, plugin);
        }
      else
        {
          /* show the menu at the button */
          applications_menu_plugin_menu (plugin->button, NULL, plugin);
        }

      /* don't popup another menu */
      return TRUE;
    }

  return FALSE;
}



static void
applications_menu_plugin_menu_deactivate (GtkWidget *menu,
                                          ApplicationsMenuPlugin *plugin)
{
  panel_return_if_fail (plugin->button == NULL || GTK_IS_TOGGLE_BUTTON (plugin->button));
  panel_return_if_fail (GTK_IS_MENU (menu));

  xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (plugin), FALSE);

  /* button is NULL when we popup the menu under the cursor position */
  if (plugin->button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), FALSE);

  gtk_menu_popdown (GTK_MENU (menu));
}



static void
applications_menu_plugin_set_garcon_menu (ApplicationsMenuPlugin *plugin)
{
  GarconMenu *menu = NULL;
  gchar      *filename;
  GFile      *file;

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (GARCON_GTK_IS_MENU (plugin->menu));

  /* load the custom menu if set */
  if (plugin->custom_menu
      && plugin->custom_menu_file != NULL)
    menu = garcon_menu_new_for_path (plugin->custom_menu_file);

  /* use the applications menu, this also respects the
   * XDG_MENU_PREFIX environment variable */
  if (G_LIKELY (menu == NULL))
    menu = garcon_menu_new_applications ();

  /* set the menu */
  garcon_gtk_menu_set_menu (GARCON_GTK_MENU (plugin->menu), menu);

  /* debugging information */
  if (0)
    {
  file = garcon_menu_get_file (menu);
  filename = g_file_get_parse_name (file);
  g_object_unref (G_OBJECT (file));

  panel_debug (PANEL_DEBUG_APPLICATIONSMENU,
               "menu from \"%s\"", filename);
  g_free (filename);
    }

  g_object_unref (G_OBJECT (menu));
}



static gboolean
applications_menu_plugin_menu (GtkWidget              *button,
                               GdkEventButton         *event,
                               ApplicationsMenuPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (button == NULL || plugin->button == button, FALSE);

  if (event != NULL /* remove event */
      && !(event->button == 1
           && event->type == GDK_BUTTON_PRESS
           && !PANEL_HAS_FLAG (event->state, GDK_CONTROL_MASK)))
    return FALSE;

  xfce_panel_plugin_block_autohide (XFCE_PANEL_PLUGIN (plugin), TRUE);

  if (button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  /* show the menu */
  gtk_menu_popup_at_widget (GTK_MENU (plugin->menu), button,
                            xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_VERTICAL
                            ? GDK_GRAVITY_NORTH_EAST : GDK_GRAVITY_SOUTH_WEST,
                            GDK_GRAVITY_NORTH_WEST,
                            (GdkEvent *) event);

  return TRUE;
}



static void
applications_menu_button_theme_changed (ApplicationsMenuPlugin *plugin)
{
  XfcePanelPlugin *panel_plugin = XFCE_PANEL_PLUGIN (plugin);

  applications_menu_plugin_size_changed (panel_plugin,
      xfce_panel_plugin_get_size (panel_plugin));
}
