/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_EXTERNAL_PANEL_ITEM_H
#define _XFCE_EXTERNAL_PANEL_ITEM_H

#include <gtk/gtksocket.h>
#include <libxfce4panel/xfce-panel-enums.h>

#define XFCE_TYPE_EXTERNAL_PANEL_ITEM            (xfce_external_panel_item_get_type ())
#define XFCE_EXTERNAL_PANEL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_EXTERNAL_PANEL_ITEM, XfceExternalPanelItem))
#define XFCE_EXTERNAL_PANEL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_EXTERNAL_PANEL_ITEM, XfceExternalPanelItemClass))
#define XFCE_IS_EXTERNAL_PANEL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_EXTERNAL_PANEL_ITEM))
#define XFCE_IS_EXTERNAL_PANEL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_EXTERNAL_PANEL_ITEM))
#define XFCE_EXTERNAL_PANEL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_EXTERNAL_PANEL_ITEM, XfceExternalPanelItemClass))


G_BEGIN_DECLS 

typedef struct _XfceExternalPanelItem XfceExternalPanelItem;
typedef struct _XfceExternalPanelItemClass XfceExternalPanelItemClass;

struct _XfceExternalPanelItem
{
    GtkSocket parent;
};

struct _XfceExternalPanelItemClass
{
    GtkSocketClass parent_class;
};


GType xfce_external_panel_item_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_external_panel_item_new (const char *name,
                                         const char *id,
                                         const char *display_name,
                                         const char *file,
                                         int size,
                                         XfceScreenPosition position);

G_END_DECLS

#endif /* _XFCE_EXTERNAL_PANEL_ITEM_H */
