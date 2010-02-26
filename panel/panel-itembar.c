/* $Id$ */
/*
 * Copyright (C) 2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <exo/exo.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-itembar.h>



#define OFFSCREEN           (-9999)
#define HIGHLIGHT_THINKNESS (2)



static void      panel_itembar_class_init       (PanelItembarClass *klass);
static void      panel_itembar_init             (PanelItembar      *itembar);
static void      panel_itembar_set_property     (GObject           *object,
                                                 guint              prop_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void      panel_itembar_finalize         (GObject           *object);
static void      panel_itembar_realize          (GtkWidget         *widget);
static void      panel_itembar_unrealize        (GtkWidget         *widget);
static void      panel_itembar_map              (GtkWidget         *widget);
static void      panel_itembar_unmap            (GtkWidget         *widget);
static gboolean  panel_itembar_expose_event     (GtkWidget         *widget,
                                                 GdkEventExpose    *event);
static void      panel_itembar_size_request     (GtkWidget         *widget,
                                                 GtkRequisition    *requisition);
static void      panel_itembar_size_allocate    (GtkWidget         *widget,
                                                 GtkAllocation     *allocation);
static gboolean  panel_itembar_drag_motion      (GtkWidget         *widget,
                                                 GdkDragContext    *drag_context,
                                                 gint               drag_x,
                                                 gint               drag_y,
                                                 guint              time);
static void      panel_itembar_drag_leave       (GtkWidget         *widget,
                                                 GdkDragContext    *drag_context,
                                                 guint              time);
static void      panel_itembar_add              (GtkContainer      *container,
                                                 GtkWidget         *child);
static void      panel_itembar_remove           (GtkContainer      *container,
                                                 GtkWidget         *child);
static void      panel_itembar_forall           (GtkContainer      *container,
                                                 gboolean           include_internals,
                                                 GtkCallback        callback,
                                                 gpointer           callback_data);
static GType     panel_itembar_child_type       (GtkContainer      *container);



struct _PanelItembarClass
{
  GtkContainerClass __parent__;
};

struct _PanelItembar
{
  GtkContainer __parent__;

  /* window to send all events to the itembar */
  GdkWindow      *event_window;

  /* dnd highlight line */
  GdkWindow      *highlight_window;

  /* whether the itembar is horizontal */
  guint           horizontal : 1;

  /* internal list of children */
  GSList         *children;

  /* current sensitivity state */
  guint           sensitive : 1;
};

struct _PanelItembarChild
{
  GtkWidget *widget;
  guint      expand : 1;
};

enum
{
  PROP_0,
  PROP_HORIZONTAL
};



G_DEFINE_TYPE (PanelItembar, panel_itembar, GTK_TYPE_CONTAINER);



/* drop targets */
static const GtkTargetEntry drop_targets[] =
{
  { "application/x-xfce-panel-plugin-name", 0, PANEL_ITEMBAR_TARGET_PLUGIN_NAME },
  { "application/x-xfce-panel-plugin-widget", 0, PANEL_ITEMBAR_TARGET_PLUGIN_WIDGET },
};



static void
panel_itembar_class_init (PanelItembarClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = panel_itembar_set_property;
  gobject_class->finalize = panel_itembar_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = panel_itembar_realize;
  gtkwidget_class->unrealize = panel_itembar_unrealize;
  gtkwidget_class->map = panel_itembar_map;
  gtkwidget_class->unmap = panel_itembar_unmap;
  gtkwidget_class->expose_event = panel_itembar_expose_event;
  gtkwidget_class->size_request = panel_itembar_size_request;
  gtkwidget_class->size_allocate = panel_itembar_size_allocate;
  gtkwidget_class->drag_motion = panel_itembar_drag_motion;
  gtkwidget_class->drag_leave = panel_itembar_drag_leave;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = panel_itembar_add;
  gtkcontainer_class->remove = panel_itembar_remove;
  gtkcontainer_class->forall = panel_itembar_forall;
  gtkcontainer_class->child_type = panel_itembar_child_type;
  gtkcontainer_class->get_child_property = NULL;
  gtkcontainer_class->set_child_property = NULL;

  g_object_class_install_property (gobject_class,
                                   PROP_HORIZONTAL,
                                   g_param_spec_boolean ("horizontal", NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_WRITABLE));
}



