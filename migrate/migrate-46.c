/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundatoin; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundatoin, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <xfconf/xfconf.h>
#include <libxfce4util/libxfce4util.h>
#include <migrate/migrate-46.h>
#include <libxfce4panel/xfce-panel-macros.h>



#define LAUNCHER_FOLDER "launcher"



typedef enum
{
  START,
  PANELS,
  PANEL,
  PROPERTIES,
  ITEMS,
  UNKNOWN
}
ParserState;

typedef enum
{
  /* no snapping */
  SNAP_POSITION_NONE, /* snapping */

  /* right edge */
  SNAP_POSITION_E,    /* right */
  SNAP_POSITION_NE,   /* top right */
  SNAP_POSITION_EC,   /* right center */
  SNAP_POSITION_SE,   /* bottom right */

  /* left edge */
  SNAP_POSITION_W,    /* left */
  SNAP_POSITION_NW,   /* top left */
  SNAP_POSITION_WC,   /* left center */
  SNAP_POSITION_SW,   /* bottom left */

  /* top and bottom */
  SNAP_POSITION_NC,   /* top center */
  SNAP_POSITION_SC,   /* bottom center */
  SNAP_POSITION_N,    /* top */
  SNAP_POSITION_S,    /* bottom */
}
SnapPosition;

typedef struct
{
  ParserState         state;
  guint               plugin_id_counter;
  guint               panel_id_counter;
  XfconfChannel      *channel;

  GPtrArray          *panel_plugin_ids;
  gint                panel_yoffset;
  gint                panel_xoffset;
  XfceScreenPosition  panel_screen_position;
  guint               panel_transparency;
  gboolean            panel_activetrans;
}
ConfigParser;



static void
migrate_46_panel_screen_position (XfceScreenPosition  screen_position,
                                  SnapPosition       *snap_position,
                                  gboolean           *horizontal)
{
  /* defaults */
  *horizontal = FALSE;
  *snap_position = SNAP_POSITION_NONE;

  switch (screen_position)
    {
    /* top */
    case XFCE_SCREEN_POSITION_NW_H:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_NW;
      break;

    case XFCE_SCREEN_POSITION_N:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_NC;
      break;

    case XFCE_SCREEN_POSITION_NE_H:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_NE;
      break;

    /* left */
    case XFCE_SCREEN_POSITION_NW_V:
      *snap_position = SNAP_POSITION_NW;
      break;

    case XFCE_SCREEN_POSITION_W:
      *snap_position = SNAP_POSITION_WC;
      break;

    case XFCE_SCREEN_POSITION_SW_V:
      *snap_position = SNAP_POSITION_SW;
      break;

    /* right */
    case XFCE_SCREEN_POSITION_NE_V:
      *snap_position = SNAP_POSITION_NE;
      break;

    case XFCE_SCREEN_POSITION_E:
      *snap_position = SNAP_POSITION_EC;
      break;

    case XFCE_SCREEN_POSITION_SE_V:
      *snap_position = SNAP_POSITION_SE;
      break;

    /* bottom */
    case XFCE_SCREEN_POSITION_SW_H:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_SW;
      break;

    case XFCE_SCREEN_POSITION_S:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_SC;
      break;

    case XFCE_SCREEN_POSITION_SE_H:
      *horizontal = TRUE;
      *snap_position = SNAP_POSITION_SE;
      break;

    /* floating */
    case XFCE_SCREEN_POSITION_FLOATING_H:
      *horizontal = TRUE;
      break;

    default:
      break;
    }
}



