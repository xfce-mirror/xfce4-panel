/* $Id$ */
/*
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
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include "launcher.h"
#include "launcher-dialog.h"

#define MENU_POPUP_DELAY  (225)
#define ARROW_BUTTON_SIZE (16)
#define TOOLTIP_ICON_SIZE (32)
#define MENU_ICON_SIZE    (24)



static void launcher_plugin_class_init (LauncherPluginClass *klass);
static void launcher_plugin_init (LauncherPlugin *plugin);
static void launcher_plugin_construct (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_free_data (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_orientation_changed (XfcePanelPlugin *panel_plugin, GtkOrientation orientation);
static gboolean launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin, gint size);
static void launcher_plugin_save (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void launcher_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin, gint position);
static void launcher_plugin_execute_string_append_quoted (GString *string, const gchar *unquoted);
static gchar *launcher_plugin_execute_parse_command (LauncherPluginEntry *entry, GSList *file_list);
static void launcher_plugin_execute (GdkScreen *screen, LauncherPluginEntry *entry, GSList *file_list);
static void launcher_plugin_execute_from_clipboard (GdkScreen *screen, LauncherPluginEntry *entry);
static void launcher_plugin_icon_theme_changed (GtkIconTheme *icon_theme, LauncherPlugin *plugin);
static void launcher_plugin_update_icon (LauncherPlugin *plugin);
static void launcher_plugin_reorder_buttons (LauncherPlugin *plugin);
static gboolean launcher_plugin_read (LauncherPlugin *plugin);
static void launcher_plugin_button_state_changed (GtkWidget *button_a, GtkStateType state, GtkWidget *button_b);
static gboolean launcher_plugin_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, LauncherPluginEntry *entry);
static gboolean launcher_plugin_icon_button_pressed (GtkWidget *button, GdkEventButton *event, LauncherPlugin *plugin);
static gboolean launcher_plugin_icon_button_released (GtkWidget *button, GdkEventButton *event, LauncherPlugin *plugin);
static void launcher_plugin_icon_button_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, LauncherPlugin *plugin);
static gboolean launcher_plugin_icon_button_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, LauncherPlugin *plugin);
static gboolean launcher_plugin_arrow_button_pressed (GtkWidget *button, GdkEventButton *event, LauncherPlugin *plugin);
static gboolean launcher_plugin_menu_popup (gpointer user_data);
static void launcher_plugin_menu_popup_destroyed (gpointer user_data);
static void launcher_plugin_menu_deactivate (GtkWidget *menu, LauncherPlugin *plugin);
static void launcher_plugin_menu_destroy (LauncherPlugin *plugin);
static void launcher_plugin_menu_build (LauncherPlugin *plugin);
static gboolean launcher_plugin_menu_item_released (GtkMenuItem *menu_item, GdkEventButton *event, LauncherPluginEntry *entry);
static LauncherPluginEntry *launcher_plugin_entry_new_default (void);
static void launcher_plugin_entry_free (LauncherPluginEntry *entry);



static GQuark launcher_plugin_quark = 0;



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

  /* initialize the quark */
  launcher_plugin_quark = g_quark_from_static_string ("xfce-launcher-plugin");
}



static void
launcher_plugin_init (LauncherPlugin *plugin)
{
  GdkScreen *screen;

  /* initialize variables */
  plugin->entries = NULL;
  plugin->menu = NULL;
  plugin->popup_timeout_id = 0;
  plugin->move_first = FALSE;
  plugin->disable_tooltips = FALSE;
  plugin->show_labels = FALSE; /* TODO */
  plugin->arrow_position = ARROW_POS_DEFAULT;

  /* create the dialog widgets */
  plugin->box = xfce_hvbox_new (GTK_ORIENTATION_HORIZONTAL, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->box);
  gtk_widget_show (plugin->box);

  plugin->icon_button = xfce_panel_create_button ();
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->icon_button, TRUE, TRUE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->icon_button);
  gtk_widget_show (plugin->icon_button);

  plugin->image = xfce_scaled_image_new ();
  gtk_container_add (GTK_CONTAINER (plugin->icon_button), plugin->image);
  gtk_widget_show (plugin->image);

  plugin->arrow_button = xfce_arrow_button_new (GTK_ARROW_UP);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->arrow_button, FALSE, FALSE, 0);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->arrow_button);
  GTK_WIDGET_UNSET_FLAGS (plugin->arrow_button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (plugin->arrow_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (plugin->arrow_button), FALSE);

  /* signals */
  g_signal_connect (G_OBJECT (plugin->icon_button), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed), plugin->arrow_button);
  g_signal_connect (G_OBJECT (plugin->arrow_button), "state-changed",
                    G_CALLBACK (launcher_plugin_button_state_changed), plugin->icon_button);

  g_signal_connect (G_OBJECT (plugin->icon_button), "button-press-event",
                    G_CALLBACK (launcher_plugin_icon_button_pressed), plugin);
  g_signal_connect (G_OBJECT (plugin->icon_button), "button-release-event",
                    G_CALLBACK (launcher_plugin_icon_button_released), plugin);

  gtk_drag_dest_set (plugin->icon_button, 0, drop_targets, /* TODO check flags */
                     G_N_ELEMENTS (drop_targets), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (plugin->icon_button), "drag-data-received",
                    G_CALLBACK (launcher_plugin_icon_button_drag_data_received), plugin);

  g_signal_connect (G_OBJECT (plugin->arrow_button), "button-press-event",
                    G_CALLBACK (launcher_plugin_arrow_button_pressed), plugin);

  g_object_set (G_OBJECT (plugin->icon_button), "has-tooltip", TRUE, NULL);
  g_signal_connect (G_OBJECT (plugin->icon_button), "query-tooltip",
                    G_CALLBACK (launcher_plugin_icon_button_query_tooltip), plugin);

  /* store and monitor the icon theme */
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  plugin->icon_theme = screen ? gtk_icon_theme_get_for_screen (screen) : gtk_icon_theme_get_default ();
  g_signal_connect (G_OBJECT (plugin->icon_theme), "changed",
                    G_CALLBACK (launcher_plugin_icon_theme_changed), plugin);
}


