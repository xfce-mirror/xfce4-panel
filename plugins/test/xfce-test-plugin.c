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
static void xfce_test_plugin_finalize (GObject *object);
static void xfce_test_plugin_construct (XfcePanelPlugin *plugin);
static void xfce_test_plugin_free_data (XfcePanelPlugin *plugin);
static void xfce_test_plugin_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation);
static gboolean xfce_test_plugin_size_changed (XfcePanelPlugin *plugin, gint size);
static void xfce_test_plugin_save (XfcePanelPlugin *plugin);
static void xfce_test_plugin_configure_plugin (XfcePanelPlugin *plugin);
static void xfce_test_plugin_screen_position_changed (XfcePanelPlugin *plugin, gint position);

XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_TEST_PLUGIN);

G_DEFINE_TYPE (XfceTestPlugin, xfce_test_plugin, XFCE_TYPE_PANEL_PLUGIN);



static void
xfce_test_plugin_class_init (XfceTestPluginClass *klass)
{
  GObjectClass *gobject_class;
  XfcePanelPluginClass *plugin_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_test_plugin_finalize;
  
  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = xfce_test_plugin_construct;
  plugin_class->free_data = xfce_test_plugin_free_data;
  plugin_class->orientation_changed = xfce_test_plugin_orientation_changed;
  plugin_class->size_changed = xfce_test_plugin_size_changed;
  plugin_class->save = xfce_test_plugin_save;
  plugin_class->configure_plugin = xfce_test_plugin_configure_plugin;
  plugin_class->screen_position_changed = xfce_test_plugin_screen_position_changed;
}



static void
xfce_test_plugin_init (XfceTestPlugin *plugin)
{
  g_message ("init plugin %s", xfce_panel_plugin_get_name (XFCE_PANEL_PLUGIN (plugin)));
}



static void 
xfce_test_plugin_finalize (GObject *object)
{
  g_message ("plugin %s finalized", xfce_panel_plugin_get_name (XFCE_PANEL_PLUGIN (object)));
  
  (*G_OBJECT_CLASS (xfce_test_plugin_parent_class)->finalize) (object);
}



static void
xfce_test_plugin_construct (XfcePanelPlugin *plugin)
{
  g_message ("construct plugin %s", xfce_panel_plugin_get_name (plugin));
}



static void
xfce_test_plugin_free_data (XfcePanelPlugin *plugin)
{
  g_message ("free data");
}



static void
xfce_test_plugin_orientation_changed (XfcePanelPlugin *plugin,
                                      GtkOrientation   orientation)
{
  g_message ("orientation changed");
}



static gboolean
xfce_test_plugin_size_changed (XfcePanelPlugin *plugin,
                               gint             size)
{
  g_message ("size changed: %d px", size);

  return TRUE;
}



static void
xfce_test_plugin_save (XfcePanelPlugin *plugin)
{
  g_message ("save the plugin");
}



static void
xfce_test_plugin_configure_plugin (XfcePanelPlugin *plugin)
{
  g_message ("configure the plugin");
}



static void
xfce_test_plugin_screen_position_changed (XfcePanelPlugin *plugin,
                                          gint             position)
{
  g_message ("screen position changed: %d", position);
}
