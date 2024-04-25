/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PAGER_BUTTONS_H__
#define __PAGER_BUTTONS_H__

#include "libxfce4panel/libxfce4panel.h"

#include <gtk/gtk.h>
#include <libxfce4windowing/libxfce4windowing.h>

G_BEGIN_DECLS

#define PAGER_TYPE_BUTTONS (pager_buttons_get_type ())
G_DECLARE_FINAL_TYPE (PagerButtons, pager_buttons, PAGER, BUTTONS, XfcePanelPlugin)

void
pager_buttons_register_type (XfcePanelTypeModule *type_module);

GtkWidget *
pager_buttons_new (XfwScreen *screen) G_GNUC_MALLOC;

void
pager_buttons_set_orientation (PagerButtons *pager,
                               GtkOrientation orientation);

void
pager_buttons_set_n_rows (PagerButtons *pager,
                          gint rows);

void
pager_buttons_set_numbering (PagerButtons *pager,
                             gboolean numbering);

G_END_DECLS

#endif /* !__PAGER_BUTTONS_H__ */