static void
launcher_plugin_property_changed (XfconfChannel  *channel,
                                  const gchar    *property_name,
                                  const GValue   *value,
                                  LauncherPlugin *plugin)
{
  guint                nth;
  gchar               *property;
  LauncherPluginEntry *entry;

  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->channel == channel);

  if (strcmp (property_name, "/disable-tooltips") == 0)
    {
      plugin->disable_tooltips = g_value_get_boolean (value);
    }
  else if (strcmp (property_name, "/move-first") == 0)
    {
      plugin->move_first = g_value_get_boolean (value);
    }
  else if (strcmp (property_name, "/show-labels") == 0)
    {
      plugin->show_labels = g_value_get_boolean (value);
    }
  else if (strcmp (property_name, "/arrow-position") == 0)
    {
      plugin->arrow_position = CLAMP (g_value_get_uint (value),
                                      ARROW_POS_DEFAULT, ARROW_POS_INSIDE_BUTTON);
    }
  else if (sscanf (property_name, "/entries/entry-%u/%a[a-z]", &nth, &property) == 2)
    {
      /* lookup the launcher entry */
      entry = g_list_nth_data (plugin->entries, nth);
      if (G_LIKELY (entry))
        {
          if (strcmp (property, "name") == 0)
            {
              g_free (entry->name);
              entry->name = g_value_dup_string (value);

              if (nth > 0 || plugin->show_labels)
                launcher_plugin_rebuild (plugin, FALSE);
            }
          else if (strcmp (property, "comment") == 0)
            {
              g_free (entry->comment);
              entry->comment = g_value_dup_string (value);
            }
          else if (strcmp (property, "icon") == 0)
            {
              g_free (entry->icon);
              entry->icon = g_value_dup_string (value);

              launcher_plugin_rebuild (plugin, (nth == 0));
            }
          else if (strcmp (property, "command") == 0)
            {
              g_free (entry->exec);
              entry->exec = g_value_dup_string (value);
            }
          else if (strcmp (property, "working-directory") == 0)
            {
              g_free (entry->path);
              entry->path = g_value_dup_string (value);
            }
          else if (strcmp (property, "terminal") == 0)
            entry->terminal = g_value_get_boolean (value);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
          else if (strcmp (property, "startup-notify") == 0)
            entry->startup_notify = g_value_get_boolean (value);
#endif
        }

      g_free (property);
    }
}



static void
launcher_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin      *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  const gchar * const *filenames;
  guint                i;

  /* open the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);
  g_signal_connect (G_OBJECT (plugin->channel), "property-changed", G_CALLBACK (launcher_plugin_property_changed), plugin);

  /* show the configure menu item */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* read the plugin configuration */
  if (launcher_plugin_read (plugin) == FALSE)
    {
      /* try to build a launcher from the passed arguments */
      filenames = xfce_panel_plugin_get_arguments (panel_plugin);
      if (filenames)
        {
          /* try to add all the passed filenames */
          for (i = 0; filenames[i] != NULL; i++)
            {
                /* TODO */
                g_message ("TODO %s", filenames[i]);
            }
        }

      /* create a new launcher if there are still no entries */
      if (plugin->entries == NULL)
        plugin->entries = g_list_prepend (plugin->entries, launcher_plugin_entry_new_default ());
    }

  /* set the arrow direction */
  launcher_plugin_screen_position_changed (panel_plugin, 0 /* TODO */);

  /* set the buttons in the correct position */
  launcher_plugin_reorder_buttons (plugin);

  /* change the visiblity of the arrow button */
  launcher_plugin_menu_destroy (plugin);

  /* update the icon */
  launcher_plugin_update_icon (plugin);
}



