/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <exo/exo.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4panel/xfce-panel-plugin-provider.h>

#include <panel/panel-private.h>
#include <panel/panel-window.h>
#include <panel/panel-application.h>
#include <panel/panel-itembar.h>
#include <panel/panel-module-factory.h>
#include <panel/panel-preferences-dialog.h>
#include <panel/panel-item-dialog.h>
#include <panel/panel-dialogs.h>
#include <panel/panel-glue.h>
#include <panel/panel-plugin-external.h>

#define PANEL_CONFIG_PATH "xfce4" G_DIR_SEPARATOR_S "panel" G_DIR_SEPARATOR_S "panels.new.xml"
#define AUTOSAVE_INTERVAL (10 * 60)



static void      panel_application_class_init         (PanelApplicationClass  *klass);
static void      panel_application_init               (PanelApplication       *application);
static void      panel_application_finalize           (GObject                *object);
static void      panel_application_load               (PanelApplication       *application);
static void      panel_application_load_set_property  (PanelWindow            *window,
                                                       const gchar            *name,
                                                       const gchar            *value);
static void      panel_application_load_start_element (GMarkupParseContext    *context,
                                                       const gchar            *element_name,
                                                       const gchar           **attribute_names,
                                                       const gchar           **attribute_values,
                                                       gpointer                user_data,
                                                       GError                **error);
static void      panel_application_load_end_element   (GMarkupParseContext    *context,
                                                       const gchar            *element_name,
                                                       gpointer                user_data,
                                                       GError                **error);
static void      panel_application_plugin_move        (GtkWidget              *item,
                                                       PanelApplication       *application);
static gboolean  panel_application_plugin_insert      (PanelApplication       *application,
                                                       PanelWindow            *window,
                                                       GdkScreen              *screen,
                                                       const gchar            *name,
                                                       const gchar            *id,
                                                       gchar                 **arguments,
                                                       gint                    position);
static gboolean  panel_application_save_timeout       (gpointer                user_data);
static gchar    *panel_application_save_xml_contents  (PanelApplication       *application);
static void      panel_application_window_destroyed   (GtkWidget              *window,
                                                       PanelApplication       *application);
static void      panel_application_dialog_destroyed   (GtkWindow              *dialog,
                                                       PanelApplication       *application);
static void      panel_application_drag_data_received (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       GtkSelectionData       *selection_data,
                                                       guint                   info,
                                                       guint                   time,
                                                       PanelWindow            *window);
static gboolean  panel_application_drag_drop          (GtkWidget              *itembar,
                                                       GdkDragContext         *context,
                                                       gint                    x,
                                                       gint                    y,
                                                       guint                   time,
                                                       PanelWindow            *window);



struct _PanelApplicationClass
{
  GObjectClass __parent__;
};

struct _PanelApplication
{
  GObject  __parent__;

  /* the plugin factory */
  PanelModuleFactory *factory;

  /* internal list of all the panel windows */
  GSList  *windows;

  /* internal list of opened dialogs */
  GSList  *dialogs;

  /* autosave timeout */
  guint    autosave_timeout_id;
};

typedef enum
{
  PARSER_START,
  PARSER_PANELS,
  PARSER_PANEL,
  PARSER_PROPERTIES,
  PARSER_ITEMS
}
ParserState;

typedef struct
{
  ParserState       state;
  PanelApplication *application;
  PanelWindow      *window;
}
Parser;

static GMarkupParser markup_parser =
{
  panel_application_load_start_element,
  panel_application_load_end_element,
  NULL,
  NULL,
  NULL
};

static const GtkTargetEntry drag_targets[] =
{
    { "application/x-xfce-panel-plugin-widget", 0, 0 }
};



G_DEFINE_TYPE (PanelApplication, panel_application, G_TYPE_OBJECT);



static void
panel_application_class_init (PanelApplicationClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = panel_application_finalize;
}



