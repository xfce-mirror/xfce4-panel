/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-window.h>
#include <libxfce4panel/xfce-itembar.h>
#include <libxfce4panel/xfce-panel-item-iface.h>
#include <libxfce4panel/xfce-panel-enums.h>
#include <libxfce4panel/xfce-panel-macros.h>

#include "panel-properties.h"
#include "panel-private.h"
#include "panel-dnd.h"

#define HIDDEN_SIZE     2
#define OPAQUE          0xffffffff
#define HIDE_TIMEOUT    350
#define UNHIDE_TIMEOUT  200



/* prototypes */
static void     panel_move_end          (XfcePanelWindow  *window,
                                         gint              x,
                                         gint              y);
static void     panel_move_function     (XfcePanelWindow  *window,
                                         Panel            *panel,
                                         gint             *x,
                                         gint             *y);
static void     panel_resize_function   (XfcePanelWindow  *window,
                                         Panel            *panel,
                                         GtkAllocation    *alloc_old,
                                         GtkAllocation    *alloc_new,
                                         gint             *x,
                                         gint             *y);
static void     panel_enter             (Panel            *p,
                                         GdkEventCrossing *event);
static void     panel_leave             (Panel            *p,
                                         GdkEventCrossing *event);



/* moving and resizing */
static void
_calculate_coordinates (XfceScreenPosition  position, /* screen position */
                        GdkRectangle       *screen,   /* monitor geometry */
                        gint                width,    /* panel size */
                        gint                height,
                        gint                *x,       /* output coordinates */
                        gint                *y)
{
    switch (position)
    {
        /* floating */
        case XFCE_SCREEN_POSITION_NONE:
        case XFCE_SCREEN_POSITION_FLOATING_H:
        case XFCE_SCREEN_POSITION_FLOATING_V:
            *x = CLAMP (*x, screen->x,
                        MAX (screen->x, screen->x + screen->width - width));
            *y = CLAMP (*y, screen->y,
                        MAX (screen->y, screen->y + screen->height - height));
            break;

        /* top */
        case XFCE_SCREEN_POSITION_NW_H:
            *x = screen->x;
            *y = screen->y;
            break;

        case XFCE_SCREEN_POSITION_N:
            *x = MAX (screen->x, screen->x + (screen->width - width) / 2);
            *y = screen->y;
            break;

        case XFCE_SCREEN_POSITION_NE_H:
            *x = MAX (screen->x, screen->x + screen->width - width);
            *y = screen->y;
            break;

        /* left */
        case XFCE_SCREEN_POSITION_NW_V:
            *x = screen->x;
            *y = screen->y;
            break;

        case XFCE_SCREEN_POSITION_W:
            *x = screen->x;
            *y = MAX (screen->y, screen->y + (screen->height - height) / 2);
            break;

        case XFCE_SCREEN_POSITION_SW_V:
            *x = screen->x;
            *y = MAX (screen->y, screen->y + screen->height - height);
            break;

        /* right */
        case XFCE_SCREEN_POSITION_NE_V:
            *x = screen->x + screen->width - width;
            *y = screen->y;
            break;

        case XFCE_SCREEN_POSITION_E:
            *x = screen->x + screen->width - width;
            *y = MAX (screen->y, screen->y + (screen->height - height) / 2);
            break;

        case XFCE_SCREEN_POSITION_SE_V:
            *x = screen->x + screen->width - width;
            *y = MAX (screen->y, screen->y + screen->height - height);
            break;

        /* bottom */
        case XFCE_SCREEN_POSITION_SW_H:
            *x = screen->x;
            *y = screen->y + screen->height - height;
            break;

        case XFCE_SCREEN_POSITION_S:
            *x = MAX (screen->x, screen->x + (screen->width - width) / 2);
            *y = screen->y + screen->height - height;
            break;

        case XFCE_SCREEN_POSITION_SE_H:
            *x = MAX (screen->x, screen->x + screen->width - width);
            *y = screen->y + screen->height - height;
            break;
    }
}



static void
_set_borders (Panel *panel)
{
    PanelPrivate *priv;
    gboolean      top, bottom, left, right;

    priv = panel->priv;

    top = bottom = left = right = TRUE;

    switch (priv->screen_position)
    {
        /* floating */
        case XFCE_SCREEN_POSITION_NONE:
        case XFCE_SCREEN_POSITION_FLOATING_H:
        case XFCE_SCREEN_POSITION_FLOATING_V:
            break;

        /* top */
        case XFCE_SCREEN_POSITION_NW_H:
            top = left = FALSE;
            right = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_N:
            top = FALSE;
            left = right = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_NE_H:
            top = right = FALSE;
            left = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        /* left */
        case XFCE_SCREEN_POSITION_NW_V:
            left = top = FALSE;
            bottom = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_W:
            left = FALSE;
            top = bottom = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_SW_V:
            left = bottom = FALSE;
            top = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        /* right */
        case XFCE_SCREEN_POSITION_NE_V:
            right = top = FALSE;
            bottom = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_E:
            right = FALSE;
            top = bottom = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_SE_V:
            right = bottom = FALSE;
            top = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        /* bottom */
        case XFCE_SCREEN_POSITION_SW_H:
            bottom = left = FALSE;
            right = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_S:
            bottom = FALSE;
            left = right = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;

        case XFCE_SCREEN_POSITION_SE_H:
            bottom = right = FALSE;
            left = (priv->full_width == XFCE_PANEL_NORMAL_WIDTH);
            break;
    }

    xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (panel),
                                       top, bottom, left, right);
}

