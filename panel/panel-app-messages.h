/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PANEL_APP_MESSAGES_H__
#define __PANEL_APP_MESSAGES_H__

#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

typedef enum
{
    PANEL_APP_CUSTOMIZE,
    PANEL_APP_SAVE,
    PANEL_APP_RESTART,
    PANEL_APP_QUIT,
    PANEL_APP_EXIT,
    PANEL_APP_ADD
}
PanelAppMessage;

gboolean panel_app_send (PanelAppMessage message);

void panel_app_listen (GtkWidget *ipc_window);

G_END_DECLS

#endif /* !__PANEL_APP_MESSAGES_H__ */
