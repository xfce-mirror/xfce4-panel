/* $Id$ */
/*
 * Copyright (c) 2007 Nick Schermer <nick@xfce.org>
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
#include <libxfce4panel/xfce-arrow-button.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"
#include "xfce-tray-plugin.h"

#define XFCE_TRAY_WIDGET_BUTTON_SIZE          (16)
#define XFCE_TRAY_WIDGET_OFFSCREEN            (-9999)
#define XFCE_TRAY_WIDGET_IS_HORIZONTAL(tray)  ((tray)->arrow_type == GTK_ARROW_LEFT || (tray)->arrow_type == GTK_ARROW_RIGHT)
#define XFCE_TRAY_WIDGET_SWAP_INT(x,y)        G_STMT_START{ gint __v = (x); (x) = (y); (y) = __v; }G_STMT_END



/* prototypes */
static void     xfce_tray_widget_class_init         (XfceTrayWidgetClass *klass);
static void     xfce_tray_widget_init               (XfceTrayWidget      *tray);
static void     xfce_tray_widget_finalize           (GObject             *object);
static void     xfce_tray_widget_size_request       (GtkWidget           *widget,
                                                     GtkRequisition      *requisition);
static void     xfce_tray_widget_size_allocate      (GtkWidget           *widget,
                                                     GtkAllocation       *allocation);
static void     xfce_tray_widget_add                (GtkContainer        *container,
                                                     GtkWidget           *child);
static void     xfce_tray_widget_remove             (GtkContainer        *container,
                                                     GtkWidget           *child);
static void     xfce_tray_widget_forall             (GtkContainer        *container,
                                                     gboolean             include_internals,
                                                     GtkCallback          callback,
                                                     gpointer             callback_data);
static GType    xfce_tray_widget_child_type         (GtkContainer        *container);
static void     xfce_tray_widget_button_set_arrow   (XfceTrayWidget      *tray);
static gboolean xfce_tray_widget_button_press_event (GtkWidget           *widget,
                                                     GdkEventButton      *event,
                                                     GtkWidget           *tray);
static void     xfce_tray_widget_button_clicked     (GtkToggleButton     *button,
                                                     XfceTrayWidget      *tray);



struct _XfceTrayWidgetClass
{
    GtkContainerClass __parent__;
};

struct _XfceTrayWidget
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

    /* last allocated child size, used to prevent icon overflow */
    gint          last_alloc_child_size;

    /* whether hidden icons are visible */
    guint         show_hidden : 1;

    /* number of rows */
    gint          rows;
};

struct _XfceTrayWidgetChild
{
    /* the child widget */
    GtkWidget    *widget;

    /* whether it could be hidden */
    guint         hidden : 1;

    /* whether the icon is invisible */
    guint         invisible : 1;

    /* the name of the applcation */
    gchar        *name;
};



static GObjectClass *xfce_tray_widget_parent_class;



GType
xfce_tray_widget_get_type (void)
{
    static GType type = G_TYPE_INVALID;

    if (G_UNLIKELY (type == G_TYPE_INVALID))
    {
        type = g_type_register_static_simple (GTK_TYPE_CONTAINER,
                                              I_("XfceTrayWidget"),
                                              sizeof (XfceTrayWidgetClass),
                                              (GClassInitFunc) xfce_tray_widget_class_init,
                                              sizeof (XfceTrayWidget),
                                              (GInstanceInitFunc) xfce_tray_widget_init,
                                              0);
    }

    return type;
}



static void
xfce_tray_widget_class_init (XfceTrayWidgetClass *klass)
{
    GObjectClass      *gobject_class;
    GtkWidgetClass    *gtkwidget_class;
    GtkContainerClass *gtkcontainer_class;

    /* determine the parent type class */
    xfce_tray_widget_parent_class = g_type_class_peek_parent (klass);

    gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = xfce_tray_widget_finalize;

    gtkwidget_class = GTK_WIDGET_CLASS (klass);
    gtkwidget_class->size_request = xfce_tray_widget_size_request;
    gtkwidget_class->size_allocate = xfce_tray_widget_size_allocate;

    gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
    gtkcontainer_class->add = xfce_tray_widget_add;
    gtkcontainer_class->remove = xfce_tray_widget_remove;
    gtkcontainer_class->forall = xfce_tray_widget_forall;
    gtkcontainer_class->child_type = xfce_tray_widget_child_type;
}