static void
launcher_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);

  /* release the xfconf channel */
  g_object_unref (G_OBJECT (plugin->channel));

  /* stop popup timeout */
  if (G_UNLIKELY (plugin->popup_timeout_id))
    g_source_remove (plugin->popup_timeout_id);

  /* destroy the popup menu */
  if (plugin->menu)
    gtk_widget_destroy (plugin->menu);

  /* remove the entries */
  g_list_foreach (plugin->entries, (GFunc) launcher_plugin_entry_free, NULL);
  g_list_free (plugin->entries);
}



static void
launcher_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                     GtkOrientation   orientation)
{
  /* update the arrow direction */
  launcher_plugin_screen_position_changed (panel_plugin, 0 /* TODO */);

  /* reorder the buttons */
  launcher_plugin_reorder_buttons (XFCE_LAUNCHER_PLUGIN (panel_plugin));

  /* update the plugin size */
  launcher_plugin_size_changed (panel_plugin, xfce_panel_plugin_get_size (panel_plugin));
}



static gboolean
launcher_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint             size)
{
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  gint            width, height;
  GtkOrientation  orientation;

  /* init size */
  width = height = size;

  if (plugin->arrow_position != ARROW_POS_INSIDE_BUTTON
      && LIST_HAS_TWO_OR_MORE_ENTRIES (plugin->entries))
    {
      /* get the orientation of the panel */
      orientation = xfce_panel_plugin_get_orientation (panel_plugin);

      switch (plugin->arrow_position)
        {
          case ARROW_POS_DEFAULT:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width += ARROW_BUTTON_SIZE;
            else
              height += ARROW_BUTTON_SIZE;
            break;

          case ARROW_POS_LEFT:
          case ARROW_POS_RIGHT:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width += ARROW_BUTTON_SIZE;
            else
              height -= ARROW_BUTTON_SIZE;
            break;

          default:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width -= ARROW_BUTTON_SIZE;
            else
              height += ARROW_BUTTON_SIZE;
            break;
        }
    }

  /* set the size */
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), width, height);

  return TRUE;
}



static void
launcher_plugin_save (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin      *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  gchar                buf[100];
  GList               *li;
  guint                i;
  LauncherPluginEntry *entry;

  /* save the global settings */
  xfconf_channel_set_bool (plugin->channel, "/move-first", plugin->move_first);
  xfconf_channel_set_bool (plugin->channel, "/disable-tooltips", plugin->disable_tooltips);
  xfconf_channel_set_bool (plugin->channel, "/show-labels", plugin->show_labels);
  xfconf_channel_set_uint (plugin->channel, "/arrow-position", plugin->arrow_position);

  for (li = plugin->entries, i = 0; li != NULL; li = li->next, i++)
    {
      entry = li->data;

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/name", i);
      if (entry->name)
        xfconf_channel_set_string (plugin->channel, buf, entry->name);
      else
        xfconf_channel_reset_property (plugin->channel, buf, FALSE);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/comment", i);
      if (entry->comment)
        xfconf_channel_set_string (plugin->channel, buf, entry->comment);
      else
        xfconf_channel_reset_property (plugin->channel, buf, FALSE);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/icon", i);
      if (entry->icon)
        xfconf_channel_set_string (plugin->channel, buf, entry->icon);
      else
        xfconf_channel_reset_property (plugin->channel, buf, FALSE);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/command", i);
      if (entry->exec)
        xfconf_channel_set_string (plugin->channel, buf, entry->exec);
      else
        xfconf_channel_reset_property (plugin->channel, buf, FALSE);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/working-directory", i);
      if (entry->path)
        xfconf_channel_set_string (plugin->channel, buf, entry->path);
      else
        xfconf_channel_reset_property (plugin->channel, buf, FALSE);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/terminal", i);
      xfconf_channel_set_bool (plugin->channel, buf, entry->terminal);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/startup-notify", i);
      xfconf_channel_set_bool (plugin->channel, buf, entry->startup_notify);
#endif
    }

  /* store the number of launchers */
  xfconf_channel_set_uint (plugin->channel, "/entries", i);
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
  GtkArrowType    arrow_type;
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);

  /* get the arrow type */
  arrow_type = xfce_panel_plugin_arrow_type (panel_plugin);

  /* set the arrow direction */
  xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow_button), arrow_type);

  /* destroy the menu to update the popup menu order */
  launcher_plugin_menu_destroy (plugin);
}



static void
launcher_plugin_execute_string_append_quoted (GString     *string,
                                              const gchar *unquoted)
{
  gchar *quoted;

  quoted = g_shell_quote (unquoted);
  g_string_append (string, quoted);
  g_free (quoted);
}



