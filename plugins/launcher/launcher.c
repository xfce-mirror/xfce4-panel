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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "xfce-test-plugin.h"



static void xfce_test_plugin_class_init (XfceTestPluginClass *klass);
static void xfce_test_plugin_init (XfceTestPlugin *plugin);
static void xfce_test_plugin_construct (XfcePanelPlugin *panel_plugin);
static void xfce_test_plugin_free_data (XfcePanelPlugin *panel_plugin);
static void xfce_test_plugin_orientation_changed (XfcePanelPlugin *panel_plugin, GtkOrientation orientation);
static gboolean xfce_test_plugin_size_changed (XfcePanelPlugin *panel_plugin, gint size);
static void xfce_test_plugin_save (XfcePanelPlugin *panel_plugin);
static void xfce_test_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static void xfce_test_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin, gint position);
static void xfce_test_plugin_icon_theme_changed (GtkIconTheme *icon_theme, XfceTestPlugin *plugin);
static void xfce_test_plugin_update_icon (XfceTestPlugin *plugin);
static void xfce_test_plugin_reorder_buttons (XfceTestPlugin *plugin);
static inline gchar *xfce_test_plugin_read_entry (XfceRc *rc, const gchar *name);
static gboolean xfce_test_plugin_read (XfceTestPlugin *plugin);
static void xfce_test_plugin_button_state_changed (GtkWidget *button_a, GtkStateType state, GtkWidget *button_b);
static gboolean xfce_test_plugin_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, XfceTestPluginEntry *entry);
static gboolean xfce_test_plugin_icon_button_pressed (GtkWidget *button, GdkEventButton *event, XfceTestPlugin *plugin);
static gboolean xfce_test_plugin_icon_button_released (GtkWidget *button, GdkEventButton *event, XfceTestPlugin *plugin);
static void xfce_test_plugin_icon_button_drag_data_received (GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, XfceTestPlugin *plugin);
static gboolean xfce_test_plugin_icon_button_query_tooltip (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, XfceTestPlugin *plugin);
static gboolean xfce_test_plugin_arrow_button_pressed (GtkWidget *button, GdkEventButton *event, XfceTestPlugin *plugin);
static gboolean xfce_test_plugin_menu_popup (gpointer user_data);
static void xfce_test_plugin_menu_popup_destroyed (gpointer user_data);
static void xfce_test_plugin_menu_deactivate (GtkWidget *menu, XfceTestPlugin *plugin);
static void xfce_test_plugin_menu_destroy (XfceTestPlugin *plugin);
static void xfce_test_plugin_menu_build (XfceTestPlugin *plugin);
static gboolean xfce_test_plugin_menu_item_released (GtkMenuItem *menu_item, GdkEventButton *event, XfceTestPluginEntry *entry);
static XfceTestPluginEntry *xfce_test_plugin_entry_new_default (void);
static void xfce_test_plugin_entry_free (XfceTestPluginEntry *entry);



static GQuark xfce_test_plugin_quark = 0;



G_DEFINE_TYPE (XfceTestPlugin, xfce_test_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_TEST_PLUGIN);



static void
xfce_test_plugin_class_init (XfceTestPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = xfce_test_plugin_construct;
  plugin_class->free_data = xfce_test_plugin_free_data;
  plugin_class->orientation_changed = xfce_test_plugin_orientation_changed;
  plugin_class->size_changed = xfce_test_plugin_size_changed;
  plugin_class->save = xfce_test_plugin_save;
  plugin_class->configure_plugin = xfce_test_plugin_configure_plugin;
  plugin_class->screen_position_changed = xfce_test_plugin_screen_position_changed;

  /* initialize the quark */
  xfce_test_plugin_quark = g_quark_from_static_string ("xfce-test-plugin");
}



