/*
 * Copyright (C) 2007-2010 Nick Schermer <nick@xfce.org>
 *
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <exo/exo.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <common/panel-private.h>
#include <common/panel-xfconf.h>

#include "systray-box.h"
#include "systray-socket.h"

#define SPACING            (2)
#define OFFSCREEN          (-9999)



static void     systray_box_get_property          (GObject         *object,
                                                   guint            prop_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
/*
static void     systray_box_set_property          (GObject         *object,
                                                   guint            prop_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
*/
static void     systray_box_finalize              (GObject         *object);
static void     systray_box_size_request          (GtkWidget       *widget,
                                                   GtkRequisition  *requisition);
static void     systray_box_size_allocate         (GtkWidget       *widget,
                                                   GtkAllocation   *allocation);
static gboolean systray_box_expose_event          (GtkWidget       *widget,
                                                   GdkEventExpose  *event);
static void     systray_box_add                   (GtkContainer    *container,
                                                   GtkWidget       *child);
static void     systray_box_remove                (GtkContainer    *container,
                                                   GtkWidget       *child);
static void     systray_box_forall                (GtkContainer    *container,
                                                   gboolean         include_internals,
                                                   GtkCallback      callback,
                                                   gpointer         callback_data);
static GType    systray_box_child_type            (GtkContainer    *container);
static gint     systray_box_compare_function      (gconstpointer    a,
                                                   gconstpointer    b);



enum
{
  PROP_0,
  PROP_HAS_HIDDEN
};

struct _SystrayBoxClass
{
  GtkContainerClass __parent__;
};

struct _SystrayBox
{
  GtkContainer  __parent__;

  /* all the icons packed in this box */
  GSList       *childeren;

  /* orientation of the box */
  guint         horizontal : 1;

  /* hidden childeren counter */
  gint          n_hidden_childeren;

  /* whether hidden icons are visible */
  guint         show_hidden : 1;

  /* number of rows */
  gint          rows;

  /* guess size, this is a value used to reduce the tray flickering */
  gint          guess_size;
};



XFCE_PANEL_DEFINE_TYPE (SystrayBox, systray_box, GTK_TYPE_CONTAINER)



static void
systray_box_class_init (SystrayBoxClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->get_property = systray_box_get_property;
  /*gobject_class->set_property = systray_box_set_property;*/
  gobject_class->finalize = systray_box_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request = systray_box_size_request;
  gtkwidget_class->size_allocate = systray_box_size_allocate;
  gtkwidget_class->expose_event = systray_box_expose_event;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->add = systray_box_add;
  gtkcontainer_class->remove = systray_box_remove;
  gtkcontainer_class->forall = systray_box_forall;
  gtkcontainer_class->child_type = systray_box_child_type;

  g_object_class_install_property (gobject_class,
                                   PROP_HAS_HIDDEN,
                                   g_param_spec_boolean ("has-hidden",
                                                         NULL, NULL,
                                                         FALSE,
                                                         EXO_PARAM_READABLE));
}



static void
systray_box_init (SystrayBox *box)
{
  GTK_WIDGET_SET_FLAGS (box, GTK_NO_WINDOW);

  box->childeren = NULL;
  box->rows = 1;
  box->n_hidden_childeren = 0;
  box->horizontal = TRUE;
  box->show_hidden = FALSE;
  box->guess_size = 128;
}