static void
panel_application_init (PanelApplication *application)
{
  /* initialize */
  application->windows = NULL;
  application->dialogs = NULL;

  /* get a factory reference so it never unloads */
  application->factory = panel_module_factory_get ();

  /* load setup */
  panel_application_load (application);

  /* start the autosave timeout */
#if GLIB_CHECK_VERSION (2, 14, 0)
  application->autosave_timeout_id = g_timeout_add_seconds (AUTOSAVE_INTERVAL, panel_application_save_timeout, application);
#else
  application->autosave_timeout_id = g_timeout_add (AUTOSAVE_INTERVAL * 1000, panel_application_save_timeout, application);
#endif
}



static void
panel_application_finalize (GObject *object)
{
  PanelApplication *application = PANEL_APPLICATION (object);
  GSList           *li;

  panel_return_if_fail (application->dialogs == NULL);

  /* stop the autosave timeout */
  g_source_remove (application->autosave_timeout_id);

  /* destroy the windows if they are still opened */
  for (li = application->windows; li != NULL; li = li->next)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (li->data), G_CALLBACK (panel_application_window_destroyed), application);
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }

  /* cleanup the list of windows */
  g_slist_free (application->windows);

  /* release the factory */
  g_object_unref (G_OBJECT (application->factory));

  (*G_OBJECT_CLASS (panel_application_parent_class)->finalize) (object);
}



static void
panel_application_load (PanelApplication *application)
{
  gchar               *filename;
  gchar               *contents;
  gboolean             succeed = FALSE;
  gsize                length;
  GError              *error = NULL;
  GMarkupParseContext *context;
  Parser               parser;
  PanelWindow         *window;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  if (G_LIKELY (TRUE))
    {
      /* get filename from user config */
      filename = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, PANEL_CONFIG_PATH);
    }
  else
    {
      /* get config from xdg directory (kiosk mode) */
      filename = g_build_filename (SYSCONFDIR, PANEL_CONFIG_PATH, NULL);
    }

  /* test config file */
  if (G_LIKELY (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR)))
    {
      /* load the file contents */
      succeed = g_file_get_contents (filename, &contents, &length, &error);
      if (G_LIKELY (succeed))
        {
          /* initialize the parser */
          parser.state = PARSER_START;
          parser.application = application;
          parser.window = NULL;

          /* create parse context */
          context = g_markup_parse_context_new (&markup_parser, 0, &parser, NULL);

          /* parse the content */
          succeed = g_markup_parse_context_parse (context, contents, length, &error);
          if (G_UNLIKELY (succeed == FALSE))
            goto done;

          /* finish parsing */
          succeed = g_markup_parse_context_end_parse (context, &error);

          /* goto label */
          done:

          /* cleanup */
          g_markup_parse_context_free (context);
          g_free (contents);

          /* show error */
          if (G_UNLIKELY (succeed == FALSE))
            {
              /* print warning */
              g_critical ("Failed to parse configuration from \"%s\": %s", filename, error->message);

              /* cleanup */
              g_error_free (error);
            }
        }
      else
        {
          /* print warning */
          g_critical ("Failed to load configuration from \"%s\": %s", filename, error->message);

          /* cleanup */
          g_error_free (error);
        }
    }

  /* cleanup */
  g_free (filename);

  /* loading failed, create fallback panel */
  if (G_UNLIKELY (succeed == FALSE || application->windows == NULL))
    {
      /* create empty panel window */
      window = panel_application_new_window (application, NULL);

      /* TODO: create fallback panel layout instead of an empty window
       *       not entritely sure if an empty window is that bad... */

      /* show window */
      gtk_widget_show (GTK_WIDGET (window));
    }
}



