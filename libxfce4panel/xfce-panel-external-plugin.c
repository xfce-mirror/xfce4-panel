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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "xfce-panel-enum-types.h"
#include "xfce-panel-plugin-iface.h"
#include "xfce-panel-plugin-iface-private.h"
#include "xfce-panel-external-plugin.h"
#include "xfce-panel-plugin-messages.h"

#define XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFCE_TYPE_EXTERNAL_PANEL_PLUGIN, \
                                  XfceExternalPanelPluginPrivate))

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


typedef struct _XfceExternalPanelPluginPrivate XfceExternalPanelPluginPrivate;

struct _XfceExternalPanelPluginPrivate
{
    char *name;
    char *id;
    char *display_name;

    int size;
    XfceScreenPosition screen_position;

    gboolean expand;

    XfcePanelPluginFunc construct;

    gulong socket_id;
};

/* prototypes */

/* initialization */
static void xfce_external_panel_plugin_interface_init (gpointer g_iface,
                                                       gpointer data);

static void
xfce_external_panel_plugin_class_init (XfceExternalPanelPluginClass * class);

static void 
xfce_external_panel_plugin_init (XfceExternalPanelPlugin * plugin);


/* GObject */
static void xfce_external_panel_plugin_finalize (GObject * object);

static void xfce_external_panel_plugin_get_property (GObject * object,
                                                     guint prop_id,
                                                     GValue * value,
                                                     GParamSpec * pspec);

static void xfce_external_panel_plugin_set_property (GObject * object,
                                                     guint prop_id,
                                                     const GValue * value,
                                                     GParamSpec * pspec);


/* plugin interface */
static void xfce_external_panel_plugin_remove (XfcePanelPlugin * plugin);

static void xfce_external_panel_plugin_set_expand (XfcePanelPlugin * plugin,
                                                   gboolean expand);

static void 
xfce_external_panel_plugin_customize_panel (XfcePanelPlugin * plugin);

static void 
xfce_external_panel_plugin_customize_items (XfcePanelPlugin * plugin);

static void 
xfce_external_panel_plugin_new_panel (XfcePanelPlugin * plugin);

static void 
xfce_external_panel_plugin_remove_panel (XfcePanelPlugin * plugin);

static void 
xfce_external_panel_plugin_about_panel (XfcePanelPlugin * plugin);


/* properties */
static void xfce_external_panel_plugin_set_name (XfceExternalPanelPlugin *
                                                 plugin, const char *name);

static void xfce_external_panel_plugin_set_id (XfceExternalPanelPlugin *
                                               plugin, const char *id);

static void
xfce_external_panel_plugin_set_display_name (XfceExternalPanelPlugin * plugin,
                                             const char *display_name);

static void xfce_external_panel_plugin_set_size (XfceExternalPanelPlugin *
                                                 plugin, int size);

static void
xfce_external_panel_plugin_set_screen_position (XfceExternalPanelPlugin *
                                                plugin,
                                                XfceScreenPosition position);

/* messages */
static void xfce_external_panel_plugin_construct (XfceExternalPanelPlugin *
                                                  plugin);

static void xfce_external_panel_plugin_save (XfceExternalPanelPlugin *
                                             plugin);

static void xfce_external_panel_plugin_free_data (XfceExternalPanelPlugin *
                                                  plugin);


/* internal functions */
static void _plugin_menu_deactivated (GtkWidget * menu,
                                      XfceExternalPanelPlugin * plugin);

static gboolean _plugin_event_received (GtkWidget * win, GdkEventClient * ev,
                                        XfceExternalPanelPlugin * plugin);


/* type definition and initialization */
G_DEFINE_TYPE_EXTENDED (XfceExternalPanelPlugin,
                        xfce_external_panel_plugin,
                        GTK_TYPE_PLUG, 0,
                        G_IMPLEMENT_INTERFACE (XFCE_TYPE_PANEL_PLUGIN,
                                               xfce_external_panel_plugin_interface_init));

