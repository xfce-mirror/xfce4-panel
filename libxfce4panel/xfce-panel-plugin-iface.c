/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>

#include "xfce-panel-macros.h"
#include "xfce-panel-convenience.h"
#include "xfce-panel-plugin-iface.h"
#include "xfce-panel-plugin-iface-private.h"
#include "xfce-panel-enum-types.h"
#include "xfce-marshal.h"

enum
{
    SCREEN_POSITION_CHANGED,
    ORIENTATION_CHANGED,
    SIZE_CHANGED,
    FREE_DATA,
    SAVE,
    ABOUT,
    CONFIGURE,
    LAST_SIGNAL
};

static guint xfce_panel_plugin_signals[LAST_SIGNAL] = { 0 };



static void
xfce_panel_plugin_base_init (gpointer g_class)
{
    static gboolean initialized = FALSE;

    if (!initialized)
    {
        GType ptypes[1];

        /* signals (note: there are no class closures here) */

        /* screen position */
        ptypes[0] = XFCE_TYPE_SCREEN_POSITION;

        /**
         * XfcePanelPlugin::screen-position-changed
         * @plugin   : a #XfcePanelPlugin widget
         * @position : new #XfceScreenPosition of the panel
         *
         * Emitted when the screen position changes. Most plugins will be
         * more interested in the "orientation-changed" signal.
         **/
        xfce_panel_plugin_signals [SCREEN_POSITION_CHANGED] =
            g_signal_newv (I_("screen-position-changed"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__ENUM,
                           G_TYPE_NONE,
                           1, ptypes);

        /* orientation */
        ptypes[0] = GTK_TYPE_ORIENTATION;

        /**
         * XfcePanelPlugin::orientation-changed
         * @plugin      : a #XfcePanelPlugin widget
         * @orientation : new #GtkOrientation of the panel
         *
         * Emitted when the panel orientation changes.
         **/
        xfce_panel_plugin_signals [ORIENTATION_CHANGED] =
            g_signal_newv (I_("orientation-changed"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__ENUM,
                           G_TYPE_NONE,
                           1, ptypes);

        /* size */
        ptypes[0] = G_TYPE_INT;

        /**
         * XfcePanelPlugin::size-changed
         * @plugin      : a #XfcePanelPlugin widget
         * @size        : new panel size
         *
         * Emitted when the panel size changes.
         *
         * Returns: %TRUE when handled, %FALSE otherwise.
         **/
        xfce_panel_plugin_signals [SIZE_CHANGED] =
            g_signal_newv (I_("size-changed"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL,
                           g_signal_accumulator_true_handled,
                           NULL,
                           _xfce_marshal_BOOLEAN__INT,
                           G_TYPE_BOOLEAN,
                           1, ptypes);

        /**
         * XfcePanelPlugin::free-data
         * @plugin      : a #XfcePanelPlugin widget
         *
         * Emitted when the panel is closing. Plugin writers should connect to
         * this signal to free any allocated resources.
         *
         * See also: #XfcePanelPlugin::save
         **/
        xfce_panel_plugin_signals [FREE_DATA] =
            g_signal_newv (I_("free-data"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID,
                           G_TYPE_NONE,
                           0, NULL);

        /**
         * XfcePanelPlugin::save
         * @plugin      : a #XfcePanelPlugin widget
         *
         * Emitted before the panel is closing. May be called more than once
         * while the panel is running. Plugin writers should connect to
         * this signal to save the plugins configuration.
         *
         * See also: xfce_panel_plugin_get_rc_file()
         **/
        xfce_panel_plugin_signals [SAVE] =
            g_signal_newv (I_("save"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID,
                           G_TYPE_NONE,
                           0, NULL);

        /**
         * XfcePanelPlugin::about
         * @plugin      : a #XfcePanelPlugin widget
         *
         * Emitted when the 'About' menu item is clicked. Plugin writers
         * should connect to this signal to show information about their
         * plugin (and its authors).
         *
         * See also: xfce_panel_plugin_menu_show_about()
         **/
        xfce_panel_plugin_signals [ABOUT] =
            g_signal_newv (I_("about"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID,
                           G_TYPE_NONE,
                           0, NULL);

        /**
         * XfcePanelPlugin::configure-plugin
         * @plugin      : a #XfcePanelPlugin widget
         *
         * Emitted when the 'Configure' menu item is clicked. Plugin writers
         * should connect to this signal to show a settings dialog.
         *
         * See also: xfce_panel_plugin_menu_show_configure()
         **/
        xfce_panel_plugin_signals [CONFIGURE] =
            g_signal_newv (I_("configure-plugin"),
                           XFCE_TYPE_PANEL_PLUGIN,
                           G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                           NULL, NULL, NULL,
                           g_cclosure_marshal_VOID__VOID,
                           G_TYPE_NONE,
                           0, NULL);

        /* properties */

        /**
         * XfcePanelPlugin:name
         *
         * Untranslated plugin name. This identifies the plugin type and
         * therefore has to be unique.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_string ("name",
                                                                  "xfce_panel_plugin_name",
                                                                  "Plugin name",
                                                                  NULL,
                                                                  G_PARAM_READABLE));

        /**
         * XfcePanelPlugin:id
         *
         * Unique identifier string created for every #XfcePanelPlugin instance.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_string ("id",
                                                                  "xfce_panel_plugin_id",
                                                                  "Plugin id",
                                                                  NULL,
                                                                  G_PARAM_READABLE));

        /**
         * XfcePanelPlugin:display-name
         *
         * Translated plugin name. This is the name that can be presented to
         * the user, e.g. in dialogs or menus.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_string ("display-name",
                                                                  "xfce_panel_plugin_display_name",
                                                                  "Plugin display name",
                                                                  NULL,
                                                                  G_PARAM_READABLE));

        /**
         * XfcePanelPlugin:size
         *
         * The current panel size.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_int ("size",
                                                               "xfce_panel_plugin_size",
                                                               "Panel size",
                                                               10, 128, 32,
                                                               G_PARAM_READABLE));

        /**
         * XfcePanelPlugin:screen-position
         *
         * The current #XfceScreenPosition of the panel.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_enum ("screen-position",
                                                                "xfce_panel_plugin_screen_position",
                                                                "Panel screen position",
                                                                XFCE_TYPE_SCREEN_POSITION,
                                                                XFCE_SCREEN_POSITION_S,
                                                                G_PARAM_READABLE));

        /**
         * XfcePanelPlugin:expand
         *
         * Whether to expand the plugin when the panel width increases.
         **/
        g_object_interface_install_property (g_class,
                                             g_param_spec_boolean ("expand",
                                                                   "xfce_panel_plugin_expand",
                                                                   "Whether to expand the plugin",
                                                                   FALSE,
                                                                   G_PARAM_READWRITE));

        initialized = TRUE;
    }
}



GType
xfce_panel_plugin_get_type (void)
{
    static GType type = 0;

    if (type == 0)
    {
        static const GTypeInfo info = {
            sizeof (XfcePanelPluginInterface),
            xfce_panel_plugin_base_init,
            NULL,
            NULL,
            NULL,
            NULL,
            0,
            0,
            NULL,
            NULL
        };

        type = g_type_register_static (G_TYPE_INTERFACE, I_("XfcePanelPlugin"),
                                       &info, 0);

        g_type_interface_add_prerequisite(type, GTK_TYPE_CONTAINER);
    }

    return type;
}



/**
 * xfce_panel_plugin_signal_screen_position
 * @plugin   : an #XfcePanelPlugin
 * @position : the new #XfceScreenPosition of the panel.
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_screen_position (XfcePanelPlugin    *plugin,
                                          XfceScreenPosition  position)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[SCREEN_POSITION_CHANGED],
                   0, position);
}



/**
 * xfce_panel_plugin_signal_orientation
 * @plugin      : an #XfcePanelPlugin
 * @orientation : the new #GtkOrientation of the panel.
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_orientation (XfcePanelPlugin *plugin,
                                      GtkOrientation   orientation)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[ORIENTATION_CHANGED], 0,
                   orientation);
}



/**
 * xfce_panel_plugin_signal_orientation
 * @plugin      : an #XfcePanelPlugin
 * @size        : the new size of the panel.
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_size (XfcePanelPlugin *plugin,
                               gint             size)
{
    gboolean handled = FALSE;

    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[SIZE_CHANGED], 0,
                   size, &handled);

    if (!handled)
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, size);
}



/**
 * xfce_panel_plugin_signal_free_data
 * @plugin      : an #XfcePanelPlugin
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_free_data (XfcePanelPlugin *plugin)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[FREE_DATA], 0);
}



/**
 * xfce_panel_plugin_signal_save
 * @plugin      : an #XfcePanelPlugin
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_save (XfcePanelPlugin *plugin)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[SAVE], 0);
}



/**
 * xfce_panel_plugin_signal_about
 * @plugin      : an #XfcePanelPlugin
 *
 * Should be called by implementations of the interface.
 **/
static void
xfce_panel_plugin_signal_about (XfcePanelPlugin *plugin)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[ABOUT], 0);
}



/**
 * xfce_panel_plugin_signal_configure
 * @plugin      : an #XfcePanelPlugin
 *
 * Should be called by implementations of the interface.
 **/
void
xfce_panel_plugin_signal_configure (XfcePanelPlugin *plugin)
{
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    g_signal_emit (G_OBJECT (plugin), xfce_panel_plugin_signals[CONFIGURE], 0);
}



/**
 * xfce_panel_plugin_get_name
 * @plugin : an #XfcePanelPlugin
 *
 * The plugin name identifies a plugin type and therefore must be unique.
 *
 * Returns: the plugin name.
 **/
gchar *
xfce_panel_plugin_get_name (XfcePanelPlugin *plugin)
{
    gchar *name = NULL;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

    g_object_get (G_OBJECT (plugin), I_("name"), &name, NULL);

    return name;
}



/**
 * xfce_panel_plugin_get_id
 * @plugin : an #XfcePanelPlugin
 *
 * The plugin id is a unique identifier string that is given to every instance
 * of a panel plugin.
 *
 * Returns: the plugin id.
 **/
gchar *
xfce_panel_plugin_get_id (XfcePanelPlugin *plugin)
{
    gchar *id = NULL;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

    g_object_get (G_OBJECT (plugin), I_("id"), &id, NULL);

    return id;
}



/**
 * xfce_panel_plugin_get_display_name
 * @plugin : an #XfcePanelPlugin
 *
 * The display name is the (translated) plugin name that can be used in a user
 * interface, e.g. a dialog or a menu.
 *
 * Returns: the display name of @plugin.
 **/
gchar *
xfce_panel_plugin_get_display_name (XfcePanelPlugin *plugin)
{
    gchar *name = NULL;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), NULL);

    g_object_get (G_OBJECT (plugin), I_("display-name"), &name, NULL);

    return name;
}



/**
 * xfce_panel_plugin_get_size
 * @plugin : an #XfcePanelPlugin
 *
 * Returns: the current panel size.
 **/
gint
xfce_panel_plugin_get_size (XfcePanelPlugin *plugin)
{
    gint size;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), 48);

    g_object_get (G_OBJECT (plugin), I_("size"), &size, NULL);

    return size;
}



