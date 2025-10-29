/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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

#include "tasklist-widget.h"
#include "tasklist.h"

#include "common/panel-private.h"
#include "common/panel-utils.h"
#include "common/panel-xfconf.h"

#include <libxfce4ui/libxfce4ui.h>


#define HANDLE_SIZE (4)

/* Taken from panel/panel-preferences-dialog.c */
enum
{
  OUTPUT_NAME,
  OUTPUT_TITLE
};


struct _TasklistPlugin
{
  XfcePanelPlugin __parent__;

  /* the tasklist widget */
  GtkWidget *tasklist;
  GtkWidget *handle;
  GObject *settings_dialog;
};



static void
tasklist_plugin_construct (XfcePanelPlugin *panel_plugin);
static void
tasklist_plugin_mode_changed (XfcePanelPlugin *panel_plugin,
                              XfcePanelPluginMode mode);
static gboolean
tasklist_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint size);
static void
tasklist_plugin_nrows_changed (XfcePanelPlugin *panel_plugin,
                               guint nrows);
static void
tasklist_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                         XfceScreenPosition position);
static void
tasklist_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);
static gboolean
tasklist_plugin_handle_draw (GtkWidget *widget,
                             cairo_t *cr,
                             TasklistPlugin *plugin);



/* define and register the plugin */
XFCE_PANEL_DEFINE_PLUGIN_RESIDENT (TasklistPlugin, tasklist_plugin)



static void
tasklist_plugin_class_init (TasklistPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = tasklist_plugin_construct;
  plugin_class->mode_changed = tasklist_plugin_mode_changed;
  plugin_class->size_changed = tasklist_plugin_size_changed;
  plugin_class->nrows_changed = tasklist_plugin_nrows_changed;
  plugin_class->screen_position_changed = tasklist_plugin_screen_position_changed;
  plugin_class->configure_plugin = tasklist_plugin_configure_plugin;
}



static void
tasklist_plugin_init (TasklistPlugin *plugin)
{
  GtkWidget *box;

  /* create widgets */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (plugin), box);
  g_object_bind_property (G_OBJECT (plugin), "orientation",
                          G_OBJECT (box), "orientation",
                          G_BINDING_SYNC_CREATE);
  gtk_widget_show (box);

  plugin->handle = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), plugin->handle, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (plugin->handle), "draw",
                    G_CALLBACK (tasklist_plugin_handle_draw), plugin);
  gtk_widget_set_size_request (plugin->handle, 8, 8);
  gtk_widget_show (plugin->handle);

  plugin->tasklist = g_object_new (XFCE_TYPE_TASKLIST, NULL);
  gtk_box_pack_start (GTK_BOX (box), plugin->tasklist, TRUE, TRUE, 0);

  g_object_bind_property (G_OBJECT (plugin->tasklist), "show-handle",
                          G_OBJECT (plugin->handle), "visible",
                          G_BINDING_SYNC_CREATE);
}



static void
tasklist_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);
  const PanelProperty properties[] = {
    { "show-labels", G_TYPE_BOOLEAN },
    { "grouping", G_TYPE_BOOLEAN },
    { "include-all-workspaces", G_TYPE_BOOLEAN },
    { "include-all-monitors", G_TYPE_BOOLEAN },
    { "include-single-monitor", G_TYPE_UINT}, 
    { "flat-buttons", G_TYPE_BOOLEAN },
    { "switch-workspace-on-unminimize", G_TYPE_BOOLEAN },
    { "show-only-minimized", G_TYPE_BOOLEAN },
    { "show-wireframes", G_TYPE_BOOLEAN },
    { "show-handle", G_TYPE_BOOLEAN },
    { "show-tooltips", G_TYPE_BOOLEAN },
    { "sort-order", G_TYPE_UINT },
    { "window-scrolling", G_TYPE_BOOLEAN },
    { "wrap-windows", G_TYPE_BOOLEAN },
    { "include-all-blinking", G_TYPE_BOOLEAN },
    { "middle-click", G_TYPE_UINT },
    { "label-decorations", G_TYPE_BOOLEAN },
    { NULL }
  };

  /* show configure */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* bind all properties */
  panel_properties_bind (NULL, G_OBJECT (plugin->tasklist),
                         xfce_panel_plugin_get_property_base (panel_plugin),
                         properties, FALSE);

  /* show the tasklist */
  gtk_widget_show (plugin->tasklist);
}



static void
tasklist_plugin_mode_changed (XfcePanelPlugin *panel_plugin,
                              XfcePanelPluginMode mode)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);

  /* set the new tasklist mode */
  xfce_tasklist_set_mode (XFCE_TASKLIST (plugin->tasklist), mode);
}



