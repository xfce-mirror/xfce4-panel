#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "xfce.h"
#include "popup.h"
#include "callbacks.h"
#include "item.h"
#include "icons.h"

#define MAXITEMS 10

/*****************************************************************************/

PanelPopup *
panel_popup_new (void)
{
  PanelPopup *pp = g_new (PanelPopup, 1);

  pp->button = NULL;
  pp->up = get_pixbuf_from_id (UP_ICON);
  pp->down = get_pixbuf_from_id (DOWN_ICON);
  pp->arrow_image= gtk_image_new ();
  pp->hgroup = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  pp->window = NULL;
  pp->frame = NULL;
  pp->vbox = NULL;
  pp->addtomenu_item = NULL;
  pp->separator = NULL;
  pp->items = NULL;
  pp->tearoff_button = NULL;
  pp->detached = FALSE;

  return pp;
}

void
create_panel_popup (PanelPopup * pp)
{
  GtkWidget *sep;
  GList *li;
  int i;

  /* the button */
  pp->button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (pp->button), GTK_RELIEF_NONE);
  gtk_widget_set_size_request (pp->button, 
			       MEDIUM_PANEL_ICONS + 4,
			       MEDIUM_TOPHEIGHT);

  gtk_image_set_from_pixbuf (GTK_IMAGE (pp->arrow_image), pp->up);
  gtk_container_add (GTK_CONTAINER (pp->button), pp->arrow_image);
  
  gtk_widget_show_all (pp->button);
  
  /* the menu */
  pp->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated (GTK_WINDOW (pp->window), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (pp->window), FALSE);
  gtk_window_stick (GTK_WINDOW (pp->window));
  /* don't care aboutdecorations when calculating position */
  gtk_window_set_gravity (GTK_WINDOW (pp->window), GDK_GRAVITY_STATIC);
  
  set_transient_for_panel (pp->window);
  
  pp->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pp->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (pp->window), pp->frame);
  
  pp->vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (pp->frame), pp->vbox);
  
  pp->addtomenu_item = panel_item_new (pp);
  create_addtomenu_item (pp->addtomenu_item);
  gtk_size_group_add_widget (pp->hgroup, pp->addtomenu_item->image);
  gtk_box_pack_start (GTK_BOX (pp->vbox),
		      pp->addtomenu_item->button, FALSE, FALSE, 0);
		      
  pp->separator = gtk_hseparator_new ();
  gtk_size_group_add_widget (pp->hgroup, pp->separator);
  gtk_box_pack_start (GTK_BOX (pp->vbox), pp->separator, FALSE, FALSE, 0);

  for (i = 0, li = pp->items; li && li->data; li = li->next, i++)
    {
      PanelItem *item = (PanelItem *) li->data;
      item->pos = i;
      create_panel_item (item);
      gtk_size_group_add_widget (pp->hgroup, item->image);
      gtk_box_pack_start (GTK_BOX (pp->vbox), item->button, TRUE, TRUE, 0);
    }

  pp->tearoff_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (pp->tearoff_button), GTK_RELIEF_NONE);
  gtk_box_pack_start (GTK_BOX (pp->vbox),
		      pp->tearoff_button, FALSE, TRUE, 0);
  sep = gtk_hseparator_new ();
  gtk_container_add (GTK_CONTAINER (pp->tearoff_button), sep);

  /* signals */
  g_signal_connect (pp->button, "clicked", G_CALLBACK (toggle_popup), pp);
    
      
  g_signal_connect (pp->tearoff_button, "clicked", 
		    G_CALLBACK (tearoff_popup), pp);
    
  g_signal_connect (pp->window, "delete-event", 
		    G_CALLBACK (delete_popup), pp);
    
  /* apparently this is necessary to make the popup show correctly */  
  pp->detached = FALSE;
  
  gtk_widget_show_all (pp->frame);
}

void
panel_popup_set_size (PanelPopup * pp, int size)
{
  gtk_widget_set_size_request (pp->button, 
			       icon_size (size) + 4, top_height (size));
}

void
panel_popup_set_style (PanelPopup * pp, int style)
{
  GList *li;
  if (style == OLD_STYLE)
    {
      gtk_button_set_relief (GTK_BUTTON (pp->button), GTK_RELIEF_NORMAL);
      gtk_widget_set_name (pp->button, "gxfce_color1");
      gtk_frame_set_shadow_type (GTK_FRAME (pp->addtomenu_item->frame), 
				 GTK_SHADOW_IN);
    }
  else
    {
      gtk_button_set_relief (GTK_BUTTON (pp->button), GTK_RELIEF_NONE);
      gtk_widget_set_name (pp->button, "gxfce_popup_button");
      gtk_frame_set_shadow_type (GTK_FRAME (pp->addtomenu_item->frame), 
				 GTK_SHADOW_NONE);
    }

  for (li = pp->items; li && li->data; li = li->next)
    {
      PanelItem *pi = (PanelItem *) li->data;
      panel_item_set_style (pi, style);
    }
}

void
panel_popup_set_popup_size (PanelPopup * pp, int size)
{
  GList *li;
  int s = popup_icon_size (size);
  
  gtk_widget_set_size_request (pp->addtomenu_item->frame, 
			       s, s);
  
  gtk_widget_set_size_request (pp->addtomenu_item->button, 
			       -1, s + 4);
  
  for (li = pp->items; li && li->data; li = li->next)
    {
      PanelItem *pi = (PanelItem *) li->data;
      panel_item_set_popup_size (pi, size);
    }
}

void
panel_popup_free (PanelPopup * pp)
{
  /* only items contain non-gtk elements to be freed */
  GList *li;
  for (li = pp->items; li && li->data; li = li->next)
    {
      PanelItem *pi = (PanelItem *) li->data;
      panel_item_free (pi);
    }
  
  g_free (pp);
}