static void
xfce_external_panel_plugin_interface_init (gpointer g_iface, gpointer data)
{
    XfcePanelPluginInterface *iface = g_iface;

    iface->remove = xfce_external_panel_plugin_remove;
    iface->set_expand = xfce_external_panel_plugin_set_expand;
    iface->customize_panel = xfce_external_panel_plugin_customize_panel;
    iface->customize_items = xfce_external_panel_plugin_customize_items;
    iface->new_panel = xfce_external_panel_plugin_new_panel;
    iface->remove_panel = xfce_external_panel_plugin_remove_panel;
    iface->about_panel = xfce_external_panel_plugin_about_panel;
}

static void
xfce_external_panel_plugin_class_init (XfceExternalPanelPluginClass * klass)
{
    GObjectClass *object_class;

    g_type_class_add_private (klass, sizeof (XfceExternalPanelPluginPrivate));

    object_class = (GObjectClass *) klass;

    object_class->finalize = xfce_external_panel_plugin_finalize;
    object_class->get_property = xfce_external_panel_plugin_get_property;
    object_class->set_property = xfce_external_panel_plugin_set_property;

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
xfce_external_panel_plugin_init (XfceExternalPanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    priv->name = NULL;
    priv->id = NULL;
    priv->display_name = NULL;

    priv->size = 0;
    priv->screen_position = XFCE_SCREEN_POSITION_NONE;

    priv->expand = FALSE;

    priv->construct = NULL;

    priv->socket_id = 0;
}

/* GObject */

static void
xfce_external_panel_plugin_finalize (GObject * object)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (object);

    g_free (priv->name);
    g_free (priv->id);
    g_free (priv->display_name);

    G_OBJECT_CLASS (xfce_external_panel_plugin_parent_class)->
        finalize (object);
}

static void
xfce_external_panel_plugin_get_property (GObject * object, guint prop_id,
                                         GValue * value, GParamSpec * pspec)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (object);

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
xfce_external_panel_plugin_set_property (GObject * object, guint prop_id,
                                         const GValue * value,
                                         GParamSpec * pspec)
{
    switch (prop_id)
    {
        case PROP_NAME:
            xfce_external_panel_plugin_set_name (XFCE_EXTERNAL_PANEL_PLUGIN
                                                 (object),
                                                 g_value_get_string (value));
            break;
        case PROP_ID:
            xfce_external_panel_plugin_set_id (XFCE_EXTERNAL_PANEL_PLUGIN
                                               (object),
                                               g_value_get_string (value));
            break;
        case PROP_DISPLAY_NAME:
            xfce_external_panel_plugin_set_display_name
                (XFCE_EXTERNAL_PANEL_PLUGIN (object),
                 g_value_get_string (value));
        case PROP_SIZE:
            xfce_external_panel_plugin_set_size (XFCE_EXTERNAL_PANEL_PLUGIN
                                                 (object),
                                                 g_value_get_int (value));
            break;
        case PROP_SCREEN_POSITION:
            xfce_external_panel_plugin_set_screen_position
                (XFCE_EXTERNAL_PANEL_PLUGIN (object),
                 g_value_get_enum (value));
            break;
        case PROP_EXPAND:
            xfce_external_panel_plugin_set_expand (XFCE_PANEL_PLUGIN (object),
                                                   g_value_get_boolean
                                                   (value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* plugin interface */
static void
xfce_external_panel_plugin_remove (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;
    char *file;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    file = xfce_panel_plugin_save_location (plugin, FALSE);
    if (file)
    {
        unlink (file);
        g_free (file);
    }
    
    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id, XFCE_PANEL_PLUGIN_REMOVE,
                                    0);
}

static void
xfce_external_panel_plugin_set_expand (XfcePanelPlugin * plugin,
                                       gboolean expand)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    if (expand != priv->expand)
    {
        priv->expand = expand;

        xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                        priv->socket_id,
                                        XFCE_PANEL_PLUGIN_EXPAND, expand);
    }
}


static void
xfce_external_panel_plugin_customize_panel (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_CUSTOMIZE, 0);
}

