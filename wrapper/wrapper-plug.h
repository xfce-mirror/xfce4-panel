/*
 * Copyright (C) 2022 Gaël Bonithon <gael@xfce.org>
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

#ifndef __WRAPPER_PLUG_H__
#define __WRAPPER_PLUG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define WRAPPER_TYPE_PLUG (wrapper_plug_get_type ())
G_DECLARE_INTERFACE (WrapperPlug, wrapper_plug, WRAPPER, PLUG, GtkWindow)

struct _WrapperPlugInterface
{
    GTypeInterface g_iface;

    /* X11 only */
    void        (*set_background_color)        (WrapperPlug         *plug,
                                                const gchar         *color);
    void        (*set_background_image)        (WrapperPlug         *plug,
                                                const gchar         *image);
};

GtkWidget    *wrapper_plug_new                      (gulong                            socket_id);

void          wrapper_plug_set_background_color     (WrapperPlug                      *plug,
                                                     const gchar                      *color);

void          wrapper_plug_set_background_image     (WrapperPlug                      *plug,
                                                     const gchar                      *image);

G_END_DECLS

#endif /* ! __WRAPPER_PLUG_H__ */
