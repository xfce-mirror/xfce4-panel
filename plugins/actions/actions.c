/*
 * Copyright (C) 2009-2011 Nick Schermer <nick@xfce.org>
 * Copyright (c) 2009      Brian Tarricone <brian@tarricone.org>
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

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <exo/exo.h>
#include <dbus/dbus-glib.h>

#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>

#include "actions.h"
#include "actions-dialog_ui.h"



#define DEFAULT_ICON_SIZE (16)
#define DEFAULT_TIMEOUT   (30)



static void       actions_plugin_get_property        (GObject               *object,
                                                      guint                  prop_id,
                                                      GValue                *value,
                                                      GParamSpec            *pspec);
static void       actions_plugin_set_property        (GObject               *object,
                                                      guint                  prop_id,
                                                      const GValue          *value,
                                                      GParamSpec            *pspec);
static void       actions_plugin_construct           (XfcePanelPlugin       *panel_plugin);
static void       actions_plugin_free_data           (XfcePanelPlugin       *panel_plugin);
static gboolean   actions_plugin_size_changed        (XfcePanelPlugin       *panel_plugin,
                                                      gint                   size);
static void       actions_plugin_configure_plugin    (XfcePanelPlugin       *panel_plugin);
static void       actions_plugin_orientation_changed (XfcePanelPlugin       *panel_plugin,
                                                      GtkOrientation         orientation);
static void       actions_plugin_pack                (ActionsPlugin         *plugin);
static GPtrArray *actions_plugin_default_array       (void);
static void       actions_plugin_menu                (GtkWidget             *button,
                                                      ActionsPlugin         *plugin);



typedef enum
{
  APPEARANCE_TYPE_BUTTONS,
  APPEARANCE_TYPE_MENU
}
AppearanceType;

enum
{
  PROP_0,
  PROP_ITEMS,
  PROP_APPEARANCE,
  PROP_INVERT_ORIENTATION,
  PROP_ASK_CONFIRMATION
};

enum
{
  COLUMN_VISIBLE,
  COLUMN_NAME,
  COLUMN_TYPE
};

struct _ActionsPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _ActionsPlugin
{
  XfcePanelPlugin __parent__;

  AppearanceType  type;
  GPtrArray      *items;
  GtkWidget      *menu;
  guint           invert_orientation : 1;
  guint           ask_confirmation : 1;
  guint           pack_idle_id;
};

typedef enum
{
  ACTION_TYPE_0,
  ACTION_TYPE_SEPARATOR,
  ACTION_TYPE_LOGOUT,
  ACTION_TYPE_LOGOUT_DIALOG,
  ACTION_TYPE_SWITCH_USER,
  ACTION_TYPE_LOCK_SCREEN,
  ACTION_TYPE_HIBERNATE,
  ACTION_TYPE_SUSPEND,
  ACTION_TYPE_RESTART,
  ACTION_TYPE_SHUTDOWN,
  N_ACTION_TYPES
}
ActionType;

/* copied from xfce4-session/shutdown.h -- ORDER MATTERS.
 * The numbers correspond to the 'type' parameter of
 * org.xfce.Session.Manager.Shutdown */
typedef enum
{
  ACTION_SHUTDOWN_ASK = 0,
  ACTION_SHUTDOWN_LOGOUT,
  ACTION_SHUTDOWN_HALT,
  ACTION_SHUTDOWN_REBOOT,
  ACTION_SHUTDOWN_SUSPEND,
  ACTION_SHUTDOWN_HIBERNATE,
} ActionShutdownType;

typedef struct
{
  ActionType   type;
  const gchar *name;
  const gchar *name_mnemonic;
  const gchar *question;
  const gchar *status;
  const gchar *icon_name;
}
ActionEntry;

typedef struct
{
  ActionEntry *entry;
  GtkWidget   *dialog;
  gint         time_left;
  guint        unattended : 1;
}
ActionTimeout;