static gchar *
launcher_plugin_execute_parse_command (LauncherPluginEntry *entry,
                                       GSList              *file_list)
{
  GString     *cmd = g_string_sized_new (50);
  const gchar *p;
  gchar       *tmp;
  GSList      *li;

  /* parse the execute command */
  for (p = entry->exec; *p != '\0'; ++p)
    {
      if (p[0] == '%' && p[1] != '\0')
        {
          switch (*++p)
            {
              /* a single filename or url */
              case 'u':
              case 'f':
                if (file_list != NULL)
                  launcher_plugin_execute_string_append_quoted (cmd, (gchar *) file_list->data);
                break;

              /* a list of filenames or urls */
              case 'U':
              case 'F':
                for (li = file_list; li != NULL; li = li->next)
                  {
                    if (G_LIKELY (li != file_list))
                      g_string_append_c (cmd, ' ');

                    launcher_plugin_execute_string_append_quoted (cmd, (gchar *) li->data);
                  }
                break;

              /* directory containing the file that would be passed in a %f field */
              case 'd':
                if (file_list != NULL)
                  {
                    tmp = g_path_get_dirname ((gchar *) file_list->data);
                    if (tmp != NULL)
                      {
                        launcher_plugin_execute_string_append_quoted (cmd, tmp);
                        g_free (tmp);
                      }
                  }
                break;

              /* list of directories containing the files that would be passed in to a %F field */
              case 'D':
                for (li = file_list; li != NULL; li = li->next)
                  {
                    tmp = g_path_get_dirname (li->data);
                    if (tmp != NULL)
                      {
                        if (G_LIKELY (li != file_list))
                          g_string_append_c (cmd, ' ');

                        launcher_plugin_execute_string_append_quoted (cmd, tmp);
                        g_free (tmp);
                      }
                  }
                break;

              /* a single filename (without path) */
              case 'n':
                if (file_list != NULL)
                  {
                    tmp = g_path_get_basename ((gchar *) file_list->data);
                    if (tmp != NULL)
                      {
                        launcher_plugin_execute_string_append_quoted (cmd, tmp);
                        g_free (tmp);
                      }
                  }
                break;

              /* a list of filenames (without paths) */
              case 'N':
                for (li = file_list; li != NULL; li = li->next)
                  {
                    tmp = g_path_get_basename (li->data);
                    if (tmp != NULL)
                      {
                        if (G_LIKELY (li != file_list))
                          g_string_append_c (cmd, ' ');

                        launcher_plugin_execute_string_append_quoted (cmd, tmp);
                        g_free (tmp);
                      }
                  }
                break;

              /* the icon name used in the panel */
              case 'i':
                if (G_LIKELY (entry->icon != NULL))
                  {
                    g_string_append (cmd, "--icon ");
                    launcher_plugin_execute_string_append_quoted (cmd, entry->icon);
                  }
                break;

              /* the 'translated' name of the application */
              case 'c':
                if (G_LIKELY (entry->name != NULL))
                  launcher_plugin_execute_string_append_quoted (cmd, entry->name);
                break;

              /* percentage character */
              case '%':
                g_string_append_c (cmd, '%');
                break;
            }
        }
      else
        {
          g_string_append_c (cmd, *p);
        }
    }

  /* return the command */
  return g_string_free (cmd, FALSE);
}



static void
launcher_plugin_execute (GdkScreen           *screen,
                         LauncherPluginEntry *entry,
                         GSList              *file_list)
{
  GSList   *li, fake;
  gboolean  succeed = TRUE;
  gchar    *command_line;
  gboolean  startup_notify;
  GError   *error = NULL;

  /* set the startup notification boolean */
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  startup_notify = entry->startup_notify;
#else
  startup_notify = FALSE;
#endif

  /* leave when no command has been set */
  if (G_UNLIKELY (entry->exec == NULL || *entry->exec == '\0'))
    return;

  /* make sure we've set a screen */
  if (G_UNLIKELY (screen == NULL))
    screen = gdk_screen_get_default ();

  /* check if we have multiple files to launches */
  if (file_list != NULL && LIST_HAS_TWO_OR_MORE_ENTRIES (file_list)
       && !(strstr (entry->exec, "%F") || strstr (entry->exec, "%U")))
    {
      /* fake an empty list */
      fake.next = NULL;

      /* run a new instance for each file in the list */
      for (li = file_list; li != NULL && succeed; li = li->next)
        {
          /* point to data */
          fake.data = li->data;

          /* parse the command and execute the command */
          command_line = launcher_plugin_execute_parse_command (entry, &fake);
          succeed = xfce_execute_on_screen (screen, command_line, entry->terminal, startup_notify, &error);
          g_free (command_line);
        }
    }
  else
    {
      /* parse the command and execute the command */
      command_line = launcher_plugin_execute_parse_command (entry, file_list);
      succeed = xfce_execute_on_screen (screen, command_line, entry->terminal, startup_notify, &error);
      g_free (command_line);
    }

  if (G_UNLIKELY (succeed == FALSE))
    {
      g_message ("Failed to execute: %s", error->message);
      g_error_free (error);
    }
}



