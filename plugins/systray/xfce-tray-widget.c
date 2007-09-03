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

#include "xfce-tray-manager.h"
#include "xfce-tray-widget.h"

#define XFCE_TRAY_WIDGET_BUTTON_SIZE          (16)
#define XFCE_TRAY_WIDGET_SPACING              (1)
#define XFCE_TRAY_WIDGET_OFFSCREEN            (-9999)
#define XFCE_TRAY_WIDGET_IS_HORIZONTAL(obj)   ((obj)->arrow_position == GTK_ARROW_LEFT || (obj)->arrow_position == GTK_ARROW_RIGHT)
#define XFCE_TRAY_WIDGET_IS_SOUTH_EAST(obj)   ((obj)->arrow_position == GTK_ARROW_RIGHT || (obj)->arrow_position == GTK_ARROW_DOWN)
#define XFCE_TRAY_WIDGET_GET_ORIENTATION(obj) (XFCE_TRAY_WIDGET_IS_HORIZONTAL (obj) ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL)
#define XFCE_TRAY_WIDGET_SWAP_INT(x,y)        G_STMT_START{ gint __v = x; x = y; y = __v; }G_STMT_END



/* prototypes */
static void     xfce_tray_widget_class_init         (XfceTrayWidgetClass *klass);
static void     xfce_tray_widget_init               (XfceTrayWidget      *tray);
static void     xfce_tray_widget_finalize           (GObject             *object);
static void     xfce_tray_widget_size_request       (GtkWidget           *widget, 
                                                     GtkRequisition      *requisition);
static void     xfce_tray_widget_size_allocate      (GtkWidget           *widget, 
                                                     GtkAllocation       *allocation);
static void     xfce_tray_widget_style_set          (GtkWidget           *widget,
                                                     GtkStyle            *previous_style);
static void     xfce_tray_widget_map                (GtkWidget           *widget);
static gint     xfce_tray_widget_expose_event       (GtkWidget           *widget, 
                                                     GdkEventExpose      *event);
static void     xfce_tray_widget_button_set_arrow   (XfceTrayWidget      *tray);
static void     xfce_tray_widget_button_clicked     (GtkToggleButton     *button, 
                                                     XfceTrayWidget      *tray);
static gboolean xfce_tray_widget_button_press_event (GtkWidget           *widget,
                                                     GdkEventButton      *event,
                                                     GtkWidget           *tray);
static gint     xfce_tray_widget_compare_function   (gconstpointer        a, 
                                                     gconstpointer        b);
static void     xfce_tray_widget_icon_added         (XfceTrayManager     *manager, 
                                                     GtkWidget           *icon,
                                                     XfceTrayWidget      *tray);
static void     xfce_tray_widget_icon_removed       (XfceTrayManager     *manager, 
                                                     GtkWidget           *icon, 
                                                     XfceTrayWidget      *tray);



struct _XfceTrayWidgetClass
{
    GtkContainerClass __parent__;
};

struct _XfceTrayWidget
{
    GtkContainer __parent__;
    
    /* tray manager of this tray */
    XfceTrayManager  *manager;
    
    /* arrow toggle button */
    GtkWidget        *button;
    
    /* all the icons packed in this box */
    GSList           *childeren;
    
    /* counters for the icons in the list */
    guint             n_childeren;
    guint             n_hidden_childeren;
    
    /* whether hidden icons are visible */
    guint             all_visible : 1;
    
    /* reqested icon size */
    guint             req_child_size;
    
    /* properties */
    guint             lines;
    GtkArrowType      arrow_position;
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
    gtkwidget_class->style_set = xfce_tray_widget_style_set;
    gtkwidget_class->expose_event = xfce_tray_widget_expose_event;
    gtkwidget_class->map = xfce_tray_widget_map;
    
    gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
    gtkcontainer_class->add = NULL;
    gtkcontainer_class->remove = NULL;
}



