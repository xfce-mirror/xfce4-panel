/*
 *  Copyright (c) 2013 Andrzej Radecki <andrzejr@xfce.org>
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

#ifndef __SN_CONFIG_H__
#define __SN_CONFIG_H__

#include "libxfce4panel/xfce-panel-plugin.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SN_TYPE_CONFIG (sn_config_get_type ())
G_DECLARE_FINAL_TYPE (SnConfig, sn_config, SN, CONFIG, GObject)

#define DEFAULT_ICON_SIZE 22
#define DEFAULT_SINGLE_ROW FALSE
#define DEFAULT_SQUARE_ICONS FALSE
#define DEFAULT_SYMBOLIC_ICONS FALSE
#define DEFAULT_MENU_IS_PRIMARY FALSE
#define DEFAULT_ORIENTATION GTK_ORIENTATION_HORIZONTAL
#define DEFAULT_PANEL_ORIENTATION GTK_ORIENTATION_HORIZONTAL
#define DEFAULT_PANEL_SIZE 28
#define DEFAULT_HIDE_NEW_ITEMS FALSE

typedef enum _SnItemType
{
  SN_ITEM_TYPE_DEFAULT,
  SN_ITEM_TYPE_LEGACY,
  N_SN_ITEM_TYPES,
} SnItemType;

SnConfig *
sn_config_new (XfcePanelPlugin *panel_plugin);

void
sn_config_set_orientation (SnConfig *config,
                           GtkOrientation panel_orientation,
                           GtkOrientation orientation);

GtkOrientation
sn_config_get_orientation (SnConfig *config);

GtkOrientation
sn_config_get_panel_orientation (SnConfig *config);

void
sn_config_set_size (SnConfig *config,
                    gint panel_size,
                    gint nrows,
                    gint icon_size);

gint
sn_config_get_nrows (SnConfig *config);

gint
sn_config_get_panel_size (SnConfig *config);

gboolean
sn_config_get_single_row (SnConfig *config);

gboolean
sn_config_get_square_icons (SnConfig *config);

gboolean
sn_config_get_symbolic_icons (SnConfig *config);

gboolean
sn_config_get_menu_is_primary (SnConfig *config);

gint
sn_config_get_icon_size (SnConfig *config);

gboolean
sn_config_get_icon_size_is_automatic (SnConfig *config);

void
sn_config_get_dimensions (SnConfig *config,
                          gint *ret_icon_size,
                          gint *ret_n_rows,
                          gint *ret_row_size,
                          gint *ret_padding);

gboolean
sn_config_is_hidden (SnConfig *config,
                     SnItemType type,
                     const gchar *name);

void
sn_config_set_hidden (SnConfig *config,
                      SnItemType type,
                      const gchar *name,
                      gboolean filtered);

GList *
sn_config_get_known_items (SnConfig *config,
                           SnItemType type);

gboolean
sn_config_add_known_item (SnConfig *config,
                          SnItemType type,
                          const gchar *name);

GList *
sn_config_get_hidden_legacy_items (SnConfig *config);

void
sn_config_swap_known_items (SnConfig *config,
                            SnItemType type,
                            const gchar *name1,
                            const gchar *name2);
gboolean
sn_config_items_clear (SnConfig *config);
gboolean
sn_config_legacy_items_clear (SnConfig *config);

G_END_DECLS

#endif /* !__SN_CONFIG_H__ */
