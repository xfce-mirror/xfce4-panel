/* $Id$ */
/*
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

#ifndef __PANEL_ITEMBAR_H__
#define __PANEL_ITEMBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _PanelItembarClass PanelItembarClass;
typedef struct _PanelItembar      PanelItembar;
typedef struct _PanelItembarChild PanelItembarChild;

enum
{
  PANEL_ITEMBAR_TARGET_PLUGIN_NAME,
  PANEL_ITEMBAR_TARGET_PLUGIN_WIDGET
};

#define PANEL_TYPE_ITEMBAR            (panel_itembar_get_type ())
#define PANEL_ITEMBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PANEL_TYPE_ITEMBAR, PanelItembar))
#define PANEL_ITEMBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANEL_TYPE_ITEMBAR, PanelItembarClass))
#define PANEL_IS_ITEMBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PANEL_TYPE_ITEMBAR))
#define PANEL_IS_ITEMBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANEL_TYPE_ITEMBAR))
#define PANEL_ITEMBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANEL_TYPE_ITEMBAR, PanelItembarClass))

#define panel_itembar_append(itembar,widget)  panel_itembar_insert ((itembar), (widget), -1)
#define panel_itembar_prepend(itembar,widget) panel_itembar_insert ((itembar), (widget), 0)

GType           panel_itembar_get_type              (void) G_GNUC_CONST;

GtkWidget      *panel_itembar_new                   (void);

void            panel_itembar_set_sensitive         (PanelItembar   *itembar,
                                                     gboolean        sensitive);

void            panel_itembar_insert                (PanelItembar   *itembar,
                                                     GtkWidget      *widget,
                                                     gint            position);

void            panel_itembar_reorder_child         (PanelItembar   *itembar,
                                                     GtkWidget      *widget,
                                                     gint            position);

gboolean        panel_itembar_get_child_expand      (PanelItembar   *itembar,
                                                     GtkWidget      *widget);

void            panel_itembar_set_child_expand      (PanelItembar   *itembar,
                                                     GtkWidget      *widget,
                                                     gboolean        expand);

GSList         *panel_itembar_get_children          (PanelItembar   *itembar);

gint            panel_itembar_get_child_index       (PanelItembar   *itembar,
                                                     GtkWidget      *widget);

GtkWidget      *panel_itembar_get_nth_child         (PanelItembar   *itembar,
                                                     guint           idx);

guint           panel_itembar_get_drop_index        (PanelItembar   *itembar,
                                                     gint            x,
                                                     gint            y);

GtkWidget      *panel_itembar_get_child_at_position (PanelItembar   *itembar,
                                                     gint            x,
                                                     gint            y);

G_END_DECLS

#endif /* !__PANEL_ITEMBAR_H__ */