static void
migrate_46_panel_set_property (ConfigParser  *parser,
                               const gchar   *property_name,
                               const gchar   *value,
                               GError       **error)
{
  gchar       prop[128];
  GdkDisplay *display;
  gchar      *name;
  gint        num;

  if (strcmp (property_name, "size") == 0)
    {
      g_snprintf (prop, sizeof (prop), "/panels/panel-%u/size", parser->panel_id_counter);
      xfconf_channel_set_uint (parser->channel, prop, CLAMP (atoi (value), 16, 128));
    }
  else if (strcmp (property_name, "fullwidth") == 0)
    {
      g_snprintf (prop, sizeof (prop), "/panels/panel-%u/length", parser->panel_id_counter);
      xfconf_channel_set_uint (parser->channel, prop, (atoi (value) != 0) ? 100 : 0);
    }
  else if (strcmp (property_name, "screen-position") == 0)
    {
      parser->panel_screen_position = CLAMP (atoi (value),
                                             XFCE_SCREEN_POSITION_NONE,
                                             XFCE_SCREEN_POSITION_FLOATING_V);
    }
  else if (strcmp (property_name, "xoffset") == 0)
    {
      parser->panel_xoffset = MAX (0, atoi (value));
    }
  else if (strcmp (property_name, "yoffset") == 0)
    {
      parser->panel_yoffset = MAX (0, atoi (value));
    }
  else if (strcmp (property_name, "monitor") == 0)
    {
      /* in 4.4 and 4.6 we only use monitor and make no difference between monitors
       * and screen's, so check the setup of the user to properly convert this */
      num = MAX (0, atoi (value));
      if (G_LIKELY (num > 0))
        {
          display = gdk_display_get_default ();
          if (display != NULL && gdk_display_get_n_screens (display) > 1)
            name = g_strdup_printf ("screen-%d", num);
          else
            name = g_strdup_printf ("monitor-%d", num);

          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/output", parser->panel_id_counter);
          xfconf_channel_set_string (parser->channel, prop, name);
          g_free (name);
        }
    }
  else if (strcmp (property_name, "handlestyle") == 0)
    {
      g_snprintf (prop, sizeof (prop), "/panels/panel-%u/locked", parser->panel_id_counter);
      xfconf_channel_set_bool (parser->channel, prop, atoi (value) == 0);
    }
  else if (strcmp (property_name, "autohide") == 0)
    {
      g_snprintf (prop, sizeof (prop), "/panels/panel-%u/autohide", parser->panel_id_counter);
      xfconf_channel_set_bool (parser->channel, prop, (atoi (value) == 1));
    }
  else if (strcmp (property_name, "transparency") == 0)
    {
      parser->panel_transparency = CLAMP (atoi (value), 0, 100);
    }
  else if (strcmp (property_name, "activetrans") == 0)
    {
      parser->panel_activetrans = (atoi (value) == 1);
    }
  else
    {
      g_set_error (error, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, G_MARKUP_ERROR,
                   "Unknown property \"%s\" in #%d \"panel\" element",
                   property_name, parser->panel_id_counter);
    }
}



#define migrate_46_plugin_string(old_name, new_name, fallback) \
  if (xfce_rc_has_entry (rc, old_name)) \
    xfconf_channel_set_string (channel, "/" new_name, \
        xfce_rc_read_entry (rc, old_name, fallback))

#define migrate_46_plugin_bool(old_name, new_name, fallback) \
  if (xfce_rc_has_entry (rc, old_name)) \
    xfconf_channel_set_bool (channel, "/" new_name, \
        xfce_rc_read_bool_entry (rc, old_name, fallback))

#define migrate_46_plugin_uint(old_name, new_name, fallback) \
  if (xfce_rc_has_entry (rc, old_name)) \
    xfconf_channel_set_uint (channel, "/" new_name, \
        xfce_rc_read_int_entry (rc, old_name, fallback))



static void
migrate_46_plugin_actions (XfconfChannel *channel,
                           XfceRc        *rc)
{
  gint  type;
  guint first_action = 0;
  guint second_action = 0;

  if (!xfce_rc_has_entry (rc, "type"))
    return;

  type = xfce_rc_read_int_entry (rc, "types", 0);
  switch (type)
    {
    case 0: /* ACTION_QUIT */
      first_action = 1;
      break;

    case 1: /* ACTION_LOCK */
      first_action = 2;
      break;

    default: /* ACTION_QUIT_LOCK */
      first_action = 1;
      second_action = 3;
      break;
    }

    xfconf_channel_set_uint (channel, "/first-action", first_action);
    xfconf_channel_set_uint (channel, "/second-action", second_action);
}