static ActionEntry action_entries[] =
{
  { ACTION_TYPE_LOGOUT,
    N_("Log Out"),
    N_("_Log Out"),
    N_("Are you sure you want to log out?"),
    N_("Logging out in %d seconds."),
    "system-log-out"
  },
  { ACTION_TYPE_LOGOUT_DIALOG,
    N_("Log Out..."),
    N_("Log _Out..."),
    NULL, NULL, /* already shows a dialog */
    "system-log-out"
  },
  { ACTION_TYPE_SWITCH_USER,
    N_("Switch User"),
    N_("_Switch User"),
    NULL, NULL, /* not needed */
    "system-users"
  },
  { ACTION_TYPE_LOCK_SCREEN,
    N_("Lock Screen"),
    N_("L_ock Screen"),
    NULL, NULL, /* not needed */
    "system-lock-screen"
  },
  { ACTION_TYPE_HIBERNATE,
    N_("Hibernate"),
    N_("_Hibernate"),
    N_("Do you want to suspend to disk?"),
    N_("Hibernating computer in %d seconds."),
    "system-hibernate"
  },
  { ACTION_TYPE_SUSPEND,
    N_("Suspend"),
    N_("Sus_pend"),
    N_("Do you want to suspend to RAM?"),
    N_("Suspending computer in %d seconds."),
    "system-suspend"
  },
  { ACTION_TYPE_RESTART,
    N_("Restart"),
    N_("_Restart"),
    N_("Are you sure you want to restart?"),
    N_("Restarting computer in %d seconds."),
    "xfsm-reboot"
  },
  { ACTION_TYPE_SHUTDOWN,
    N_("Shut Down"),
    N_("Shut _Down"),
    N_("Are you sure you want to shut down?"),
    N_("Turning off computer in %d seconds."),
    "system-shutdown"
  }
};



/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (ActionsPlugin, actions_plugin)



static GtkIconSize menu_icon_size = GTK_ICON_SIZE_INVALID;
static GQuark      action_quark = 0;



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
                                   PROP_ITEMS,
                                   g_param_spec_boxed ("items",
                                                       NULL, NULL,
                                                       PANEL_PROPERTIES_TYPE_VALUE_ARRAY,
                                                       EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_APPEARANCE,
                                   g_param_spec_uint ("appearance",
                                                      NULL, NULL,
                                                      APPEARANCE_TYPE_BUTTONS,
                                                      APPEARANCE_TYPE_MENU,
                                                      APPEARANCE_TYPE_MENU,
                                                      EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_INVERT_ORIENTATION,
                                   g_param_spec_boolean ("invert-orientation",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_ASK_CONFIRMATION,
                                   g_param_spec_boolean ("ask-confirmation",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));

  menu_icon_size = gtk_icon_size_from_name ("panel-actions-menu");
  if (menu_icon_size == GTK_ICON_SIZE_INVALID)
    menu_icon_size = gtk_icon_size_register ("panel-actions-menu",
                                             DEFAULT_ICON_SIZE,
                                             DEFAULT_ICON_SIZE);

  action_quark = g_quark_from_string ("panel-action-quark");
}



static void
actions_plugin_init (ActionsPlugin *plugin)
{
  plugin->type = APPEARANCE_TYPE_MENU;
  plugin->invert_orientation = FALSE;
  plugin->ask_confirmation = TRUE;
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
    case PROP_ITEMS:
      g_value_set_boxed (value, plugin->items);
      break;

    case PROP_APPEARANCE:
      g_value_set_uint (value, plugin->type);
      break;

    case PROP_INVERT_ORIENTATION:
      g_value_set_boolean (value, plugin->invert_orientation);
      break;

    case PROP_ASK_CONFIRMATION:
      g_value_set_boolean (value, plugin->ask_confirmation);
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

  switch (prop_id)
    {
    case PROP_ITEMS:
      if (plugin->items != NULL)
        xfconf_array_free (plugin->items);
      plugin->items = g_value_dup_boxed (value);
      actions_plugin_pack (plugin);
      break;

    case PROP_APPEARANCE:
      plugin->type = g_value_get_uint (value);
      actions_plugin_pack (plugin);
      break;

    case PROP_INVERT_ORIENTATION:
      plugin->invert_orientation = g_value_get_boolean (value);
      actions_plugin_pack (plugin);
      break;

    case PROP_ASK_CONFIRMATION:
      plugin->ask_confirmation = g_value_get_boolean (value);
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
    { "items", PANEL_PROPERTIES_TYPE_VALUE_ARRAY },
    { "appearance", G_TYPE_UINT },
    { "invert-orientation", G_TYPE_BOOLEAN },
    { "ask-confirmation", G_TYPE_BOOLEAN },
    { NULL }
  };

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* bind all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  actions_plugin_pack (plugin);

  /* set orientation and size */
  actions_plugin_orientation_changed (panel_plugin,
      xfce_panel_plugin_get_orientation (panel_plugin));
}



static void
actions_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);

  if (plugin->pack_idle_id != 0)
    g_source_remove (plugin->pack_idle_id);

  if (plugin->items != NULL)
    xfconf_array_free (plugin->items);

  if (plugin->menu != NULL)
    gtk_widget_destroy (plugin->menu);
}



