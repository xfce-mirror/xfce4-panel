/* $Id$
 *
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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
 *
 * Based on the ThunarPathEntry class, which is part of the Thunar file manager,
 * Copyright (c) 2004-2006 Benedikt Meurer <benny@xfce.org>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "frap-icon-entry.h"



/* the margin around the icon */
#define FRAP_ICON_ENTRY_ICON_MARGIN (2)



#define FRAP_ICON_ENTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FRAP_TYPE_ICON_ENTRY, FrapIconEntryPrivate))



/* Property identifiers */
enum
{
  PROP_0,
  PROP_SIZE,
  PROP_STOCK_ID,
};



static void     frap_icon_entry_class_init          (FrapIconEntryClass *klass);
static void     frap_icon_entry_init                (FrapIconEntry      *icon_entry);
static void     frap_icon_entry_finalize            (GObject            *object);
static void     frap_icon_entry_get_property        (GObject            *object,
                                                     guint               prop_id,
                                                     GValue             *value,
                                                     GParamSpec         *pspec);
static void     frap_icon_entry_set_property        (GObject            *object,
                                                     guint               prop_id,
                                                     const GValue       *value,
                                                     GParamSpec         *pspec);
static void     frap_icon_entry_size_request        (GtkWidget          *widget,
                                                     GtkRequisition     *requisition);
static void     frap_icon_entry_size_allocate       (GtkWidget          *widget,
                                                     GtkAllocation      *allocation);
static void     frap_icon_entry_realize             (GtkWidget          *widget);
static void     frap_icon_entry_unrealize           (GtkWidget          *widget);
static gboolean frap_icon_entry_expose_event        (GtkWidget          *widget,
                                                     GdkEventExpose     *event);
static void     frap_icon_entry_get_borders         (FrapIconEntry      *icon_entry,
                                                     gint               *xborder_return,
                                                     gint               *yborder_return);
static void     frap_icon_entry_get_text_area_size  (FrapIconEntry      *icon_entry,
                                                     gint               *x_return,
                                                     gint               *y_return,
                                                     gint               *width_return,
                                                     gint               *height_return);



struct _FrapIconEntryPrivate
{
  GtkIconSize size;
  gchar      *stock_id;
};



G_DEFINE_TYPE (FrapIconEntry,
               frap_icon_entry,
               GTK_TYPE_ENTRY);



static void
frap_icon_entry_class_init (FrapIconEntryClass *klass)
{
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  /* add the private instance data */
  g_type_class_add_private (klass, sizeof (FrapIconEntryPrivate));

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = frap_icon_entry_finalize;
  gobject_class->get_property = frap_icon_entry_get_property;
  gobject_class->set_property = frap_icon_entry_set_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->size_request = frap_icon_entry_size_request;
  gtkwidget_class->size_allocate = frap_icon_entry_size_allocate;
  gtkwidget_class->realize = frap_icon_entry_realize;
  gtkwidget_class->unrealize = frap_icon_entry_unrealize;
  gtkwidget_class->expose_event = frap_icon_entry_expose_event;

  /**
   * FrapIconEntry:size:
   *
   * The #GtkIconSize for the icon in this entry.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_enum ("size",
                                                      "Size",
                                                      "The size of the icon in the entry",
                                                      GTK_TYPE_ICON_SIZE,
                                                      GTK_ICON_SIZE_MENU,
                                                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  /**
   * FrapIconEntry:stock-id:
   *
   * The stock-id of the icon to render, or %NULL if no
   * icon should be rendered.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_STOCK_ID,
                                   g_param_spec_string ("stock-id",
                                                        "Stock Id",
                                                        "The stock-id of the icon in the entry",
                                                        NULL,
                                                        G_PARAM_READWRITE));
}



static void
frap_icon_entry_init (FrapIconEntry *icon_entry)
{
  icon_entry->priv = FRAP_ICON_ENTRY_GET_PRIVATE (icon_entry);
}



static void
frap_icon_entry_finalize (GObject *object)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (object);

  /* release the stock-id if any */
  g_free (icon_entry->priv->stock_id);

  (*G_OBJECT_CLASS (frap_icon_entry_parent_class)->finalize) (object);
}



