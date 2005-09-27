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
#include <unistd.h>
#include <gtk/gtk.h>

#include "xfce-panel-enum-types.h"
#include "xfce-panel-internal-plugin.h"
#include "xfce-panel-item-iface.h"
#include "xfce-panel-plugin-iface-private.h"
#include "xfce-panel-plugin-iface.h"

#define XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_INTERNAL_PANEL_PLUGIN, \
                                  XfceInternalPanelPluginPrivate))


enum
{
    PROP_0,
    PROP_NAME,
    PROP_ID,
    PROP_DISPLAY_NAME,
    PROP_SIZE,
    PROP_SCREEN_POSITION,
    PROP_EXPAND
};


typedef struct _XfceInternalPanelPluginPrivate XfceInternalPanelPluginPrivate;

struct _XfceInternalPanelPluginPrivate
{
    char *name;
    char *id;
    char *display_name;
    int size;
    XfceScreenPosition screen_position;
    guint expand:1;
};


/* prototypes */

/* item interface */
static void xfce_internal_panel_plugin_init_item_interface (gpointer g_iface,
                                                            gpointer data);

static G_CONST_RETURN char *
    xfce_internal_panel_plugin_get_name (XfcePanelItem * plugin);

static G_CONST_RETURN char *
    xfce_internal_panel_plugin_get_id (XfcePanelItem * plugin);

static G_CONST_RETURN char *
    xfce_internal_panel_plugin_get_display_name (XfcePanelItem * plugin);

static gboolean xfce_internal_panel_plugin_get_expand (XfcePanelItem * item);

static void xfce_internal_panel_plugin_free_data (XfcePanelItem * plugin);

static void xfce_internal_panel_plugin_save (XfcePanelItem * plugin);

static void xfce_internal_panel_plugin_set_size (XfcePanelItem * plugin,
                                                 int size);

static void
xfce_internal_panel_plugin_set_screen_position (XfcePanelItem * plugin,
                                                XfceScreenPosition position);

static void
    xfce_internal_panel_plugin_set_sensitive (XfcePanelItem * plugin,
                                              gboolean sensitive);

/* plugin interface */
static void
    xfce_internal_panel_plugin_init_plugin_interface (gpointer g_iface,
                                                      gpointer data);

static void xfce_internal_panel_plugin_remove (XfcePanelPlugin * plugin);

static void xfce_internal_panel_plugin_set_expand (XfcePanelPlugin * plugin,
                                                   gboolean expand);

static void xfce_internal_panel_plugin_customize_panel (XfcePanelPlugin *
                                                        plugin);

static void xfce_internal_panel_plugin_customize_items (XfcePanelPlugin *
                                                        plugin);

static void xfce_internal_panel_plugin_new_panel (XfcePanelPlugin * plugin);

static void xfce_internal_panel_plugin_remove_panel (XfcePanelPlugin * plugin);

static void xfce_internal_panel_plugin_about_panel (XfcePanelPlugin * plugin);


/* properties */
static void xfce_internal_panel_plugin_set_name (XfcePanelPlugin * plugin,
                                                 const char *name);

static void xfce_internal_panel_plugin_set_id (XfcePanelPlugin * plugin,
                                               const char *id);

static void 
    xfce_internal_panel_plugin_set_display_name (XfcePanelPlugin * plugin,
                                                 const char *name);


/* GObject */
static void xfce_internal_panel_plugin_finalize (GObject * object);

static void xfce_internal_panel_plugin_get_property (GObject * object,
                                                     guint prop_id,
                                                     GValue * value,
                                                     GParamSpec * pspec);

static void xfce_internal_panel_plugin_set_property (GObject * object,
                                                     guint prop_id,
                                                     const GValue * value,
                                                     GParamSpec * pspec);


/* internal functions */
static void _plugin_menu_deactivated (GtkWidget * menu, XfcePanelItem * item);


/* type definition and initialization */

G_DEFINE_TYPE_EXTENDED (XfceInternalPanelPlugin, xfce_internal_panel_plugin,
        GTK_TYPE_EVENT_BOX, 0,
        G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_ITEM,
            xfce_internal_panel_plugin_init_item_interface)
        G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN,
            xfce_internal_panel_plugin_init_plugin_interface));

