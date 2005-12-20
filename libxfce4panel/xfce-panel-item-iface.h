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

#ifndef _XFCE_PANEL_ITEM_H
#define _XFCE_PANEL_ITEM_H

#include <glib-object.h>
#include <libxfce4panel/xfce-panel-enums.h>

#define XFCE_TYPE_PANEL_ITEM                (xfce_panel_item_get_type ())
#define XFCE_PANEL_ITEM(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_ITEM, XfcePanelItem))
#define XFCE_IS_PANEL_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_ITEM))
#define XFCE_PANEL_ITEM_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), XFCE_TYPE_PANEL_ITEM, XfcePanelItemInterface))


G_BEGIN_DECLS

typedef struct _XfcePanelItem XfcePanelItem;    /* dummy object */
typedef struct _XfcePanelItemInterface XfcePanelItemInterface;

struct _XfcePanelItemInterface
{
    GTypeInterface parent;

    /* vtable */
    G_CONST_RETURN char *(*get_name)         (XfcePanelItem *item);

    G_CONST_RETURN char *(*get_id)           (XfcePanelItem *item);

    G_CONST_RETURN char *(*get_display_name) (XfcePanelItem *item);

    gboolean (*get_expand)                   (XfcePanelItem *item);

    void (*free_data)                        (XfcePanelItem *item);
    
    void (*save)                             (XfcePanelItem *item);

    void (*set_size)                         (XfcePanelItem *item, int size);

    void (*set_screen_position)              (XfcePanelItem *item, 
                                              XfceScreenPosition position);

    void (*set_sensitive)                    (XfcePanelItem *item,
                                              gboolean sensitive);

    void (*remove)                           (XfcePanelItem *item);

    /* reserved for future expansion */
    void (*_panel_reserved1) (void);
    void (*_panel_reserved2) (void);
};

GType xfce_panel_item_get_type (void) G_GNUC_CONST;


/* emit signals -- to be called from implementors */

void xfce_panel_item_expand_changed (XfcePanelItem *item, gboolean expand);

void xfce_panel_item_menu_deactivated (XfcePanelItem *item);

void xfce_panel_item_menu_opened (XfcePanelItem *item);

void xfce_panel_item_customize_panel (XfcePanelItem *item);

void xfce_panel_item_customize_items (XfcePanelItem *item);

void xfce_panel_item_move (XfcePanelItem *item);


/* vtable */

G_CONST_RETURN char *xfce_panel_item_get_name (XfcePanelItem *item);

G_CONST_RETURN char *xfce_panel_item_get_id (XfcePanelItem *item);

G_CONST_RETURN char *xfce_panel_item_get_display_name (XfcePanelItem *item);

gboolean xfce_panel_item_get_expand (XfcePanelItem *item);

void xfce_panel_item_save (XfcePanelItem *item);

void xfce_panel_item_free_data (XfcePanelItem *item);

void xfce_panel_item_set_size (XfcePanelItem *item, 
                               int size);

void xfce_panel_item_set_screen_position (XfcePanelItem *item, 
                                          XfceScreenPosition position);

void xfce_panel_item_set_sensitive (XfcePanelItem *item,
                                    gboolean sensitive);

void xfce_panel_item_remove (XfcePanelItem *item);


G_END_DECLS

#endif /* _XFCE_PANEL_ITEM_H */