/* fairly arbitrary maximum strut size */
static gboolean
_validate_strut( gint width,
                 gint offset )
{
    return ( offset < width / 4 );
}

static void
_set_struts (Panel       *panel,
             XfceMonitor *xmon,
             gint         x,
             gint         y,
             gint         w,
             gint         h)
{
    PanelPrivate *priv;
    gulong        data[12] = { 0, };
    gint          i;
    gboolean      update = FALSE;

    /* _NET_WM_STRUT_PARTIAL: CARDINAL[12]/32
     *
     * 0: left          1: right       2:  top             3:  bottom
     * 4: left_start_y  5: left_end_y  6:  right_start_y   7:  right_end_y
     * 8: top_start_x   9: top_end_x   10: bottom_start_x  11: bottom_end_x
     *
     * Note: In xinerama use struts relative to combined screen dimensions,
     *       not just the current monitor.
     */

    priv = panel->priv;

    /* use edit_mode property to test if we are changing position */
    if (G_UNLIKELY (priv->edit_mode))
        return;

    if (!priv->autohide &&
            !xfce_screen_position_is_floating (priv->screen_position))
    {
        if (xfce_screen_position_is_left (priv->screen_position))
        {
            /* no struts possible on Xinerama screens when this monitor
             * has neighbors on the left (see fd.o spec).
             */
            if (!xmon->has_neighbor_left
                && _validate_strut (xmon->geometry.width, w))
            {
                data[0] = xmon->geometry.x + w; /* left           */
                data[4] = y;                    /* left_start_y   */
                data[5] = y + h - 1;            /* left_end_y     */
            }
        }
        else if (xfce_screen_position_is_right (priv->screen_position))
        {
            /* no struts possible on Xinerama screens when this monitor
             * has neighbors on the right (see fd.o spec).
             */
            if (!xmon->has_neighbor_right
                && _validate_strut (xmon->geometry.width, w))
            {
                data[1] =  gdk_screen_get_width (xmon->screen)
                           - xmon->geometry.x - xmon->geometry.width
                           + w;         /* right          */
                data[6] = y;            /* right_start_y  */
                data[7] = y + h - 1;    /* right_end_y    */
            }
        }
        else if (xfce_screen_position_is_top (priv->screen_position))
        {
            /* no struts possible on Xinerama screens when this monitor
             * has neighbors on the top (see fd.o spec).
             */
            if (!xmon->has_neighbor_above
                && _validate_strut (xmon->geometry.height, h))
            {
                data[2] = xmon->geometry.y
                          + h;          /* top            */
                data[8] = x;            /* top_start_x    */
                data[9] = x + w - 1;    /* top_end_x      */
            }
        }
        else
        {
            /* no struts possible on Xinerama screens when this monitor
             * has neighbors on the bottom (see fd.o spec).
             */
            if (!xmon->has_neighbor_below
                && _validate_strut (xmon->geometry.height, h))
            {
                data[3] = gdk_screen_get_height (xmon->screen)
                           - xmon->geometry.y - xmon->geometry.height
                           + h;         /* bottom         */
                data[10] = x;           /* bottom_start_x */
                data[11] = x + w - 1;   /* bottom_end_x   */
            }
        }
    }

    DBG ("\nStruts\n"
         "%ld\t%ld\t%ld\t%ld\n"
         "%ld\t%ld\t%ld\t%ld\n"
         "%ld\t%ld\t%ld\t%ld\n",
         data[0], data[1], data[2], data[3],
         data[4], data[5], data[6], data[7],
         data[8], data[9], data[10], data[11]);

    /* Check if values have changed. */
    for (i = 0; i < 12; i++)
    {
        if (data[i] != priv->struts[i])
        {
            update = TRUE;
            priv->struts[i] = data[i];
        }
    }

    if (update)
    {
        gdk_error_trap_push ();
        gdk_property_change (GTK_WIDGET (panel)->window,
                             gdk_atom_intern ("_NET_WM_STRUT_PARTIAL", FALSE),
                             gdk_atom_intern ("CARDINAL", FALSE), 32,
                             GDK_PROP_MODE_REPLACE, (guchar *) & data, 12);
        gdk_error_trap_pop ();

        gdk_error_trap_push ();
        gdk_property_change (GTK_WIDGET (panel)->window,
                             gdk_atom_intern ("_NET_WM_STRUT", FALSE),
                             gdk_atom_intern ("CARDINAL", FALSE), 32,
                             GDK_PROP_MODE_REPLACE, (guchar *) & data, 4);
        gdk_error_trap_pop ();
    }

    DBG ("all struts are checked and updated");
}