static void
xfce_tray_widget_init (XfceTrayWidget *tray)
{
    /* initialize the widget */
    GTK_WIDGET_SET_FLAGS (tray, GTK_NO_WINDOW);
    gtk_widget_set_redraw_on_allocate (GTK_WIDGET (tray), FALSE);

    /* initialize */
    tray->childeren = NULL;
    tray->button = NULL;
    tray->rows = 1;
    tray->n_hidden_childeren = 0;
    tray->arrow_type = GTK_ARROW_LEFT;
    tray->show_hidden = FALSE;
    tray->last_alloc_child_size = 0;

    /* create hash table */
    tray->names = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    /* create tray button */
    tray->button = xfce_arrow_button_new (tray->arrow_type);
    GTK_WIDGET_UNSET_FLAGS (tray->button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
    gtk_button_set_focus_on_click (GTK_BUTTON (tray->button), FALSE);
    g_signal_connect (G_OBJECT (tray->button), "clicked", G_CALLBACK (xfce_tray_widget_button_clicked), tray);
    g_signal_connect (G_OBJECT (tray->button), "button-press-event", G_CALLBACK (xfce_tray_widget_button_press_event), tray);
    gtk_widget_set_parent (tray->button, GTK_WIDGET (tray));
}



static void
xfce_tray_widget_finalize (GObject *object)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (object);

    /* check if we're leaking */
    if (G_UNLIKELY (tray->childeren != NULL))
    {
        g_message ("Leaking memory: Not all icons have been removed");

        /* free the child list */
        g_slist_free (tray->childeren);
    }

    /* destroy the hash table */
    g_hash_table_destroy (tray->names);

    G_OBJECT_CLASS (xfce_tray_widget_parent_class)->finalize (object);
}



static void
xfce_tray_widget_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
    XfceTrayWidget      *tray = XFCE_TRAY_WIDGET (widget);
    GSList              *li;
    XfceTrayWidgetChild *child_info;
    gint                 child_size = 0;
    gint                 n_columns;
    GtkRequisition       child_requisition;
    gint                 n_visible_childeren = 0;

    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (widget));
    _panel_return_if_fail (requisition != NULL);

    for (li = tray->childeren; li != NULL; li = li->next)
    {
        child_info = li->data;

        /* get the icons size request */
        gtk_widget_size_request (child_info->widget, &child_requisition);

        if (G_UNLIKELY (child_requisition.width == 1 || child_requisition.height == 1))
        {
            /* don't do anything with already invisible icons */
            if (child_info->invisible == FALSE)
            {
                /* this icon should not be visible */
                child_info->invisible = TRUE;

                /* decrease the hidden counter if needed */
                if (child_info->hidden)
                    tray->n_hidden_childeren--;
            }
        }
        else
        {
            /* restore icon if it was previously invisible */
            if (G_UNLIKELY (child_info->invisible))
            {
               /* visible icon */
               child_info->invisible = FALSE;

               /* update counter */
               if (child_info->hidden)
                   tray->n_hidden_childeren++;
            }

            /* only allocate size for not hidden icons */
            if (child_info->hidden == FALSE || tray->show_hidden == TRUE)
            {
                /* update the child size (tray is homogeneous, so pick the largest icon) */
                child_size = MAX (child_size, MAX (child_requisition.width, child_requisition.height));

                /* increase number of visible childeren */
                n_visible_childeren++;
            }
        }
    }

    /* number of columns */
    n_columns = n_visible_childeren / tray->rows;
    if (n_visible_childeren > (n_columns * tray->rows))
        n_columns++;

    /* make sure the maximum child size does not overflow the tray */
    child_size = MIN (child_size, tray->last_alloc_child_size);

    /* set the width and height needed for the icons */
    if (n_visible_childeren > 0)
    {
        requisition->width = ((child_size + XFCE_TRAY_WIDGET_SPACING) * n_columns) - XFCE_TRAY_WIDGET_SPACING;
        requisition->height = ((child_size + XFCE_TRAY_WIDGET_SPACING) * tray->rows) - XFCE_TRAY_WIDGET_SPACING;
    }
    else
    {
        requisition->width = requisition->height = 0;
    }

    /* add the button size if there are hidden icons */
    if (tray->n_hidden_childeren > 0)
    {
        /* add the button size */
        requisition->width += XFCE_TRAY_WIDGET_BUTTON_SIZE;

        /* add space */
        if (n_visible_childeren > 0)
             requisition->width += XFCE_TRAY_WIDGET_SPACING;
    }

    /* swap the sizes if the orientation is vertical */
    if (!XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
        XFCE_TRAY_WIDGET_SWAP_INT (requisition->width, requisition->height);

    /* add container border */
    requisition->width += GTK_CONTAINER (widget)->border_width * 2;
    requisition->height += GTK_CONTAINER (widget)->border_width * 2;
}



