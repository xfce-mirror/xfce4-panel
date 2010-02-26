/* $Id: tasklist.c 26626 2008-02-18 12:42:14Z nick $
 *
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007      Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tasklist.h"
#include "tasklist-dialogs.h"

#define TASKLIST_HANDLE_SIZE (8)


/* prototypes */
static gboolean        tasklist_handle_exposed                (GtkWidget          *widget,
                                                               GdkEventExpose     *event,
                                                               TasklistPlugin     *tasklist);
static GdkPixbuf      *tasklist_icon_loader                   (const gchar        *name,
                                                               gint                size,
                                                               guint               flags,
                                                               TasklistPlugin     *tasklist);
static TasklistPlugin *tasklist_plugin_new                    (XfcePanelPlugin    *panel_plugin);
static void            tasklist_plugin_screen_changed         (TasklistPlugin     *tasklist,
                                                               GdkScreen          *previous_screen);
static void            tasklist_plugin_orientation_changed    (TasklistPlugin     *tasklist,
                                                               GtkOrientation      orientation);
static gboolean        tasklist_plugin_size_changed           (TasklistPlugin     *tasklist,
                                                               guint               size);
static void            tasklist_plugin_size_request           (TasklistPlugin     *tasklist,
                                                               GtkRequisition     *requisition);
static void            tasklist_plugin_read                   (TasklistPlugin     *tasklist);
static void            tasklist_plugin_free                   (TasklistPlugin     *tasklist);
static void            tasklist_plugin_construct              (XfcePanelPlugin    *panel_plugin);



/* register with the panel */
XFCE_PANEL_PLUGIN_REGISTER (tasklist_plugin_construct);



gboolean
tasklist_using_xinerama (XfcePanelPlugin *panel_plugin)
{
    return (gdk_screen_get_n_monitors (gtk_widget_get_screen (GTK_WIDGET (panel_plugin))) > 1);
}



static gboolean
tasklist_handle_exposed (GtkWidget      *widget,
                         GdkEventExpose *event,
                         TasklistPlugin *tasklist)
{
    GtkOrientation orientation;
    gint           x, y, w, h;

    if (GTK_WIDGET_DRAWABLE (widget))
    {
        /* get the panel orientation */
        orientation = xfce_panel_plugin_get_orientation (tasklist->panel_plugin);

        /* set sizes */
        x = widget->allocation.x;
        y = widget->allocation.y;
        w = widget->allocation.width;
        h = widget->allocation.height;

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
                          &(event->area), widget, "handlebox",
                          x, y, w, h, orientation);

        return TRUE;
    }

    return FALSE;
}



static GdkPixbuf *
tasklist_icon_loader (const gchar    *name,
                      gint            size,
                      guint           flags,
                      TasklistPlugin *tasklist)
{
    GdkPixbuf   *pixbuf = NULL;
    gchar       *base = NULL;
    const gchar *p;

    /* do nothing on invalid names */
    if (G_UNLIKELY (name == NULL || *name == '\0'))
        return NULL;

    if (g_path_is_absolute (name))
    {
        if (g_file_test (name, G_FILE_TEST_EXISTS))
        {
            /* directly load the file */
            pixbuf = gdk_pixbuf_new_from_file_at_size (name, size, size, NULL);
        }
        else
        {
            /* get the base name */
            base = g_path_get_basename (name);

            /* use this function to try again */
            pixbuf = tasklist_icon_loader (base, size, flags, tasklist);

            /* cleanup */
            g_free (base);
        }
    }
    else
    {
        /* strip prefix */
        p = strrchr (name, '.');
        if (G_UNLIKELY (p))
            base = g_strndup (name, p - name);

        /* load the icon */
        pixbuf = gtk_icon_theme_load_icon (tasklist->icon_theme, base ? base : name, size, 0, NULL);

        /* cleanup */
        g_free (base);
    }

    return pixbuf;
}



