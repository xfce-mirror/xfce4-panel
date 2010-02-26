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



static void launcher_plugin_construct (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_free_data (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_orientation_changed (XfcePanelPlugin *panel_plugin, GtkOrientation orientation);
static gboolean launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin, gint size);
static void launcher_plugin_save (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin, gint position);

static void launcher_plugin_button_set_icon (LauncherPlugin *plugin);
static void launcher_plugin_button_state_changed (GtkWidget *button_a, GtkStateType state, GtkWidget *button_b);
static gboolean launcher_plugin_button_press_event (GtkWidget *button, GdkEventButton *event, LauncherPlugin *plugin);
static gboolean launcher_plugin_button_release_event (GtkWidget *button, GdkEventButton *event, LauncherPlugin *plugin);
static gboolean launcher_plugin_button_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, LauncherPlugin *plugin);
static void launcher_plugin_button_drag_data_received (GtkWidget *widget,GdkDragContext *context, gint x,gint y,GtkSelectionData *selection_data, guint info, guint drag_time, LauncherPlugin *plugin);

static void launcher_plugin_items_load (LauncherPlugin *plugin);
static gboolean launcher_plugin_item_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, XfceMenuItem *item);

static gboolean launcher_plugin_item_exec_on_screen (XfceMenuItem *item, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_item_exec (XfceMenuItem *item, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_item_exec_from_clipboard (XfceMenuItem *item, guint32 event_time, GdkScreen *screen);
static void launcher_plugin_exec_append_quoted (GString *string, const gchar *unquoted);
static gboolean launcher_plugin_exec_parse (XfceMenuItem *item, GSList *uri_list, gint *argc, gchar ***argv, GError **error);



struct _LauncherPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _LauncherPlugin
{
  XfcePanelPlugin __parent__;

  XfconfChannel *channel;

  GtkWidget *box;
  GtkWidget *button;
  GtkWidget *arrow;
  GtkWidget *image;

  GSList    *items;

  guint      disable_tooltips : 1;
};



G_DEFINE_TYPE (LauncherPlugin, launcher_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_LAUNCHER_PLUGIN);



static void
launcher_plugin_class_init (LauncherPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = launcher_plugin_construct;
  plugin_class->free_data = launcher_plugin_free_data;
  plugin_class->orientation_changed = launcher_plugin_orientation_changed;
  plugin_class->size_changed = launcher_plugin_size_changed;
  plugin_class->save = launcher_plugin_save;
  plugin_class->configure_plugin = launcher_plugin_configure_plugin;
  plugin_class->screen_position_changed = launcher_plugin_screen_position_changed;
}



static void
launcher_plugin_init (LauncherPlugin *plugin)
{
  /* initialize xfconf */
  xfconf_init (NULL);

  /* show the configure menu item */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));
}



static void
launcher_plugin_property_changed (XfconfChannel  *channel,
                                  const gchar    *property_name,
                                  const GValue   *value,
                                  LauncherPlugin *plugin)
{
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->channel == channel);

  if (exo_str_is_equal (property_name, "/disable-tooltips"))
    {
      plugin->disable_tooltips = g_value_get_boolean (value);
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
    }
}



static void
launcher_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin      *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  GtkWidget           *widget;
  const gchar * const *filenames;
  guint                i;
  XfceMenuItem        *item;

  /* open the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);
  g_signal_connect (G_OBJECT (plugin->channel), "property-changed",
                    G_CALLBACK (launcher_plugin_property_changed), plugin);

  /* create the dialog widgets */
  plugin->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->box);
  gtk_widget_show (plugin->box);

  plugin->button = widget = xfce_panel_create_button ();
  gtk_box_pack_start (GTK_BOX (plugin->box), widget, TRUE, TRUE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), widget);
  gtk_widget_set_has_tooltip (widget, TRUE);
  gtk_widget_show (widget);
  g_signal_connect (G_OBJECT (widget), "button-press-event",
                    G_CALLBACK (launcher_plugin_button_press_event),
                    plugin);
  g_signal_connect (G_OBJECT (widget), "button-release-event",
                    G_CALLBACK (launcher_plugin_button_release_event),
                    plugin);
  g_signal_connect (G_OBJECT (widget), "query-tooltip",
                    G_CALLBACK (launcher_plugin_button_query_tooltip),
                    plugin);
  g_signal_connect (G_OBJECT (widget), "drag-data-received",
                    G_CALLBACK (launcher_plugin_button_drag_data_received),
                    plugin);

  plugin->image = xfce_scaled_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->button), plugin->image);
  gtk_widget_show (plugin->image);

  plugin->arrow = xfce_arrow_button_new (GTK_ARROW_UP);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->arrow, FALSE, FALSE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->arrow);
  gtk_button_set_relief (GTK_BUTTON (plugin->arrow), GTK_RELIEF_NONE);

  /* sync button states */
  g_signal_connect (G_OBJECT (plugin->button), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed),
                    plugin->arrow);
  g_signal_connect (G_OBJECT (plugin->arrow), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed),
                    plugin->button);

  /* load global settings */
  plugin->disable_tooltips = xfconf_channel_get_bool (plugin->channel,
                                                      "/disable-tooltips",
                                                      FALSE);

  /* load the items */
  launcher_plugin_items_load (plugin);

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
}