static void
frap_icon_entry_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (object);

  switch (prop_id)
    {
    case PROP_SIZE:
      g_value_set_enum (value, frap_icon_entry_get_size (icon_entry));
      break;

    case PROP_STOCK_ID:
      g_value_set_string (value, frap_icon_entry_get_stock_id (icon_entry));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
frap_icon_entry_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (object);

  switch (prop_id)
    {
    case PROP_SIZE:
      frap_icon_entry_set_size (icon_entry, g_value_get_enum (value));
      break;

    case PROP_STOCK_ID:
      frap_icon_entry_set_stock_id (icon_entry, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
frap_icon_entry_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (widget);
  gint           text_height;
  gint           icon_height;
  gint           icon_width;
  gint           xborder;
  gint           yborder;

  /* determine the size request of the text entry */
  (*GTK_WIDGET_CLASS (frap_icon_entry_parent_class)->size_request) (widget, requisition);

  /* lookup the icon dimensions */
  gtk_icon_size_lookup (icon_entry->priv->size, &icon_width, &icon_height);

  /* determine the text area size */
  frap_icon_entry_get_text_area_size (icon_entry, &xborder, &yborder, NULL, &text_height);

  /* adjust the requisition */
  requisition->width += icon_width + xborder + 2 * FRAP_ICON_ENTRY_ICON_MARGIN;
  requisition->height = 2 * yborder + MAX (icon_height + 2 * FRAP_ICON_ENTRY_ICON_MARGIN, text_height);
}



static void
frap_icon_entry_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (widget);
  GtkAllocation  text_allocation;
  GtkAllocation  icon_allocation;
  gint           text_height;
  gint           icon_height;
  gint           icon_width;
  gint           xborder;
  gint           yborder;

  /* lookup the icon dimensions */
  gtk_icon_size_lookup (icon_entry->priv->size, &icon_width, &icon_height);

  /* determine the text area size */
  frap_icon_entry_get_text_area_size (icon_entry, &xborder, &yborder, NULL, &text_height);

  /* calculate the base text allocation */
  text_allocation.y = yborder;
  text_allocation.width = allocation->width - icon_width - 2 * (xborder + FRAP_ICON_ENTRY_ICON_MARGIN);
  text_allocation.height = text_height;

  /* calculate the base icon allocation */
  icon_allocation.y = yborder;
  icon_allocation.width = icon_width + 2 * FRAP_ICON_ENTRY_ICON_MARGIN;
  icon_allocation.height = MAX (icon_height + 2 * FRAP_ICON_ENTRY_ICON_MARGIN, text_height);

  /* the x offset depends on the text direction */
  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      text_allocation.x = xborder;
      icon_allocation.x = allocation->width - icon_allocation.width - xborder - 2 * FRAP_ICON_ENTRY_ICON_MARGIN;
    }
  else
    {
      icon_allocation.x = xborder;
      text_allocation.x = allocation->width - text_allocation.width - xborder;
    }

  /* setup the text area */
  (*GTK_WIDGET_CLASS (frap_icon_entry_parent_class)->size_allocate) (widget, allocation);

  /* adjust the dimensions/positions of the areas */
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move_resize (GTK_ENTRY (icon_entry)->text_area,
                              text_allocation.x,
                              text_allocation.y,
                              text_allocation.width,
                              text_allocation.height);

      gdk_window_move_resize (icon_entry->icon_area,
                              icon_allocation.x,
                              icon_allocation.y,
                              icon_allocation.width,
                              icon_allocation.height);
    }
}



