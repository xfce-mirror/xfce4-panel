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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <common/panel-private.h>

#include "systray-box.h"
#include "systray-socket.h"

#define BUTTON_SIZE        (16)
#define SPACING            (2)
#define OFFSCREEN          (-9999)
#define IS_HORIZONTAL(box) ((box)->arrow_type == GTK_ARROW_LEFT \
                            || (box)->arrow_type == GTK_ARROW_RIGHT)



static void     systray_box_finalize           (GObject         *object);
static void     systray_box_size_request       (GtkWidget       *widget,
                                                GtkRequisition  *requisition);
static void     systray_box_size_allocate      (GtkWidget       *widget,
                                                GtkAllocation   *allocation);
static gboolean systray_box_expose_event       (GtkWidget       *widget,
                                                GdkEventExpose  *event);
static void     systray_box_add                (GtkContainer    *container,
                                                GtkWidget       *child);
static void     systray_box_remove             (GtkContainer    *container,
                                                GtkWidget       *child);
static void     systray_box_forall             (GtkContainer    *container,
                                                gboolean         include_internals,
                                                GtkCallback      callback,
                                                gpointer         callback_data);
static GType    systray_box_child_type         (GtkContainer    *container);
static void     systray_box_button_set_arrow   (SystrayBox      *box);
static gboolean systray_box_button_press_event (GtkWidget       *widget,
                                                GdkEventButton  *event,
                                                GtkWidget       *box);
static void     systray_box_button_clicked     (GtkToggleButton *button,
                                                SystrayBox      *box);



struct _SystrayBoxClass
{
  GtkContainerClass __parent__;
};

struct _SystrayBox
{
  GtkContainer  __parent__;

  /* all the icons packed in this box */
  GSList       *childeren;

  /* table with names, value contains an uint
   * that represents the hidden bool */
  GHashTable   *names;

  /* expand button */
  GtkWidget    *button;

  /* position of the arrow button */
  GtkArrowType  arrow_type;

  /* hidden childeren counter */
  gint          n_hidden_childeren;

  /* whether hidden icons are visible */
  guint         show_hidden : 1;

  /* number of rows */
  gint          rows;

  /* guess size, this is a value used to reduce the tray flickering */
  gint          guess_size;
};

typedef struct
{
  /* the child widget */
  GtkWidget    *widget;

  /* whether it could be hidden */
  guint         auto_hide : 1;

  /* invisible icon because of invalid requisition */
  guint         invalid : 1;

  /* the name of the applcation */
  gchar        *name;
}
SystrayBoxChild;



XFCE_PANEL_DEFINE_TYPE (SystrayBox, systray_box, GTK_TYPE_CONTAINER)



static void
systray_box_class_init (SystrayBoxClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;
  GtkContainerClass *gtkcontainer_class;

  gobject_class = G_OBJECT_CLASS (klass);
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
}