static void
actions_plugin_size_changed_child (GtkWidget *child,
                                   gpointer   data)
{
  gint size = GPOINTER_TO_INT (data);

  if (!GTK_IS_SEPARATOR (child))
    gtk_widget_set_size_request (child, size, size);
}



static gboolean
actions_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                             gint             size)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  GtkWidget     *box;
  GList         *children, *li;
  gint           n_children;
  gint           child_size;

  if (plugin->type == APPEARANCE_TYPE_BUTTONS)
    {
      box = gtk_bin_get_child (GTK_BIN (plugin));
      if (box != NULL)
        {
          if (plugin->invert_orientation)
            {
              children = gtk_container_get_children (GTK_CONTAINER (box));
              if (G_UNLIKELY (children == NULL))
                return TRUE;
              n_children = g_list_length (children);

              for (li = children; li != NULL; li = li->next)
                {
                  child_size = size / n_children--;
                  size -= child_size;

                  gtk_widget_set_size_request (GTK_WIDGET (li->data),
                                               child_size, child_size);
                }
            }
          else
            {
              gtk_container_foreach (GTK_CONTAINER (box),
                  actions_plugin_size_changed_child,
                  GINT_TO_POINTER (size));
            }
        }
    }

  return TRUE;
}



static gboolean
actions_plugin_configure_store (gpointer data)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (data);
  GtkTreeModel  *model;
  GtkTreeIter    iter;
  GPtrArray     *array;
  gboolean       visible;
  guint          type;
  GValue        *val;

  model = g_object_get_data (G_OBJECT (plugin), "items-store");
  panel_return_val_if_fail (GTK_IS_LIST_STORE (model), FALSE);

  array = g_ptr_array_new ();

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      for (;;)
        {
          gtk_tree_model_get (model, &iter,
                              COLUMN_VISIBLE, &visible,
                              COLUMN_TYPE, &type, -1);

          val = g_new0 (GValue, 1);
          g_value_init (val, G_TYPE_INT);
          g_value_set_int (val, visible ? type : -type);
          g_ptr_array_add (array, val);

          if (!gtk_tree_model_iter_next (model, &iter))
            break;
        }
    }

  /* Store the new array */
  if (plugin->items != NULL)
    xfconf_array_free (plugin->items);
  plugin->items = array;
  g_object_notify (G_OBJECT (plugin), "items");

  return FALSE;
}



static void
actions_plugin_configure_store_idle (ActionsPlugin *plugin)
{
  g_idle_add (actions_plugin_configure_store, plugin);
}



static void
actions_plugin_configure_visible_toggled (GtkCellRendererToggle *renderer,
                                          const gchar           *path_string,
                                          ActionsPlugin         *plugin)
{
  GtkTreeIter   iter;
  gboolean      visible;
  GtkTreeModel *model;

  panel_return_if_fail (XFCE_IS_ACTIONS_PLUGIN (plugin));

  model = g_object_get_data (G_OBJECT (plugin), "items-store");
  panel_return_if_fail (GTK_IS_LIST_STORE (model));
  if (gtk_tree_model_get_iter_from_string (model, &iter, path_string))
    {
      gtk_tree_model_get (model, &iter, COLUMN_VISIBLE, &visible, -1);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_VISIBLE, !visible, -1);

      actions_plugin_configure_store (plugin);
    }
}



