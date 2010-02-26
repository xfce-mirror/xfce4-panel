/* $Id$ */
/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#ifndef __PANEL_PRIVATE_H__
#define __PANEL_PRIVATE_H__

/* handling flags */
#define PANEL_SET_FLAG(flags,flag) G_STMT_START{ ((flags) |= (flag)); }G_STMT_END
#define PANEL_UNSET_FLAG(flags,flag) G_STMT_START{ ((flags) &= ~(flag)); }G_STMT_END
#define PANEL_HAS_FLAG(flags,flag) (((flags) & (flag)) != 0)

/* check if the string is not empty */
#define IS_STRING(string) ((string) != NULL && *(string) != '\0')

/* relative path to the plugin directory */
#define PANEL_PLUGIN_RELATIVE_PATH "xfce4" G_DIR_SEPARATOR_S "panel"

/* relative plugin's rc filename (printf format) */
#define PANEL_PLUGIN_RC_RELATIVE_PATH PANEL_PLUGIN_RELATIVE_PATH G_DIR_SEPARATOR_S "%s-%d.rc"

/* xfconf property base (printf format) */
#define PANEL_PLUGIN_PROPERTY_BASE "/plugins/plugin-%d"

/* quick GList and GSList counting without traversing */
#define LIST_HAS_ONE_ENTRY(l)           ((l) != NULL && (l)->next == NULL)
#define LIST_HAS_ONE_OR_NO_ENTRIES(l)   ((l) == NULL || (l)->next == NULL)
#define LIST_HAS_TWO_OR_MORE_ENTRIES(l) ((l) != NULL && (l)->next != NULL)

#endif /* !__PANEL_PRIVATE_H__ */