static TasklistPlugin *
tasklist_plugin_new (XfcePanelPlugin *panel_plugin)
{
    TasklistPlugin *tasklist;
    GdkScreen      *screen;
    gint            screen_n;

    /* allocate structure */
    tasklist = g_slice_new0 (TasklistPlugin);

    /* init data */
    tasklist->panel_plugin = panel_plugin;

    /* read settings */
    tasklist_plugin_read (tasklist);

    /* create hvbox */
    tasklist->box = xfce_hvbox_new (xfce_panel_plugin_get_orientation (panel_plugin), FALSE, 0);
    gtk_container_add (GTK_CONTAINER (panel_plugin), tasklist->box);
    gtk_widget_show (tasklist->box);

    /* create handle */
    tasklist->handle = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
    gtk_widget_set_size_request (tasklist->handle, TASKLIST_HANDLE_SIZE, TASKLIST_HANDLE_SIZE);
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->handle, FALSE, FALSE, 0);
    g_signal_connect (tasklist->handle, "expose-event", G_CALLBACK (tasklist_handle_exposed), tasklist);
    if (tasklist->show_handles)
        gtk_widget_show (tasklist->handle);

    /* get the current screen number */
    screen = gtk_widget_get_screen (GTK_WIDGET (panel_plugin));
    screen_n = gdk_screen_get_number (screen);

    /* set the icon theme */
    tasklist->icon_theme = gtk_icon_theme_get_for_screen (screen);

    /* create tasklist */
    tasklist->list = wnck_tasklist_new (wnck_screen_get (screen_n));
    gtk_box_pack_start (GTK_BOX (tasklist->box), tasklist->list, FALSE, FALSE, 0);
    gtk_widget_show (tasklist->list);

    /* set the tasklist settings */
    wnck_tasklist_set_include_all_workspaces (WNCK_TASKLIST (tasklist->list), tasklist->all_workspaces);
    wnck_tasklist_set_grouping (WNCK_TASKLIST (tasklist->list), tasklist->grouping);
    wnck_tasklist_set_button_relief (WNCK_TASKLIST (tasklist->list), tasklist->flat_buttons ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);
    wnck_tasklist_set_icon_loader (WNCK_TASKLIST (tasklist->list), (WnckLoadIconFunction) tasklist_icon_loader, tasklist, NULL);

    return tasklist;
}



static void
tasklist_plugin_screen_changed (TasklistPlugin *tasklist,
                                GdkScreen      *previous_screen)
{
    GdkScreen  *screen;
    WnckScreen *wnck_screen;

    /* get the new screen */
    screen = gtk_widget_get_screen (GTK_WIDGET (tasklist->panel_plugin));
    if (G_UNLIKELY (screen == NULL))
        screen = gdk_screen_get_default ();

    /* get the wnck screen */
    wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));

    /* set the new tasklist screen */
    wnck_tasklist_set_screen (WNCK_TASKLIST (tasklist->list), wnck_screen);

    /* set the icon theme */
    tasklist->icon_theme = gtk_icon_theme_get_for_screen (screen);
}



static void
tasklist_plugin_orientation_changed (TasklistPlugin *tasklist,
                                     GtkOrientation orientation)
{
    /* set the new orientation of the hvbox */
    xfce_hvbox_set_orientation (XFCE_HVBOX (tasklist->box), orientation);

    /* redraw the handle */
    gtk_widget_queue_draw (tasklist->handle);
}



gboolean
tasklist_plugin_size_changed (TasklistPlugin *tasklist,
                              guint           size)
{
    /* size is handled in the size_request function */
    return TRUE;
}



static void
tasklist_plugin_size_request (TasklistPlugin *tasklist,
                              GtkRequisition *requisition)
{
    const gint     *size_hints;
    gint            length;
    gint            size;
    GtkOrientation  orientation;

    /* get the size hints */
    size_hints = wnck_tasklist_get_size_hint_list (WNCK_TASKLIST (tasklist->list), &length);

    /* check for pairs of 2 */
    if (G_LIKELY (length > 0))
    {
        /* get the first size */
        size = size_hints[0];

        /* add the handle size */
        if (tasklist->show_handles)
            size += TASKLIST_HANDLE_SIZE;

        /* use the requested size when it is bigger then the prefered size */
        if (tasklist->width > size)
           size = tasklist->width;

        /* get plugin orientation */
        orientation = xfce_panel_plugin_get_orientation (tasklist->panel_plugin);

        /* set the panel size */
        requisition->width = requisition->height = xfce_panel_plugin_get_size (tasklist->panel_plugin);

        /* set the requested plugin size */
        if (orientation == GTK_ORIENTATION_HORIZONTAL)
            requisition->width = size;
        else
            requisition->height = size;

        /* save the requested size */
        tasklist->req_size = size;
    }
}


static void
tasklist_plugin_size_allocate (TasklistPlugin *tasklist,
                               GtkAllocation  *allocation)
{
  GtkOrientation  orientation;
  gint            a_size, p_size;

  /* get orientation */
  orientation = xfce_panel_plugin_get_orientation (tasklist->panel_plugin);

  /* get plugin size */
  p_size = xfce_panel_plugin_get_size (tasklist->panel_plugin);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    a_size = MIN (tasklist->req_size, allocation->width);
  else
    a_size = MIN (tasklist->req_size, allocation->height);

  if (tasklist->show_handles)
    a_size -= TASKLIST_HANDLE_SIZE;

  /* force the size request of the taskbar */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (tasklist->list), a_size, p_size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (tasklist->list), p_size, a_size);
}