static void
xfce_tray_widget_init (XfceTrayWidget *tray)
{
    GTK_WIDGET_SET_FLAGS (tray, GTK_NO_WINDOW);
    
    /* initialize */
    tray->childeren = NULL;
    tray->button = NULL;
    tray->manager = NULL;
    
    tray->n_childeren = 0;
    tray->n_hidden_childeren = 0;
    
    tray->all_visible = FALSE;
    tray->req_child_size = 1;
    
    tray->lines = 1;
    tray->arrow_position = GTK_ARROW_LEFT; 
}



static void
xfce_tray_widget_finalize (GObject *object)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (object);
    
    /* free the child list */
    g_slist_free (tray->childeren);
    
    /* release the manager */
    g_object_unref (G_OBJECT (tray->manager));
    
    G_OBJECT_CLASS (xfce_tray_widget_parent_class)->finalize (object);
}



static void
xfce_tray_widget_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (widget);
    gint            width, height, size;
    gint            req_child_size, n_childeren;
    gint            columns;
    
    /* get the requested sizes */
    gtk_widget_get_size_request (widget, &width, &height);
    
    /* get the size we can not change */
    size = (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray) ? height : width);
    
    /* calculate the requested child size */
    req_child_size = (size - (XFCE_TRAY_WIDGET_SPACING * (tray->lines - 1))) / tray->lines;
    
    /* save the requested child size */
    tray->req_child_size = req_child_size;
    
    /* number of icons in the tray */
    n_childeren = tray->n_childeren;
    if (!tray->all_visible)
        n_childeren -= tray->n_hidden_childeren;
    
    /* number of columns in the tray */
    columns = n_childeren / tray->lines;
    if (n_childeren > (columns * tray->lines))
        columns++;
        
    /* calculate the tray size we can set */
    size = (req_child_size * columns) + (XFCE_TRAY_WIDGET_SPACING * MAX (columns - 1, 0));
    
    /* add the hidden button when visible */
    if (tray->n_hidden_childeren > 0)
    {
        size += XFCE_TRAY_WIDGET_BUTTON_SIZE;
        
        /* space between the button and the icons */
        if (columns > 0)
            size += XFCE_TRAY_WIDGET_SPACING;
    }
    
    /* set the other size */
    if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
        width = size;
    else
        height = size;
        
    /* set your request width and height */
    requisition->width = width;
    requisition->height = height;
}



static void
xfce_tray_widget_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (widget);
    GSList         *li;
    GtkWidget      *child;
    GtkAllocation   child_allocation;
    gint            n = 0, i = 0;
    gint            x_offset = 0;
    
    /* set the widget allocation */
    widget->allocation = *allocation;
    
    /* allocation for the arrow button */
    if (tray->n_hidden_childeren > 0)
    {
        /* set the coordinates and default size */
        child_allocation.x = allocation->x;
        child_allocation.y = allocation->y;
        child_allocation.width = child_allocation.height = XFCE_TRAY_WIDGET_BUTTON_SIZE;
        
        /* set the offset for the icons */
        x_offset = XFCE_TRAY_WIDGET_BUTTON_SIZE + XFCE_TRAY_WIDGET_SPACING;
        
        /* position the button on the other side of the tray */
        if (XFCE_TRAY_WIDGET_IS_SOUTH_EAST (tray))
        {
            /* set the coordinates to the other side of the tray */
            if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
                child_allocation.x += allocation->width - XFCE_TRAY_WIDGET_BUTTON_SIZE;
            else
                child_allocation.y += allocation->height - XFCE_TRAY_WIDGET_BUTTON_SIZE;
        }
        
        /* set the width or height to the allocated size */
        if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
            child_allocation.height = allocation->height;
        else
            child_allocation.width = allocation->width;
        
        /* position the arrow button */
        gtk_widget_size_allocate (tray->button, &child_allocation);
    }
    
    /* position all the icons */
    for (li = tray->childeren; li != NULL; li = li->next, i++)
    {
        /* get the child */
        child = li->data;
        
        /* handle hidden icons */
        if (!tray->all_visible && i < tray->n_hidden_childeren)
        {
            /* put the widget offscreen */
            child_allocation.x = child_allocation.y = XFCE_TRAY_WIDGET_OFFSCREEN;
        }
        else
        {
            /* position of the child on the tray */
            child_allocation.x = (tray->req_child_size + XFCE_TRAY_WIDGET_SPACING) * (n / tray->lines) + x_offset;
            child_allocation.y = (tray->req_child_size + XFCE_TRAY_WIDGET_SPACING) * (n % tray->lines);
            
            /* increase counter */
            n++;
            
            /* swap coordinates on a vertical panel */
            if (!XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
                XFCE_TRAY_WIDGET_SWAP_INT (child_allocation.x, child_allocation.y);
            
            /* invert the icon order if the arrow button position is right or down */
            if (tray->arrow_position == GTK_ARROW_RIGHT)
                child_allocation.x = allocation->width - child_allocation.x - tray->req_child_size;
            else if (tray->arrow_position == GTK_ARROW_DOWN)
                child_allocation.y = allocation->height - child_allocation.y - tray->req_child_size;
            
            /* add the tray coordinates */
            child_allocation.x += allocation->x + XFCE_TRAY_WIDGET_SPACING;
            child_allocation.y += allocation->y + XFCE_TRAY_WIDGET_SPACING;
        }        
        
        /* set the child width and height */
        child_allocation.width = child_allocation.height = tray->req_child_size - 2 * XFCE_TRAY_WIDGET_SPACING;
      
        /* send the child allocation */
        gtk_widget_size_allocate (child, &child_allocation);
    }
}