static void
panel_application_load_set_property (PanelWindow *window,
                                     const gchar *name,
                                     const gchar *value)
{
  gint integer;

  panel_return_if_fail (name != NULL && value != NULL);
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the integer */
  integer = atoi (value);

  /* set the property */
  if (exo_str_is_equal (name, "locked"))
    panel_window_set_locked (window, !!integer);
  else if (exo_str_is_equal (name, "orientation"))
    panel_window_set_orientation (window, integer == 1 ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
  else if (exo_str_is_equal (name, "size"))
    panel_window_set_size (window, integer);
  else if (exo_str_is_equal (name, "snap-edge"))
    panel_window_set_snap_edge (window, CLAMP (integer, PANEL_SNAP_EGDE_NONE, PANEL_SNAP_EGDE_S));
  else if (exo_str_is_equal (name, "length"))
    panel_window_set_length (window, integer);
  else if (exo_str_is_equal (name, "autohide"))
    panel_window_set_autohide (window, !!integer);
  else if (exo_str_is_equal (name, "xoffset"))
    panel_window_set_xoffset (window, integer);
  else if (exo_str_is_equal (name, "yoffset"))
    panel_window_set_yoffset (window, integer);
  else if (exo_str_is_equal (name, "background-alpha"))
    panel_window_set_background_alpha (window, integer);
  else if (exo_str_is_equal (name, "enter-opacity"))
    panel_window_set_enter_opacity (window, integer);
  else if (exo_str_is_equal (name, "leave-opacity"))
    panel_window_set_leave_opacity (window, integer);
  else if (exo_str_is_equal (name, "span-monitors"))
    panel_window_set_span_monitors (window, !!integer);

  /* xfce 4.4 panel compatibility */
  else if (exo_str_is_equal (name, "transparency"))
    panel_window_set_leave_opacity (window, integer);
  else if (exo_str_is_equal (name, "activetrans"))
    panel_window_set_enter_opacity (window, integer == 1 ? 100 : panel_window_get_leave_opacity (window));
  else if (exo_str_is_equal (name, "fullwidth"))
    {
      /* 0: normal width, 1: full width and 2: span monitors */
      if (integer > 1)
        panel_window_set_length (window, 100);

      if (integer == 2)
        panel_window_set_span_monitors (window, TRUE);
    }
  else if (exo_str_is_equal (name, "screen-position"))
    {
      /* TODO: convert to the old screen position enum */
    }
}



static void
panel_application_load_start_element (GMarkupParseContext  *context,
                                      const gchar          *element_name,
                                      const gchar         **attribute_names,
                                      const gchar         **attribute_values,
                                      gpointer              user_data,
                                      GError              **error)
{
  Parser      *parser = user_data;
  gint         n;
  const gchar *name = NULL;
  const gchar *value = NULL;
  const gchar *id = NULL;

  switch (parser->state)
    {
      case PARSER_START:
        /* update parser state */
        if (exo_str_is_equal (element_name, "panels"))
          parser->state = PARSER_PANELS;
        break;

      case PARSER_PANELS:
        if (exo_str_is_equal (element_name, "panel"))
          {
            /* update parser state */
            parser->state = PARSER_PANEL;

            /* create new window */
            parser->window = panel_application_new_window (parser->application, NULL);
          }
        break;

      case PARSER_PANEL:
        /* update parser state */
        if (exo_str_is_equal (element_name, "properties"))
          parser->state = PARSER_PROPERTIES;
        else if (exo_str_is_equal (element_name, "items"))
          parser->state = PARSER_ITEMS;
        break;

      case PARSER_PROPERTIES:
        if (exo_str_is_equal (element_name, "property"))
          {
            /* walk attributes */
            for (n = 0; attribute_names[n] != NULL; n++)
              {
                if (exo_str_is_equal (attribute_names[n], "name"))
                  name = attribute_values[n];
                else if (exo_str_is_equal (attribute_names[n], "value"))
                  value = attribute_values[n];
              }

            /* set panel property */
            if (G_LIKELY (name != NULL && value != NULL))
              panel_application_load_set_property (parser->window, name, value);
          }
        break;

      case PARSER_ITEMS:
        if (exo_str_is_equal (element_name, "item"))
          {
            panel_return_if_fail (PANEL_IS_WINDOW (parser->window));

            /* walk attributes */
            for (n = 0; attribute_names[n] != NULL; n++)
              {
                /* get plugin name and id */
                if (exo_str_is_equal (attribute_names[n], "name"))
                  name = attribute_values[n];
                else if (exo_str_is_equal (attribute_names[n], "id"))
                  id = attribute_values[n];
              }

            /* append the new plugin */
            if (G_LIKELY (name != NULL))
              panel_application_plugin_insert (parser->application, parser->window,
                                               gtk_window_get_screen (GTK_WINDOW (parser->window)),
                                               name, id, NULL, -1);
          }
        break;

      default:
        /* set an error */
        g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                     "Unknown element <%s>", element_name);
        break;
    }
}



