/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <time.h>
#include <math.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-window.h>
#include <libxfce4panel/xfce-itembar.h>
#include <libxfce4panel/xfce-panel-item-iface.h>
#include <libxfce4panel/xfce-panel-enums.h>

#include "panel-properties.h"
#include "panel-private.h"

#define HIDDEN_SIZE	3
#define OPAQUE          0xffffffff
#define HIDE_TIMEOUT    350
#define UNHIDE_TIMEOUT  200


/* automatic (re)sizing and moving */
static void panel_move_end            (XfcePanelWindow *window,
                                       int x,
                                       int y);

static void panel_move_function       (XfcePanelWindow *window,
                                       Panel *panel,
                                       int *x,
                                       int *y);

static void panel_resize_function     (XfcePanelWindow *window,
                                       Panel *panel,
                                       GtkAllocation *old,
                                       GtkAllocation *new,
                                       int *x,
                                       int *y);

static void panel_screen_size_changed (GdkScreen *screen, 
                                       Panel *panel);

/* autohide and transparency */
static void panel_enter (Panel *p, GdkEventCrossing * event);

static void panel_leave (Panel *p, GdkEventCrossing * event);


/* moving and resizing */
static void
_calculate_coordinates (XfceScreenPosition position, /* screen position */
                        GdkRectangle *screen,         /* monitor geometry */
                        int width, int height,        /* panel size */
                        int *x, int *y)               /* output coordinates */
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
    gboolean top, bottom, left, right;

    priv = PANEL_GET_PRIVATE (panel);

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
            if (priv->full_width)
                right = FALSE;
            break;
        case XFCE_SCREEN_POSITION_N:
            top = FALSE;
            if (priv->full_width)
                left = right = FALSE;
            break;
        case XFCE_SCREEN_POSITION_NE_H:
            top = right = FALSE;
            if (priv->full_width)
                left = FALSE;
            break;
        /* left */
        case XFCE_SCREEN_POSITION_NW_V:
            left = top = FALSE;
            if (priv->full_width)
                bottom = FALSE;
            break;
        case XFCE_SCREEN_POSITION_W:
            left = FALSE;
            if (priv->full_width)
                top = bottom = FALSE;
            break;
        case XFCE_SCREEN_POSITION_SW_V:
            left = bottom = FALSE;
            if (priv->full_width)
                top = FALSE;
            break;
        /* right */
        case XFCE_SCREEN_POSITION_NE_V:
            right = top = FALSE;
            if (priv->full_width)
                bottom = FALSE;
            break;
        case XFCE_SCREEN_POSITION_E:
            right = FALSE;
            if (priv->full_width)
                top = bottom = FALSE;
            break;
        case XFCE_SCREEN_POSITION_SE_V:
            right = bottom = FALSE;
            if (priv->full_width)
                top = FALSE;
            break;
        /* bottom */
        case XFCE_SCREEN_POSITION_SW_H:
            bottom = left = FALSE;
            if (priv->full_width)
                right = FALSE;
            break;
        case XFCE_SCREEN_POSITION_S:
            bottom = FALSE;
            if (priv->full_width)
                left = right = FALSE;
            break;
        case XFCE_SCREEN_POSITION_SE_H:
            bottom = right = FALSE;
            if (priv->full_width)
                left = FALSE;
            break;
    }

    xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (panel), 
                                       top, bottom, left, right);
}

