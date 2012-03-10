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
#define DEFAULT_TITLE     _("Applications Menu")
#define DEFAULT_ICON_NAME "xfce4-panel-menu"
#define DEFAULT_ICON_SIZE (16)



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

  guint            show_generic_names : 1;
  guint            show_menu_icons : 1;
  guint            show_tooltips : 1;
  guint            show_button_title : 1;
  gchar           *button_title;
  gchar           *button_icon;
  gboolean         custom_menu;
  gchar           *custom_menu_file;

  /* temp item we store here when the
   * properties dialog is opened */
  GtkWidget       *dialog_icon;
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
  PROP_CUSTOM_MENU_FILE
};

static const GtkTargetEntry dnd_target_list[] = {
  { "text/uri-list", 0, 0 }
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
static void      applications_menu_plugin_menu_reload          (ApplicationsMenuPlugin *plugin);
static gboolean  applications_menu_plugin_menu                 (GtkWidget              *button,
                                                                GdkEventButton         *event,
                                                                ApplicationsMenuPlugin *plugin);



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ApplicationsMenuPlugin, applications_menu_plugin)



static GtkIconSize menu_icon_size = GTK_ICON_SIZE_INVALID;



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
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_MENU_ICONS,
                                   g_param_spec_boolean ("show-menu-icons",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_TOOLTIPS,
                                   g_param_spec_boolean ("show-tooltips",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_BUTTON_TITLE,
                                   g_param_spec_boolean ("show-button-title",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BUTTON_TITLE,
                                   g_param_spec_string ("button-title",
                                                        NULL, NULL,
                                                        DEFAULT_TITLE,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_BUTTON_ICON,
                                   g_param_spec_string ("button-icon",
                                                        NULL, NULL,
                                                        DEFAULT_ICON_NAME,
                                                        EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_MENU,
                                   g_param_spec_boolean ("custom-menu",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_CUSTOM_MENU_FILE,
                                   g_param_spec_string ("custom-menu-file",
                                                        NULL, NULL,
                                                        NULL,
                                                        EXO_PARAM_READWRITE));

  menu_icon_size = gtk_icon_size_from_name ("panel-applications-menu");
  if (menu_icon_size == GTK_ICON_SIZE_INVALID)
    menu_icon_size = gtk_icon_size_register ("panel-applications-menu",
                                             DEFAULT_ICON_SIZE,
                                             DEFAULT_ICON_SIZE);
}



static void
applications_menu_plugin_init (ApplicationsMenuPlugin *plugin)
{
  const gchar *desktop;

  plugin->show_menu_icons = TRUE;
  plugin->show_button_title = TRUE;
  plugin->custom_menu = FALSE;

  /* if the value is unset, fallback to XFCE, if the
   * value is empty, allow all applications in the menu */
  desktop = g_getenv ("XDG_CURRENT_DESKTOP");
  if (G_LIKELY (desktop == NULL))
    desktop = "XFCE";
  else if (*desktop == '\0')
    desktop = NULL;

  panel_debug (PANEL_DEBUG_APPLICATIONSMENU,
               "XDG_MENU_PREFIX is set to \"%s\", menu environment is \"%s\"",
               g_getenv ("XDG_MENU_PREFIX"), desktop);

  garcon_set_environment (desktop);

  plugin->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->button);
  gtk_widget_set_name (plugin->button, "applicationmenu-button");
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (plugin->button, DEFAULT_TITLE);
  g_signal_connect (G_OBJECT (plugin->button), "button-press-event",
      G_CALLBACK (applications_menu_plugin_menu), plugin);

  plugin->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 1);
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->box);
  gtk_widget_show (plugin->box);

  plugin->icon = xfce_panel_image_new_from_source (DEFAULT_ICON_NAME);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->icon, FALSE, FALSE, 0);
  gtk_widget_show (plugin->icon);

  plugin->label = gtk_label_new (DEFAULT_TITLE);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->label, FALSE, FALSE, 0);
  gtk_widget_show (plugin->label);
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
      g_value_set_boolean (value, plugin->show_generic_names);
      break;

    case PROP_SHOW_MENU_ICONS:
      g_value_set_boolean (value, plugin->show_menu_icons);
      break;

    case PROP_SHOW_TOOLTIPS:
      g_value_set_boolean (value, plugin->show_tooltips);
      break;

    case PROP_SHOW_BUTTON_TITLE:
      g_value_set_boolean (value, plugin->show_button_title);
      break;

    case PROP_BUTTON_TITLE:
      g_value_set_string (value, plugin->button_title == NULL ?
          DEFAULT_TITLE : plugin->button_title);
      break;

    case PROP_BUTTON_ICON:
      g_value_set_string (value, exo_str_is_empty (plugin->button_icon) ?
          DEFAULT_ICON_NAME : plugin->button_icon);
      break;

    case PROP_CUSTOM_MENU:
      g_value_set_boolean (value, plugin->custom_menu);
      break;

    case PROP_CUSTOM_MENU_FILE:
      g_value_set_string (value, plugin->custom_menu_file);
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
  gboolean                reload_menu = FALSE;

  switch (prop_id)
    {
    case PROP_SHOW_GENERIC_NAMES:
      plugin->show_generic_names = g_value_get_boolean (value);
      reload_menu = TRUE;
      break;

    case PROP_SHOW_MENU_ICONS:
      plugin->show_menu_icons = g_value_get_boolean (value);
      reload_menu = TRUE;
      break;

    case PROP_SHOW_TOOLTIPS:
      plugin->show_tooltips = g_value_get_boolean (value);
      reload_menu = TRUE;
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
          exo_str_is_empty (plugin->button_title) ? NULL : plugin->button_title);

      /* check if the label still fits */
      if (xfce_panel_plugin_get_mode (XFCE_PANEL_PLUGIN (plugin)) == XFCE_PANEL_PLUGIN_MODE_DESKBAR
          && plugin->show_button_title)
        {
          force_a_resize = TRUE;
        }
      break;

    case PROP_BUTTON_ICON:
    g_message ("set icon");
      g_free (plugin->button_icon);
      plugin->button_icon = g_value_dup_string (value);
      xfce_panel_image_set_from_source (XFCE_PANEL_IMAGE (plugin->icon),
          exo_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon);

      force_a_resize = TRUE;
      break;

    case PROP_CUSTOM_MENU:
      plugin->custom_menu = g_value_get_boolean (value);
      reload_menu = TRUE;
      break;

    case PROP_CUSTOM_MENU_FILE:
      g_free (plugin->custom_menu_file);
      plugin->custom_menu_file = g_value_dup_string (value);
      reload_menu = TRUE;
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

  if (reload_menu)
    applications_menu_plugin_menu_reload (plugin);
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
    { NULL }
  };

  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* bind all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  gtk_widget_show (plugin->button);
}