static void
xfce_tray_widget_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
    XfceTrayWidget      *tray = XFCE_TRAY_WIDGET (widget);
    XfceTrayWidgetChild *child_info;
    GSList              *li;
    gint                 n;
    gint                 x, y;
    gint                 width, height;
    gint                 offset = 0;
    gint                 child_size;
    GtkAllocation        child_allocation;

    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (widget));
    _panel_return_if_fail (allocation != NULL);

    /* set widget allocation */
    widget->allocation = *allocation;

    /* get root coordinates */
    x = allocation->x + GTK_CONTAINER (widget)->border_width;
    y = allocation->y + GTK_CONTAINER (widget)->border_width;

    /* get real size */
    width = allocation->width - 2 * GTK_CONTAINER (widget)->border_width;
    height = allocation->height - 2 * GTK_CONTAINER (widget)->border_width;

    /* child size */
    child_size = XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray) ? height : width;
    child_size -= XFCE_TRAY_WIDGET_SPACING * (tray->rows - 1);
    child_size /= tray->rows;

    /* set last allocated child size */
    tray->last_alloc_child_size = child_size;

    /* position arrow button */
    if (tray->n_hidden_childeren > 0)
    {
        /* initialize allocation */
        child_allocation.x = x;
        child_allocation.y = y;

        /* set the width and height */
        if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
        {
            child_allocation.width = XFCE_TRAY_WIDGET_BUTTON_SIZE;
            child_allocation.height = height;
        }
        else
        {
            child_allocation.width = width;
            child_allocation.height = XFCE_TRAY_WIDGET_BUTTON_SIZE;
        }

        /* position the button on the other side of the tray */
        if (tray->arrow_type == GTK_ARROW_RIGHT)
            child_allocation.x += width - child_allocation.width;
        else if (tray->arrow_type == GTK_ARROW_DOWN)
            child_allocation.y += height - child_allocation.height;

        /* set the offset for the icons */
        offset = XFCE_TRAY_WIDGET_BUTTON_SIZE + XFCE_TRAY_WIDGET_SPACING;

        /* position the arrow button */
        gtk_widget_size_allocate (tray->button, &child_allocation);

        /* show button if not already visible */
        if (!GTK_WIDGET_VISIBLE (tray->button))
            gtk_widget_show (tray->button);
    }
    else if (GTK_WIDGET_VISIBLE (tray->button))
    {
        /* hide the button */
        gtk_widget_hide (tray->button);
    }

    /* position icons */
    for (li = tray->childeren, n = 0; li != NULL; li = li->next)
    {
        child_info = li->data;

        if (child_info->invisible || (child_info->hidden && !tray->show_hidden))
        {
            /* put icons offscreen */
            child_allocation.x = child_allocation.y = XFCE_TRAY_WIDGET_OFFSCREEN;
        }
        else
        {
            /* set coordinates */
            child_allocation.x = (child_size + XFCE_TRAY_WIDGET_SPACING) * (n / tray->rows) + offset;
            child_allocation.y = (child_size + XFCE_TRAY_WIDGET_SPACING) * (n % tray->rows);

            /* increase item counter */
            n++;

            /* swap coordinates on a vertical panel */
            if (!XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
                XFCE_TRAY_WIDGET_SWAP_INT (child_allocation.x, child_allocation.y);

            /* invert the icon order if the arrow button position is right or down */
            if (tray->arrow_type == GTK_ARROW_RIGHT)
                child_allocation.x = width - child_allocation.x - child_size;
            else if (tray->arrow_type == GTK_ARROW_DOWN)
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



static void
xfce_tray_widget_add (GtkContainer *container,
                      GtkWidget    *child)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (container);

    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (container));

    /* add the entry */
    xfce_tray_widget_add_with_name (tray, child, NULL);
}



static void
xfce_tray_widget_remove (GtkContainer *container,
                         GtkWidget    *child)
{
    XfceTrayWidget      *tray = XFCE_TRAY_WIDGET (container);
    XfceTrayWidgetChild *child_info;
    gboolean             need_resize;
    GSList              *li;

    /* search the child */
    for (li = tray->childeren; li != NULL; li = li->next)
    {
        child_info = li->data;

        if (child_info->widget == child)
        {
            /* whether the need to redraw afterwards */
            need_resize = !child_info->hidden;

            /* update hidden counter */
            if (child_info->hidden && !child_info->invisible)
                tray->n_hidden_childeren--;

            /* remove from list */
            tray->childeren = g_slist_remove_link (tray->childeren, li);

            /* free name */
            g_free (child_info->name);

            /* free child info */
            panel_slice_free (XfceTrayWidgetChild, child_info);

            /* unparent the widget */
            gtk_widget_unparent (child);

            /* resize when the child was visible */
            if (need_resize)
                gtk_widget_queue_resize (GTK_WIDGET (container));

            break;
        }
    }
}



static void
xfce_tray_widget_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
    XfceTrayWidget      *tray = XFCE_TRAY_WIDGET (container);
    XfceTrayWidgetChild *child_info;
    GSList              *li;

    /* for button */
    (*callback) (GTK_WIDGET (tray->button), callback_data);

    /* run callback for all childeren */
    for (li = tray->childeren; li != NULL; li = li->next)
    {
        child_info = li->data;

        (*callback) (GTK_WIDGET (child_info->widget), callback_data);
    }
}



