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

#include "tasklist-plugin.h"

struct _TasklistPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _TasklistPlugin
{
  XfcePanelPlugin __parent__;
};



static void tasklist_plugin_class_init              (TasklistPluginClass  *klass);
static void tasklist_plugin_init                    (TasklistPlugin       *plugin);
static void tasklist_plugin_orientation_changed     (XfcePanelPlugin      *plugin,
                                                     GtkOrientation        orientation);
static void tasklist_plugin_size_changed            (XfcePanelPlugin      *plugin,
                                                     gint                  size);
static void tasklist_plugin_about                   (XfcePanelPlugin      *plugin);
static void tasklist_plugin_save                    (XfcePanelPlugin      *plugin);
static void tasklist_plugin_configure_plugin        (XfcePanelPlugin      *plugin);
static void tasklist_plugin_screen_position_changed (XfcePanelPlugin      *plugin,
                                                     gint                  position);



XFCE_PANEL_PLUGIN_DEFINE_TYPE (TasklistPlugin, tasklist_plugin);



static void
tasklist_plugin_class_init (TasklistPluginClass *klass)
{
  GObjectClass         *gobject_class;
  XfcePanelPluginClass *plugin_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  
  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->orientation_changed = tasklist_plugin_orientation_changed;
  plugin_class->size_changed = tasklist_plugin_size_changed;
  plugin_class->about = tasklist_plugin_about;
  plugin_class->save = tasklist_plugin_save;
  plugin_class->configure_plugin = tasklist_plugin_configure_plugin;
  plugin_class->screen_position_changed = tasklist_plugin_screen_position_changed;
}



static void
tasklist_plugin_init (TasklistPlugin *plugin)
{
  GtkWidget *button;
  
  button = gtk_button_new_from_stock (GTK_STOCK_FLOPPY);
  gtk_container_add (GTK_CONTAINER (plugin), button);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), button);
  gtk_widget_show (button);
}



static void
tasklist_plugin_orientation_changed (XfcePanelPlugin *plugin,
                                     GtkOrientation   orientation)
{

}



static void
tasklist_plugin_size_changed (XfcePanelPlugin *plugin,
                              gint             size)
{

}



static void
tasklist_plugin_about (XfcePanelPlugin *plugin)
{

}



static void
tasklist_plugin_save (XfcePanelPlugin *plugin)
{

}



static void
tasklist_plugin_configure_plugin (XfcePanelPlugin *plugin)
{

}



static void
tasklist_plugin_screen_position_changed (XfcePanelPlugin *plugin,
                                         gint             position)
{

}
