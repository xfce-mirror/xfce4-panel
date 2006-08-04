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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "xfce-panel-item-iface.h"

enum
{
    EXPAND_CHANGED,
    MENU_DEACTIVATED,
    MENU_OPENED,
    CUSTOMIZE_PANEL,
    CUSTOMIZE_ITEMS,
    MOVE,
    SET_HIDDEN,
    LAST_SIGNAL
};

static guint xfce_panel_item_signals[LAST_SIGNAL] = { 0 };

/* interface definition and initialization */

static void
xfce_panel_item_base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (!initialized)
    {
        GType ptypes[1];
        
        /* signals (note: there are no class closures here) */
        ptypes[0] = G_TYPE_BOOLEAN;
        
        /**
         * XfcePanelItem::expand-changed
         * @item   : #XfcePanelItem
         * @expand : whether the item should expand with the panel size
         *
         * The signal is emitted when a plugin requests a different
         * expand behaviour.
         **/
        xfce_panel_item_signals [EXPAND_CHANGED] =
            g_signal_newv ("expand-changed",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__BOOLEAN, 
                           G_TYPE_NONE, 1, ptypes);

        /**
         * XfcePanelItem::menu-deactivated
         * @item   : #XfcePanelItem
         *
         * The signal is emitted when a plugin menu is deactivated.
         **/
        xfce_panel_item_signals [MENU_DEACTIVATED] =
            g_signal_newv ("menu-deactivated",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID, 
                           G_TYPE_NONE, 0, NULL);

        /**
         * XfcePanelItem::menu-opened
         * @item   : #XfcePanelItem
         *
         * The signal is emitted when a plugin menu is opened.
         **/
        xfce_panel_item_signals [MENU_OPENED] =
            g_signal_newv ("menu-opened",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID, 
                           G_TYPE_NONE, 0, NULL);

        /**
         * XfcePanelItem::customize-panel
         * @item   : #XfcePanelItem
         *
         * The signal is emitted when a plugin requests the settings dialog to
         * be shown.
         **/
        xfce_panel_item_signals [CUSTOMIZE_PANEL] =
            g_signal_newv ("customize-panel",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID, 
                           G_TYPE_NONE, 0, NULL);

        /**
         * XfcePanelItem::customize-items
         * @item   : #XfcePanelItem
         *
         * The signal is emitted when a plugin requests the 'Add Items' dialog.
         **/
        xfce_panel_item_signals [CUSTOMIZE_ITEMS] =
            g_signal_newv ("customize-items",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID, 
                           G_TYPE_NONE, 0, NULL);

        /**
         * XfcePanelItem::move
         * @item   : #XfcePanelItem
         *
         * The signal is emitted when a plugin requests the item to be moved.
         **/
        xfce_panel_item_signals [MOVE] =
            g_signal_newv ("move",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID, 
                           G_TYPE_NONE, 0, NULL);

        /**
         * XfcePanelItem::set-hidden
         * @item   : #XfcePanelItem
         * @hidden : %FALSE to unhide the panel, %TRUE to hide it
         *
         * The signal is emitted when a plugin requests the panel to unhide.
         **/
        xfce_panel_item_signals [SET_HIDDEN] =
            g_signal_newv ("set-hidden",
                           XFCE_TYPE_PANEL_ITEM,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | 
                                 G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__BOOLEAN, 
                           G_TYPE_NONE, 1, ptypes);

        initialized = TRUE;
    }
}

GType
xfce_panel_item_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        static const GTypeInfo info = 
        {
            sizeof (XfcePanelItemInterface),
            xfce_panel_item_base_init,  /* base_init */
            NULL,                       /* base_finalize */
            NULL,                       /* class_init */
            NULL,                       /* class_finalize */
            NULL,                       /* class_data */
            0,
            0,                          /* n_preallocs */
            NULL                        /* instance_init */
        };
        type = g_type_register_static (G_TYPE_INTERFACE, "XfcePanelItem", 
                                       &info, 0);
    }
    
    return type;
}

/* public API */

void
xfce_panel_item_focus_panel (XfcePanelItem *item)
{
    static Atom atom = 0;
    XClientMessageEvent xev;
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET(item));

    if (G_UNLIKELY (!atom))
        atom = XInternAtom (GDK_DISPLAY (), "_NET_ACTIVE_WINDOW", FALSE);

    xev.type         = ClientMessage;
    xev.window       = GDK_WINDOW_XID (toplevel->window);
    xev.message_type = atom;
    xev.format       = 32;
    xev.data.l[0]    = 0;
    xev.data.l[1]    = 0;
    xev.data.l[2]    = 0;
    xev.data.l[3]    = 0;
    xev.data.l[4]    = 0;

    XSendEvent (GDK_DISPLAY (), GDK_ROOT_WINDOW (), False,
                StructureNotifyMask, (XEvent *) & xev);
    gdk_flush();
}

/* signals */

/**
 * xfce_panel_item_expand_changed
 * @item   : #XfcePanelItem
 * @expand : %TRUE if item should be expanded
 *
 * Emits the "expand-changed" signal on the item. Should only be called by
 * item implementations.
 **/
void 
xfce_panel_item_expand_changed (XfcePanelItem *item, gboolean expand)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[EXPAND_CHANGED], 0, expand, NULL);
}