static void
_set_struts (Panel *panel, int x, int y, int w, int h)
{
    PanelPrivate *priv;
    gulong data[12] = { 0, };

    priv = PANEL_GET_PRIVATE (panel);

    if (!priv->autohide && 
            !xfce_screen_position_is_floating (priv->screen_position))
    {
        if (xfce_screen_position_is_left (priv->screen_position))
        {
            data[0] = w;	/* left           */

            data[4] = y;	/* left_start_y   */
            data[5] = y + h;	/* left_end_y     */
        }
        else if (xfce_screen_position_is_right (priv->screen_position))
        {
            data[1] = w;	/* right          */

            data[6] = y;	/* right_start_y  */
            data[7] = y + h;	/* right_end_y    */
        }
        else if (xfce_screen_position_is_top (priv->screen_position))
        {
            data[2] = h;	/* top            */

            data[8] = x;	/* top_start_x    */
            data[9] = x + w;	/* top_end_x      */
        }
        else
        {
            data[3] = h;	/* bottom         */

            data[10] = x;	/* bottom_start_x */
            data[11] = x + w;	/* bottom_end_x   */
	}
    }

    DBG ("\nStruts: "
	 "%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld",
	 data[0], data[1], data[2], data[3], data[4], data[5],
	 data[6], data[7], data[8], data[9], data[10], data[11]);

    gdk_error_trap_push ();

    gdk_property_change (GTK_WIDGET (panel)->window,
			 gdk_atom_intern ("_NET_WM_STRUT_PARTIAL", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & data, 12);

    gdk_error_trap_pop ();
}

static void 
panel_move_end (XfcePanelWindow *window, int x, int y)
{
    PanelPrivate *priv;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (window);

    if (xfce_screen_position_is_floating (priv->screen_position))
    {
        gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                (GtkCallback)xfce_panel_item_set_screen_position,
                GINT_TO_POINTER (priv->screen_position));
    }

    xmon = panel_app_get_monitor (priv->monitor);
    
    priv->xoffset = x - xmon->geometry.x;
    priv->yoffset = y - xmon->geometry.y;

    panel_app_queue_save ();
}

static void 
panel_move_function (XfcePanelWindow *window, Panel *panel, int *x, int *y)
{
    PanelPrivate *priv;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);

    priv->monitor = gdk_screen_get_monitor_at_point (
                        gtk_widget_get_screen (GTK_WIDGET (window)), *x, *y);
    
    xmon = panel_app_get_monitor (priv->monitor);
    
    _calculate_coordinates (priv->screen_position, &(xmon->geometry),
                            GTK_WIDGET (window)->allocation.width, 
                            GTK_WIDGET (window)->allocation.height, 
                            x, y);
    
    priv->xoffset = *x - xmon->geometry.x;
    priv->yoffset = *y - xmon->geometry.y;
}

static void 
panel_resize_function (XfcePanelWindow *window, Panel *panel, 
                       GtkAllocation *old, GtkAllocation *new, int *x, int *y)
{
    PanelPrivate *priv;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);

    xmon = panel_app_get_monitor (priv->monitor);
    
    _calculate_coordinates (priv->screen_position, &(xmon->geometry),
                            new->width, new->height, x, y);
    
    _set_struts (panel, *x, *y, new->width, new->height);
}

static void 
panel_set_position (Panel *panel, XfceScreenPosition position,
                    int xoffset, int yoffset)
{
    PanelPrivate *priv;
    GtkRequisition req;
    int x, y;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);
    
    xmon = panel_app_get_monitor (priv->monitor);

    priv->screen_position = position;

    _set_borders (panel);
    
    gtk_widget_size_request (GTK_WIDGET (panel), &req);

    x = y = 0;    

    _calculate_coordinates (position, &(xmon->geometry), 
                            req.width, req.height, &x, &y);
    
    if (G_UNLIKELY (xoffset < 0 || yoffset < 0))
    {
        gtk_window_get_position (GTK_WINDOW (panel), &xoffset, &yoffset);
        
        xoffset -= xmon->geometry.x;
        yoffset -= xmon->geometry.y;
    }
    
    priv->xoffset = xoffset;
    priv->yoffset = yoffset;
    
    x += xoffset;
    y += yoffset;
    
    gtk_window_move (GTK_WINDOW (panel), x, y);

    _set_struts (panel, x, y, req.width, req.height);
}

/* screen size */
static void 
panel_screen_size_changed (GdkScreen *screen, Panel *panel)
{
    PanelPrivate *priv;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);

    xmon = panel_app_get_monitor (priv->monitor);
    
    /* FIXME: do this in panel_app.c ? */
    gdk_screen_get_monitor_geometry (screen, xmon->num, 
                                     &(xmon->geometry));

    panel_set_position (panel, priv->screen_position, 
                        priv->xoffset, priv->yoffset);
}

