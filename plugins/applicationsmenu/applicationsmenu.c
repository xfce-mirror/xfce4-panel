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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
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

#include "applicationsmenu.h"
#include "applicationsmenu-dialog_ui.h"



#define DEFAULT_ICON_NAME "xfce4-panel-menu"
#define DEFAULT_TITLE     _("Xfce Menu")
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

  guint            show_generic_names : 1;
  guint            show_menu_icons : 1;
  guint            show_button_title : 1;
  gchar           *button_title;
  gchar           *button_icon;
  guint            custom_menu : 1;
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
  PROP_SHOW_BUTTON_TITLE,
  PROP_BUTTON_TITLE,
  PROP_BUTTON_ICON,
  PROP_CUSTOM_MENU,
  PROP_CUSTOM_MENU_FILE
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
static void      applications_menu_plugin_orientation_changed  (XfcePanelPlugin        *panel_plugin,
                                                                GtkOrientation          orientation);
static void      applications_menu_plugin_configure_plugin     (XfcePanelPlugin        *panel_plugin);
static gboolean  applications_menu_plugin_remote_event         (XfcePanelPlugin        *panel_plugin,
                                                                const gchar            *name,
                                                                const GValue           *value);
static void      applications_menu_plugin_menu                 (GtkWidget              *button,
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
  plugin_class->orientation_changed = applications_menu_plugin_orientation_changed;
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
  plugin->show_menu_icons = TRUE;
  plugin->show_button_title = TRUE;

  garcon_set_environment ("XFCE");

  plugin->button = xfce_panel_create_toggle_button ();
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->button);
  gtk_widget_set_name (plugin->button, "applicationmenu-button");
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (plugin->button, DEFAULT_TITLE);
  g_signal_connect (G_OBJECT (plugin->button), "toggled",
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

    case PROP_SHOW_BUTTON_TITLE:
      g_value_set_boolean (value, plugin->show_button_title);
      break;

    case PROP_BUTTON_TITLE:
      g_value_set_string (value, exo_str_is_empty (plugin->button_title) ?
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

  switch (prop_id)
    {
    case PROP_SHOW_GENERIC_NAMES:
      plugin->show_generic_names = g_value_get_boolean (value);
      break;

    case PROP_SHOW_MENU_ICONS:
      plugin->show_menu_icons = g_value_get_boolean (value);
      break;

    case PROP_SHOW_BUTTON_TITLE:
      plugin->show_button_title = g_value_get_boolean (value);
      if (plugin->show_button_title)
        gtk_widget_show (plugin->label);
      else
        gtk_widget_hide (plugin->label);
      applications_menu_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
          xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
      break;

    case PROP_BUTTON_TITLE:
      g_free (plugin->button_title);
      plugin->button_title = g_value_dup_string (value);
      gtk_label_set_text (GTK_LABEL (plugin->label),
          plugin->button_title != NULL ? plugin->button_title : "");
      gtk_widget_set_tooltip_text (plugin->button, plugin->button_title);
      break;

    case PROP_BUTTON_ICON:
      g_free (plugin->button_icon);
      plugin->button_icon = g_value_dup_string (value);
      xfce_panel_image_set_from_source (XFCE_PANEL_IMAGE (plugin->icon),
          exo_str_is_empty (plugin->button_icon) ? DEFAULT_ICON_NAME : plugin->button_icon);
      break;

    case PROP_CUSTOM_MENU:
      plugin->custom_menu = g_value_get_boolean (value);
      break;

    case PROP_CUSTOM_MENU_FILE:
      g_free (plugin->custom_menu_file);
      plugin->custom_menu_file = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
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

  g_free (plugin->button_title);
  g_free (plugin->button_icon);
  g_free (plugin->custom_menu_file);
}



static gboolean
applications_menu_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                                       gint             size)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  gint                    icon_size;
  GtkStyle               *style;

  gtk_box_set_child_packing (GTK_BOX (plugin->box), plugin->icon,
                             !plugin->show_button_title, !plugin->show_button_title,
                             0, GTK_PACK_START);

  if (!plugin->show_button_title)
    {
      xfce_panel_image_set_size (XFCE_PANEL_IMAGE (plugin->icon), -1);
      gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, size);
    }
  else
    {
      style = gtk_widget_get_style (plugin->button);
      icon_size = size - 2 * MAX (style->xthickness, style->ythickness);
      xfce_panel_image_set_size (XFCE_PANEL_IMAGE (plugin->icon), icon_size);
      gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), -1, -1);
    }

  return TRUE;
}



static void
applications_menu_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                              GtkOrientation   orientation)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);

  xfce_hvbox_set_orientation (XFCE_HVBOX (plugin->box), orientation);
  gtk_label_set_angle (GTK_LABEL (plugin->label), orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 270);
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
      xfce_panel_image_set_from_source (XFCE_PANEL_IMAGE (plugin->dialog_icon), icon);
      g_free (icon);
    }

  gtk_widget_destroy (chooser);
}