static void
panel_itembar_init (PanelItembar *itembar)
{
  /* initialize */
  itembar->children = NULL;
  itembar->event_window = NULL;
  itembar->highlight_window = NULL;
  itembar->sensitive = TRUE;
  itembar->horizontal = TRUE;

  /* setup */
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (itembar), GTK_NO_WINDOW);

  /* don't redraw on allocation */
  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (itembar), FALSE);

  /* set the itembar drag destination targets */
  gtk_drag_dest_set (GTK_WIDGET (itembar), GTK_DEST_DEFAULT_MOTION,
                     drop_targets, G_N_ELEMENTS (drop_targets),
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);
}



static void
panel_itembar_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  PanelItembar *itembar = PANEL_ITEMBAR (object);

  switch (prop_id)
    {
      case PROP_HORIZONTAL:
        itembar->horizontal = g_value_get_boolean (value);
        gtk_widget_queue_resize (GTK_WIDGET (itembar));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}



static void
panel_itembar_finalize (GObject *object)
{
  panel_return_if_fail (PANEL_ITEMBAR (object)->children == NULL);

  (*G_OBJECT_CLASS (panel_itembar_parent_class)->finalize) (object);
}



static void
panel_itembar_realize (GtkWidget *widget)
{
  PanelItembar  *itembar = PANEL_ITEMBAR (widget);
  GdkWindowAttr  attributes;

  /* let gtk handle it's own realation first */
  (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->realize) (widget);

  /* setup the window attributes */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget);

  /* allocate the event window */
  itembar->event_window = gdk_window_new (GDK_WINDOW (widget->window), &attributes, GDK_WA_X | GDK_WA_Y);

  /* set the window user data */
  gdk_window_set_user_data (GDK_WINDOW (itembar->event_window), widget);
}



static void
panel_itembar_unrealize (GtkWidget *widget)
{
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  /* destroy the event window */
  if (G_LIKELY (itembar->event_window))
    {
      gdk_window_set_user_data (itembar->event_window, NULL);
      gdk_window_destroy (itembar->event_window);
      itembar->event_window = NULL;
    }

  (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->unrealize) (widget);
}



static void
panel_itembar_map (GtkWidget *widget)
{
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  /* show the event window */
  if (G_LIKELY (itembar->event_window))
    gdk_window_show (itembar->event_window);

  (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->map) (widget);

  /* raise the window if we're in insensitive mode */
  if (G_UNLIKELY (!itembar->sensitive && itembar->event_window))
    gdk_window_raise (itembar->event_window);
}



static void
panel_itembar_unmap (GtkWidget *widget)
{
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  /* hide the event window */
  if (G_LIKELY (itembar->event_window))
    gdk_window_hide (itembar->event_window);

  (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->unmap) (widget);
}



static gboolean
panel_itembar_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->expose_event) (widget, event);

  /* keep our event window on top */
  if (itembar->sensitive == FALSE && itembar->event_window)
    gdk_window_raise (itembar->event_window);

  return TRUE;
}



static void
panel_itembar_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (widget);
  GSList            *li;
  PanelItembarChild *child;
  GtkRequisition     child_requisition;

  /* initialize */
  requisition->width = GTK_CONTAINER (widget)->border_width * 2;
  requisition->height = requisition->width;

  /* walk the childeren */
  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      if (G_LIKELY (GTK_WIDGET_VISIBLE (child->widget)))
        {
          /* get the child size request */
          gtk_widget_size_request (child->widget, &child_requisition);

          /* update the itembar requisition */
          if (itembar->horizontal)
            {
              requisition->width += child_requisition.width;
              requisition->height = MAX (requisition->height, child_requisition.height);
            }
          else
            {
              requisition->height += child_requisition.height;
              requisition->width = MAX (requisition->width, child_requisition.width);
            }
        }
    }
}