static gboolean
unblock_struts (Panel *panel)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;
    gint          x, y, w, h;

    priv = PANEL (panel)->priv;
    priv->edit_mode = FALSE;

    gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
    gtk_window_get_size (GTK_WINDOW (panel), &w, &h);

    xmon = panel_app_get_monitor (priv->monitor);

    _set_struts (panel, xmon, x, y, w, h);
    return FALSE;
}

static void
panel_move_end (XfcePanelWindow *window,
                gint             x,
                gint             y)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;

    priv = PANEL (window)->priv;

    if (xfce_screen_position_is_floating (priv->screen_position))
    {
        /* Emit this signal to allow plugins to do something based on the
         * new position. E.g. the launcher plugin adjusts the arrow type
         * of the associated menu button. */
        gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                               (GtkCallback)xfce_panel_item_set_screen_position,
                                GINT_TO_POINTER (priv->screen_position));
    }

    xmon = panel_app_get_monitor (priv->monitor);

    priv->xoffset = x - xmon->geometry.x;
    priv->yoffset = y - xmon->geometry.y;

    DBG ("\n + Position: %d\n + Offset: (%d, %d)",
         priv->screen_position, priv->xoffset, priv->yoffset);

    panel_app_queue_save ();
}

static void
panel_move_function (XfcePanelWindow *window,
                     Panel           *panel,
                     gint            *x,
                     gint            *y)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;
    GdkScreen    *screen;

    priv = panel->priv;

    screen = gtk_widget_get_screen (GTK_WIDGET (window));
    priv->monitor = gdk_screen_get_monitor_at_point (screen, *x, *y);

    DBG (" + Monitor at (%d, %d) = %d\n", *x, *y, priv->monitor);

    xmon = panel_app_get_monitor (priv->monitor);

    _calculate_coordinates (priv->screen_position,
                            &(xmon->geometry),
                            GTK_WIDGET (window)->allocation.width,
                            GTK_WIDGET (window)->allocation.height,
                            x, y);
}

static void
panel_resize_function (XfcePanelWindow *window,
                       Panel           *panel,
                       GtkAllocation   *alloc_old,
                       GtkAllocation   *alloc_new,
                       gint            *x,
                       gint            *y)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;
    GdkRectangle  rect;

    DBG ("old: %dx%d\tnew: %dx%d",
         alloc_old ? alloc_old->width : 0, alloc_old ? alloc_old->height : 0,
         alloc_new->width, alloc_new->height );

    if (!GTK_WIDGET_VISIBLE (panel))
        return;

    priv = panel->priv;
    xmon = panel_app_get_monitor (priv->monitor);

    if (priv->full_width < XFCE_PANEL_SPAN_MONITORS)
    {
        _calculate_coordinates (priv->screen_position,
                                &(xmon->geometry),
                                alloc_new->width,
                                alloc_new->height,
                                x, y);

        priv->xoffset = *x - xmon->geometry.x;
        priv->yoffset = *y - xmon->geometry.y;
    }
    else
    {
        rect.x      = 0;
        rect.y      = 0;
        rect.width  = gdk_screen_get_width (xmon->screen);
        rect.height = gdk_screen_get_height (xmon->screen);

        _calculate_coordinates (priv->screen_position,
                                &rect,
                                alloc_new->width,
                                alloc_new->height,
                                x, y);

        priv->xoffset = *x;
        priv->yoffset = *y;
    }

    _set_struts (panel, xmon, *x, *y, alloc_new->width, alloc_new->height);
}

