/*
 *  Copyright (c) 2012-2013 Andrzej Radecki <andrzejr@xfce.org>
 *  Copyright (c) 2017      Viktor Odintsev <ninetls@xfce.org>
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

#ifndef __SN_BOX_H__
#define __SN_BOX_H__

#include <gtk/gtk.h>

#include "sn-config.h"
#include "sn-item.h"

G_BEGIN_DECLS

typedef struct _SnBoxClass SnBoxClass;
typedef struct _SnBox      SnBox;

#define XFCE_TYPE_SN_BOX            (sn_box_get_type ())
#define XFCE_SN_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SN_BOX, SnBox))
#define XFCE_SN_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SN_BOX, SnBoxClass))
#define XFCE_IS_SN_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SN_BOX))
#define XFCE_IS_SN_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SN_BOX))
#define XFCE_SN_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SN_BOX, SnBoxClass))

GType                  sn_box_get_type                         (void) G_GNUC_CONST;

void                   sn_box_remove_item                      (SnBox                   *box,
                                                                SnItem                  *item);

GtkWidget             *sn_box_new                              (SnConfig                *config);

G_END_DECLS

#endif /* !__SN_BOX_H__ */
