/*  $Id$
 *
 *  Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __XFCE_PANEL_EXEC_H__
#define __XFCE_PANEL_EXEC_H__

void launcher_execute                (GdkScreen     *screen,
                                      LauncherEntry *entry,
                                      GSList        *file_list,
                                      guint32        event_time) G_GNUC_INTERNAL;
void launcher_execute_from_clipboard (GdkScreen     *screen,
                                      LauncherEntry *entry,
                                      guint32        event_time) G_GNUC_INTERNAL;

#endif /* !__XFCE_PANEL_EXEC_H__ */