/**
 * xfce_panel_plugin_get_screen_position
 * @plugin : an #XfcePanelPlugin
 *
 * Returns: the current #XfceScreenPosition of the panel.
 **/
XfceScreenPosition
xfce_panel_plugin_get_screen_position (XfcePanelPlugin *plugin)
{
    XfceScreenPosition screen_position;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin),
                          XFCE_SCREEN_POSITION_S);

    g_object_get (G_OBJECT (plugin), I_("screen-position"), &screen_position,
                  NULL);

    return screen_position;
}



/**
 * xfce_panel_plugin_get_expand
 * @plugin : an #XfcePanelPlugin
 *
 * Returns: whether the plugin will expand when the panel width increases.
 **/
gboolean
xfce_panel_plugin_get_expand (XfcePanelPlugin *plugin)
{
    gboolean expand;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin), FALSE);

    g_object_get (G_OBJECT (plugin), I_("expand"), &expand, NULL);

    return expand;
}



/**
 * xfce_panel_plugin_get_orientation
 * @plugin : an #XfcePanelPlugin
 *
 * Returns: the current #GtkOrientation of the panel.
 **/
GtkOrientation
xfce_panel_plugin_get_orientation (XfcePanelPlugin *plugin)
{
    XfceScreenPosition screen_position;

    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (plugin),
                          GTK_ORIENTATION_HORIZONTAL);

    g_object_get (G_OBJECT (plugin), I_("screen-position"), &screen_position,
                  NULL);

    return xfce_screen_position_get_orientation (screen_position);
}



