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

#include <xfconf/xfconf.h>
#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>

#include "tasklist-widget.h"
#include "tasklist-dialog_glade.h"

/* TODO move to header */
GType tasklist_plugin_get_type (void) G_GNUC_CONST;
#define XFCE_TYPE_TASKLIST_PLUGIN            (tasklist_plugin_get_type ())
#define XFCE_TASKLIST_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_TASKLIST_PLUGIN, TasklistPlugin))
#define XFCE_TASKLIST_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_TASKLIST_PLUGIN, TasklistPluginClass))
#define XFCE_IS_TASKLIST_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_TASKLIST_PLUGIN))
#define XFCE_IS_TASKLIST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_TASKLIST_PLUGIN))
#define XFCE_TASKLIST_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_TASKLIST_PLUGIN, TasklistPluginClass))


typedef struct _TasklistPluginClass TasklistPluginClass;
struct _TasklistPluginClass
{
  XfcePanelPluginClass __parent__;
};

typedef struct _TasklistPlugin TasklistPlugin;
struct _TasklistPlugin
{
  XfcePanelPlugin __parent__;

  /* the xfconf channel */
  XfconfChannel *channel;

  /* the tasklist widget */
  GtkWidget     *tasklist;
};



static void tasklist_plugin_construct (XfcePanelPlugin *panel_plugin);
static void tasklist_plugin_free_data (XfcePanelPlugin *panel_plugin);
static void tasklist_plugin_orientation_changed (XfcePanelPlugin *panel_plugin, GtkOrientation orientation);
static gboolean tasklist_plugin_size_changed (XfcePanelPlugin *panel_plugin, gint size);
static void tasklist_plugin_configure_plugin (XfcePanelPlugin *panel_plugin);



G_DEFINE_TYPE (TasklistPlugin, tasklist_plugin, XFCE_TYPE_PANEL_PLUGIN);



/* register the panel plugin */
XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_TASKLIST_PLUGIN);



static void
tasklist_plugin_class_init (TasklistPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = tasklist_plugin_construct;
  plugin_class->free_data = tasklist_plugin_free_data;
  plugin_class->orientation_changed = tasklist_plugin_orientation_changed;
  plugin_class->size_changed = tasklist_plugin_size_changed;
  plugin_class->configure_plugin = tasklist_plugin_configure_plugin;
}



static void
tasklist_plugin_init (TasklistPlugin *plugin)
{
  /* initialize xfconf */
  xfconf_init (NULL);

  /* show configure */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (plugin));

  /* create the tasklist */
  plugin->tasklist = g_object_new (XFCE_TYPE_TASKLIST, NULL);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->tasklist);
}



static void
tasklist_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  TasklistPlugin *plugin = XFCE_TASKLIST_PLUGIN (panel_plugin);

  /* expand the plugin */
  xfce_panel_plugin_set_expand (panel_plugin, TRUE);

  /* open the xfconf channel */
  plugin->channel = xfce_panel_plugin_xfconf_channel_new (panel_plugin);

#define TASKLIST_XFCONF_BIND(name, gtype) \
  xfconf_g_property_bind (plugin->channel, "/" name, gtype, \
                          plugin->tasklist, name);

  /* create bindings */
  TASKLIST_XFCONF_BIND ("show-labels", G_TYPE_BOOLEAN)
  TASKLIST_XFCONF_BIND ("grouping", G_TYPE_UINT)
  TASKLIST_XFCONF_BIND ("include-all-workspaces", G_TYPE_BOOLEAN)
  TASKLIST_XFCONF_BIND ("flat-buttons", G_TYPE_BOOLEAN)
  TASKLIST_XFCONF_BIND ("switch-workspace-on-unminimize", G_TYPE_BOOLEAN)
  TASKLIST_XFCONF_BIND ("show-only-minimized", G_TYPE_BOOLEAN)
  TASKLIST_XFCONF_BIND ("show-wireframes", G_TYPE_BOOLEAN)

  /* show the tasklist */
  gtk_widget_show (plugin->tasklist);
}



static void
tasklist_plugin_free_data (XfcePanelPlugin *panel_plugin)
{
  TasklistPlugin *plugin = XFCE_TASKLIST_PLUGIN (panel_plugin);

  /* release the xfconf channel */
  if (G_LIKELY (plugin->channel))
    g_object_unref (G_OBJECT (plugin->channel));

  /* shutdown xfconf */
  xfconf_shutdown ();
}



static void
tasklist_plugin_orientation_changed (XfcePanelPlugin *panel_plugin,
                                     GtkOrientation   orientation)
{
  TasklistPlugin *plugin = XFCE_TASKLIST_PLUGIN (panel_plugin);

  /* set the new tasklist orientation */
  xfce_tasklist_set_orientation (XFCE_TASKLIST (plugin->tasklist), orientation);
}



static gboolean
tasklist_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                              gint             size)
{
  TasklistPlugin *plugin = XFCE_TASKLIST_PLUGIN (panel_plugin);

  /* set the tasklist size */
  xfce_tasklist_set_size (XFCE_TASKLIST (plugin->tasklist), size);

  return TRUE;
}



static void
tasklist_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  TasklistPlugin *plugin = XFCE_TASKLIST_PLUGIN (panel_plugin);
  GtkBuilder     *builder;
  GObject        *dialog;
  GObject        *object;

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, tasklist_dialog_glade,
                                   tasklist_dialog_glade_length, NULL))
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

#define TASKLIST_DIALOG_BIND(name, property) \
      object = gtk_builder_get_object (builder, (name)); \
      exo_mutual_binding_new (G_OBJECT (plugin->tasklist), (name), \
                              G_OBJECT (object), (property));

#define TASKLIST_DIALOG_BIND_INV(name, property) \
      object = gtk_builder_get_object (builder, name); \
      exo_mutual_binding_new_with_negation (G_OBJECT (plugin->tasklist), \
                                            name,  G_OBJECT (object), \
                                            property);

      TASKLIST_DIALOG_BIND ("show-labels", "active")
      TASKLIST_DIALOG_BIND ("grouping", "active")
      TASKLIST_DIALOG_BIND ("include-all-workspaces", "active")
      TASKLIST_DIALOG_BIND ("flat-buttons", "active")
      TASKLIST_DIALOG_BIND_INV ("switch-workspace-on-unminimize", "active")
      TASKLIST_DIALOG_BIND ("show-only-minimized", "active")
      TASKLIST_DIALOG_BIND ("show-wireframes", "active")

      gtk_widget_show (GTK_WIDGET (dialog));
	}
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}