static void
frap_icon_entry_realize (GtkWidget *widget)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (widget);
  GdkWindowAttr  attributes;
  gint           attributes_mask;
  gint           text_height;
  gint           icon_height;
  gint           icon_width;
  gint           spacing;

  /* let GtkEntry handle the realization of the text area */
  (*GTK_WIDGET_CLASS (frap_icon_entry_parent_class)->realize) (widget);

  /* lookup the icon dimensions */
  gtk_icon_size_lookup (icon_entry->priv->size, &icon_width, &icon_height);

  /* determine the spacing for the icon area */
  frap_icon_entry_get_text_area_size (icon_entry, NULL, NULL, NULL, &text_height);
  spacing = widget->requisition.height - text_height;

  /* setup the icon_area attributes */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget)
                        | GDK_EXPOSURE_MASK;
  attributes.x = widget->allocation.x + widget->allocation.width - icon_width - spacing;
  attributes.y = widget->allocation.y + (widget->allocation.height - widget->requisition.height) / 2;
  attributes.width = icon_width + spacing;
  attributes.height = widget->requisition.height;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  /* setup the window for the icon area */
  icon_entry->icon_area = gdk_window_new (widget->window, &attributes, attributes_mask);
  gdk_window_set_user_data (icon_entry->icon_area, widget);
  gdk_window_set_background (icon_entry->icon_area, &widget->style->base[GTK_WIDGET_STATE (widget)]);
  gdk_window_show (icon_entry->icon_area);

  /* need to resize the text_area afterwards */
  gtk_widget_queue_resize (widget);
}



static void
frap_icon_entry_unrealize (GtkWidget *widget)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (widget);

  /* destroy the icon_area */
  gdk_window_set_user_data (icon_entry->icon_area, NULL);
  gdk_window_destroy (icon_entry->icon_area);
  icon_entry->icon_area = NULL;

  (*GTK_WIDGET_CLASS (frap_icon_entry_parent_class)->unrealize) (widget);
}



static gboolean
frap_icon_entry_expose_event (GtkWidget      *widget,
                              GdkEventExpose *event)
{
  FrapIconEntry *icon_entry = FRAP_ICON_ENTRY (widget);
  GdkPixbuf     *icon;
  gint           icon_height;
  gint           icon_width;
  gint           height;
  gint           width;

  /* check if the expose is on the icon_area */
  if (event->window == icon_entry->icon_area)
    {
      /* determine the size of the icon_area */
      gdk_drawable_get_size (GDK_DRAWABLE (icon_entry->icon_area), &width, &height);

      /* clear the icon area */
      gtk_paint_flat_box (widget->style, icon_entry->icon_area,
                          GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                          NULL, widget, "entry_bg", 0, 0, width, height);

      /* check if a stock-id is set */
      if (G_LIKELY (icon_entry->priv->stock_id != NULL))
        {
          /* try to render the icon for the stock-id */
          icon = gtk_widget_render_icon (widget, icon_entry->priv->stock_id, icon_entry->priv->size, "icon_entry");
          if (G_LIKELY (icon != NULL))
            {
              /* determine the dimensions of the icon */
              icon_width = gdk_pixbuf_get_width (icon);
              icon_height = gdk_pixbuf_get_height (icon);

              /* draw the icon */
              gdk_draw_pixbuf (icon_entry->icon_area,
                               widget->style->black_gc,
                               icon, 0, 0,
                               (width - icon_width) / 2,
                               (height - icon_height) / 2,
                               icon_width, icon_height,
                               GDK_RGB_DITHER_NORMAL, 0, 0);

              /* release the icon */
              g_object_unref (G_OBJECT (icon));
            }
        }
    }
  else
    {
      /* the expose is probably for the text_area */
      return (*GTK_WIDGET_CLASS (frap_icon_entry_parent_class)->expose_event) (widget, event);
    }

  return TRUE;
}



static void
frap_icon_entry_get_borders (FrapIconEntry *icon_entry,
                             gint          *xborder_return,
                             gint          *yborder_return)
{
	gboolean interior_focus;
	gint     focus_width;

	gtk_widget_style_get (GTK_WIDGET (icon_entry),
                        "focus-line-width", &focus_width,
                        "interior-focus", &interior_focus,
                        NULL);

	if (gtk_entry_get_has_frame (GTK_ENTRY (icon_entry)))
    {
		  *xborder_return = GTK_WIDGET (icon_entry)->style->xthickness;
  		*yborder_return = GTK_WIDGET (icon_entry)->style->ythickness;
	  }
	else
	  {
  		*xborder_return = 0;
	  	*yborder_return = 0;
  	}

	if (!interior_focus)
	  {
  		*xborder_return += focus_width;
	  	*yborder_return += focus_width;
  	}
}



