/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __PANEL_BUILDER_H__
#define __PANEL_BUILDER_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

GtkBuilder *panel_builder_new (XfcePanelPlugin  *panel_plugin,
                               const gchar      *buffer,
                               gsize             length,
                               GObject         **dialog_return) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

#endif /* !__PANEL_BUILDER_H__ */