static void
systray_box_init (SystrayBox *box)
{
  /* initialize the widget */
  GTK_WIDGET_SET_FLAGS (box, GTK_NO_WINDOW);

  /* initialize */
  box->childeren = NULL;
  box->button = NULL;
  box->rows = 1;
  box->n_hidden_childeren = 0;
  box->arrow_type = GTK_ARROW_LEFT;
  box->show_hidden = FALSE;
  box->guess_size = 128;

  /* create hash table */
  box->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* create arrow button */
  box->button = xfce_arrow_button_new (box->arrow_type);
  GTK_WIDGET_UNSET_FLAGS (box->button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
  gtk_button_set_focus_on_click (GTK_BUTTON (box->button), FALSE);
  g_signal_connect (G_OBJECT (box->button), "clicked",
      G_CALLBACK (systray_box_button_clicked), box);
  g_signal_connect (G_OBJECT (box->button), "button-press-event",
      G_CALLBACK (systray_box_button_press_event), box);
  gtk_widget_set_parent (box->button, GTK_WIDGET (box));
}



static void
systray_box_finalize (GObject *object)
{
  SystrayBox *box = XFCE_SYSTRAY_BOX (object);

  /* check if we're leaking */
  if (G_UNLIKELY (box->childeren != NULL))
    {
      /* free the child list */
      g_slist_free (box->childeren);
      g_debug ("Leaking memory, not all children have been removed");
    }

  /* destroy the hash table */
  g_hash_table_destroy (box->names);

  G_OBJECT_CLASS (systray_box_parent_class)->finalize (object);
}



static void
systray_box_size_request (GtkWidget      *widget,
                          GtkRequisition *requisition)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (widget);
  GSList          *li;
  SystrayBoxChild *child_info;
  gint             n_columns;
  gint             child_size = -1;
  GtkRequisition   child_req;
  gint             n_visible_childeren = 0;
  gint             swap;
  gint             guess_size, icon_size;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (widget));
  panel_return_if_fail (requisition != NULL);

  /* get the guess size */
  guess_size = box->guess_size - (SPACING * (box->rows - 1));
  guess_size /= box->rows;

  /* check if we need to hide or show any childeren */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child_info = li->data;

      /* get the icons size request */
      gtk_widget_size_request (child_info->widget, &child_req);

      if (G_UNLIKELY (child_req.width == 1 || child_req.height == 1))
        {
          /* icons that return a 1 by 1 requisition supposed to be hidden */
          if (child_info->invalid == FALSE)
            {
              /* this icon should not be visible */
              child_info->invalid = TRUE;

              /* decrease the hidden counter if needed */
              if (child_info->auto_hide)
                box->n_hidden_childeren--;
            }
        }
      else
        {
          /* restore icon if it was previously invisible */
          if (G_UNLIKELY (child_info->invalid))
            {
              /* visible icon */
              child_info->invalid = FALSE;

              /* update counter */
              if (child_info->auto_hide)
                box->n_hidden_childeren++;
            }

          /* count the number of visible childeren */
          if (child_info->auto_hide == FALSE || box->show_hidden == TRUE)
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
        }
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

  /* add the button size if there are hidden icons */
  if (box->n_hidden_childeren > 0)
    {
      /* add the button size */
      requisition->width += BUTTON_SIZE;

      /* add space */
      if (n_visible_childeren > 0)
        requisition->width += SPACING;
    }

  /* swap the sizes if the orientation is vertical */
  if (!IS_HORIZONTAL (box))
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
  SystrayBoxChild *child_info;
  GSList          *li;
  gint             n;
  gint             x, y;
  gint             width, height;
  gint             offset = 0;
  gint             child_size;
  GtkAllocation    child_allocation;
  gint             swap;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (widget));
  panel_return_if_fail (allocation != NULL);

  /* set widget allocation */
  widget->allocation = *allocation;

  /* get root coordinates */
  x = allocation->x + GTK_CONTAINER (widget)->border_width;
  y = allocation->y + GTK_CONTAINER (widget)->border_width;

  /* get real size */
  width = allocation->width - 2 * GTK_CONTAINER (widget)->border_width;
  height = allocation->height - 2 * GTK_CONTAINER (widget)->border_width;

  /* child size */
  child_size = IS_HORIZONTAL (box) ? height : width;
  child_size -= SPACING * (box->rows - 1);
  child_size /= box->rows;

  /* don't allocate zero width icon */
  if (child_size < 1)
    child_size = 1;

  /* position arrow button */
  if (box->n_hidden_childeren > 0)
    {
      /* initialize allocation */
      child_allocation.x = x;
      child_allocation.y = y;

      /* set the width and height */
      if (IS_HORIZONTAL (box))
        {
          child_allocation.width = BUTTON_SIZE;
          child_allocation.height = height;
        }
      else
        {
          child_allocation.width = width;
          child_allocation.height = BUTTON_SIZE;
        }

      /* position the button on the other side of the box */
      if (box->arrow_type == GTK_ARROW_RIGHT)
        child_allocation.x += width - child_allocation.width;
      else if (box->arrow_type == GTK_ARROW_DOWN)
        child_allocation.y += height - child_allocation.height;

      /* set the offset for the icons */
      offset = BUTTON_SIZE + SPACING;

      /* position the arrow button */
      gtk_widget_size_allocate (box->button, &child_allocation);

      /* show button if not already visible */
      if (!GTK_WIDGET_VISIBLE (box->button))
        gtk_widget_show (box->button);
    }
  else if (GTK_WIDGET_VISIBLE (box->button))
    {
      /* hide the button */
      gtk_widget_hide (box->button);
    }

  /* position icons */
  for (li = box->childeren, n = 0; li != NULL; li = li->next)
    {
      child_info = li->data;

      if (child_info->invalid || (child_info->auto_hide && !box->show_hidden))
        {
          /* put icons offscreen */
          child_allocation.x = child_allocation.y = OFFSCREEN;
        }
      else
        {
          /* set coordinates */
          child_allocation.x = (child_size + SPACING) * (n / box->rows) + offset;
          child_allocation.y = (child_size + SPACING) * (n % box->rows);

          /* increase item counter */
          n++;

          /* swap coordinates on a vertical panel */
          if (!IS_HORIZONTAL (box))
            {
              swap = child_allocation.x;
              child_allocation.x = child_allocation.y;
              child_allocation.y = swap;
            }

          /* invert the icon order if the arrow button position is right or down */
          if (box->arrow_type == GTK_ARROW_RIGHT)
            child_allocation.x = width - child_allocation.x - child_size;
          else if (box->arrow_type == GTK_ARROW_DOWN)
            child_allocation.y = height - child_allocation.y - child_size;

          /* add root */
          child_allocation.x += x;
          child_allocation.y += y;
        }

      /* set child width and height */
      child_allocation.width = child_size;
      child_allocation.height = child_size;

      /* allocate widget size */
      gtk_widget_size_allocate (child_info->widget, &child_allocation);
    }
}



