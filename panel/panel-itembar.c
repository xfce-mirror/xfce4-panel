/*
 * Copyright (C) 2008-2009 Nick Schermer <nick@xfce.org>
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

#include <common/panel-private.h>
#include <libxfce4panel/libxfce4panel.h>

#include <panel/panel-itembar.h>



typedef struct _PanelItembarChild PanelItembarChild;



static void               panel_itembar_set_property       (GObject         *object,
                                                            guint            prop_id,
                                                            const GValue    *value,
                                                            GParamSpec      *pspec);
static void               panel_itembar_get_property       (GObject         *object,
                                                            guint            prop_id,
                                                            GValue          *value,
                                                            GParamSpec      *pspec);
static void               panel_itembar_finalize           (GObject         *object);
static void               panel_itembar_size_request       (GtkWidget       *widget,
                                                            GtkRequisition  *requisition);
static void               panel_itembar_size_allocate      (GtkWidget       *widget,
                                                            GtkAllocation   *allocation);
static gboolean           panel_itembar_expose_event       (GtkWidget       *widget,
                                                            GdkEventExpose  *event);
static void               panel_itembar_add                (GtkContainer    *container,
                                                            GtkWidget       *child);
static void               panel_itembar_remove             (GtkContainer    *container,
                                                            GtkWidget       *child);
static void               panel_itembar_forall             (GtkContainer    *container,
                                                            gboolean         include_internals,
                                                            GtkCallback      callback,
                                                            gpointer         callback_data);
static GType              panel_itembar_child_type         (GtkContainer    *container);
static void               panel_itembar_set_child_property (GtkContainer    *container,
                                                            GtkWidget       *widget,
                                                            guint            prop_id,
                                                            const GValue    *value,
                                                            GParamSpec      *pspec);
static void               panel_itembar_get_child_property (GtkContainer    *container,
                                                            GtkWidget       *widget,
                                                            guint            prop_id,
                                                            GValue          *value,
                                                            GParamSpec      *pspec);
static PanelItembarChild *panel_itembar_get_child          (PanelItembar    *itembar,
                                                            GtkWidget       *widget);



struct _PanelItembarClass
{
  GtkContainerClass __parent__;
};

struct _PanelItembar
{
  GtkContainer __parent__;

  GSList         *children;

  /* some properties we clone from the panel window */
  guint           horizontal : 1;
  guint           size;

  /* dnd support */
  gint            highlight_index;
  gint            highlight_x, highlight_y;
};

struct _PanelItembarChild
{
  GtkWidget *widget;
  guint      expand : 1;
  guint      wrap : 1;
};

enum
{
  PROP_0,
  PROP_HORIZONTAL,
  PROP_SIZE
};

enum
{
  CHILD_PROP_0,
  CHILD_PROP_EXPAND,
  CHILD_PROP_WRAP
};

enum
{
  CHANGED,
  LAST_SIGNAL
};



static guint itembar_signals[LAST_SIGNAL];



G_DEFINE_TYPE (PanelItembar, panel_itembar, GTK_TYPE_CONTAINER)



