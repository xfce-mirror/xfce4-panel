/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2004-2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_ARROW_BUTTON_H
#define _XFCE_ARROW_BUTTON_H

#include <gtk/gtkenums.h>
#include <gtk/gtktogglebutton.h>

#define XFCE_TYPE_ARROW_BUTTON            (xfce_arrow_button_get_type ())
#define XFCE_ARROW_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_ARROW_BUTTON, XfceArrowButton))
#define XFCE_ARROW_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_ARROW_BUTTON, XfceArrowButtonClass))
#define XFCE_IS_ARROW_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_ARROW_BUTTON))
#define XFCE_IS_ARROW_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_ARROW_BUTTON))
#define XFCE_ARROW_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_ARROW_BUTTON, XfceArrowButtonClass))


G_BEGIN_DECLS

typedef struct _XfceArrowButton 	XfceArrowButton;

typedef struct _XfceArrowButtonClass 	XfceArrowButtonClass;

struct _XfceArrowButton
{
    GtkToggleButton parent;

    GtkArrowType arrow_type;
};

struct _XfceArrowButtonClass
{
    GtkToggleButtonClass parent_class;

    /* signals */
    void (*arrow_type_changed)     (GtkWidget * widget,
                                    GtkArrowType type);

    /* Padding for future expansion */
    void (*_xfce_reserved1)        (void);
    void (*_xfce_reserved2)        (void);
    void (*_xfce_reserved3)        (void);
};


GType xfce_arrow_button_get_type (void) G_GNUC_CONST;

GtkWidget *xfce_arrow_button_new              (GtkArrowType type);

void xfce_arrow_button_set_arrow_type         (XfceArrowButton * button,
                                               GtkArrowType type);

GtkArrowType xfce_arrow_button_get_arrow_type (XfceArrowButton * button);

G_END_DECLS

#endif /* _XFCE_ARROW_BUTTON_H */
