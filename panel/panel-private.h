/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* handling flags */
#define PANEL_SET_FLAG(flags,flag)   G_STMT_START{ ((flags) |= (flag)); }G_STMT_END
#define PANEL_UNSET_FLAG(flags,flag) G_STMT_START{ ((flags) &= ~(flag)); }G_STMT_END
#define PANEL_HAS_FLAG(flags,flag)   (((flags) & (flag)) != 0)

G_END_DECLS

#endif /* !__PANEL_PRIVATE_H__ */
