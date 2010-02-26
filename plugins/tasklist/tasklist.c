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
#include <exo/exo.h>
#include <libwnck/libwnck.h>
#include <libxfce4panel/libxfce4panel.h>

#include "tasklist.h"

#define MIN_BUTTON_SIZE                      (25)
#define MAX_BUTTON_LENGTH                    (200)
#define xfce_taskbar_lock()                  G_BEGIN_DECLS { locked++; } G_END_DECLS;
#define xfce_taskbar_unlock()                G_BEGIN_DECLS { if (locked > 0) locked--; else g_assert_not_reached (); } G_END_DECLS;
#define xfce_taskbar_is_locked()             (locked > 0)



enum
{
  PROP_0,
  PROP_STYLE,
  PROP_INCLUDE_ALL_WORKSPACES,
  PROP_FLAT_BUTTONS,
  PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE,
  PROP_SHOW_ONLY_MINIMIZED
};

struct _XfceTasklistClass
{
  GtkContainerClass __parent__;
};

struct _XfceTasklist
{
  GtkContainer __parent__;

  /* the screen of this tasklist */
  WnckScreen        *screen;

  /* all the applications in the tasklist */
  GSList            *children;

  /* number of visible buttons, we cache this to avoid a loop */
  guint              n_visible_children;
  
  /* classgroups of all the windows in the taskbar */
  GSList            *class_groups;

  /* normal or iconbox style */
  XfceTasklistStyle  style;

  /* orientation of the tasklist */
  GtkOrientation     orientation;

  /* relief of the tasklist buttons */
  GtkReliefStyle     button_relief;

  /* whether we show application from all workspaces or
   * only the active workspace */
  guint              all_workspaces : 1;

  /* whether we switch to another workspace when we try to
   * unminimize an application on another workspace */
  guint              switch_workspace : 1;

  /* whether we only show monimized applications in the
   * tasklist */
  guint              only_minimized : 1;
};

struct _XfceTasklistChild
{
  /* pointer to the tasklist */
  XfceTasklist      *tasklist;

  /* button widgets */
  GtkWidget         *button;
  GtkWidget         *box;
  GtkWidget         *icon;
  GtkWidget         *label;

  /* this window of this button */
  WnckWindow        *window;
  
  /* class group of this window */
  WnckClassGroup    *class_group;

  /* unique is for sorting by insert time */
  guint              unique_id;
};

static const GtkTargetEntry drop_targets[] =
{
  { "application/x-xfce-panel-plugin-task", GTK_TARGET_SAME_WIDGET, 0 }
};


static gint locked = 0;


static void xfce_tasklist_class_init (XfceTasklistClass *klass);
static void xfce_tasklist_init (XfceTasklist      *tasklist);
static void xfce_tasklist_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void xfce_tasklist_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void xfce_tasklist_finalize (GObject          *object);
static void xfce_tasklist_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void xfce_tasklist_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void xfce_tasklist_realize (GtkWidget *widget);
static void xfce_tasklist_unrealize (GtkWidget *widget);
static gboolean xfce_tasklist_drag_motion (GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time);
static void xfce_tasklist_remove (GtkContainer *container, GtkWidget *widget);
static void xfce_tasklist_forall (GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);
static GType xfce_tasklist_child_type (GtkContainer *container);
static void xfce_tasklist_connect_screen (XfceTasklist *tasklist);
static void xfce_tasklist_disconnect_screen (XfceTasklist *tasklist);
static void xfce_tasklist_active_window_changed (WnckScreen *screen, WnckWindow  *previous_window, XfceTasklist *tasklist);
static void xfce_tasklist_active_workspace_changed (WnckScreen *screen, WnckWorkspace *previous_workspace, XfceTasklist *tasklist);
static void xfce_tasklist_window_added (WnckScreen *screen, WnckWindow *window, XfceTasklist *tasklist);
static void xfce_tasklist_window_removed (WnckScreen *screen, WnckWindow *window, XfceTasklist *tasklist);
static void xfce_tasklist_viewports_changed (WnckScreen *screen, XfceTasklist *tasklist);
static void tasklist_button_new (XfceTasklistChild *child);
static void xfce_tasklist_set_style (XfceTasklist *tasklist, XfceTasklistStyle style);
static void xfce_tasklist_set_include_all_workspaces (XfceTasklist *tasklist, gboolean all_workspaces);
static void xfce_tasklist_set_button_relief (XfceTasklist *tasklist, GtkReliefStyle button_relief);
static void xfce_tasklist_set_show_only_minimized (XfceTasklist *tasklist, gboolean only_minimized);