static void
xfce_internal_panel_plugin_init_item_interface (gpointer g_iface,
                                                gpointer data)
{
    XfcePanelItemInterface *iface = g_iface;

    iface->get_name = xfce_internal_panel_plugin_get_name;
    iface->get_id = xfce_internal_panel_plugin_get_id;
    iface->get_display_name = xfce_internal_panel_plugin_get_display_name;
    iface->get_expand = xfce_internal_panel_plugin_get_expand;
    iface->free_data = xfce_internal_panel_plugin_free_data;
    iface->save = xfce_internal_panel_plugin_save;
    iface->set_size = xfce_internal_panel_plugin_set_size;
    iface->set_screen_position =
        xfce_internal_panel_plugin_set_screen_position;
    iface->set_sensitive = xfce_internal_panel_plugin_set_sensitive;
}

static void
xfce_internal_panel_plugin_init_plugin_interface (gpointer g_iface,
                                                  gpointer data)
{
    XfcePanelPluginInterface *iface = g_iface;

    iface->remove = xfce_internal_panel_plugin_remove;
    iface->set_expand = xfce_internal_panel_plugin_set_expand;
    iface->customize_panel = xfce_internal_panel_plugin_customize_panel;
    iface->customize_items = xfce_internal_panel_plugin_customize_items;
    iface->new_panel = xfce_internal_panel_plugin_new_panel;
    iface->remove_panel = xfce_internal_panel_plugin_remove_panel;
    iface->about_panel = xfce_internal_panel_plugin_about_panel;
}

static void
xfce_internal_panel_plugin_class_init (XfceInternalPanelPluginClass * klass)
{
    GObjectClass *object_class;

    g_type_class_add_private (klass, sizeof (XfceInternalPanelPluginPrivate));

    object_class = (GObjectClass *) klass;

    object_class->finalize = xfce_internal_panel_plugin_finalize;
    object_class->get_property = xfce_internal_panel_plugin_get_property;
    object_class->set_property = xfce_internal_panel_plugin_set_property;

    /* properties */

    g_object_class_override_property (object_class, PROP_NAME, "name");

    g_object_class_override_property (object_class, PROP_ID, "id");

    g_object_class_override_property (object_class, PROP_DISPLAY_NAME,
                                      "display-name");

    g_object_class_override_property (object_class, PROP_SIZE, "size");

    g_object_class_override_property (object_class, PROP_SCREEN_POSITION,
                                      "screen-position");

    g_object_class_override_property (object_class, PROP_EXPAND, "expand");
}

static void
xfce_internal_panel_plugin_init (XfceInternalPanelPlugin * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    priv->name = NULL;
    priv->id = NULL;
    priv->display_name = NULL;
    priv->size = 0;
    priv->screen_position = XFCE_SCREEN_POSITION_NONE;
    priv->expand = FALSE;
}

/* GObject */
static void
xfce_internal_panel_plugin_finalize (GObject * object)
{
    XfceInternalPanelPluginPrivate *priv;

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (object);

    g_free (priv->name);
    g_free (priv->id);
    g_free (priv->display_name);

    G_OBJECT_CLASS (xfce_internal_panel_plugin_parent_class)->
        finalize (object);
}