static void
systray_box_get_property (GObject      *object,
                          guint         prop_id,
                          GValue       *value,
                          GParamSpec   *pspec)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (object);

  switch (prop_id)
    {
    case PROP_HAS_HIDDEN:
      g_value_set_boolean (value, box->n_hidden_childeren > 0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



/*
static void
systray_box_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
*/



static void
systray_box_finalize (GObject *object)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (object);

  /* check if we're leaking */
  if (G_UNLIKELY (box->childeren != NULL))
    {
      /* free the child list */
      g_slist_free (box->childeren);
      g_debug ("Not all icons has been removed from the systray.");
    }

  G_OBJECT_CLASS (systray_box_parent_class)->finalize (object);
}



static void
systray_box_size_request (GtkWidget      *widget,
                          GtkRequisition *requisition)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (widget);
  GSList          *li;
  GtkWidget       *child;
  gint             n_columns;
  gint             child_size = -1;
  GtkRequisition   child_req;
  gint             n_visible_childeren = 0;
  gint             swap;
  gint             guess_size, icon_size;
  gint             n_hidden_childeren = 0;
  gboolean         hidden;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (widget));
  panel_return_if_fail (requisition != NULL);

  /* get the guess size */
  guess_size = box->guess_size - (SPACING * (box->rows - 1));
  guess_size /= box->rows;

  /* check if we need to hide or show any childeren */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child = GTK_WIDGET (li->data);

      /* get the icons size request */
      gtk_widget_size_request (child, &child_req);

      /* a 1x1px request is in some tray implementations the same as
       * an invisible icon, we allocate those offscreen in allocate */
      if (child_req.width <= 1 && child_req.height <= 1)
        continue;

      hidden = systray_socket_get_hidden (XFCE_SYSTRAY_SOCKET (child));

      if (!hidden || box->show_hidden)
        {
          /* get the icon size */
          icon_size = MIN (guess_size, MAX (child_req.width, child_req.height));

          /* pick largest icon */
          if (G_UNLIKELY (child_size == -1))
            child_size = icon_size;
          else
            child_size = MAX (child_size, icon_size);

          /* increase number of visible childeren */
          n_visible_childeren++;
        }

      if (hidden)
        n_hidden_childeren++;
    }

  /* update the visibility of the arrow button */
  if (box->n_hidden_childeren != n_hidden_childeren)
    {
      box->n_hidden_childeren = n_hidden_childeren;
      g_object_notify (G_OBJECT (box), "has-hidden");
    }

  /* number of columns */
  n_columns = n_visible_childeren / box->rows;
  if (n_visible_childeren > (n_columns * box->rows))
    n_columns++;

  /* set the width and height needed for the icons */
  if (n_visible_childeren > 0)
    {
      requisition->width = ((child_size + SPACING) * n_columns) - SPACING;
      requisition->height = ((child_size + SPACING) * box->rows) - SPACING;
    }
  else
    {
      requisition->width = requisition->height = 0;
    }

  /* swap the sizes if the orientation is vertical */
  if (!box->horizontal)
    {
      swap = requisition->width;
      requisition->width = requisition->height;
      requisition->height = swap;
    }

  /* add container border */
  requisition->width += GTK_CONTAINER (widget)->border_width * 2;
  requisition->height += GTK_CONTAINER (widget)->border_width * 2;
}



static void
systray_box_size_allocate (GtkWidget     *widget,
                           GtkAllocation *allocation)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (widget);
  GtkWidget       *child;
  GSList          *li;
  gint             n;
  gint             x, y;
  gint             width, height;
  gint             offset = 0;
  gint             child_size;
  GtkAllocation    child_alloc;
  GtkRequisition   child_req;
  gint             swap;
  gint             n_children;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (widget));
  panel_return_if_fail (allocation != NULL);

  /* set widget allocation */
  widget->allocation = *allocation;

  n_children = g_slist_length (box->childeren);
  if (n_children == 0)
    return;

  /* get root coordinates */
  x = allocation->x + GTK_CONTAINER (widget)->border_width;
  y = allocation->y + GTK_CONTAINER (widget)->border_width;

  /* get real size */
  width = allocation->width - 2 * GTK_CONTAINER (widget)->border_width;
  height = allocation->height - 2 * GTK_CONTAINER (widget)->border_width;

  /* child size */
  if (box->rows == 1)
    {
      child_size = box->horizontal ? width : height;
      n = n_children - (box->show_hidden ? 0 : box->n_hidden_childeren);
      child_size -= SPACING * MAX (n - 1, 0);
      if (n > 1)
        child_size /= n;

      if (box->horizontal)
        y += MAX (height - child_size, 0) / 2;
      else
        x += MAX (width - child_size, 0) / 2;
    }
  else
    {
      child_size = box->horizontal ? height : width;
      child_size -= SPACING * (box->rows - 1);
      child_size /= box->rows;
    }

  /* don't allocate zero width icon */
  if (child_size < 1)
    child_size = 1;

  /* position icons */
  for (li = box->childeren, n = 0; li != NULL; li = li->next)
    {
      child = GTK_WIDGET (li->data);

      gtk_widget_get_child_requisition (child, &child_req);
      if ((child_req.width == 1 && child_req.height == 1)
          || (systray_socket_get_hidden (XFCE_SYSTRAY_SOCKET (child)) && !box->show_hidden))
        {
          /* put icons offscreen */
          child_alloc.x = child_alloc.y = OFFSCREEN;
        }
      else
        {
          /* set coordinates */
          child_alloc.x = (child_size + SPACING) * (n / box->rows) + offset;
          child_alloc.y = (child_size + SPACING) * (n % box->rows);

          /* increase item counter */
          n++;

          /* swap coordinates on a vertical panel */
          if (!box->horizontal)
            {
              swap = child_alloc.x;
              child_alloc.x = child_alloc.y;
              child_alloc.y = swap;
            }

          /* add root */
          child_alloc.x += x;
          child_alloc.y += y;
        }

      /* set child width and height */
      child_alloc.width = child_size;
      child_alloc.height = child_size;

      /* allocate widget size */
      gtk_widget_size_allocate (child, &child_alloc);
    }
}



