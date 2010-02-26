/* $Id$ */
/*
 * Copyright (c) 2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __XFCE_BLINK_BUTTON_H__
#define __XFCE_BLINK_BUTTON_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _XfceBlinkButtonPrivate XfceBlinkButtonPrivate;
typedef struct _XfceBlinkButtonClass   XfceBlinkButtonClass;
typedef struct _XfceBlinkButton        XfceBlinkButton;

#define XFCE_TYPE_BLINK_BUTTON            (xfce_blink_button_get_type ())
#define XFCE_BLINK_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_BLINK_BUTTON, XfceBlinkButton))
#define XFCE_BLINK_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_BLINK_BUTTON, XfceBlinkButtonClass))
#define XFCE_IS_BLINK_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_BLINK_BUTTON))
#define XFCE_IS_BLINK_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_BLINK_BUTTON))
#define XFCE_BLINK_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_BLINK_BUTTON, XfceBlinkButtonClass))

struct _XfceBlinkButtonClass
{
  /*< private >*/
  GtkToggleButtonClass __parent__;
};

struct _XfceBlinkButton
{
  /*< private >*/
  GtkToggleButton __parent__;

  /*< private >*/
  XfceBlinkButtonPrivate *priv;
};

PANEL_SYMBOL_EXPORT
GType      xfce_blink_button_get_type     (void) G_GNUC_CONST;

GtkWidget *xfce_blink_button_new          (void) G_GNUC_MALLOC;

gboolean   xfce_blink_button_get_blinking (XfceBlinkButton *button);
void       xfce_blink_button_set_blinking (XfceBlinkButton *button,
                                           gboolean         blinking);

G_END_DECLS

#endif /* !__XFCE_BLINK_BUTTON_H__ */

