/*  $Id$
 *
 *  Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
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

#ifndef __PANEL_H__
#define __PANEL_H__

#include <glib-object.h>
#include <gtk/gtkwidget.h>

#include <libxfce4panel/xfce-panel-window.h>
#include <libxfce4panel/xfce-panel-enums.h>

#define PANEL_TYPE_PANEL            (panel_get_type ())
#define PANEL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_PANEL, Panel))
#define PANEL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_PANEL, PanelClass))
#define PANEL_IS_PANEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_PANEL))
#define PANEL_IS_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_PANEL))
#define PANEL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_PANEL, PanelClass))


G_BEGIN_DECLS

typedef struct _Panel               Panel;
typedef struct _PanelClass          PanelClass;

struct _Panel
{
    XfcePanelWindow parent;
    /*< private >*/
    gpointer priv;
};

struct _PanelClass
{
    XfcePanelWindowClass parent_class;
};

GType panel_get_type (void) G_GNUC_CONST;

Panel *panel_new (void);

void panel_free_data (Panel *panel);


/* adding items */
GtkWidget *panel_add_item (Panel *panel, const char *name);

GtkWidget *panel_insert_item (Panel *panel, const char *name, int position);

GtkWidget *panel_add_item_with_id (Panel *panel, const char *name,
                                   const char *id);

/* configuration */
GList *panel_get_item_list (Panel *panel);

void panel_save_items (Panel *panel);


/* convenience */

gboolean panel_is_horizontal (Panel *panel);

void panel_set_items_sensitive (Panel *panel, gboolean sensitive);


G_END_DECLS

#endif /* !__PANEL_H__ */