static void
actions_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  ActionsPlugin *plugin = XFCE_ACTIONS_PLUGIN (panel_plugin);
  GtkBuilder    *builder;
  GObject       *dialog;
  GObject       *object;
  GObject       *combo;
  ActionEntry   *entry;
  guint          i;
  const GValue  *val;
  gint           type;
  ActionType     real_type;
  guint          n;
  GObject       *store;
  gboolean       found;
  GtkTreeIter    iter;
  gchar         *sep_str;

  panel_return_if_fail (XFCE_IS_ACTIONS_PLUGIN (plugin));
  panel_return_if_fail (plugin->items != NULL);

  /* setup the dialog */
  PANEL_UTILS_LINK_4UI
  builder = panel_utils_builder_new (panel_plugin, actions_dialog_ui,
                                     actions_dialog_ui_length, &dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

  combo = gtk_builder_get_object (builder, "combo-mode");
  exo_mutual_binding_new (G_OBJECT (plugin), "appearance",
                          G_OBJECT (combo), "active");

  object = gtk_builder_get_object (builder, "invert-orientation");
  exo_mutual_binding_new (G_OBJECT (plugin), "invert-orientation",
                          G_OBJECT (object), "active");
  exo_binding_new_with_negation (G_OBJECT (combo), "active",
                                 G_OBJECT (object), "sensitive");

  object = gtk_builder_get_object (builder, "confirmation-dialog");
  exo_mutual_binding_new (G_OBJECT (plugin), "ask-confirmation",
                          G_OBJECT (object), "active");

  store = gtk_builder_get_object (builder, "items-store");
  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  g_object_set_data (G_OBJECT (plugin), "items-store", store);

  object = gtk_builder_get_object (builder, "visible-toggle");
  panel_return_if_fail (GTK_IS_CELL_RENDERER_TOGGLE (object));
  g_signal_connect (G_OBJECT (object), "toggled",
      G_CALLBACK (actions_plugin_configure_visible_toggled), plugin);

  sep_str = g_markup_printf_escaped ("<span color='grey' style='italic'>%s</span>", _("Separator"));

  /* add items from the settings */
  for (i = 0; i < plugin->items->len; i++)
    {
      /* get the value and check if it is within range */
      val = g_ptr_array_index (plugin->items, i);
      type = g_value_get_int (val);
      real_type = ABS (type);
      if (type == 0 || real_type > N_ACTION_TYPES)
        continue;

      /* find the entry in the available actions */
      entry = NULL;
      for (n = 0; n < G_N_ELEMENTS (action_entries); n++)
        {
          if (action_entries[n].type == real_type)
            {
              entry = &action_entries[n];
              break;
            }
        }

      if (entry == NULL)
        {
          if (real_type == ACTION_TYPE_SEPARATOR)
            {
              gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, i,
                                                 COLUMN_VISIBLE, type > 0,
                                                 COLUMN_NAME, sep_str,
                                                 COLUMN_TYPE, ACTION_TYPE_SEPARATOR,
                                                 -1);
            }
        }
      else
        {
          gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, i,
                                             COLUMN_VISIBLE, type > 0,
                                             COLUMN_NAME, _(entry->name),
                                             COLUMN_TYPE, entry->type,
                                             -1);
        }
    }

  g_free (sep_str);

  /* check if there are known actions not in the settings */
  for (i = 0; i < G_N_ELEMENTS (action_entries); i++)
    {
      entry = &action_entries[i];
      found = FALSE;

      for (n = 0; n < plugin->items->len; n++)
        {
          val = g_ptr_array_index (plugin->items, n);
          type = g_value_get_int (val);
          if (ABS (type) == (gint) entry->type)
            {
              found = TRUE;
              break;
            }
        }

      if (!found)
        {
          gtk_list_store_append (GTK_LIST_STORE (store), &iter);
          gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                              COLUMN_VISIBLE, FALSE,
                              COLUMN_NAME, _(entry->name),
                              COLUMN_TYPE, entry->type,
                              -1);
        }
    }

  /* save on dnd changes */
  g_signal_connect_swapped (G_OBJECT (store), "row-inserted",
      G_CALLBACK (actions_plugin_configure_store_idle), plugin);

  gtk_widget_show (GTK_WIDGET (dialog));
}



static void
actions_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                    GtkOrientation   orientation)
{
  actions_plugin_pack (XFCE_ACTIONS_PLUGIN (panel_plugin));
}