static gint
xfce_tray_widget_expose_event (GtkWidget      *widget,
                               GdkEventExpose *event)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (widget);
    
    /* expose the button, because it doesn't have its own window */
    gtk_container_propagate_expose (GTK_CONTAINER (widget), tray->button, event);
    
    return FALSE;
}



static void
xfce_tray_widget_style_set (GtkWidget *widget,
                            GtkStyle  *previous_style)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (widget);
    GSList         *li;
    
    /* set the button style */
    if (tray->button)
        gtk_widget_set_style (tray->button, widget->style);
        
    /* send the style to all the childeren */
    for (li = tray->childeren; li != NULL; li = li->next)
        gtk_widget_set_style (GTK_WIDGET (li->data), widget->style);
        
    /* invoke the parent */
    GTK_WIDGET_CLASS (xfce_tray_widget_parent_class)->style_set (widget, previous_style);
}



static void
xfce_tray_widget_map (GtkWidget *widget)
{
    XfceTrayWidget *tray = XFCE_TRAY_WIDGET (widget);
    
    /* we've been mapped */
    GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
    
    /* create the arrow button (needs a mapped tray before the parent is set) */
    tray->button = xfce_arrow_button_new (tray->arrow_position);
    GTK_WIDGET_UNSET_FLAGS (tray->button, GTK_CAN_DEFAULT | GTK_CAN_FOCUS);
    gtk_button_set_focus_on_click (GTK_BUTTON (tray->button), FALSE);
    g_signal_connect (G_OBJECT (tray->button), "clicked", G_CALLBACK (xfce_tray_widget_button_clicked), tray);
    g_signal_connect (G_OBJECT (tray->button), "button-press-event", G_CALLBACK (xfce_tray_widget_button_press_event), tray);
    gtk_widget_set_parent (tray->button, widget);
    gtk_widget_show (tray->button);
}



static void
xfce_tray_widget_button_set_arrow (XfceTrayWidget *tray)
{
    GtkArrowType arrow_type;
    
    /* get the origional arrow type */
    arrow_type = tray->arrow_position;
    
    /* invert the arrow direction when the button is toggled */
    if (tray->all_visible)
    {
        if (XFCE_TRAY_WIDGET_IS_HORIZONTAL (tray))
            arrow_type = (arrow_type == GTK_ARROW_LEFT ? GTK_ARROW_RIGHT : GTK_ARROW_LEFT);
        else
            arrow_type = (arrow_type == GTK_ARROW_UP ? GTK_ARROW_DOWN : GTK_ARROW_UP);
    }
    
    /* set the arrow type */
    xfce_arrow_button_set_arrow_type (XFCE_ARROW_BUTTON (tray->button), arrow_type);
}