static void
panel_application_load_end_element (GMarkupParseContext  *context,
                                    const gchar          *element_name,
                                    gpointer              user_data,
                                    GError              **error)
{
  Parser *parser = user_data;

  switch (parser->state)
    {
      case PARSER_PANELS:
        /* update state */
        if (exo_str_is_equal (element_name, "panels"))
          parser->state = PARSER_START;
        break;

      case PARSER_PANEL:
        if (exo_str_is_equal (element_name, "panel"))
          {
            panel_return_if_fail (PANEL_IS_WINDOW (parser->window));

            /* show panel */
            gtk_widget_show (GTK_WIDGET (parser->window));

            /* update parser state */
            parser->state = PARSER_PANELS;
            parser->window = NULL;
          }
        break;

      case PARSER_PROPERTIES:
        /* update state */
        if (exo_str_is_equal (element_name, "properties"))
          parser->state = PARSER_PANEL;
        break;

      case PARSER_ITEMS:
        /* update state */
        if (exo_str_is_equal (element_name, "items"))
          parser->state = PARSER_PANEL;
        break;

      default:
        /* set an error */
        g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                     "Unknown element <%s>", element_name);
        break;
    }
}



static void
panel_application_plugin_move_end (GtkWidget        *item,
                                   GdkDragContext   *context,
                                   PanelApplication *application)
{
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* disconnect this signal */
  g_signal_handlers_disconnect_by_func (G_OBJECT (item), G_CALLBACK (panel_application_plugin_move_end), application);

  /* make the window insensitive */
  panel_application_windows_sensitive (application, TRUE);
}



static void
panel_application_plugin_move (GtkWidget        *item,
                               PanelApplication *application)
{
  GtkTargetList  *target_list;
  const gchar    *icon_name;
  GdkDragContext *context;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (item));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* make the window insensitive */
  panel_application_windows_sensitive (application, FALSE);

  /* create a target list */
  target_list = gtk_target_list_new (drag_targets, G_N_ELEMENTS (drag_targets));

  /* begin a drag */
  context = gtk_drag_begin (item, target_list, GDK_ACTION_MOVE, 1, NULL);
  
  /* set the drag context icon name */
  icon_name = panel_module_get_icon_name_from_plugin (XFCE_PANEL_PLUGIN_PROVIDER (item));
  gtk_drag_set_icon_name (context, icon_name ? icon_name : GTK_STOCK_DND, 0, 0); 

  /* release the drag list */
  gtk_target_list_unref (target_list);

  /* signal to make the window sensitive again on a drag end */
  g_signal_connect (G_OBJECT (item), "drag-end", G_CALLBACK (panel_application_plugin_move_end), application);
}



static void
panel_application_plugin_provider_signal (XfcePanelPluginProvider       *provider,
                                          XfcePanelPluginProviderSignal  signal,
                                          PanelApplication              *application)
{
  GtkWidget   *itembar;
  PanelWindow *window;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));
  
  /* get the panel of the plugin */
  window = PANEL_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (provider)));

  switch (signal)
    {
      case MOVE_PLUGIN:
        /* invoke the move function */
        panel_application_plugin_move (GTK_WIDGET (provider), application);
        break;

      case EXPAND_PLUGIN:
      case COLLAPSE_PLUGIN:
        /* get the itembar */
        itembar = gtk_bin_get_child (GTK_BIN (window));

        /* set new expand mode */
        panel_itembar_set_child_expand (PANEL_ITEMBAR (itembar), GTK_WIDGET (provider), !!(signal == EXPAND_PLUGIN));
        break;

      case LOCK_PANEL:
        /* block autohide */
        panel_window_freeze_autohide (window);
        break;
        
      case UNLOCK_PANEL:
        /* unblock autohide */
        panel_window_thaw_autohide (window);
        break;

      case REMOVE_PLUGIN:
        /* destroy the plugin if it's a panel plugin (ie. not external) */
        if (XFCE_IS_PANEL_PLUGIN (provider))
          gtk_widget_destroy (GTK_WIDGET (provider));
        break;

      case ADD_NEW_ITEMS:
        /* open the items dialog */
        panel_item_dialog_show (window);
        break;

      case PANEL_PREFERENCES:
        /* open the panel preferences */
        panel_preferences_dialog_show (window);
        break;

      default:
        g_critical ("Reveived unknown signal %d", signal);
        break;
    }
}