static gboolean
actions_plugin_action_confirmation_time (gpointer data)
{
  ActionTimeout *timeout = data;

  panel_return_val_if_fail (timeout->entry != NULL, FALSE);

  if (timeout->time_left == 0)
    {
      /* unattended shutdown, so don't allow apps to cancel shutdown */
      timeout->unattended = TRUE;

      gtk_dialog_response (GTK_DIALOG (timeout->dialog),
                           GTK_RESPONSE_ACCEPT);
    }
  else
    {
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (timeout->dialog),
                                                timeout->entry->status, timeout->time_left);
    }

  return --timeout->time_left >= 0;
}



static gboolean
actions_plugin_action_confirmation (ActionsPlugin *plugin,
                                    ActionEntry   *entry,
                                    gboolean      *unattended)
{
  GtkWidget     *dialog;
  GtkWidget     *button;
  gint           result;
  GtkWidget     *image;
  ActionTimeout *timeout;
  guint          timeout_id;

  panel_return_val_if_fail (entry->question != NULL, FALSE);
  panel_return_val_if_fail (entry->status != NULL, FALSE);

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_CANCEL,
                                   "%s", entry->question);
  gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
  gtk_window_stick (GTK_WINDOW (dialog));
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (dialog), _(entry->name));

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), _(entry->name_mnemonic), GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  image = gtk_image_new_from_icon_name (entry->icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), image);

  timeout = g_slice_new0 (ActionTimeout);
  timeout->entry = entry;
  timeout->time_left = DEFAULT_TIMEOUT;
  timeout->dialog = dialog;
  timeout->unattended = FALSE;

  /* first second looks out of sync with a second timer */
  timeout_id = g_timeout_add (1000, actions_plugin_action_confirmation_time, timeout);
  actions_plugin_action_confirmation_time (timeout);

  result = gtk_dialog_run (GTK_DIALOG (dialog));

  if (unattended != NULL)
    *unattended = timeout->unattended;

  g_source_remove (timeout_id);
  gtk_widget_destroy (dialog);
  g_slice_free (ActionTimeout, timeout);

  return result == GTK_RESPONSE_ACCEPT;
}



static gboolean
actions_plugin_action_activate_dbus (ActionShutdownType   type,
                                     gboolean             allow_save,
                                     GError             **error)
{
  DBusGConnection *connection;
  DBusGProxy      *proxy;
  gboolean         retval;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, error);
  if (connection == NULL)
    return FALSE;

  proxy = dbus_g_proxy_new_for_name (connection,
                                     "org.xfce.SessionManager",
                                     "/org/xfce/SessionManager",
                                     "org.xfce.Session.Manager");
  if (G_LIKELY (proxy != NULL))
    {
      retval = dbus_g_proxy_call (proxy, "Shutdown", error,
                                  G_TYPE_UINT, type,
                                  G_TYPE_BOOLEAN, allow_save,
                                  G_TYPE_INVALID, G_TYPE_INVALID);

      g_object_unref (G_OBJECT (proxy));
    }

  return retval;
}



static void
actions_plugin_action_activate (GtkWidget      *widget,
                                ActionsPlugin  *plugin)
{
  ActionEntry *entry;
  gboolean     unattended = FALSE;
  GError      *error = NULL;
  gboolean     succeed = FALSE;

  entry = g_object_get_qdata (G_OBJECT (widget), action_quark);
  panel_return_if_fail (entry != NULL);

  if (plugin->ask_confirmation
      && entry->question != NULL
      && entry->status != NULL
      && !actions_plugin_action_confirmation (plugin, entry, &unattended))
    return;

  switch (entry->type)
    {
    case ACTION_TYPE_LOGOUT:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_LOGOUT,
                                                     unattended, &error);
      break;

    case ACTION_TYPE_LOGOUT_DIALOG:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_ASK,
                                                     unattended, &error);
      break;

    case ACTION_TYPE_SWITCH_USER:
      succeed = g_spawn_command_line_async ("gdmflexiserver", &error);
      break;

    case ACTION_TYPE_LOCK_SCREEN:
      succeed = g_spawn_command_line_async ("xflock4", &error);
      break;

    case ACTION_TYPE_HIBERNATE:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_HIBERNATE,
                                                     unattended, &error);
      break;

    case ACTION_TYPE_SUSPEND:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_SUSPEND,
                                                     unattended, &error);
      break;

    case ACTION_TYPE_RESTART:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_REBOOT,
                                                     unattended, &error);
      break;

    case ACTION_TYPE_SHUTDOWN:
      succeed = actions_plugin_action_activate_dbus (ACTION_SHUTDOWN_HALT,
                                                     unattended, &error);
      break;

    default:
      panel_assert_not_reached ();
      return;
    }

  if (!succeed)
    {
      xfce_dialog_show_error (NULL, error,
          _("Failed to run action \"%s\""),
          _(entry->name));
    }
}



