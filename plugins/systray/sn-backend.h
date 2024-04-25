/*
 *  Copyright (c) 2017 Viktor Odintsev <ninetls@xfce.org>
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

#ifndef __SN_BACKEND_H__
#define __SN_BACKEND_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define SN_TYPE_BACKEND (sn_backend_get_type ())
G_DECLARE_FINAL_TYPE (SnBackend, sn_backend, SN, BACKEND, GObject)

SnBackend *
sn_backend_new (void);

void
sn_backend_start (SnBackend *backend);

G_END_DECLS

#endif /* !__SN_BACKEND_H__ */
