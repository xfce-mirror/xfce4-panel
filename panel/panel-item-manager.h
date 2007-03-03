/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFCE_PANEL_ITEM_MANAGER_H__
#define __XFCE_PANEL_ITEM_MANAGER_H__

#include <gtk/gtkwidget.h>
#include <libxfce4panel/xfce-panel-enums.h>

G_BEGIN_DECLS

typedef struct _XfcePanelItemInfo XfcePanelItemInfo;

struct _XfcePanelItemInfo
{
    char *name;
    char *display_name;
    char *comment;
    GdkPixbuf *icon;
};

void xfce_panel_item_manager_init (void);

void xfce_panel_item_manager_cleanup (void);

GtkWidget *xfce_panel_item_manager_create_item (GdkScreen *screen,
                                                const char *name,
                                                const char *id,
                                                int size,
                                                XfceScreenPosition position);

/* for 'Add Item' dialog */
GPtrArray *xfce_panel_item_manager_get_item_info_list (void);

void xfce_panel_item_manager_free_item_info_list (GPtrArray *info_list);

gboolean xfce_panel_item_manager_is_available (const char *name);


#endif /* !__XFCE_PANEL_ITEM_MANAGER_H__ */
