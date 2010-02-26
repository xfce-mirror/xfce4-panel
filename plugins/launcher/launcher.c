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

static void launcher_plugin_entries_load (LauncherPlugin *plugin);
static gboolean launcher_plugin_entry_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, LauncherEntry *entry);
static LauncherEntry *launcher_plugin_entry_new_from_filename (const gchar *filename);
static void launcher_plugin_entry_free (LauncherEntry *entry);

static gboolean launcher_plugin_entry_exec_on_screen (LauncherEntry *entry, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_entry_exec (LauncherEntry *entry, guint32 event_time, GdkScreen *screen, GSList *uri_list);
static void launcher_plugin_entry_exec_from_clipboard (LauncherEntry *entry, guint32 event_time, GdkScreen *screen);
static void launcher_plugin_exec_append_quoted (GString *string, const gchar *unquoted);
static gboolean launcher_plugin_exec_parse (LauncherEntry *entry, GSList *uri_list, gboolean terminal, gint *argc, gchar ***argv, GError **error);



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

  GSList *entries;

  guint disable_tooltips : 1;
};

struct _LauncherEntry
{
  gchar *filename;
  gchar *name;
  gchar *comment;
  gchar *exec;
  gchar *path;
  gchar *icon;
  guint terminal : 1;
  guint startup_notify : 1;
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
  else if (exo_str_is_equal (property_name, "/entries"))
    {
      /* free entries */
      g_slist_foreach (plugin->entries, (GFunc)
                       launcher_plugin_entry_free, NULL);
      g_slist_free (plugin->entries);
      plugin->entries = NULL;

      /* load the new entries */
      launcher_plugin_entries_load (plugin);

      /* update the icon */
      launcher_plugin_button_set_icon (plugin);
    }
}



static void
launcher_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  LauncherPlugin *plugin = XFCE_LAUNCHER_PLUGIN (panel_plugin);
  GtkWidget     *widget;

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
  GTK_WIDGET_UNSET_FLAGS (plugin->arrow, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_relief (GTK_BUTTON (plugin->arrow), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (plugin->arrow), FALSE);

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

  /* load the entries */
  launcher_plugin_entries_load (plugin);

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

  /* free entries */
  g_slist_foreach (plugin->entries, (GFunc)
                   launcher_plugin_entry_free, NULL);
  g_slist_free (plugin->entries);
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
  gchar         **filenames;
  guint           i, length;
  GSList          *li;
  LauncherEntry  *entry;

  /* save the global settings */
  xfconf_channel_set_bool (plugin->channel, "/disable-tooltips",
                           plugin->disable_tooltips);

  length = g_slist_length (plugin->entries);
  if (G_LIKELY (length > 0))
    {
      /* create the array with filenames */
      filenames = g_new0 (gchar *, length + 1);
      for (li = plugin->entries, i = 0; li != NULL; li = li->next)
        if (G_LIKELY ((entry = li->data) != NULL))
          filenames[i++] = entry->filename;

      /* store the list of filenames */
      xfconf_channel_set_string_list (plugin->channel, "/entries",
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
  LauncherEntry *entry;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  panel_return_if_fail (XFCE_IS_SCALED_IMAGE (plugin->image));

  if (G_LIKELY (plugin->entries != NULL))
    {
      entry = plugin->entries->data;

      if (IS_STRING (entry->icon))
        {
          if (g_path_is_absolute (entry->icon))
            xfce_scaled_image_set_from_file (XFCE_SCALED_IMAGE (plugin->image),
                                             entry->icon);
          else
            xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image),
                                                  entry->icon);
          /* TODO some more name checking */
        }
    }
  else
    {
      /* set missing image icon */
      xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image),
                                            GTK_STOCK_MISSING_IMAGE);
    }
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
  LauncherEntry *entry;
  GdkScreen     *screen;

  panel_return_val_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin), FALSE);

  if (G_UNLIKELY (plugin->entries == NULL))
    return FALSE;

  entry = plugin->entries->data;
  screen = gtk_widget_get_screen (button);

  if (event->button == 1)
    launcher_plugin_entry_exec (entry, event->time, screen, NULL);
  else if (event->button == 2)
    launcher_plugin_entry_exec_from_clipboard (entry, event->time, screen);
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
  if (plugin->entries == NULL || plugin->entries->data == NULL
      || plugin->disable_tooltips)
    return FALSE;

  return launcher_plugin_entry_query_tooltip (widget, x, y, keyboard_mode,
                                              tooltip, plugin->entries->data);
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
launcher_plugin_entries_load (LauncherPlugin *plugin)
{
  gchar        **filenames;
  guint          i;
  LauncherEntry *entry;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  /* get the list of launcher filenames */
  filenames = xfconf_channel_get_string_list (plugin->channel, "/entries");
  if (G_LIKELY (filenames != NULL))
    {
      /* try to load all the entries */
      for (i = 0; filenames[i] != NULL; i++)
        {
          entry = launcher_plugin_entry_new_from_filename (filenames[i]);
          if (G_LIKELY (entry != NULL))
            plugin->entries = g_slist_append (plugin->entries, entry);
        }

      /* cleanup */
      g_strfreev (filenames);
    }
}



static gboolean
launcher_plugin_entry_query_tooltip (GtkWidget     *widget,
                                     gint           x,
                                     gint           y,
                                     gboolean       keyboard_mode,
                                     GtkTooltip    *tooltip,
                                     LauncherEntry *entry)
{
  gchar *markup;

  panel_return_val_if_fail (entry != NULL, FALSE);

  /* require atleast an entry name */
  if (!IS_STRING (entry->name))
    return FALSE;

  if (IS_STRING (entry->comment))
    {
      markup = g_strdup_printf ("<b>%s</b>\n%s", entry->name, entry->comment);
      gtk_tooltip_set_markup (tooltip, markup);
      g_free (markup);
    }
  else
    {
      gtk_tooltip_set_text (tooltip, entry->name);
    }

  if (G_LIKELY (entry->icon))
    {
      /* TODO pixbuf with cache */
    }

  return TRUE;
}



