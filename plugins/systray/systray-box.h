/*
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __SYSTRAY_BOX_H__
#define __SYSTRAY_BOX_H__

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SYSTRAY_TYPE_BOX (systray_box_get_type ())
G_DECLARE_FINAL_TYPE (SystrayBox, systray_box, SYSTRAY, BOX, GtkContainer)

void
systray_box_register_type (XfcePanelTypeModule *module);

GtkWidget *
systray_box_new (void) G_GNUC_MALLOC;

void
systray_box_set_orientation (SystrayBox *box,
                             GtkOrientation orientation);

void
systray_box_set_dimensions (SystrayBox *box,
                            gint icon_size,
                            gint n_rows,
                            gint row_size,
                            gint padding);

void
systray_box_set_size_alloc (SystrayBox *box,
                            gint size_alloc);

void
systray_box_set_show_hidden (SystrayBox *box,
                             gboolean show_hidden);

gboolean
systray_box_get_show_hidden (SystrayBox *box);

void
systray_box_set_squared (SystrayBox *box,
                         gboolean square_icons);

gboolean
systray_box_get_squared (SystrayBox *box);

void
systray_box_update (SystrayBox *box,
                    GSList *names_ordered);

gboolean
systray_box_has_hidden_items (SystrayBox *box);

void
systray_box_set_single_row (SystrayBox *box,
                            gboolean single_row);

G_END_DECLS

#endif /* !__SYSTRAY_BOX_H__ */