static void
migrate_46_plugin_clock (XfconfChannel *channel,
                         XfceRc        *rc)
{
  gint type;

  if (xfce_rc_has_entry (rc, "ClockType"))
    {
      type = xfce_rc_read_int_entry (rc, "ClockType", 0);
      if (type == 4) /* XFCE_CLOCK_LCD */
        type++; /* Skip CLOCK_PLUGIN_MODE_FUZZY */
      xfconf_channel_set_uint (channel, "/mode", type);
    }

  migrate_46_plugin_string ("DigitalFormat", "digital-format", "%R");
  migrate_46_plugin_string ("TooltipFormat", "tooltip-format", "%A %d %B %Y");

  migrate_46_plugin_bool ("ShowFrame", "show-frame", TRUE);
  migrate_46_plugin_bool ("ShowSeconds", "show-seconds", FALSE);
  migrate_46_plugin_bool ("ShowMilitary", "show-military", FALSE);
  migrate_46_plugin_bool ("ShowMeridiem", "show-meridiem", TRUE);
  migrate_46_plugin_bool ("FlashSeparators", "flash-separators", FALSE);
  migrate_46_plugin_bool ("TrueBinary", "true-binary", FALSE);
}



static void
migrate_46_plugin_iconbox (XfconfChannel *channel,
                           XfceRc        *rc)
{
  /* tasklist in iconbox mode */
  xfconf_channel_set_uint (channel, "/show-labels", FALSE);

  migrate_46_plugin_bool ("only_hidden", "show-only-minimized", FALSE);
  migrate_46_plugin_bool ("all_workspaces", "include-all-workspaces", TRUE);

  /* TODO
   * xfce_rc_write_int_entry (rc, "expand", iconbox->expand); */
}



static void
migrate_46_plugin_launcher (XfconfChannel  *channel,
                            XfceRc         *rc,
                            guint           plugin_id,
                            GError        **error)
{
  guint      i;
  gchar      buf[128];
  XfceRc    *new_desktop;
  gchar     *path;
  GTimeVal   timeval;
  GPtrArray *array;
  GValue    *value;
  gchar     *filename;

  if (xfce_rc_has_group (rc, "Global"))
    {
      xfce_rc_set_group (rc, "Global");

      migrate_46_plugin_bool ("MoveFirst", "move-first", FALSE);
      migrate_46_plugin_bool ("ArrowPosition", "arrow-position", 0);
    }

  g_get_current_time (&timeval);
  array = g_ptr_array_new ();

  for (i = 0; i < 100 /* arbitrary */; i++)
    {
      g_snprintf (buf, sizeof (buf), "Entry %d", i);
      if (!xfce_rc_has_group (rc, buf))
        break;

      xfce_rc_set_group (rc, buf);

      g_snprintf (buf, sizeof (buf), "xfce4" G_DIR_SEPARATOR_S "panel"
                  G_DIR_SEPARATOR_S LAUNCHER_FOLDER "-%d" G_DIR_SEPARATOR_S "%ld%d.desktop",
                  plugin_id, timeval.tv_sec, i);
      path = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, buf, TRUE);
      if (G_UNLIKELY (path == NULL))
        {
          g_set_error (error, G_FILE_ERROR_FAILED, G_FILE_ERROR,
                       "Failed to create new launcher desktop file in \"%s\"", buf);
          break;
        }
      else if (g_file_test (path, G_FILE_TEST_EXISTS))
        {
          g_set_error (error, G_FILE_ERROR_EXIST, G_FILE_ERROR,
                       "Desktop item \"%s\" already exists", path);
          g_free (path);
          break;
        }

      new_desktop = xfce_rc_simple_open (path, FALSE);
      if (G_UNLIKELY (new_desktop == NULL))
        {
          g_set_error (error, G_FILE_ERROR_FAILED, G_FILE_ERROR,
                       "Failed to create new desktop file \"%s\"", path);
          g_free (path);
          break;
        }


      xfce_rc_set_group (new_desktop, G_KEY_FILE_DESKTOP_GROUP);

      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_TYPE,
          G_KEY_FILE_DESKTOP_TYPE_APPLICATION);
      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_NAME,
          xfce_rc_read_entry (rc, "Name", ""));
      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_COMMENT,
          xfce_rc_read_entry (rc, "Comment", ""));
      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_ICON,
          xfce_rc_read_entry (rc, "Icon", ""));
      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_EXEC,
          xfce_rc_read_entry (rc, "Exec", ""));
      xfce_rc_write_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_PATH,
          xfce_rc_read_entry (rc, "Path", ""));
      xfce_rc_write_bool_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_TERMINAL,
          xfce_rc_read_bool_entry (rc, "Terminal", FALSE));
      xfce_rc_write_bool_entry (new_desktop, G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY,
          xfce_rc_read_bool_entry (rc, "StartupNotify", FALSE));

      xfce_rc_flush (new_desktop);
      if (xfce_rc_is_dirty (new_desktop))
        {
          g_set_error (error, G_FILE_ERROR_FAILED, G_FILE_ERROR,
                       "Failed to flush desktop file \"%s\"", path);
          g_free (path);
          xfce_rc_close (new_desktop);
          break;
        }

      g_free (path);
      xfce_rc_close (new_desktop);

      value = g_new0 (GValue, 1);
      g_value_init (value, G_TYPE_STRING);
      filename = g_strdup_printf ("%ld%d.desktop", timeval.tv_sec, i);
      g_value_take_string (value, filename);
      g_ptr_array_add (array, value);
    }

  xfconf_channel_set_arrayv (channel, "/items", array);
  xfconf_array_free (array);
}