static gboolean
systray_box_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (widget);
  cairo_t         *cr;
  SystrayBoxChild *child_info;
  GSList          *li;
  gboolean         result;

  result = GTK_WIDGET_CLASS (systray_box_parent_class)->expose_event (widget, event);

  if (gtk_widget_is_composited (widget))
    {
      cr = gdk_cairo_create (widget->window);
      gdk_cairo_region (cr, event->region);
      cairo_clip (cr);

      for (li = box->childeren; li != NULL; li = li->next)
        {
          child_info = li->data;

          /* skip invisible or not composited children */
          if (child_info->invalid
              || (child_info->auto_hide && !box->show_hidden)
              || !systray_socket_is_composited (XFCE_SYSTRAY_SOCKET (child_info->widget)))
            continue;

          /* paint the child */
          gdk_cairo_set_source_pixmap (cr, child_info->widget->window,
                                       child_info->widget->allocation.x,
                                       child_info->widget->allocation.y);
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

  /* add the entry */
  systray_box_add_with_name (box, child, NULL);
}



static void
systray_box_remove (GtkContainer *container,
                    GtkWidget    *child)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (container);
  SystrayBoxChild *child_info;
  gboolean         need_resize;
  GSList          *li;

  /* search the child */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child_info = li->data;

      if (child_info->widget == child)
        {
          /* whether the need to redraw afterwards */
          need_resize = !child_info->auto_hide;

          /* update hidden counter */
          if (child_info->auto_hide && !child_info->invalid)
            box->n_hidden_childeren--;

          /* remove from list */
          box->childeren = g_slist_remove_link (box->childeren, li);

          /* free name */
          g_free (child_info->name);

          /* free child info */
          g_slice_free (SystrayBoxChild, child_info);

          /* unparent the widget */
          gtk_widget_unparent (child);

          /* resize when the child was visible */
          if (need_resize)
            gtk_widget_queue_resize (GTK_WIDGET (container));

          return;
        }
    }
}



static void
systray_box_forall (GtkContainer *container,
                    gboolean      include_internals,
                    GtkCallback   callback,
                    gpointer      callback_data)
{
  SystrayBox      *box = XFCE_SYSTRAY_BOX (container);
  SystrayBoxChild *child_info;
  GSList          *li;

  /* for button */
  (*callback) (GTK_WIDGET (box->button), callback_data);

  /* run callback for all childeren */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child_info = li->data;

      (*callback) (GTK_WIDGET (child_info->widget), callback_data);
    }
}



static GType
systray_box_child_type (GtkContainer *container)

{
  return GTK_TYPE_WIDGET;
}



static void
systray_box_button_set_arrow (SystrayBox *box)
{
  GtkArrowType arrow_type;

  /* set arrow type */
  arrow_type = box->arrow_type;

  /* invert the arrow direction when the button is toggled */
  if (box->show_hidden)
    {
      if (IS_HORIZONTAL (box))
        arrow_type = (arrow_type == GTK_ARROW_LEFT ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
      else
        arrow_type = (arrow_type == GTK_ARROW_UP ? GTK_ARROW_DOWN : GTK_ARROW_UP);
    }

  /* set the arrow type */
  xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (box->button), arrow_type);
}



static gboolean
systray_box_button_press_event (GtkWidget      *widget,
                                GdkEventButton *event,
                                GtkWidget      *box)
{
  /* send the event to the box for the panel menu */
  gtk_widget_event (box, (GdkEvent *) event);

  return FALSE;
}



static void
systray_box_button_clicked (GtkToggleButton *button,
                            SystrayBox      *box)
{
  /* whether to show hidden icons */
  box->show_hidden = gtk_toggle_button_get_active (button);

  /* update the arrow */
  systray_box_button_set_arrow (box);

  /* queue a resize */
  gtk_widget_queue_resize (GTK_WIDGET (box));
}



