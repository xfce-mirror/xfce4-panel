/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *  
 *  Copyright (C) 2003,2004 Olivier Fourdan (fourdan@xfce.org)
 *  Copyright (c) 2003,2004 Benedikt Meurer <benny@xfce.org>
 *  Copyright  ©  2005      Jasper Huijsmans <jasper@xfce.org>
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

#include <config.h>

#ifdef GDK_MULTIHEAD_SAFE
#undef GDK_MULTIHEAD_SAFE
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-client.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-panel-window.h"
#include "xfce-item.h"
#include "xfce-itembar.h"

#define CHANNEL                "taskbar"
#define HIDDEN_HEIGHT          3
#define HIDE_TIMEOUT           500
#define UNHIDE_TIMEOUT         100
#define TOP                    TRUE
#define BOTTOM                 FALSE
#define DEFAULT_HEIGHT         30
#define DEFAULT_WIDTH_PERCENT  100
#define DEFAULT_HORIZ_ALIGN    0

/* taskbar struct */
typedef struct _Taskbar Taskbar;

struct _Taskbar
{
    Display *dpy;
    int scr;
    GdkScreen *gscr;

    /* position */
    guint x, y, width, height;
    gint saved_width;
    gboolean position;
    guint width_percent;
    gboolean shrink;            /* shrink to fit */
    gint horiz_align;           /* <0 left, =0 center, >0 right */

    /* autohide */
    gint hide_timeout;
    gint unhide_timeout;
    gboolean autohide;
    gboolean hidden;

    /* main window */
    GtkWidget *win;
    GtkWidget *itembar;
    
    /* tasklist */
    gboolean show_tasklist;
    gboolean all_tasks;
    gboolean group_tasks;
    gboolean show_text;
    GtkWidget *tasklist;

    /* pager */
    gboolean show_pager;
    GtkWidget *pageralign;
    GtkWidget *pager;
    
    /* status area */
    GtkWidget *statusframe;
    GtkWidget *statusbox;
    
    /* status: systray */
    gboolean show_tray;
    XfceSystemTray *tray;
    gboolean tray_registered;
    GtkWidget *iconbox;

    /* status: time */
    gboolean show_time;
    gint time_id;
    GtkTooltips *timetips;
    GtkWidget *timebox;
    GtkWidget *timelabel;
};

/* global vars */
static McsClient *client = NULL;

static gboolean quit_signal = FALSE;

/* size and position */
static gint
taskbar_get_thickness (Taskbar * taskbar)
{
    GtkStyle *style;

    g_return_val_if_fail (taskbar != NULL, 2);
    style = gtk_widget_get_style (taskbar->win);
    if (style)
    {
        return style->ythickness;
    }
    g_warning ("Cannot get initial style");
    return 2;
}

static gint
taskbar_get_height (Taskbar * taskbar)
{
    g_return_val_if_fail (taskbar != NULL, DEFAULT_HEIGHT);
    if (taskbar->hidden)
    {
        return (2 * taskbar_get_thickness (taskbar) + 1);
    }
    return taskbar->height;
}

static gint
taskbar_get_width (Taskbar * taskbar)
{
    /* use previously calculated width when hidden */
    if (!taskbar->hidden)
    {
        if (!taskbar->shrink)
        {
            taskbar->saved_width =
                (taskbar->width * taskbar->width_percent) / 100.0;
        }
        else
        {
            GtkRequisition req;

            gtk_widget_size_request (taskbar->win, &req);
            
            taskbar->saved_width = req.width;
        }
    }
    
    return taskbar->saved_width;
}

static gint
taskbar_get_x (Taskbar * taskbar)
{
    if (taskbar->horiz_align < 0)
    {
        /* left */
        return taskbar->x;
    }
    else if (taskbar->horiz_align > 0)
    {
        /* right */
        return taskbar->x + (taskbar->width - taskbar_get_width (taskbar));
    }
    else
    {
        /* center */
        return taskbar->x + (taskbar->width -
                             taskbar_get_width (taskbar)) / 2;
    }
}

