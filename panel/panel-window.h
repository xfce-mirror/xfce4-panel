/* $Id$ */
/*
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

#ifndef __PANEL_WINDOW_H__
#define __PANEL_WINDOW_H__

#include <gtk/gtk.h>
#include <panel/panel-private.h>

G_BEGIN_DECLS

typedef struct _PanelWindowClass    PanelWindowClass;
typedef struct _PanelWindow         PanelWindow;
typedef enum   _PanelWindowSnapEdge PanelWindowSnapEdge;

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



GType                panel_window_get_type              (void) G_GNUC_CONST;

GtkWidget           *panel_window_new                   (void);

gboolean             panel_window_is_composited         (PanelWindow         *window);

void                 panel_window_set_selected          (PanelWindow         *window,
                                                         gboolean             selected);

PanelWindowSnapEdge  panel_window_get_snap_edge         (PanelWindow         *window);
void                 panel_window_set_snap_edge         (PanelWindow         *window,
                                                         PanelWindowSnapEdge  snap_edge);

gboolean             panel_window_get_locked            (PanelWindow         *window);
void                 panel_window_set_locked            (PanelWindow         *window,
                                                         gboolean             locked);

GtkOrientation       panel_window_get_orientation       (PanelWindow         *window);
void                 panel_window_set_orientation       (PanelWindow         *window,
                                                         GtkOrientation       orientation);

gint                 panel_window_get_size              (PanelWindow         *window);
void                 panel_window_set_size              (PanelWindow         *window,
                                                         gint                 size);

gint                 panel_window_get_length            (PanelWindow         *window);
void                 panel_window_set_length            (PanelWindow         *window,
                                                         gint                 percentage);

gint                 panel_window_get_xoffset           (PanelWindow         *window);
void                 panel_window_set_xoffset           (PanelWindow         *window,
                                                         gint                 xoffset);

gint                 panel_window_get_yoffset           (PanelWindow         *window);
void                 panel_window_set_yoffset           (PanelWindow         *window,
                                                         gint                 yoffset);

gboolean             panel_window_get_autohide          (PanelWindow         *window);
void                 panel_window_set_autohide          (PanelWindow         *window,
                                                         gboolean             autohide);

void                 panel_window_freeze_autohide       (PanelWindow         *window);
void                 panel_window_thaw_autohide         (PanelWindow         *window);

gint                 panel_window_get_background_alpha  (PanelWindow         *window);
void                 panel_window_set_background_alpha  (PanelWindow         *window,
                                                         gint                 alpha);

gint                 panel_window_get_enter_opacity     (PanelWindow         *window);
void                 panel_window_set_enter_opacity     (PanelWindow         *window,
                                                         gint                 opacity);

gint                 panel_window_get_leave_opacity     (PanelWindow         *window);
void                 panel_window_set_leave_opacity     (PanelWindow         *window,
                                                         gint                 opacity);

gboolean             panel_window_get_span_monitors     (PanelWindow         *window);
void                 panel_window_set_span_monitors     (PanelWindow         *window,
                                                         gboolean             span_monitors);

G_END_DECLS

#endif /* !__PANEL_WINDOW_H__ */