static void
launcher_plugin_execute_from_clipboard (GdkScreen           *screen,
                                        LauncherPluginEntry *entry)
{
  GtkClipboard     *clipboard;
  gchar            *text = NULL;
  GSList           *filenames;
  GtkSelectionData  selection_data;

  /* get the primary clipboard text */
  clipboard = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  if (G_LIKELY (clipboard))
    text = gtk_clipboard_wait_for_text (clipboard);

  /* try other clipboard if this one was empty */
  if (text == NULL)
    {
      /* get the secondary clipboard text */
      clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
      if (G_LIKELY (clipboard))
        text = gtk_clipboard_wait_for_text (clipboard);
    }

  if (G_LIKELY (text))
    {
      /* create some fake selection data */
      selection_data.data = (guchar *) text;
      selection_data.length = 1;

      /* parse the filelist, this way we can handle 'copied' file from thunar */
      filenames = launcher_plugin_filenames_from_selection_data (&selection_data);
      if (G_LIKELY (filenames))
        {
          /* run the command with filenames from the clipboard */
          launcher_plugin_execute (screen, entry, filenames);

          /* cleanup */
          launcher_plugin_filenames_free (filenames);
        }

      /* cleanup */
      g_free (text);
    }
}



static void
launcher_plugin_icon_theme_changed (GtkIconTheme   *icon_theme,
                                    LauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_ICON_THEME (icon_theme));

  /* set the new icon theme */
  plugin->icon_theme = icon_theme;

  /* update the button icon */
  launcher_plugin_update_icon (plugin);

  /* destroy the menu */
  launcher_plugin_menu_destroy (plugin);
}



static void
launcher_plugin_update_icon (LauncherPlugin *plugin)
{
  LauncherPluginEntry *entry;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->entries != NULL);

  /* get the first entry */
  entry = plugin->entries->data;

  if (g_path_is_absolute (entry->icon))
    xfce_scaled_image_set_from_file (XFCE_SCALED_IMAGE (plugin->image), entry->icon);
  else
    xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image), entry->icon);
}



void
launcher_plugin_rebuild (LauncherPlugin *plugin,
                         gboolean        update_icon)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* destroy the popup menu */
  launcher_plugin_menu_destroy (plugin);

  /* reorder the buttons */
  launcher_plugin_reorder_buttons (plugin);

  /* update the size */
  launcher_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
                                xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));

  /* update the icon if needed */
  if (update_icon)
    launcher_plugin_update_icon (plugin);
}



static void
launcher_plugin_reorder_buttons (LauncherPlugin *plugin)
{
  GtkOrientation         orientation;
  LauncherPluginArrowPos arrow_position = plugin->arrow_position;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  if (arrow_position == ARROW_POS_DEFAULT)
    {
      /* get the plugin orientation */
      orientation = xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin));

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        arrow_position = ARROW_POS_RIGHT;
      else
        arrow_position = ARROW_POS_BOTTOM;
    }
  else if (arrow_position == ARROW_POS_INSIDE_BUTTON)
    {
      /* nothing to pack */
      return;
    }

  /* set the position of the arrow button in the box */
  gtk_box_reorder_child (GTK_BOX (plugin->box), plugin->arrow_button,
                         (arrow_position == ARROW_POS_LEFT
                          || arrow_position == ARROW_POS_TOP) ? 0 : -1);

  /* set the hxbox orientation */
  if (arrow_position == ARROW_POS_LEFT || arrow_position == ARROW_POS_RIGHT)
    orientation = GTK_ORIENTATION_HORIZONTAL;
  else
    orientation = GTK_ORIENTATION_VERTICAL;

  xfce_hvbox_set_orientation (XFCE_HVBOX (plugin->box), orientation);
}



static gboolean
launcher_plugin_read (LauncherPlugin *plugin)
{
  guint                i, n_entries;
  gchar                buf[100];
  LauncherPluginEntry *entry;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (XFCONF_IS_CHANNEL (plugin->channel), FALSE);

  /* read global settings */
  plugin->move_first = xfconf_channel_get_bool (plugin->channel, "/move-first", FALSE);
  plugin->disable_tooltips = xfconf_channel_get_bool (plugin->channel, "/disable-tooltips", FALSE);
  plugin->show_labels = xfconf_channel_get_bool (plugin->channel, "/show-labels", FALSE);
  plugin->arrow_position = CLAMP (xfconf_channel_get_uint (plugin->channel, "/arrow-position", ARROW_POS_DEFAULT),
                                  ARROW_POS_DEFAULT, ARROW_POS_INSIDE_BUTTON);

  /* number of launcher entries */
  n_entries = xfconf_channel_get_uint (plugin->channel, "/entries", 0);
  for (i = 0; i < n_entries; i++)
    {
      entry = g_slice_new (LauncherPluginEntry);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/name", i);
      entry->name = xfconf_channel_get_string (plugin->channel, buf, NULL);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/comment", i);
      entry->comment = xfconf_channel_get_string (plugin->channel, buf, NULL);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/icon", i);
      entry->icon = xfconf_channel_get_string (plugin->channel, buf, NULL);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/command", i);
      entry->exec = xfconf_channel_get_string (plugin->channel, buf, NULL);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/working-directory", i);
      entry->path = xfconf_channel_get_string (plugin->channel, buf, NULL);

      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/terminal", i);
      entry->terminal = xfconf_channel_get_bool (plugin->channel, buf, FALSE);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
      g_snprintf (buf, sizeof (buf), "/entries/entry-%d/startup-notify", i);
      entry->startup_notify = xfconf_channel_get_bool (plugin->channel, buf, FALSE);
#endif

      /* prepend the entry */
      plugin->entries = g_list_prepend (plugin->entries, entry);
    }

  /* reverse the order of the list */
  plugin->entries = g_list_reverse (plugin->entries);

  return (plugin->entries != NULL);
}



