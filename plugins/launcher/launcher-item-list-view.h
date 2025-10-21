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