static void
migrate_46_plugin_pager (XfconfChannel *channel,
                         XfceRc        *rc)
{
  migrate_46_plugin_uint ("rows", "rows", 1);
  migrate_46_plugin_bool ("show-names", "show-names", FALSE);
  migrate_46_plugin_bool ("scrolling", "workspace-scrolling", TRUE);
}



static void
migrate_46_plugin_separator (XfconfChannel *channel,
                             XfceRc        *rc)
{
  gint  type;
  guint style;

  if (!xfce_rc_has_entry (rc, "separator-type"))
    return;

  type = xfce_rc_read_int_entry (rc, "separator-type", 0);
  switch (type)
    {
    case 0: /* SEP_SPACE */
      style = 0; /* SEPARATOR_PLUGIN_STYLE_TRANSPARENT */
      break;

    case 1: /* SEP_EXPAND */
      style = 0; /* SEPARATOR_PLUGIN_STYLE_TRANSPARENT */
      xfconf_channel_set_bool (channel, "/expand", TRUE);
      break;

    case 2: /* SEP_LINE */
      style = 1; /* SEPARATOR_PLUGIN_STYLE_SEPARATOR */
      break;

    case 3: /* SEP_HANDLE */
      style = 2; /* SEPARATOR_PLUGIN_STYLE_HANDLE */
      break;

    default: /* SEP_DOTS */
      style = 3; /* SEPARATOR_PLUGIN_STYLE_DOTS */
      break;
    }

  xfconf_channel_set_uint (channel, "/style", style);
}



static void
migrate_46_plugin_showdesktop (XfconfChannel *channel,
                               XfceRc        *rc)
{
  /* no settings */
}



static void
migrate_46_plugin_systray (XfconfChannel *channel,
                           XfceRc        *rc)
{
  if (xfce_rc_has_group (rc, "Global"))
    {
      xfce_rc_set_group (rc, "Global");

      migrate_46_plugin_bool ("ShowFrame", "show-frame", TRUE);
      migrate_46_plugin_uint ("Rows", "rows", 1);
    }

  if (xfce_rc_has_group (rc, "Applications"))
    {
      xfce_rc_set_group (rc, "Applications");

      /* TODO */
      /* xfce_rc_read_bool_entry (rc, appname, hidden); */
    }
}