G_DEFINE_TYPE (XfceTasklist, xfce_tasklist, GTK_TYPE_CONTAINER);



static void
xfce_tasklist_class_init (XfceTasklistClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = xfce_tasklist_get_property;
  gobject_class->set_property = xfce_tasklist_set_property;
  gobject_class->finalize = xfce_tasklist_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request = xfce_tasklist_size_request;
  gtkwidget_class->size_allocate = xfce_tasklist_size_allocate;
  gtkwidget_class->realize = xfce_tasklist_realize;
  gtkwidget_class->unrealize = xfce_tasklist_unrealize;
  gtkwidget_class->drag_motion = xfce_tasklist_drag_motion;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = NULL;
  gtkcontainer_class->remove = xfce_tasklist_remove;
  gtkcontainer_class->forall = xfce_tasklist_forall;
  gtkcontainer_class->child_type = xfce_tasklist_child_type;
  
  g_object_class_install_property (gobject_class,
                                   PROP_STYLE,
                                   g_param_spec_uint ("style", NULL, NULL,
                                                      XFCE_TASKLIST_STYLE_NORMAL,
                                                      XFCE_TASKLIST_STYLE_ICONBOX,
                                                      XFCE_TASKLIST_STYLE_NORMAL,
                                                      EXO_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class,
                                   PROP_INCLUDE_ALL_WORKSPACES,
                                   g_param_spec_boolean ("include-all-workspaces", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class,
                                   PROP_FLAT_BUTTONS,
                                   g_param_spec_boolean ("flat-buttons", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class,
                                   PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE,
                                   g_param_spec_boolean ("switch-workspace-on-unminimize", NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_ONLY_MINIMIZED,
                                   g_param_spec_boolean ("show-only-minimized", NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
}



static void
xfce_tasklist_init (XfceTasklist *tasklist)
{
  /* setup widget */
  GTK_WIDGET_SET_FLAGS (tasklist, GTK_NO_WINDOW);

  /* initialize */
  tasklist->screen = NULL;
  tasklist->children = NULL;
  tasklist->orientation = GTK_ORIENTATION_HORIZONTAL;
  tasklist->all_workspaces = FALSE;
  tasklist->n_visible_children = 0;
  tasklist->button_relief = GTK_RELIEF_NORMAL;
  tasklist->switch_workspace = TRUE;
  tasklist->only_minimized = FALSE;
  tasklist->style = XFCE_TASKLIST_STYLE_NORMAL;
  tasklist->class_groups = NULL;

  /* set the itembar drag destination targets */
  gtk_drag_dest_set (GTK_WIDGET (tasklist), 0, drop_targets,
                     G_N_ELEMENTS (drop_targets), GDK_ACTION_MOVE);
}



static void 
xfce_tasklist_get_property (GObject    *object, 
                            guint       prop_id, 
                            GValue     *value, 
                            GParamSpec *pspec)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);
  
  switch (prop_id)
    {
      case PROP_STYLE:
        g_value_set_uint (value, tasklist->style);
        break;
        
      case PROP_INCLUDE_ALL_WORKSPACES:
        g_value_set_boolean (value, tasklist->all_workspaces);
        break;
        
      case PROP_FLAT_BUTTONS:
        g_value_set_boolean (value, !!(tasklist->button_relief == GTK_RELIEF_NONE));
        break;
        
      case PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE:
        g_value_set_boolean (value, tasklist->switch_workspace);
        break;
        
      case PROP_SHOW_ONLY_MINIMIZED:
        g_value_set_boolean (value, tasklist->only_minimized);
        break;
    
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void 
xfce_tasklist_set_property (GObject      *object, 
                            guint         prop_id, 
                            const GValue *value, 
                            GParamSpec   *pspec)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);
  
  switch (prop_id)
    {
      case PROP_STYLE:
        /* set the tasklist style */
        xfce_tasklist_set_style (tasklist, g_value_get_uint (value));
        break;
        
      case PROP_INCLUDE_ALL_WORKSPACES:
        /* set include all workspaces */
        xfce_tasklist_set_include_all_workspaces (tasklist, g_value_get_boolean (value));
        break;
        
      case PROP_FLAT_BUTTONS:
        /* set the tasklist relief */
        xfce_tasklist_set_button_relief (tasklist,
                                         g_value_get_boolean (value) ? 
                                           GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);
        break;
        
      case PROP_SWITCH_WORKSPACE_ON_UNMINIMIZE:
        /* set the new value */
        tasklist->switch_workspace = g_value_get_boolean (value);
        break;
        
      case PROP_SHOW_ONLY_MINIMIZED:
        /* whether the tasklist shows only minimized applications */
        xfce_tasklist_set_show_only_minimized (tasklist, g_value_get_boolean (value));
        break;
    
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
xfce_tasklist_finalize (GObject *object)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (object);
  
  g_return_if_fail (tasklist->children == NULL);
  g_return_if_fail (tasklist->class_groups == NULL);

  (*G_OBJECT_CLASS (xfce_tasklist_parent_class)->finalize) (object);
}



static void
xfce_tasklist_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  XfceTasklist      *tasklist = XFCE_TASKLIST (widget);
  GSList            *li;
  GtkRequisition     child_requisition;
  XfceTasklistChild *child;
  guint              n;
  gint               max_length = MAX_BUTTON_LENGTH;

  /* initialize */
  requisition->width = GTK_CONTAINER (widget)->border_width * 2;
  requisition->height = requisition->width;

  /* keep the buttons small when we're in iconbox mode */
  if (tasklist->style == XFCE_TASKLIST_STYLE_ICONBOX)
    max_length = 0;

  for (li = tasklist->children, n = 0; li != NULL; li = li->next)
    {
      child = li->data;

      if (GTK_WIDGET_VISIBLE (child->button) == FALSE)
        continue;

      gtk_widget_size_request (child->button, &child_requisition);

      if (tasklist->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          requisition->width += MIN (child_requisition.width, max_length);
          //requisition->height = MAX (requisition->height, child_requisition.height);
        }
      else
        {
          requisition->height += MIN (child_requisition.height, max_length);
          //requisition->width = MAX (requisition->width, child_requisition.width);
        }

      n++;
    }

  /* update the count */
  tasklist->n_visible_children = n;
}



static void
xfce_tasklist_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  XfceTasklist      *tasklist = XFCE_TASKLIST (widget);
  guint              rows, cols;
  gint               width, height;
  GSList            *li;
  XfceTasklistChild *child = NULL;
  guint              i;
  GtkAllocation      child_allocation;

  /* set widget allocation */
  widget->allocation = *allocation;

  /* leave when the tasklist is empty */
  if (tasklist->n_visible_children == 0)
    return;

  if (tasklist->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* get the number of rows and columns */
      rows = MAX (allocation->height / MIN_BUTTON_SIZE, 1);
      cols = MAX (tasklist->n_visible_children / rows, 1);

      /* make sure the children fit and use all the space */
      if (cols * rows < tasklist->n_visible_children)
        cols++;
      else if (rows > tasklist->n_visible_children)
        rows = tasklist->n_visible_children;

      /* calculate the size of the buttons */
      width = MIN (allocation->width / cols, MAX_BUTTON_LENGTH);
      height = allocation->height / rows;

      if (tasklist->style == XFCE_TASKLIST_STYLE_ICONBOX && height < width)
        width = height;
    }
  else
    {
      /* get the number of rows and columns */
      rows = MAX (allocation->width / MIN_BUTTON_SIZE, 1);
      cols = MAX (tasklist->n_visible_children / rows, 1);

      /* make sure the children fit and use all the space */
      if (cols * rows < tasklist->n_visible_children)
        cols++;
      else if (rows > tasklist->n_visible_children)
        rows = tasklist->n_visible_children;

      /* calculate the size of the buttons */
      width = allocation->width / rows;
      height = MIN (allocation->height / cols, MAX_BUTTON_LENGTH);

      if (tasklist->style == XFCE_TASKLIST_STYLE_ICONBOX && width < height)
        height = width;
    }

  /* initialize the child allocation */
  child_allocation.width = width;
  child_allocation.height = height;

  /* allocate all the children */
  for (li = tasklist->children, i = 0; li != NULL; li = li->next)
    {
      child = li->data;

      /* skip hidden buttons */
      if (GTK_WIDGET_VISIBLE (child->button) == FALSE)
        continue;

      /* calculate the child position */
      child_allocation.x = allocation->x + (i / rows) * width;
      child_allocation.y = allocation->y + (i % rows) * height;

      /* allocate the child */
      gtk_widget_size_allocate (child->button, &child_allocation);

      /* increase the position counter */
      i++;
    }

  /* check if those two are the same, they should... */
  g_assert (i == tasklist->n_visible_children);
}



static void
xfce_tasklist_realize (GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  /* let gtk realize the widget */
  (*GTK_WIDGET_CLASS (xfce_tasklist_parent_class)->realize) (widget);

  /* connect to the screen */
  xfce_tasklist_connect_screen (tasklist);
}



static void
xfce_tasklist_unrealize (GtkWidget *widget)
{
  XfceTasklist *tasklist = XFCE_TASKLIST (widget);

  /* disconnect from the screen */
  xfce_tasklist_disconnect_screen (tasklist);

  /* let gtk unrealize the widget */
  (*GTK_WIDGET_CLASS (xfce_tasklist_parent_class)->unrealize) (widget);
}



static gboolean
xfce_tasklist_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
  XfceTasklist      *tasklist = XFCE_TASKLIST (widget);
  XfceTasklistChild *child;
  GtkWidget         *button;
  GtkAllocation     *alloc;
  GSList            *li, *source = NULL, *sibling = NULL;

  /* get de button we're dragging */
  button = gtk_drag_get_source_widget (context);

  /* add the widget coordinates to the drag */
  x += widget->allocation.x;
  y += widget->allocation.y;

  /* get the button allocation */
  alloc = &button->allocation;

  /* check if we're not dragging inside this button (and half the size around it) */
  if (alloc->x - alloc->width / 2 >= x || alloc->x + alloc->width * 1.5 <= x
      || alloc->y - alloc->height / 2 >= y || alloc->y + alloc->height * 1.5 <= y)
    {
      /* walk the children in the box */
      for (li = tasklist->children; li != NULL; li = li->next)
        {
          child = li->data;

          if (child->button == button)
            {
              /* we've found the drag source */
              source = li;

              /* break if we already found a sibling */
              if (sibling)
                break;
            }
          else if (sibling == NULL)
            {
              /* get the allocation of the button in the list */
              alloc = &child->button->allocation;

              /* check if we hover this button */
              if (x >= alloc->x && x <= alloc->x + alloc->width
                  && y >= alloc->y && y <= alloc->y + alloc->height)
                {
                  if (source == NULL)
                    {
                      /* there is no source yet, so insert before this child */
                      sibling = li;
                    }
                  else
                    {
                      /* there is a source, so insert after this child */
                      sibling = li->next;

                      break;
                    }
                }
            }
        }

      if (G_LIKELY (source))
        {
          /* get the child data */
          child = source->data;

          /* remove the link in the list */
          tasklist->children = g_slist_delete_link (tasklist->children, source);

          /* insert in the new position */
          tasklist->children = g_slist_insert_before (tasklist->children, sibling, child);

          /* update the tasklist */
          gtk_widget_queue_resize (widget);
        }
    }

  /* update the drag status so we keep receiving the drag motions */
  gdk_drag_status (context, 0, time);

  return TRUE;
}



static void
xfce_tasklist_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  XfceTasklist      *tasklist = XFCE_TASKLIST (container);
  gboolean           was_visible;
  XfceTasklistChild *child;
  GSList *li;

  for (li = tasklist->children; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->button == widget)
        {
          /* remove the child from the list */
          tasklist->children = g_slist_delete_link (tasklist->children, li);

          /* whether the widget is currently visible */
          was_visible = GTK_WIDGET_VISIBLE (widget);

          /* remove from the itembar */
          gtk_widget_unparent (child->button);

          /* cleanup the slice */
          g_slice_free (XfceTasklistChild, child);

          /* queue a resize if needed */
          if (G_LIKELY (was_visible))
            gtk_widget_queue_resize (GTK_WIDGET (container));

          /* done */
          break;
        }
    }
}