static void
taskbar_update_margins (Taskbar * taskbar)
{
    guint height;
    gulong data[12] = { 0, };

    g_return_if_fail (taskbar != NULL);

    if (taskbar->autohide)
    {
        height = (2 * taskbar_get_thickness (taskbar) + 1);
    }
    else
    {
        height = taskbar->height;
    }

    data[0]  = 0;                                /* left           */
    data[1]  = 0;                                /* right          */
    data[2]  = (taskbar->position) ? height : 0; /* top            */
    data[3]  = (taskbar->position) ? 0 : height; /* bottom         */
    data[4]  = 0;                                /* left_start_y   */
    data[5]  = 0;                                /* left_end_y     */
    data[6]  = 0;                                /* right_start_y  */
    data[7]  = 0;                                /* right_end_y    */
    data[8]  = taskbar_get_x (taskbar);          /* top_start_x    */
    data[9]  = taskbar_get_x (taskbar) 
               + taskbar_get_width (taskbar);    /* top_end_x      */
    data[10] = taskbar_get_x (taskbar);          /* bottom_start_x */
    data[11] = taskbar_get_x (taskbar) 
               + taskbar_get_width (taskbar);    /* bottom_end_x   */

    gdk_error_trap_push ();
    gdk_property_change (taskbar->win->window,
                         gdk_atom_intern ("_NET_WM_STRUT", FALSE),
                         gdk_atom_intern ("CARDINAL", FALSE), 32,
                         GDK_PROP_MODE_REPLACE, (guchar *) & data, 4);
    gdk_property_change (taskbar->win->window,
                         gdk_atom_intern ("_NET_WM_STRUT_PARTIAL", FALSE),
                         gdk_atom_intern ("CARDINAL", FALSE), 32,
                         GDK_PROP_MODE_REPLACE, (guchar *) & data, 12);
    gdk_error_trap_pop ();
}

static void
taskbar_position (Taskbar * taskbar)
{
    static int recursive = 0;
    XfcePanelWindow *win;
    GdkRectangle rect;
    int w, h;
    
    g_return_if_fail (taskbar != NULL);
    
    while (recursive > 0)
        g_usleep (1000);

    recursive++;
    
    win = XFCE_PANEL_WINDOW (taskbar->win);
    
    w = (!taskbar->shrink || taskbar->hidden) ? 
                taskbar_get_width (taskbar) : -1;
    h = taskbar_get_height (taskbar);

    gtk_widget_set_size_request (GTK_WIDGET (taskbar->win), w, h);
    
    gdk_screen_get_monitor_geometry (taskbar->gscr, 0, &rect);
        
    if (taskbar->position == TOP)
    {
        taskbar->y = rect.y;

        if (taskbar->shrink || taskbar->width_percent != 100)
            xfce_panel_window_set_show_border (win, FALSE, TRUE, TRUE, TRUE);
        else
            xfce_panel_window_set_show_border (win, FALSE, TRUE, FALSE, FALSE);
    }
    else
    {
        taskbar->y = rect.y + rect.height - taskbar->win->allocation.height;
        
        if (taskbar->shrink || taskbar->width_percent != 100)
            xfce_panel_window_set_show_border (win, TRUE, FALSE, TRUE, TRUE);
        else
            xfce_panel_window_set_show_border (win, TRUE, FALSE, FALSE, FALSE);
    }
    
    gtk_window_move (GTK_WINDOW (taskbar->win), taskbar_get_x (taskbar),
                     taskbar->y);
    
    if (GTK_WIDGET_REALIZED (taskbar->win))
    {
        taskbar_update_margins (taskbar);
    }
    
    recursive--;
}

static void
taskbar_change_size (Taskbar * taskbar, int height)
{
    if (taskbar->height != height)
    {
        taskbar->height = height;
        taskbar_position (taskbar);
    }
}

