/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __PANEL_WINDOW_H__
#define __PANEL_WINDOW_H__

#include <gtk/gtk.h>
#include <panel/panel-private.h>

G_BEGIN_DECLS

typedef struct _PanelWindowClass    PanelWindowClass;
typedef struct _PanelWindow         PanelWindow;
typedef enum   _PanelWindowSnapEdge PanelWindowSnapEdge;
typedef enum   _PanelWindowBorders  PanelWindowBorders;

#define PANEL_TYPE_WINDOW            (panel_window_get_type ())
#define PANEL_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_WINDOW, PanelWindow))
#define PANEL_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_WINDOW, PanelWindowClass))
#define PANEL_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_WINDOW))
#define PANEL_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_WINDOW))
#define PANEL_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_WINDOW, PanelWindowClass))

#define snap_edge_is_top(snap_edge)    (snap_edge == PANEL_SNAP_EGDE_NE || snap_edge == PANEL_SNAP_EGDE_NW || \
                                        snap_edge == PANEL_SNAP_EGDE_NC || snap_edge == PANEL_SNAP_EGDE_N)
#define snap_edge_is_bottom(snap_edge) (snap_edge == PANEL_SNAP_EGDE_SE || snap_edge == PANEL_SNAP_EGDE_SW || \
                                        snap_edge == PANEL_SNAP_EGDE_SC || snap_edge == PANEL_SNAP_EGDE_S)
#define snap_edge_is_left(snap_edge)   (snap_edge >= PANEL_SNAP_EGDE_W && snap_edge <= PANEL_SNAP_EGDE_SW)
#define snap_edge_is_right(snap_edge)  (snap_edge >= PANEL_SNAP_EGDE_E && snap_edge <= PANEL_SNAP_EGDE_SE)

#define PANEL_BORDER_ALL (PANEL_BORDER_LEFT | PANEL_BORDER_RIGHT | PANEL_BORDER_TOP | PANEL_BORDER_BOTTOM)


enum _PanelWindowSnapEdge
{
  /* no snapping */
  PANEL_SNAP_EGDE_NONE, /* 0  no snapping */

  /* right edge */
  PANEL_SNAP_EGDE_E,    /* 1  right */
  PANEL_SNAP_EGDE_NE,   /* 2  top right */
  PANEL_SNAP_EGDE_EC,   /* 3  right center */
  PANEL_SNAP_EGDE_SE,   /* 4  bottom right */

  /* left edge */
  PANEL_SNAP_EGDE_W,    /* 5  left */
  PANEL_SNAP_EGDE_NW,   /* 6  top left */
  PANEL_SNAP_EGDE_WC,   /* 7  left center */
  PANEL_SNAP_EGDE_SW,   /* 8  bottom left */

  /* top and bottom */
  PANEL_SNAP_EGDE_NC,   /* 9  top center */
  PANEL_SNAP_EGDE_SC,   /* 10 bottom center */
  PANEL_SNAP_EGDE_N,    /* 11 top */
  PANEL_SNAP_EGDE_S,    /* 12 bottom */
};

enum _PanelWindowBorders
{
  PANEL_BORDER_LEFT   = 1 << 0,
  PANEL_BORDER_RIGHT  = 1 << 1,
  PANEL_BORDER_TOP    = 1 << 2,
  PANEL_BORDER_BOTTOM = 1 << 3,
};


GType                panel_window_get_type              (void) G_GNUC_CONST;

gboolean             panel_window_is_composited         (PanelWindow         *window);

void                 panel_window_set_active_panel      (PanelWindow         *window,
                                                         gboolean             selected);

void                 panel_window_freeze_autohide       (PanelWindow         *window);

void                 panel_window_thaw_autohide         (PanelWindow         *window);

G_END_DECLS

#endif /* !__PANEL_WINDOW_H__ */
