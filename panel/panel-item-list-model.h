/*
 * Copyright (C) 2025 Dmitry Petrachkov <dmitry-petrachkov@outlook.com>
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

#ifndef __PANEL_ITEM_LIST_MODEL_H__
#define __PANEL_ITEM_LIST_MODEL_H__

#include "panel-module.h"
#include "panel-window.h"

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define PANEL_TYPE_ITEM_LIST_MODEL (panel_item_list_model_get_type ())
G_DECLARE_FINAL_TYPE (PanelItemListModel, panel_item_list_model, PANEL, ITEM_LIST_MODEL, XfceItemListModel)

enum
{
  PANEL_ITEM_LIST_MODEL_COLUMN_ABOUT = XFCE_ITEM_LIST_MODEL_COLUMN_USER,
  PANEL_ITEM_LIST_MODEL_N_COLUMNS,
};

XfceItemListModel *
panel_item_list_model_new (PanelWindow *panel);

PanelWindow *
panel_item_list_model_get_panel (PanelItemListModel *model);

XfcePanelPluginProvider *
panel_item_list_model_get_item_provider (PanelItemListModel *model,
                                         gint index);

G_END_DECLS

#endif /* !__PANEL_ITEM_LIST_MODEL_H__ */
