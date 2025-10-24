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

#ifndef __LAUNCHER_ITEM_LIST_MODEL_H__
#define __LAUNCHER_ITEM_LIST_MODEL_H__

#include "launcher.h"

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ITEM_LIST_MODEL (launcher_item_list_model_get_type ())
G_DECLARE_FINAL_TYPE (LauncherItemListModel, launcher_item_list_model, LAUNCHER, ITEM_LIST_MODEL, XfceItemListModel)

XfceItemListModel *
launcher_item_list_model_new (LauncherPlugin *plugin);

GarconMenuItem *
launcher_item_list_model_get_item (LauncherItemListModel *model,
                                   gint index);

void
launcher_item_list_model_insert (LauncherItemListModel *model,
                                 gint index,
                                 GList *items);

gchar *
launcher_item_list_model_get_item_name_text (GarconMenuItem *item);

gchar *
launcher_item_list_model_get_item_name_markup (GarconMenuItem *item);

GIcon *
launcher_item_list_model_get_item_icon (GarconMenuItem *item);

gchar *
launcher_item_list_model_get_item_tooltip (GarconMenuItem *item);

G_END_DECLS

#endif /* !__LAUNCHER_ITEM_LIST_MODEL_H__ */