/* transparency and autohide */
static void
_set_transparent (Panel *panel, gboolean transparent)
{
    PanelPrivate *priv;
    guint opacity;

    priv = PANEL_GET_PRIVATE (panel);
    
    if (G_UNLIKELY (priv->opacity == 0))
    {
        if (priv->transparency != 0)
            priv->opacity =  
                OPAQUE - rint ((double)priv->transparency * OPAQUE / 100);
        else
            priv->opacity = OPAQUE;
    }
    
    opacity = (transparent ? priv->opacity : OPAQUE);
    
    gdk_error_trap_push ();

    gdk_property_change (GTK_WIDGET (panel)->window,
			 gdk_atom_intern ("_NET_WM_WINDOW_OPACITY", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & opacity, 1L);

    gdk_error_trap_pop ();

    gtk_widget_queue_draw (GTK_WIDGET (panel));
}

static void
_set_hidden (Panel *panel, gboolean hide)
{
    PanelPrivate *priv;
    int w, h;

    priv = PANEL_GET_PRIVATE (panel);

    priv->hidden = hide;
    
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
        gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);
    }
    else
    {
        XfceMonitor *xmon;
        
        xmon = panel_app_get_monitor (priv->monitor);
    
        if (!priv->full_width)
        {
            w = h = -1;
        }
        else if (xfce_screen_position_is_horizontal (priv->screen_position))
        {
            w = xmon->geometry.width;
            h = -1;
        }
        else
        {
            w = -1;
            h = xmon->geometry.height;
        }

        gtk_widget_show (priv->itembar);
        gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);
    }
}

static gboolean
_hide_timeout (Panel *panel)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);
    priv->hide_timeout = 0;

    if (!priv->hidden && !priv->block_autohide)
        _set_hidden (panel, TRUE);
    
    return FALSE;
}

static gboolean
_unhide_timeout (Panel *panel)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);
    priv->unhide_timeout = 0;

    if (priv->hidden)
        _set_hidden (panel, FALSE);
    
    return FALSE;
}

static void 
panel_enter (Panel *panel, GdkEventCrossing * event)
{
    PanelPrivate *priv;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        panel_app_set_current_panel ((gpointer)panel);
        
        priv = PANEL_GET_PRIVATE (panel);
        
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
panel_leave (Panel *panel, GdkEventCrossing * event)
{
    PanelPrivate *priv;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        priv = PANEL_GET_PRIVATE (panel);
        
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
    XfceMonitor *xmon;
    int x, y;

    priv = PANEL_GET_PRIVATE (panel);

    if (priv->autohide)
        priv->hide_timeout =
            g_timeout_add (2000, (GSourceFunc)_hide_timeout, panel);
    
    _set_transparent (panel, TRUE);

    gtk_window_get_position (GTK_WINDOW (panel), &x, &y);

    xmon = panel_app_get_monitor (priv->monitor);
    priv->xoffset = x - xmon->geometry.x;
    priv->yoffset = y - xmon->geometry.y;
}

/* public API */

void 
panel_init_position (Panel *panel)
{
    PanelPrivate *priv;
    GtkOrientation orientation;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);
    
    xmon = panel_app_get_monitor (priv->monitor);

    orientation = 
        xfce_screen_position_is_horizontal (priv->screen_position) ?
                GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
    
    xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (panel), orientation);
    xfce_itembar_set_orientation (XFCE_ITEMBAR (priv->itembar), orientation);

    xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (panel),
            (XfcePanelWindowMoveFunc)panel_move_function, panel);

    xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (panel),
            (XfcePanelWindowResizeFunc)panel_resize_function, panel);

    g_signal_connect (panel, "move-end", G_CALLBACK (panel_move_end), NULL);
    
    xmon = panel_app_get_monitor (priv->monitor);
    
    gtk_window_set_screen (GTK_WINDOW (panel), xmon->screen);
    
    g_signal_connect (xmon->screen, "size-changed", 
                      G_CALLBACK (panel_screen_size_changed), panel);
    
    if (priv->full_width)
    {
        if (xfce_screen_position_is_horizontal (priv->screen_position))
            gtk_widget_set_size_request (GTK_WIDGET (panel), 
                                         xmon->geometry.width, -1);
        else
            gtk_widget_set_size_request (GTK_WIDGET (panel), 
                                         -1, xmon->geometry.height);
    }

    panel_set_position (panel, priv->screen_position, 
                        priv->xoffset, priv->yoffset);
    
    g_signal_connect (panel, "enter-notify-event", 
                      G_CALLBACK (panel_enter), NULL);
    
    g_signal_connect (panel, "leave-notify-event", 
                      G_CALLBACK (panel_leave), NULL);
    
    g_signal_connect (panel, "map", G_CALLBACK (_window_mapped), NULL);
    
    if (!xfce_screen_position_is_floating (priv->screen_position))
    {
        xfce_panel_window_set_movable (XFCE_PANEL_WINDOW (panel), FALSE);
        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (panel),
                                            XFCE_HANDLE_STYLE_NONE);
    }
}

