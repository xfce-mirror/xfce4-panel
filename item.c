#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "xfce.h"
#include "item.h"
#include "callbacks.h"
#include "popup.h"
#include "icons.h"

/*****************************************************************************/

enum
{
  TARGET_STRING,
  TARGET_ROOTWIN,
  TARGET_URL
};

static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URL},
  {"STRING", 0, TARGET_STRING}
};

static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

/*****************************************************************************/

PanelItem *
panel_item_new (PanelPopup * pp)
{
  PanelItem *pi = g_new (PanelItem, 1);

  pi->pp = pp;
  pi->in_menu = FALSE;
  pi->pos = 0;
  
  pi->caption = NULL;
  pi->command = NULL;
  pi->tooltip = NULL;
  pi->id = UNKNOWN_ICON;
  pi->path = NULL;
  
  pi->button = NULL;
  pi->pb = NULL;
  pi->image = NULL;
  pi->hbox = NULL;
  pi->frame = NULL;
  pi->label = NULL;

  pi->xmlnode = NULL;
  
  return pi;
}

void
create_addtomenu_item (PanelItem * pi)
{
  pi->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (pi->button), GTK_RELIEF_NONE);
  gtk_widget_set_size_request (pi->button, -1, MEDIUM_POPUP_ICONS);

  pi->hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (pi->button), pi->hbox);

  pi->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pi->frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (pi->hbox), pi->frame, FALSE, FALSE, 0);
      
  pi->pb = get_pixbuf_from_id (ADDICON_ICON);
  pi->image = gtk_image_new_from_pixbuf (pi->pb);
  gtk_container_add (GTK_CONTAINER (pi->frame), pi->image);

  panel_set_tooltip (pi->button, _("Add new item"));
  pi->label = gtk_label_new (_("Add icon..."));
  gtk_box_pack_start (GTK_BOX (pi->hbox), pi->label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (pi->label), 0.1, 0.5);
  
  /* signals */
  gtk_drag_dest_set (pi->button, GTK_DEST_DEFAULT_ALL, 
		     target_table, n_targets, GDK_ACTION_COPY);
		     
  g_signal_connect (pi->button, "drag_data_received",
		    G_CALLBACK (addtomenu_item_drop_cb), pi->pp);
		    
  g_signal_connect (pi->button, "clicked",
		    G_CALLBACK (addtomenu_item_click_cb), pi->pp);
}

void
create_panel_item (PanelItem * pi)
{
  GdkPixbuf *pb;

  pi->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (pi->button), GTK_RELIEF_NONE);

  if (pi->id == EXTERN_ICON && pi->path)
    pi->pb = gdk_pixbuf_new_from_file (pi->path, NULL);
  else
    pi->pb = get_pixbuf_from_id (pi->id);

  if (pi->in_menu)
    {
      pi->hbox = gtk_hbox_new (FALSE, 8);
      gtk_container_add (GTK_CONTAINER (pi->button), pi->hbox);
      
      pi->frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (pi->frame), GTK_SHADOW_NONE);
      gtk_box_pack_start (GTK_BOX (pi->hbox), pi->frame, FALSE, FALSE, 0);
      
      pb = gdk_pixbuf_scale_simple (pi->pb,
				    MEDIUM_POPUP_ICONS, 
				    MEDIUM_POPUP_ICONS,
				    GDK_INTERP_BILINEAR);
    
      pi->image = gtk_image_new_from_pixbuf (pb);
      g_object_unref (pb);
      gtk_container_add (GTK_CONTAINER (pi->frame), pi->image);
      
      pi->label = gtk_label_new (pi->caption);
      gtk_box_pack_start (GTK_BOX (pi->hbox), pi->label, FALSE, FALSE, 0);
      gtk_misc_set_alignment (GTK_MISC (pi->label), 0.1, 0.5);
    }
  else
    {
      gtk_container_set_border_width (GTK_CONTAINER (pi->button), 2);
      pb = gdk_pixbuf_scale_simple (pi->pb,
				    MEDIUM_PANEL_ICONS - 4,
				    MEDIUM_PANEL_ICONS - 4, 
				    GDK_INTERP_BILINEAR);
    
      pi->image = gtk_image_new_from_pixbuf (pb);
      g_object_unref (pb);
      gtk_container_add (GTK_CONTAINER (pi->button), pi->image);
    }

  if (pi->tooltip)
    panel_set_tooltip (pi->button, pi->tooltip);

  gtk_widget_show_all (pi->button);

  /* signals */
  if (pi->in_menu)
    g_signal_connect (pi->button, "button-press-event",
		      G_CALLBACK (item_in_menu_press_cb), pi);
  
  if (pi->command)
    {
      g_signal_connect (pi->button, "clicked",
			G_CALLBACK (item_click_cb), pi->command);
      
      if (pi->tooltip)
	panel_set_tooltip (pi->button, pi->tooltip);
      else
	panel_set_tooltip (pi->button, pi->command);
      
      gtk_drag_dest_set (pi->button, GTK_DEST_DEFAULT_ALL, 
			 target_table, n_targets, GDK_ACTION_COPY);
      
      g_signal_connect (pi->button, "drag_data_received",
			G_CALLBACK (item_drop_cb), pi->command);
    }
}

void
panel_item_set_size (PanelItem * pi, int size)
{
  GdkPixbuf *pb;
  int width, height, bw;

  if (pi->in_menu)
    return;
  
  bw = size == SMALL ? 0 : 2;
  width = icon_size (size);
  height = icon_size (size);

  gtk_container_set_border_width (GTK_CONTAINER (pi->button), bw);
  gtk_widget_set_size_request (pi->button, width + 4, height + 4);

  pb = gdk_pixbuf_scale_simple (pi->pb,
				width - 2 * bw,
				height - 2 * bw, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (pi->image), pb);
  g_object_unref (pb);
}

void
panel_item_set_popup_size (PanelItem * pi, int size)
{
  GdkPixbuf *pb;
  int s = popup_icon_size (size);

  if (!pi->in_menu)
    return;
  
  pb = gdk_pixbuf_scale_simple (pi->pb, s - 4, s - 4, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (pi->image), pb);
  g_object_unref (pb);
  
  gtk_widget_set_size_request (pi->frame, s, s);
  gtk_widget_set_size_request (pi->button, -1, s + 4);
}

void
panel_item_set_style (PanelItem * pi, int style)
{
  if (pi->in_menu)
    {
      if (style == OLD_STYLE)
	gtk_frame_set_shadow_type (GTK_FRAME (pi->frame), GTK_SHADOW_IN);
      else
	gtk_frame_set_shadow_type (GTK_FRAME (pi->frame), GTK_SHADOW_NONE);
    }
}

void
panel_item_free (PanelItem * pi)
{
  g_free (pi->command);
  g_free (pi->path);
  g_free (pi->tooltip);

  g_free (pi);
}