static void
taskbar_set_width_percent (Taskbar * taskbar, int width_percent)
{
    if (taskbar->width_percent != width_percent)
    {
        taskbar->width_percent = width_percent;
        taskbar_position (taskbar);
    }
}

static void
taskbar_set_horiz_align (Taskbar * taskbar, int horiz_align)
{
    if (taskbar->horiz_align != horiz_align)
    {
        taskbar->horiz_align = horiz_align;
        
        taskbar_position (taskbar);
    }
}

static gboolean
taskbar_size_allocate (GtkWidget * widget, GtkAllocation * allocation,
                       gpointer data)
{
    Taskbar *taskbar = (Taskbar *) data;
    static GtkAllocation alloc = {-1, -1, -1, -1};

    g_assert (data != NULL);

    if (!allocation)
        return FALSE;

    if (alloc.width == allocation->width && alloc.height == allocation->height)
        return FALSE;

    alloc = *allocation;
    
    taskbar_position (taskbar);
    
    return FALSE;
}

static void
taskbar_size_changed (GdkScreen * screen, Taskbar * taskbar)
{
    /*
     * NOTE: This breaks Xinerama, but anyway, Xinerama is not compatible
     * with Xrandr
     */
    taskbar->width = gdk_screen_get_width (screen);
    taskbar_position (taskbar);
}

/* autohide */
static void
taskbar_toggle_autohide (Taskbar * taskbar)
{
    g_return_if_fail (taskbar != NULL);
    
    if (taskbar->autohide)
    {
        taskbar->hidden = TRUE;
        gtk_widget_hide (taskbar->itembar);
        taskbar_position (taskbar);
    }
    else
    {
        taskbar->hidden = FALSE;
        gtk_widget_show (taskbar->itembar);
        taskbar_position (taskbar);
    }
}

static gboolean
taskbar_unhide_timeout (Taskbar * taskbar)
{
    g_return_val_if_fail (taskbar != NULL, FALSE);

    taskbar->hidden = FALSE;
    gtk_widget_show (taskbar->itembar);
    taskbar_position (taskbar);

    /* remove timeout */
    return FALSE;
}

static gboolean
taskbar_enter (GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    Taskbar *taskbar = (Taskbar *) data;

    if (!(taskbar->autohide))
    {
        return FALSE;
    }
    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        if (taskbar->hide_timeout)
        {
            g_source_remove (taskbar->hide_timeout);
            taskbar->hide_timeout = 0;
        }

        if (!taskbar->unhide_timeout)
        {
            taskbar->unhide_timeout =
                g_timeout_add (UNHIDE_TIMEOUT,
                               (GSourceFunc) taskbar_unhide_timeout, taskbar);
        }

    }

    return FALSE;
}

static gboolean
taskbar_hide_timeout (Taskbar * taskbar)
{
    g_return_val_if_fail (taskbar != NULL, FALSE);

    taskbar->hidden = TRUE;
    gtk_widget_hide (taskbar->itembar);
    taskbar_position (taskbar);

    /* remove timeout */
    return FALSE;
}

static gboolean
taskbar_leave (GtkWidget * widget, GdkEventCrossing * event, gpointer data)
{
    Taskbar *taskbar = (Taskbar *) data;

    if (!(taskbar->autohide))
    {
        return FALSE;
    }

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        if (taskbar->unhide_timeout)
        {
            g_source_remove (taskbar->unhide_timeout);
            taskbar->unhide_timeout = 0;
        }

        if (!taskbar->hide_timeout)
        {
            taskbar->hide_timeout =
                g_timeout_add (HIDE_TIMEOUT,
                               (GSourceFunc) taskbar_hide_timeout, taskbar);
        }
    }
    return FALSE;
}