static gboolean
tasklist_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint size)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);

  /* set the tasklist size */
  xfce_tasklist_set_size (XFCE_TASKLIST (plugin->tasklist), size);

  return TRUE;
}



static void
tasklist_plugin_nrows_changed (XfcePanelPlugin *panel_plugin,
                               guint nrows)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);

  /* set the tasklist nrows */
  xfce_tasklist_set_nrows (XFCE_TASKLIST (plugin->tasklist), nrows);
}



static void
tasklist_plugin_screen_position_changed (XfcePanelPlugin *panel_plugin,
                                         XfceScreenPosition position)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);

  /* update monitor geometry; this function is also triggered when
   * the panel is moved to another monitor during runtime */
  xfce_tasklist_update_monitor_geometry (XFCE_TASKLIST (plugin->tasklist));
}

static void
tasklist_plugin_include_monitors_changed (GtkComboBox *combobox, XfceTasklist *tasklist)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;
  gchar        *name;
  gint          index;
  guint         all_monitors;
  GValue        value = G_VALUE_INIT;

  g_value_init(&value, G_TYPE_UINT);

  if (gtk_combo_box_get_active_iter (combobox, &iter))
    {
      model = gtk_combo_box_get_model (combobox);
      gtk_tree_model_get (model, &iter, OUTPUT_NAME, &name, -1);
      if (sscanf (name, "monitor-%d", &index) == 1)
        {
          all_monitors = 0;
        }
      else
        {
          index = 0;
          all_monitors = 1;
        }

      /* set monitor index */
      g_value_set_uint(&value, index);
      g_object_set_property(G_OBJECT(tasklist), "include-single-monitor", &value);

      /* set all monitors flag */
      g_value_set_uint(&value, all_monitors);
      g_object_set_property(G_OBJECT(tasklist), "include-all-monitors", &value);

      g_free (name);
    }
}

static void
tasklist_plugin_configure_monitor_combobox (GtkBuilder    *builder, 
                                            GObject       *dialog,
                                            XfceTasklist  *tasklist)
{
  /* This function configures ComboBox with monitors name */

  GtkTreeIter  iter;
  gint         n_monitors, i, n = 0;
  GObject     *combobox, *store;
  GdkDisplay  *display;
  GdkMonitor  *monitor;
  gchar       *title, *name;
  gboolean     selected = FALSE;

  /* Get ComboBox, make sure it is here */
  combobox = gtk_builder_get_object (builder, "include-monitors");
  panel_return_if_fail (GTK_IS_COMBO_BOX (combobox));

  /* Get ComboBox model, make sure it is here, clear it */
  store = gtk_builder_get_object (builder, "monitors-model");
  panel_return_if_fail (GTK_IS_LIST_STORE (store));
  gtk_list_store_clear (GTK_LIST_STORE (store));

  /* Insert primary option: do not filter buttons by monitor */
  gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                     OUTPUT_NAME, "all",
                                     OUTPUT_TITLE, _("Show windows from all monitors"), -1);


  /* Get total number of monitors */
  display = gtk_widget_get_display (GTK_WIDGET (dialog));
  n_monitors = gdk_display_get_n_monitors (display);
  for (i = 0; i < n_monitors; i++)
    {
      /* Taken from panel/panel-preferences-dialog.c */

      monitor = gdk_display_get_monitor (display, i);

      /* `name` must have this format since it is parsed later with sscanf() */
      name = g_strdup_printf ("monitor-%d", i);

      title = g_strdup(gdk_monitor_get_model (monitor));
      if (xfce_str_is_empty (title))
        {
          g_free (title);
          title = g_strdup_printf (_("Monitor %d"), i + 1);
        }

      /* insert row into model */
      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, n++,
                                         OUTPUT_NAME, name,
                                         OUTPUT_TITLE, title, -1);

      /* if we have any monitor chosen - select it in combobox */
      if (!tasklist->all_monitors && tasklist->monitor_index == (guint)i)
        {
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combobox), &iter);
          selected = TRUE;
        }

      g_free (name);
      g_free (title);
    }

  /* select default combobox row if none selected previously */
  if (!selected)
    gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);

  /* catch `changed` events */
  g_signal_connect (
    gtk_builder_get_object (builder, "include-monitors"),
    "changed",
    G_CALLBACK (tasklist_plugin_include_monitors_changed),
    tasklist
  );
}