static GType
xfce_tray_widget_child_type (GtkContainer *container)

{
    return GTK_TYPE_WIDGET;
}



static void
xfce_tray_widget_button_set_arrow (XfceTrayWidget *tray)
{
    GtkArrowType arrow_type;

    /* set arrow type */
    arrow_type = tray->arrow_type;

    /* invert the arrow direction when the button is toggled */
    if (tray->show_hidden)
    {
        if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
            arrow_type = (arrow_type == GTK_ARROW_LEFT ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
        else
            arrow_type = (arrow_type == GTK_ARROW_UP ? GTK_ARROW_DOWN : GTK_ARROW_UP);
    }

    /* set the arrow type */
    xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (tray->button), arrow_type);
}



static gboolean
xfce_tray_widget_button_press_event (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     GtkWidget      *tray)
{
    /* send the event to the tray for the panel menu */
    gtk_widget_event (tray, (GdkEvent *) event);

    return FALSE;
}



static void
xfce_tray_widget_button_clicked (GtkToggleButton *button,
                                 XfceTrayWidget  *tray)
{
    /* whether to show hidden icons */
    tray->show_hidden = gtk_toggle_button_get_active (button);

    /* update the arrow */
    xfce_tray_widget_button_set_arrow (tray);

    /* queue a resize */
    gtk_widget_queue_resize (GTK_WIDGET (tray));
}



static gint
xfce_tray_widget_compare_function (gconstpointer a,
                                   gconstpointer b)
{
    const XfceTrayWidgetChild *child_a = a;
    const XfceTrayWidgetChild *child_b = b;

    /* sort hidden icons before visible ones */
    if (child_a->hidden != child_b->hidden)
        return (child_a->hidden ? -1 : 1);

    /* put icons without name after the hidden icons */
    if (G_UNLIKELY (child_a->name == NULL || child_b->name == NULL))
        return (child_a->name == child_b->name ? 0 : (child_a->name == NULL ? -1 : 1));

    /* sort by name */
    return strcmp (child_a->name, child_b->name);
}



GtkWidget *
xfce_tray_widget_new (void)
{
    return g_object_new (XFCE_TYPE_TRAY_WIDGET, NULL);
}



void
xfce_tray_widget_add_with_name (XfceTrayWidget *tray,
                                GtkWidget      *child,
                                const gchar    *name)
{
    XfceTrayWidgetChild *child_info;

    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    _panel_return_if_fail (GTK_IS_WIDGET (child));
    _panel_return_if_fail (child->parent == NULL);
    _panel_return_if_fail (name == NULL || g_utf8_validate (name, -1, NULL));

    /* create child info */
    child_info = panel_slice_new (XfceTrayWidgetChild);
    child_info->widget = child;
    child_info->invisible = FALSE;
    child_info->name = g_strdup (name);
    child_info->hidden = xfce_tray_widget_name_hidden (tray, child_info->name);

    /* update hidden counter */
    if (child_info->hidden)
        tray->n_hidden_childeren++;

    /* insert sorted */
    tray->childeren = g_slist_insert_sorted (tray->childeren, child_info, xfce_tray_widget_compare_function);

    /* set parent widget */
    gtk_widget_set_parent (child, GTK_WIDGET (tray));
}