/* tasklist */
static void
taskbar_toggle_tasklist (Taskbar * taskbar)
{
    g_return_if_fail (taskbar != NULL);
    if (taskbar->show_tasklist)
    {
        gtk_widget_show (taskbar->tasklist);
    }
    else
    {
        gtk_widget_hide (taskbar->tasklist);
    }
}


/* pager */
static void
taskbar_toggle_pager (Taskbar * taskbar)
{
    g_return_if_fail (taskbar != NULL);
    if (taskbar->show_pager)
    {
        gtk_widget_show (taskbar->pageralign);
    }
    else
    {
        gtk_widget_hide (taskbar->pageralign);
    }
}

/* status area */

/* status: systray */
static gboolean
register_tray (Taskbar * taskbar)
{
    GError *error = NULL;
    Screen *scr = GDK_SCREEN_XSCREEN (taskbar->gscr);

    if (xfce_system_tray_check_running (scr))
    {
        xfce_info (_("There is already a system tray running on this "
                     "screen"));
        return FALSE;
    }
    else if (!xfce_system_tray_register (taskbar->tray, scr, &error))
    {
        xfce_err (_("Unable to register system tray: %s"), error->message);
        g_error_free (error);

        return FALSE;
    }

    return TRUE;
}

static gboolean
taskbar_toggle_tray (Taskbar * taskbar)
{
    g_return_val_if_fail (taskbar != NULL, FALSE);

    if (taskbar->show_tray)
    {
        if (!taskbar->tray_registered)
        {
            taskbar->tray_registered = register_tray (taskbar);
        }

        taskbar->iconbox = gtk_hbox_new (FALSE, 3);
        gtk_box_pack_start (GTK_BOX (taskbar->statusbox), taskbar->iconbox,
                            FALSE, FALSE, 0);
        gtk_widget_show (taskbar->iconbox);
        gtk_widget_show (taskbar->statusframe);
    }
    else
    {
        if (taskbar->tray_registered)
        {
            xfce_system_tray_unregister (taskbar->tray);
            taskbar->tray_registered = FALSE;
        }

        if (taskbar->iconbox)
        {
            gtk_widget_destroy (taskbar->iconbox);
            taskbar->iconbox = NULL;

            if (!taskbar->show_time)
                gtk_widget_hide (taskbar->statusframe);
        }
    }

    /* called from an idle loop, so return FALSE to end loop */
    return FALSE;
}

static void
icon_docked (XfceSystemTray * tray, GtkWidget * icon, Taskbar * taskbar)
{
    if (taskbar->tray_registered)
    {
        gtk_box_pack_end (GTK_BOX (taskbar->iconbox), icon, 
                          FALSE, FALSE, 0);
        gtk_widget_show (icon);
        gtk_widget_queue_draw (icon);
    }
}

static void
icon_undocked (XfceSystemTray * tray, GtkWidget * icon, Taskbar * taskbar)
{
    if (taskbar->tray_registered)
    {
        gtk_container_remove (GTK_CONTAINER (taskbar->iconbox), icon);
    }
}

static void
message_new (XfceSystemTray * tray, GtkWidget * icon, glong id,
             glong timeout, const gchar * text)
{
    g_print ("++ notification: %s\n", text);
}