static void
applications_menu_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);

  if (plugin->menu != NULL)
    gtk_widget_destroy (plugin->menu);

  g_free (plugin->button_title);
  g_free (plugin->button_icon);
  g_free (plugin->custom_menu_file);
}



static gboolean
applications_menu_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                       gint             size)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  gint                    row_size;
  gint                    icon_size;
  GtkStyle               *style;
  XfcePanelPluginMode     mode;
  GtkRequisition          label_size;
  GtkOrientation          orientation;
  gint                    border_thickness;
  gdouble                 icon_wh_ratio;
  GdkPixbuf              *icon;

  row_size = size / xfce_panel_plugin_get_nrows (panel_plugin);

  gtk_box_set_child_packing (GTK_BOX (plugin->box), plugin->icon,
                             !plugin->show_button_title,
                             !plugin->show_button_title,
                             0, GTK_PACK_START);

  mode = xfce_panel_plugin_get_mode (panel_plugin);

  if (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
    orientation = GTK_ORIENTATION_HORIZONTAL;
  else
    orientation = GTK_ORIENTATION_VERTICAL;

  if (!plugin->show_button_title)
    {
      xfce_panel_image_set_size (XFCE_PANEL_IMAGE (plugin->icon), -1);

        /* get scale of the icon for non-squared custom files */
        if (mode != XFCE_PANEL_PLUGIN_MODE_DESKBAR
            && plugin->button_icon != NULL
            && g_path_is_absolute (plugin->button_icon))
          {
            icon = gdk_pixbuf_new_from_file (plugin->button_icon, NULL);
            if (G_LIKELY (icon != NULL))
              {
                icon_wh_ratio = (gdouble) gdk_pixbuf_get_width (icon) / (gdouble) gdk_pixbuf_get_height (icon);
                g_object_unref (G_OBJECT (icon));
              }
          }
        else
          {
            icon_wh_ratio = 1.0;
          }

      if (mode == XFCE_PANEL_PLUGIN_MODE_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), row_size * icon_wh_ratio, size);
      else if (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, row_size / icon_wh_ratio);
      else
        gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, row_size);
    }
  else
    {
      style = gtk_widget_get_style (plugin->button);
      border_thickness = 2 * MAX (style->xthickness, style->ythickness) + 2;

      icon_size = row_size - border_thickness;
      xfce_panel_image_set_size (XFCE_PANEL_IMAGE (plugin->icon), icon_size);
      gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), -1, -1);

      if (mode == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
        {
          /* check if the label fits next to the icon */
          gtk_widget_size_request (GTK_WIDGET (plugin->label), &label_size);
          if (label_size.width <= size - border_thickness - icon_size)
            orientation = GTK_ORIENTATION_HORIZONTAL;
        }
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
  GtkWidget *chooser;
  gchar     *icon;

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_PANEL_IMAGE (plugin->dialog_icon));

  chooser = exo_icon_chooser_dialog_new (_("Select An Icon"),
                                         GTK_WINDOW (gtk_widget_get_toplevel (button)),
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (chooser),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL, -1);

  exo_icon_chooser_dialog_set_icon (EXO_ICON_CHOOSER_DIALOG (chooser),
      exo_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon);

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      icon = exo_icon_chooser_dialog_get_icon (EXO_ICON_CHOOSER_DIALOG (chooser));
      g_object_set (G_OBJECT (plugin), "button-icon", icon, NULL);
      g_free (icon);
    }

  gtk_widget_destroy (chooser);
}



