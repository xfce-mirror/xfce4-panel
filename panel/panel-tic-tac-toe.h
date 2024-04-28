/*
 * Copyright (C) 2010 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_TIC_TAC_TOE_H__
#define __PANEL_TIC_TAC_TOE_H__

#include <libxfce4ui/libxfce4ui.h>

G_BEGIN_DECLS

#define PANEL_TYPE_TIC_TAC_TOE (panel_tic_tac_toe_get_type ())
#ifndef glib_autoptr_clear_XfceTitledDialog
G_DEFINE_AUTOPTR_CLEANUP_FUNC (XfceTitledDialog, g_object_unref)
#endif
G_DECLARE_FINAL_TYPE (PanelTicTacToe, panel_tic_tac_toe, PANEL, TIC_TAC_TOE, XfceTitledDialog)

void
panel_tic_tac_toe_show (void);

G_END_DECLS

#endif /* !__PANEL_TIC_TAC_TOE_H__ */
