/*
 *  Copyright (c) 2017 Viktor Odintsev <ninetls@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>
#include <common/panel-utils.h>
#include <common/panel-debug.h>

#include "systray.h"
#include "systray-box.h"
#include "systray-socket.h"
#include "systray-manager.h"

#include "sn-backend.h"
#include "sn-box.h"
#include "sn-button.h"
#include "sn-dialog.h"
#include "sn-item.h"
#include "sn-plugin.h"

#define BUTTON_SIZE   (16)


static void                  sn_plugin_construct                     (XfcePanelPlugin         *panel_plugin);

static void                  sn_plugin_free                          (XfcePanelPlugin         *panel_plugin);

static gboolean              sn_plugin_size_changed                  (XfcePanelPlugin         *panel_plugin,
                                                                      gint                     size);

static void                  sn_plugin_mode_changed                  (XfcePanelPlugin         *panel_plugin,
                                                                      XfcePanelPluginMode      mode);

static void                  sn_plugin_configure_plugin              (XfcePanelPlugin         *panel_plugin);

static void                  sn_plugin_show_about                    (XfcePanelPlugin         *panel_plugin);





XFCE_PANEL_DEFINE_PLUGIN (SnPlugin, sn_plugin)



static void
sn_plugin_class_init (SnPluginClass *klass)
{
  XfcePanelPluginClass *panel_plugin_class;

  panel_plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  panel_plugin_class->construct = sn_plugin_construct;
  panel_plugin_class->free_data = sn_plugin_free;
  panel_plugin_class->size_changed = sn_plugin_size_changed;
  panel_plugin_class->mode_changed = sn_plugin_mode_changed;
  panel_plugin_class->configure_plugin = sn_plugin_configure_plugin;
  panel_plugin_class->about = sn_plugin_show_about;
}



static void
sn_plugin_init (SnPlugin *plugin)
{
  /* Systray init */
  plugin->manager = NULL;
  plugin->idle_startup = 0;
  plugin->names_ordered = NULL;
  plugin->names_hidden = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Statusnotifier init */
  plugin->item = NULL;
  plugin->systray_box = NULL;
  plugin->backend = NULL;
  plugin->config = NULL;
}



static void
sn_plugin_free (XfcePanelPlugin *panel_plugin)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (panel_plugin);

  /* Statusnotifier */
  /* remove children so they won't use unrefed SnItems and SnConfig */
  gtk_container_remove (GTK_CONTAINER (panel_plugin), plugin->systray_box);

  g_object_unref (plugin->backend);
  g_object_unref (plugin->config);

  /* Systray */
  /* stop pending idle startup */
  if (plugin->idle_startup != 0)
    g_source_remove (plugin->idle_startup);

  /* disconnect screen changed signal */
  //g_signal_handlers_disconnect_by_func (G_OBJECT (plugin),
  //    systray_plugin_screen_changed, NULL);

  g_slist_free_full (plugin->names_ordered, g_free);
  g_hash_table_destroy (plugin->names_hidden);

  if (G_LIKELY (plugin->manager != NULL))
    {
      systray_manager_unregister (plugin->manager);
      g_object_unref (G_OBJECT (plugin->manager));
    }
}



static gboolean
sn_plugin_size_changed (XfcePanelPlugin *panel_plugin,
                        gint             size)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (panel_plugin);

  sn_config_set_size (plugin->config, size, xfce_panel_plugin_get_nrows (panel_plugin));

  return TRUE;
}



static void
sn_plugin_mode_changed (XfcePanelPlugin     *panel_plugin,
                        XfcePanelPluginMode  mode)
{
  SnPlugin       *plugin = XFCE_SN_PLUGIN (panel_plugin);
  GtkOrientation  orientation;
  GtkOrientation  panel_orientation;

  panel_orientation = xfce_panel_plugin_get_orientation (panel_plugin);
  orientation = mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL
                ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;

  sn_config_set_orientation (plugin->config, panel_orientation, orientation);

  sn_plugin_size_changed (panel_plugin, xfce_panel_plugin_get_size (panel_plugin));
}



