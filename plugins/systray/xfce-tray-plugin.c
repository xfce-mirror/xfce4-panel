/* $Id$ */
/*
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
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
#include <libxfce4panel/libxfce4panel.h>

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"
#include "xfce-tray-plugin.h"
#include "xfce-tray-dialogs.h"



/* prototypes */
static void            xfce_tray_plugin_message                 (GtkMessageType      type,
                                                                 GdkScreen          *screen,
                                                                 const gchar        *message);
static gboolean        xfce_tray_plugin_check                   (GdkScreen          *screen);
static void            xfce_tray_plugin_update_position         (XfceTrayPlugin     *plugin);
static XfceTrayPlugin *xfce_tray_plugin_new                     (XfcePanelPlugin    *panel_plugin);
static void            xfce_tray_plugin_screen_position_changed (XfceTrayPlugin     *plugin,
                                                                 XfceScreenPosition  position);
static void            xfce_tray_plugin_orientation_changed     (XfceTrayPlugin     *plugin,
                                                                 GtkOrientation      orientation);
static gboolean        xfce_tray_plugin_size_changed            (XfceTrayPlugin     *plugin,
                                                                 guint               size);
static void            xfce_tray_plugin_read                    (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_write                   (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_free                    (XfceTrayPlugin     *plugin);
static void            xfce_tray_plugin_construct               (XfcePanelPlugin    *panel_plugin);



/* register the plugin */
XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK (xfce_tray_plugin_construct, xfce_tray_plugin_check)



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
    {
        xfce_tray_plugin_message (GTK_MESSAGE_INFO, screen,
                                  _("There is already a system tray running on this screen"));
    }

    return (!running);
}



static void
xfce_tray_plugin_update_position (XfceTrayPlugin *plugin)
{
    XfceScreenPosition  position;
    GdkScreen          *screen;
    GdkRectangle        geom;
    gint                mon, x, y;
    GtkArrowType        arrow_type;

    panel_return_if_fail (GTK_WIDGET_REALIZED (plugin->panel_plugin));

    /* get the plugin position */
    position = xfce_panel_plugin_get_screen_position (plugin->panel_plugin);

    /* get the button position */
    switch (position)
    {
        /*    horizontal west */
        case XFCE_SCREEN_POSITION_NW_H:
        case XFCE_SCREEN_POSITION_SW_H:
            arrow_type = GTK_ARROW_RIGHT;
            break;

        /* horizontal east */
        case XFCE_SCREEN_POSITION_N:
        case XFCE_SCREEN_POSITION_NE_H:
        case XFCE_SCREEN_POSITION_S:
        case XFCE_SCREEN_POSITION_SE_H:
            arrow_type = GTK_ARROW_LEFT;
            break;

        /* vertical north */
        case XFCE_SCREEN_POSITION_NW_V:
        case XFCE_SCREEN_POSITION_NE_V:
            arrow_type = GTK_ARROW_DOWN;
            break;

        /* vertical south */
        case XFCE_SCREEN_POSITION_W:
        case XFCE_SCREEN_POSITION_SW_V:
        case XFCE_SCREEN_POSITION_E:
        case XFCE_SCREEN_POSITION_SE_V:
            arrow_type = GTK_ARROW_UP;
            break;

        /* floating */
        default:
            /* get the screen information */
            screen = gtk_widget_get_screen (GTK_WIDGET (plugin->panel_plugin));
            mon = gdk_screen_get_monitor_at_window (screen, GTK_WIDGET (plugin->panel_plugin)->window);
            gdk_screen_get_monitor_geometry (screen, mon, &geom);
            gdk_window_get_root_origin (GTK_WIDGET (plugin->panel_plugin)->window, &x, &y);

            /* get the position based on the screen position */
            if (position == XFCE_SCREEN_POSITION_FLOATING_H)
                arrow_type = ((x < (geom.x + geom.width / 2)) ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
            else
                arrow_type = ((y < (geom.y + geom.height / 2)) ? GTK_ARROW_DOWN : GTK_ARROW_UP);
            break;
    }

    /* set the arrow type of the tray widget */
    xfce_tray_widget_set_arrow_type (XFCE_TRAY_WIDGET (plugin->tray), arrow_type);

    /* update the manager orientation */
    xfce_tray_manager_set_orientation (plugin->manager, xfce_screen_position_get_orientation (position));
}



static void
xfce_tray_plugin_icon_added (XfceTrayManager *manager,
                             GtkWidget       *icon,
                             XfceTrayPlugin  *plugin)
{
    gchar *name;

    /* get the application name */
    name = xfce_tray_manager_get_application_name (icon);

    /* add the icon to the widget */
    xfce_tray_widget_add_with_name (XFCE_TRAY_WIDGET (plugin->tray), icon, name);

    /* cleanup */
    g_free (name);

    /* show icon */
    gtk_widget_show (icon);
}



static void
xfce_tray_plugin_icon_removed (XfceTrayManager *manager,
                               GtkWidget       *icon,
                               XfceTrayPlugin  *plugin)
{
    /* remove from the tray */
    gtk_container_remove (GTK_CONTAINER (plugin->tray), icon);
}



static void
xfce_tray_plugin_lost_selection (XfceTrayManager *manager,
                                 XfceTrayPlugin  *plugin)
{
    GdkScreen *screen;

    /* get screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (plugin->panel_plugin));

    /* message */
    xfce_tray_plugin_message (GTK_MESSAGE_WARNING, screen, _("The tray manager lost selection"));
}



static XfceTrayPlugin *
xfce_tray_plugin_new (XfcePanelPlugin *panel_plugin)
{
    XfceTrayPlugin *plugin;
    gboolean        result;
    GError         *error = NULL;
    GdkScreen      *screen;

    /* create structure */
    plugin = g_slice_new0 (XfceTrayPlugin);

    /* set some data */
    plugin->panel_plugin = panel_plugin;
    plugin->manager = NULL;
    plugin->show_frame = TRUE;

    /* create the frame */
    plugin->frame = gtk_frame_new (NULL);
    gtk_container_add (GTK_CONTAINER (panel_plugin), plugin->frame);
    gtk_widget_show (plugin->frame);

    /* create tray widget */
    plugin->tray = xfce_tray_widget_new ();
    gtk_container_add (GTK_CONTAINER (plugin->frame), plugin->tray);
    gtk_widget_show (plugin->tray);

    /* create a tray manager */
    plugin->manager = xfce_tray_manager_new ();

    /* read the plugin settings */
    xfce_tray_plugin_read (plugin);

    /* set frame shadow */
    gtk_frame_set_shadow_type (GTK_FRAME (plugin->frame), plugin->show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);

    /* get screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (panel_plugin));

    /* register the tray */
    result = xfce_tray_manager_register (plugin->manager, screen, &error);

    /* check for problems */
    if (G_LIKELY (result == TRUE))
    {
        /* connect signals */
        g_signal_connect (G_OBJECT (plugin->manager), "tray-icon-added", G_CALLBACK (xfce_tray_plugin_icon_added), plugin);
        g_signal_connect (G_OBJECT (plugin->manager), "tray-icon-removed", G_CALLBACK (xfce_tray_plugin_icon_removed), plugin);
        g_signal_connect (G_OBJECT (plugin->manager), "tray-lost-selection", G_CALLBACK (xfce_tray_plugin_lost_selection), plugin);

        /* update the plugin position */
        xfce_tray_plugin_update_position (plugin);
    }
    else
    {
        /* show error */
        xfce_tray_plugin_message (GTK_MESSAGE_ERROR, screen, error->message);

        /* free error */
        g_error_free (error);
    }

    return plugin;
}



static void
xfce_tray_plugin_screen_position_changed (XfceTrayPlugin     *plugin,
                                          XfceScreenPosition  position)
{
    /* update the plugin position */
    xfce_tray_plugin_update_position (plugin);
}



static void
xfce_tray_plugin_orientation_changed (XfceTrayPlugin *plugin,
                                      GtkOrientation  orientation)
{
    /* update the plugin position */
    xfce_tray_plugin_update_position (plugin);
}



static gboolean
xfce_tray_plugin_size_changed (XfceTrayPlugin *plugin,
                               guint           size)
{
    /* set the border sizes */
    gtk_container_set_border_width (GTK_CONTAINER (plugin->frame), (size > SMALL_PANEL_SIZE && plugin->show_frame) ? 1 : 0);
    gtk_container_set_border_width (GTK_CONTAINER (plugin->tray),  plugin->show_frame ? 1 : 0);

    /* we handled the size of the plugin */
    return TRUE;
}



static void
xfce_tray_plugin_read (XfceTrayPlugin *plugin)
{
    gchar     *file;
    gchar    **names;
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

            /* set number of rows */
            xfce_tray_widget_set_rows (XFCE_TRAY_WIDGET (plugin->tray), xfce_rc_read_int_entry (rc, "Rows", 1));

            if (G_LIKELY (plugin->manager))
            {
                /* list of known applications */
                names = xfce_rc_get_entries (rc, "Applications");

                if (G_LIKELY (names))
                {
                    /* set the group */
                    xfce_rc_set_group (rc, "Applications");

                    /* read their visibility */
                    for (i = 0; names[i] != NULL; i++)
                    {
                        /* whether the application is hidden */
                        hidden = xfce_rc_read_bool_entry (rc, names[i], FALSE);

                        /* add the application name */
                        xfce_tray_widget_name_add (XFCE_TRAY_WIDGET (plugin->tray), names[i], hidden);
                    }

                    /* cleanup */
                    g_strfreev (names);
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
    gchar       *file;
    GList       *names, *li;
    XfceRc      *rc;
    const gchar *name;
    gboolean     hidden;

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
            xfce_rc_write_int_entry (rc, "Rows", xfce_tray_widget_get_rows (XFCE_TRAY_WIDGET (plugin->tray)));

            if (G_LIKELY (plugin->manager))
            {
                /* get the list of known applications */
                names = xfce_tray_widget_name_list (XFCE_TRAY_WIDGET (plugin->tray));

                if (names == NULL)
                {
                    /* delete group */
                    if (xfce_rc_has_group (rc, "Applications"))
                        xfce_rc_delete_group (rc, "Applications", FALSE);
                }
                else
                {
                    /* save the list of known applications and their visibility */
                    xfce_rc_set_group (rc, "Applications");

                    /* save their state */
                    for (li = names; li != NULL; li = li->next)
                    {
                        /* get name and status */
                        name = li->data;
                        hidden = xfce_tray_widget_name_hidden (XFCE_TRAY_WIDGET (plugin->tray), name);

                        /* write entry */
                        xfce_rc_write_bool_entry (rc, name, hidden);
                    }

                    /* cleanup */
                    g_list_free (names);
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
    /* unregister manager */
    xfce_tray_manager_unregister (plugin->manager);

    /* release */
    g_object_unref (G_OBJECT (plugin->manager));

    /* free slice */
    g_slice_free (XfceTrayPlugin, plugin);
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