static void
panel_set_position (Panel              *panel,
                    XfceScreenPosition  position,
                    gint                xoffset,
                    gint                yoffset)
{
    PanelPrivate   *priv;
    GtkRequisition  req;
    gint            x, y;
    XfceMonitor    *xmon;
    GdkRectangle    rect;

    if (!GTK_WIDGET_VISIBLE (panel))
        return;

    priv = panel->priv;
    xmon = panel_app_get_monitor (priv->monitor);

    priv->screen_position = position;
    _set_borders (panel);

    gtk_widget_size_request (GTK_WIDGET (panel), &req);

    x = y = 0;

    /* NOTE: xoffset and yoffset are only used for floating panels */
    if (xfce_screen_position_is_floating (position))
    {
        if (G_UNLIKELY (xoffset < 0 || yoffset < 0))
        {
            /* This means we switched to being a floating panel;
             * calculate the actual position here. */
            gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
        }
        else
        {
            x = xmon->geometry.x + xoffset;
            y = xmon->geometry.y + yoffset;
        }
    }

    if (priv->full_width < XFCE_PANEL_SPAN_MONITORS)
    {
        _calculate_coordinates (position,
                                &(xmon->geometry),
                                req.width,
                                req.height,
                                &x, &y);

        priv->xoffset = x - xmon->geometry.x;
        priv->yoffset = y - xmon->geometry.y;
    }
    else
    {
        rect.x      = 0;
        rect.y      = 0;
        rect.width  = gdk_screen_get_width (xmon->screen);
        rect.height = gdk_screen_get_height (xmon->screen);

        _calculate_coordinates (position,
                                &rect,
                                req.width,
                                req.height,
                                &x, &y);

        priv->xoffset = x;
        priv->yoffset = y;
    }

    DBG ("\n + Position: %d\n + Offset: (%d, %d)",
         priv->screen_position, priv->xoffset, priv->yoffset);

    DBG (" + coordinates: (%d, %d)\n", x, y);

    gtk_window_move (GTK_WINDOW (panel), x, y);

    _set_struts (panel, xmon, x, y, req.width, req.height);
}

/* screen size */
void
panel_screen_size_changed (GdkScreen *screen,
                           Panel     *panel)
{
    PanelPrivate       *priv;
    XfceMonitor        *xmon;
    XfcePanelWidthType  width;

    priv = panel->priv;

    xmon = panel_app_get_monitor (priv->monitor);

    gdk_screen_get_monitor_geometry (screen,
                                     xmon->num,
                                     &(xmon->geometry));

    /* new size constraints */
    if ((width = priv->full_width) != XFCE_PANEL_NORMAL_WIDTH)
    {
        priv->full_width = XFCE_PANEL_NORMAL_WIDTH;
        panel_set_full_width (panel, width);
    }
    else
    {
        gtk_widget_queue_resize (GTK_WIDGET (panel));
    }
}

/* transparency and autohide */
static void
_set_transparent (Panel    *panel,
                  gboolean  transparent)
{
    PanelPrivate *priv;
    gulong        opacity;

    if (!GTK_WIDGET (panel)->window)
        return;

    priv = panel->priv;

    if (G_UNLIKELY (priv->opacity == 0))
    {
        if (priv->transparency != 0)
        {
            priv->opacity =
                OPAQUE - rint ((gdouble)priv->transparency * OPAQUE / 100);
        }
        else
        {
            priv->opacity = OPAQUE;
        }
    }

    opacity = (transparent || priv->activetrans) ? priv->opacity : OPAQUE;

    if (opacity != priv->saved_opacity)
    {
        priv->saved_opacity = opacity;

        gdk_error_trap_push ();

        gdk_property_change (GTK_WIDGET (panel)->window,
                             gdk_atom_intern ("_NET_WM_WINDOW_OPACITY", FALSE),
                             gdk_atom_intern ("CARDINAL", FALSE), 32,
                             GDK_PROP_MODE_REPLACE,
                             (guchar *) & opacity,
                             1L);

        gdk_error_trap_pop ();

        gtk_widget_queue_draw (GTK_WIDGET (panel));
    }
}

void
panel_set_hidden (Panel    *panel,
                  gboolean  hide)
{
    PanelPrivate *priv;
    gint          w, h;
    XfceMonitor  *xmon;

    priv = panel->priv;

    priv->hidden = hide;
    w = h = 0;

    if (hide)
    {
        if (xfce_screen_position_is_horizontal (priv->screen_position))
        {
            w = GTK_WIDGET (panel)->allocation.width;
            h = HIDDEN_SIZE;
        }
        else
        {
            w = HIDDEN_SIZE;
            h = GTK_WIDGET (panel)->allocation.height;
        }

        gtk_widget_hide (priv->itembar);
    }
    else
    {
        xmon = panel_app_get_monitor (priv->monitor);

        switch (priv->full_width)
        {
            case XFCE_PANEL_NORMAL_WIDTH:
                w = h = -1;
                break;

            case XFCE_PANEL_FULL_WIDTH:
            case XFCE_PANEL_SPAN_MONITORS:
                if (xfce_screen_position_is_horizontal (priv->screen_position))
                {
                    h = -1;
                    if (priv->full_width == XFCE_PANEL_FULL_WIDTH)
                        w = xmon->geometry.width;
                    else
                        w = gdk_screen_get_width (xmon->screen);
                }
                else
                {
                    w = -1;
                    if (priv->full_width == XFCE_PANEL_FULL_WIDTH)
                        h = xmon->geometry.height;
                    else
                        h = gdk_screen_get_height (xmon->screen);
                }
                break;
        }

        gtk_widget_show (priv->itembar);
    }

    gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);
}

