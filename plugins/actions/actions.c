/* $Id$ */
/*
 * Copyright (c) 2009 Nick Schermer <nick@xfce.org>
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
#include <xfconf/xfconf.h>
#include <exo/exo.h>

#include "actions.h"
#include "actions-dialog_glade.h"



static void     actions_plugin_get_property        (GObject               *object,
                                                    guint                  prop_id,
                                                    GValue                *value,
                                                    GParamSpec            *pspec);
static void     actions_plugin_set_property        (GObject               *object,
                                                    guint                  prop_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void     actions_plugin_construct           (XfcePanelPlugin       *panel_plugin);
static void     actions_plugin_free_data           (XfcePanelPlugin       *panel_plugin);
static gboolean actions_plugin_size_changed        (XfcePanelPlugin       *panel_plugin,
                                                    gint                   size);
static void     actions_plugin_configure_plugin    (XfcePanelPlugin       *panel_plugin);
static void     actions_plugin_orientation_changed (XfcePanelPlugin       *panel_plugin,
                                                    GtkOrientation         orientation);
static void     actions_plugin_button_clicked      (GtkWidget             *button,
                                                    ActionsPlugin         *plugin);



typedef enum
{
  ACTION_DISABLED = 0,
  ACTION_LOG_OUT_DIALOG,
  ACTION_LOG_OUT,
  ACTION_LOCK_SCREEN,
  ACTION_SHUT_DOWN,
  ACTION_RESTART,
  ACTION_SUSPEND,
  ACTION_HIBERNATE
}
ActionType;

typedef struct
{
  ActionType   type;
  const gchar *title;
  const gchar *icon_name;
  const gchar *command;
}
ActionEntry;

struct _ActionsPluginClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;

};

struct _ActionsPlugin
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* xfconf channel */
  XfconfChannel *channel;

  /* widgets */
  GtkWidget     *box;
  GtkWidget     *first_button;
  GtkWidget     *first_image;
  GtkWidget     *second_button;
  GtkWidget     *second_image;

  /* settings */
  ActionType     first_action;
  ActionType     second_action;
};

enum
{
  PROP_0,
  PROP_FIRST_ACTION,
  PROP_SECOND_ACTION
};

static ActionEntry action_entries[] =
{
  { ACTION_DISABLED, N_("Disabled"),
    NULL, NULL },
  { ACTION_LOG_OUT_DIALOG, N_("Log Out Dialog"),
    "system-log-out", "xfce4-session-logout" },
  { ACTION_LOG_OUT, N_("Log Out"),
    "system-log-out", "xfce4-session-logout --logout" },
  { ACTION_LOCK_SCREEN, N_("Lock Screen"),
    "system-lock-screen", "xflock4" },
  { ACTION_SHUT_DOWN, N_("Shut Down"),
    "system-shutdown", "xfce4-session-logout --halt" },
  { ACTION_RESTART, N_("Restart"),
    "xfsm-reboot", "xfce4-session-logout --reboot" },
  { ACTION_SUSPEND, N_("Suspend"),
    "system-suspend", "xfce4-session-logout --suspend" },
  { ACTION_HIBERNATE, N_("Hibernate"),
    "system-hibernate", "xfce4-session-logout --hibernate" }
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ActionsPlugin, actions_plugin)



static void
actions_plugin_class_init (ActionsPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;
  GObjectClass         *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = actions_plugin_set_property;
  gobject_class->get_property = actions_plugin_get_property;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = actions_plugin_construct;
  plugin_class->free_data = actions_plugin_free_data;
  plugin_class->size_changed = actions_plugin_size_changed;
  plugin_class->configure_plugin = actions_plugin_configure_plugin;
  plugin_class->orientation_changed = actions_plugin_orientation_changed;

  g_object_class_install_property (gobject_class,
                                   PROP_FIRST_ACTION,
                                   g_param_spec_uint ("first-action",
                                                      NULL, NULL,
                                                      ACTION_DISABLED,
                                                      ACTION_HIBERNATE - 1,
                                                      ACTION_LOG_OUT_DIALOG,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SECOND_ACTION,
                                   g_param_spec_uint ("second-action",
                                                      NULL, NULL,
                                                      ACTION_DISABLED,
                                                      ACTION_HIBERNATE,
                                                      ACTION_DISABLED,
                                                      EXO_PARAM_READWRITE));
}



static void
actions_plugin_init (ActionsPlugin *plugin)
{
  GtkWidget   *widget;
  ActionEntry *entry = &action_entries[ACTION_LOG_OUT_DIALOG];

  /* initialize */
  plugin->first_action = ACTION_LOG_OUT_DIALOG;
  plugin->second_action = ACTION_DISABLED;

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* initialize xfconf */
  xfconf_init (NULL);

  plugin->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->box);

  plugin->first_button = widget = xfce_panel_create_button ();
  gtk_box_pack_start (GTK_BOX (plugin->box), widget, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (widget), "clicked",
      G_CALLBACK (actions_plugin_button_clicked), plugin);
  gtk_widget_set_tooltip_text (widget, _(entry->title));
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), widget);
  gtk_widget_show (widget);

  plugin->first_image = xfce_scaled_image_new_from_icon_name (entry->icon_name);
  gtk_container_add (GTK_CONTAINER (widget), plugin->first_image);
  gtk_widget_show (plugin->first_image);

  plugin->second_button = widget = xfce_panel_create_button ();
  gtk_box_pack_start (GTK_BOX (plugin->box), widget, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (widget), "clicked",
      G_CALLBACK (actions_plugin_button_clicked), plugin);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), widget);

  plugin->second_image = xfce_scaled_image_new ();
  gtk_container_add (GTK_CONTAINER (widget), plugin->second_image);
  gtk_widget_show (plugin->second_image);
}