GSList *
launcher_plugin_filenames_from_selection_data (GtkSelectionData *selection_data)
{
  gchar  **uri_list;
  GSList  *filenames = NULL;
  gchar   *file;
  guint    i;

  /* check whether the retrieval worked */
  if (G_LIKELY (selection_data->length > 0))
    {
      /* split the received uri list */
      uri_list = g_uri_list_extract_uris ((gchar *) selection_data->data);
      if (G_LIKELY (uri_list))
        {
          /* walk though the list */
          for (i = 0; uri_list[i] != NULL; i++)
            {
              /* convert the uri to a filename */
              file = g_filename_from_uri (uri_list[i], NULL, NULL);

              /* prepend the filename */
              if (G_LIKELY (file))
                filenames = g_slist_prepend (filenames, file);
            }

          /* cleanup */
          g_strfreev (uri_list);

          /* reverse the list */
          filenames = g_slist_reverse (filenames);
      }
  }

  return filenames;
}



GdkPixbuf *
launcher_plugin_load_pixbuf (const gchar  *name,
                             gint          size,
                             GtkIconTheme *icon_theme)
{
  GdkPixbuf *pixbuf, *scaled;

  panel_return_val_if_fail (size > 0, NULL);
  panel_return_val_if_fail (GTK_IS_ICON_THEME (icon_theme), NULL);

  /* return null if there is no name */
  if (G_UNLIKELY (name == NULL || *name == '\0'))
    return NULL;

  /* load the icon from a file or the icon theme */
  if (g_path_is_absolute (name))
    {
      pixbuf = exo_gdk_pixbuf_new_from_file_at_max_size (name, size, size, TRUE, NULL);
    }
  else
    {
      pixbuf = gtk_icon_theme_load_icon (icon_theme, name, size, 0, NULL);
      if (G_LIKELY (pixbuf))
        {
          scaled = exo_gdk_pixbuf_scale_down (pixbuf, TRUE, size, size);
          g_object_unref (G_OBJECT (pixbuf));
          pixbuf = scaled;
        }
    }

  return pixbuf;
}



static gboolean
launcher_plugin_query_tooltip (GtkWidget           *widget,
                               gint                 x,
                               gint                 y,
                               gboolean             keyboard_mode,
                               GtkTooltip          *tooltip,
                               LauncherPluginEntry *entry)
{
  gchar        *string;
  GdkPixbuf    *pixbuf;
  GtkIconTheme *icon_theme;
  GdkScreen    *screen;

  if (G_LIKELY (entry && entry->name))
    {
      /* create tooltip label */
      if (entry->comment)
        string = g_strdup_printf ("<b>%s</b>\n%s", entry->name, entry->comment);
      else
        string = g_strdup_printf ("%s", entry->name);

      /* set the markup tooltip and cleanup */
      gtk_tooltip_set_markup (tooltip, string);
      g_free (string);

      if (G_LIKELY (entry->icon))
        {
          /* get the icon theme */
          screen = gtk_widget_get_screen (widget);
          icon_theme = gtk_icon_theme_get_for_screen (screen);

          /* try to load a pixbuf */
          pixbuf = launcher_plugin_load_pixbuf (entry->icon, TOOLTIP_ICON_SIZE, icon_theme);
          if (G_LIKELY (pixbuf))
            {
              /* set the tooltip icon and release it */
              gtk_tooltip_set_icon (tooltip, pixbuf);
              g_object_unref (G_OBJECT (pixbuf));
            }
        }

      /* show the tooltip */
      return TRUE;
    }

  /* nothing to show */
  return FALSE;
}



static void
launcher_plugin_button_state_changed (GtkWidget    *button_a,
                                      GtkStateType  state,
                                      GtkWidget    *button_b)
{
  /* sync the button states */
  if (GTK_WIDGET_STATE (button_b) != GTK_WIDGET_STATE (button_a)
      && GTK_WIDGET_STATE (button_a) != GTK_STATE_INSENSITIVE)
    gtk_widget_set_state (button_b, GTK_WIDGET_STATE (button_a));
}