/**
 * xfce_panel_plugin_remove_confirm
 * @plugin : an #XfcePanelPlugin
 *
 * Ask the plugin to be removed.
 **/
void
xfce_panel_plugin_remove_confirm (XfcePanelPlugin *plugin)
{
    GtkWidget *dialog;
    gint       response;
    gchar     *name;

    name = xfce_panel_plugin_get_display_name (plugin);

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_NONE,
                                     _("Remove \"%s\"?"), name);

    gtk_dialog_add_buttons (GTK_DIALOG (dialog), 
                            GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                            GTK_STOCK_REMOVE, GTK_RESPONSE_YES,
                            NULL);
    g_free (name);
    
    gtk_window_set_screen (GTK_WINDOW (dialog),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              _("The item will be removed from "
                                                "the panel and its configuration "
                                                "will be lost."));

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if (G_LIKELY (response == GTK_RESPONSE_YES))
        xfce_panel_plugin_remove (plugin);
}



/**
 * xfce_panel_plugin_remove
 * @plugin : an #XfcePanelPlugin
 *
 * Remove the plugin from the panel.
 **/
void
xfce_panel_plugin_remove (XfcePanelPlugin *plugin)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->remove (plugin);
}



/**
 * xfce_panel_plugin_set_expand
 * @plugin : an #XfcePanelPlugin
 * @expand : whether to expand the plugin
 *
 * Sets whether to expand the plugin when the width of the panel increases.
 **/