static gboolean
_hide_timeout (Panel *panel)
{
    PanelPrivate *priv;

    priv = panel->priv;
    priv->hide_timeout = 0;

    if (!priv->hidden && !priv->block_autohide)
        panel_set_hidden (panel, TRUE);

    return FALSE;
}

static gboolean
_unhide_timeout (Panel *panel)
{
    PanelPrivate *priv;

    priv = panel->priv;
    priv->unhide_timeout = 0;

    if (priv->hidden)
        panel_set_hidden (panel, FALSE);

    return FALSE;
}

static gboolean
drag_motion (Panel          *panel,
             GdkDragContext *context,
             int             x,
             int             y,
             guint           time_,
             gpointer        user_data)
{
    PanelPrivate *priv;

    priv = panel->priv;

    if (!priv->hidden || priv->block_autohide)
        return TRUE;

    if (priv->hide_timeout)
    {
        g_source_remove (priv->hide_timeout);
        priv->hide_timeout = 0;
    }

    if (!priv->unhide_timeout)
    {
        priv->unhide_timeout =
            g_timeout_add (UNHIDE_TIMEOUT,
                           (GSourceFunc) _unhide_timeout, panel);
    }

    return TRUE;
}


static void
drag_leave (Panel          *panel,
            GdkDragContext *drag_context,
            guint           time_,
            gpointer        user_data)
{
    int    x, y, w, h, px, py;
    PanelPrivate *priv;

    priv = panel->priv;

    if (!priv->autohide || priv->hidden || priv->block_autohide)
        return;

    /* check if pointer is inside the window */
    gdk_display_get_pointer (gdk_display_get_default (), NULL, &px, &py, NULL);
    gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
    gtk_window_get_size (GTK_WINDOW (panel), &w, &h);

    if (px < x || px > x + w || py < y || py > y + h)
    {
        if (priv->unhide_timeout)
        {
            g_source_remove (priv->unhide_timeout);
            priv->unhide_timeout = 0;
        }

        if (!priv->hide_timeout)
        {
            priv->hide_timeout =
                g_timeout_add (HIDE_TIMEOUT,
                               (GSourceFunc) _hide_timeout, panel);
        }
    }
}


static void
panel_enter (Panel            *panel,
             GdkEventCrossing *event)
{
    PanelPrivate *priv;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        panel_app_set_current_panel ((gpointer)panel);

        priv = panel->priv;

        if (priv->block_autohide)
            return;

        _set_transparent (panel, FALSE);

        if (!priv->autohide)
            return;

        if (priv->hide_timeout)
        {
            g_source_remove (priv->hide_timeout);
            priv->hide_timeout = 0;
        }

        if (priv->hidden)
            priv->unhide_timeout = g_timeout_add (UNHIDE_TIMEOUT,
                                                  (GSourceFunc)_unhide_timeout,
                                                  panel);
    }
}


static void
panel_leave (Panel            *panel,
             GdkEventCrossing *event)
{
    PanelPrivate *priv;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        priv = panel->priv;

        if (priv->block_autohide)
            return;

        _set_transparent (panel, TRUE);

        if (!priv->autohide)
            return;

        if (priv->unhide_timeout)
        {
            g_source_remove (priv->unhide_timeout);
            priv->unhide_timeout = 0;
        }

        if (!priv->hidden)
            priv->hide_timeout = g_timeout_add (HIDE_TIMEOUT,
                                                (GSourceFunc)_hide_timeout,
                                                panel);
    }
}

static void
_window_mapped (Panel *panel)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;
    gint          x, y;

    priv = panel->priv;

    panel_set_position (panel,
                        priv->screen_position,
                        priv->xoffset,
                        priv->yoffset);

    _set_transparent (panel, TRUE);

    gtk_window_get_position (GTK_WINDOW (panel), &x, &y);

    xmon = panel_app_get_monitor (priv->monitor);
    priv->xoffset = x - xmon->geometry.x;
    priv->yoffset = y - xmon->geometry.y;

    DBG (" + coordinates: (%d, %d)\n", x, y);

    DBG ("\n + Position: %d\n + Offset: (%d, %d)",
         priv->screen_position, priv->xoffset, priv->yoffset);

    if (priv->autohide)
    {
        priv->hide_timeout =
            g_timeout_add (2000, (GSourceFunc)_hide_timeout, panel);
    }

}

/* public API */
void panel_init_signals (Panel *panel)
{
    DBG (" + Connect signals for panel %p", panel);

    g_signal_connect (G_OBJECT (panel), "enter-notify-event",
                      G_CALLBACK (panel_enter), NULL);

    g_signal_connect (G_OBJECT (panel), "leave-notify-event",
                      G_CALLBACK (panel_leave), NULL);

    g_signal_connect (G_OBJECT (panel), "map",
                      G_CALLBACK (_window_mapped), NULL);

    g_signal_connect (G_OBJECT (panel), "move-end",
                      G_CALLBACK (panel_move_end), NULL);

    panel_dnd_set_dest_name_and_widget (GTK_WIDGET (panel));
    g_signal_connect (panel, "drag-motion", G_CALLBACK (drag_motion), NULL);
    g_signal_connect (panel, "drag-leave", G_CALLBACK (drag_leave), NULL);
}