static gboolean
launcher_plugin_icon_button_pressed (GtkWidget      *button,
                                     GdkEventButton *event,
                                     LauncherPlugin *plugin)
{
  guint modifiers;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* get the default accelerator modifier mask */
  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  /* leave when button 1 is not pressed, shift if hold */
  if (event->button != 1 || modifiers == GDK_CONTROL_MASK)
    return FALSE;

  /* popup the menu or start the popup timeout */
  if (plugin->arrow_position == ARROW_POS_INSIDE_BUTTON)
    launcher_plugin_menu_popup (plugin);
  else if (plugin->popup_timeout_id == 0
           && LIST_HAS_TWO_OR_MORE_ENTRIES (plugin->entries))
    plugin->popup_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT, MENU_POPUP_DELAY,
                                                   launcher_plugin_menu_popup, plugin,
                                                   launcher_plugin_menu_popup_destroyed);

  return FALSE;
}



static gboolean
launcher_plugin_icon_button_released (GtkWidget      *button,
                                      GdkEventButton *event,
                                      LauncherPlugin *plugin)
{
  LauncherPluginEntry *entry;
  GdkScreen           *screen;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (plugin->entries != NULL, FALSE);

  /* remove a delayout popup timeout */
  if (G_LIKELY (plugin->popup_timeout_id != 0))
    g_source_remove (plugin->popup_timeout_id);

  /* only accept click in the button and don't respond on multiple clicks */
  if (GTK_BUTTON (button)->in_button
      && plugin->arrow_position != ARROW_POS_INSIDE_BUTTON)
    {
      /* get the first launcher entry */
      entry = plugin->entries->data;

      /* get the widget screen */
      screen = gtk_widget_get_screen (button);

      /* execute the command on button 1 and 2 */
      if (event->button == 1)
        launcher_plugin_execute (screen, entry, NULL);
      else if (event->button == 2)
        launcher_plugin_execute_from_clipboard (screen, entry);
    }

  return FALSE;
}



static void
launcher_plugin_icon_button_drag_data_received (GtkWidget        *widget,
                                                GdkDragContext   *context,
                                                gint              x,
                                                gint              y,
                                                GtkSelectionData *selection_data,
                                                guint             info,
                                                guint             time,
                                                LauncherPlugin   *plugin)
{
  GSList *filenames;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->entries != NULL);

  /* leave when arrow is inside the button */
  if (plugin->arrow_position == ARROW_POS_INSIDE_BUTTON)
    return;

  /* get the filenames from the selection data */
  filenames = launcher_plugin_filenames_from_selection_data (selection_data);
  if (G_LIKELY (filenames))
    {
      /* execute */
      launcher_plugin_execute (gtk_widget_get_screen (widget), plugin->entries->data, filenames);

      /* cleanup */
      launcher_plugin_filenames_free (filenames);
    }

  /* finish the drag */
  gtk_drag_finish (context, TRUE, FALSE, time);
}



static gboolean
launcher_plugin_icon_button_query_tooltip (GtkWidget      *widget,
                                           gint            x,
                                           gint            y,
                                           gboolean        keyboard_mode,
                                           GtkTooltip     *tooltip,
                                           LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (plugin->entries != NULL, FALSE);

  /* don't show tooltips on a menu button or when tooltips are disabled */
  if (plugin->disable_tooltips
      || plugin->arrow_position == ARROW_POS_INSIDE_BUTTON)
    return FALSE;

  /* run the tooltip query function */
  return launcher_plugin_query_tooltip (widget, x, y, keyboard_mode, tooltip, plugin->entries->data);
}



static gboolean
launcher_plugin_arrow_button_pressed (GtkWidget      *button,
                                      GdkEventButton *event,
                                      LauncherPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* only popup when button 1 is pressed */
  if (event->button == 1)
    launcher_plugin_menu_popup (plugin);

  return FALSE;
}



static gboolean
launcher_plugin_menu_popup (gpointer user_data)
{
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (user_data);

  /* build the menu */
  if (G_UNLIKELY (plugin->menu == NULL))
    launcher_plugin_menu_build (plugin);

  GDK_THREADS_ENTER ();

  /* toggle the arrow button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow_button), TRUE);

  /* popup the menu */
  gtk_menu_popup (GTK_MENU (plugin->menu), NULL, NULL,
                  xfce_panel_plugin_position_menu,
                  XFCE_PANEL_PLUGIN (plugin), 1,
                  gtk_get_current_event_time ());

  GDK_THREADS_LEAVE ();

  return FALSE;
}



static void
launcher_plugin_menu_popup_destroyed (gpointer user_data)
{
  XFCE_LAUNCHER_PLUGIN (user_data)->popup_timeout_id = 0;
}