static void
applications_menu_plugin_configure_plugin_edit (GtkWidget              *button,
                                                ApplicationsMenuPlugin *plugin)
{
  GError      *error = NULL;
  const gchar *command = "alacarte";

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_WIDGET (button));

  if (!xfce_spawn_command_line_on_screen (gtk_widget_get_screen (button), command,
                                          FALSE, FALSE, &error))
    {
      xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\"."), command);
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
      exo_mutual_binding_new (G_OBJECT (plugin), check_names[i],
                              G_OBJECT (object), "active");
    }

  object = gtk_builder_get_object (builder, "button-title");
  panel_return_if_fail (GTK_IS_ENTRY (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "button-title",
                          G_OBJECT (object), "text");

  object = gtk_builder_get_object (builder, "icon-button");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  g_signal_connect (G_OBJECT (object), "clicked",
     G_CALLBACK (applications_menu_plugin_configure_plugin_icon_chooser), plugin);

  plugin->dialog_icon = xfce_panel_image_new_from_source (
      exo_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon);
  xfce_panel_image_set_size (XFCE_PANEL_IMAGE (plugin->dialog_icon), 48);
  gtk_container_add (GTK_CONTAINER (object), plugin->dialog_icon);
  g_object_add_weak_pointer (G_OBJECT (plugin->dialog_icon), (gpointer) &plugin->dialog_icon);
  gtk_widget_show (plugin->dialog_icon);

  /* whether we show the edit menu button */
  object = gtk_builder_get_object (builder, "edit-menu-button");
  panel_return_if_fail (GTK_IS_BUTTON (object));
  path = g_find_program_in_path ("alacarte");
  if (path != NULL)
    {
      object2 = gtk_builder_get_object (builder, "use-default-menu");
      panel_return_if_fail (GTK_IS_RADIO_BUTTON (object2));
      exo_binding_new (G_OBJECT (object2), "active", G_OBJECT (object), "sensitive");
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
  exo_mutual_binding_new (G_OBJECT (plugin), "custom-menu",
                          G_OBJECT (object), "active");

  /* sensitivity of custom file selector */
  object2 = gtk_builder_get_object (builder, "custom-box");
  panel_return_if_fail (GTK_IS_WIDGET (object2));
  exo_binding_new (G_OBJECT (object), "active", G_OBJECT (object2), "sensitive");

  object = gtk_builder_get_object (builder, "custom-file");
  panel_return_if_fail (GTK_IS_FILE_CHOOSER_BUTTON (object));
  if (!exo_str_is_empty (plugin->custom_menu_file))
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
      && GTK_WIDGET_VISIBLE (panel_plugin)
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
                                          GtkWidget *button)
{
  panel_return_if_fail (button == NULL || GTK_IS_TOGGLE_BUTTON (button));
  panel_return_if_fail (GTK_IS_MENU (menu));

  /* button is NULL when we popup the menu under the cursor position */
  if (button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  gtk_menu_popdown (GTK_MENU (menu));
}



static void
applications_menu_plugin_append_quoted (GString     *string,
                                        const gchar *unquoted)
{
  gchar *quoted;

  quoted = g_shell_quote (unquoted);
  g_string_append (string, quoted);
  g_free (quoted);
}



static void
applications_menu_plugin_menu_item_activate (GtkWidget      *mi,
                                             GarconMenuItem *item)
{
  GString      *string;
  const gchar  *command;
  const gchar  *p;
  const gchar  *tmp;
  gchar       **argv;
  gboolean      result = FALSE;
  gchar        *uri;
  GError       *error = NULL;

  panel_return_if_fail (GTK_IS_WIDGET (mi));
  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));

  command = garcon_menu_item_get_command (item);
  if (exo_str_is_empty (command))
    return;

  string = g_string_sized_new (100);

  if (garcon_menu_item_requires_terminal (item))
    g_string_append (string, "exo-open --launch TerminalEmulator ");

  /* expand the field codes */
  for (p = command; *p != '\0'; ++p)
    {
      if (G_UNLIKELY (p[0] == '%' && p[1] != '\0'))
        {
          switch (*++p)
            {
            case 'f': case 'F':
            case 'u': case 'U':
              /* TODO for dnd, not a regression, xfdesktop never had this */
              break;

            case 'i':
              tmp = garcon_menu_item_get_icon_name (item);
              if (!exo_str_is_empty (tmp))
                {
                  g_string_append (string, "--icon ");
                  applications_menu_plugin_append_quoted (string, tmp);
                }
              break;

            case 'c':
              tmp = garcon_menu_item_get_name (item);
              if (!exo_str_is_empty (tmp))
                applications_menu_plugin_append_quoted (string, tmp);
              break;

            case 'k':
              uri = garcon_menu_item_get_uri (item);
              if (!exo_str_is_empty (uri))
                applications_menu_plugin_append_quoted (string, uri);
              g_free (uri);
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

  /* parse and spawn command */
  if (g_shell_parse_argv (string->str, NULL, &argv, &error))
    {
      result = xfce_spawn_on_screen (gtk_widget_get_screen (mi),
                                     garcon_menu_item_get_path (item),
                                     argv, NULL, G_SPAWN_SEARCH_PATH,
                                     garcon_menu_item_supports_startup_notification (item),
                                     gtk_get_current_event_time (),
                                     garcon_menu_item_get_icon_name (item),
                                     &error);

      g_strfreev (argv);
    }

  if (G_UNLIKELY (!result))
    {
      xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\"."), command);
      g_error_free (error);
    }

  g_string_free (string, TRUE);
}



static void
applications_menu_plugin_menu_item_drag_begin (GarconMenuItem   *item,
                                               GdkDragContext   *drag_context)
{
  const gchar *icon_name;

  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));

  icon_name = garcon_menu_item_get_icon_name (item);
  if (!exo_str_is_empty (icon_name))
    gtk_drag_set_icon_name (drag_context, icon_name, 0, 0);
}