void 
panel_center (Panel *panel)
{
    PanelPrivate *priv;
    GtkRequisition req;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);

    if (priv->screen_position < XFCE_SCREEN_POSITION_FLOATING_H)
        return;
    
    xmon = panel_app_get_monitor (priv->monitor);

    gtk_widget_size_request (GTK_WIDGET (panel), &req);

    priv->xoffset = (xmon->geometry.width - req.width) / 2;
    priv->yoffset = (xmon->geometry.height - req.height) / 2;
}

void panel_set_autohide (Panel *panel, gboolean autohide)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);

    if (autohide == priv->autohide)
        return;
    
    priv->autohide = autohide;
    
    if (!GTK_WIDGET_VISIBLE (GTK_WIDGET (panel)))
        return;
    
    if (autohide)
    {
        _set_hidden (panel, TRUE);

        /* If autohide is blocked unhide again.
         * We use the first _set_hidden() call to give the user
         * some visual feedback. */
        if (priv->block_autohide)
        {
            while (gtk_events_pending ())
                gtk_main_iteration ();

            g_usleep (500000); /* 1/2 sec */
            _set_hidden (panel, FALSE);
        }
    }
    else if (priv->hidden)
    {
        _set_hidden (panel, FALSE);
    }        
}

void panel_block_autohide (Panel *panel)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);

    priv->block_autohide++;

    if (priv->hidden)
        _set_hidden (panel, FALSE);

    _set_transparent (panel, FALSE);
}

void panel_unblock_autohide (Panel *panel)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);

    if (priv->block_autohide > 0)
    {
        priv->block_autohide--;

        if (!priv->block_autohide)
        {
            if (priv->autohide && !priv->hidden)
                _set_hidden (panel, TRUE);
            
            _set_transparent (panel, TRUE);
        }
    }
}

void
panel_set_full_width (Panel *panel, gboolean fullwidth)
{
    PanelPrivate *priv;
    XfceMonitor *xmon;

    priv = PANEL_GET_PRIVATE (panel);
    
    xmon = panel_app_get_monitor (priv->monitor);

    if (fullwidth != priv->full_width)
    {
        priv->full_width = fullwidth;

        if (GTK_WIDGET_VISIBLE (GTK_WIDGET (panel)))
        {
            int w, h;

            if (!priv->full_width)
            {
                w = h = -1;
            }
            else if (xfce_screen_position_is_horizontal (
                        priv->screen_position))
            {
                w = xmon->geometry.width;
                h = -1;
            }
            else
            {
                w = -1;
                h = xmon->geometry.height;
            }

            gtk_widget_set_size_request (GTK_WIDGET (panel), w, h);
            
            panel_set_position (panel, priv->screen_position, 
                                priv->xoffset, priv->yoffset);

            gtk_widget_queue_draw (GTK_WIDGET (panel));
        }
    }
}