static void
xfce_external_panel_plugin_customize_items (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_CUSTOMIZE_ITEMS, 0);
}

static void
xfce_external_panel_plugin_new_panel (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_NEW_PANEL, 0);
}

static void
xfce_external_panel_plugin_remove_panel (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_REMOVE_PANEL, 0);
}

static void
xfce_external_panel_plugin_about_panel (XfcePanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_ABOUT_PANEL, 0);
}


/* item/plugin interaction */
static void
xfce_external_panel_plugin_construct (XfceExternalPanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    if (priv->construct)
        priv->construct (XFCE_PANEL_PLUGIN (plugin));

    /* this should be run here to make sure the default size function
     * is called when the plugin doesn't handle this signal */
    xfce_panel_plugin_signal_size (XFCE_PANEL_PLUGIN (plugin), priv->size);
}

static void
xfce_external_panel_plugin_save (XfceExternalPanelPlugin * plugin)
{
    xfce_panel_plugin_signal_save (XFCE_PANEL_PLUGIN (plugin));
}

static void
xfce_external_panel_plugin_free_data (XfceExternalPanelPlugin * plugin)
{
    xfce_panel_plugin_signal_free_data (XFCE_PANEL_PLUGIN (plugin));

    gtk_widget_destroy (GTK_WIDGET (plugin));
}

/* properties */
static void
xfce_external_panel_plugin_set_name (XfceExternalPanelPlugin * plugin,
                                     const char *name)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    g_free (priv->name);
    priv->name = g_strdup (name);
}

static void
xfce_external_panel_plugin_set_id (XfceExternalPanelPlugin * plugin,
                                   const char *id)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    g_free (priv->id);
    priv->id = g_strdup (id);
}

static void
xfce_external_panel_plugin_set_display_name (XfceExternalPanelPlugin * plugin,
                                             const char *name)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    g_free (priv->display_name);
    priv->display_name = g_strdup (name);
}

static void
xfce_external_panel_plugin_set_screen_position (XfceExternalPanelPlugin *
                                                plugin,
                                                XfceScreenPosition position)
{
    XfceExternalPanelPluginPrivate *priv;
    gboolean orientation_changed;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

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
xfce_external_panel_plugin_set_size (XfceExternalPanelPlugin * plugin,
                                     int size)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    if (size != priv->size)
    {
        priv->size = size;

        xfce_panel_plugin_signal_size (XFCE_PANEL_PLUGIN (plugin), size);
    }
}


/* internal functions */
static void
_plugin_menu_deactivated (GtkWidget * menu, XfceExternalPanelPlugin * plugin)
{
    XfceExternalPanelPluginPrivate *priv;

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    xfce_panel_plugin_message_send (GTK_WIDGET (plugin)->window,
                                    priv->socket_id,
                                    XFCE_PANEL_PLUGIN_MENU_DEACTIVATED, 0);
}