static void
panel_itembar_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (widget);
  GSList            *li;
  PanelItembarChild *child;
  GtkRequisition     child_requisition;
  GtkAllocation      child_allocation;
  guint              n_expand_children = 0;
  gint               alloc_expandable_size;
  gint               req_expandable_size = 0;
  gint               x, y;
  gint               border_width;
  gint               alloc_size, req_size;
  gboolean           expandable_children_fit;

  /* set widget allocation */
  widget->allocation = *allocation;

  /* allocate the event window */
  if (G_LIKELY (itembar->event_window))
    gdk_window_move_resize (GDK_WINDOW (itembar->event_window), allocation->x,
                            allocation->y, allocation->width, allocation->height);

  /* get the border width */
  border_width = GTK_CONTAINER (widget)->border_width;

  /* get the itembar size */
  if (itembar->horizontal)
    alloc_expandable_size = allocation->width - 2 * border_width;
  else
    alloc_expandable_size = allocation->height - 2 * border_width;

  /* walk the children to get the (remaining) expandable length */
  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* skip hidden widgets */
      if (GTK_WIDGET_VISIBLE (child->widget) == FALSE)
        continue;

      /* get the child size request */
      gtk_widget_get_child_requisition (child->widget, &child_requisition);

      if (G_UNLIKELY (child->expand))
        {
          /* increase counter */
          n_expand_children++;

          /* update the size requested by the expanding children */
          if (itembar->horizontal)
            req_expandable_size += child_requisition.width;
          else
            req_expandable_size += child_requisition.height;
        }
      else
        {
          /* update the size avaible for allocation */
          if (itembar->horizontal)
            alloc_expandable_size -= child_requisition.width;
          else
            alloc_expandable_size -= child_requisition.height;
        }
    }

  /* set coordinates */
  x = allocation->x + border_width;
  y = allocation->y + border_width;

  /* check if the expandable childs fit in the available expandable size */
  expandable_children_fit = (req_expandable_size == alloc_expandable_size || req_expandable_size <= 0);

  /* make sure the allocated expandable size is not below zero */
  alloc_expandable_size = MAX (0, alloc_expandable_size);

  /* allocate the children */
  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* still skip hidden widgets */
      if (GTK_WIDGET_VISIBLE (child->widget) == FALSE)
        continue;

      /* get the child size request */
      gtk_widget_get_child_requisition (child->widget, &child_requisition);

      /* set coordinates for the child allocation */
      child_allocation.x = x;
      child_allocation.y = y;

      /* set the width or height of the child */
      if (G_UNLIKELY (child->expand && expandable_children_fit == FALSE))
        {
          /* get requested size */
          req_size = itembar->horizontal ? child_requisition.width : child_requisition.height;

          /* calculate allocated size */
          alloc_size = alloc_expandable_size * req_size / req_expandable_size;

          /* set the calculated expanding size */
          if (itembar->horizontal)
            child_allocation.width = alloc_size;
          else
            child_allocation.height = alloc_size;

          /* update total sizes */
          alloc_expandable_size -= alloc_size;
          req_expandable_size -= req_size;
        }
      else
        {
          /* set the requested size in the allocation */
          if (itembar->horizontal)
            child_allocation.width = child_requisition.width;
          else
            child_allocation.height = child_requisition.height;
        }

      /* update the coordinates and set the allocated (user defined) panel size */
      if (itembar->horizontal)
        {
          x += child_allocation.width;
          child_allocation.height = allocation->height;

          /* check if everything fits in the allocated size */
          if (G_UNLIKELY (x > allocation->width + allocation->x))
            {
              /* draw the next plugin offscreen */
              x = OFFSCREEN;

              /* make this plugin fit exactly on the panel */
              child_allocation.width = MAX (0, allocation->width + allocation->x - child_allocation.x);
            }
        }
      else
        {
          y += child_allocation.height;
          child_allocation.width = allocation->width;

          /* check if everything fits in the allocated size */
          if (G_UNLIKELY (y > allocation->height + allocation->y))
            {
              /* draw the next plugin offscreen */
              y = OFFSCREEN;

              /* make this plugin fit exactly on the panel */
              child_allocation.height = MAX (0, allocation->height + allocation->y - child_allocation.y);
            }
        }

      /* allocate the child */
      gtk_widget_size_allocate (child->widget, &child_allocation);
    }
}