static void
tasklist_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  TasklistPlugin *plugin = TASKLIST_PLUGIN (panel_plugin);
  GtkBuilder *builder;
  GObject *object;

  /* setup the dialog */
  builder = panel_utils_builder_new (panel_plugin, "/org/xfce/panel/tasklist-dialog.glade", &plugin->settings_dialog);
  if (G_UNLIKELY (builder == NULL))
    return;

#define TASKLIST_DIALOG_BIND(name, property) \
  object = gtk_builder_get_object (builder, (name)); \
  panel_return_if_fail (G_IS_OBJECT (object)); \
  g_object_bind_property (G_OBJECT (plugin->tasklist), (name), \
                          G_OBJECT (object), (property), \
                          G_BINDING_BIDIRECTIONAL \
                            | G_BINDING_SYNC_CREATE);

#define TASKLIST_DIALOG_BIND_INV(name, property) \
  object = gtk_builder_get_object (builder, (name)); \
  panel_return_if_fail (G_IS_OBJECT (object)); \
  g_object_bind_property (G_OBJECT (plugin->tasklist), \
                          name, G_OBJECT (object), \
                          property, \
                          G_BINDING_BIDIRECTIONAL \
                            | G_BINDING_SYNC_CREATE \
                            | G_BINDING_INVERT_BOOLEAN);

  TASKLIST_DIALOG_BIND ("show-labels", "active")
  TASKLIST_DIALOG_BIND ("grouping", "active")
  TASKLIST_DIALOG_BIND ("include-all-workspaces", "active")
  TASKLIST_DIALOG_BIND ("flat-buttons", "active")
  TASKLIST_DIALOG_BIND_INV ("switch-workspace-on-unminimize", "active")
  TASKLIST_DIALOG_BIND ("show-only-minimized", "active")
  TASKLIST_DIALOG_BIND ("show-wireframes", "active")
  TASKLIST_DIALOG_BIND ("show-handle", "active")
  TASKLIST_DIALOG_BIND ("show-tooltips", "active")
  TASKLIST_DIALOG_BIND ("sort-order", "active")
  TASKLIST_DIALOG_BIND ("window-scrolling", "active")
  TASKLIST_DIALOG_BIND ("middle-click", "active")

  if (!WINDOWING_IS_X11 ())
    {
      /* not functional in x11, so avoid confusion */
      object = gtk_builder_get_object (builder, "include-all-workspaces");
      gtk_widget_hide (GTK_WIDGET (object));
      object = gtk_builder_get_object (builder, "switch-workspace-on-unminimize");
      gtk_widget_hide (GTK_WIDGET (object));
      object = gtk_builder_get_object (builder, "show-wireframes");
      gtk_widget_hide (GTK_WIDGET (object));
    }

  tasklist_plugin_configure_monitor_combobox(builder, plugin->settings_dialog, XFCE_TASKLIST (plugin->tasklist));

  gtk_widget_show (GTK_WIDGET (plugin->settings_dialog));
}



static gboolean
tasklist_plugin_handle_draw (GtkWidget *widget,
                             cairo_t *cr,
                             TasklistPlugin *plugin)
{
  GtkAllocation allocation;
  GtkStyleContext *ctx;
  gdouble x, y;
  guint i;
  GdkRGBA fg_rgba;

  panel_return_val_if_fail (TASKLIST_IS_PLUGIN (plugin), FALSE);
  panel_return_val_if_fail (plugin->handle == widget, FALSE);

  if (!gtk_widget_is_drawable (widget))
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);
  ctx = gtk_widget_get_style_context (widget);

  gtk_style_context_get_color (ctx, gtk_widget_get_state_flags (widget), &fg_rgba);
  /* Tone down the foreground color a bit for the separators */
  fg_rgba.alpha = 0.5;
  gdk_cairo_set_source_rgba (cr, &fg_rgba);
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);

  x = (allocation.width - HANDLE_SIZE) / 2;
  y = (allocation.height - HANDLE_SIZE) / 2;
  cairo_set_line_width (cr, 1.0);
  /* draw the handle */
  for (i = 0; i < 3; i++)
    {
      if (xfce_panel_plugin_get_orientation (XFCE_PANEL_PLUGIN (plugin)) == GTK_ORIENTATION_HORIZONTAL)
        {
          cairo_move_to (cr, x, y + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2));
          cairo_line_to (cr, x + HANDLE_SIZE, y + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2));
        }
      else
        {
          cairo_move_to (cr, x + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2), y);
          cairo_line_to (cr, x + (i * HANDLE_SIZE) - (HANDLE_SIZE / 2), y + HANDLE_SIZE);
        }
      cairo_stroke (cr);
    }

  return TRUE;
}