/* not really positioning, but easier to put here */
void
panel_set_transparency (Panel *panel, int transparency)
{
    PanelPrivate *priv;

    priv = PANEL_GET_PRIVATE (panel);

    if (transparency != priv->transparency)
    {
        DBG ("Transparency: %d", transparency);

        priv->transparency = transparency;
        priv->opacity = 0;

        _set_transparent (panel, TRUE);
    }
}

/* properties */

int 
panel_get_size (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_SIZE);
    
    priv = PANEL_GET_PRIVATE (panel);

    return priv->size;
}

void 
panel_set_size (Panel *panel, int size)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));
    
    priv = PANEL_GET_PRIVATE (panel);

    if (size != priv->size)
    {
        priv->size = size;

        gtk_container_foreach (GTK_CONTAINER (priv->itembar),
                               (GtkCallback)xfce_panel_item_set_size,
                               GINT_TO_POINTER (size));
    }
}

int 
panel_get_monitor (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_MONITOR);
    
    priv = PANEL_GET_PRIVATE (panel);

    return priv->monitor;
}

void 
panel_set_monitor (Panel *panel, int monitor)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));
    
    priv = PANEL_GET_PRIVATE (panel);

    if (monitor != priv->monitor)
    {
        XfceMonitor *xmon;
        
        /* TODO: check range */
        priv->monitor = monitor;

        xmon = panel_app_get_monitor (monitor);

        gtk_widget_hide (GTK_WIDGET (panel));
        gtk_window_set_screen (GTK_WINDOW (panel), xmon->screen);
        gtk_widget_show (GTK_WIDGET (panel));
                
        panel_set_position (panel, priv->screen_position, 
                            priv->xoffset, priv->yoffset);
    }
}

XfceScreenPosition 
panel_get_screen_position (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_SCREEN_POSITION);
    
    priv = PANEL_GET_PRIVATE (panel);

    return priv->screen_position;
}

void 
panel_set_screen_position (Panel *panel, XfceScreenPosition position)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));
    
    priv = PANEL_GET_PRIVATE (panel);

    if (position != priv->screen_position)
    {
        GtkOrientation orientation;
        gboolean full_width = priv->full_width;
        
        if (position == XFCE_SCREEN_POSITION_NONE)
            position = XFCE_SCREEN_POSITION_FLOATING_H;

        orientation = xfce_screen_position_get_orientation (position);

        if (xfce_screen_position_get_orientation (priv->screen_position) != 
            orientation)
        {
            if (full_width)
                panel_set_full_width (panel, FALSE);
        
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
            full_width = FALSE;
            panel_set_full_width (panel, FALSE);
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

        /* let changes take effect */
        while (gtk_events_pending ())
            gtk_main_iteration ();

        panel_set_position (panel, position, priv->xoffset, priv->yoffset);
        
        if (full_width)
        {
            panel_set_full_width (panel, TRUE);
        }

        gtk_widget_queue_draw (GTK_WIDGET (panel));
    }
}

int 
panel_get_xoffset (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_XOFFSET);
    
    priv = PANEL_GET_PRIVATE (panel);

    return priv->xoffset;
}

void 
panel_set_xoffset (Panel *panel, int xoffset)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));
    
    priv = PANEL_GET_PRIVATE (panel);

    if (xoffset != priv->xoffset)
    {
        panel_set_position (panel, priv->screen_position, 
                            xoffset, priv->yoffset);
    }
}

int 
panel_get_yoffset (Panel *panel)
{
    PanelPrivate *priv;

    g_return_val_if_fail (PANEL_IS_PANEL (panel), DEFAULT_YOFFSET);
    
    priv = PANEL_GET_PRIVATE (panel);

    return priv->yoffset;
}

void 
panel_set_yoffset (Panel *panel, int yoffset)
{
    PanelPrivate *priv;

    g_return_if_fail (PANEL_IS_PANEL (panel));
    
    priv = PANEL_GET_PRIVATE (panel);

    if (yoffset != priv->yoffset)
    {
        panel_set_position (panel, priv->screen_position, 
                            priv->xoffset, yoffset);
    }
}