static void
xfce_test_plugin_init (XfceTestPlugin *plugin)
{
  GdkScreen *screen;
  
  /* initialize variables */
  plugin->entries = NULL;
  plugin->menu = NULL;
  plugin->popup_timeout_id = 0;
  plugin->move_clicked_to_button = FALSE;
  plugin->disable_tooltips = FALSE;
  plugin->menu_reversed_order = FALSE;
  plugin->show_labels = FALSE; /* TODO */
  plugin->arrow_position = ARROW_POS_DEFAULT;

  /* show the configure menu item */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

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
                    G_CALLBACK (xfce_test_plugin_button_state_changed), plugin->arrow_button);
  g_signal_connect (G_OBJECT (plugin->arrow_button), "state-changed",
                    G_CALLBACK (xfce_test_plugin_button_state_changed), plugin->icon_button);

  g_signal_connect (G_OBJECT (plugin->icon_button), "button-press-event",
                    G_CALLBACK (xfce_test_plugin_icon_button_pressed), plugin);
  g_signal_connect (G_OBJECT (plugin->icon_button), "button-release-event",
                    G_CALLBACK (xfce_test_plugin_icon_button_released), plugin);
                    
  gtk_drag_dest_set (plugin->icon_button, 0, drop_targets, /* TODO check flags */
                     G_N_ELEMENTS (drop_targets), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (plugin->icon_button), "drag-data-received",
                    G_CALLBACK (xfce_test_plugin_icon_button_drag_data_received), plugin);

  g_object_set (G_OBJECT (plugin->icon_button), "has-tooltip", TRUE, NULL);
  g_signal_connect (G_OBJECT (plugin->icon_button), "query-tooltip",
                    G_CALLBACK (xfce_test_plugin_icon_button_query_tooltip), plugin);
                    
  /* store the icon theme */
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  plugin->icon_theme = screen ? gtk_icon_theme_get_for_screen (screen) 
                         : gtk_icon_theme_get_default ();
  g_signal_connect (G_OBJECT (plugin->icon_theme), "changed", 
                    G_CALLBACK (xfce_test_plugin_icon_theme_changed), plugin);
}



static void
xfce_test_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  XfceTestPlugin      *plugin = XFCE_TEST_PLUGIN (panel_plugin);
  const gchar * const *filenames;
  guint                i;
  
  /* read the plugin configuration */
  if (xfce_test_plugin_read (plugin) == FALSE)
    {
      /* try to build a launcher from the passed arguments */
      filenames = xfce_panel_plugin_get_arguments (panel_plugin);
      if (filenames)
        {
          /* try to add all the passed filenames */
          for (i = 0; filenames[i] != NULL; i++)
            {
                /* TODO */
                g_message ("%s", filenames[i]);
            }
        }
      
      /* create a new launcher if there are still no entries */
      if (plugin->entries == NULL)
        plugin->entries = g_list_prepend (plugin->entries, xfce_test_plugin_entry_new_default ());
    }
    
  /* set the arrow direction */
  xfce_test_plugin_screen_position_changed (panel_plugin, 0 /* TODO */);
  
  /* set the buttons in the correct position */
  launcher_plugin_pack_buttons (launcher);
  
  /* change the visiblity of the arrow button */
  launcher_menu_destroy (launcher);
}



static void
xfce_test_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  XfceTestPlugin *plugin = XFCE_TEST_PLUGIN (panel_plugin);

  /* stop popup timeout */
  if (G_UNLIKELY (plugin->popup_timeout_id))
    g_source_remove (plugin->popup_timeout_id);

  /* destroy the popup menu */
  if (plugin->menu)
    gtk_widget_destroy (plugin->menu);

  /* remove the entries */
  g_list_foreach (plugin->entries, (GFunc) xfce_test_plugin_entry_free, NULL);
  g_list_free (plugin->entries);
}



static void
xfce_test_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                      GtkOrientation   orientation)
{
  /* update the arrow direction */
  xfce_test_plugin_screen_position_changed (panel_plugin, 0 /* TODO */);
  
  /* reorder the buttons */
  xfce_test_plugin_reorder_buttons (XFCE_TEST_PLUGIN (panel_plugin));

  /* update the plugin size */
  xfce_test_plugin_size_changed (plugin, xfce_panel_plugin_get_size (panel_plugin));
}