static void
migrate_46_plugin_tasklist (XfconfChannel *channel,
                            XfceRc        *rc)
{
  migrate_46_plugin_uint ("grouping", "grouping", 0);
  migrate_46_plugin_bool ("all_workspaces", "include-all-workspaces", TRUE);
  migrate_46_plugin_bool ("flat_buttons", "flat-buttons", FALSE);
  migrate_46_plugin_bool ("show_handles", "show-handle", TRUE);

  /* TODO
   * xfce_rc_write_int_entry (rc, "width", tasklist->width);
   * xfce_rc_write_bool_entry (rc, "fixed_width", tasklist->fixed_width);
   * xfce_rc_write_bool_entry (rc, "expand", tasklist->expand); */
}



static void
migrate_46_plugin_windowlist (XfconfChannel *channel,
                              XfceRc        *rc)
{
  if (xfce_rc_has_entry (rc, "urgency_notify"))
    xfconf_channel_set_bool (channel, "/urgentcy-notification",
      xfce_rc_read_int_entry (rc, "button_layout", 0) > 0);

  migrate_46_plugin_uint ("button_layout", "style", 0);
  migrate_46_plugin_bool ("show_all_workspaces", "all-workspaces", TRUE);
  migrate_46_plugin_bool ("show_workspace_actions", "workspace-actions", FALSE);

  /* TODO
   * xfce_rc_read_bool_entry (rc, "show_window_icons", TRUE); */
}



static void
migrate_46_plugin_xfce4_menu (XfconfChannel *channel,
                              XfceRc        *rc)
{
  migrate_46_plugin_bool ("show_menu_icons", "show-menu-icons", TRUE);
  migrate_46_plugin_bool ("show_button_title", "show-button-title", TRUE);
  migrate_46_plugin_string ("menu_file", "custom-menu-file", "");
  migrate_46_plugin_string ("icon_file", "button-icon", "xfce4-panel-menu");
  migrate_46_plugin_string ("button_title", "button-title", "");

  if (xfce_rc_has_entry (rc, "use_default_menu"))
    xfconf_channel_set_bool (channel, "/custom-menu",
       !xfce_rc_read_bool_entry (rc, "use_default_menu", TRUE));
}



static void
migrate_46_panel_add_plugin (ConfigParser  *parser,
                             const gchar   *name,
                             const gchar   *id,
                             GError       **error)
{
  XfconfChannel *channel;
  gchar          base[256];
  XfceRc        *rc;
  const gchar   *plugin_name = name;

  g_return_if_fail (XFCONF_IS_CHANNEL (parser->channel));

  /* open the old rc file of the plugin */
  g_snprintf (base, sizeof (base), "xfce4" G_DIR_SEPARATOR_S
             "panel" G_DIR_SEPARATOR_S "%s-%s.rc", name, id);
  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, base, TRUE);

  /* open a panel with the propert base for the plugin */
  g_snprintf (base, sizeof (base), "/plugins/plugin-%d", parser->plugin_id_counter);
  channel = xfconf_channel_new_with_property_base (XFCE_PANEL_CHANNEL_NAME, base);

  if (strcmp (name, "actions") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_actions (channel, rc);
    }
  else if (strcmp (name, "clock") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_clock (channel, rc);
    }
  else if (strcmp (name, "iconbox") == 0)
    {
      plugin_name = "tasklist";
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_iconbox (channel, rc);
    }
  else if (strcmp (name, "launcher") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_launcher (channel, rc, parser->plugin_id_counter, error);
    }
  else if (strcmp (name, "pager") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_pager (channel, rc);
    }
  else if (strcmp (name, "separator") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_separator (channel, rc);
    }
  else if (strcmp (name, "showdesktop") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_showdesktop (channel, rc);
    }
  else if (strcmp (name, "systray") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_systray (channel, rc);
    }
  else if (strcmp (name, "tasklist") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_tasklist (channel, rc);
    }
  else if (strcmp (name, "windowlist") == 0)
    {
      plugin_name = "windowmenu";
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_windowlist (channel, rc);
    }
  else if (strcmp (name, "xfce4-menu") == 0)
    {
      plugin_name = "applicationsmenu";
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_xfce4_menu (channel, rc);
    }
  else
    {
      /* handle other "external" plugins */
    }

  /* close plugin configs */
  g_object_unref (G_OBJECT (channel));
  if (G_LIKELY (rc != NULL))
    xfce_rc_close (rc);

  /* store the (new) plugin name */
  xfconf_channel_set_string (parser->channel, base, plugin_name);
}



