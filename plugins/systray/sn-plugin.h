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

#ifndef __SN_PLUGIN_H__
#define __SN_PLUGIN_H__

#include "sn-backend.h"
#include "sn-config.h"

#ifdef ENABLE_X11
#include "systray-manager.h"
#endif

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SN_TYPE_PLUGIN (sn_plugin_get_type ())
G_DECLARE_FINAL_TYPE (SnPlugin, sn_plugin, SN, PLUGIN, XfcePanelPlugin)

struct _SnPlugin
{
  XfcePanelPlugin __parent__;

#ifdef ENABLE_X11
  /* Systray manager */
  SystrayManager *manager;
#endif

  guint idle_startup;
  gboolean has_hidden_systray_items;
  gboolean has_hidden_sn_items;

  /* Widgets */
  GtkWidget *box;
  GtkWidget *systray_box;
  GtkWidget *button;
  GtkWidget *item;
  GtkWidget *sn_box;

  /* Systray settings */
  GSList *names_ordered;
  GHashTable *names_hidden;

  GtkBuilder *configure_builder;

  /* Statusnotifier settings */
#ifdef HAVE_DBUSMENU
  SnBackend *backend;
#endif
  SnConfig *config;
};

void
sn_plugin_register_type (XfcePanelTypeModule *panel_type_module);

gboolean
sn_plugin_legacy_item_added (SnPlugin *plugin,
                             const gchar *name);

G_END_DECLS

#endif /* !__SN_PLUGIN_H__ */