static gboolean
xfce_test_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                               gint             size)
{
  XfceTestPlugin *plugin = XFCE_TEST_PLUGIN (panel_plugin);
  gint            width, height;
  GtkOrientation  orientation;
  
  /* init size */
  width = height = size;
  
  if (plugin->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON
      && LIST_HAS_TWO_OR_MORE_ENTRIES (plugin->entries))
    {
      /* get the orientation of the panel */
      orientation = xfce_panel_plugin_get_orientation (panel_plugin);
      
      switch (launcher->arrow_position)
        {
          case LAUNCHER_ARROW_DEFAULT:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width += LAUNCHER_ARROW_SIZE;
            else
              height += LAUNCHER_ARROW_SIZE;
            break;
            
          case LAUNCHER_ARROW_LEFT:
          case LAUNCHER_ARROW_RIGHT:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width += LAUNCHER_ARROW_SIZE;
            else
              height -= LAUNCHER_ARROW_SIZE;
            break;

          default:
            if (orientation == GTK_ORIENTATION_HORIZONTAL)
              width -= LAUNCHER_ARROW_SIZE;
            else
              height += LAUNCHER_ARROW_SIZE;
            break;
        }
    }
    
  /* set the size */
  gtk_widget_set_size_request (GTK_WIDGET (panel_plugin), width, height);

  return TRUE;
}



static void
xfce_test_plugin_save (XfcePanelPlugin *panel_plugin)
{
  gchar                *file;
  gchar               **groups;
  gchar                 group[10];
  XfceRc               *rc;
  GList                *li;
  guint                 i;
  XfceTestPluginEntry  *entry;

  /* get rc file name, create it if needed */
  file = xfce_panel_plugin_save_location (panel_plugin, TRUE);
  if (G_LIKELY (file))
    {
      /* open the config file, writable */
      rc = xfce_rc_simple_open (file, FALSE);
      g_free (file);

      if (G_LIKELY (rc))
        {
          /* delete all the existing groups */
          groups = xfce_rc_get_groups (rc);
          if (G_LIKELY (groups))
            {
              for (i = 0; groups[i] != NULL; i++)
                xfce_rc_delete_group (rc, groups[i], TRUE);
              g_strfreev (groups);
            }

          /* save global launcher settings */
          xfce_rc_set_group (rc, "Global");
          xfce_rc_write_bool_entry (rc, "MoveFirst", plugin->move_clicked_to_button);
          xfce_rc_write_bool_entry (rc, "DisableTooltips", plugin->disable_tooltips);
          xfce_rc_write_bool_entry (rc, "ShowLabels", plugin->show_labels);

          /* save all the entries */
          for (li = launcher->entries, i = 0; li != NULL; li = li->next, i++)
            {
              entry = li->data;

              /* set group */
              g_snprintf (group, sizeof (group), "Entry %d", i);
              xfce_rc_set_group (rc, group);

              /* write entry settings */
              if (G_LIKELY (entry->name))
                xfce_rc_write_entry (rc, "Name", entry->name);
              if (G_LIKELY (entry->comment))
                xfce_rc_write_entry (rc, "Comment", entry->comment);
              if (G_LIKELY (entry->icon))
                xfce_rc_write_entry (rc, "Icon", entry->icon);
              if (G_LIKELY (entry->exec))
                xfce_rc_write_entry (rc, "Exec", entry->exec);
              if (G_LIKELY (entry->path))
                xfce_rc_write_entry (rc, "Path", entry->path);

              xfce_rc_write_bool_entry (rc, "Terminal", entry->terminal);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
              xfce_rc_write_bool_entry (rc, "StartupNotify", entry->startup);
#endif
            }

          /* close the rc file */
          xfce_rc_close (rc);
        }
    }
}



static void
xfce_test_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{

}



static void
xfce_test_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                          gint             position)
{
  GtkArrowType    arrow_type;
  XfceTestPlugin *plugin = XFCE_TEST_PLUGIN (panel_plugin);

  /* get the arrow type */
  arrow_type = xfce_panel_plugin_arrow_type (panel_plugin);

  /* set the arrow direction */
  xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (plugin->arrow_button), arrow_type);
}