void
panel_init_position (Panel *panel)
{
    PanelPrivate   *priv;
    GtkOrientation  orientation;
    XfceMonitor    *xmon;
    gint            x, y, w, h, max;
    GtkRequisition  req;
    GdkRectangle    rect;

    priv = panel->priv;

    orientation =
        xfce_screen_position_is_horizontal (priv->screen_position) ?
                GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;

    xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (panel), orientation);
    xfce_itembar_set_orientation (XFCE_ITEMBAR (priv->itembar), orientation);

    xmon = panel_app_get_monitor (priv->monitor);

    gtk_window_set_screen (GTK_WINDOW (panel), xmon->screen);

    w = h = -1;

    if (priv->full_width > XFCE_PANEL_NORMAL_WIDTH)
        xfce_itembar_set_allow_expand (XFCE_ITEMBAR (priv->itembar), TRUE);

    if (xfce_screen_position_is_horizontal (priv->screen_position))
    {
        if (priv->full_width < XFCE_PANEL_SPAN_MONITORS)
            w = xmon->geometry.width;
        else
            w = gdk_screen_get_width (xmon->screen);

        max = w;
    }
    else
    {
        if (priv->full_width < XFCE_PANEL_SPAN_MONITORS)
            h = xmon->geometry.height;
        else
            h = gdk_screen_get_height (xmon->screen);

        max = h;
    }

    xfce_itembar_set_maximum_size (XFCE_ITEMBAR (priv->itembar), max);

    if (priv->full_width > XFCE_PANEL_NORMAL_WIDTH)
        gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);

    if (!xfce_screen_position_is_floating (priv->screen_position))
    {
        xfce_panel_window_set_movable (XFCE_PANEL_WINDOW (panel), FALSE);
        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (panel),
                                            XFCE_HANDLE_STYLE_NONE);
    }

    x = xmon->geometry.x;
    y = xmon->geometry.y;

    if (priv->xoffset > 0 || priv->yoffset > 0)
    {
        x += priv->xoffset;
        y += priv->yoffset;
    }
    else
    {
        if (priv->full_width < XFCE_PANEL_SPAN_MONITORS)
        {
            gtk_widget_size_request (GTK_WIDGET (panel), &req);
            _calculate_coordinates (priv->screen_position,
                                    &(xmon->geometry),
                                    req.width,
                                    req.height,
                                    &x, &y);

            priv->xoffset = x - xmon->geometry.x;
            priv->yoffset = y - xmon->geometry.y;
        }
        else
        {
            rect.x      = 0;
            rect.y      = 0;
            rect.width  = gdk_screen_get_width (xmon->screen);
            rect.height = gdk_screen_get_height (xmon->screen);

            _calculate_coordinates (priv->screen_position,
                                    &rect,
                                    req.width,
                                    req.height,
                                    &x, &y);

            priv->xoffset = x;
            priv->yoffset = y;
        }
    }

    DBG (" + offsets: (%d, %d)\n",
             priv->xoffset, priv->yoffset);

    DBG (" + coordinates: (%d, %d)\n", x, y);

    gtk_window_move (GTK_WINDOW (panel), x, y);

    xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (panel),
            (XfcePanelWindowMoveFunc)panel_move_function, panel);

    xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (panel),
            (XfcePanelWindowResizeFunc)panel_resize_function, panel);
}

void
panel_center (Panel *panel)
{
    PanelPrivate   *priv;
    GtkRequisition  req;
    XfceMonitor    *xmon;

    priv = panel->priv;

    if (priv->screen_position < XFCE_SCREEN_POSITION_FLOATING_H)
        return;

    xmon = panel_app_get_monitor (priv->monitor);

    gtk_widget_size_request (GTK_WIDGET (panel), &req);

    priv->xoffset = (xmon->geometry.width - req.width) / 2;
    priv->yoffset = (xmon->geometry.height - req.height) / 2;

    DBG ("\n + Position: %d\n + Offset: (%d, %d)",
         priv->screen_position, priv->xoffset, priv->yoffset);
}