void
xfce_tray_widget_set_arrow_type (XfceTrayWidget *tray,
                                 GtkArrowType    arrow_type)
{
    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));

    if (G_LIKELY (arrow_type != tray->arrow_type))
    {
        /* set new setting */
        tray->arrow_type = arrow_type;

        /* update button arrow */
        xfce_tray_widget_button_set_arrow (tray);

        /* queue a resize */
        if (tray->childeren != NULL)
            gtk_widget_queue_resize (GTK_WIDGET (tray));
    }
}



GtkArrowType
xfce_tray_widget_get_arrow_type (XfceTrayWidget *tray)
{
    _panel_return_val_if_fail (XFCE_IS_TRAY_WIDGET (tray), GTK_ARROW_LEFT);

    return tray->arrow_type;
}



void
xfce_tray_widget_set_rows (XfceTrayWidget *tray,
                           gint            rows)
{
    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));

    if (G_LIKELY (rows != tray->rows))
    {
        /* set new setting */
        tray->rows = MAX (1, rows);

        /* queue a resize */
        if (tray->childeren != NULL)
            gtk_widget_queue_resize (GTK_WIDGET (tray));
    }
}



gint
xfce_tray_widget_get_rows (XfceTrayWidget *tray)
{
    _panel_return_val_if_fail (XFCE_IS_TRAY_WIDGET (tray), 1);

    return tray->rows;
}



void
xfce_tray_widget_name_add (XfceTrayWidget *tray,
                           const gchar    *name,
                           gboolean        hidden)
{
    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    _panel_return_if_fail (name != NULL && *name != '\0');

    /* insert the application */
    g_hash_table_insert (tray->names, g_strdup (name), GUINT_TO_POINTER (hidden ? 1 : 0));
}



void
xfce_tray_widget_name_update (XfceTrayWidget *tray,
                              const gchar    *name,
                              gboolean        hidden)
{
    XfceTrayWidgetChild *child_info;
    GSList              *li;
    gint                 n_hidden_childeren;

    _panel_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    _panel_return_if_fail (name != NULL && *name != '\0');

    /* replace the old name */
    g_hash_table_replace (tray->names, g_strdup (name), GUINT_TO_POINTER (hidden ? 1 : 0));

    /* reset counter */
    n_hidden_childeren = 0;

    /* update the icons */
    for (li = tray->childeren; li != NULL; li = li->next)
    {
        child_info = li->data;

        /* update the hidden state */
        child_info->hidden = xfce_tray_widget_name_hidden (tray, child_info->name);

        /* increase counter if needed */
        if (child_info->hidden && !child_info->invisible)
            n_hidden_childeren++;
    }

    if (tray->n_hidden_childeren != n_hidden_childeren)
    {
        /* set value */
        tray->n_hidden_childeren = n_hidden_childeren;

        /* sort the list again */
        tray->childeren = g_slist_sort (tray->childeren, xfce_tray_widget_compare_function);

        /* update the tray */
        gtk_widget_queue_resize (GTK_WIDGET (tray));
    }
}



gboolean
xfce_tray_widget_name_hidden (XfceTrayWidget *tray,
                              const gchar    *name)
{
    gpointer p;

    /* do not hide icons without name */
    if (G_UNLIKELY (name == NULL))
        return FALSE;

    /* lookup the name in the table */
    p = g_hash_table_lookup (tray->names, name);

    /* check the pointer */
    if (G_UNLIKELY (p == NULL))
    {
        /* add the name */
        xfce_tray_widget_name_add (tray, name, FALSE);

        /* do not hide the icon */
        return FALSE;
    }
    else
    {
        return (GPOINTER_TO_UINT (p) == 1 ? TRUE : FALSE);
    }
}



GList *
xfce_tray_widget_name_list (XfceTrayWidget *tray)
{
    GList *keys;

    /* get the hash table keys */
    keys = g_hash_table_get_keys (tray->names);

    /* sort the list */
    keys = g_list_sort (keys, (GCompareFunc) strcmp);

    return keys;
}