static void
xfce_test_plugin_icon_theme_changed (GtkIconTheme   *icon_theme, 
                                     XfceTestPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  panel_return_if_fail (GTK_IS_ICON_THEME (icon_theme));
  
  /* set the new icon theme */
  plugin->icon_theme = icon_theme;
  
  /* update the button icon */
  xfce_test_plugin_update_icon (plugin);
  
  /* destroy the menu */
  xfce_test_plugin_menu_destroy (plugin);
}



static void
xfce_test_plugin_update_icon (XfceTestPlugin *plugin)
{
  XfceTestPluginEntry *entry;
  
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  
  /* get the first entry */
  entry = plugin->entries->data;
  
  if (g_path_is_absolute (entry->icon))
    xfce_scaled_image_set_from_file (XFCE_SCALED_IMAGE (plugin->image), entry->icon);
  else
    xfce_scaled_image_set_from_icon_name (XFCE_SCALED_IMAGE (plugin->image), entry->icon);
}



void
xfce_test_plugin_rebuild (XfceTestPlugin *plugin,
                          gboolean        update_icon)
{
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  
  /* destroy the popup menu */
  xfce_test_plugin_menu_destroy (plugin);
  
  /* reorder the buttons */
  xfce_test_plugin_reorder_buttons (plugin);
  
  /* update the size */
  xfce_test_plugin_size_changed (XFCE_PANEL_PLUGIN (plugin),
                                 xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin));

  /* update the icon if needed */
  if (update_icon)
    xfce_test_plugin_update_icon (plugin);
}



static void
xfce_test_plugin_reorder_buttons (XfceTestPlugin *plugin)
{
  GtkOrientation         orientation;
  XfceTestPluginArrowPos arrow_position = plugin->arrow_position;
  
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));

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
                         (arrow_position == LAUNCHER_ARROW_LEFT 
                          || arrow_position == LAUNCHER_ARROW_TOP) ? 0 : -1);
  
  /* set the hxbox orientation */
  if (arrow_position == LAUNCHER_ARROW_LEFT || arrow_position == LAUNCHER_ARROW_RIGHT)
    orientation = GTK_ORIENTATION_HORIZONTAL;
  else
    orientation = GTK_ORIENTATION_VERTICAL;
  
  xfce_hvbox_set_orientation (XFCE_HVBOX (launcher->box), orientation);
}



static inline gchar *
xfce_test_plugin_read_entry (XfceRc      *rc,
                             const gchar *name)
{
    const gchar *temp;
    gchar       *value = NULL;

    temp = xfce_rc_read_entry (rc, name, NULL);
    if (G_LIKELY (temp != NULL && *temp != '\0'))
        value = g_strdup (temp);

    return value;
}