void
xfce_panel_plugin_set_expand (XfcePanelPlugin *plugin,
                              gboolean         expand)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->set_expand (plugin, expand);
}



/**
 * xfce_panel_plugin_customize_panel
 * @plugin : an #XfcePanelPlugin
 *
 * Ask the panel to show the settings dialog.
 **/
void
xfce_panel_plugin_customize_panel (XfcePanelPlugin *plugin)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->customize_panel (plugin);
}



/**
 * xfce_panel_plugin_customize_items
 * @plugin : an #XfcePanelPlugin
 *
 * Ask the panel to show the settings dialog, with the 'Add Items' tab shown.
 **/
void
xfce_panel_plugin_customize_items (XfcePanelPlugin *plugin)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->customize_items (plugin);
}



/**
 * xfce_panel_plugin_move
 * @plugin : an #XfcePanelPlugin
 *
 * Ask the panel to start a move operation.
 **/
void
xfce_panel_plugin_move (XfcePanelPlugin *plugin)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->move (plugin);
}



/**
 * xfce_panel_plugin_register_menu
 * @plugin : an #XfcePanelPlugin
 * @menu   : a #GtkMenu that will be opened
 *
 * Register an open menu. This will make sure the panel will properly handle
 * its autohide behaviour.
 **/
void
xfce_panel_plugin_register_menu (XfcePanelPlugin *plugin,
                                 GtkMenu         *menu)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->register_menu (plugin, menu);
}



/**
 * xfce_panel_plugin_focus_widget
 * @plugin : an #XfcePanelPlugin
 * @widget : widget to focus
 *
 * Grab the focus on @widget. Asks the panel to allow focus on its items and
 * set the focus to the requested widget.
 **/
void
xfce_panel_plugin_focus_widget (XfcePanelPlugin *plugin,
                                GtkWidget       *widget)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->focus_panel (plugin);

    gtk_widget_grab_focus (widget);
}



/**
 * xfce_panel_plugin_set_panel_hidden
 * @plugin : an #XfcePanelPlugin
 * @hidden : %FALSE to unhide, %TRUE to hide the panel
 *
 * Ask the panel to hide or unhide. This only has effect when autohide is
 * enabled.
 **/
void
xfce_panel_plugin_set_panel_hidden (XfcePanelPlugin *plugin,
                                    gboolean         hidden)
{
    XFCE_PANEL_PLUGIN_GET_INTERFACE (plugin)->set_panel_hidden (plugin, hidden);
}



