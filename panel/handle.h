/*  handle.h
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __XFCE_HANDLE_H__
#define __XFCE_HANDLE_H__

#include "global.h"

typedef struct _Handle Handle;

Handle *handle_new(int side);

void handle_pack(Handle *mh, GtkBox *box);

void handle_unpack(Handle *mh, GtkContainer *container);

void handle_set_size(Handle *mh, int size);

void handle_set_style(Handle *mh, int style);

void handle_set_popup_position(Handle *mh);

#endif /* __XFCE_HANDLE_H__ */