static void
panel_itembar_class_init (PanelItembarClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->set_property = panel_itembar_set_property;
  gobject_class->get_property = panel_itembar_get_property;
  gobject_class->finalize = panel_itembar_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request = panel_itembar_size_request;
  gtkwidget_class->size_allocate = panel_itembar_size_allocate;
  gtkwidget_class->expose_event = panel_itembar_expose_event;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = panel_itembar_add;
  gtkcontainer_class->remove = panel_itembar_remove;
  gtkcontainer_class->forall = panel_itembar_forall;
  gtkcontainer_class->child_type = panel_itembar_child_type;
  gtkcontainer_class->get_child_property = panel_itembar_get_child_property;
  gtkcontainer_class->set_child_property = panel_itembar_set_child_property;

  itembar_signals[CHANGED] =
    g_signal_new (g_intern_static_string ("changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (gobject_class,
                                   PROP_HORIZONTAL,
                                   g_param_spec_boolean ("horizontal",
                                                         NULL, NULL,
                                                         TRUE,
                                                         EXO_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_uint ("size",
                                                      NULL, NULL,
                                                      16, 128, 48,
                                                      EXO_PARAM_WRITABLE));

  gtk_container_class_install_child_property (gtkcontainer_class,
                                              CHILD_PROP_EXPAND,
                                              g_param_spec_boolean ("expand",
                                                                    NULL, NULL,
                                                                    FALSE,
                                                                    EXO_PARAM_READWRITE));

  gtk_container_class_install_child_property (gtkcontainer_class,
                                              CHILD_PROP_WRAP,
                                              g_param_spec_boolean ("wrap",
                                                                    NULL, NULL,
                                                                    FALSE,
                                                                    EXO_PARAM_READWRITE));
}



static void
panel_itembar_init (PanelItembar *itembar)
{
  itembar->children = NULL;
  itembar->horizontal = TRUE;
  itembar->size = 30;
  itembar->highlight_index = -1;

  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (itembar), GTK_NO_WINDOW);

  gtk_widget_set_redraw_on_allocate (GTK_WIDGET (itembar), FALSE);
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
      break;

    case PROP_SIZE:
      itembar->size = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

  gtk_widget_queue_resize (GTK_WIDGET (itembar));
}



static void
panel_itembar_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (prop_id)
    {
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
panel_itembar_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (widget);
  GSList            *li;
  PanelItembarChild *child;
  GtkRequisition     child_requisition;
  gint               border_width;
  gint               row_length = 0;

  /* intialize the requisition, we always set the panel height */
  if (itembar->horizontal)
    {
      requisition->height = itembar->size;
      requisition->width = 0;
    }
  else
    {
      requisition->height = 0;
      requisition->width = itembar->size;
    }

  /* get the size requests of the children and use the longest row for
   * the requested width/height when we have wrap items */
  for (li = itembar->children; li != NULL; li = g_slist_next (li))
    {
      child = li->data;
      if (G_LIKELY (child != NULL))
        {
          if (!GTK_WIDGET_VISIBLE (child->widget))
            continue;

          gtk_widget_size_request (child->widget, &child_requisition);

          if (G_LIKELY (!child->wrap))
            {
              if (itembar->horizontal)
                row_length += child_requisition.width;
              else
                row_length += child_requisition.height;
            }
          else
            {
              /* add to size for new wrap element */
              if (itembar->horizontal)
                {
                  requisition->height += itembar->size;
                  requisition->width = MAX (requisition->width, row_length);
                }
              else
                {
                  requisition->width += itembar->size;
                  requisition->height = MAX (requisition->height, row_length);
                }

              /* reset length for new row */
              row_length = 0;
            }
        }
      else
        {
          /* this noop item is the dnd position */
          row_length += itembar->size;
        }
    }

  /* also take the last row_length into account */
  if (itembar->horizontal)
    requisition->width = MAX (requisition->width, row_length);
  else
    requisition->height = MAX (requisition->height, row_length);

  /* add border width */
  border_width = GTK_CONTAINER (widget)->border_width * 2;
  requisition->height += border_width;
  requisition->width += border_width;
}



static void
panel_itembar_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
  PanelItembar      *itembar = PANEL_ITEMBAR (widget);
  GSList            *li, *lp;
  PanelItembarChild *child;
  GtkRequisition     child_req;
  GtkAllocation      child_alloc;
  guint              n_expand_children;
  guint              row;
  gint               expand_length, border_width;
  gint               expand_length_avail;
  gint               expand_length_req;
  gint               length_req, length;
  gint               alloc_length;
  gint               x, y;
  gboolean           expand_children_fit;

  widget->allocation = *allocation;

  border_width = GTK_CONTAINER (widget)->border_width;

  if (itembar->horizontal)
    expand_length = allocation->width - 2 * border_width;
  else
    expand_length = allocation->height - 2 * border_width;

  /* loop for wrap items */
  for (row = 0, li = itembar->children; li != NULL; li = g_slist_next (li), row++)
    {
      expand_length_avail = expand_length;
      expand_length_req = 0;
      n_expand_children = 0;

      /* get information about the expandable lengths */
      for (lp = li; lp != NULL; lp = g_slist_next (lp))
        {
          child = lp->data;
          if (G_LIKELY (child != NULL))
            {
              if (!GTK_WIDGET_VISIBLE (child->widget))
                continue;

              /* continue allocating until we hit a wrap child */
              if (G_UNLIKELY (child->wrap))
                break;

              gtk_widget_get_child_requisition (child->widget, &child_req);
              length = itembar->horizontal ? child_req.width : child_req.height;

              if (G_UNLIKELY (child->expand))
                {
                  n_expand_children++;
                  expand_length_req += length;
                }
              else
                {
                  expand_length_avail -= length;
                }
            }
          else
            {
              expand_length_avail -= itembar->size;
            }
        }

      /* set start coordinates for the items in the row*/
      x = allocation->x + border_width;
      y = allocation->y + border_width;
      if (itembar->horizontal)
        y += row * itembar->size;
      else
        x += row * itembar->size;

      /* whether the expandable items fit on this row; we use this
       * as a fast-path when there are expanding items on a panel with
       * not really enough length to expand (ie. items make the panel grow,
       * not the length set by the user) */
      expand_children_fit = expand_length_req == expand_length_avail;
      if (expand_length_avail < 0)
        expand_length_avail = 0;

      /* allocate the children on this row */
      for (; li != NULL; li = g_slist_next (li))
        {
          child = li->data;

          /* the highlight item for which we keep some spare space */
          if (G_UNLIKELY (child == NULL))
            {
              itembar->highlight_x = x;
              itembar->highlight_y = y;
              expand_length_avail -= itembar->size;

              if (itembar->horizontal)
                x += itembar->size;
              else
                y += itembar->size;

              continue;
            }

          if (!GTK_WIDGET_VISIBLE (child->widget))
            continue;

          gtk_widget_get_child_requisition (child->widget, &child_req);

          child_alloc.x = x;
          child_alloc.y = y;

          if (child->wrap)
            {
              /* if there is any expanding length available use it for the
               * wrapping plugin to improve accessibility */
              if (expand_length_avail > 0)
                {
                  if (itembar->horizontal)
                    {
                      child_alloc.height = itembar->size;
                      child_alloc.width = expand_length_avail;
                    }
                  else
                    {
                      child_alloc.width = itembar->size;
                      child_alloc.height = expand_length_avail;
                    }
                }
              else
                {
                  /* hide it */
                  child_alloc.width = child_alloc.height = 0;
                }

              gtk_widget_size_allocate (child->widget, &child_alloc);

              /* stop and continue to the next row */
              break;
            }

          if (G_UNLIKELY (!expand_children_fit && child->expand))
            {
              /* try to equally distribute the length between the expanding plugins */
              length_req = itembar->horizontal ? child_req.width : child_req.height;
              panel_assert (n_expand_children > 0);
              if (G_LIKELY (expand_length_req > 0 || length_req > 0))
                alloc_length = expand_length_avail * length_req / expand_length_req;
              else
                alloc_length = expand_length_avail / n_expand_children;

              if (itembar->horizontal)
                child_alloc.width = alloc_length;
              else
                child_alloc.height = alloc_length;

              n_expand_children--;
              expand_length_req -= length_req;
              expand_length_avail -= alloc_length;
            }
          else
            {
              if (itembar->horizontal)
                child_alloc.width = child_req.width;
              else
                child_alloc.height = child_req.height;
            }

          if (itembar->horizontal)
            {
              child_alloc.height = itembar->size;
              x += child_alloc.width;
            }
          else
            {
              child_alloc.width = itembar->size;
              y += child_alloc.height;
            }

          gtk_widget_size_allocate (child->widget, &child_alloc);
        }
    }
}



