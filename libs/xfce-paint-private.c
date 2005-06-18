/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2004-2005 Jasper Huijsmans <jasper@xfce.org>
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

#include "xfce-paint-private.h"

enum { 
    HANDLE_STYLE_DOT, 
    HANDLE_STYLE_GRIP, 
    HANDLE_STYLE_BOX
};

#define HANDLE_STYLE HANDLE_STYLE_GRIP

/* 4.2 style handle */

#define DECOR_WIDTH  6

static unsigned char dark_bits[] = { 0x00, 0x0e, 0x02, 0x02, 0x00, 0x00, };
static unsigned char light_bits[] = { 0x00, 0x00, 0x10, 0x10, 0x1c, 0x00, };
static unsigned char mid_bits[] = { 0x00, 0x00, 0x0c, 0x0c, 0x00, 0x00, };

static void
paint_dotted_handle (GtkWidget * widget, GdkRectangle * area,
                     const char *detail, GtkOrientation orientation,
                     int x, int y, int width, int height)
{
    int rows, columns, w, h;
    static GdkBitmap *light_bmap = NULL;
    static GdkBitmap *mid_bmap = NULL;
    static GdkBitmap *dark_bmap = NULL;

    if (G_UNLIKELY (light_bmap == NULL))
    {
        light_bmap = gdk_bitmap_create_from_data (widget->window, light_bits,
                                                  DECOR_WIDTH, DECOR_WIDTH);
        mid_bmap = gdk_bitmap_create_from_data (widget->window, mid_bits,
                                                DECOR_WIDTH, DECOR_WIDTH);
        dark_bmap = gdk_bitmap_create_from_data (widget->window, dark_bits,
                                                 DECOR_WIDTH, DECOR_WIDTH);
    }

    gdk_gc_set_clip_rectangle (widget->style->light_gc[widget->state], area);
    gdk_gc_set_clip_rectangle (widget->style->dark_gc[widget->state], area);
    gdk_gc_set_clip_rectangle (widget->style->mid_gc[widget->state], area);

    if (GTK_ORIENTATION_HORIZONTAL == orientation)
    {
        rows = 1;
        columns = CLAMP (width / DECOR_WIDTH - 2, 1 ,3);
    }
    else
    {
        rows = CLAMP (height / DECOR_WIDTH - 2, 1, 3);
        columns = 1;
    }

    w = columns * DECOR_WIDTH;
    h = rows * DECOR_WIDTH;
    x = x + (width - w) / 2;
    y = y + (height - h) / 2;

    gdk_gc_set_stipple (widget->style->light_gc[widget->state], light_bmap);
    gdk_gc_set_stipple (widget->style->mid_gc[widget->state], mid_bmap);
    gdk_gc_set_stipple (widget->style->dark_gc[widget->state], dark_bmap);

    gdk_gc_set_fill (widget->style->light_gc[widget->state], GDK_STIPPLED);
    gdk_gc_set_fill (widget->style->mid_gc[widget->state], GDK_STIPPLED);
    gdk_gc_set_fill (widget->style->dark_gc[widget->state], GDK_STIPPLED);

    gdk_gc_set_ts_origin (widget->style->light_gc[widget->state], x, y);
    gdk_gc_set_ts_origin (widget->style->mid_gc[widget->state], x, y);
    gdk_gc_set_ts_origin (widget->style->dark_gc[widget->state], x, y);

    gdk_draw_rectangle (widget->window,
                        widget->style->light_gc[widget->state], TRUE, x,
                        y, w, h);
    gdk_draw_rectangle (widget->window,
                        widget->style->mid_gc[widget->state], TRUE, x, y,
                        w, h);
    gdk_draw_rectangle (widget->window,
                        widget->style->dark_gc[widget->state], TRUE, x, y,
                        w, h);

    gdk_gc_set_fill (widget->style->light_gc[widget->state], GDK_SOLID);
    gdk_gc_set_fill (widget->style->mid_gc[widget->state], GDK_SOLID);
    gdk_gc_set_fill (widget->style->dark_gc[widget->state], GDK_SOLID);

    gdk_gc_set_clip_rectangle (widget->style->light_gc[widget->state], NULL);
    gdk_gc_set_clip_rectangle (widget->style->dark_gc[widget->state], NULL);
    gdk_gc_set_clip_rectangle (widget->style->mid_gc[widget->state], NULL);
}

/* paint function */
void
xfce_paint_handle (GtkWidget * widget, GdkRectangle * area,
                   const char *detail, GtkOrientation orientation, int x,
                   int y, int width, int height)
{
    switch (HANDLE_STYLE)
    {
        case HANDLE_STYLE_DOT:
            paint_dotted_handle (widget, area, detail, orientation,
                                 x, y, width, height);
            break;
        case HANDLE_STYLE_GRIP:
            gtk_paint_handle (widget->style,
                              widget->window,
                              GTK_WIDGET_STATE (widget),
                              GTK_SHADOW_OUT,
                              area, widget, "handlebox",
                              x, y, width, height,
                              orientation);
            break;
        default:
            gtk_paint_box (widget->style,
                           widget->window,
                           GTK_WIDGET_STATE (widget),
                           GTK_SHADOW_OUT, area, widget, detail, 
                           x, y, width, height);
    }
}