static void
_plugin_menu_deactivate (GtkWidget *menu)
{
    gint id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu),
                                                  I_("xfce-panel-plugin-button-release-callback")));

    if (id)
    {
        g_signal_handler_disconnect (G_OBJECT (menu), id);
        g_object_set_data (G_OBJECT (menu),
                           I_("xfce-panel-plugin-button-release-callback"),
                           NULL);
    }
}



static gboolean
_plugin_menu_button_released (GtkWidget       *menu,
                              GdkEventButton  *ev,
                              XfcePanelPlugin *plugin)
{
    gint id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu),
                                                  I_("xfce-panel-plugin-button-release-callback")));

    if (id)
    {
        g_signal_handler_disconnect (G_OBJECT (menu), id);
        g_object_set_data (G_OBJECT (menu),
                           I_("xfce-panel-plugin-button-release-callback"),
                           NULL);
        return TRUE;
    }

    return FALSE;
}



/**
 * xfce_panel_plugin_create_menu
 * @plugin          : an #XfcePanelPlugin
 *
 * This should only be called by implementors of the panel plugin interface.
 **/
void
xfce_panel_plugin_create_menu (XfcePanelPlugin *plugin)
{
    GtkWidget *menu, *mi, *img;
    gint       insert_position;
    gint       configure_position;
    gchar     *name;
    gboolean   allow_customization;

    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (plugin));

    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    allow_customization = xfce_allow_panel_customization ();

    menu = gtk_menu_new ();

    /* title */
    name = xfce_panel_plugin_get_display_name (plugin);
    mi = gtk_menu_item_new_with_label (name);
    g_free (name);
    gtk_widget_set_sensitive (mi, FALSE);
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    /* configure, hide by default */

    mi = gtk_image_menu_item_new_with_label (_("Properties"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    img = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU);
    gtk_widget_show (img);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

    g_signal_connect_swapped (G_OBJECT (mi), "activate",
                              G_CALLBACK (xfce_panel_plugin_signal_configure),
                              plugin);

    configure_position = 2;
    g_object_set_data (G_OBJECT (plugin),
                       I_("xfce-panel-plugin-configure-position"),
                       GINT_TO_POINTER (configure_position));

    /* about item, hide by default */
    mi = gtk_image_menu_item_new_with_label (_("About"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    img = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU);
    gtk_widget_show (img);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

    g_signal_connect_swapped (G_OBJECT (mi), "activate",
                              G_CALLBACK (xfce_panel_plugin_signal_about),
                              plugin);

    /* move */
    if (G_LIKELY (allow_customization))
    {
        mi = gtk_image_menu_item_new_with_label (_("Move"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                  G_CALLBACK (xfce_panel_plugin_move), plugin);

        /* insert custom items after move */
        insert_position = 5;

        /* remove */
        mi = gtk_separator_menu_item_new();
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_image_menu_item_new_with_label (_("Remove"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                  G_CALLBACK (xfce_panel_plugin_remove_confirm),
                                  plugin);

        /* panel section */
        mi = gtk_separator_menu_item_new();
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        mi = gtk_image_menu_item_new_with_label (_("Add New Item"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                  G_CALLBACK (xfce_panel_plugin_customize_items), plugin);

        mi = gtk_image_menu_item_new_with_label (_("Customize Panel"));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        img = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);

        g_signal_connect_swapped (G_OBJECT (mi), "activate",
                                  G_CALLBACK (xfce_panel_plugin_customize_panel), plugin);
    }
    else
    {
        /* insert custom items after about */
        insert_position = 4;
    }

    g_object_set_data (G_OBJECT (plugin),
                       I_("xfce-panel-plugin-insert-position"),
                       GINT_TO_POINTER (insert_position));

    /* deactivation */
    g_signal_connect (G_OBJECT (menu), "deactivate",
                      G_CALLBACK (_plugin_menu_deactivate), NULL);

    g_object_set_data_full (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"),
                            menu, (GDestroyNotify) gtk_widget_destroy);
}



/**
 * xfce_panel_plugin_popup_menu
 * @plugin : an #XfcePanelPlugin
 *
 * Shows the right-click menu.
 **/
void
xfce_panel_plugin_popup_menu (XfcePanelPlugin *plugin)
{
    GtkMenu *menu;
    gint     block, id;

    block = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin),
                                                I_("xfce-panel-plugin-block")));

    if (block > 0)
        return;

    menu = g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    if (menu)
    {
        id = g_signal_connect (G_OBJECT (menu), "button-release-event",
                               G_CALLBACK (_plugin_menu_button_released),
                               plugin);

        g_object_set_data (G_OBJECT (menu),
                           I_("xfce-panel-plugin-button-release-callback"),
                           GINT_TO_POINTER (id));

        xfce_panel_plugin_register_menu (plugin, menu);

        gtk_menu_set_screen (menu, gtk_widget_get_screen (GTK_WIDGET (plugin)));

        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
    }
}