static gboolean
panel_itembar_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
  gboolean      result;
  PanelItembar *itembar = PANEL_ITEMBAR (widget);

  result = (*GTK_WIDGET_CLASS (panel_itembar_parent_class)->expose_event) (widget, event);

  if (itembar->highlight_index != -1)
    {
      gtk_paint_box (widget->style, widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                     &event->area, widget, "panel-dnd",
                     itembar->highlight_x + 1,
                     itembar->highlight_y + 1,
                     itembar->size - 2,
                     itembar->size - 2);
    }

  return result;
}



static void
panel_itembar_add (GtkContainer *container,
                   GtkWidget    *child)
{
  panel_itembar_insert (PANEL_ITEMBAR (container), child, -1);
}



static void
panel_itembar_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  PanelItembarChild *child;
  PanelItembar      *itembar = PANEL_ITEMBAR (container);

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == GTK_WIDGET (container));
  panel_return_if_fail (itembar->children != NULL);

  child = panel_itembar_get_child (itembar, widget);
  if (G_LIKELY (child != NULL))
    {
      itembar->children = g_slist_remove (itembar->children, child);

      gtk_widget_unparent (widget);

      g_slice_free (PanelItembarChild, child);

      gtk_widget_queue_resize (GTK_WIDGET (container));

      g_signal_emit (G_OBJECT (itembar), itembar_signals[CHANGED], 0);
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
      children = g_slist_next (children);

      if (G_LIKELY (child != NULL))
        (* callback) (child->widget, callback_data);
    }
}



static GType
panel_itembar_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}



static void
panel_itembar_set_child_property (GtkContainer *container,
                                  GtkWidget    *widget,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PanelItembarChild *child;
  gboolean           boolean;

  child = panel_itembar_get_child (PANEL_ITEMBAR (container), widget);
  if (G_UNLIKELY (child == NULL))
    return;

  switch (prop_id)
    {
    case CHILD_PROP_EXPAND:
      boolean = g_value_get_boolean (value);
      if (child->expand == boolean)
        return;
      child->expand = boolean;
      break;

    case CHILD_PROP_WRAP:
      boolean = g_value_get_boolean (value);
      if (child->wrap == boolean)
        return;
      child->wrap = boolean;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (container, prop_id, pspec);
      break;
    }

  gtk_widget_queue_resize (GTK_WIDGET (container));
}



