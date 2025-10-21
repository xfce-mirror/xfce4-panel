#ifndef __LAUNCHER_ITEM_INFO_H__
#define __LAUNCHER_ITEM_INFO_H__

#include <garcon/garcon.h>

gchar *
launcher_get_item_name (GarconMenuItem *item);

gchar *
launcher_get_item_name_markup (GarconMenuItem *item);

GIcon *
launcher_get_item_icon (GarconMenuItem *item);

gchar *
launcher_get_item_tooltip (GarconMenuItem *item);

#endif
