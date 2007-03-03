/* $Id$
 *
 * Copyright (c) 2004-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __XFCE_ITEMBAR_H__
#define __XFCE_ITEMBAR_H__

#include <gtk/gtkenums.h>
#include <gtk/gtkcontainer.h>

G_BEGIN_DECLS

typedef struct _XfceItembar      XfceItembar;
typedef struct _XfceItembarClass XfceItembarClass;

#define XFCE_TYPE_ITEMBAR            (xfce_itembar_get_type ())
#define XFCE_ITEMBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_ITEMBAR, XfceItembar))
#define XFCE_ITEMBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_ITEMBAR, XfceItembarClass))
#define XFCE_IS_ITEMBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_ITEMBAR))
#define XFCE_IS_ITEMBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_ITEMBAR))
#define XFCE_ITEMBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_ITEMBAR, XfceItembarClass))

struct _XfceItembar
{
    GtkContainer parent;
};

struct _XfceItembarClass
{
    GtkContainerClass parent_class;

    /* signals */
    void (*orientation_changed)   (XfceItembar    *itembar,
                                   GtkOrientation  orientation);
    void (*contents_changed)      (XfceItembar    *itembar);

    /* reserved for future expansion */
    void (*reserved1) (void);
    void (*reserved2) (void);
    void (*reserved3) (void);
    void (*reserved4) (void);
};

GType           xfce_itembar_get_type               (void) G_GNUC_CONST;
GtkWidget      *xfce_itembar_new                    (GtkOrientation        orientation) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void            xfce_itembar_set_orientation        (XfceItembar          *itembar,
                                                     GtkOrientation        orientation);
GtkOrientation  xfce_itembar_get_orientation        (XfceItembar          *itembar);
void            xfce_itembar_insert                 (XfceItembar          *itembar,
                                                     GtkWidget            *item,
                                                     gint                  position);
void            xfce_itembar_append                 (XfceItembar          *itembar,
                                                     GtkWidget            *item);
void            xfce_itembar_prepend                (XfceItembar          *itembar,
                                                     GtkWidget            *item);
void            xfce_itembar_reorder_child          (XfceItembar          *itembar,
                                                     GtkWidget            *item,
                                                     gint                  position);
void            xfce_itembar_set_child_expand       (XfceItembar          *itembar,
                                                     GtkWidget            *item,
                                                     gboolean              expand);
gboolean        xfce_itembar_get_child_expand       (XfceItembar          *itembar,
                                                     GtkWidget            *item);
gint            xfce_itembar_get_n_items            (XfceItembar          *itembar);
gint            xfce_itembar_get_item_index         (XfceItembar          *itembar,
                                                    GtkWidget            *item);
GtkWidget      *xfce_itembar_get_nth_item           (XfceItembar          *itembar,
                                                    gint                  n) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void            xfce_itembar_raise_event_window     (XfceItembar          *itembar);
void            xfce_itembar_lower_event_window     (XfceItembar          *itembar);
gboolean        xfce_itembar_event_window_is_raised (XfceItembar          *itembar);
GtkWidget      *xfce_itembar_get_item_at_point      (XfceItembar          *itembar,
                                                     gint                  x,
                                                     gint                  y) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gint            xfce_itembar_get_drop_index         (XfceItembar          *itembar,
                                                     gint                  x,
                                                     gint                  y);

G_END_DECLS

#endif /* !__XFCE_ITEMBAR_H__ */
