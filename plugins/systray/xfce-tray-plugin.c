/* $Id$ */
/*
 * Copyright (c) 2007 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define SMALL_PANEL_SIZE (26)

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-macros.h>

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"
#include "xfce-tray-plugin.h"
#include "xfce-tray-dialogs.h"



/* prototypes */
static void            xfce_tray_plugin_message                 (GtkMessageType      type,
                                                                 GdkScreen          *screen,
                                                                 const gchar        *message);
static gboolean        xfce_tray_plugin_check                   (GdkScreen          *screen);
static GtkArrowType    xfce_tray_plugin_button_position         (XfcePanelPlugin    *panel_plugin);
static XfceTrayPlugin *xfce_tray_plugin_new                     (XfcePanelPlugin    *panel_plugin);
static void            xfce_tray_plugin_screen_position_changed (XfceTrayPlugin     *plugin,
                                                                 XfceScreenPosition  position);
static void            xfce_tray_plugin_orientation_changed     (XfceTrayPlugin     *plugin,
                                                                 GtkOrientation      orientation);
static void            xfce_tray_plugin_tray_size_changed       (XfceTrayPlugin     *plugin,
                                                                 gint                size);
static gboolean        xfce_tray_plugin_size_changed            (XfceTrayPlugin     *plugin,
                                                                 guint               size);
