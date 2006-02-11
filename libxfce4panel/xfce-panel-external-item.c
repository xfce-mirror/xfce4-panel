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

#include <string.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>
#include <errno.h>

#include "xfce-panel-enum-types.h"
#include "xfce-panel-item-iface.h"
#include "xfce-panel-external-item.h"
#include "xfce-panel-plugin-messages.h"

#define XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_EXTERNAL_PANEL_ITEM, \
                                  XfceExternalPanelItemPrivate))


typedef struct _XfceExternalPanelItemPrivate XfceExternalPanelItemPrivate;

struct _XfceExternalPanelItemPrivate
{
    char *name;
    char *id;
    char *display_name;
    guint expand:1;
    int size;
    XfceScreenPosition screen_position;

    /* detect problems */
    guint to_be_removed:1;
};

/* prototypes */

/* item interface */
static void xfce_external_panel_item_interface_init (gpointer g_iface,
                                                     gpointer data);

static G_CONST_RETURN char *
    xfce_external_panel_item_get_name (XfcePanelItem * item);

static G_CONST_RETURN char *
    xfce_external_panel_item_get_id (XfcePanelItem * item);

static G_CONST_RETURN char *
    xfce_external_panel_item_get_display_name (XfcePanelItem * item);

static gboolean xfce_external_panel_item_get_expand (XfcePanelItem * item);

static void xfce_external_panel_item_free_data (XfcePanelItem * item);

static void xfce_external_panel_item_save (XfcePanelItem * item);

static void xfce_external_panel_item_set_size (XfcePanelItem * item,
                                               int size);

static void
    xfce_external_panel_item_set_screen_position (XfcePanelItem * item,
                                                  XfceScreenPosition position);

static void xfce_external_panel_item_set_sensitive (XfcePanelItem *item,
                                                    gboolean sensitive);

static void xfce_external_panel_item_remove (XfcePanelItem * item);

static void xfce_external_panel_item_configure (XfcePanelItem * item);


/* GObject */
static void xfce_external_panel_item_finalize (GObject * object);

/* GtkWidget */
static gboolean xfce_external_panel_item_button_press_event (GtkWidget *widget,
                                                           GdkEventButton *ev);

/* internal functions */
static gboolean _item_event_received (XfceExternalPanelItem * item,
                                      GdkEventClient * ev);

static void _item_construct (XfceExternalPanelItem * item);

static void _item_setup (XfceExternalPanelItem * item, const char *file);


/* type definition and initialization */

G_DEFINE_TYPE_EXTENDED (XfceExternalPanelItem, xfce_external_panel_item,
        GTK_TYPE_SOCKET, 0,
        G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_ITEM,
                               xfce_external_panel_item_interface_init));

static void
xfce_external_panel_item_interface_init (gpointer g_iface, gpointer data)
{
    XfcePanelItemInterface *iface = g_iface;

    iface->get_name            = xfce_external_panel_item_get_name;
    iface->get_id              = xfce_external_panel_item_get_id;
    iface->get_display_name    = xfce_external_panel_item_get_display_name;
    iface->get_expand          = xfce_external_panel_item_get_expand;
    iface->free_data           = xfce_external_panel_item_free_data;
    iface->save                = xfce_external_panel_item_save;
    iface->set_size            = xfce_external_panel_item_set_size;
    iface->set_screen_position = xfce_external_panel_item_set_screen_position;
    iface->set_sensitive       = xfce_external_panel_item_set_sensitive;
    iface->remove              = xfce_external_panel_item_remove;
    iface->configure           = xfce_external_panel_item_configure;
}

static void
xfce_external_panel_item_class_init (XfceExternalPanelItemClass * klass)
{
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;

    g_type_class_add_private (klass, sizeof (XfceExternalPanelItemPrivate));

    object_class = (GObjectClass *) klass;
    widget_class = (GtkWidgetClass *) klass;

    object_class->finalize = xfce_external_panel_item_finalize;

    widget_class->button_press_event = 
        xfce_external_panel_item_button_press_event;
}

static void
xfce_external_panel_item_init (XfceExternalPanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (item);

    priv->name = NULL;
    priv->id = NULL;
    priv->display_name = NULL;
    priv->expand = FALSE;
    priv->size = 0;
    priv->screen_position = XFCE_SCREEN_POSITION_NONE;
    priv->to_be_removed = FALSE;
}

/* GObject */
static void
xfce_external_panel_item_finalize (GObject * object)
{
    XfceExternalPanelItemPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (object);

    if (!priv->to_be_removed)
    {
        /* dialogs are annoying, just spit out a warning */
        g_critical (_("An item was unexpectedly removed: \"%s\"."),
                    priv->display_name);
    }

    g_free (priv->name);
    g_free (priv->id);
    g_free (priv->display_name);

    G_OBJECT_CLASS (xfce_external_panel_item_parent_class)->finalize (object);
}