static void
xfce_internal_panel_plugin_get_property (GObject * object, guint prop_id,
                                         GValue * value, GParamSpec * pspec)
{
    XfceInternalPanelPluginPrivate *priv;

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (object);

    switch (prop_id)
    {
        case PROP_NAME:
            g_value_set_static_string (value, priv->name);
            break;
        case PROP_ID:
            g_value_set_static_string (value, priv->id);
            break;
        case PROP_DISPLAY_NAME:
            g_value_set_static_string (value, priv->display_name);
            break;
        case PROP_SIZE:
            g_value_set_int (value, priv->size);
            break;
        case PROP_SCREEN_POSITION:
            g_value_set_enum (value, priv->screen_position);
            break;
        case PROP_EXPAND:
            g_value_set_boolean (value, priv->expand);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
xfce_internal_panel_plugin_set_property (GObject * object, guint prop_id,
                                         const GValue * value,
                                         GParamSpec * pspec)
{
    XfceInternalPanelPluginPrivate *priv;

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (object);

    switch (prop_id)
    {
        case PROP_EXPAND:
            xfce_internal_panel_plugin_set_expand (XFCE_PANEL_PLUGIN (object),
                                                   g_value_get_boolean
                                                   (value));
            break;
        case PROP_NAME:
            xfce_internal_panel_plugin_set_name (XFCE_PANEL_PLUGIN (object),
                                                 g_value_get_string (value));
            break;
        case PROP_ID:
            xfce_internal_panel_plugin_set_id (XFCE_PANEL_PLUGIN (object),
                                               g_value_get_string (value));
            break;
        case PROP_DISPLAY_NAME:
            xfce_internal_panel_plugin_set_display_name (XFCE_PANEL_PLUGIN
                                                         (object),
                                                         g_value_get_string
                                                         (value));
            break;
        case PROP_SIZE:
            xfce_internal_panel_plugin_set_size (XFCE_PANEL_ITEM (object),
                                                 g_value_get_int (value));
            break;
        case PROP_SCREEN_POSITION:
            xfce_internal_panel_plugin_set_screen_position (XFCE_PANEL_ITEM
                                                            (object),
                                                            g_value_get_enum
                                                            (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* item interface */
static G_CONST_RETURN char *
xfce_internal_panel_plugin_get_name (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_val_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin), NULL);

    priv =
        XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (XFCE_INTERNAL_PANEL_PLUGIN
                                                (plugin));

    return priv->name;
}

static G_CONST_RETURN char *
xfce_internal_panel_plugin_get_id (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_val_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin), NULL);

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    return priv->id;
}

static G_CONST_RETURN char *
xfce_internal_panel_plugin_get_display_name (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_val_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin), NULL);

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    return priv->display_name;
}

static gboolean
xfce_internal_panel_plugin_get_expand (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_val_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin), FALSE);

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    return priv->expand;
}

static void
xfce_internal_panel_plugin_free_data (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    xfce_panel_plugin_signal_free_data (XFCE_PANEL_PLUGIN (plugin));

    gtk_widget_destroy (GTK_WIDGET (plugin));
}

static void
xfce_internal_panel_plugin_save (XfcePanelItem * plugin)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    xfce_panel_plugin_signal_save (XFCE_PANEL_PLUGIN (plugin));
}

static void
xfce_internal_panel_plugin_set_screen_position (XfcePanelItem * plugin,
                                                XfceScreenPosition position)
{
    XfceInternalPanelPluginPrivate *priv;
    gboolean orientation_changed;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    orientation_changed =
        (xfce_screen_position_is_horizontal (position) !=
         xfce_screen_position_is_horizontal (priv->screen_position));

    priv->screen_position = position;

    xfce_panel_plugin_signal_screen_position (XFCE_PANEL_PLUGIN (plugin),
                                              position);

    if (orientation_changed)
        xfce_panel_plugin_signal_orientation (XFCE_PANEL_PLUGIN (plugin),
                xfce_screen_position_get_orientation (position));

    xfce_panel_plugin_signal_size (XFCE_PANEL_PLUGIN (plugin), priv->size);
}

static void
xfce_internal_panel_plugin_set_size (XfcePanelItem * plugin, int size)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    if (size != priv->size)
    {
        priv->size = size;

        xfce_panel_plugin_signal_size (XFCE_PANEL_PLUGIN (plugin), size);
    }
}

static void
xfce_internal_panel_plugin_set_sensitive (XfcePanelItem * plugin, 
                                          gboolean sensitive)
{
    xfce_panel_plugin_set_sensitive (XFCE_PANEL_PLUGIN (plugin), sensitive);
}

/* plugin interface */
static void
xfce_internal_panel_plugin_remove (XfcePanelPlugin * plugin)
{
    char *file;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    file = xfce_panel_plugin_save_location (plugin);
    if (file)
    {
        unlink (file);
        g_free (file);
    }

    xfce_internal_panel_plugin_free_data (XFCE_PANEL_ITEM (plugin));
}

