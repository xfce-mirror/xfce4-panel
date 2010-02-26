/* $Id$ */
/*
 * Copyright (c) 2007-2009 Nick Schermer <nick@xfce.org>
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

#ifndef __SYSTRAY_BOX_H__
#define __SYSTRAY_BOX_H__

typedef struct _SystrayBoxClass SystrayBoxClass;
typedef struct _SystrayBox      SystrayBox;

#define XFCE_TYPE_SYSTRAY_BOX            (systray_box_get_type ())
#define XFCE_SYSTRAY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SYSTRAY_BOX, SystrayBox))
#define XFCE_SYSTRAY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SYSTRAY_BOX, SystrayBoxClass))
#define XFCE_IS_SYSTRAY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SYSTRAY_BOX))
#define XFCE_IS_SYSTRAY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SYSTRAY_BOX))
#define XFCE_SYSTRAY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SYSTRAY_BOX, SystrayBoxClass))

GType      systray_box_get_type        (void) G_GNUC_CONST;

void       systray_box_register_type   (XfcePanelTypeModule *module);

GtkWidget *systray_box_new             (void) G_GNUC_MALLOC;

void       systray_box_set_guess_size  (SystrayBox          *box,
                                        gint                 guess_size);

void       systray_box_set_arrow_type  (SystrayBox          *box,
                                        GtkArrowType         arrow_type);

void       systray_box_set_rows        (SystrayBox          *box,
                                        gint                 rows);

gint       systray_box_get_rows        (SystrayBox          *box);

void       systray_box_add_with_name   (SystrayBox          *box,
                                        GtkWidget           *child,
                                        const gchar         *name);

void       systray_box_name_add        (SystrayBox          *box,
                                        const gchar         *name,
                                        gboolean             hidden);

void       systray_box_name_set_hidden (SystrayBox          *box,
                                        const gchar         *name,
                                        gboolean             hidden);

gboolean   systray_box_name_get_hidden (SystrayBox          *box,
                                        const gchar         *name);

GList     *systray_box_name_list       (SystrayBox          *box);

void       systray_box_name_clear      (SystrayBox          *box);

#endif /* !__SYSTRAY_BOX_H__ */