/* status: time */
static gboolean
update_time_label (Taskbar * taskbar)
{
    static int mday = -1;
    time_t ticks;
    struct tm *tm;
    char date_s[255];

    ticks = time (0);
    tm = localtime (&ticks);

    if (mday != tm->tm_mday)
    {
        mday = tm->tm_mday;
        /* Use format characters from strftime(3)
         * to get the proper string for your locale.
         * I used these:
         * %A  : full weekday name
         * %d  : day of the month
         * %B  : full month name
         * %Y  : four digit year
         */
        strftime (date_s, 255, _("%A %d %B %Y"), tm);

        /* Conversion to utf8
         * patch by Oliver M. Bolzer <oliver@fakeroot.net>
         */
        if (!g_utf8_validate (date_s, -1, NULL))
        {
            char *utf8date;
            
            utf8date = g_locale_to_utf8 (date_s, -1, NULL, NULL, NULL);
            gtk_tooltips_set_tip (taskbar->timetips, taskbar->timebox, 
                                  utf8date, NULL);
            g_free (utf8date);
        }
        else
        {
            gtk_tooltips_set_tip (taskbar->timetips, taskbar->timebox, 
                                  date_s, NULL);
        }
    }

    strftime (date_s, 255, _("%H:%M"), tm);

    if (!g_utf8_validate (date_s, -1, NULL))
    {
        char *utf8date;
        
        utf8date = g_locale_to_utf8 (date_s, -1, NULL, NULL, NULL);
        gtk_label_set_text (GTK_LABEL (taskbar->timelabel), utf8date);
        g_free (utf8date);
    }
    else
    {
        gtk_label_set_text (GTK_LABEL (taskbar->timelabel), date_s);
    }
    
    return TRUE;
}

static void
taskbar_toggle_time (Taskbar * taskbar)
{
    g_return_if_fail (taskbar != NULL);
    
    if (taskbar->time_id > 0)
    {
        g_source_remove (taskbar->time_id);
        taskbar->time_id = 0;
    }
    
    if (taskbar->show_time)
    {
        update_time_label (taskbar);
        gtk_widget_show (taskbar->timelabel);
        taskbar->time_id = g_timeout_add (5000, 
                                          (GSourceFunc)update_time_label,
                                          taskbar);
        gtk_widget_show (taskbar->statusframe);
    }
    else
    {
        gtk_widget_hide (taskbar->timelabel);

        if (!taskbar->show_tray)
            gtk_widget_hide (taskbar->statusframe);
    }
}

/* mcs */
static GdkFilterReturn
client_event_filter (GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
    if (mcs_client_process_event (client, (XEvent *) xevent))
        return GDK_FILTER_REMOVE;
    else
        return GDK_FILTER_CONTINUE;
}

static void
watch_cb (Window window, Bool is_start, long mask, void *cb_data)
{
    GdkWindow *gdkwin;

    gdkwin = gdk_window_lookup (window);

    if (is_start)
    {
        if (!gdkwin)
        {
            gdkwin = gdk_window_foreign_new (window);
        }
        else
        {
            g_object_ref (gdkwin);
        }
        gdk_window_add_filter (gdkwin, client_event_filter, cb_data);
    }
    else
    {
        g_assert (gdkwin);
        gdk_window_remove_filter (gdkwin, client_event_filter, cb_data);
        g_object_unref (gdkwin);
    }
}

