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

G_END_DECLS

#endif /* !__LAUNCHER_ITEM_LIST_MODEL_H__ */