static gboolean
xfce_test_plugin_read (XfceTestPlugin *plugin)
{
  gchar               *file;
  XfceRc              *rc;
  guint                i;
  gchar                group[10];
  XfceTestPluginEntry *entry;

  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* get rc file name, create it if needed */
  file = xfce_panel_plugin_lookup_rc_file (XFCE_PANEL_PLUGIN (plugin));
  if (G_LIKELY (file))
    {
      /* open config file, read-only, and cleanup */
      rc = xfce_rc_simple_open (file, TRUE);
      g_free (file);

      if (G_LIKELY (rc))
        {
          /* read the global settings */
          xfce_rc_set_group (rc, "Global");
          plugin->move_clicked_to_button = xfce_rc_read_bool_entry (rc, "MoveFirst", FALSE);
          plugin->disable_tooltips = xfce_rc_read_bool_entry (rc, "DisableTooltips", FALSE);
          plugin->show_labels = xfce_rc_read_bool_entry (rc, "ShowLabels", FALSE);

          /* read all the entries */
          for (i = 0; i < 100 /* arbitrary */; i++)
            {
              /* set group, leave if we reached the last entry */
              g_snprintf (group, sizeof (group), "Entry %d", i);
              if (xfce_rc_has_group (rc, group) == FALSE)
                break;
              xfce_rc_set_group (rc, group);

              /* create entry */
              entry = g_slice_new (XfceTestPluginEntry);
              entry->name = launcher_plugin_read_entry (rc, "Name");
              entry->comment = launcher_plugin_read_entry (rc, "Comment");
              entry->icon = launcher_plugin_read_entry (rc, "Icon");
              entry->exec = launcher_plugin_read_entry (rc, "Exec");
              entry->path = launcher_plugin_read_entry (rc, "Path");
              entry->terminal = xfce_rc_read_bool_entry (rc, "Terminal", FALSE);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
              entry->startup = xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE);
#endif

              /* prepend to the list */
              plugin->entries = g_list_prepend (plugin->entries, entry);
            }

          /* close the rc file */
          xfce_rc_close (rc);

          /* reverse the order of the list */
          plugin->entries = g_list_reverse (plugin->entries);
        }
    }

  return (plugin->entries != NULL);
}



