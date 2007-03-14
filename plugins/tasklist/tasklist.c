/*  $Id$
 *
 *  Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-hvbox.h>

#include "tasklist.h"
#include "tasklist-dialogs.h"


/* prototypes */
static void        tasklist_orientation_changed     (Tasklist        *tasklist,
                                                     GtkOrientation   orientation);
static void        tasklist_free_data               (Tasklist        *tasklist);
static void        tasklist_read_rc_file            (Tasklist        *tasklist);
static gboolean    tasklist_handle_exposed          (GtkWidget       *widget,
                                                     GdkEventExpose  *ev,
                                                     Tasklist        *tasklist);
static void        tasklist_screen_changed          (Tasklist        *tasklist,
                                                     GdkScreen       *screen);
static void        tasklist_construct               (XfcePanelPlugin *plugin);



/* register with the panel */
XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (tasklist_construct);



static void
tasklist_orientation_changed (Tasklist       *tasklist,
                              GtkOrientation  orientation)
{

    xfce_hvbox_set_orientation (XFCE_HVBOX (tasklist->box), orientation);

    gtk_widget_queue_draw (tasklist->handle);
}



gboolean
tasklist_set_size (Tasklist *tasklist,
                   gint      size)
{
    GtkOrientation orientation;

    orientation = xfce_panel_plugin_get_orientation (tasklist->plugin);

    if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (tasklist->plugin),
                                     tasklist->width, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (tasklist->plugin),
                                     size, tasklist->width);
    }

    return TRUE;
}



static void
tasklist_free_data (Tasklist *tasklist)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (tasklist->plugin), "dialog");

    if (G_UNLIKELY (dlg))
        gtk_widget_destroy (dlg);

    /* disconnect the screen changed signal */
    g_signal_handler_disconnect (G_OBJECT (tasklist->plugin),
                                 tasklist->screen_changed_id);

    panel_slice_free (Tasklist, tasklist);
}