/* Widget */
static gboolean 
xfce_external_panel_item_button_press_event (GtkWidget *widget,
                                             GdkEventButton *ev)
{
    guint modifiers;

    modifiers = gtk_accelerator_get_default_mod_mask ();

    if (ev->button == 3 || (ev->button == 1 && 
        (ev->state & modifiers) == GDK_CONTROL_MASK))
    {
        gdk_pointer_ungrab (ev->time);
        gdk_keyboard_ungrab (ev->time);

        xfce_panel_plugin_message_send (widget->window,
                GDK_WINDOW_XID (GTK_SOCKET (widget)->plug_window),
                XFCE_PANEL_PLUGIN_POPUP_MENU, 0);

        return TRUE;
    }

    return FALSE;
}

/* item interface */
static G_CONST_RETURN char *
xfce_external_panel_item_get_name (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_val_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item), NULL);

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    return priv->name;
}

static G_CONST_RETURN char *
xfce_external_panel_item_get_id (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_val_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item), NULL);

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    return priv->id;
}

static G_CONST_RETURN char *
xfce_external_panel_item_get_display_name (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_val_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item), NULL);

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    return priv->display_name;
}

gboolean
xfce_external_panel_item_get_expand (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_val_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item), FALSE);

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    return priv->expand;
}

static void
xfce_external_panel_item_free_data (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    priv->to_be_removed = TRUE;

    xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
            GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
            XFCE_PANEL_PLUGIN_FREE_DATA, 0);
}

static void
xfce_external_panel_item_save (XfcePanelItem * item)
{
    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    if (!GDK_IS_WINDOW (GTK_SOCKET (item)->plug_window))
        return;

    xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
            GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
            XFCE_PANEL_PLUGIN_SAVE, 0);
}

static void
xfce_external_panel_item_set_size (XfcePanelItem * item, int size)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    if (size != priv->size)
    {
        priv->size = size;

        xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
                GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
                XFCE_PANEL_PLUGIN_SIZE, size);
    }
}

static void
xfce_external_panel_item_set_screen_position (XfcePanelItem * item,
                                              XfceScreenPosition position)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    priv->screen_position = position;

    xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
            GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
            XFCE_PANEL_PLUGIN_SCREEN_POSITION, position);
}

static void
delayed_set_sensitive(XfcePanelItem * item, gpointer sensitive)
{
    xfce_external_panel_item_set_sensitive (item, 
                                            GPOINTER_TO_INT (sensitive));
    
    g_signal_handlers_disconnect_by_func(item, 
                                         G_CALLBACK(delayed_set_sensitive), 
                                         sensitive);
}

static void
xfce_external_panel_item_set_sensitive (XfcePanelItem * item, 
                                        gboolean sensitive)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    if (GDK_IS_WINDOW (GTK_SOCKET (item)->plug_window))
    {
        xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
                GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
                XFCE_PANEL_PLUGIN_SENSITIVE, sensitive ? 1 : 0);
    }
    else
    {
        g_signal_connect (item, "plug-added", 
                          G_CALLBACK(delayed_set_sensitive),
                          GINT_TO_POINTER (sensitive));
    }
}


static void 
xfce_external_panel_item_remove (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
            GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
            XFCE_PANEL_PLUGIN_REMOVE, 0);
}

static void
delayed_configure(XfcePanelItem *item)
{
    xfce_external_panel_item_configure (item);

    g_signal_handlers_disconnect_by_func (item,
                                          G_CALLBACK (delayed_configure), 
                                          NULL);
}

static void 
xfce_external_panel_item_configure (XfcePanelItem * item)
{
    XfceExternalPanelItemPrivate *priv;

    g_return_if_fail (XFCE_IS_EXTERNAL_PANEL_ITEM (item));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    if (GDK_IS_WINDOW (GTK_SOCKET (item)->plug_window))
    {
        xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
                GDK_WINDOW_XID (GTK_SOCKET (item)->plug_window),
                XFCE_PANEL_PLUGIN_CUSTOMIZE, 0);
    }
    else
    {
        g_signal_connect (item, "plug-added", 
                          G_CALLBACK(delayed_configure), NULL);
    }
}