static void
launcher_plugin_menu_deactivate (GtkWidget      *menu,
                                 LauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == menu);

  /* deactivate the arrow button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow_button), FALSE);
}



static void
launcher_plugin_menu_destroy (LauncherPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  if (G_LIKELY (plugin->menu != NULL))
    {
      /* destroy the menu and null the variable */
      gtk_widget_destroy (plugin->menu);
      plugin->menu = NULL;

      /* deactivate arrow button */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow_button), FALSE);
    }

  /* set the visibility of the arrow button */
  if (plugin->arrow_position == ARROW_POS_INSIDE_BUTTON
      || LIST_HAS_ONE_ENTRY (plugin->entries))
    gtk_widget_hide (plugin->arrow_button);
  else
    gtk_widget_show (plugin->arrow_button);
}



static void
launcher_plugin_menu_build (LauncherPlugin *plugin)
{
  GList               *li;
  guint                n;
  LauncherPluginEntry *entry;
  GtkWidget           *mi, *image;
  GdkScreen           *screen;
  GdkPixbuf           *pixbuf;
  GtkArrowType         arrow_type;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == NULL);

  /* create a new menu */
  plugin->menu = gtk_menu_new ();
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  gtk_menu_set_screen (GTK_MENU (plugin->menu), screen);
  g_signal_connect (G_OBJECT (plugin->menu), "deactivate", G_CALLBACK (launcher_plugin_menu_deactivate), plugin);

  /* get the arrow type from the button for the menu direction */
  arrow_type = xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow_button));

  /* walk through the entries */
  for (li = plugin->entries, n = 0; li != NULL; li = li->next, n++)
    {
      /* skip the first entry when the arrow is visible */
      if (n == 0 && plugin->arrow_position != ARROW_POS_INSIDE_BUTTON)
        continue;

      entry = li->data;

      /* create menu item */
      mi = gtk_image_menu_item_new_with_label (entry->name ? entry->name : _("New Item"));
      g_object_set_qdata (G_OBJECT (mi), launcher_plugin_quark, plugin);
      g_object_set (G_OBJECT (mi), "has-tooltip", TRUE, NULL);
      gtk_widget_show (mi);

      /* depending on the menu position we append or prepend */
      if (arrow_type == GTK_ARROW_DOWN)
        gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), mi);
      else
        gtk_menu_shell_prepend (GTK_MENU_SHELL (plugin->menu), mi);

      /* connect signals */
      g_signal_connect (G_OBJECT (mi), "button-release-event", G_CALLBACK (launcher_plugin_menu_item_released), entry);

      if (plugin->disable_tooltips == FALSE)
        g_signal_connect (G_OBJECT (mi), "query-tooltip", G_CALLBACK (launcher_plugin_query_tooltip), entry);

      /* try to set an image */
      if (G_LIKELY (entry->icon))
        {
          /* load pixbuf */
          pixbuf = launcher_plugin_load_pixbuf (entry->icon, MENU_ICON_SIZE, plugin->icon_theme);
          if (G_LIKELY (pixbuf))
            {
              /* set image */
              image = gtk_image_new_from_pixbuf (pixbuf);
              gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
              gtk_widget_show (image);

              /* release reference */
              g_object_unref (G_OBJECT (pixbuf));
            }
        }
    }
}



static gboolean
launcher_plugin_menu_item_released (GtkMenuItem         *menu_item,
                                    GdkEventButton      *event,
                                    LauncherPluginEntry *entry)
{
  LauncherPlugin *plugin;
  GdkScreen      *screen;

  /* get the plugin */
  plugin = g_object_get_qdata (G_OBJECT (menu_item), launcher_plugin_quark);
  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  /* get the current screen */
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));

  if (event->button != 2)
    launcher_plugin_execute (screen, entry, NULL);
  else
    launcher_plugin_execute_from_clipboard (screen, entry);

  /* move the item to the first position if enabled */
  if (G_UNLIKELY (plugin->move_first))
    {
      /* remove the item to the first place */
      plugin->entries = g_list_remove (plugin->entries, entry);
      plugin->entries = g_list_prepend (plugin->entries, entry);

      /* destroy the menu and update the icon */
      launcher_plugin_menu_destroy (plugin);
      launcher_plugin_update_icon (plugin);
    }

  return FALSE;
}



static LauncherPluginEntry *
launcher_plugin_entry_new_default (void)
{
  LauncherPluginEntry *entry;

  /* allocate */
  entry = g_slice_new0 (LauncherPluginEntry);
  entry->name = g_strdup (_("New Item"));
  entry->icon = g_strdup ("applications-other");

  return entry;
}



static void
launcher_plugin_entry_free (LauncherPluginEntry *entry)
{
  /* free data */
  g_free (entry->name);
  g_free (entry->comment);
  g_free (entry->path);
  g_free (entry->icon);
  g_free (entry->exec);

  /* free structure */
  g_slice_free (LauncherPluginEntry, entry);
}