static void
applications_menu_plugin_menu_item_drag_data_get (GarconMenuItem   *item,
                                                  GdkDragContext   *drag_context,
                                                  GtkSelectionData *selection_data,
                                                  guint             info,
                                                  guint             drag_time)
{
  gchar *uris[2] = { NULL, NULL };

  panel_return_if_fail (GARCON_IS_MENU_ITEM (item));

  uris[0] = garcon_menu_item_get_uri (item);
  if (G_LIKELY (uris[0] != NULL))
    {
      gtk_selection_data_set_uris (selection_data, uris);
      g_free (uris[0]);
    }
}



static void
applications_menu_plugin_menu_item_drag_end (ApplicationsMenuPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_TOGGLE_BUTTON (plugin->button));
  panel_return_if_fail (GTK_IS_MENU (plugin->menu));

  /* selection-done is never called, so handle that manually */
  applications_menu_plugin_menu_deactivate (plugin->menu, plugin->button);
}



static void
applications_menu_plugin_menu_reload (ApplicationsMenuPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));

  if (plugin->menu != NULL)
    {
      panel_debug (PANEL_DEBUG_APPLICATIONSMENU,
                   "destroy menu for reload");

      /* if the menu is opened, do not destroy it under the users'
       * cursor, else destroy the menu in an idle, to give garcon
       * time to finalize the events that triggered the reload */
      if (GTK_WIDGET_VISIBLE (plugin->menu))
        g_signal_connect (G_OBJECT (plugin->menu), "selection-done",
            G_CALLBACK (exo_gtk_object_destroy_later), NULL);
      else
        exo_gtk_object_destroy_later (GTK_OBJECT (plugin->menu));
    }
}