/* internal functions */
static gboolean
_item_event_received (XfceExternalPanelItem * item, GdkEventClient * ev)
{
    GdkAtom atom = gdk_atom_intern (XFCE_PANEL_PLUGIN_ATOM, FALSE);

    if (ev->message_type == atom)
    {
        XfceExternalPanelItemPrivate *priv;

        priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (item);

        switch (ev->data.s[0])
        {
            case XFCE_PANEL_PLUGIN_REMOVE:
                priv->to_be_removed = TRUE;
                xfce_panel_item_free_data (XFCE_PANEL_ITEM (item));
                break;
            case XFCE_PANEL_PLUGIN_EXPAND:
                xfce_panel_item_expand_changed (XFCE_PANEL_ITEM (item),
                                                ev->data.s[1]);
                break;
            case XFCE_PANEL_PLUGIN_CUSTOMIZE:
                xfce_panel_item_customize_panel (XFCE_PANEL_ITEM (item));
                break;
            case XFCE_PANEL_PLUGIN_CUSTOMIZE_ITEMS:
                xfce_panel_item_customize_items (XFCE_PANEL_ITEM (item));
                break;
            case XFCE_PANEL_PLUGIN_MOVE:
                xfce_panel_item_move (XFCE_PANEL_ITEM (item));
                break;
            case XFCE_PANEL_PLUGIN_MENU_DEACTIVATED:
                xfce_panel_item_menu_deactivated (XFCE_PANEL_ITEM (item));
                break;
            case XFCE_PANEL_PLUGIN_POPUP_MENU:
                xfce_panel_item_menu_opened (XFCE_PANEL_ITEM (item));
                break;
            default:
                DBG ("Unknown message: %d", ev->data.s[0]);
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

static void
_item_construct (XfceExternalPanelItem * item)
{
    GtkSocket *socket = GTK_SOCKET (item);

    xfce_panel_plugin_message_send (GTK_WIDGET (item)->window,
                                    GDK_WINDOW_XID (socket->plug_window),
                                    XFCE_PANEL_PLUGIN_CONSTRUCT, 0);
}

static void
_item_setup (XfceExternalPanelItem * item, const char *file)
{
    char **argv = NULL;
    gulong sock;
    XfceExternalPanelItemPrivate *priv;

    g_signal_handlers_disconnect_by_func (item, G_CALLBACK (_item_setup),
                                          (gpointer) file);

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (
                XFCE_EXTERNAL_PANEL_ITEM (item));

    sock = gtk_socket_get_id (GTK_SOCKET (item));

    argv = g_new (char *, 8);

    argv[0] = g_strdup (file);
    argv[1] = g_strdup_printf ("socket_id=%ld", sock);
    argv[2] = g_strdup_printf ("name=%s", priv->name);
    argv[3] = g_strdup_printf ("id=%s", priv->id);
    argv[4] = g_strdup_printf ("display_name=%s", priv->display_name);
    argv[5] = g_strdup_printf ("size=%d", priv->size);
    argv[6] = g_strdup_printf ("screen_position=%d", priv->screen_position);
    argv[7] = NULL;

    g_signal_connect (item, "plug-added", G_CALLBACK (_item_construct), NULL);

    g_signal_connect (item, "client-event",
                      G_CALLBACK (_item_event_received), NULL);

    switch (fork())
    {
        case -1:
            g_critical ("Could not run plugin: %s", g_strerror (errno));
            gtk_widget_destroy (GTK_WIDGET (item));
            break;
        case 0:
            execv (argv[0], argv);
            g_critical ("Could not run plugin: %s", g_strerror (errno));
            gtk_widget_destroy (GTK_WIDGET (item));
            _exit (1);
        default:
            /* parent: do nothing */;
    }
    
    g_strfreev (argv);
}

/* public API */

/**
 * xfce_external_panel_item_new
 * @name         : plugin name
 * @id           : unique identifier string
 * @display_name : translated name for the plugin
 * @file         : plugin file name
 * @size         : panel size
 * @position     : panel screen position
 *
 * Used by the Xfce Panel to create a new item that is implemented as an
 * external plugin.
 *
 * Returns: a newly created #GtkWidget
 **/
GtkWidget *
xfce_external_panel_item_new (const char *name,
                              const char *id,
                              const char *display_name,
                              const char *file,
                              int size, XfceScreenPosition position)
{
    GtkWidget *item;
    XfceExternalPanelItemPrivate *priv;

    item = GTK_WIDGET (g_object_new (XFCE_TYPE_EXTERNAL_PANEL_ITEM, NULL));

    priv = XFCE_EXTERNAL_PANEL_ITEM_GET_PRIVATE (item);

    priv->name = g_strdup (name);
    priv->id = g_strdup (id);
    priv->display_name = g_strdup (display_name);
    priv->size = size;
    priv->screen_position = position;

    g_signal_connect (item, "realize", G_CALLBACK (_item_setup),
                      (gpointer) file);

    return item;
}