static gboolean
_plugin_event_received (GtkWidget * win, GdkEventClient * ev,
                        XfceExternalPanelPlugin * plugin)
{
    GdkAtom atom = gdk_atom_intern (XFCE_PANEL_PLUGIN_ATOM, FALSE);

    if (ev->message_type == atom)
    {
        switch (ev->data.s[0])
        {
            case XFCE_PANEL_PLUGIN_CONSTRUCT:
                xfce_external_panel_plugin_construct (plugin);
                break;
            case XFCE_PANEL_PLUGIN_FREE_DATA:
                xfce_external_panel_plugin_free_data (plugin);
                break;
            case XFCE_PANEL_PLUGIN_SAVE:
                xfce_external_panel_plugin_save (plugin);
                break;
            case XFCE_PANEL_PLUGIN_SIZE:
                xfce_external_panel_plugin_set_size (plugin, ev->data.s[1]);
                break;
            case XFCE_PANEL_PLUGIN_SCREEN_POSITION:
                xfce_external_panel_plugin_set_screen_position (plugin,
                                                                ev->data.
                                                                s[1]);
                break;
            case XFCE_PANEL_PLUGIN_POPUP_MENU:
                xfce_panel_plugin_popup_menu (XFCE_PANEL_PLUGIN (plugin));
                break;
            case XFCE_PANEL_PLUGIN_SENSITIVE:
                xfce_panel_plugin_set_sensitive (XFCE_PANEL_PLUGIN (plugin), 
                                                 ev->data.s[1] == 1);
                break;
            case XFCE_PANEL_PLUGIN_REMOVE:
                xfce_panel_plugin_remove_confirm (XFCE_PANEL_PLUGIN (plugin));
                break;
            default:
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

/* public API */

/* Required arguments:
 * - socket_id
 * - name
 * - id
 * - display_name
 * - size
 * - screen_position
 */
#define REQUIRED_ARGS  6

/**
 * xfce_external_panel_plugin_new
 * @argc      : number of arguments (from the plugins main () function)
 * @argv      : argument array (from the plugins main () function)
 * @construct : #XfcePanelPluginFunc that will be called to construct the
 *              plugin widgets.
 *
 * Creates a new external plugin. This function should not be used directly
 * but only through the XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL() macro.
 *
 * Returns: newly created panel plugin widget.
 **/
GtkWidget *
xfce_external_panel_plugin_new (int argc, char **argv,
                                XfcePanelPluginFunc construct)
{
    XfcePanelPlugin *plugin;
    XfceExternalPanelPluginPrivate *priv;
    int i;
    char *key, *value, *p;

    if (argc < 1 + REQUIRED_ARGS)
        return NULL;

    plugin = g_object_new (XFCE_TYPE_EXTERNAL_PANEL_PLUGIN, NULL);

    priv = XFCE_EXTERNAL_PANEL_PLUGIN_GET_PRIVATE (plugin);

    priv->construct = construct;
    priv->socket_id = 0;

    for (i = 1; i < argc; ++i)
    {
        if (!(p = strchr (argv[i], '=')))
            continue;

        key = argv[i];
        *p = '\0';
        value = ++p;

        if (!priv->socket_id && strcmp ("socket_id", key) == 0)
        {
            priv->socket_id = strtol (value, NULL, 0);
        }
        else if (!priv->name && strcmp ("name", key) == 0)
        {
            priv->name = g_strdup (value);
        }
        else if (!priv->id && strcmp ("id", key) == 0)
        {
            priv->id = g_strdup (value);
        }
        else if (!priv->display_name && strcmp ("display_name", key) == 0)
        {
            priv->display_name = g_strdup (value);
        }
        else if (!priv->size && strcmp ("size", key) == 0)
        {
            int size = (int) strtol (value, NULL, 0);
            
            priv->size = size;
        }
        else if (!priv->screen_position &&
                 strcmp ("screen_position", key) == 0)
        {
            priv->screen_position =
                (XfceScreenPosition) strtol (value, NULL, 0);
        }
    }

    if (priv->name && priv->id && priv->socket_id > 0)
    {
        gtk_plug_construct (GTK_PLUG (plugin), priv->socket_id);

        gtk_widget_show (GTK_WIDGET (plugin));

        /* add menu */
        xfce_panel_plugin_create_menu (plugin, 
                                       G_CALLBACK (_plugin_menu_deactivated));

        xfce_panel_plugin_add_action_widget (XFCE_PANEL_PLUGIN (plugin),
                                             GTK_WIDGET (plugin));

        /* messages */
        g_signal_connect (plugin, "client-event",
                          G_CALLBACK (_plugin_event_received), plugin);

        return GTK_WIDGET (plugin);
    }

    /* plugin creation failed */
    gtk_widget_destroy (GTK_WIDGET (plugin));

    return NULL;
}