static LauncherEntry *
launcher_plugin_entry_new_from_filename (const gchar *filename)
{
  LauncherEntry *entry = NULL;
  XfceRc        *rc;

  panel_return_val_if_fail (filename != NULL, NULL);
  panel_return_val_if_fail (g_path_is_absolute (filename), NULL);

  rc = xfce_rc_simple_open (filename, TRUE);
  if (G_LIKELY (rc != NULL))
    {
      /* allocate a new entry */
      entry = g_slice_new0 (LauncherEntry);

      /* set group */
      xfce_rc_set_group (rc, "Desktop Entry");

      /* set entry values */
      entry->filename = g_strdup (filename);
      entry->name = g_strdup (xfce_rc_read_entry (rc, "Name", NULL));
      entry->comment = g_strdup (xfce_rc_read_entry (rc, "Comment", NULL));
      entry->exec = g_strdup (xfce_rc_read_entry_untranslated (rc, "Exec", NULL));
      entry->icon = g_strdup (xfce_rc_read_entry_untranslated (rc, "Icon", NULL));
      entry->path = g_strdup (xfce_rc_read_entry_untranslated (rc, "Path", NULL));
      entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);
      entry->startup_notify = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);

      /* close */
      xfce_rc_close (rc);
    }

  return entry;
}



static void
launcher_plugin_entry_free (LauncherEntry *entry)
{
  panel_return_if_fail (entry != NULL);

  g_free (entry->filename);
  g_free (entry->name);
  g_free (entry->comment);
  g_free (entry->exec);
  g_free (entry->icon);
  g_free (entry->path);

  g_slice_free (LauncherEntry, entry);
}



static gboolean
launcher_plugin_entry_exec_on_screen (LauncherEntry *entry,
                                      guint32        event_time,
                                      GdkScreen     *screen,
                                      GSList        *uri_list)
{
  GError    *error = NULL;
  gchar    **argv;
  gboolean   succeed = FALSE;

  /* parse the execute command */
  if (launcher_plugin_exec_parse (entry, uri_list, entry->terminal,
                                  NULL, &argv, &error))
    {
      /* launch the command on the screen */
      succeed = xfce_execute_argv_on_screen (screen, entry->path, argv,
                                             NULL, G_SPAWN_SEARCH_PATH,
                                             entry->startup_notify,
                                             event_time, entry->icon, &error);

      /* cleanup */
      g_strfreev (argv);
    }

  if (G_UNLIKELY (succeed == FALSE))
    {
      /* TODO */
      g_message ("Failed to launch.... (%s)", error->message);
      g_error_free (error);
    }

  return succeed;
}



static void
launcher_plugin_entry_exec (LauncherEntry *entry,
                            guint32        event_time,
                            GdkScreen     *screen,
                            GSList        *uri_list)
{
  GSList   *li, fake;
  gboolean  proceed = TRUE;

  panel_return_if_fail (entry != NULL);

  /* leave when there is nothing to execute */
  if (!IS_STRING (entry->exec))
    return;

  if (G_UNLIKELY (uri_list != NULL
      && strstr (entry->exec, "%F") == NULL
      && strstr (entry->exec, "%U") == NULL))
    {
      fake.next = NULL;

      /* run an instance for each file, break on the first error */
      for (li = uri_list; li != NULL && proceed; li = li->next)
        {
          fake.data = li->data;
          proceed = launcher_plugin_entry_exec_on_screen (entry,
                                                          event_time,
                                                          screen, &fake);
        }
    }
  else
    {
      launcher_plugin_entry_exec_on_screen (entry, event_time, screen,
                                            uri_list);
    }
}



static void
launcher_plugin_entry_exec_from_clipboard (LauncherEntry *entry,
                                           guint32        event_time,
                                           GdkScreen     *screen)
{
  panel_return_if_fail (entry != NULL);

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
launcher_plugin_exec_parse (LauncherEntry   *entry,
                            GSList          *uri_list,
                            gboolean         terminal,
                            gint            *argc,
                            gchar         ***argv,
                            GError         **error)
{
  GString     *string;
  const gchar *p;
  gboolean     result;
  GSList      *li;
  gchar       *filename;

  panel_return_val_if_fail (entry != NULL, FALSE);
  panel_return_val_if_fail (IS_STRING (entry->exec), FALSE);

  /* allocate an empty string */
  string = g_string_sized_new (100);

  /* prepend terminal command if required */
  if (G_UNLIKELY (terminal))
    g_string_append (string, "exo-open --launch TerminalEmulator ");

  for (p = entry->exec; *p != '\0'; ++p)
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
                      g_string_insert_c (string, -1, ' ');
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
                      g_string_insert_c (string, -1, ' ');
                  }
                break;

              case 'i':
                if (IS_STRING (entry->icon))
                  {
                    g_string_append (string, "--icon ");
                    launcher_plugin_exec_append_quoted (string, entry->icon);
                  }
                break;

              case 'c':
                if (IS_STRING (entry->name))
                  launcher_plugin_exec_append_quoted (string, entry->name);
                break;

              case 'k':
                if (IS_STRING (entry->filename))
                  launcher_plugin_exec_append_quoted (string, entry->filename);
                break;

              case '%':
                g_string_insert_c (string, -1, '%');
                break;
            }
        }
      else
        {
          g_string_insert_c (string, -1, *p);
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