static GtkWidget *
actions_plugin_action_button (ActionsPlugin  *plugin,
                              guint           type,
                              GtkOrientation  orientation)
{
  GtkWidget   *widget;
  GtkWidget   *image;
  guint        i;
  ActionEntry *entry = NULL;

  if (type == ACTION_TYPE_SEPARATOR)
    {
      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        widget = gtk_vseparator_new ();
      else
        widget = gtk_hseparator_new ();
    }
  else
    {
      /* lookup the action entry */
      for (i = 0; i < G_N_ELEMENTS (action_entries); i++)
        if (action_entries[i].type == type)
           entry = &action_entries[i];
      if (entry == NULL)
        return NULL;

      widget = xfce_panel_create_button ();
      gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
      g_object_set_qdata (G_OBJECT (widget), action_quark, entry);
      gtk_widget_set_tooltip_text (widget, _(entry->name));
      g_signal_connect (G_OBJECT (widget), "clicked",
          G_CALLBACK (actions_plugin_action_activate), plugin);

      image = xfce_panel_image_new_from_source (entry->icon_name);
      gtk_container_add (GTK_CONTAINER (widget), image);
      gtk_widget_show (image);
    }

  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), widget);
  return widget;
}



static GtkWidget *
actions_plugin_action_menu_item (ActionsPlugin *plugin,
                                 guint          type,
                                 gint           size)
{
  GtkWidget   *mi;
  GtkWidget   *image;
  guint        i;
  ActionEntry *entry = NULL;

  if (type == ACTION_TYPE_SEPARATOR)
    return gtk_separator_menu_item_new ();

  /* lookup the action entry */
  for (i = 0; i < G_N_ELEMENTS (action_entries); i++)
    if (action_entries[i].type == type)
       entry = &action_entries[i];
  if (entry == NULL)
    return NULL;

  mi = gtk_image_menu_item_new_with_mnemonic (_(entry->name_mnemonic));
  g_object_set_qdata (G_OBJECT (mi), action_quark, entry);
  g_signal_connect (G_OBJECT (mi), "activate",
      G_CALLBACK (actions_plugin_action_activate), plugin);

  if (size > 0)
    {
      image = xfce_panel_image_new_from_source (entry->icon_name);
      xfce_panel_image_set_size (XFCE_PANEL_IMAGE (image), size);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
      gtk_widget_show (image);
    }

  return mi;
}