static void
xfce_tasklist_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  XfceTasklist      *tasklist = XFCE_TASKLIST (container);
  GSList            *children = tasklist->children;
  XfceTasklistChild *child;

  while (children != NULL)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->button, callback_data);
    }
}



static GType
xfce_tasklist_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}



static void
xfce_tasklist_connect_screen (XfceTasklist *tasklist)
{
  GdkScreen *screen;

  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == NULL);

  /* get the widget screen */
  screen = gtk_widget_get_screen (GTK_WIDGET (tasklist));

  /* set the wnck screen */
  tasklist->screen = wnck_screen_get (gdk_screen_get_number (screen));

  /* monitor screen changes */
  g_signal_connect (G_OBJECT (tasklist->screen), "active-window-changed", G_CALLBACK (xfce_tasklist_active_window_changed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "active-workspace-changed", G_CALLBACK (xfce_tasklist_active_workspace_changed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "window-opened", G_CALLBACK (xfce_tasklist_window_added), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "window-closed", G_CALLBACK (xfce_tasklist_window_removed), tasklist);
  g_signal_connect (G_OBJECT (tasklist->screen), "viewports-changed", G_CALLBACK (xfce_tasklist_viewports_changed), tasklist);
}



static void
xfce_tasklist_disconnect_screen (XfceTasklist *tasklist)
{
  GSList            *li, *lnext;
  XfceTasklistChild *child;
  
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (WNCK_IS_SCREEN (tasklist->screen));

  /* disconnect monitor signals */
  g_signal_handlers_disconnect_by_func (G_OBJECT (tasklist->screen), G_CALLBACK (xfce_tasklist_active_window_changed), tasklist);
  g_signal_handlers_disconnect_by_func (G_OBJECT (tasklist->screen), G_CALLBACK (xfce_tasklist_active_workspace_changed), tasklist);
  g_signal_handlers_disconnect_by_func (G_OBJECT (tasklist->screen), G_CALLBACK (xfce_tasklist_window_added), tasklist);
  g_signal_handlers_disconnect_by_func (G_OBJECT (tasklist->screen), G_CALLBACK (xfce_tasklist_window_removed), tasklist);
  g_signal_handlers_disconnect_by_func (G_OBJECT (tasklist->screen), G_CALLBACK (xfce_tasklist_viewports_changed), tasklist);
  
  /* remove all the windows */
  for (li = tasklist->children; li != NULL; li = lnext)
    {
      lnext = li->next;
      child = li->data;
      
      /* do a fake window remove */
      xfce_tasklist_window_removed (tasklist->screen, child->window, tasklist);
    }

  /* unset the screen */
  tasklist->screen = NULL;
}



static void
xfce_tasklist_active_window_changed (WnckScreen   *screen,
                                     WnckWindow   *previous_window,
                                     XfceTasklist *tasklist)
{
  WnckWindow        *active_window;
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (previous_window == NULL || WNCK_IS_WINDOW (previous_window));
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == screen);

  /* get the new active window */
  active_window = wnck_screen_get_active_window (screen);

  /* lock the taskbar */
  xfce_taskbar_lock ();

  for (li = tasklist->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* skip hidden buttons */
      if (GTK_WIDGET_VISIBLE (child->button) == FALSE
          || !(child->window == previous_window || child->window == active_window))
        continue;

      /* set the toggle button state */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (child->button),
                                    !!(child->window == active_window));
    }

  /* release the lock */
  xfce_taskbar_unlock ();
}