static void
notify_cb (const char *name, const char *channel_name, McsAction action,
           McsSetting * setting, void *data)
{
    Taskbar *taskbar = (Taskbar *) data;

    g_return_if_fail (data != NULL);
    
    if (action == MCS_ACTION_DELETED)
        return;
    
    if (setting->type == MCS_TYPE_INT)
    {
        if (!strcmp (name, "Taskbar/Position"))
        {
            taskbar->position = setting->data.v_int ? TOP : BOTTOM;
            taskbar_position (taskbar);
        }
        else if (!strcmp (name, "Taskbar/Height"))
        {
            taskbar_change_size (taskbar, setting->data.v_int);
        }
        else if (!strcmp (name, "Taskbar/WidthPercent"))
        {
            taskbar_set_width_percent (taskbar, setting->data.v_int);
        }
        else if (!strcmp (name, "Taskbar/Shrink"))
        {
            taskbar->shrink = setting->data.v_int ? TRUE : FALSE;
            taskbar_position (taskbar);
        }
        else if (!strcmp (name, "Taskbar/HorizAlign"))
        {
            taskbar_set_horiz_align (taskbar, setting->data.v_int);
        }
        else if (!strcmp (name, "Taskbar/AutoHide"))
        {
            taskbar->autohide = setting->data.v_int ? TRUE : FALSE;
            taskbar_toggle_autohide (taskbar);
        }
        else if (!strcmp (name, "Taskbar/ShowTasklist"))
        {
            taskbar->show_tasklist = setting->data.v_int ? TRUE : FALSE;
            taskbar_toggle_tasklist (taskbar);
        }
        else if (!strcmp (name, "Taskbar/ShowPager"))
        {
            taskbar->show_pager = setting->data.v_int ? TRUE : FALSE;
            taskbar_toggle_pager (taskbar);
        }
        else if (!strcmp (name, "Taskbar/ShowTray"))
        {
            taskbar->show_tray = setting->data.v_int ? TRUE : FALSE;
            g_idle_add ((GSourceFunc) taskbar_toggle_tray, (taskbar));
        }
        else if (!strcmp (name, "Taskbar/ShowAllTasks"))
        {
            taskbar->all_tasks = setting->data.v_int ? TRUE : FALSE;
            netk_tasklist_set_include_all_workspaces (NETK_TASKLIST
                                                      (taskbar->
                                                       tasklist),
                                                      taskbar->
                                                      all_tasks);
        }
        else if (!strcmp (name, "Taskbar/GroupTasks"))
        {
            taskbar->group_tasks = setting->data.v_int ? TRUE : FALSE;
            netk_tasklist_set_grouping (NETK_TASKLIST
                                        (taskbar->tasklist),
                                        taskbar->
                                        group_tasks ?
                                        NETK_TASKLIST_ALWAYS_GROUP :
                                        NETK_TASKLIST_AUTO_GROUP);
        }
        else if (!strcmp (name, "Taskbar/ShowText"))
        {
            taskbar->show_text = setting->data.v_int ? TRUE : FALSE;
            netk_tasklist_set_show_label (NETK_TASKLIST
                                          (taskbar->tasklist),
                                          taskbar->show_text);
        }
        else if (!strcmp (name, "Taskbar/ShowTime"))
        {
            taskbar->show_time = setting->data.v_int ? TRUE : FALSE;
            taskbar_toggle_time (taskbar);
        }
    }
}

/* main window */
static void
sighandler (int sig)
{
    /* Don't do any reall stuff here.
     * Only set a signal state flag. There's a timeout in the main loop
     * that tests the flag.
     * This will prevent problems with gtk main loop threads and stuff
     */
    quit_signal = TRUE;
}

static gboolean
check_signal_state (void)
{
    if (quit_signal)
        gtk_main_quit ();

    /* keep running */
    return TRUE;
}

static void
quit (gpointer client_data)
{
    gtk_main_quit ();
}