static gboolean
actions_plugin_pack_idle (gpointer data)
{
  ActionsPlugin  *plugin = XFCE_ACTIONS_PLUGIN (data);
  GtkWidget      *label;
  GtkWidget      *button;
  GtkWidget      *widget;
  const gchar    *username;
  GtkWidget      *child;
  GtkWidget      *box;
  guint           i;
  const GValue   *val;
  gint            type;
  GtkOrientation  orientation;

  child = gtk_bin_get_child (GTK_BIN (plugin));
  if (child != NULL)
    gtk_widget_destroy (child);

  if (plugin->menu != NULL)
    gtk_widget_destroy (plugin->menu);

  if (plugin->items == NULL)
    plugin->items = actions_plugin_default_array ();

  orientation = xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin));

  if (plugin->type == APPEARANCE_TYPE_BUTTONS)
    {
      if (plugin->invert_orientation)
        orientation = !orientation;
      box = xfce_hvbox_new (orientation, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (plugin), box);
      gtk_widget_show (box);

      for (i = 0; i < plugin->items->len; i++)
        {
          val = g_ptr_array_index (plugin->items, i);
          type = g_value_get_int (val);
          if (type <= 0)
            continue;

          /* skip separators when packing buttons in the opposite
           * orientation */
          if (plugin->invert_orientation
              && type == ACTION_TYPE_SEPARATOR)
            continue;

          widget = actions_plugin_action_button (plugin, type, orientation);
          if (widget != NULL)
            {
              gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);
              gtk_widget_show (widget);
            }
        }

      actions_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
          xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
    }
  else
    {
      /* get a decent username, not the glib defaults */
      username = g_get_real_name ();
      if (exo_str_is_empty (username)
          || strcmp (username, "Unknown") == 0)
        {
          username = g_get_user_name ();
          if (exo_str_is_empty (username)
              || strcmp (username, "somebody") == 0)
            username = _("John Doe");
        }

      button = xfce_arrow_button_new (GTK_ARROW_NONE);
      gtk_widget_set_name (button, "actions-button");
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), button);
      gtk_container_add (GTK_CONTAINER (plugin), button);
      g_signal_connect (G_OBJECT (button), "toggled",
          G_CALLBACK (actions_plugin_menu), plugin);
      gtk_widget_show (button);

      label = gtk_label_new (username);
      gtk_container_add (GTK_CONTAINER (button), label);
      gtk_label_set_angle (GTK_LABEL (label),
          orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 270);
      gtk_widget_show (label);
    }

  return FALSE;
}



static void
actions_plugin_pack_idle_destoyed (gpointer data)
{
  XFCE_ACTIONS_PLUGIN (data)->pack_idle_id = 0;
}



static void
actions_plugin_pack (ActionsPlugin *plugin)
{
  if (plugin->pack_idle_id == 0)
    {
      plugin->pack_idle_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, actions_plugin_pack_idle,
                                              plugin, actions_plugin_pack_idle_destoyed);
    }
}



static GPtrArray *
actions_plugin_default_array (void)
{
  GPtrArray  *array;
  GValue     *val;
  guint       i;
  gint        defaults[] =
    {
      ACTION_TYPE_LOCK_SCREEN,
      ACTION_TYPE_SWITCH_USER,
      ACTION_TYPE_SEPARATOR,
      ACTION_TYPE_SUSPEND,
      -ACTION_TYPE_HIBERNATE, /*hidden */
      -ACTION_TYPE_SEPARATOR, /*hidden */
      ACTION_TYPE_SHUTDOWN,
      -ACTION_TYPE_RESTART,
      ACTION_TYPE_SEPARATOR, /*hidden */
      ACTION_TYPE_LOGOUT
    };

  array = g_ptr_array_sized_new (G_N_ELEMENTS (defaults));
  for (i = 0; i < G_N_ELEMENTS (defaults); i++)
    {
      val = g_new0 (GValue, 1);
      g_value_init (val, G_TYPE_INT);
      g_value_set_int (val, defaults[i]);
      g_ptr_array_add (array, val);
    }

  return array;
}



static void
actions_plugin_menu_deactivate (GtkWidget *menu,
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
actions_plugin_menu (GtkWidget     *button,
                     ActionsPlugin *plugin)
{
  guint         i;
  const GValue *val;
  gint          type;
  GtkWidget    *mi;
  gint          w, h, size;

  panel_return_if_fail (XFCE_IS_ACTIONS_PLUGIN (plugin));

  if (plugin->menu == NULL)
    {
      plugin->menu = gtk_menu_new ();
      g_signal_connect (G_OBJECT (plugin->menu), "selection-done",
          G_CALLBACK (actions_plugin_menu_deactivate), button);
      g_object_add_weak_pointer (G_OBJECT (plugin->menu), (gpointer) &plugin->menu);

      if (gtk_icon_size_lookup (menu_icon_size, &w, &h))
        size = MIN (w, h);

      for (i = 0; i < plugin->items->len; i++)
        {
          val = g_ptr_array_index (plugin->items, i);
          type = g_value_get_int (val);
          if (type <= 0)
            continue;

          mi = actions_plugin_action_menu_item (plugin, type, size);
          if (mi != NULL)
            {
              gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), mi);
              gtk_widget_show (mi);
            }
        }
    }

  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  button != NULL ? xfce_panel_plugin_position_menu : NULL,
                  plugin, 1, gtk_get_current_event_time ());
}