static gboolean
_plugin_popup_menu (GtkWidget       *widget,
                    GdkEventButton  *ev,
                    XfcePanelPlugin *plugin)
{
    GtkMenu *menu;
    guint    modifiers;
    gint     block;

    block = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin),
                                                I_("xfce-panel-plugin-block")));

    if (block > 0)
        return FALSE;

    menu = g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    modifiers = gtk_accelerator_get_default_mod_mask ();

    if (ev->button == 3 || (ev->button == 1 &&
        (ev->state & modifiers) == GDK_CONTROL_MASK))
    {
        gtk_menu_set_screen (menu, gtk_widget_get_screen (widget));

        xfce_panel_plugin_register_menu (plugin, menu);

        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, ev->button, ev->time);

        return TRUE;
    }

    return FALSE;
}



/**
 * xfce_panel_plugin_add_action_widget
 * @plugin : an #XfcePanelPlugin
 * @widget : a #GtkWidget that receives mouse events
 *
 * Attach the plugin menu to this widget. Plugin writers should call this
 * for every widget that can receive mouse events. If you forget to call this
 * the plugin will not have a right-click menu and the user won't be able to
 * remove it.
 **/
void
xfce_panel_plugin_add_action_widget (XfcePanelPlugin *plugin,
                                     GtkWidget       *widget)
{
    GtkWidget *menu =
        g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    if (menu)
    {
        g_signal_connect (G_OBJECT (widget), "button-press-event",
                          G_CALLBACK (_plugin_popup_menu), plugin);
    }
}



/**
 * xfce_panel_plugin_menu_insert_item
 * @plugin   : an #XfcePanelPlugin
 * @item     : the menu item to add
 *
 * Insert custom menu item.
 **/
void
xfce_panel_plugin_menu_insert_item (XfcePanelPlugin *plugin,
                                    GtkMenuItem     *item)
{
    gint       position;
    GtkWidget *menu =
        g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    if (menu)
    {
        position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin),
                                                       I_("xfce-panel-plugin-insert-position")));

        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), GTK_WIDGET (item),
                               position);

        position++;

        g_object_set_data (G_OBJECT (plugin),
                           I_("xfce-panel-plugin-insert-position"),
                           GINT_TO_POINTER (position));
    }
}



/**
 * xfce_panel_plugin_menu_show_about
 * @plugin : an #XfcePanelPlugin
 *
 * Show the 'About' item in the menu. Clicking on the menu item will emit
 * the "about" signal.
 **/
void
xfce_panel_plugin_menu_show_about (XfcePanelPlugin *plugin)
{
    gint       position;
    GList     *l;
    GtkWidget *menu =
        g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    if (menu)
    {
        position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin),
                                                       I_("xfce-panel-plugin-configure-position")));

        l = g_list_nth (GTK_MENU_SHELL (menu)->children, position+1);

        if (l)
            gtk_widget_show (GTK_WIDGET (l->data));
    }
}



/**
 * xfce_panel_plugin_menu_show_configure
 * @plugin : an #XfcePanelPlugin
 *
 * Show the 'Configure' item in the menu. Clicking on the menu item will emit
 * the "configure-plugin" signal.
 **/