static void
xfce_tasklist_active_workspace_changed (WnckScreen    *screen,
                                        WnckWorkspace *previous_workspace,
                                        XfceTasklist  *tasklist)
{
  GSList            *li;
  WnckWorkspace     *active_ws;
  XfceTasklistChild *child;

  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (previous_workspace == NULL || WNCK_IS_WORKSPACE (previous_workspace));
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == screen);

  /* leave when we show tasks from all workspaces or are locked */
  if (xfce_taskbar_is_locked () || tasklist->all_workspaces == TRUE)
    return;

  /* get the new active workspace */
  active_ws = wnck_screen_get_active_workspace (screen);

  /* walk all the children and hide buttons on other workspaces */
  for (li = tasklist->children; li != NULL; li = li->next)
    {
      child = li->data;

      if (wnck_window_get_workspace (child->window) == active_ws
          && (!tasklist->only_minimized || wnck_window_is_minimized (child->window)))
        gtk_widget_show (child->button);
      else
        gtk_widget_hide (child->button);
    }
}



static void
xfce_tasklist_window_added (WnckScreen   *screen,
                            WnckWindow   *window,
                            XfceTasklist *tasklist)
{
  XfceTasklistChild *child;
  static guint       unique_id_counter = 0;
  GSList            *li;
  WnckClassGroup    *class_group;

  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (WNCK_IS_WINDOW (window));
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == screen);

  if (wnck_window_is_skip_tasklist (window))
    return;

  /* allocate new child */
  child = g_slice_new0 (XfceTasklistChild);
  child->tasklist = tasklist;
  child->window = window;
  child->unique_id = unique_id_counter++;
  
  /* get the class group of the new window */
  class_group = wnck_window_get_class_group (window);
  
  if (G_LIKELY (class_group))
    {
      /* try to find the class group in the list */
      for (li = tasklist->class_groups; li != NULL; li = li->next)
        if (li->data == class_group)
          break;

      /* prepend the class group if it's new */
      if (li == NULL)
        tasklist->class_groups = g_slist_prepend (tasklist->class_groups, class_group);
        
      /* set the class group */
      child->class_group = g_object_ref (G_OBJECT (class_group));
    }
  else
    {
      child->class_group = NULL;
    }

  /* create the application button */
  tasklist_button_new (child);

  /* insert in the internal list */
  tasklist->children = g_slist_append (tasklist->children, child);

  /* set the parent */
  gtk_widget_set_parent (child->button, GTK_WIDGET (tasklist));

  /* resize the itembar */
  gtk_widget_queue_resize (GTK_WIDGET (tasklist));
}