static gboolean
applications_menu_plugin_menu_add (GtkWidget              *gtk_menu,
                                   GtkWidget              *button,
                                   GarconMenu             *menu,
                                   ApplicationsMenuPlugin *plugin)
{
  GList               *elements, *li;
  GtkWidget           *mi, *image;
  const gchar         *name, *icon_name;
  const gchar         *comment;
  GtkWidget           *submenu;
  gboolean             has_children = FALSE;
  gint                 size = DEFAULT_ICON_SIZE, w, h;
  const gchar         *command;
  GarconMenuDirectory *directory;

  panel_return_val_if_fail (GTK_IS_MENU (gtk_menu), FALSE);
  panel_return_val_if_fail (GARCON_IS_MENU (menu), FALSE);
  panel_return_val_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (button == NULL || GTK_IS_TOGGLE_BUTTON (button), FALSE);

  if (gtk_icon_size_lookup (menu_icon_size, &w, &h))
    size = MIN (w, h);

  elements = garcon_menu_get_elements (menu);
  for (li = elements; li != NULL; li = li->next)
    {
      panel_return_val_if_fail (GARCON_IS_MENU_ELEMENT (li->data), FALSE);
      if (GARCON_IS_MENU_ITEM (li->data))
        {
          g_signal_connect_swapped (G_OBJECT (li->data), "changed",
              G_CALLBACK (applications_menu_plugin_menu_reload), plugin);

          if (!garcon_menu_element_get_visible (li->data))
            continue;

          name = NULL;
          if (plugin->show_generic_names)
            name = garcon_menu_item_get_generic_name (li->data);
          if (name == NULL)
            name = garcon_menu_item_get_name (li->data);
          if (G_UNLIKELY (name == NULL))
            continue;

          mi = gtk_image_menu_item_new_with_label (name);
          gtk_menu_shell_append (GTK_MENU_SHELL (gtk_menu), mi);
          g_signal_connect (G_OBJECT (mi), "activate",
              G_CALLBACK (applications_menu_plugin_menu_item_activate), li->data);
          gtk_widget_show (mi);

          if (plugin->show_tooltips)
            {
              comment = garcon_menu_item_get_comment (li->data);
              if (!exo_str_is_empty (comment))
                gtk_widget_set_tooltip_text (mi, comment);
            }

          /* dragging items from the menu to the panel */
          gtk_drag_source_set (mi, GDK_BUTTON1_MASK, dnd_target_list,
              G_N_ELEMENTS (dnd_target_list), GDK_ACTION_COPY);
          g_signal_connect_swapped (G_OBJECT (mi), "drag-begin",
              G_CALLBACK (applications_menu_plugin_menu_item_drag_begin), li->data);
          g_signal_connect_swapped (G_OBJECT (mi), "drag-data-get",
              G_CALLBACK (applications_menu_plugin_menu_item_drag_data_get), li->data);
          g_signal_connect_swapped (G_OBJECT (mi), "drag-end",
              G_CALLBACK (applications_menu_plugin_menu_item_drag_end), plugin);

          command = garcon_menu_item_get_command (li->data);
          if (G_UNLIKELY (exo_str_is_empty (command)))
            gtk_widget_set_sensitive (mi, FALSE);

          if (plugin->show_menu_icons)
            {
              icon_name = garcon_menu_item_get_icon_name (li->data);
              if (exo_str_is_empty (icon_name))
                icon_name = "applications-other";

              image = xfce_panel_image_new_from_source (icon_name);
              xfce_panel_image_set_size (XFCE_PANEL_IMAGE (image), size);
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              gtk_widget_show (image);
            }

          has_children = TRUE;
        }
      else if (GARCON_IS_MENU_SEPARATOR (li->data))
        {
          mi = gtk_separator_menu_item_new ();
          gtk_menu_shell_append (GTK_MENU_SHELL (gtk_menu), mi);
          gtk_widget_show (mi);
        }
      else if (GARCON_IS_MENU (li->data))
        {
          /* the element check for menu also copies the item list to
           * check if all the elements are visible, we do that with the
           * return value of this function, so avoid that and only check
           * the visibility of the menu directory */
          directory = garcon_menu_get_directory (li->data);
          if (directory != NULL
              && !garcon_menu_directory_get_visible (directory))
            continue;

          submenu = gtk_menu_new ();
          if (applications_menu_plugin_menu_add (submenu, button, li->data, plugin))
            {
              name = garcon_menu_element_get_name (li->data);
              mi = gtk_image_menu_item_new_with_label (name);
              gtk_menu_shell_append (GTK_MENU_SHELL (gtk_menu), mi);
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), submenu);
              g_signal_connect (G_OBJECT (submenu), "selection-done",
                  G_CALLBACK (applications_menu_plugin_menu_deactivate), button);
              gtk_widget_show (mi);

              g_signal_connect_swapped (G_OBJECT (li->data), "directory-changed",
                  G_CALLBACK (applications_menu_plugin_menu_reload), plugin);

              if (plugin->show_menu_icons)
                {
                  icon_name = garcon_menu_element_get_icon_name (li->data);
                  if (exo_str_is_empty (icon_name))
                    icon_name = "applications-other";

                  image = xfce_panel_image_new_from_source (icon_name);
                  xfce_panel_image_set_size (XFCE_PANEL_IMAGE (image), size);
                  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
                  gtk_widget_show (image);
                }

              has_children = TRUE;
            }
          else
            {
              gtk_widget_destroy (submenu);
            }
        }
    }

  g_list_free (elements);

  return has_children;
}