static void
tasklist_plugin_read (TasklistPlugin *tasklist)
{
    gchar  *file;
    XfceRc *rc;

    /* set defaults */
    tasklist->grouping       = WNCK_TASKLIST_AUTO_GROUP;
    tasklist->all_workspaces = FALSE;
    tasklist->expand         = TRUE;
    tasklist->flat_buttons   = TRUE;
    tasklist->show_handles   = TRUE;
    tasklist->width          = 300;

    /* get rc file name */
    file = xfce_panel_plugin_lookup_rc_file (tasklist->panel_plugin);

    if (G_LIKELY (file))
    {
        /* open the file, readonly */
        rc = xfce_rc_simple_open (file, TRUE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* read settings */
            tasklist->grouping       = xfce_rc_read_int_entry  (rc, "grouping", tasklist->grouping);
            tasklist->all_workspaces = xfce_rc_read_bool_entry (rc, "all_workspaces", tasklist->all_workspaces);
            tasklist->flat_buttons   = xfce_rc_read_bool_entry (rc, "flat_buttons", tasklist->flat_buttons);
            tasklist->show_handles   = xfce_rc_read_bool_entry (rc, "show_handles", tasklist->show_handles);
            tasklist->width          = xfce_rc_read_int_entry  (rc, "width",tasklist->width);

            /* only set expand flag if xinerama is used */
            if (tasklist_using_xinerama (tasklist->panel_plugin))
                tasklist->expand = xfce_rc_read_bool_entry (rc, "expand", tasklist->expand);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



void
tasklist_plugin_write (TasklistPlugin *tasklist)
{
    gchar  *file;
    XfceRc *rc;

    /* get rc file name, create it if needed */
    file = xfce_panel_plugin_save_location (tasklist->panel_plugin, TRUE);

    if (G_LIKELY (file))
    {
        /* open the file, writable */
        rc = xfce_rc_simple_open (file, FALSE);

        /* cleanup */
        g_free (file);

        if (G_LIKELY (rc))
        {
            /* write settings */
            xfce_rc_write_int_entry (rc, "grouping", tasklist->grouping);
            xfce_rc_write_int_entry (rc, "width", tasklist->width);
            xfce_rc_write_bool_entry (rc, "all_workspaces", tasklist->all_workspaces);
            xfce_rc_write_bool_entry (rc, "expand", tasklist->expand);
            xfce_rc_write_bool_entry (rc, "flat_buttons", tasklist->flat_buttons);
            xfce_rc_write_bool_entry (rc, "show_handles", tasklist->show_handles);

            /* close the rc file */
            xfce_rc_close (rc);
        }
    }
}



static void
tasklist_plugin_free (TasklistPlugin *tasklist)
{
    GtkWidget *dialog;

    /* destroy the dialog */
    dialog = g_object_get_data (G_OBJECT (tasklist->panel_plugin), I_("dialog"));
    if (dialog)
        gtk_widget_destroy (dialog);

    /* disconnect screen changed signal */
    g_signal_handler_disconnect (G_OBJECT (tasklist->panel_plugin), tasklist->screen_changed_id);

    /* free slice */
    g_slice_free (TasklistPlugin, tasklist);
}



static void
tasklist_plugin_construct (XfcePanelPlugin *panel_plugin)
{
    TasklistPlugin *tasklist;

    /* create the tray panel plugin */
    tasklist = tasklist_plugin_new (panel_plugin);

    /* set the action widgets and show configure */
    xfce_panel_plugin_add_action_widget (panel_plugin, tasklist->handle);
    xfce_panel_plugin_menu_show_configure (panel_plugin);

    /* whether to expand the plugin */
    xfce_panel_plugin_set_expand (panel_plugin, tasklist->expand);

    /* connect plugin signals */
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "orientation-changed",
                              G_CALLBACK (tasklist_plugin_orientation_changed), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "size-changed",
                              G_CALLBACK (tasklist_plugin_size_changed), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "size-request",
                              G_CALLBACK (tasklist_plugin_size_request), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "size-allocate",
                              G_CALLBACK (tasklist_plugin_size_allocate), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "save",
                              G_CALLBACK (tasklist_plugin_write), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "free-data",
                              G_CALLBACK (tasklist_plugin_free), tasklist);
    g_signal_connect_swapped (G_OBJECT (panel_plugin), "configure-plugin",
                              G_CALLBACK (tasklist_dialogs_configure), tasklist);

    /* screen changed signal */
    tasklist->screen_changed_id =
        g_signal_connect_swapped (G_OBJECT (panel_plugin), "screen-changed",
                                  G_CALLBACK (tasklist_plugin_screen_changed), tasklist);
}