static void 
xfce_tray_widget_button_clicked (GtkToggleButton *button, 
                                 XfceTrayWidget  *tray)
{
    /* set the new visible state */
    tray->all_visible = gtk_toggle_button_get_active (button);
    
    /* set the button arrow */
    xfce_tray_widget_button_set_arrow (tray);
    
    /* update the tray */
    gtk_widget_queue_resize (GTK_WIDGET (tray));
}



static gboolean
xfce_tray_widget_button_press_event (GtkWidget      *widget,
                                     GdkEventButton *event,
                                     GtkWidget      *tray)
{
    /* send the event to the tray for poping up the menu */
    gtk_widget_event (tray, (GdkEvent *)event);
    
    return FALSE;
}



static gint
xfce_tray_widget_compare_function (gconstpointer a,
                                   gconstpointer b)
{
    gboolean     a_hidden, b_hidden;
    const gchar *a_wmname, *b_wmname;
    
    /* get hidden state */
    a_hidden = xfce_tray_manager_application_get_hidden (GTK_WIDGET (a));
    b_hidden = xfce_tray_manager_application_get_hidden (GTK_WIDGET (b));
    
    /* sort hidden icons before visible ones */
    if (a_hidden != b_hidden)
        return (a_hidden ? -1 : 1);
    
    /* get the window names */
    a_wmname = xfce_tray_manager_application_get_name (GTK_WIDGET (a));
    b_wmname = xfce_tray_manager_application_get_name (GTK_WIDGET (b));
    
    /* return when one of them has no name */
    if (G_UNLIKELY (!a_wmname || !b_wmname))
        return (a_wmname == b_wmname ? 0 : (!a_wmname ? -1 : 1));
        
    return strcmp (a_wmname, b_wmname);
}



static void
xfce_tray_widget_icon_added (XfceTrayManager *manager,
                             GtkWidget       *icon,
                             XfceTrayWidget  *tray)
{
    gboolean hidden;
    
    g_return_if_fail (XFCE_IS_TRAY_MANAGER (manager));
    g_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    g_return_if_fail (GTK_IS_WIDGET (icon));
    
    /* add the icon to the list */
    tray->childeren = g_slist_insert_sorted (tray->childeren, icon, xfce_tray_widget_compare_function);
    
    /* increase counter */
    tray->n_childeren++;
    
    /* whether this icon could be hidden */
    hidden = xfce_tray_manager_application_get_hidden (icon);
    
    /* increase hidden counter */
    if (hidden)
        tray->n_hidden_childeren++;
        
    /* show the icon */
    gtk_widget_show (icon);
    
    /* set the parent window */
    gtk_widget_set_parent (icon, GTK_WIDGET (tray));
}



static void
xfce_tray_widget_icon_removed (XfceTrayManager *manager,
                               GtkWidget       *icon,
                               XfceTrayWidget  *tray)
{
    g_return_if_fail (XFCE_IS_TRAY_MANAGER (manager));
    g_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    g_return_if_fail (GTK_IS_WIDGET (icon));
    
    /* decrease counter */
    tray->n_childeren--;
    
    /* remove the child from the list */
    tray->childeren = g_slist_remove (tray->childeren, icon);
    
    /* handle hidden icons */
    if (xfce_tray_manager_application_get_hidden (icon))
    {
        /* decrease hidden counter */
        tray->n_hidden_childeren--;
        
        /* collapse the hidden button */
        if (tray->n_hidden_childeren == 0)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tray->button), FALSE);
    }
    
    /* update the tray */
    gtk_widget_queue_resize (GTK_WIDGET (tray));
}



