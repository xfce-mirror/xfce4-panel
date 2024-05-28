/*
 * Copyright (C) 2008-2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_ITEM_DIALOG_H__
#define __PANEL_ITEM_DIALOG_H__

/* includes libxfce4ui and the definition of glib_autoptr_clear_XfceTitledDialog */
#include "panel-tic-tac-toe.h"

G_BEGIN_DECLS

#define PANEL_TYPE_ITEM_DIALOG (panel_item_dialog_get_type ())
G_DECLARE_FINAL_TYPE (PanelItemDialog, panel_item_dialog, PANEL, ITEM_DIALOG, XfceTitledDialog)

void
panel_item_dialog_show (PanelWindow *active);

void
panel_item_dialog_show_from_id (gint panel_id);

gboolean
panel_item_dialog_visible (void);

G_END_DECLS

#endif /* !__PANEL_ITEM_DIALOG_H__ */
