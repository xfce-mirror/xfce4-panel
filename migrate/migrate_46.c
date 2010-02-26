/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
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
#include <migrate/migrate_46.h>


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

typedef enum
{
  XFCE_SCREEN_POSITION_NONE,

  /* top */
  XFCE_SCREEN_POSITION_NW_H,       /* North West Horizontal */
  XFCE_SCREEN_POSITION_N,          /* North                 */
  XFCE_SCREEN_POSITION_NE_H,       /* North East Horizontal */

  /* left */
  XFCE_SCREEN_POSITION_NW_V,       /* North West Vertical   */
  XFCE_SCREEN_POSITION_W,          /* West                  */
  XFCE_SCREEN_POSITION_SW_V,       /* South West Vertical   */

  /* right */
  XFCE_SCREEN_POSITION_NE_V,       /* North East Vertical   */
  XFCE_SCREEN_POSITION_E,          /* East                  */
  XFCE_SCREEN_POSITION_SE_V,       /* South East Vertical   */

  /* bottom */
  XFCE_SCREEN_POSITION_SW_H,       /* South West Horizontal */
  XFCE_SCREEN_POSITION_S,          /* South                 */
  XFCE_SCREEN_POSITION_SE_H,       /* South East Horizontal */

  /* floating */
  XFCE_SCREEN_POSITION_FLOATING_H, /* Floating Horizontal */
  XFCE_SCREEN_POSITION_FLOATING_V  /* Floating Vertical */
}
ScreenPosition;

typedef struct
{
  ParserState     state;
  guint           plugin_id_counter;
  guint           panel_id_counter;
  XfconfChannel  *channel;

  gint            panel_yoffset;
  gint            panel_xoffset;
  ScreenPosition  panel_screen_position;
  guint           panel_transparency;
  gboolean        panel_activetrans;
}
ConfigParser;