static void
applications_menu_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ApplicationsMenuPlugin *plugin = XFCE_APPLICATIONS_MENU_PLUGIN (panel_plugin);
  GtkBuilder             *builder;
  GObject                *dialog, *object, *object2;

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, applicationsmenu_dialog_ui,
                                     applicationsmenu_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  object = gtk_builder_get_object (builder, "show-generic-names");
  panel_return_if_fail (GTK_IS_CHECK_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "show-generic-names",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "show-menu-icons");
  panel_return_if_fail (GTK_IS_CHECK_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "show-menu-icons",
                          G_OBJECT (object), "active");

  object = gtk_builder_get_object (builder, "show-button-title");
  panel_return_if_fail (GTK_IS_CHECK_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "show-button-title",
                          G_OBJECT (object), "active");

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

  object = gtk_builder_get_object (builder, "use-custom-file");
  panel_return_if_fail (GTK_IS_RADIO_BUTTON (object));
  exo_mutual_binding_new (G_OBJECT (plugin), "custom-menu",
                          G_OBJECT (object), "active");
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
      && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (plugin->button)))
    {
      if (value != NULL
          && G_VALUE_HOLDS_BOOLEAN (value)
          && g_value_get_boolean (value))
        {
          /* show menu under cursor */
          applications_menu_plugin_menu (NULL, plugin);
        }
      else
        {
          /* show the menu at the button */
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->button), TRUE);
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

  if (button != NULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

  /* delay destruction so we can handle the activate event first */
  exo_gtk_object_destroy_later (GTK_OBJECT (menu));
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



static gboolean
applications_menu_plugin_menu_add (GtkWidget              *gtk_menu,
                                   GarconMenu             *menu,
                                   ApplicationsMenuPlugin *plugin)
{
  GList               *elements, *li;
  GtkWidget           *mi, *image;
  const gchar         *name, *icon_name;
  GtkWidget           *submenu;
  gboolean             has_children = FALSE;
  gint                 size = DEFAULT_ICON_SIZE, w, h;
  const gchar         *command;
  GarconMenuDirectory *directory;

  panel_return_val_if_fail (GTK_IS_MENU (gtk_menu), FALSE);
  panel_return_val_if_fail (GARCON_IS_MENU (menu), FALSE);
  panel_return_val_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin), FALSE);

  if (gtk_icon_size_lookup (menu_icon_size, &w, &h))
    size = MIN (w, h);

  elements = garcon_menu_get_elements (menu);
  for (li = elements; li != NULL; li = li->next)
    {
      panel_return_val_if_fail (GARCON_IS_MENU_ELEMENT (li->data), FALSE);
      if (GARCON_IS_MENU_ITEM (li->data))
        {
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

          g_signal_connect_data (G_OBJECT (mi), "activate",
              G_CALLBACK (applications_menu_plugin_menu_item_activate),
              g_object_ref (li->data), (GClosureNotify) g_object_unref, 0);
          gtk_widget_show (mi);

          command = garcon_menu_item_get_command (li->data);
          gtk_widget_set_sensitive (mi, !exo_str_is_empty (command));

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
          if (!garcon_menu_directory_get_visible (directory))
            continue;

          submenu = gtk_menu_new ();
          if (applications_menu_plugin_menu_add (submenu, li->data, plugin))
            {
              name = garcon_menu_element_get_name (li->data);
              mi = gtk_image_menu_item_new_with_label (name);
              gtk_menu_shell_append (GTK_MENU_SHELL (gtk_menu), mi);
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), submenu);
              gtk_widget_show (mi);

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



static void
applications_menu_plugin_menu (GtkWidget              *button,
                               ApplicationsMenuPlugin *plugin)
{
  GtkWidget  *gtk_menu, *mi;
  GarconMenu *menu;
  GError     *error = NULL;

  panel_return_if_fail (XFCE_IS_APPLICATIONS_MENU_PLUGIN (plugin));
  panel_return_if_fail (button == NULL || plugin->button == button);

  if (button != NULL
      && !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    return;

  if (G_UNLIKELY (plugin->custom_menu
      && plugin->custom_menu_file != NULL))
    menu = garcon_menu_new_for_path (plugin->custom_menu_file);
  else
    menu = garcon_menu_new_applications ();

  if (garcon_menu_load (menu, NULL, &error))
    {
      gtk_menu = gtk_menu_new ();
      g_signal_connect (G_OBJECT (gtk_menu), "deactivate",
           G_CALLBACK (applications_menu_plugin_menu_deactivate), button);

      if (!applications_menu_plugin_menu_add (gtk_menu, menu, plugin))
        {
          mi = gtk_menu_item_new_with_label (_("No applications found"));
          gtk_menu_shell_append (GTK_MENU_SHELL (gtk_menu), mi);
          gtk_widget_set_sensitive (mi, FALSE);
          gtk_widget_show (mi);
        }

      gtk_menu_popup (GTK_MENU (gtk_menu), NULL, NULL,
                      button != NULL ? xfce_panel_plugin_position_menu : NULL,
                      plugin, 1, gtk_get_current_event_time ());
    }
  else
    {
      xfce_dialog_show_error (NULL, error, _("Failed to load the applications menu"));
      g_error_free (error);
    }

  g_object_unref (G_OBJECT (menu));
}
