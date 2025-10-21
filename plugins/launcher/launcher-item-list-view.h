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

#ifndef __LAUNCHER_ITEM_LIST_VIEW_H__
#define __LAUNCHER_ITEM_LIST_VIEW_H__

#include "launcher-item-list-model.h"

G_BEGIN_DECLS

#define LAUNCHER_TYPE_ITEM_LIST_VIEW (launcher_item_list_view_get_type ())
G_DECLARE_FINAL_TYPE (LauncherItemListView, launcher_item_list_view, LAUNCHER, ITEM_LIST_VIEW, GtkBox)

void
launcher_item_list_view_set_model (LauncherItemListView *view,
                                   LauncherItemListModel *model);

void
launcher_item_list_view_append (LauncherItemListView *view,
                                GList *items);

G_END_DECLS

#endif /* !__LAUNCHER_ITEM_LIST_VIEW_H__ */