static gboolean
panel_application_plugin_insert (PanelApplication  *application,
                                 PanelWindow       *window,
                                 GdkScreen         *screen,
                                 const gchar       *name,
                                 const gchar       *id,
                                 gchar            **arguments,
                                 gint               position)
{
  GtkWidget               *itembar;
  gboolean                 succeed = FALSE;
  XfcePanelPluginProvider *provider;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);
  panel_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  panel_return_val_if_fail (name != NULL, FALSE);

  /* create a new panel plugin */
  provider = panel_module_factory_create_plugin (application->factory, screen, name, id, arguments);

  if (G_LIKELY (provider != NULL))
    {
      /* get the panel itembar */
      itembar = gtk_bin_get_child (GTK_BIN (window));

      /* add signal to monitor provider signals */
      g_signal_connect (G_OBJECT (provider), "provider-signal", G_CALLBACK (panel_application_plugin_provider_signal), application);

      /* add the item to the panel */
      panel_itembar_insert (PANEL_ITEMBAR (itembar), GTK_WIDGET (provider), position);

      /* send all the needed info about the panel to the plugin */
      panel_glue_set_provider_info (provider);

      /* show the plugin */
      gtk_widget_show (GTK_WIDGET (provider));

      /* we've succeeded */
      succeed = TRUE;
    }

  return succeed;
}



static gboolean
panel_application_save_timeout (gpointer user_data)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (user_data), FALSE);

  GDK_THREADS_ENTER ();

  /* save */
  panel_application_save (PANEL_APPLICATION (user_data));

  GDK_THREADS_LEAVE ();

  return TRUE;
}



static gchar *
panel_application_save_xml_contents (PanelApplication *application)
{
  GString                 *contents;
  GSList                  *li;
  PanelWindow             *window;
  GtkWidget               *itembar;
  GList                   *children, *lp;
  XfcePanelPluginProvider *provider;
  gchar                   *date_string;
  GTimeVal                 stamp;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);

  /* create string with some size to avoid reallocations */
  contents = g_string_sized_new (3072);

  /* create time string */
  g_get_current_time (&stamp);
  date_string = g_time_val_to_iso8601 (&stamp);

  /* start xml file */
  g_string_append_printf (contents, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE config SYSTEM \"config.dtd\">\n"
                          "<!-- Generated on %s -->\n"
                          "<panels>\n", date_string);

  /* cleanup */
  g_free (date_string);

  /* store each panel */
  for (li = application->windows; li != NULL; li = li->next)
    {
      /* get window */
      window = PANEL_WINDOW (li->data);

      /* panel grouping */
      contents = g_string_append (contents, "\t<panel>\n"
                                            "\t\t<properties>\n");

      /* store panel properties */
      g_string_append_printf (contents, "\t\t\t<property name=\"locked\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"orientation\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"size\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"snap-edge\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"length\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"autohide\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"xoffset\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"yoffset\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"background-alpha\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"enter-opacity\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"leave-opacity\" value=\"%d\" />\n"
                                        "\t\t\t<property name=\"span-monitors\" value=\"%d\" />\n",
                                        panel_window_get_locked (window),
                                        panel_window_get_orientation (window),
                                        panel_window_get_size (window),
                                        panel_window_get_snap_edge (window),
                                        panel_window_get_length (window),
                                        panel_window_get_autohide (window),
                                        panel_window_get_xoffset (window),
                                        panel_window_get_yoffset (window),
                                        panel_window_get_background_alpha (window),
                                        panel_window_get_enter_opacity (window),
                                        panel_window_get_leave_opacity (window),
                                        panel_window_get_span_monitors (window));

      /* item grouping */
      contents = g_string_append (contents, "\t\t</properties>\n"
                                            "\t\t<items>\n");

      /* get the itembar */
      itembar = gtk_bin_get_child (GTK_BIN (window));

      /* debug check */
      panel_assert (PANEL_IS_ITEMBAR (itembar));

      /* get the itembar children */
      children = gtk_container_get_children (GTK_CONTAINER (itembar));

      /* store the plugin properties */
      for (lp = children; lp != NULL; lp = lp->next)
        {
          /* cast to a plugin provider */
          provider = XFCE_PANEL_PLUGIN_PROVIDER (lp->data);

          /* store plugin name and id */
          g_string_append_printf (contents, "\t\t\t<item name=\"%s\" id=\"%s\" />\n",
                                  xfce_panel_plugin_provider_get_name (provider),
                                  xfce_panel_plugin_provider_get_id (provider));
        }

      /* cleanup */
      g_list_free (children);

      /* close grouping */
      contents = g_string_append (contents, "\t\t</items>\n"
                                            "\t</panel>\n");
    }

  /* close xml file */
  contents = g_string_append (contents, "</panels>\n");

  return g_string_free (contents, FALSE);
}