static void
migrate_46_panel_screen_position (ScreenPosition  screen_position,
                                  SnapPosition   *snap_position,
                                  gboolean       *horizontal)
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
  gchar prop[128];

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
      parser->panel_xoffset = MAX (0, atoi (value) / 2);
    }
  else if (strcmp (property_name, "yoffset") == 0)
    {
      parser->panel_yoffset = MAX (0, atoi (value) / 2);
    }
  else if (strcmp (property_name, "monitor") == 0)
    {
      /* TODO */
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



static void
migrate_46_plugin_actions (XfconfChannel *channel,
                           XfceRc        *rc)
{
  /* enum: ACTION_QUIT,
   *       ACTION_LOCK,
   *       ACTION_QUIT_LOCK
   *
   * xfce_rc_write_int_entry (rc, "type", action->type);
   * xfce_rc_write_int_entry (rc, "orientation", action->orientation == GTK_ORIENTATION_HORIZONTAL ? 0 : 1);
   */
}



static void
migrate_46_plugin_clock (XfconfChannel *channel,
                         XfceRc        *rc)
{
  /* enum: XFCE_CLOCK_ANALOG,
   *       XFCE_CLOCK_BINARY,
   *       XFCE_CLOCK_DIGITAL,
   *       XFCE_CLOCK_LCD
   *
   * xfce_rc_write_entry (rc, "DigitalFormat", plugin->digital_format);
   * xfce_rc_write_entry (rc, "TooltipFormat", plugin->tooltip_format);
   * xfce_rc_write_int_entry (rc, "ClockType", plugin->mode);
   * xfce_rc_write_bool_entry (rc, "ShowFrame", plugin->show_frame);
   * xfce_rc_write_bool_entry (rc, "ShowSeconds", plugin->show_seconds);
   * xfce_rc_write_bool_entry (rc, "ShowMilitary", plugin->show_military);
   * xfce_rc_write_bool_entry (rc, "ShowMeridiem", plugin->show_meridiem);
   * xfce_rc_write_bool_entry (rc, "TrueBinary", plugin->true_binary);
   * xfce_rc_write_bool_entry (rc, "FlashSeparators", plugin->flash_separators);
   */
}



static void
migrate_46_plugin_iconbox (XfconfChannel *channel,
                           XfceRc        *rc)
{
  /* xfce_rc_write_int_entry (rc, "only_hidden", iconbox->only_hidden);
   * xfce_rc_write_int_entry (rc, "all_workspaces", iconbox->all_workspaces);
   * xfce_rc_write_int_entry (rc, "expand", iconbox->expand);
   */
}



static void
migrate_46_plugin_launcher (XfconfChannel *channel,
                            XfceRc        *rc)
{
  /* xfce_rc_set_group (rc, "Global");
   * xfce_rc_write_bool_entry (rc, "MoveFirst", launcher->move_first);
   * xfce_rc_write_int_entry (rc, "ArrowPosition", launcher->arrow_position);
   *
   * xfce_rc_set_group (rc, Entry %d);
   * xfce_rc_write_entry (rc, "Name", entry->name);
   * xfce_rc_write_entry (rc, "Comment", entry->comment);
   * xfce_rc_write_entry (rc, "Icon", entry->icon);
   * xfce_rc_write_entry (rc, "Exec", entry->exec);
   * xfce_rc_write_entry (rc, "Path", entry->path);
   * xfce_rc_write_bool_entry (rc, "Terminal", entry->terminal);
   * xfce_rc_write_bool_entry (rc, "StartupNotify", entry->startup);
   */
}



static void
migrate_46_plugin_pager (XfconfChannel *channel,
                         XfceRc        *rc)
{
  /* xfce_rc_write_int_entry (rc, "rows", pager->rows);
   * xfce_rc_write_bool_entry (rc, "scrolling", pager->scrolling);
   * xfce_rc_write_bool_entry (rc, "show-names", pager->show_names);
   */
}



static void
migrate_46_plugin_separator (XfconfChannel *channel,
                             XfceRc        *rc)
{
  /* enum: SEP_SPACE,
   *       SEP_EXPAND,
   *       SEP_LINE,
   *       SEP_HANDLE,
   *       SEP_DOTS
   * xfce_rc_write_int_entry (rc, "separator-type", sep->type);
   */
}



static void
migrate_46_plugin_showdesktop (XfconfChannel *channel,
                               XfceRc        *rc)
{
}



static void
migrate_46_plugin_systray (XfconfChannel *channel,
                           XfceRc        *rc)
{
  /* xfce_rc_set_group (rc, "Global");
   * xfce_rc_write_bool_entry (rc, "ShowFrame", plugin->show_frame);
   * xfce_rc_write_int_entry (rc, "Rows", ...);
   *
   * xfce_rc_set_group (rc, "Applications");
   * xfce_rc_write_bool_entry (rc, appname, hidden);
   */
}



static void
migrate_46_plugin_tasklist (XfconfChannel *channel,
                            XfceRc        *rc)
{
  /* xfce_rc_write_int_entry (rc, "grouping", tasklist->grouping);
   * xfce_rc_write_int_entry (rc, "width", tasklist->width);
   * xfce_rc_write_bool_entry (rc, "all_workspaces", tasklist->all_workspaces);
   * xfce_rc_write_bool_entry (rc, "expand", tasklist->expand);
   * xfce_rc_write_bool_entry (rc, "flat_buttons", tasklist->flat_buttons);
   * xfce_rc_write_bool_entry (rc, "show_handles", tasklist->show_handles);
   * xfce_rc_write_bool_entry (rc, "fixed_width", tasklist->fixed_width);
   */
}



static void
migrate_46_plugin_windowlist (XfconfChannel *channel,
                              XfceRc        *rc)
{
  /* enum: ICON_BUTTON
   *       ARROW_BUTTON
   * xfce_rc_write_int_entry (rc, "button_layout", ...);
   *
   * enum: DISABLED
   *       OTHER_WORKSPACES
   *       ALL_WORKSPACES
   * xfce_rc_write_int_entry (rc, "urgency_notify", ...
   *
   * xfce_rc_write_bool_entry (rc, "show_all_workspaces", wl->show_all_workspaces);
   * xfce_rc_write_bool_entry (rc, "show_window_icons", wl->show_window_icons);
   * xfce_rc_write_bool_entry (rc, "show_workspace_actions", wl->show_workspace_actions);
   */
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

  /* open a panel with the propert base for the plugin */
  g_snprintf (base, sizeof (base), "/plugins/plugin-%d", parser->plugin_id_counter);
  channel = xfconf_channel_new_with_property_base ("panel-test", base);

  /* open the old rc file of the plugin */
  g_snprintf (base, sizeof (base), "xfce4" G_DIR_SEPARATOR_S
             "panel" G_DIR_SEPARATOR_S "%s-%s.rc", name, id);
  rc = xfce_rc_config_open (XFCE_RESOURCE_CONFIG, base, TRUE);

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
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_iconbox (channel, rc);
    }
  else if (strcmp (name, "launcher") == 0)
    {
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_launcher (channel, rc);
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
      if (G_LIKELY (rc != NULL))
        migrate_46_plugin_windowlist (channel, rc);
    }
  else
    {
      /* handle other "external" plugins */
    }

  if (G_LIKELY (rc != NULL))
    xfce_rc_close (rc);

  g_object_unref (G_OBJECT (channel));
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
          xfconf_channel_set_uint (parser->channel, prop,  parser->panel_transparency);

          g_snprintf (prop, sizeof (prop), "/panels/panel-%u/enter-opacity", parser->panel_id_counter);
          xfconf_channel_set_uint (parser->channel, prop,  parser->panel_activetrans ?
                                   parser->panel_transparency : 100);

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
  g_return_val_if_fail (XFCONF_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

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

  g_free (contents);
  g_markup_parse_context_free (context);
  g_slice_free (ConfigParser, parser);

  return succeed;
}