static void
xfce_tasklist_window_removed (WnckScreen   *screen,
                              WnckWindow   *window,
                              XfceTasklist *tasklist)
{
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (WNCK_IS_WINDOW (window));
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == screen);
    
  /* remove the child from the taskbar */
  for (li = tasklist->children; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->window == window)
        {
          if (child->class_group)
            {
              /* remove the class group from the internal list if this was the last
               * window in the group */
              if (g_list_length (wnck_class_group_get_windows (child->class_group)) == 1)
                tasklist->class_groups = g_slist_remove (tasklist->class_groups, child->class_group);
              
              /* release the class group */
              g_object_unref (G_OBJECT (child->class_group));
            }
          
          /* destroy the button */
          gtk_widget_destroy (child->button);

          break;
        }
    }
}



static void
xfce_tasklist_viewports_changed (WnckScreen   *screen,
                                 XfceTasklist *tasklist)
{
  g_return_if_fail (WNCK_IS_SCREEN (screen));
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
  g_return_if_fail (tasklist->screen == screen);
}



/**************************************************************
 **************************************************************
 **************************************************************/
static void
tasklist_button_icon_changed (WnckWindow        *window,
                              XfceTasklistChild *child)
{
  GdkPixbuf *pixbuf;
  GdkPixbuf *lucent;

  /* get the application icon */
  if (child->tasklist->style == XFCE_TASKLIST_STYLE_NORMAL)
    pixbuf = wnck_window_get_mini_icon (window);
  else
    pixbuf = wnck_window_get_icon (window);

  /* leave when there is no valid pixbuf */
  if (G_UNLIKELY (pixbuf == NULL))
    return;

  /* create a spotlight version of the icon when minimized */
  if (child->tasklist->only_minimized == FALSE
      && wnck_window_is_minimized (window))
    {
      /* create lightened version */
      lucent = exo_gdk_pixbuf_lucent (pixbuf, 50);

      if (G_LIKELY (lucent))
        {
          /* set the button icon */
          xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (child->icon), lucent);

          /* release the pixbuf */
          g_object_unref (G_OBJECT (lucent));
        }
    }
  else
    {
      /* set the button icon */
      xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (child->icon), pixbuf);
    }
}