static void
actions_plugin_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (object);

  switch (prop_id)
    {
      case PROP_FIRST_ACTION:
        g_value_set_uint (value, plugin->first_action - 1);
        break;

      case PROP_SECOND_ACTION:
        g_value_set_uint (value, plugin->second_action);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
actions_plugin_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (object);
  ActionType     action;

  switch (prop_id)
    {
      case PROP_FIRST_ACTION:
        /* set new value and update icon */
        action = plugin->first_action = g_value_get_uint (value) + 1;
        gtk_widget_set_tooltip_text (plugin->first_button,
            _(action_entries[action].title));
        xfce_scaled_image_set_from_icon_name (
            XFCE_SCALED_IMAGE (plugin->first_image),
            action_entries[action].icon_name);
        break;

      case PROP_SECOND_ACTION:
        /* set new value */
        action = plugin->second_action = g_value_get_uint (value);

        /* update button visibility and icon */
        if (action == ACTION_DISABLED)
          {
            gtk_widget_hide (plugin->second_button);
          }
        else
          {
            gtk_widget_show (plugin->second_button);
            gtk_widget_set_tooltip_text (plugin->second_button,
                _(action_entries[action].title));
            xfce_scaled_image_set_from_icon_name (
                XFCE_SCALED_IMAGE (plugin->second_image),
                action_entries[action].icon_name);
          }

        /* update plugin size */
        actions_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));

        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
actions_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  ActionsPlugin       *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  const PanelProperty  properties[] =
  {
    { "first-action", G_TYPE_UINT },
    { "second-action", G_TYPE_UINT },
    { NULL, G_TYPE_NONE }
  };

  /* open the xfconf channel */
  plugin->channel = panel_properties_get_channel ();

  /* bind all properties */
  panel_properties_bind (plugin->channel, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* set orientation and size */
  actions_plugin_orientation_changed (panel_plugin,
      xfce_panel_plugin_get_orientation (panel_plugin));

  /* show the plugin */
  gtk_widget_show (plugin->box);
}



static void
actions_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);

  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* release the xfonf channel */
  if (G_LIKELY (plugin->channel != NULL))
    g_object_unref (G_OBJECT (plugin->channel));

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static gboolean
actions_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                             gint             size)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  gint           width, height;

  /* init size */
  width = height = size;

  if (plugin->second_action != ACTION_DISABLED)
    {
      if (xfce_panel_plugin_get_orientation (panel_plugin) ==
          GTK_ORIENTATION_HORIZONTAL)
        width /= 2;
      else
        height /= 2;
    }

  /* set the plugin size */
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin),
                               width, height);

  return TRUE;
}



static void
actions_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  GtkBuilder    *builder;
  GObject       *dialog;
  GObject       *object;
  guint          i;

  panel_return_if_fail (XFCE_IS_ACTIONS_PLUGIN (plugin));
  panel_return_if_fail (XFCONF_IS_CHANNEL (plugin->channel));

  /* load the dialog from the glade file */
  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, actions_dialog_glade,
                                   actions_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify)
          xfce_panel_plugin_unblock_menu, panel_plugin);

      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked",
          G_CALLBACK (gtk_widget_destroy), dialog);

      /* populate the first store */
      object = gtk_builder_get_object (builder, "first-action-model");
      for (i = 1; i < G_N_ELEMENTS (action_entries); i++)
        gtk_list_store_insert_with_values (GTK_LIST_STORE (object), NULL, i - 1,
                                           0, _(action_entries[i].title), -1);

      object = gtk_builder_get_object (builder, "first-action");
      exo_mutual_binding_new (G_OBJECT (plugin), "first-action",
                              G_OBJECT (object), "active");

      /* populate the second store */
      object = gtk_builder_get_object (builder, "second-action-model");
      for (i = 0; i < G_N_ELEMENTS (action_entries); i++)
        gtk_list_store_insert_with_values (GTK_LIST_STORE (object), NULL, i,
                                           0, _(action_entries[i].title), -1);

      object = gtk_builder_get_object (builder, "second-action");
      exo_mutual_binding_new (G_OBJECT (plugin), "second-action",
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
actions_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                    GtkOrientation   orientation)
{
  ActionsPlugin  *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  GtkOrientation  box_orientation;

  /* box orientation is opposite to the panel orientation */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    box_orientation = GTK_ORIENTATION_VERTICAL;
  else
    box_orientation = GTK_ORIENTATION_HORIZONTAL;

  /* set orientation */
  xfce_hvbox_set_orientation (XFCE_HVBOX (plugin->box), box_orientation);

  /* update the plugin size */
  actions_plugin_size_changed (panel_plugin,
      xfce_panel_plugin_get_size (panel_plugin));
}



static void
actions_plugin_button_clicked (GtkWidget     *button,
                               ActionsPlugin *plugin)
{
  ActionType  action;
  GError     *error = NULL;

  panel_return_if_fail (XFCE_IS_ACTIONS_PLUGIN (plugin));

  /* get the action to execute */
  if (button == plugin->first_button)
    action = plugin->first_action;
  else
    action = plugin->second_action;

  /* spawn the command */
  if (!g_spawn_command_line_async (action_entries[action].command, &error))
    {
      /* show error */
      xfce_dialog_show_error (gtk_widget_get_screen (button),
                              error, _("Failed to execute command \"%s\""),
                              action_entries[action].command);
      g_error_free (error);
    }
}

