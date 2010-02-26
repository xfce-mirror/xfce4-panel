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

#ifndef __XFCE_TEST_PLUGIN_H__
#define __XFCE_TEST_PLUGIN_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

typedef struct _XfceTestPluginClass    XfceTestPluginClass;
typedef struct _XfceTestPlugin         XfceTestPlugin;
typedef struct _XfceTestPluginEntry    XfceTestPluginEntry;
typedef enum   _XfceTestPluginArrowPos XfceTestPluginArrowPos;

#define XFCE_TYPE_TEST_PLUGIN            (xfce_test_plugin_get_type ())
#define XFCE_TEST_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_TEST_PLUGIN, XfceTestPlugin))
#define XFCE_TEST_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_TEST_PLUGIN, XfceTestPluginClass))
#define XFCE_IS_TEST_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_TEST_PLUGIN))
#define XFCE_IS_TEST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_TEST_PLUGIN))
#define XFCE_TEST_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_TEST_PLUGIN, XfceTestPluginClass))

#define LIST_HAS_ONE_ENTRY(list)         ((list) != NULL && (list)->next == NULL)
#define LIST_HAS_TWO_OR_MORE_ENTRIES     ((list) != NULL && (list)->next != NULL)

struct _XfceTestPluginClass
{
  XfcePanelPluginClass __parent__;
};

struct _XfceTestPlugin
{
  XfcePanelPlugin __parent__;

  /* settings */
  guint                   move_clicked_to_button : 1;
  guint                   disable_tooltips : 1;
  guint                   show_labels : 1;
  XfceTestPluginArrowPos  arrow_position;

  /* list of entries in the launcher */
  GList                  *entries;
  
  /* store the icon theme */
  GtkIconTheme           *icon_theme;

  /* plugin widgets */
  GtkWidget              *box;
  GtkWidget              *icon_button;
  GtkWidget              *arrow_button;
  GtkWidget              *image;
  GtkWidget              *menu;

  /* delayout menu popup */
  guint                   popup_timeout_id;

  /* whether the menu appends in revered order */
  guint                   menu_reversed_order : 1;
};

struct _XfceTestPluginEntry
{
  gchar *name;
  gchar *comment;
  gchar *exec;
  gchar *icon;
  gchar *path;

  guint  terminal : 1;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  guint  startup_notify : 1;
#endif
};

enum _XfceTestPluginArrowPos
{
  ARROW_POS_DEFAULT,
  ARROW_POS_LEFT,
  ARROW_POS_RIGHT,
  ARROW_POS_TOP,
  ARROW_POS_BOTTOM,
  ARROW_POS_INSIDE_BUTTON  
};

GType      xfce_test_plugin_get_type (void) G_GNUC_CONST;

void       xfce_test_plugin_rebuild  (XfceTestPlugin *plugin, gboolean update_icon);

GdkPixbuf *xfce_test_plugin_load_pixbuf (const gchar *name, gint size, GtkIconTheme *icon_theme);

G_END_DECLS

#endif /* !__XFCE_TEST_PLUGIN_H__ */