void panel_set_autohide (Panel    *panel,
                         gboolean  autohide)
{
    PanelPrivate *priv;
    gint          x, y, w, h;
    XfceMonitor  *xmon;

    priv = panel->priv;

    if (autohide == priv->autohide)
        return;

    priv->autohide = autohide;

    if (!GTK_WIDGET_VISIBLE (panel))
        return;

    if (autohide)
    {
        panel_set_hidden (panel, TRUE);

        /* If autohide is blocked unhide again.
         * We use the first panel_set_hidden() call to give the user
         * some visual feedback. */
        if (priv->block_autohide)
        {
            while (gtk_events_pending ())
                gtk_main_iteration ();

            g_usleep (500000); /* 1/2 sec */
            panel_set_hidden (panel, FALSE);
        }
    }
    else if (priv->hidden)
    {
        panel_set_hidden (panel, FALSE);
    }
    else
    {
        gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
        gtk_window_get_size (GTK_WINDOW (panel), &w, &h);

        xmon = panel_app_get_monitor (priv->monitor);

        _set_struts (panel, xmon, x, y, w, h);
    }
}

void panel_block_autohide (Panel *panel)
{
    PanelPrivate *priv;

    DBG ("block");

    priv = panel->priv;

    priv->block_autohide++;

    if (priv->hidden)
        panel_set_hidden (panel, FALSE);

    _set_transparent (panel, FALSE);
}

void panel_unblock_autohide (Panel *panel)
{
    PanelPrivate *priv;
    gint          x, y, w, h, px, py;

    DBG ("unblock");

    priv = panel->priv;

    if (priv->block_autohide > 0)
    {
        priv->block_autohide--;

        if (!priv->block_autohide)
        {
            gdk_display_get_pointer (gdk_display_get_default (),
                                     NULL, &px, &py, NULL);

            gtk_window_get_position (GTK_WINDOW (panel), &x, &y);
            gtk_window_get_size (GTK_WINDOW (panel), &w, &h);

            if (px < x || px > x + w || py < y || py > y + h)
            {
                if (priv->autohide && !priv->hidden)
                    panel_set_hidden (panel, TRUE);

                _set_transparent (panel, TRUE);
            }
        }
    }
}

void
panel_set_full_width (Panel *panel,
                      gint   fullwidth)
{
    PanelPrivate *priv;
    XfceMonitor  *xmon;
    gint          w = -1, h = -1, max = -1;

    priv = panel->priv;

    if ((XfcePanelWidthType) fullwidth != priv->full_width)
    {
        priv->full_width = (XfcePanelWidthType) fullwidth;

        if (GTK_WIDGET_VISIBLE (panel))
        {
            xmon = panel_app_get_monitor (priv->monitor);

            switch (priv->full_width)
            {
                case XFCE_PANEL_NORMAL_WIDTH:
                    xfce_itembar_set_allow_expand (XFCE_ITEMBAR (priv->itembar),
                                                   FALSE);

                    if (xfce_screen_position_is_horizontal (priv->screen_position))
                        max = xmon->geometry.width;
                    else
                        max = xmon->geometry.height;
                    break;

                case XFCE_PANEL_FULL_WIDTH:
                case XFCE_PANEL_SPAN_MONITORS:
                    xfce_itembar_set_allow_expand (XFCE_ITEMBAR (priv->itembar),
                                                   TRUE);
                    if (xfce_screen_position_is_horizontal (
                                priv->screen_position))
                    {
                        if (priv->full_width == XFCE_PANEL_FULL_WIDTH)
                            w = xmon->geometry.width;
                        else
                            w = gdk_screen_get_width (xmon->screen);

                        max = w;
                    }
                    else
                    {
                        if (priv->full_width == XFCE_PANEL_FULL_WIDTH)
                            h = xmon->geometry.height;
                        else
                            h = gdk_screen_get_height (xmon->screen);

                        max = h;
                    }
                    break;
            }

            xfce_itembar_set_maximum_size (XFCE_ITEMBAR (priv->itembar), max);

            gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);

            panel_set_position (panel, priv->screen_position,
                                priv->xoffset, priv->yoffset);

            gtk_widget_queue_resize (GTK_WIDGET (panel));
        }
    }
}

void
panel_set_transparency (Panel *panel,
                        gint   transparency)
{
    PanelPrivate *priv;

    priv = panel->priv;

    if (transparency != priv->transparency)
    {
        DBG ("Transparency: %d", transparency);

        priv->transparency = transparency;
        priv->opacity      = 0;

        _set_transparent (panel, TRUE);
    }
}

void
panel_set_activetrans (Panel    *panel,
                       gboolean  activetrans)
{
    PanelPrivate *priv = panel->priv;

    if (activetrans != priv->activetrans)
    {
        priv->activetrans = activetrans;

        _set_transparent (panel, TRUE);
    }
}

/* properties */

gint
panel_get_size (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_SIZE);

    priv = panel->priv;

    return priv->size;
}

void
panel_set_size (Panel *panel,
                gint   size)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (xfce_screen_position_is_horizontal (priv->screen_position))
    {
        gtk_widget_set_size_request (priv->itembar, -1, size);
    }
    else
    {
        gtk_widget_set_size_request (priv->itembar, size, -1);
    }

    if (size != priv->size)
    {
        priv->size = size;

        gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                               (GtkCallback)xfce_panel_item_set_size,
                               GINT_TO_POINTER (size));
    }
}