static void            xfce_tray_plugin_read                    (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_write                   (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_free                    (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_construct               (XfcePanelPlugin    *panel_plugin);



/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK (xfce_tray_plugin_construct, xfce_tray_plugin_check);



static void
xfce_tray_plugin_message (GtkMessageType  type,
                          GdkScreen      *screen,
                          const gchar    *message)
{
    GtkWidget *dialog;

    /* create a dialog */
    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, type, GTK_BUTTONS_CLOSE, _("System Tray"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s.", message);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_screen (GTK_WINDOW (dialog), screen);

    /* run the dialog */
    gtk_dialog_run (GTK_DIALOG (dialog));

    /* destroy */
    gtk_widget_destroy (dialog);
}



static gboolean
xfce_tray_plugin_check (GdkScreen *screen)
{
    gboolean   running;


    /* check if there is already a tray running */
    running = xfce_tray_manager_check_running (screen);

    /* message */
    if (running)
        xfce_tray_plugin_message (GTK_MESSAGE_INFO, screen,
                                  _("There is already a system tray running on this screen"));

    return (!running);
}



static GtkArrowType
xfce_tray_plugin_button_position (XfcePanelPlugin *panel_plugin)
{
    XfceScreenPosition  position;
    GdkScreen          *screen;
    GdkRectangle        geom;
    gint                mon, x, y;

    g_return_val_if_fail (GTK_WIDGET_REALIZED (panel_plugin), GTK_ARROW_LEFT);

    /* get the plugin position */
    position = xfce_panel_plugin_get_screen_position (panel_plugin);

    /* get the button position */
    switch (position)
    {
        /*    horizontal west */
        case XFCE_SCREEN_POSITION_NW_H:
        case XFCE_SCREEN_POSITION_SW_H:
            return GTK_ARROW_RIGHT;

        /* horizontal east */
        case XFCE_SCREEN_POSITION_N:
        case XFCE_SCREEN_POSITION_NE_H:
        case XFCE_SCREEN_POSITION_S:
        case XFCE_SCREEN_POSITION_SE_H:
            return GTK_ARROW_LEFT;

        /* vertical north */
        case XFCE_SCREEN_POSITION_NW_V:
        case XFCE_SCREEN_POSITION_NE_V:
            return GTK_ARROW_DOWN;

        /* vertical south */
        case XFCE_SCREEN_POSITION_W:
        case XFCE_SCREEN_POSITION_SW_V:
        case XFCE_SCREEN_POSITION_E:
        case XFCE_SCREEN_POSITION_SE_V:
            return GTK_ARROW_UP;

        /* floating */
        default:
            /* get the screen information */
            screen = gtk_widget_get_screen (GTK_WIDGET (panel_plugin));
            mon = gdk_screen_get_monitor_at_window (screen, GTK_WIDGET (panel_plugin)->window);
            gdk_screen_get_monitor_geometry (screen, mon, &geom);
            gdk_window_get_root_origin (GTK_WIDGET (panel_plugin)->window, &x, &y);

            /* get the position based on the screen position */
            if (position == XFCE_SCREEN_POSITION_FLOATING_H)
                return ((x < (geom.x + geom.width / 2)) ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
            else
                return ((y < (geom.y + geom.height / 2)) ? GTK_ARROW_DOWN : GTK_ARROW_UP);
    }
}



static XfceTrayPlugin *
xfce_tray_plugin_new (XfcePanelPlugin *panel_plugin)
{
    XfceTrayPlugin *plugin;
    GError         *error = NULL;
    GtkArrowType    position;
    GdkScreen      *screen;

    /* create structure */
    plugin = panel_slice_new0 (XfceTrayPlugin);

    /* set some data */
    plugin->panel_plugin = panel_plugin;
    plugin->manager = NULL;
    plugin->show_frame = TRUE;

    /* get the button position */
    position = xfce_tray_plugin_button_position (panel_plugin);

    /* get the screen of the plugin */
    screen = gtk_widget_get_screen (GTK_WIDGET (panel_plugin));

    /* try to create the tray */
    plugin->tray = xfce_tray_widget_new_for_screen (screen, position, &error);

    if (G_LIKELY (plugin->tray))
    {
        /* get the manager */
        plugin->manager = xfce_tray_widget_get_manager (XFCE_TRAY_WIDGET (plugin->tray));

        /* read the plugin settings */
        xfce_tray_plugin_read (plugin);

        /* create the frame */
        plugin->frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), plugin->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->frame);
        gtk_widget_show (plugin->frame);

        /* add the tray */
        gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->tray);
        gtk_widget_show (plugin->tray);

        /* connect signal to handle the real plugin size */
        g_signal_connect_swapped (G_OBJECT (plugin->tray), "tray-size-changed",
                                  G_CALLBACK (xfce_tray_plugin_tray_size_changed), plugin);
    }
    else
    {
        /* show the message */
        xfce_tray_plugin_message (GTK_MESSAGE_ERROR, screen, error->message);

        /* cleanup */
        g_error_free (error);
    }

    return plugin;
}



static void
xfce_tray_plugin_screen_position_changed (XfceTrayPlugin     *plugin,
                                          XfceScreenPosition  position)
{
    GtkArrowType button_position;

    /* get the new position */
    button_position = xfce_tray_plugin_button_position (plugin->panel_plugin);

    /* set the position */
    xfce_tray_widget_set_arrow_position (XFCE_TRAY_WIDGET (plugin->tray), button_position);
}



static void
xfce_tray_plugin_orientation_changed (XfceTrayPlugin *plugin,
                                      GtkOrientation  orientation)
{
    /* invoke the function above */
    xfce_tray_plugin_screen_position_changed (plugin, XFCE_SCREEN_POSITION_NONE);
}



static void
xfce_tray_plugin_tray_size_changed (XfceTrayPlugin *plugin,
                                    gint            size)
{
    gint panel_size;

    /* get the panel size */
    panel_size = xfce_panel_plugin_get_size (plugin->panel_plugin);

    /* correct the requested plugin size */
    size += panel_size > SMALL_PANEL_SIZE ? 6 : 4;

    /* update the plugin size */
    if (xfce_panel_plugin_get_orientation (plugin->panel_plugin) == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_set_size_request (GTK_WIDGET (plugin->panel_plugin), size, panel_size);
    else
        gtk_widget_set_size_request (GTK_WIDGET (plugin->panel_plugin), panel_size, size);
}



static gboolean
xfce_tray_plugin_size_changed (XfceTrayPlugin  *plugin,
                               guint            size)
{
    gint border;

    /* set the frame border size */
    gtk_container_set_border_width (GTK_CONTAINER (plugin->frame), size > SMALL_PANEL_SIZE ? 1 : 0);

    /* get the border size */
    border = size > SMALL_PANEL_SIZE ? 6 : 4;

    /* set the new plugin size */
    xfce_tray_widget_set_size_request (XFCE_TRAY_WIDGET (plugin->tray), size - border);

    /* we handled the size of the plugin */
    return TRUE;
}



static void
xfce_tray_plugin_read (XfceTrayPlugin *plugin)
{
    gchar     *file;
    gchar    **applications;
    gboolean   hidden;
    XfceRc    *rc;
    guint      i;

    /* get rc file name */
    file = xfce_panel_plugin_lookup_rc_file (plugin->panel_plugin);

    if (G_LIKELY (file))
    {
        /* open the file, readonly */
        rc = xfce_rc_simple_open (file, TRUE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* save global plugin settings */
            xfce_rc_set_group (rc, "Global");

            /* frame setting */
            plugin->show_frame = xfce_rc_read_bool_entry (rc, "ShowFrame", TRUE);

            if (G_LIKELY (plugin->manager))
            {
                /* list of known applications */
                applications = xfce_rc_get_entries (rc, "Applications");

                if (G_LIKELY (applications))
                {
                    /* set the group */
                    xfce_rc_set_group (rc, "Applications");

                    /* read their visibility */
                    for (i = 0; applications[i] != NULL; i++)
                    {
                        /* whether the application is hidden */
                        hidden = xfce_rc_read_bool_entry (rc, applications[i], FALSE);

                        /* add the application name */
                        xfce_tray_manager_application_add (plugin->manager, applications[i], hidden);
                    }

                    /* cleanup */
                    g_strfreev (applications);
                }
            }

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_tray_plugin_write (XfceTrayPlugin *plugin)
{
    gchar               *file;
    GSList              *applications, *li;
    XfceRc              *rc;
    XfceTrayApplication *application;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_save_location (plugin->panel_plugin, TRUE);

    if (G_LIKELY (file))
    {
        /* open the file, writable */
        rc = xfce_rc_simple_open (file, FALSE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* save global plugin settings */
            xfce_rc_set_group (rc, "Global");

            /* write setting */
            xfce_rc_write_bool_entry (rc, "ShowFrame", plugin->show_frame);

            if (G_LIKELY (plugin->manager))
            {
                /* save the list of known applications and their visibility */
                xfce_rc_set_group (rc, "Applications");

                /* get the list of known applications */
                applications = xfce_tray_manager_application_list (plugin->manager, FALSE);

                /* save their state */
                for (li = applications; li != NULL; li = li->next)
                {
                    application = li->data;

                    if (G_LIKELY (application->name))
                        xfce_rc_write_bool_entry (rc, application->name, application->hidden);
                }
            }

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
xfce_tray_plugin_free (XfceTrayPlugin *plugin)
{
    /* free slice */
    panel_slice_free (XfceTrayPlugin, plugin);
}



static void
xfce_tray_plugin_construct (XfcePanelPlugin *panel_plugin)
{
    XfceTrayPlugin *plugin;

    /* create the tray panel plugin */
    plugin = xfce_tray_plugin_new (panel_plugin);

    /* set the action widgets and show configure */
    xfce_panel_plugin_add_action_widget (panel_plugin, plugin->frame);
    xfce_panel_plugin_add_action_widget (panel_plugin, plugin->tray);
    xfce_panel_plugin_menu_show_configure (panel_plugin);

    /* connect signals */
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "screen-position-changed",
                              G_CALLBACK (xfce_tray_plugin_screen_position_changed), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "orientation-changed",
                              G_CALLBACK (xfce_tray_plugin_orientation_changed), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "size-changed",
                              G_CALLBACK (xfce_tray_plugin_size_changed), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "save",
                              G_CALLBACK (xfce_tray_plugin_write), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "free-data",
                              G_CALLBACK (xfce_tray_plugin_free), plugin);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "configure-plugin",
                              G_CALLBACK (xfce_tray_dialogs_configure), plugin);
}