static gboolean
systray_box_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  SystrayBox    *box = XFCE_SYSTRAY_BOX (widget);
  cairo_t       *cr;
  GtkWidget     *child;
  GSList        *li;
  gboolean       result;
  GtkAllocation *child_alloc;

  result = GTK_WIDGET_CLASS (systray_box_parent_class)->expose_event (widget, event);

  if (gtk_widget_is_composited (widget))
    {
      cr = gdk_cairo_create (widget->window);
      gdk_cairo_region (cr, event->region);
      cairo_clip (cr);

      for (li = box->childeren; li != NULL; li = li->next)
        {
          child = GTK_WIDGET (li->data);
          child_alloc = &child->allocation;

          /* skip invisible (offscreen) or not composited children */
          if (child_alloc->x < 0
              || child_alloc->y < 0
              || !systray_socket_is_composited (XFCE_SYSTRAY_SOCKET (child)))
            continue;

          /* paint the child */
          gdk_cairo_set_source_pixmap (cr, gtk_widget_get_window (child),
                                       child_alloc->x, child_alloc->y);
          cairo_paint (cr);
        }

      cairo_destroy (cr);
    }

  return result;
}



static void
systray_box_add (GtkContainer *container,
                 GtkWidget    *child)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (container);

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));
  panel_return_if_fail (GTK_IS_WIDGET (child));
  panel_return_if_fail (child->parent == NULL);

  box->childeren = g_slist_insert_sorted (box->childeren, child,
                                          systray_box_compare_function);

  gtk_widget_set_parent (child, GTK_WIDGET (box));

  gtk_widget_queue_resize (GTK_WIDGET (container));
}



static void
systray_box_remove (GtkContainer *container,
                    GtkWidget    *child)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (container);
  GSList     *li;

  /* search the child */
  li = g_slist_find (box->childeren, child);
  if (G_LIKELY (li != NULL))
    {
      panel_assert (GTK_WIDGET (li->data) == child);

      /* unparent widget */
      box->childeren = g_slist_remove_link (box->childeren, li);
      gtk_widget_unparent (child);

      /* resize, so we update has-hidden */
      gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}



static void
systray_box_forall (GtkContainer *container,
                    gboolean      include_internals,
                    GtkCallback   callback,
                    gpointer      callback_data)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (container);
  GSList     *li, *lnext;

  /* run callback for all childeren */
  for (li = box->childeren; li != NULL; li = lnext)
    {
      lnext = li->next;
      (*callback) (GTK_WIDGET (li->data), callback_data);
    }
}



static GType
systray_box_child_type (GtkContainer *container)

{
  return GTK_TYPE_WIDGET;
}



static gint
systray_box_compare_function (gconstpointer a,
                              gconstpointer b)
{
  const gchar *name_a, *name_b;
  gboolean     hidden_a, hidden_b;

  /* sort hidden icons before visible ones */
  hidden_a = systray_socket_get_hidden (XFCE_SYSTRAY_SOCKET (a));
  hidden_b = systray_socket_get_hidden (XFCE_SYSTRAY_SOCKET (b));
  if (hidden_a != hidden_b)
    return hidden_a ? -1 : 1;

  /* sort icons by name */
  name_a = systray_socket_get_name (XFCE_SYSTRAY_SOCKET (a));
  name_b = systray_socket_get_name (XFCE_SYSTRAY_SOCKET (b));

#if GLIB_CHECK_VERSION (2, 16, 0)
  return g_strcmp0 (name_a, name_b);
#else
  if (name_a == NULL)
    return -(name_a != name_b);
  if (name_b == NULL)
    return name_a != name_b;

  return strcmp (name_a, name_b);
#endif
}



GtkWidget *
systray_box_new (void)
{
  return g_object_new (XFCE_TYPE_SYSTRAY_BOX, NULL);
}



void
systray_box_set_guess_size (SystrayBox *box,
                            gint        guess_size)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  /* set the systray guess size */
  box->guess_size = guess_size;
}



void
systray_box_set_orientation (SystrayBox     *box,
                             GtkOrientation  orientation)
{
  gboolean horizontal;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  horizontal = !!(orientation == GTK_ORIENTATION_HORIZONTAL);
  if (G_LIKELY (box->horizontal != horizontal))
    {
      box->horizontal = horizontal;

      if (box->childeren != NULL)
        gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}



void
systray_box_set_rows (SystrayBox *box,
                      gint        rows)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  if (G_LIKELY (rows != box->rows))
    {
      /* set new setting */
      box->rows = MAX (1, rows);

      /* queue a resize */
      if (box->childeren != NULL)
        gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}



gint
systray_box_get_rows (SystrayBox *box)
{
  panel_return_val_if_fail (XFCE_IS_SYSTRAY_BOX (box), 1);

  return box->rows;
}



void
systray_box_set_show_hidden (SystrayBox *box,
                              gboolean   show_hidden)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  if (box->show_hidden != show_hidden)
    {
      box->show_hidden = show_hidden;
      gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}



gboolean
systray_box_get_show_hidden (SystrayBox *box)
{
  panel_return_val_if_fail (XFCE_IS_SYSTRAY_BOX (box), FALSE);

  return box->show_hidden;
}



void
systray_box_update (SystrayBox *box)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  box->childeren = g_slist_sort (box->childeren,
                                 systray_box_compare_function);

  /* update the box, so we update the has-hidden property */
  gtk_widget_queue_resize (GTK_WIDGET (box));
}