GtkWidget *
xfce_tray_widget_new_for_screen (GdkScreen     *screen,
                                 GtkArrowType   arrow_position,
                                 GError       **error)
{
    XfceTrayWidget  *tray = NULL;
    XfceTrayManager *manager;
    
    g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
    g_return_val_if_fail (error == NULL || *error == NULL, NULL);
    
    /* new tray manager */
    manager = xfce_tray_manager_new ();
        
    /* register the manager for this screen */
    if (G_LIKELY (xfce_tray_manager_register (manager, screen, error) == TRUE))
    {
        /* create a tray */
        tray = g_object_new (XFCE_TYPE_TRAY_WIDGET, NULL);
        
        /* set the manager */
        tray->manager = manager;
        
        /* set the orientations */
        tray->arrow_position = arrow_position;
        xfce_tray_manager_set_orientation (manager, XFCE_TRAY_WIDGET_GET_ORIENTATION (tray));
        
        /* manager signals */
        g_signal_connect (G_OBJECT (manager), "tray-icon-added", G_CALLBACK (xfce_tray_widget_icon_added), tray);
        g_signal_connect (G_OBJECT (manager), "tray-icon-removed", G_CALLBACK (xfce_tray_widget_icon_removed), tray);
        //g_signal_connect (G_OBJECT (manager), "tray-message-sent", G_CALLBACK (xfce_tray_widget_message_sent), tray);
        //g_signal_connect (G_OBJECT (manager), "tray-message-cancelled", G_CALLBACK (xfce_tray_widget_message_cancelled), tray);
        //g_signal_connect (G_OBJECT (manager), "tray-lost-selection", G_CALLBACK (xfce_tray_widget_lost_selection), tray);
    }
    else
    {
        /* release the manager */
        g_object_unref (G_OBJECT (manager));
    }
    
    return GTK_WIDGET (tray);
}



void
xfce_tray_widget_sort (XfceTrayWidget *tray)
{
    GSList *li;
    guint   n_hidden_childeren = 0;
    
    /* sort the list */
    tray->childeren = g_slist_sort (tray->childeren, xfce_tray_widget_compare_function);
    
    /* count the number of hidden items again */
    for (li = tray->childeren; li != NULL; li = li->next)
    {
        /* increase counter or break (hidden items are in the beginning of the list) */
        if (xfce_tray_manager_application_get_hidden (GTK_WIDGET (li->data)))
            n_hidden_childeren++;
        else
            break;
    }
    
    /* update if changed */
    if (tray->n_hidden_childeren != n_hidden_childeren)
    {
        /* collapse the button if there are no hidden childeren */
        if (n_hidden_childeren == 0)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tray->button), FALSE);
        
        /* set new value */
        tray->n_hidden_childeren = n_hidden_childeren;
        
        /* update the tray */
        gtk_widget_queue_resize (GTK_WIDGET (tray));        
    }
}



XfceTrayManager *
xfce_tray_widget_get_manager (XfceTrayWidget *tray)
{
    g_return_val_if_fail (XFCE_IS_TRAY_WIDGET (tray), NULL);
    
    return tray->manager;
}



void 
xfce_tray_widget_set_arrow_position (XfceTrayWidget     *tray, 
                                     GtkArrowType  arrow_position)
{
    g_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    
    if (G_LIKELY (tray->arrow_position != arrow_position))
    {
        /* set property */
        tray->arrow_position = arrow_position;
        
        /* set orientation of the manager */
        xfce_tray_manager_set_orientation (tray->manager, XFCE_TRAY_WIDGET_GET_ORIENTATION (tray));
        
        /* set the arrow on the button */
        xfce_tray_widget_button_set_arrow (tray);
        
        /* update the tray */
        gtk_widget_queue_resize (GTK_WIDGET (tray));
    }
}


                                             
void         
xfce_tray_widget_set_lines (XfceTrayWidget *tray,
                            guint           lines)
{
    g_return_if_fail (XFCE_IS_TRAY_WIDGET (tray));
    g_return_if_fail (lines >= 1);
    
    /* set property */
    tray->lines = lines;
}