static gboolean
panel_itembar_drag_motion (GtkWidget      *widget,
                           GdkDragContext *drag_context,
                           gint            drag_x,
                           gint            drag_y,
                           guint           time)
{
  PanelItembar  *itembar = PANEL_ITEMBAR (widget);
  GdkWindowAttr  attributes;
  gint           drop_index;
  GtkWidget     *child = NULL;
  gint           x = 0, y = 0;
  gint           width, height;

  if (G_UNLIKELY (itembar->highlight_window == NULL))
    {
      /* setup window attributes */
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.visual = gtk_widget_get_visual (widget);
      attributes.colormap = gtk_widget_get_colormap (widget);
      attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK;
      attributes.width = HIGHLIGHT_THINKNESS;
      attributes.height = HIGHLIGHT_THINKNESS;

      /* allocate window */
      itembar->highlight_window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes,
                                                  GDK_WA_VISUAL | GDK_WA_COLORMAP);

      /* set user data */
      gdk_window_set_user_data (itembar->highlight_window, widget);

      /* set window background */
      gdk_window_set_background (itembar->highlight_window, &widget->style->fg[widget->state]);
    }

  /* get the drop index */
  drop_index = panel_itembar_get_drop_index (itembar, drag_x, drag_y);

  /* get the nth child */
  child = panel_itembar_get_nth_child (itembar, drop_index);
  if (G_LIKELY (child))
    {
      /* get child start coordinate */
      if (itembar->horizontal)
        x = child->allocation.x;
      else
        y = child->allocation.y;
    }
  else if (itembar->children)
    {
      /* get the last child */
      child = panel_itembar_get_nth_child (itembar, g_slist_length (itembar->children) - 1);

      /* get coordinate at end of the child */
      if (itembar->horizontal)
        x = child->allocation.x + child->allocation.width;
      else
        y = child->allocation.y + child->allocation.height;
    }

  /* get size of the hightlight */
  width = itembar->horizontal ? HIGHLIGHT_THINKNESS : widget->allocation.width;
  height = itembar->horizontal ? widget->allocation.height : HIGHLIGHT_THINKNESS;

  /* show line between the two children */
  x += HIGHLIGHT_THINKNESS / 2;
  y += HIGHLIGHT_THINKNESS / 2;

  /* keep the heightlight window inside the itembar */
  x = CLAMP (x, widget->allocation.x, widget->allocation.x + widget->allocation.width - HIGHLIGHT_THINKNESS);
  y = CLAMP (y, widget->allocation.y, widget->allocation.y + widget->allocation.height - HIGHLIGHT_THINKNESS);

  /* move the hightlight window */
  gdk_window_move_resize (itembar->highlight_window, x, y, width, height);

  /* show the window */
  gdk_window_show (itembar->highlight_window);

  return TRUE;
}



static void
panel_itembar_drag_leave (GtkWidget      *widget,
                          GdkDragContext *drag_context,
                          guint           time)
{
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  /* destroy the drag highlight window */
  if (G_LIKELY (itembar->highlight_window))
    {
      gdk_window_set_user_data (itembar->highlight_window, NULL);
      gdk_window_destroy (itembar->highlight_window);
      itembar->highlight_window = NULL;
    }
}



static void
panel_itembar_add (GtkContainer *container,
                   GtkWidget    *child)
{
  /* append the item */
  panel_itembar_append (PANEL_ITEMBAR (container), child);
}



static void
panel_itembar_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (container);
  GSList            *li;
  PanelItembarChild *child;
  gboolean           was_visible;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_CONTAINER (container));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == GTK_WIDGET (itembar));
  panel_return_if_fail (itembar->children != NULL);

  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      if (child->widget == widget)
        {
          /* remove the child from the list */
          itembar->children = g_slist_delete_link (itembar->children, li);

          /* whether the widget is currently visible */
          was_visible = GTK_WIDGET_VISIBLE (widget);

          /* remove from the itembar */
          gtk_widget_unparent (child->widget);

          /* cleanup the slice */
          g_slice_free (PanelItembarChild, child);

          /* queue a resize if needed */
          if (G_LIKELY (was_visible))
            gtk_widget_queue_resize (GTK_WIDGET (container));

          /* done */
          break;
        }
    }
}



static void
panel_itembar_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (container);
  GSList            *children = itembar->children;
  PanelItembarChild *child;

  panel_return_if_fail (PANEL_IS_ITEMBAR (container));

  while (children != NULL)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}



static GType
panel_itembar_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}



GtkWidget *
panel_itembar_new (void)
{
  return g_object_new (PANEL_TYPE_ITEMBAR, NULL);
}



void
panel_itembar_set_sensitive (PanelItembar *itembar,
                             gboolean      sensitive)
{
  PanelItembarChild *child;
  GSList            *li;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (itembar->event_window == NULL || GDK_IS_WINDOW (itembar->event_window));

  /* set internal value */
  itembar->sensitive = !!sensitive;

  /* raise or lower the event window */
  if (G_LIKELY (itembar->event_window))
    {
      if (sensitive)
        gdk_window_lower (itembar->event_window);
      else
        gdk_window_raise (itembar->event_window);
    }

  /* walk the children */
  for (li = itembar->children; li != NULL; li = li->next)
    {
      /* get child */
      child = li->data;

      /* set widget sensitivity */
      gtk_widget_set_sensitive (child->widget, sensitive);
    }
}



void
panel_itembar_insert (PanelItembar *itembar,
                      GtkWidget    *widget,
                      gint          position)
{
  PanelItembarChild *child;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == NULL);

  /* allocate new child */
  child = g_slice_new (PanelItembarChild);

  /* set properties */
  child->widget = widget;
  child->expand = FALSE;

  /* insert in the internal list */
  itembar->children = g_slist_insert (itembar->children, child, position);

  /* set the parent */
  gtk_widget_set_parent (widget, GTK_WIDGET (itembar));

  /* sensitivity of the new item */
  gtk_widget_set_sensitive (widget, itembar->sensitive);

  /* resize the itembar */
  gtk_widget_queue_resize (GTK_WIDGET (itembar));
}



