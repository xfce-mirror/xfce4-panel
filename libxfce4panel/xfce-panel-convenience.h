/* $Id$ */
/*
 * Copyright (c) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
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

#if !defined(LIBXFCE4PANEL_INSIDE_LIBXFCE4PANEL_H) && !defined(LIBXFCE4PANEL_COMPILATION)
#error "Only <libxfce4panel/libxfce4panel.h> can be included directly, this file may disappear or change contents"
#endif

#ifndef __XFCE_PANEL_CONVENIENCE_H__
#define __XFCE_PANEL_CONVENIENCE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget  *xfce_create_panel_button         (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
GtkWidget  *xfce_create_panel_toggle_button  (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean    xfce_allow_panel_customization   (void);

G_END_DECLS

#endif /* !__XFCE_PANEL_CONVENIENCE_H__ */