static gint
systray_box_compare_function (gconstpointer a,
                                   gconstpointer b)
{
  const SystrayBoxChild *child_a = a;
  const SystrayBoxChild *child_b = b;

  /* sort hidden icons before visible ones */
  if (child_a->auto_hide != child_b->auto_hide)
    return (child_a->auto_hide ? -1 : 1);

  /* put icons without name after the hidden icons */
  if (!IS_STRING (child_a->name) || !IS_STRING (child_b->name))
    {
      if (IS_STRING (child_a->name) == IS_STRING (child_b->name))
        return 0;
      else
        return !IS_STRING (child_a->name) ? -1 : 1;
    }

  /* sort by name */
  return strcmp (child_a->name, child_b->name);
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
systray_box_set_arrow_type (SystrayBox   *box,
                            GtkArrowType  arrow_type)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));

  if (G_LIKELY (arrow_type != box->arrow_type))
    {
      /* set new setting */
      box->arrow_type = arrow_type;

      /* update button arrow */
      systray_box_button_set_arrow (box);

      /* queue a resize */
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
systray_box_add_with_name (SystrayBox  *box,
                           GtkWidget   *child,
                           const gchar *name)
{
  SystrayBoxChild *child_info;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));
  panel_return_if_fail (GTK_IS_WIDGET (child));
  panel_return_if_fail (child->parent == NULL);
  panel_return_if_fail (name == NULL || g_utf8_validate (name, -1, NULL));

  /* create child info */
  child_info = g_slice_new (SystrayBoxChild);
  child_info->widget = child;
  child_info->invalid = FALSE;
  child_info->name = g_strdup (name);
  child_info->auto_hide = systray_box_name_get_hidden (box, child_info->name);

  /* update hidden counter */
  if (child_info->auto_hide)
      box->n_hidden_childeren++;

  /* insert sorted */
  box->childeren = g_slist_insert_sorted (box->childeren, child_info,
                                          systray_box_compare_function);

  /* set parent widget */
  gtk_widget_set_parent (child, GTK_WIDGET (box));
}



void
systray_box_name_add (SystrayBox  *box,
                      const gchar *name,
                      gboolean     hidden)
{
  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));
  panel_return_if_fail (IS_STRING (name));

  /* insert the application */
  g_hash_table_insert (box->names, g_strdup (name),
                       GUINT_TO_POINTER (hidden ? 1 : 0));
}



void
systray_box_name_set_hidden (SystrayBox  *box,
                             const gchar *name,
                             gboolean     hidden)
{
  SystrayBoxChild *child_info;
  GSList          *li;
  gint             n_hidden_childeren;

  panel_return_if_fail (XFCE_IS_SYSTRAY_BOX (box));
  panel_return_if_fail (IS_STRING (name));

  /* replace the old name */
  g_hash_table_replace (box->names, g_strdup (name),
      GUINT_TO_POINTER (hidden ? 1 : 0));

  /* reset counter */
  n_hidden_childeren = 0;

  /* update the icons */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child_info = li->data;

      /* update the hidden state */
      child_info->auto_hide = systray_box_name_get_hidden (box, child_info->name);

      /* increase counter if needed */
      if (child_info->auto_hide && !child_info->invalid)
        n_hidden_childeren++;
    }

  if (box->n_hidden_childeren != n_hidden_childeren)
    {
      /* set value */
      box->n_hidden_childeren = n_hidden_childeren;

      /* sort the list again */
      box->childeren = g_slist_sort (box->childeren,
          systray_box_compare_function);

      /* update the box */
      gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}



gboolean
systray_box_name_get_hidden (SystrayBox  *box,
                             const gchar *name)
{
  gpointer p;

  /* do not hide icons without name */
  if (G_UNLIKELY (name == NULL))
    return FALSE;

  /* lookup the name in the table */
  p = g_hash_table_lookup (box->names, name);

  /* check the pointer */
  if (G_UNLIKELY (p == NULL))
    {
      /* add the name */
      systray_box_name_add (box, name, FALSE);

      /* do not hide the icon */
      return FALSE;
    }
  else
    {
      return (GPOINTER_TO_UINT (p) == 1 ? TRUE : FALSE);
    }
}



GList *
systray_box_name_list (SystrayBox *box)
{
  GList *keys;

  /* get the hash table keys */
  keys = g_hash_table_get_keys (box->names);

  /* sort the list */
  keys = g_list_sort (keys, (GCompareFunc) strcmp);

  return keys;
}



void
systray_box_name_clear (SystrayBox *box)
{
  SystrayBoxChild *child_info;
  GSList          *li;
  gint             n_hidden_childeren = 0;

  /* remove all the entries from the list */
  g_hash_table_remove_all (box->names);

  /* remove hidden flags from all childeren */
  for (li = box->childeren; li != NULL; li = li->next)
    {
      child_info = li->data;

      /* update the hidden state */
      if (child_info->auto_hide)
        {
          n_hidden_childeren++;

          child_info->auto_hide = FALSE;
        }
    }

  /* reset */
  box->n_hidden_childeren = 0;

  /* update box if needed */
  if (n_hidden_childeren > 0)
    {
      /* sort the list again */
      box->childeren = g_slist_sort (box->childeren,
          systray_box_compare_function);

      /* update the box */
      gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}