gint
panel_get_monitor (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_MONITOR);

    priv = panel->priv;

    return priv->monitor;
}

void
panel_set_monitor (Panel *panel,
                   gint   monitor)
{
    PanelPrivate       *priv;
    XfceMonitor        *xmon;
    XfcePanelWidthType  width;

    g_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (monitor != priv->monitor)
    {
        /* TODO: check range */
        priv->monitor = monitor;

        xmon = panel_app_get_monitor (monitor);

        if (xmon->screen != gtk_widget_get_screen (GTK_WIDGET (panel)))
        {
            gtk_widget_hide (GTK_WIDGET (panel));
            gtk_window_set_screen (GTK_WINDOW (panel), xmon->screen);
            gtk_widget_show (GTK_WIDGET (panel));
        }

        /* new size constraints */
        if ((width = priv->full_width) != XFCE_PANEL_NORMAL_WIDTH)
        {
            priv->full_width = XFCE_PANEL_NORMAL_WIDTH;
            panel_set_full_width (panel, width);
        }
        else
        {
            gtk_widget_queue_resize (GTK_WIDGET (panel));
        }
    }
}

XfceScreenPosition
panel_get_screen_position (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_SCREEN_POSITION);

    priv = panel->priv;

    return priv->screen_position;
}

void
panel_set_screen_position (Panel              *panel,
                           XfceScreenPosition  position)
{
    PanelPrivate       *priv;
    GtkOrientation      orientation;
    XfcePanelWidthType  full_width;

    g_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (position != priv->screen_position)
    {
        full_width = priv->full_width;

        /* use edit_mode property to test if we are changing position */
        priv->edit_mode = TRUE;

        xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (panel),
                                             NULL, NULL);

        xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (panel),
                                               NULL, NULL);

        if (position == XFCE_SCREEN_POSITION_NONE)
            position = XFCE_SCREEN_POSITION_FLOATING_H;

        orientation = xfce_screen_position_get_orientation (position);

        if (xfce_screen_position_get_orientation (priv->screen_position) !=
            orientation)
        {
            if (full_width > XFCE_PANEL_NORMAL_WIDTH)
                panel_set_full_width (panel, XFCE_PANEL_NORMAL_WIDTH);

            xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (panel),
                                               orientation);

            xfce_itembar_set_orientation (XFCE_ITEMBAR (priv->itembar),
                                          orientation);
        }

        gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                (GtkCallback)xfce_panel_item_set_screen_position,
                GINT_TO_POINTER (position));

        if (xfce_screen_position_is_floating (position))
        {
            full_width = XFCE_PANEL_NORMAL_WIDTH;
            panel_set_full_width (panel, full_width);
            panel_set_autohide (panel, FALSE);
            xfce_panel_window_set_movable (XFCE_PANEL_WINDOW (panel), TRUE);

            if (!xfce_screen_position_is_floating (priv->screen_position))
                xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (panel),
                                                    XFCE_HANDLE_STYLE_BOTH);

            priv->xoffset = priv->yoffset = -1;
        }
        else
        {
            xfce_panel_window_set_movable (XFCE_PANEL_WINDOW (panel), FALSE);
            xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (panel),
                                                XFCE_HANDLE_STYLE_NONE);
            priv->xoffset = priv->yoffset = 0;
        }

        xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (panel),
                (XfcePanelWindowMoveFunc)panel_move_function, panel);

        xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (panel),
                (XfcePanelWindowResizeFunc)panel_resize_function, panel);

        priv->screen_position = position;
        panel_set_position (panel, position, priv->xoffset, priv->yoffset);

        if (full_width > XFCE_PANEL_NORMAL_WIDTH)
        {
            panel_set_full_width (panel, full_width);
        }

        /* update itembar size request */
        panel_set_size (panel, priv->size);

        gtk_widget_queue_resize (GTK_WIDGET (panel));

        g_idle_add((GSourceFunc)unblock_struts, panel);
    }
}

gint
panel_get_xoffset (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_XOFFSET);

    priv = panel->priv;

    return priv->xoffset;
}

void
panel_set_xoffset (Panel *panel,
                   gint   xoffset)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (xoffset != priv->xoffset)
    {
        priv->xoffset = xoffset;
        panel_set_position (panel, priv->screen_position,
                            xoffset, priv->yoffset);
    }
}

gint
panel_get_yoffset (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_YOFFSET);

    priv = panel->priv;

    return priv->yoffset;
}

void
panel_set_yoffset (Panel *panel,
                   gint   yoffset)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));

    priv = panel->priv;

    if (yoffset != priv->yoffset)
    {
        priv->yoffset = yoffset;
        panel_set_position (panel, priv->screen_position,
                            priv->xoffset, yoffset);
    }
}

