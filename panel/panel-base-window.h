/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_BASE_WINDOW_H__
#define __PANEL_BASE_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _PanelBaseWindowPrivate PanelBaseWindowPrivate;
typedef enum   _PanelBorders           PanelBorders;
typedef enum   _PanelBgStyle           PanelBgStyle;

#define PANEL_TYPE_BASE_WINDOW (panel_base_window_get_type ())
G_DECLARE_FINAL_TYPE (PanelBaseWindow, panel_base_window, PANEL, BASE_WINDOW, GtkWindow)

enum _PanelBorders
{
  PANEL_BORDER_NONE   = 0,
  PANEL_BORDER_LEFT   = 1 << 0,
  PANEL_BORDER_RIGHT  = 1 << 1,
  PANEL_BORDER_TOP    = 1 << 2,
  PANEL_BORDER_BOTTOM = 1 << 3
};

enum _PanelBgStyle
{
  PANEL_BG_STYLE_NONE,
  PANEL_BG_STYLE_COLOR,
  PANEL_BG_STYLE_IMAGE
};

struct _PanelBaseWindow
{
  GtkWindow __parent__;

  /*< private >*/
  PanelBaseWindowPrivate *priv;

  guint                    is_composited : 1;

  PanelBgStyle             background_style;
  GdkRGBA                 *background_rgba;
  gchar                   *background_image;

  /* transparency settings */
  gdouble                  enter_opacity;
  gdouble                  leave_opacity;
  gboolean                 opacity_is_enter;
};

void         panel_base_window_reset_background_css        (PanelBaseWindow *window);
void         panel_base_window_orientation_changed         (PanelBaseWindow *window,
                                                            gint             mode);
void         panel_base_window_set_borders                 (PanelBaseWindow *window,
                                                            PanelBorders     borders);
PanelBorders panel_base_window_get_borders                 (PanelBaseWindow *window);
void         panel_base_window_opacity_enter               (PanelBaseWindow *window,
                                                            gboolean         enter);

G_END_DECLS

#endif /* !__PANEL_BASE_WINDOW_H__ */