static void
migrate_46_start_element_handler (GMarkupParseContext  *context,
                                  const gchar          *element_name,
                                  const gchar         **attribute_names,
                                  const gchar         **attribute_values,
                                  gpointer              user_data,
                                  GError              **error)
{
  ConfigParser *parser = user_data;
  guint         i;
  const gchar  *name, *id, *value;
  GValue       *id_value;

  g_return_if_fail (XFCONF_IS_CHANNEL (parser->channel));

  switch (parser->state)
    {
    case START:
      if (strcmp (element_name, "panels") == 0)
        parser->state = PANELS;
      break;

    case PANELS:
      if (strcmp (element_name, "panel") == 0)
        {
          parser->state = PANEL;

          /* intialize new ids array */
          parser->panel_plugin_ids = g_ptr_array_new ();

          /* set defaults */
          parser->panel_screen_position = XFCE_SCREEN_POSITION_NONE;
          parser->panel_xoffset = 100;
          parser->panel_yoffset = 100;
          parser->panel_transparency = 100;
          parser->panel_activetrans = FALSE;
        }
      break;

    case PANEL:
      if (strcmp (element_name, "properties") == 0)
        parser->state = PROPERTIES;
      else if (strcmp (element_name, "items") == 0)
        parser->state = ITEMS;
      break;

    case PROPERTIES:
      if (strcmp (element_name, "property") == 0)
        {
          name = NULL;
          value = NULL;

          for (i = 0; attribute_names[i] != NULL; i++)
            {
              if (strcmp (attribute_names[i], "name") == 0)
                name = attribute_values[i];
              else if (strcmp (attribute_names[i], "value") == 0)
                value = attribute_values[i];
            }

          if (G_LIKELY (name != NULL && value != NULL))
            {
              migrate_46_panel_set_property (parser, name, value, error);
            }
          else
            {
              g_set_error (error, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, G_MARKUP_ERROR,
                          "Unknown property name (%s) or value (%s) in element",
                           name, value);
            }
        }
      break;

    case ITEMS:
      if (strcmp (element_name, "item") == 0)
        {
          name = id = NULL;

          for (i = 0; attribute_names[i] != NULL; i++)
            {
              if (strcmp (attribute_names[i], "name") == 0)
                name = attribute_values[i];
              else if (strcmp (attribute_names[i], "id") == 0)
                id = attribute_values[i];
            }

          if (G_LIKELY (name != NULL && id != NULL))
            {
              parser->plugin_id_counter++;
              migrate_46_panel_add_plugin (parser, name, id, error);

              id_value = g_new0 (GValue, 1);
              g_value_init (id_value, G_TYPE_INT);
              g_value_set_int (id_value, parser->plugin_id_counter);
              g_ptr_array_add (parser->panel_plugin_ids, id_value);
            }
          else
            {
              g_set_error (error, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE, G_MARKUP_ERROR,
                          "Unknown item name (%s) or id (%s) in element",
                           name, id);
            }
        }
      break;

    default:
      g_set_error (error, G_MARKUP_ERROR_UNKNOWN_ELEMENT, G_MARKUP_ERROR,
                   "Unknown start element \"%s\"", element_name);
      break;
    }
}