static void
tasklist_button_name_changed (WnckWindow        *window,
                              XfceTasklistChild *child)
{
  const gchar *name;
  gchar       *label = NULL;

  g_return_if_fail (child->window == window);

  /* get the window name */
  name = wnck_window_get_name (window);

  /* set the tooltip */
  gtk_widget_set_tooltip_text (GTK_WIDGET (child->button), name);

  /* create the button label */
  if (!child->tasklist->only_minimized
      && wnck_window_is_minimized (window))
    name = label = g_strdup_printf ("[%s]", name);
  else if (wnck_window_is_shaded (window))
    name = label = g_strdup_printf ("=%s=", name);

  /* set the label */
  gtk_label_set_text (GTK_LABEL (child->label), name);

  /* cleanup */
  g_free (label);
}



static void
tasklist_button_state_changed (WnckWindow        *window,
                               WnckWindowState    changed_state,
                               WnckWindowState    new_state,
                               XfceTasklistChild *child)
{
  /* update the button name */
  if ((changed_state & (WNCK_WINDOW_STATE_SHADED | WNCK_WINDOW_STATE_MINIMIZED)) != 0
      && !child->tasklist->only_minimized)
    tasklist_button_name_changed (window, child);

  /* update the button icon if needed */
  if ((changed_state & WNCK_WINDOW_STATE_MINIMIZED) != 0)
    {
      if (G_UNLIKELY (child->tasklist->only_minimized))
        {
          if ((new_state & WNCK_WINDOW_STATE_MINIMIZED) != 0)
            gtk_widget_show (child->button);
          else
            gtk_widget_hide (child->button);
        }
      else
        {
          /* update the icon (lucient) */
          tasklist_button_icon_changed (window, child);
        }
    }
}