static void
sn_plugin_configure_plugin (XfcePanelPlugin *panel_plugin)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (panel_plugin);
  SnDialog *dialog;

  dialog = sn_dialog_new (plugin->config, gtk_widget_get_screen (GTK_WIDGET (plugin)));
  if (dialog != NULL)
    {
      xfce_panel_plugin_block_menu (panel_plugin);
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify)xfce_panel_plugin_unblock_menu, panel_plugin);
    }
}



static void
sn_plugin_show_about (XfcePanelPlugin *panel_plugin)
{
  GdkPixbuf *icon;

  const gchar *auth[] =
    {
      "Viktor Odintsev <ninetls@xfce.org>",
      "Andrzej Radecki <andrzejr@xfce.org>",
      NULL
    };

  icon = xfce_panel_pixbuf_from_source ("xfce4-statusnotifier-plugin", NULL, 32);

  gtk_show_about_dialog (NULL,
                         "logo", icon,
                         "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                         "version", PACKAGE_VERSION,
                         "program-name", PACKAGE_NAME,
                         "comments", _("Provides a panel area for status notifier items (application indicators)"),
                         "website", "https://docs.xfce.org/panel-plugins/xfce4-statusnotifier-plugin",
                         "authors", auth,
                         NULL);

  if (icon)
    g_object_unref (icon);
}



static void
sn_plugin_item_added (SnPlugin *plugin,
                      SnItem   *item)
{
  GtkWidget *button;

  button = sn_button_new (item,
                          xfce_panel_plugin_position_menu, plugin,
                          plugin->config);

  sn_config_add_known_item (plugin->config, sn_item_get_name (item));

  gtk_container_add (GTK_CONTAINER (plugin->sn_box), button);
  gtk_widget_show (button);
}



static void
sn_plugin_item_removed (SnPlugin *plugin,
                        SnItem   *item)
{
  sn_box_remove_item (XFCE_SN_BOX (plugin->sn_box), item);
}



static void
sn_plugin_construct (XfcePanelPlugin *panel_plugin)
{
  SnPlugin *plugin = XFCE_SN_PLUGIN (panel_plugin);

  plugin->manager = NULL;
  plugin->idle_startup = 0;
  plugin->names_ordered = NULL;
  plugin->names_hidden = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  xfce_panel_plugin_menu_show_configure (panel_plugin);
  xfce_panel_plugin_menu_show_about (panel_plugin);

  plugin->config = sn_config_new (xfce_panel_plugin_get_property_base (panel_plugin));

  /* Container for both plugins */
  plugin->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (plugin), plugin->box);
  gtk_widget_show (plugin->box);

  /* Add systray box */
  plugin->systray_box = systray_box_new ();
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->systray_box, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (plugin->systray_box), "draw",
      G_CALLBACK (systray_plugin_box_draw), plugin);
  gtk_container_set_border_width (GTK_CONTAINER (plugin->systray_box), 1);
  gtk_widget_show (plugin->systray_box);

  /* Systray arrow button */
  plugin->button = xfce_arrow_button_new (GTK_ARROW_RIGHT);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (plugin->button), "toggled",
      G_CALLBACK (systray_plugin_button_toggled), plugin);
  gtk_button_set_relief (GTK_BUTTON (plugin->button), GTK_RELIEF_NONE);
  g_object_bind_property (G_OBJECT (plugin->systray_box), "has-hidden",
                          G_OBJECT (plugin->button), "visible",
                          G_BINDING_SYNC_CREATE);
  xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin), plugin->button);

  /* Add statusnotifier box */
  plugin->sn_box = sn_box_new (plugin->config);
  gtk_box_pack_start (GTK_BOX (plugin->box), plugin->sn_box, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (plugin->sn_box));

  g_signal_connect_swapped (plugin->config, "configuration-changed",
                            G_CALLBACK (gtk_widget_queue_resize), plugin->systray_box);

  plugin->backend = sn_backend_new ();
  g_signal_connect_swapped (plugin->backend, "item-added",
                            G_CALLBACK (sn_plugin_item_added), plugin);
  g_signal_connect_swapped (plugin->backend, "item-removed",
                            G_CALLBACK (sn_plugin_item_removed), plugin);
  sn_backend_start (plugin->backend);
}
