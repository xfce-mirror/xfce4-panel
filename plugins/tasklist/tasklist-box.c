/* $Id$ */
/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "tasklist-box.h"



struct _TasklistBoxClass
{
  GtkContainerClass __parent__;
};

struct _TasklistBoxClass
{
  GtkContainer __parent__;
};



static void tasklist_box_class_init              (TasklistBoxClass *klass);
static void tasklist_box_init                    (TasklistBox      *box);
static void tasklist_box_finalize                (GObject          *object);



G_DEFINE_TYPE (TasklistBox, tasklist_box, GTK_TYPE_CONTAINER);


static void
tasklist_box_class_init (TasklistBoxClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = tasklist_box_finalize;
}



static void 
tasklist_box_init (TasklistBox *box)
{
  
}

static void 
tasklist_box_finalize (GObject *object)
{
  //TasklistBox *box = TASKLIST_BOX (object);
  
  (*G_OBJECT_CLASS (tasklist_box_parent_class)->finalize) (object);
}



static void
tasklist_box_connect_window (TasklistBox *box,
                             WnckWindow  *window)
{
  panel_return_if_fail (TASKLIST_IS_BOX (box));
  panel_return_if_fail (WNCK_IS_WINDOW (window));
  
  /* monitor window changes */
  g_signal_connect (G_OBJECT (window), "workspace-changed", G_CALLBACK (tasklist_box_window_changed_workspace), box);
  g_signal_connect (G_OBJECT (window), "geometry-changed", G_CALLBACK (tasklist_box_window_changed_geometry), box);
}



static void
tasklist_box_disconnect_window (TasklistBox *box,
                                WnckWindow  *window)
{
  panel_return_if_fail (TASKLIST_IS_BOX (box));
  panel_return_if_fail (WNCK_IS_WINDOW (window));
  
  /* disconnect window monitor signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (window), G_CALLBACK (tasklist_box_window_changed_workspace), box);
  g_signal_handlers_disconnect_by_func (G_OBJECT (window), G_CALLBACK (tasklist_box_window_changed_geometry), box);
}



static void
tasklist_box_connect_screen (TasklistBox *box,
                             WnckScreen  *screen)
{
  GList *windows, *li;
  
  panel_return_if_fail (TASKLIST_IS_BOX (box));
  panel_return_if_fail (WNCK_IS_SCREEN (screen));
  
  /* monitor screen changes */
  g_signal_connect (G_OBJECT (screen), "active-window-changed", G_CALLBACK (tasklist_box_active_window_changed), box);
  g_signal_connect (G_OBJECT (screen), "active-workspace-changed", G_CALLBACK (tasklist_box_active_workspace_changed), box);
  g_signal_connect (G_OBJECT (screen), "window-opened", G_CALLBACK (tasklist_box_window_added), box);
  g_signal_connect (G_OBJECT (screen), "window-closed", G_CALLBACK (tasklist_box_window_removed), box);
  g_signal_connect (G_OBJECT (screen), "viewports-changed", G_CALLBACK (tasklist_box_viewports_changed), box);
  
  /* get screen windows */
  windows = wnck_screen_get_windows (screen);
  
  /* monitor window changes */
  for (li = windows; li != NULL; li = li->next)
    tasklist_box_connect_window (box, li->data);
}


static void
tasklist_box_disconnect_screen (TasklistBox *box,
                                WnckScreen  *screen)
{
  GList *windows, *li;
  
  panel_return_if_fail (TASKLIST_IS_BOX (box));
  panel_return_if_fail (WNCK_IS_SCREEN (screen));
  
  /* get screen windows */
  windows = wnck_screen_get_windows (screen);
  
  /* disconnect window signals */
  for (li = windows; li != NULL; li = li->next)
    tasklist_box_disconnect_window (box, li->data);
  
  /* disconnect monitor signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), G_CALLBACK (tasklist_box_active_window_changed), box);
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), G_CALLBACK (tasklist_box_active_workspace_changed), box);
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), G_CALLBACK (tasklist_box_window_added), box);
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), G_CALLBACK (tasklist_box_window_removed), box);
  g_signal_handlers_disconnect_by_func (G_OBJECT (screen), G_CALLBACK (tasklist_box_viewports_changed), box);
}