static void
frap_icon_entry_get_text_area_size (FrapIconEntry *icon_entry,
                                    gint          *x_return,
                                    gint          *y_return,
                                    gint          *width_return,
                                    gint          *height_return)
{
	GtkRequisition requisition;
	gint           xborder;
  gint           yborder;

	gtk_widget_get_child_requisition (GTK_WIDGET (icon_entry), &requisition);

  frap_icon_entry_get_borders (icon_entry, &xborder, &yborder);

	if (x_return != NULL) *x_return = xborder;
	if (y_return != NULL) *y_return = yborder;
	if (width_return  != NULL) *width_return  = GTK_WIDGET (icon_entry)->allocation.width - xborder * 2;
	if (height_return != NULL) *height_return = requisition.height - yborder * 2;
}



/**
 * frap_icon_entry_new:
 *
 * Allocates a new #FrapIconEntry instance.
 *
 * Return value: the newly allocated #FrapIconEntry.
 **/
GtkWidget*
frap_icon_entry_new (void)
{
  return g_object_new (FRAP_TYPE_ICON_ENTRY, NULL);
}



/**
 * frap_icon_entry_get_size:
 * @icon_entry : a #FrapIconEntry.
 *
 * Returns the #GtkIconSize that is currently set for the
 * @icon_entry.
 *
 * Return value: the icon size for the @icon_entry.
 **/
GtkIconSize
frap_icon_entry_get_size (FrapIconEntry *icon_entry)
{
  g_return_val_if_fail (FRAP_IS_ICON_ENTRY (icon_entry), GTK_ICON_SIZE_INVALID);
  return icon_entry->priv->size;
}



/**
 * frap_icon_entry_set_size:
 * @icon_entry : a #FrapIconEntry.
 * @size       : the new icon size for the @icon_entry.
 *
 * Sets the size at which the icon of the @icon_entry is drawn
 * to @size.
 **/
void
frap_icon_entry_set_size (FrapIconEntry *icon_entry,
                          GtkIconSize    size)
{
  g_return_if_fail (FRAP_IS_ICON_ENTRY (icon_entry));

  /* check if we have a new setting */
  if (G_LIKELY (icon_entry->priv->size != size))
    {
      /* apply the new setting */
      icon_entry->priv->size = size;

      /* notify listeners */
      g_object_notify (G_OBJECT (icon_entry), "size");

      /* schedule a resize */
      gtk_widget_queue_resize (GTK_WIDGET (icon_entry));
    }
}



/**
 * frap_icon_entry_get_stock_id:
 * @icon_entry : a #FrapIconEntry.
 *
 * Returns the stock-id that is currently set for the @icon_entry
 * or %NULL if no stock-id is set.
 *
 * Return value: the stock-id for the @icon_entry.
 **/
const gchar*
frap_icon_entry_get_stock_id (FrapIconEntry *icon_entry)
{
  g_return_val_if_fail (FRAP_IS_ICON_ENTRY (icon_entry), NULL);
  return icon_entry->priv->stock_id;
}



/**
 * frap_icon_entry_set_stock_id:
 * @icon_entry : a #FrapIconEntry.
 * @stock_id   : the new stock-id or %NULL to reset.
 *
 * Sets the stock-id of the icon to be drawn in the @icon_entry to
 * @stock_id.
 **/
void
frap_icon_entry_set_stock_id (FrapIconEntry *icon_entry,
                              const gchar   *stock_id)
{
  g_return_if_fail (FRAP_IS_ICON_ENTRY (icon_entry));

  /* release the previous stock-id */
  g_free (icon_entry->priv->stock_id);

  /* apply the new stock-id */
  icon_entry->priv->stock_id = g_strdup (stock_id);

  /* notify listeners */
  g_object_notify (G_OBJECT (icon_entry), "stock-id");

  /* schedule a resize */
  gtk_widget_queue_resize (GTK_WIDGET (icon_entry));
}