static void
tasklist_read_rc_file (Tasklist *tasklist)
{
    gchar  *file;
    XfceRc *rc;

    /* defaults */
    tasklist->grouping       = NETK_TASKLIST_AUTO_GROUP;
    tasklist->all_workspaces = FALSE;
    tasklist->show_label     = TRUE;
    tasklist->expand         = TRUE;
    tasklist->flat_buttons   = TRUE;
    tasklist->show_handles   = TRUE;
    tasklist->width          = 300;

    if ((file = xfce_panel_plugin_lookup_rc_file (tasklist->plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (G_UNLIKELY (rc != NULL))
        {
            /* read user settings */
            tasklist->grouping       = xfce_rc_read_int_entry  (rc, "grouping", tasklist->grouping);
            tasklist->all_workspaces = xfce_rc_read_bool_entry (rc, "all_workspaces", tasklist->all_workspaces);
            tasklist->show_label     = xfce_rc_read_bool_entry (rc, "show_label", tasklist->show_label);
            tasklist->flat_buttons   = xfce_rc_read_bool_entry (rc, "flat_buttons", tasklist->flat_buttons);
            tasklist->show_handles   = xfce_rc_read_bool_entry (rc, "show_handles", tasklist->show_handles);
            tasklist->width          = xfce_rc_read_int_entry  (rc, "width",tasklist->width);
            tasklist->expand         = xfce_rc_read_bool_entry (rc, "expand", tasklist->expand);

            xfce_rc_close (rc);
        }
    }
}



void
tasklist_write_rc_file (Tasklist *tasklist)
{
    gchar  *file;
    XfceRc *rc = NULL;

    file = xfce_panel_plugin_save_location (tasklist->plugin, TRUE);

    if (G_UNLIKELY (file == NULL))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (G_LIKELY (rc != NULL))
    {
        xfce_rc_write_int_entry (rc, "grouping", tasklist->grouping);
        xfce_rc_write_int_entry (rc, "width", tasklist->width);

        xfce_rc_write_bool_entry (rc, "all_workspaces", tasklist->all_workspaces);
        xfce_rc_write_bool_entry (rc, "show_label", tasklist->show_label);
        xfce_rc_write_bool_entry (rc, "expand", tasklist->expand);
        xfce_rc_write_bool_entry (rc, "flat_buttons", tasklist->flat_buttons);
        xfce_rc_write_bool_entry (rc, "show_handles", tasklist->show_handles);

        xfce_rc_close (rc);
    }
}



static gboolean
tasklist_handle_exposed (GtkWidget      *widget,
                         GdkEventExpose *ev,
                         Tasklist       *tasklist)
{
    GtkAllocation  *allocation = &(widget->allocation);
    gint            x, y, w, h;
    GtkOrientation  orientation;

    if (GTK_WIDGET_DRAWABLE (widget))
    {
        orientation = xfce_panel_plugin_get_orientation (tasklist->plugin);

        x = allocation->x;
        y = allocation->y;
        w = allocation->width;
        h = allocation->height;

        if (orientation == GTK_ORIENTATION_HORIZONTAL)
        {
            y += widget->style->ythickness;
            h -= 2 * widget->style->ythickness;
        }
        else
        {
            x += widget->style->xthickness;
            w -= 2 * widget->style->xthickness;
        }

        gtk_paint_handle (widget->style, widget->window,
                          GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                          &(ev->area), widget, "handlebox",
                          x, y, w, h,
                          orientation);

        return TRUE;
    }

    return FALSE;
}



gboolean
tasklist_using_xinerama (XfcePanelPlugin *plugin)
{
    return (gdk_screen_get_n_monitors (gtk_widget_get_screen (GTK_WIDGET (plugin))) > 1);
}



static void
tasklist_screen_changed (Tasklist  *tasklist,
                         GdkScreen *screen)
{
    NetkScreen *ns;

    /* get the new screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (tasklist->plugin));

	/* be secure */
    if (G_UNLIKELY (screen == NULL))
        screen = gdk_screen_get_default ();

    ns = netk_screen_get (gdk_screen_get_number (screen));

    netk_tasklist_set_screen (NETK_TASKLIST (tasklist->list), ns);
}



static void
tasklist_construct (XfcePanelPlugin *plugin)
{
    GdkScreen *screen;
    gint       screen_idx;
    Tasklist  *tasklist = panel_slice_new0 (Tasklist);

    tasklist->plugin = plugin;

    /* show the properties menu item in the right-click menu */
    xfce_panel_plugin_menu_show_configure (plugin);

    /* connect panel signals */
    g_signal_connect_swapped (G_OBJECT (plugin), "orientation-changed",
                              G_CALLBACK (tasklist_orientation_changed), tasklist);
    g_signal_connect_swapped (G_OBJECT (plugin), "size-changed",
                              G_CALLBACK (tasklist_set_size), tasklist);
    g_signal_connect_swapped (G_OBJECT (plugin), "free-data",
                              G_CALLBACK (tasklist_free_data), tasklist);
    g_signal_connect_swapped (G_OBJECT (plugin), "save",
                              G_CALLBACK (tasklist_write_rc_file), tasklist);
    g_signal_connect_swapped (G_OBJECT (plugin), "configure-plugin",
                              G_CALLBACK (tasklist_properties_dialog), tasklist);

    /* read user settings / defaults */
    tasklist_read_rc_file (tasklist);

    /* whether to expand the plugin */
    xfce_panel_plugin_set_expand (plugin, tasklist->expand);

    /* create the main box */
    tasklist->box = xfce_hvbox_new (xfce_panel_plugin_get_orientation (plugin), FALSE, 0);
    gtk_widget_show (tasklist->box);
    gtk_container_add (GTK_CONTAINER (plugin), tasklist->box);

    /* create left handle */
    tasklist->handle = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (tasklist->handle, 8, 8);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->handle, FALSE, FALSE, 0);
    xfce_panel_plugin_add_action_widget (plugin, tasklist->handle);
    g_signal_connect (tasklist->handle, "expose-event",
                      G_CALLBACK (tasklist_handle_exposed), tasklist);

    /* create netk tasklist */
    screen = gtk_widget_get_screen (GTK_WIDGET (plugin));
    screen_idx = gdk_screen_get_number (screen);
    tasklist->list = netk_tasklist_new (netk_screen_get (screen_idx));
    gtk_widget_show (tasklist->list);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->list, TRUE, TRUE, 0);

    /* show the handles */
    if (tasklist->show_handles)
        gtk_widget_show (tasklist->handle);

    /* set netk tasklist settings */
    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST (tasklist->list),
                                              tasklist->all_workspaces);
    netk_tasklist_set_grouping               (NETK_TASKLIST (tasklist->list),
                                              tasklist->grouping);
    netk_tasklist_set_show_label             (NETK_TASKLIST (tasklist->list),
                                              tasklist->show_label);
    netk_tasklist_set_button_relief          (NETK_TASKLIST (tasklist->list),
                                              tasklist->flat_buttons ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);

    /* connect screen changed signal */
    tasklist->screen_changed_id =
        g_signal_connect_swapped (G_OBJECT (plugin), "screen-changed",
                                  G_CALLBACK (tasklist_screen_changed), tasklist);
}