GSList *
xfce_test_plugin_filenames_from_selection_data (GtkSelectionData *selection_data)
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
xfce_test_plugin_load_pixbuf (const gchar  *name,
                              gint          size,
                              GtkIconTheme *icon_theme)
{
  GdkPixbuf *pixbuf, *scaled;
  
  panel_return_val_if_fail (size > 0, NULL);
  panel_return_val_if_fail (GTK_IS_ICON_THEME (theme), NULL);
  
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
xfce_test_plugin_query_tooltip (GtkWidget           *widget,
                                gint                 x,
                                gint                 y,
                                gboolean             keyboard_mode,
                                GtkTooltip          *tooltip,
                                XfceTestPluginEntry *entry)
{
  gchar        *string;
  GdkPixbuf    *pixbuf;
  GtkIconTheme *icon_theme;

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
          pixbuf = xfce_test_plugin_load_pixbuf (entry->icon, TOOLTIP_ICON_SIZE, icon_theme);
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
xfce_test_plugin_button_state_changed (GtkWidget    *button_a,
                                       GtkStateType  state,
                                       GtkWidget    *button_b)
{
  /* sync the button states */
  if (GTK_WIDGET_STATE (button_b) != GTK_WIDGET_STATE (button_a)
      && GTK_WIDGET_STATE (button_a) != GTK_STATE_INSENSITIVE)
    gtk_widget_set_state (button_b, GTK_WIDGET_STATE (button_a));
}



static gboolean
xfce_test_plugin_icon_button_pressed (GtkWidget      *button,
                                      GdkEventButton *event,
                                      XfceTestPlugin *plugin)
{
  guint modifiers;

  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* get the default accelerator modifier mask */
  modifiers = event->state & gtk_accelerator_get_default_mod_mask ();

  /* leave when button 1 is not pressed, shift if hold */
  if (event->button != 1 || modifiers == GDK_CONTROL_MASK)
    return FALSE;

  /* popup the menu or start the popup timeout */
  if (plugin->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
    xfce_test_plugin_menu_popup (plugin);
  else if (launcher->popup_timeout_id == 0
           && LIST_HAS_TWO_OR_MORE_ENTRIES (launcher->entries))
    plugin->popup_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT, XFCE_TEST_PLUGIN_POPUP_DELAY,
                                                   xfce_test_plugin_menu_popup, plugin,
                                                   xfce_test_plugin_menu_popup_destroyed);

  return FALSE;
}



static gboolean
xfce_test_plugin_icon_button_released (GtkWidget      *button,
                                       GdkEventButton *event,
                                       XfceTestPlugin *plugin)
{
  XfceTestPluginEntry *entry;
  GdkScreen           *screen;

  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* remove a delayout popup timeout */
  if (G_LIKELY (plugin->popup_timeout_id != 0))
    g_source_remove (plugin->popup_timeout_id);

  /* only accept click in the button and don't respond on multiple clicks */
  if (GTK_BUTTON (button)->in_button
      && launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON)
    {
      /* get the first launcher entry */
      entry = plugin->entries->data;

      /* get the widget screen */
      screen = gtk_widget_get_screen (button);

      /* execute the command on button 1 and 2 */
      if (event->button == 1)
        xfce_test_plugin_execute (screen, entry, NULL);
      else if (event->button == 2)
        xfce_test_plugin_execute_from_clipboard (screen, entry);
    }

  return FALSE;
}



static void
xfce_test_plugin_icon_button_drag_data_received (GtkWidget        *widget,
                                                 GdkDragContext   *context,
                                                 gint              x,
                                                 gint              y,
                                                 GtkSelectionData *selection_data,
                                                 guint             info,
                                                 guint             time,
                                                 XfceTestPlugin   *plugin)
{
  GSList *filenames;
  
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  
  /* leave when arrow is inside the button */
  if (plugin->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
    return;
  
  /* get the filenames from the selection data */
  filenames = xfce_test_plugin_filenames_from_selection_data (selection_data);
  if (G_LIKELY (filenames))
    {
      /* execute */
      launcher_execute (gtk_widget_get_screen (widget), plugin->entries->data, filenames);
    
      /* cleanup */
      xfce_test_plugin_filenames_free (filenames);
    }
  
  /* finish the drag */
  gtk_drag_finish (context, TRUE, FALSE, time);
}



static gboolean
xfce_test_plugin_icon_button_query_tooltip (GtkWidget      *widget,
                                            gint            x,
                                            gint            y,
                                            gboolean        keyboard_mode,
                                            GtkTooltip     *tooltip,
                                            XfceTestPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* don't show tooltips on a menu button or when tooltips are disabled */
  if (plugin->disable_tooltips
      || plugin->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON)
    return FALSE;

  /* run the tooltip query function */
  return xfce_test_plugin_query_tooltip (widget, x, y, keyboard_mode, tooltip, plugin->entries->data);
}



static gboolean
xfce_test_plugin_arrow_button_pressed (GtkWidget      *button,
                                       GdkEventButton *event,
                                       XfceTestPlugin *plugin)
{
  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* only popup when button 1 is pressed */
  if (event->button == 1)
    xfce_test_plugin_menu_popup (plugin);

  return FALSE;
}



static gboolean
xfce_test_plugin_menu_popup (gpointer user_data)
{
  XfceTestPlugin *plugin = XFCE_TEST_PLUGIN (user_data);

  /* build the menu */
  if (G_UNLIKELY (plugin->menu == NULL))
    xfce_test_plugin_menu_build (plugin);

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
xfce_test_plugin_menu_popup_destroyed (gpointer user_data)
{
  XFCE_TEST_PLUGIN (user_data)->popup_timeout_id = 0;
}



static void
xfce_test_plugin_menu_deactivate (GtkWidget      *menu,
                                  XfceTestPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == menu);

  /* deactivate the arrow button */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow_button), FALSE);
}



static void
xfce_test_plugin_menu_destroy (XfceTestPlugin *plugin)
{
  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));

  if (G_LIKELY (plugin->menu != NULL))
    {
      /* destroy the menu and null the variable */
      gtk_widget_destroy (plugin->menu);
      plugin->menu = NULL;

      /* deactivate arrow button */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (plugin->arrow_button), FALSE);
    }

  /* set the visibility of the arrow button */
  if (plugin->arrow_position == LAUNCHER_ARROW_INSIDE_BUTTON
      || LIST_HAS_ONE_ENTRY (plugin->entries))
    gtk_widget_hide (plugin->arrow_button);
  else
    gtk_widget_show (plugin->arrow_button);
}