int
main (int argc, char **argv)
{
    SessionClient *client_session;
    NetkScreen *screen;
    Taskbar *taskbar;
    GdkRectangle rect;
    gint monitor_nbr;
#ifdef HAVE_SIGACTION
    struct sigaction act;
#endif

    /* This is required for UTF-8 at least - Please don't remove it */
    xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    gtk_init (&argc, &argv);

    taskbar = g_new (Taskbar, 1);

    taskbar->win = xfce_panel_window_new ();

    taskbar->gscr = gtk_widget_get_screen (taskbar->win);
    if (!taskbar->gscr)
    {
        g_error ("Cannot get default screen\n");
    }
    
    taskbar->scr = gdk_screen_get_number (taskbar->gscr);
    taskbar->dpy =
        gdk_x11_display_get_xdisplay (gdk_screen_get_display (taskbar->gscr));

    screen = netk_screen_get (taskbar->scr);
    netk_screen_force_update (screen);
    
    monitor_nbr = gdk_screen_get_monitor_at_point (taskbar->gscr, 0, 0);
    gdk_screen_get_monitor_geometry (taskbar->gscr, monitor_nbr, &rect);

    /* initialize settings */
    taskbar->position        = TOP;
    taskbar->x               = rect.x;
    taskbar->y               = rect.y;
    taskbar->width           = rect.width;
    taskbar->height          = DEFAULT_HEIGHT;
    taskbar->width_percent   = DEFAULT_WIDTH_PERCENT;
    taskbar->saved_width     = taskbar->width;
    taskbar->shrink          = FALSE;
    taskbar->horiz_align     = DEFAULT_HORIZ_ALIGN;
    
    taskbar->autohide        = FALSE;
    taskbar->hide_timeout    = 0;
    taskbar->unhide_timeout  = 0;
    taskbar->hidden          = FALSE;

    taskbar->show_tasklist   = TRUE;
    taskbar->all_tasks       = FALSE;
    taskbar->group_tasks     = FALSE;
    taskbar->show_text       = TRUE;

    taskbar->show_pager      = TRUE;

    taskbar->show_tray       = TRUE;
    taskbar->tray_registered = FALSE;

    taskbar->show_time       = TRUE;
    taskbar->time_id         = 0;

    /* main window */
    xfce_panel_window_set_moveable (XFCE_PANEL_WINDOW (taskbar->win), FALSE);
    xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (taskbar->win),
                                        XFCE_HANDLE_STYLE_NONE);
    xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (taskbar->win),
                                       FALSE, TRUE, FALSE, FALSE);
    gtk_window_set_title (GTK_WINDOW (taskbar->win), _("Taskbar"));
    
    gtk_widget_set_size_request (GTK_WIDGET (taskbar->win), taskbar->width,
                                 taskbar->height);

    g_signal_connect (G_OBJECT (taskbar->win), "enter_notify_event",
                      G_CALLBACK (taskbar_enter), taskbar);
    g_signal_connect (G_OBJECT (taskbar->win), "leave_notify_event",
                      G_CALLBACK (taskbar_leave), taskbar);
    g_signal_connect (G_OBJECT (taskbar->win), "size_allocate",
                      G_CALLBACK (taskbar_size_allocate), taskbar);
    g_signal_connect_swapped (G_OBJECT (taskbar->win), "realize",
                              G_CALLBACK (taskbar_update_margins), taskbar);

    if (gdk_screen_get_n_monitors (taskbar->gscr) < 2)
    {
        /*
         * Xrandr compatibility, since Xrandr does not work well with
         * Xinerama, we only use it when theres no more than one Xinerama
         * screen reported.
         */
        g_signal_connect (G_OBJECT (taskbar->gscr), "size-changed",
                          G_CALLBACK (taskbar_size_changed), taskbar);
    }


    /* main hbox */
    taskbar->itembar = xfce_itembar_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (taskbar->itembar);
    gtk_container_add (GTK_CONTAINER (taskbar->win), taskbar->itembar);

    /* task list */
    taskbar->tasklist = netk_tasklist_new (screen);
    gtk_widget_set_size_request (taskbar->tasklist, 300, -1);
    gtk_widget_show (taskbar->tasklist);
    netk_tasklist_set_grouping (NETK_TASKLIST (taskbar->tasklist),
                                taskbar->group_tasks);
    netk_tasklist_set_include_all_workspaces (NETK_TASKLIST
                                              (taskbar->tasklist),
                                              taskbar->all_tasks);
    xfce_itembar_append (XFCE_ITEMBAR (taskbar->itembar), 
                         taskbar->tasklist);
    xfce_itembar_set_child_expand (XFCE_ITEMBAR (taskbar->itembar),
                                   taskbar->tasklist, TRUE);

    /* pager */
    taskbar->pageralign = gtk_alignment_new (0, 0, 0, 1);
    gtk_container_set_border_width (GTK_CONTAINER (taskbar->pageralign), 1);
    /* don't show by default */
    xfce_itembar_append (XFCE_ITEMBAR (taskbar->itembar), 
                         taskbar->pageralign);

    taskbar->pager = netk_pager_new (screen);
    gtk_widget_show (taskbar->pager);
    netk_pager_set_orientation (NETK_PAGER (taskbar->pager),
                                GTK_ORIENTATION_HORIZONTAL);
    netk_pager_set_n_rows (NETK_PAGER (taskbar->pager), 1);
    gtk_container_add (GTK_CONTAINER (taskbar->pageralign), taskbar->pager);
    
    /* status area */
    taskbar->statusframe = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (taskbar->statusframe), GTK_SHADOW_IN);
    gtk_container_set_border_width (GTK_CONTAINER (taskbar->statusframe), 2);
    gtk_widget_show (taskbar->statusframe);
    xfce_itembar_append (XFCE_ITEMBAR (taskbar->itembar), 
                         taskbar->statusframe);
    
    taskbar->statusbox = gtk_hbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (taskbar->statusbox), 1);
    gtk_widget_show (taskbar->statusbox);
    gtk_container_add (GTK_CONTAINER (taskbar->statusframe), 
                       taskbar->statusbox);
    
    /* systray */
    taskbar->tray = xfce_system_tray_new ();

    taskbar->iconbox = NULL;

    g_signal_connect (taskbar->tray, "icon_docked", G_CALLBACK (icon_docked),
                      taskbar);
    g_signal_connect (taskbar->tray, "icon_undocked",
                      G_CALLBACK (icon_undocked), taskbar);
    g_signal_connect (taskbar->tray, "message_new", G_CALLBACK (message_new),
                      taskbar);

    /* time */
    taskbar->timetips = gtk_tooltips_new ();
    
    taskbar->timebox = gtk_event_box_new ();
    gtk_container_set_border_width (GTK_CONTAINER (taskbar->timebox), 2);
    gtk_widget_show (taskbar->timebox);
    gtk_box_pack_end (GTK_BOX (taskbar->statusbox), taskbar->timebox, 
                      FALSE, FALSE, 0);
    
    taskbar->timelabel = gtk_label_new (NULL);
    gtk_widget_show (taskbar->timelabel);
    gtk_container_add (GTK_CONTAINER (taskbar->timebox), taskbar->timelabel);
    
    taskbar_toggle_time (taskbar);

    /* session */
    client_session =
        client_session_new (argc, argv, NULL /* data */ ,
                            SESSION_RESTART_IF_RUNNING, 30);

    /* settings */
    client =
        mcs_client_new (taskbar->dpy, taskbar->scr, notify_cb, watch_cb,
                        taskbar);
    if (client)
    {
        mcs_client_add_channel (client, CHANNEL);
    }
    else
    {
        g_warning (_("Cannot create MCS client channel"));
        taskbar_toggle_tray (taskbar);
        taskbar_toggle_time (taskbar);
    }

    /* size and position */
    gtk_widget_realize (taskbar->win);
    taskbar_position (taskbar);

    gtk_widget_show (taskbar->win);

#ifdef HAVE_SIGACTION
    act.sa_handler = sighandler;
    sigemptyset (&act.sa_mask);
#ifdef SA_RESTART
    act.sa_flags = SA_RESTART;
#else
    act.sa_flags = 0;
#endif
    sigaction (SIGHUP, &act, NULL);
    sigaction (SIGUSR1, &act, NULL);
    sigaction (SIGUSR2, &act, NULL);
    sigaction (SIGINT, &act, NULL);
    sigaction (SIGTERM, &act, NULL);
#else
    signal (SIGHUP, sighandler);
    signal (SIGUSR1, sighandler);
    signal (SIGUSR2, sighandler);
    signal (SIGINT, sighandler);
    signal (SIGTERM, sighandler);
#endif

    g_timeout_add (500, (GSourceFunc) check_signal_state, NULL);

    client_session->die = quit;

    session_init (client_session);
    
    gtk_main ();

    if (taskbar->tray_registered)
        taskbar_toggle_tray (taskbar);

    g_free (taskbar);

    return 0;
}