static gboolean
applications_menu_plugin_menu (GtkWidget              *button,
                               GdkEventButton         *event,
                               ApplicationsMenuPlugin *plugin)
{
  GtkWidget  *mi;
  GarconMenu *menu = NULL;
  GError     *error = NULL;
  gchar      *filename;
  GFile      *file;

  panel_return_val_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (button == NULL || plugin->button == button, FALSE);

  if (event != NULL
      && !(event->button == 1
           && event->type == GDK_BUTTON_PRESS
           && !PANEL_HAS_FLAG (event->state, GDK_CONTROL_MASK)))
    return FALSE;

  if (button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  if (plugin->menu == NULL)
    {
      if (G_UNLIKELY (plugin->custom_menu
          && plugin->custom_menu_file != NULL))
        menu = garcon_menu_new_for_path (plugin->custom_menu_file);

      /* use the applications menu, this also respects the
       * XDG_MENU_PREFIX environment variable */
      if (G_LIKELY (menu == NULL))
        menu = garcon_menu_new_applications ();

      if (menu != NULL
          && garcon_menu_load (menu, NULL, &error))
        {
          plugin->menu = gtk_menu_new ();
          g_signal_connect (G_OBJECT (plugin->menu), "selection-done",
               G_CALLBACK (applications_menu_plugin_menu_deactivate), button);
          g_object_add_weak_pointer (G_OBJECT (plugin->menu), (gpointer) &plugin->menu);

          if (!applications_menu_plugin_menu_add (plugin->menu, button, menu, plugin))
            {
              mi = gtk_menu_item_new_with_label (_("No applications found"));
              gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), mi);
              gtk_widget_set_sensitive (mi, FALSE);
              gtk_widget_show (mi);
            }

          /* watch the menu for changes */
          g_object_weak_ref (G_OBJECT (plugin->menu),
              (GWeakNotify) g_object_unref, menu);
          g_signal_connect_swapped (G_OBJECT (menu), "reload-required",
              G_CALLBACK (applications_menu_plugin_menu_reload), plugin);

          /* debugging information */
          file = garcon_menu_get_file (menu);
          filename = g_file_get_parse_name (file);
          g_object_unref (G_OBJECT (file));

          panel_debug (PANEL_DEBUG_APPLICATIONSMENU,
                       "loading from %s", filename);
          g_free (filename);
        }
      else
        {
          xfce_dialog_show_error (NULL, error, _("Failed to load the applications menu"));

          if (button != NULL)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

          if (G_LIKELY (error != NULL))
            g_error_free (error);
          if (G_LIKELY (menu != NULL))
            g_object_unref (G_OBJECT (menu));

          return FALSE;
        }
    }

  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  button != NULL ? xfce_panel_plugin_position_menu : NULL,
                  plugin, 1,
                  event != NULL ? event->time : gtk_get_current_event_time ());

  return TRUE;
}