void
panel_itembar_reorder_child (PanelItembar *itembar,
                             GtkWidget    *widget,
                             gint          position)
{
  GSList            *li;
  PanelItembarChild *child;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == GTK_WIDGET (itembar));

  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* find the widget */
      if (child->widget == widget)
        {
          /* remove the link from the list */
          itembar->children = g_slist_delete_link (itembar->children, li);

          /* insert the child in the new position */
          itembar->children = g_slist_insert (itembar->children, child, position);

          /* reallocate the itembar */
          gtk_widget_queue_resize (GTK_WIDGET (itembar));

          /* we're done */
          break;
        }
    }
}



gboolean
panel_itembar_get_child_expand (PanelItembar *itembar,
                                GtkWidget    *widget)
{
  GSList            *li;
  PanelItembarChild *child;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), FALSE);
  panel_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  panel_return_val_if_fail (widget->parent == GTK_WIDGET (itembar), FALSE);

  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* find the widget and return the expand mode */
      if (child->widget == widget)
        return child->expand;
    }

  return FALSE;
}



void
panel_itembar_set_child_expand (PanelItembar *itembar,
                                GtkWidget    *widget,
                                gboolean      expand)
{
  GSList            *li;
  PanelItembarChild *child;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == GTK_WIDGET (itembar));

  /* find child and set new expand mode */
  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* find the widget */
      if (child->widget == widget)
        {
          /* only update if the expand mode changed */
          if (G_LIKELY (child->expand != expand))
            {
              /* store new mode */
              child->expand = expand;

              /* resize the itembar */
              gtk_widget_queue_resize (GTK_WIDGET (itembar));
            }

          /* stop searching */
          break;
        }
    }
}



gint
panel_itembar_get_child_index (PanelItembar *itembar,
                               GtkWidget    *widget)
{
  GSList            *li;
  PanelItembarChild *child;
  gint               idx;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), -1);
  panel_return_val_if_fail (GTK_IS_WIDGET (widget), -1);
  panel_return_val_if_fail (widget->parent == GTK_WIDGET (itembar), -1);

  /* walk the children to find the child widget */
  for (idx = 0, li = itembar->children; li != NULL; li = li->next, idx++)
    {
      child = li->data;

      /* check if this is the widget */
      if (child->widget == widget)
        return idx;
    }

  /* nothing found */
  return -1;
}



GtkWidget *
panel_itembar_get_nth_child (PanelItembar *itembar,
                             guint         idx)
{
  PanelItembarChild *child;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), NULL);

  /* get the nth item */
  child = g_slist_nth_data (itembar->children, idx);

  /* return the widget */
  return (child != NULL ? child->widget : NULL);
}



guint           
panel_itembar_get_n_children (PanelItembar *itembar)
{
  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), 0);
  
  return g_slist_length (itembar->children);
}



guint
panel_itembar_get_drop_index (PanelItembar  *itembar,
                              gint           x,
                              gint           y)
{
  PanelItembarChild *child;
  GSList            *li;
  GtkAllocation     *allocation;
  guint              idx;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), 0);

  /* add the itembar position */
  x += GTK_WIDGET (itembar)->allocation.x;
  y += GTK_WIDGET (itembar)->allocation.y;

  for (li = itembar->children, idx = 0; li != NULL; li = li->next, idx++)
    {
      child = li->data;

      /* get the child allocation */
      allocation = &(child->widget->allocation);

      /* check if the drop index is before the half of the allocation */
      if ((itembar->horizontal && x < (allocation->x + allocation->width / 2))
          || (!itembar->horizontal && y < (allocation->y + allocation->height / 2)))
        break;
    }

  return idx;
}



GtkWidget *
panel_itembar_get_child_at_position (PanelItembar *itembar,
                                     gint          x,
                                     gint          y)
{
  PanelItembarChild *child;
  GSList            *li;
  GtkAllocation     *allocation;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), NULL);

  for (li = itembar->children; li != NULL; li = li->next)
    {
      child = li->data;

      /* get the child allocation */
      allocation = &(child->widget->allocation);

      /* check if the coordinate is inside the allocation */
      if (x >= allocation->x && x <= (allocation->x + allocation->width)
          && y >= allocation->y && y <= (allocation->y + allocation->height))
        return child->widget;
    }

  return NULL;
}