static void
xfce_internal_panel_plugin_set_expand (XfcePanelPlugin * plugin,
                                       gboolean expand)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    if (expand != priv->expand)
    {
        priv->expand = expand;

        xfce_panel_item_expand_changed (XFCE_PANEL_ITEM (plugin), expand);
    }
}

static void
xfce_internal_panel_plugin_customize_panel (XfcePanelPlugin * plugin)
{
    xfce_panel_item_customize_panel (XFCE_PANEL_ITEM (plugin));
}

static void
xfce_internal_panel_plugin_customize_items (XfcePanelPlugin * plugin)
{
    xfce_panel_item_customize_items (XFCE_PANEL_ITEM (plugin));
}

static void
xfce_internal_panel_plugin_new_panel (XfcePanelPlugin * plugin)
{
    xfce_panel_item_new_panel (XFCE_PANEL_ITEM (plugin));
}

static void
xfce_internal_panel_plugin_remove_panel (XfcePanelPlugin * plugin)
{
    xfce_panel_item_remove_panel (XFCE_PANEL_ITEM (plugin));
}

static void
xfce_internal_panel_plugin_about_panel (XfcePanelPlugin * plugin)
{
    xfce_panel_item_about_panel (XFCE_PANEL_ITEM (plugin));
}


/* properties */
static void
xfce_internal_panel_plugin_set_name (XfcePanelPlugin * plugin,
                                     const char *name)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    g_free (priv->name);
    priv->name = g_strdup (name);
}

static void
xfce_internal_panel_plugin_set_id (XfcePanelPlugin * plugin, const char *id)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    g_free (priv->id);
    priv->id = g_strdup (id);
}

static void
xfce_internal_panel_plugin_set_display_name (XfcePanelPlugin * plugin,
                                             const char *name)
{
    XfceInternalPanelPluginPrivate *priv;

    g_return_if_fail (XFCE_IS_INTERNAL_PANEL_PLUGIN (plugin));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (
                XFCE_INTERNAL_PANEL_PLUGIN (plugin));

    g_free (priv->display_name);
    priv->display_name = g_strdup (name);
}

/* internal functions */
static void
_plugin_menu_deactivated (GtkWidget * menu, XfcePanelItem * item)
{
    xfce_panel_item_menu_deactivated (item);
}

/* public API */

/**
 * xfce_internal_panel_plugin_new
 * @name         : plugin name
 * @id           : unique identifier string
 * @display_name : translated plugin name
 * @size         : panel size
 * @position     : panel screen position
 * @construct    : #XfcePanelPluginFunc that will be called to construct the
 *                 plugin widgets.
 *
 * Creates a new internal plugin. This function should not be used directly,
 * but only throught the XFCE_PANEL_PLUGIN_REGISTER_INTERNAL () macro.
 *
 * Returns: Newly created panel plugin widget.
 **/
GtkWidget *
xfce_internal_panel_plugin_new (const char *name,
                                const char *id,
                                const char *display_name,
                                int size,
                                XfceScreenPosition position,
                                XfcePanelPluginFunc construct)
{
    GtkWidget *plugin;
    XfceInternalPanelPluginPrivate *priv;

    plugin =
        GTK_WIDGET (g_object_new (XFCE_TYPE_INTERNAL_PANEL_PLUGIN, NULL));

    priv = XFCE_INTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    priv->name = g_strdup (name);
    priv->id = g_strdup (id);
    priv->display_name = g_strdup (display_name);
    priv->screen_position = position;
    priv->size = size;

    /* add menu */
    xfce_panel_plugin_create_menu (XFCE_PANEL_PLUGIN (plugin),
                                   G_CALLBACK (_plugin_menu_deactivated));

    xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin),
                                         GTK_WIDGET (plugin));

    construct (XFCE_PANEL_PLUGIN (plugin));

    /* this should be run here to make sure the default size function
     * is called when the plugin doesn't handle this signal */
    xfce_panel_plugin_signal_size (XFCE_PANEL_PLUGIN (plugin), priv->size);
    
    return plugin;
}