static void
migrate_46_end_element_handler (GMarkupParseContext  *context,
                                const gchar          *element_name,
                                gpointer              user_data,
                                GError              **error)
{
  ConfigParser *parser = user_data;
  SnapPosition  snap_position;
  gboolean      horizontal;
  gchar         prop[128];
  gchar        *position;

  g_return_if_fail (XFCONF_IS_CHANNEL (parser->channel));

  switch (parser->state)
    {
    case START:
      g_set_error (error, G_MARKUP_ERROR_PARSE, G_MARKUP_ERROR,
                   "Unexpected end element \"%s\"", element_name);
      break;

    case PANEL:
      if (strcmp ("panel", element_name) == 0)
        {
          parser->state = PANELS;

          /* store ids array */
          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/plugin-ids", parser->panel_id_counter);
          xfconf_channel_set_arrayv (parser->channel, prop, parser->panel_plugin_ids);
          xfconf_array_free (parser->panel_plugin_ids);

          /* translate the old screen position to a snap position and orientation */
          migrate_46_panel_screen_position (parser->panel_screen_position,
                                            &snap_position, &horizontal);

          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/horizontal", parser->panel_id_counter);
          xfconf_channel_set_bool (parser->channel, prop, horizontal);

          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/position", parser->panel_id_counter);
          position = g_strdup_printf ("p=%d;x=%d;y=%d",
                                      snap_position,
                                      parser->panel_xoffset,
                                      parser->panel_yoffset);
          xfconf_channel_set_string (parser->channel, prop, position);
          g_free (position);

          /* set transparency */
          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/leave-opacity", parser->panel_id_counter);
          xfconf_channel_set_uint (parser->channel, prop, 100 - parser->panel_transparency);

          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/enter-opacity", parser->panel_id_counter);
          xfconf_channel_set_uint (parser->channel, prop,  parser->panel_activetrans ?
                                   100 - parser->panel_transparency : 100);

          /* prepare for the next panel */
          parser->panel_id_counter++;
        }
      break;

    case PANELS:
      if (strcmp ("panels", element_name) == 0)
        {
          parser->state = START;
          xfconf_channel_set_uint (parser->channel, "/panels", parser->panel_id_counter);
        }
      break;

    case PROPERTIES:
      if (strcmp ("properties", element_name) == 0)
        parser->state = PANEL;
      break;

    case ITEMS:
      if (strcmp ("items", element_name) == 0)
        parser->state = PANEL;
      break;

    default:
      g_set_error (error, G_MARKUP_ERROR_UNKNOWN_ELEMENT, G_MARKUP_ERROR,
                   "Unknown end element \"%s\"", element_name);
      break;
    }
}



static GMarkupParser markup_parser =
{
  migrate_46_start_element_handler,
  migrate_46_end_element_handler,
  NULL,
  NULL,
  NULL
};



gboolean
migrate_46 (const gchar    *filename,
            XfconfChannel  *channel,
            GError        **error)
{
  gsize                length;
  gchar               *contents;
  GMarkupParseContext *context;
  ConfigParser        *parser;
  gboolean             succeed = FALSE;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (XFCONF_IS_CHANNEL (channel), FALSE);

  if (!g_file_get_contents (filename, &contents, &length, error))
    return FALSE;

  parser = g_slice_new0 (ConfigParser);
  parser->state = START;
  parser->plugin_id_counter = 0;
  parser->panel_id_counter = 0;
  parser->channel = channel;

  context = g_markup_parse_context_new (&markup_parser, 0, parser, NULL);

  if (g_markup_parse_context_parse (context, contents, length, error))
    {
      /* check if the entire file is parsed */
      if (g_markup_parse_context_end_parse (context, error))
        succeed = TRUE;
    }

  /* if parsing failed somehow, empty the channel so no broken config is left */
  if (!succeed)
    xfconf_channel_reset_property (parser->channel, "/", TRUE);

  g_free (contents);
  g_markup_parse_context_free (context);
  g_slice_free (ConfigParser, parser);

  return succeed;
}
