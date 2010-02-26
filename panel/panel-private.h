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

#ifndef __PANEL_PRIVATE_H__
#define __PANEL_PRIVATE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* handling flags */
#define PANEL_SET_FLAG(flags,flag)   G_STMT_START{ ((flags) |= (flag)); }G_STMT_END
#define PANEL_UNSET_FLAG(flags,flag) G_STMT_START{ ((flags) &= ~(flag)); }G_STMT_END
#define PANEL_HAS_FLAG(flags,flag)   (((flags) & (flag)) != 0)

/* handling deprecated functions in gtk */
#if GTK_CHECK_VERSION (2,12,0)
#define _widget_set_tooltip_text(widget,text) gtk_widget_set_tooltip_text (widget, text)
#define _window_set_opacity(window,opacity) gtk_window_set_opacity (window, opacity)
#else

void _widget_set_tooltip_text (GtkWidget   *widget,
                               const gchar *text);

void _window_set_opacity      (GtkWindow   *window,
                               gdouble      opacity);
#endif

G_END_DECLS

#endif /* !__PANEL_PRIVATE_H__ */