/**
 * xfce_panel_item_menu_deactivated
 * @item   : #XfcePanelItem
 *
 * Emits the "menu-deactivated" signal on the item. Should only be called by
 * item implementations.
 **/
void 
xfce_panel_item_menu_deactivated (XfcePanelItem *item)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[MENU_DEACTIVATED], 0, NULL);
}

/**
 * xfce_panel_item_menu_opened
 * @item   : #XfcePanelItem
 *
 * Emits the "menu-opened" signal on the item. Should only be called by
 * item implementations.
 **/
void 
xfce_panel_item_menu_opened (XfcePanelItem *item)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[MENU_OPENED], 0, NULL);
}

/**
 * xfce_panel_item_customize_panel
 * @item   : #XfcePanelItem
 *
 * Emits the "customize-panel" signal on the item. Should only be called by
 * item implementations.
 **/
void
xfce_panel_item_customize_panel (XfcePanelItem *item)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[CUSTOMIZE_PANEL], 0, NULL);
}

/**
 * xfce_panel_item_customize_items
 * @item   : #XfcePanelItem
 *
 * Emits the "customize-items" signal on the item. Should only be called by
 * item implementations.
 **/
void
xfce_panel_item_customize_items (XfcePanelItem *item)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[CUSTOMIZE_ITEMS], 0, NULL);
}

/**
 * xfce_panel_item_move
 * @item   : #XfcePanelItem
 *
 * Emits the "move" signal on the item. Should only be called by
 * item implementations.
 **/
void
xfce_panel_item_move (XfcePanelItem *item)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[MOVE], 0, NULL);
}

/**
 * xfce_panel_item_set_panel_hidden
 * @item   : #XfcePanelItem
 *
 * Emits the "set-hidden" signal on the item. Should only be called by
 * item implementations.
 **/
void 
xfce_panel_item_set_panel_hidden (XfcePanelItem *item, gboolean hidden)
{
    g_return_if_fail (XFCE_IS_PANEL_ITEM (item));

    g_signal_emit (item, xfce_panel_item_signals[SET_HIDDEN], 0, hidden, NULL);
}

/* properties */

/**
 * xfce_panel_item_get_name
 * @item : #XfcePanelItem
 *
 * Returns: plugin name.
 **/
G_CONST_RETURN char *
xfce_panel_item_get_name (XfcePanelItem *item)
{
    return XFCE_PANEL_ITEM_GET_INTERFACE (item)->get_name (item);
}

/**
 * xfce_panel_item_get_id
 * @item : #XfcePanelItem
 *
 * Returns: unique identifier string for the item.
 **/
G_CONST_RETURN char *
xfce_panel_item_get_id (XfcePanelItem *item)
{
    return XFCE_PANEL_ITEM_GET_INTERFACE (item)->get_id (item);
}

/**
 * xfce_panel_item_get_display_name
 * @item : #XfcePanelItem
 *
 * Returns: translated plugin name.
 **/
G_CONST_RETURN char *
xfce_panel_item_get_display_name (XfcePanelItem *item)
{
    return XFCE_PANEL_ITEM_GET_INTERFACE (item)->get_display_name (item);
}

/**
 * xfce_panel_item_get_expand
 * @item : #XfcePanelItem
 *
 * Returns: whether @item will expand when the panel size increases.
 **/
gboolean 
xfce_panel_item_get_expand (XfcePanelItem *item)
{
    return XFCE_PANEL_ITEM_GET_INTERFACE (item)->get_expand (item);
}

/**
 * xfce_panel_item_save
 * @item : #XfcePanelItem
 *
 * Ask the item to save its settings.
 **/
void
xfce_panel_item_save (XfcePanelItem * item)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->save (item);
}

/**
 * xfce_panel_item_free_data
 * @item : #XfcePanelItem
 *
 * Tell the item it should free allocated data.
 **/
void
xfce_panel_item_free_data (XfcePanelItem * item)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->free_data (item);
}

/**
 * xfce_panel_item_set_size
 * @item : #XfcePanelItem
 * @size : new panel size
 *
 * Notifies the item of a change in panel size.
 **/
void
xfce_panel_item_set_size (XfcePanelItem * item, int size)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->set_size (item, size);
}

/**
 * xfce_panel_item_set
 * @item     : #XfcePanelItem
 * @position : new panel screen position
 *
 * Notifies the item of a change in panel screen position.
 **/
void
xfce_panel_item_set_screen_position (XfcePanelItem * item,
                                XfceScreenPosition position)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->set_screen_position (item, position);
}

/*
 * xfce_panel_item_set_sensitive
 * @item      : an #XfcePanelItem
 * @sensitive : whether to make the widget sensitive or not
 *
 * Ask the item to become (in)sensitive.
 **/
void
xfce_panel_item_set_sensitive (XfcePanelItem * item, gboolean sensitive)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->set_sensitive (item, sensitive);
}

/*
 * xfce_panel_item_remove
 * @item : an #XfcePanelItem
 *
 * Remove @item from the panel.
 **/
void 
xfce_panel_item_remove (XfcePanelItem *item)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->remove (item);
}

/*
 * xfce_panel_item_configure
 * @item : an #XfcePanelItem
 *
 * Ask @item to open a settings dialog.
 **/
void xfce_panel_item_configure (XfcePanelItem *item)
{
    XFCE_PANEL_ITEM_GET_INTERFACE (item)->configure (item);
}