static void
launcher_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);

  /* release the xfconf channel */
  g_object_unref (G_OBJECT (plugin->channel));

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

}



static gboolean
launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint             size)
{
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), size, size);

  return TRUE;
}



static void
launcher_plugin_save (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin  *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  gchar          **filenames;
  guint            i, length;
  GSList          *li;
  XfceMenuItem    *item;

  /* save the global settings */
  xfconf_channel_set_bool (plugin->channel, "/disable-tooltips",
                           plugin->disable_tooltips);

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

}



static void
launcher_plugin_button_set_icon (LauncherPlugin *plugin)
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
launcher_plugin_button_press_event (GtkWidget      *button,
                                    GdkEventButton *event,
                                    LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* do nothing on anything else then a single click */
  if (event->type != GDK_BUTTON_PRESS)
    return FALSE;

  return FALSE;
}



static gboolean
launcher_plugin_button_release_event (GtkWidget      *button,
                                      GdkEventButton *event,
                                      LauncherPlugin *plugin)
{
  XfceMenuItem *item;
  GdkScreen    *screen;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  if (G_UNLIKELY (plugin->items == NULL))
    return FALSE;

  item = XFCE_MENU_ITEM (plugin->items->data);
  screen = gtk_widget_get_screen (button);

  if (event->button == 1)
    launcher_plugin_item_exec (item, event->time, screen, NULL);
  else if (event->button == 2)
    launcher_plugin_item_exec_from_clipboard (item, event->time, screen);
  else
    return TRUE;

  return FALSE;
}



static gboolean
launcher_plugin_button_query_tooltip (GtkWidget      *widget,
                                      gint            x,
                                      gint            y,
                                      gboolean        keyboard_mode,
                                      GtkTooltip     *tooltip,
                                      LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* check if we show tooltips */
  if (plugin->disable_tooltips
      || plugin->items == NULL
      || plugin->items->data == NULL)
    return FALSE;

  return launcher_plugin_item_query_tooltip (widget, x, y, keyboard_mode,
                                             tooltip,
                                             XFCE_MENU_ITEM (plugin->items->data));
}



static void
launcher_plugin_button_drag_data_received (GtkWidget        *widget,
                                           GdkDragContext   *context,
                                           gint              x,
                                           gint              y,
                                           GtkSelectionData *selection_data,
                                           guint             info,
                                           guint             drag_time,
                                           LauncherPlugin   *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  gtk_drag_finish (context, TRUE, FALSE, drag_time);
}



static void
launcher_plugin_items_load (LauncherPlugin *plugin)
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
      /* TODO make this some nice error dialog */
      g_message ("Failed to launch.... (%s)", error->message);
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
          proceed = launcher_plugin_item_exec_on_screen (item,
                                                         event_time,
                                                         screen, &fake);
        }
    }
  else
    {
      launcher_plugin_item_exec_on_screen (item, event_time, screen,
                                           uri_list);
    }
}



static void
launcher_plugin_item_exec_from_clipboard (XfceMenuItem *item,
                                          guint32        event_time,
                                          GdkScreen     *screen)
{
  panel_return_if_fail (XFCE_IS_MENU_ITEM (item));

  /* TODO */
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



XfconfChannel *
launcher_plugin_get_channel (LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), NULL);
  return plugin->channel;
}



GSList *
launcher_plugin_get_items (LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), NULL);
  return plugin->items;
}