static void
xfce_test_plugin_menu_build (XfceTestPlugin *plugin)
{
  GList               *li;
  guint                n;
  XfceTestPluginEntry *entry;
  GtkWidget           *mi, *image;
  GdkScreen           *screen;
  GdkPixbuf           *pixbuf;

  panel_return_if_fail (XFCE_IS_TEST_PLUGIN (plugin));
  panel_return_if_fail (plugin->menu == NULL);

  /* create a new menu */
  plugin->menu = gtk_menu_new ();
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
  gtk_menu_set_screen (GTK_MENU (launcher->menu), screen);
  g_signal_connect (G_OBJECT (plugin->menu), "deactivate", G_CALLBACK (xfce_test_plugin_menu_deactivate), plugin);

  /* walk through the entries */
  for (li = plugin->entries, n = 0; li != NULL; li = li->next, n++)
    {
      /* skip the first entry when the arrow is visible */
      if (n == 0 && launcher->arrow_position != LAUNCHER_ARROW_INSIDE_BUTTON)
        continue;

      entry = li->data;

      /* create menu item */
      mi = gtk_image_menu_item_new_with_label (entry->name ? entry->name : _("New Item"));
      g_object_set_qdata (G_OBJECT (mi), xfce_test_plugin_quark, plugin);
      g_object_set (G_OBJECT (mi), "has-tooltip", TRUE, NULL);
      gtk_widget_show (mi);

      /* depending on the menu position we append or prepend */
      if (plugin->menu_reversed_order)
        gtk_menu_shell_append (GTK_MENU_SHELL (plugin->menu), mi);
      else
        gtk_menu_shell_prepend (GTK_MENU_SHELL (plugin->menu), mi);

      /* connect signals */
      g_signal_connect (G_OBJECT (mi), "button-release-event", G_CALLBACK (xfce_test_plugin_menu_item_released), entry);

      if (plugin->disable_tooltips == FALSE)
        g_signal_connect (G_OBJECT (mi), "query-tooltip", G_CALLBACK (xfce_test_plugin_query_tooltip), entry);

      /* try to set an image */
      if (G_LIKELY (entry->icon))
        {
          /* load pixbuf */
          pixbuf = xfce_test_plugin_load_pixbuf (entry->icon, MENU_ICON_SIZE, plugin->icon_theme);
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
xfce_test_plugin_menu_item_released (GtkMenuItem         *menu_item,
                                     GdkEventButton      *event,
                                     XfceTestPluginEntry *entry)
{
  XfceTestPlugin *plugin;
  GdkScreen      *screen;

  /* get the plugin */
  plugin = g_object_get_qdata (G_OBJECT (menu_item), xfce_test_plugin_quark);
  panel_return_val_if_fail (XFCE_IS_TEST_PLUGIN (plugin), FALSE);

  /* get the current screen */
  screen = gtk_widget_get_screen (GTK_WIDGET (plugin));

  if (event->button != 2)
    launcher_execute (screen, entry, NULL);
  else
    launcher_execute_from_clipboard (screen, entry);

  /* move the item to the first position if enabled */
  if (G_UNLIKELY (plugin->move_clicked_to_button))
    {
      /* remove the item to the first place */
      plugin->entries = g_list_remove (plugin->entries, entry);
      plugin->entries = g_list_prepend (plugin->entries, entry);

      /* destroy the menu and update the icon */
      xfce_test_plugin_menu_destroy (plugin);
      xfce_test_plugin_icon_button_set_icon (plugin);
    }
}



static XfceTestPluginEntry *
xfce_test_plugin_entry_new_default (void)
{
  XfceTestPluginEntry *entry;
  
  /* allocate */
  entry = g_slice_new0 (XfceTestPluginEntry);
  entry->name = g_strdup (_("New Item"));
  entry->icon = g_strdup ("applications-other");
  
  /* TODO remove after test */
  if (entry->comment)
    g_critical ("while crearing the default entry, vars were not null");
  
  return entry;
}



static void
xfce_test_plugin_entry_free (XfceTestPluginEntry *entry)
{
  /* free data */
  g_free (entry->name);
  g_free (entry->comment);
  g_free (entry->path);
  g_free (entry->icon);
  g_free (entry->exec);
  
  /* free structure */
  g_slice_free (XfceTestPluginEntry, entry);
}