static void
panel_application_window_destroyed (GtkWidget        *window,
                                    PanelApplication *application)
{
  panel_return_if_fail (PANEL_IS_WINDOW (window));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (g_slist_find (application->windows, window) != NULL);

  /* remove the window from the list */
  application->windows = g_slist_remove (application->windows, window);

  /* quit if there are no windows opened */
  if (application->windows == NULL)
    gtk_main_quit ();
}



static void
panel_application_dialog_destroyed (GtkWindow        *dialog,
                                    PanelApplication *application)
{
  panel_return_if_fail (GTK_IS_WINDOW (dialog));
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (g_slist_find (application->dialogs, dialog) != NULL);

  /* remove the window from the list */
  application->dialogs = g_slist_remove (application->dialogs, dialog);
}



static void
panel_application_drag_data_received (GtkWidget        *itembar,
                                      GdkDragContext   *context,
                                      gint              x,
                                      gint              y,
                                      GtkSelectionData *selection_data,
                                      guint             info,
                                      guint             time,
                                      PanelWindow      *window)
{
  guint             position;
  PanelApplication *application;
  GtkWidget        *provider;
  gboolean          succeed = FALSE;
  GdkScreen        *screen;
  const gchar      *name;
  guint             old_position;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GDK_IS_DRAG_CONTEXT (context));
  panel_return_if_fail (PANEL_IS_WINDOW (window));

  /* get the application */
  application = panel_application_get ();

  /* get the drop index on the itembar */
  position = panel_itembar_get_drop_index (PANEL_ITEMBAR (itembar), x, y);

  /* get the widget screen */
  screen = gtk_widget_get_screen (itembar);

  switch (info)
    {
      case PANEL_ITEMBAR_TARGET_PLUGIN_NAME:
        if (G_LIKELY (selection_data->length > 0))
          {
            /* get the name from the selection data */
            name = (const gchar *) selection_data->data;

            /* create a new item with a unique id */
            succeed = panel_application_plugin_insert (application, window, screen, name,
                                                       NULL, NULL, position);
          }
        break;

      case PANEL_ITEMBAR_TARGET_PLUGIN_WIDGET:
        /* get the source widget */
        provider = gtk_drag_get_source_widget (context);

        /* debug check */
        panel_return_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (provider));

        /* check if we move to another itembar */
        if (gtk_widget_get_parent (provider) == itembar)
          {
            /* get the current position on the itembar */
            old_position = panel_itembar_get_child_index (PANEL_ITEMBAR (itembar), provider);

            /* decrease the counter if we drop after the current position */
            if (position > old_position)
              position--;

            /* reorder the child if needed */
            if (old_position != position)
              panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, position);
          }
        else
          {
            /* reparent the widget, this will also call remove and add for the itembar */
            gtk_widget_reparent (provider, itembar);

            /* move the item to the correct position on the itembar */
            panel_itembar_reorder_child (PANEL_ITEMBAR (itembar), provider, position);

            /* send all the needed panel information to the plugin */
            panel_glue_set_provider_info (XFCE_PANEL_PLUGIN_PROVIDER (provider));
          }

        /* everything went fine */
        succeed = TRUE;
        break;

      default:
        panel_assert_not_reached ();
        break;
    }

  /* save the panel configuration if we succeeded */
  if (G_LIKELY (succeed))
    panel_application_save (application);

  /* release the application */
  g_object_unref (G_OBJECT (application));

  /* tell the peer that we handled the drop */
  gtk_drag_finish (context, succeed, FALSE, time);
}