static void
panel_itembar_get_child_property (GtkContainer *container,
                                  GtkWidget    *widget,
                                  guint         prop_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  PanelItembarChild *child;

  child = panel_itembar_get_child (PANEL_ITEMBAR (container), widget);
  if (G_UNLIKELY (child == NULL))
    return;

  switch (prop_id)
    {
    case CHILD_PROP_EXPAND:
      g_value_set_boolean (value, child->expand);
      break;

    case CHILD_PROP_WRAP:
      g_value_set_boolean (value, child->wrap);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (container, prop_id, pspec);
      break;
    }
}



static inline PanelItembarChild *
panel_itembar_get_child (PanelItembar *itembar,
                         GtkWidget    *widget)
{
  GSList            *li;
  PanelItembarChild *child;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), NULL);
  panel_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  panel_return_val_if_fail (widget->parent == GTK_WIDGET (itembar), NULL);

  for (li = itembar->children; li != NULL; li = g_slist_next (li))
    {
      child = li->data;
      if (child != NULL
          && child->widget == widget)
        return child;
    }

  return NULL;
}



GtkWidget *
panel_itembar_new (void)
{
  return g_object_new (PANEL_TYPE_ITEMBAR, NULL);
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

  child = g_slice_new0 (PanelItembarChild);
  child->widget = widget;
  child->expand = FALSE;
  child->wrap = FALSE;

  itembar->children = g_slist_insert (itembar->children, child, position);
  gtk_widget_set_parent (widget, GTK_WIDGET (itembar));

  gtk_widget_queue_resize (GTK_WIDGET (itembar));
  g_signal_emit (G_OBJECT (itembar), itembar_signals[CHANGED], 0);
}



void
panel_itembar_reorder_child (PanelItembar *itembar,
                             GtkWidget    *widget,
                             gint          position)
{
  PanelItembarChild *child;

  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));
  panel_return_if_fail (GTK_IS_WIDGET (widget));
  panel_return_if_fail (widget->parent == GTK_WIDGET (itembar));

  child = panel_itembar_get_child (itembar, widget);
  if (G_LIKELY (child != NULL))
    {
      /* move in the internal list */
      itembar->children = g_slist_remove (itembar->children, child);
      itembar->children = g_slist_insert (itembar->children, child, position);

      gtk_widget_queue_resize (GTK_WIDGET (itembar));
      g_signal_emit (G_OBJECT (itembar), itembar_signals[CHANGED], 0);
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

  for (idx = 0, li = itembar->children; li != NULL; li = g_slist_next (li), idx++)
    {
      child = li->data;
      if (G_UNLIKELY (child == NULL))
        continue;

      if (child->widget == widget)
        return idx;
    }

  return -1;
}



guint
panel_itembar_get_n_children (PanelItembar *itembar)
{
  guint n;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), 0);

  n = g_slist_length (itembar->children);
  if (G_UNLIKELY (itembar->highlight_index != -1))
    n--;

  return n;
}



guint
panel_itembar_get_drop_index (PanelItembar *itembar,
                              gint          x,
                              gint          y)
{
  PanelItembarChild *child;
  GSList            *li;
  GtkAllocation     *alloc;
  guint              idx;
  gint               row = 0;

  panel_return_val_if_fail (PANEL_IS_ITEMBAR (itembar), 0);

  /* add the itembar position */
  alloc = &GTK_WIDGET (itembar)->allocation;
  x += alloc->x;
  y += alloc->y;

  /* return -1 if point is outside the widget allocation */
  if (x > alloc->width || y > alloc->height)
    return -1;

  for (li = itembar->children, idx = 0; li != NULL; li = g_slist_next (li))
    {
      child = li->data;
      if (G_UNLIKELY (child == NULL))
        continue;

      if (child->wrap)
        {
          row += itembar->size;

          /* always make sure the item is on the row */
          if ((itembar->horizontal && y < row)
              || (!itembar->horizontal && x < row))
            break;
        }

      alloc = &child->widget->allocation;

      if (itembar->horizontal)
        {
          if (x < (alloc->x + (alloc->width / 2))
              && y >= alloc->y
              && y <= alloc->y + alloc->height)
            break;
        }
      else
        {
          if (y < (alloc->y + (alloc->height / 2))
              && x >= alloc->x
              && x <= alloc->x + alloc->width)
            break;
        }

      idx++;
    }

  return idx;
}



void
panel_itembar_set_drop_highlight_item (PanelItembar *itembar,
                                       gint          idx)
{
  panel_return_if_fail (PANEL_IS_ITEMBAR (itembar));

  if (idx == itembar->highlight_index)
    return;

  if (itembar->highlight_index != -1)
    itembar->children = g_slist_remove_all (itembar->children, NULL);
  if (idx != -1)
    itembar->children = g_slist_insert (itembar->children, NULL, idx);

  itembar->highlight_index = idx;

  gtk_widget_queue_resize (GTK_WIDGET (itembar));
}