void
xfce_panel_plugin_menu_show_configure (XfcePanelPlugin *plugin)
{
    GtkWidget *menu;
    gint       position;
    GList     *l;

    if (!xfce_allow_panel_customization())
        return;

    menu = g_object_get_data (G_OBJECT (plugin), I_("xfce-panel-plugin-menu"));

    if (menu)
    {
        position = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (plugin),
                                                       I_("xfce-panel-plugin-configure-position")));

        l = g_list_nth (GTK_MENU_SHELL (menu)->children, position);

        if (l)
            gtk_widget_show (GTK_WIDGET (l->data));
    }
}



/**
 * xfce_panel_plugin_block_menu
 * @plugin : an #XfcePanelPlugin
 *
 * Temporarily block the menu from being shown. This can be used by plugin
 * writers when the configuration dialog is active.
 *
 * See also: xfce_panel_plugin_unblock_menu()
 **/
void
xfce_panel_plugin_block_menu (XfcePanelPlugin *plugin)
{
    gint n = 1;

    g_object_set_data (G_OBJECT (plugin), I_("xfce-panel-plugin-block"),
                       GINT_TO_POINTER (n));
}



/**
 * xfce_panel_plugin_unblock_menu
 * @plugin : an #XfcePanelPlugin
 *
 * Don't block the menu from being shown.
 *
 * See also: xfce_panel_plugin_block_menu()
 **/
void
xfce_panel_plugin_unblock_menu (XfcePanelPlugin *plugin)
{
    g_object_set_data (G_OBJECT (plugin), I_("xfce-panel-plugin-block"), NULL);
}



/**
 * xfce_panel_plugin_lookup_rc_file
 * @plugin    : an #XfcePanelPlugin
 *
 * Looks up unique filename associated with @plugin in standard configuration
 * locations. Uses xfce_resource_lookup() ginternally.
 *
 * Returns: path to configuration file or %NULL if none was found.  The
 *          returned string must be freed using g_free().
 *
 * See also: xfce_panel_plugin_save_location()
 **/
gchar *
xfce_panel_plugin_lookup_rc_file (XfcePanelPlugin *plugin)
{
  gchar  path[255];
  gchar *id, *name;

    name = xfce_panel_plugin_get_name (plugin);
    id = xfce_panel_plugin_get_id (plugin);

    g_snprintf (path, sizeof (path),
                "xfce4" G_DIR_SEPARATOR_S
                "panel" G_DIR_SEPARATOR_S "%s-%s.rc", name, id);

    g_free (name);
    g_free (id);

    return xfce_resource_lookup (XFCE_RESOURCE_CONFIG, path);
}



/**
 * xfce_panel_plugin_save_location
 * @plugin    : an #XfcePanelPlugin
 * @create    : whether the file should be created
 *
 * Unique file location that can be used to store configuration information.
 * Uses xfce_resource_save_location() ginternally.
 *
 * Returns: path to configuration file or %NULL is the file could not be
 *          created. The returned string must be freed using g_free().
 *
 * See also: xfce_panel_plugin_lookup_rc_file()
 **/
gchar *
xfce_panel_plugin_save_location (XfcePanelPlugin *plugin,
                                 gboolean         create)
{
    gchar path[255];
    gchar *name, *id;

    name = xfce_panel_plugin_get_name (plugin);
    id = xfce_panel_plugin_get_id (plugin);

    g_snprintf (path, sizeof (path),
                "xfce4" G_DIR_SEPARATOR_S
                "panel" G_DIR_SEPARATOR_S "%s-%s.rc", name, id);

    g_free (name);
    g_free (id);

    return xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, create);
}



/**
 * xfce_panel_plugin_set_sensitive
 * @plugin    : an #XfcePanelPlugin
 * @sensitive : whether to make the widget sensitive
 *
 * This should only be called by plugin implementations.
 **/
void
xfce_panel_plugin_set_sensitive (XfcePanelPlugin *plugin,
                                 gboolean         sensitive)
{
    if (GTK_BIN (plugin)->child)
        gtk_widget_set_sensitive (GTK_BIN (plugin)->child, sensitive);
    else
        g_signal_connect_after (G_OBJECT (plugin), "realize",
                                G_CALLBACK (xfce_panel_plugin_set_sensitive),
                                GINT_TO_POINTER (sensitive));
}