static gboolean
panel_application_drag_drop (GtkWidget      *itembar,
                             GdkDragContext *context,
                             gint            x,
                             gint            y,
                             guint           time,
                             PanelWindow    *window)
{
  GdkAtom target;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), FALSE);
  panel_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), FALSE);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), FALSE);

  target = gtk_drag_dest_find_target (itembar, context, NULL);

  /* we cannot handle the drag data */
  if (G_UNLIKELY (target == GDK_NONE))
    return FALSE;

  /* request the drag data */
  gtk_drag_get_data (itembar, context, target, time);

  /* we call gtk_drag_finish later */
  return TRUE;
}



PanelApplication *
panel_application_get (void)
{
  static PanelApplication *application = NULL;

  if (G_LIKELY (application))
    {
      g_object_ref (G_OBJECT (application));
    }
  else
    {
      application = g_object_new (PANEL_TYPE_APPLICATION, NULL);
      g_object_add_weak_pointer (G_OBJECT (application), (gpointer) &application);
    }

  return application;
}



void
panel_application_save (PanelApplication *application)
{
  gchar     *filename;
  gchar     *contents;
  gboolean   succeed;
  GError    *error = NULL;
  GSList    *li;
  GtkWidget *itembar;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* get save location */
  filename = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, PANEL_CONFIG_PATH, TRUE);
  if (G_LIKELY (filename))
    {
      /* get the file xml data */
      contents = panel_application_save_xml_contents (application);

      /* write the data to the file */
      succeed = g_file_set_contents (filename, contents, -1, &error);
      if (G_UNLIKELY (succeed == FALSE))
        {
          /* writing failed, print warning */
          g_critical ("Failed to write panel configuration to \"%s\": %s", filename, error->message);

          /* cleanup */
          g_error_free (error);
        }

      /* cleanup */
      g_free (contents);
      g_free (filename);
    }
  else
    {
      /* print warning */
      g_critical ("Failed to create panel configuration file");
    }

  /* save the settings of all plugins */
  for (li = application->windows; li != NULL; li = li->next)
    {
      /* get the itembar */
      itembar = gtk_bin_get_child (GTK_BIN (li->data));

      /* save all the plugins on the itembar */
      gtk_container_foreach (GTK_CONTAINER (itembar), (GtkCallback) xfce_panel_plugin_provider_save, NULL);
    }
}



void
panel_application_take_dialog (PanelApplication *application,
                               GtkWindow        *dialog)
{
  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (GTK_IS_WINDOW (dialog));

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (panel_application_dialog_destroyed), application);

  /* add the window to internal list */
  application->dialogs = g_slist_prepend (application->dialogs, dialog);
}



void
panel_application_destroy_dialogs (PanelApplication *application)
{
  GSList *li, *lnext;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* destroy all dialogs */
  for (li = application->dialogs; li != NULL; li = lnext)
    {
      /* get next element */
      lnext = li->next;

      /* destroy the window */
      gtk_widget_destroy (GTK_WIDGET (li->data));
    }

  /* check */
  panel_return_if_fail (application->dialogs == NULL);
}



