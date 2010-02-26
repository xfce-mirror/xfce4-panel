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

#ifndef __XFCE_PANEL_BUTTON_H__
#define __XFCE_PANEL_BUTTON_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _XfcePanelButtonPrivate XfcePanelButtonPrivate;
typedef struct _XfcePanelButtonClass   XfcePanelButtonClass;
typedef struct _XfcePanelButton        XfcePanelButton;

#define XFCE_TYPE_PANEL_BUTTON            (xfce_panel_button_get_type ())
#define XFCE_PANEL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_PANEL_BUTTON, XfcePanelButton))
#define XFCE_PANEL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PANEL_BUTTON, XfcePanelButtonClass))
#define XFCE_IS_PANEL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_PANEL_BUTTON))
#define XFCE_IS_PANEL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_PANEL_BUTTON))
#define XFCE_PANEL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_PANEL_BUTTON, XfcePanelButtonClass))

struct _XfcePanelButtonClass
{
  /*< private >*/
  GtkToggleButtonClass __parent__;
};

struct _XfcePanelButton
{
  /*< private >*/
  GtkToggleButton __parent__;

  /*< private >*/
  XfcePanelButtonPrivate *priv;
};

PANEL_SYMBOL_EXPORT
GType      xfce_panel_button_get_type     (void) G_GNUC_CONST;

GtkWidget *xfce_panel_button_new          (void) G_GNUC_MALLOC;

gboolean   xfce_panel_button_get_blinking (XfcePanelButton *button);
void       xfce_panel_button_set_blinking (XfcePanelButton *button,
                                           gboolean         blinking);

G_END_DECLS

#endif /* !__XFCE_PANEL_BUTTON_H__ */