static void
tasklist_button_workspace_changed (WnckWindow        *window,
                                   XfceTasklistChild *child)
{
  /* leave when we show application from all workspaces */
  if (child->tasklist->all_workspaces)
    return;

  /* hide the button */
  gtk_widget_hide (child->button);
}



static gboolean
tasklist_button_toggled (GtkToggleButton   *button,
                         XfceTasklistChild *child)
{
  WnckWorkspace *workspace;
  guint32        timestamp;

  /* leave when the taskbar is locked */
  if (xfce_taskbar_is_locked ())
    return TRUE;

  if (wnck_window_is_active (child->window))
    {
      /* minimize the window */
      wnck_window_minimize (child->window);
    }
  else
    {
      /* current event time */
      timestamp = gtk_get_current_event_time ();

      /* only switch workspaces if we show application from other workspaces
       * don't switch when switch on minimize is disabled and the window is minimized */
      if (child->tasklist->all_workspaces
          && (!wnck_window_is_minimized (child->window) || child->tasklist->switch_workspace))
        {
          /* get the screen of this window and the workspaces */
          workspace = wnck_window_get_workspace (child->window);

          /* switch to the correct workspace */
          if (workspace && workspace != wnck_screen_get_active_workspace (child->tasklist->screen))
            wnck_workspace_activate (workspace, timestamp - 1);
        }

      /* active the window */
      wnck_window_activate_transient (child->window, timestamp);
    }

  return TRUE;
}



static void
tasklist_button_new (XfceTasklistChild *child)
{
  WnckWindow *window = child->window;

  /* create the application button */
  child->button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (child->button), child->tasklist->button_relief);
  g_signal_connect (G_OBJECT (child->button), "toggled", G_CALLBACK (tasklist_button_toggled), child);
  gtk_widget_show (child->button);

  child->box = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (child->button), child->box);
  gtk_widget_show (child->box);

  child->icon = xfce_scaled_image_new ();
  if (child->tasklist->style == XFCE_TASKLIST_STYLE_NORMAL)
    gtk_box_pack_start (GTK_BOX (child->box), child->icon, FALSE, FALSE, 0);
  else
    gtk_box_pack_start (GTK_BOX (child->box), child->icon, TRUE, TRUE, 0);
  gtk_widget_show (child->icon);

  child->label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (child->box), child->label, TRUE, TRUE, 0);
  gtk_misc_set_alignment (GTK_MISC (child->label), 0.0, 0.5);
  gtk_label_set_ellipsize (GTK_LABEL (child->label), PANGO_ELLIPSIZE_END);

  /* don't show the icon if we're in iconbox style */
  if (child->tasklist->style == XFCE_TASKLIST_STYLE_NORMAL)
    gtk_widget_show (child->label);

  /* set the button's drag source */
  gtk_drag_source_set (child->button, GDK_BUTTON1_MASK, drop_targets, G_N_ELEMENTS (drop_targets), GDK_ACTION_MOVE);

  /* monitor window changes */
  g_signal_connect (G_OBJECT (window), "icon-changed", G_CALLBACK (tasklist_button_icon_changed), child);
  g_signal_connect (G_OBJECT (window), "name-changed", G_CALLBACK (tasklist_button_name_changed), child);
  g_signal_connect (G_OBJECT (window), "state-changed", G_CALLBACK (tasklist_button_state_changed), child);
  g_signal_connect (G_OBJECT (window), "workspace-changed", G_CALLBACK (tasklist_button_workspace_changed), child);

  /* poke functions */
  tasklist_button_icon_changed (window, child);
  tasklist_button_name_changed (window, child);
}