void
panel_application_add_new_item (PanelApplication  *application,
                                const gchar       *plugin_name,
                                gchar            **arguments)
{
  PanelWindow *window;
  gint         nth = 0;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (plugin_name != NULL);
  panel_return_if_fail (g_slist_length (application->windows) > 0);

  if (panel_module_factory_has_module (application->factory, plugin_name))
    {
      /* ask the user what panel to use if there is more then one */
      if (g_slist_length (application->windows) > 1)
        if ((nth = panel_dialogs_choose_panel (application)) == -1)
          return;

      /* get the window */
      window = g_slist_nth_data (application->windows, nth);

      /* add the panel to the end of the choosen window */
      panel_application_plugin_insert (application, window, gtk_widget_get_screen (GTK_WIDGET (window)),
                                       plugin_name, NULL, arguments, -1);
    }
  else
    {
      /* print warning */
      g_warning (_("The plugin (%s) you want to add is not recognized by the panel."), plugin_name);
    }
}



PanelWindow *
panel_application_new_window (PanelApplication *application,
                              GdkScreen        *screen)
{
  GtkWidget *window;
  GtkWidget *itembar;

  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);
  panel_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), NULL);

  /* create panel window */
  window = panel_window_new ();

  /* realize */
  gtk_widget_realize (window);

  /* monitor window destruction */
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (panel_application_window_destroyed), application);

  /* put on the correct screen */
  gtk_window_set_screen (GTK_WINDOW (window), screen ? screen : gdk_screen_get_default ());

  /* add the window to internal list */
  application->windows = g_slist_append (application->windows, window);

  /* add the itembar */
  itembar = panel_itembar_new ();
  exo_binding_new (G_OBJECT (window), "orientation", G_OBJECT (itembar), "orientation");
  gtk_container_add (GTK_CONTAINER (window), itembar);
  gtk_widget_show (itembar);

  /* signals for drag and drop */
  g_signal_connect (G_OBJECT (itembar), "drag-data-received", G_CALLBACK (panel_application_drag_data_received), window);
  g_signal_connect (G_OBJECT (itembar), "drag-drop", G_CALLBACK (panel_application_drag_drop), window);

  return PANEL_WINDOW (window);
}



GSList *
panel_application_get_windows (PanelApplication *application)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), NULL);

  return application->windows;
}



gint
panel_application_get_n_windows (PanelApplication *application)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);

  return g_slist_length (application->windows);
}



gint
panel_application_get_window_index (PanelApplication *application,
                                    PanelWindow      *window)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);
  panel_return_val_if_fail (PANEL_IS_WINDOW (window), 0);

  return g_slist_index (application->windows, window);
}



PanelWindow *
panel_application_get_window (PanelApplication *application,
                              guint             idx)
{
  panel_return_val_if_fail (PANEL_IS_APPLICATION (application), 0);

  return g_slist_nth_data (application->windows, idx);
}



void
panel_application_window_select (PanelApplication *application,
                                 PanelWindow      *window)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));
  panel_return_if_fail (window == NULL || PANEL_IS_WINDOW (window));

  /* update state for all windows */
  for (li = application->windows; li != NULL; li = li->next)
    panel_window_set_active_panel (PANEL_WINDOW (li->data), !!(li->data == window));
}



void
panel_application_windows_sensitive (PanelApplication *application,
                                     gboolean          sensitive)
{
  GtkWidget *itembar;
  GSList    *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  /* walk the windows */
  for (li = application->windows; li != NULL; li = li->next)
    {
      /* get the window itembar */
      itembar = gtk_bin_get_child (GTK_BIN (li->data));

      /* set sensitivity of the itembar (and the plugins) */
      panel_itembar_set_sensitive (PANEL_ITEMBAR (itembar), sensitive);

      /* block autohide for all windows */
      if (sensitive)
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
    }
}



void
panel_application_windows_autohide (PanelApplication *application,
                                    gboolean          freeze)
{
  GSList *li;

  panel_return_if_fail (PANEL_IS_APPLICATION (application));

  for (li = application->windows; li != NULL; li = li->next)
    {
      if (freeze)
        panel_window_freeze_autohide (PANEL_WINDOW (li->data));
      else
        panel_window_thaw_autohide (PANEL_WINDOW (li->data));
    }
}