static void
xfce_tasklist_set_style (XfceTasklist      *tasklist,
                         XfceTasklistStyle  style)
{
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->style != style)
    {
      /* set new value */
      tasklist->style = style;

      /* change the mode of all the buttons */
      for (li = tasklist->children; li != NULL; li = li->next)
        {
          child = li->data;

          /* show or hide the label */
          if (style == XFCE_TASKLIST_STYLE_NORMAL)
            {
              gtk_widget_show (child->label);
              gtk_box_set_child_packing (GTK_BOX (child->box), child->icon,
                                         FALSE, FALSE, 0, GTK_PACK_START);
            }
          else /* XFCE_TASKLIST_STYLE_ICONBOX */
            {
              gtk_widget_hide (child->label);
              gtk_box_set_child_packing (GTK_BOX (child->box), child->icon,
                                         TRUE, TRUE, 0, GTK_PACK_START);
            }

          /* update the icon */
          tasklist_button_icon_changed (child->window, child);
        }
    }
}



static void
xfce_tasklist_set_include_all_workspaces (XfceTasklist *tasklist,
                                          gboolean      all_workspaces)
{
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  /* normalize the value */
  all_workspaces = !!all_workspaces;

  if (tasklist->all_workspaces != all_workspaces)
    {
      /* set new value */
      tasklist->all_workspaces = all_workspaces;

      if (all_workspaces)
        {
          /* make sure all buttons are visible */
          for (li = tasklist->children; li != NULL; li = li->next)
            {
              child = li->data;
              gtk_widget_show (child->button);
            }
        }
      else
        {
          /* trigger signal */
          xfce_tasklist_active_workspace_changed (tasklist->screen, NULL, tasklist);
        }
    }
}



static void
xfce_tasklist_set_button_relief (XfceTasklist   *tasklist,
                                 GtkReliefStyle  button_relief)
{
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->button_relief != button_relief)
    {
      /* set the new value */
      tasklist->button_relief = button_relief;

      /* change the relief of all buttons in the list */
      for (li = tasklist->children; li != NULL; li = li->next)
        {
          child = li->data;
          gtk_button_set_relief (GTK_BUTTON (child->button), button_relief);
        }
    }
}



static void
xfce_tasklist_set_show_only_minimized (XfceTasklist *tasklist,
                                       gboolean      only_minimized)
{
  GSList            *li;
  XfceTasklistChild *child;

  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));

  if (tasklist->only_minimized != only_minimized)
    {
      /* set the new value */
      tasklist->only_minimized = !!only_minimized;

      /* update the tray */
      for (li = tasklist->children; li != NULL; li = li->next)
        {
          child = li->data;

          /* update the icons of the minimized windows */
          if (wnck_window_is_minimized (child->window))
            {
              tasklist_button_icon_changed (child->window, child);
              tasklist_button_name_changed (child->window, child);
            }

          /* if we show all workspaces, update the visibility here */
          if (tasklist->all_workspaces == TRUE
              && wnck_window_is_minimized (child->window) == FALSE)
            {
              if (only_minimized)
                gtk_widget_hide (child->button);
              else
                gtk_widget_show (child->button);
            }
        }

      /* update the buttons when we show only the active workspace */
      if (tasklist->all_workspaces == FALSE)
        xfce_tasklist_active_workspace_changed (tasklist->screen, NULL, tasklist);
    }
}



void
xfce_tasklist_set_orientation (XfceTasklist   *tasklist,
                               GtkOrientation  orientation)
{
  g_return_if_fail (XFCE_IS_TASKLIST (tasklist));
}
