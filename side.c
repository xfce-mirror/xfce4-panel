#include <gtk/gtk.h>

#include "xfce.h"
#include "side.h"
#include "callbacks.h"
#include "popup.h"
#include "item.h"
#include "module.h"
#include "icons.h"

/*****************************************************************************/

SidePanel *
side_panel_new (int side)
{
  int i;
  SidePanel *sp = g_new (SidePanel, 1);

  sp->side = side;
  sp->hbox = NULL;

  for (i = 0; i < NBGROUPS; i++)
    sp->groups[i] = panel_group_new (i);

  return sp;
}

void
add_side_panel (XfcePanel * p, int side)
{
  SidePanel *sp = side == LEFT ?  p->left_panel : p->right_panel;

  create_side_panel (sp);

  gtk_box_pack_start (GTK_BOX (p->hbox), sp->hbox, FALSE, FALSE, 0);
}

void
create_side_panel (SidePanel * sp)
{
  int i;

  sp->hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (sp->hbox);

  for (i = 0; i < NBGROUPS; i++)
    {
      PanelGroup *pg = sp->groups[i];

      create_panel_group (pg, sp->side);

      if (sp->side == LEFT)
	gtk_box_pack_start (GTK_BOX (sp->hbox), pg->vbox, FALSE, FALSE, 0);
      else
	gtk_box_pack_end (GTK_BOX (sp->hbox), pg->vbox, FALSE, FALSE, 0);
    }
}

void
side_panel_set_size (SidePanel * sp, int size)
{
  int i;

  for (i = 0; i < NBGROUPS; i++)
    {
      panel_group_set_size (sp->groups[i], size);
    }
}

void
side_panel_set_style (SidePanel * sp, int style)
{
  int i;

  for (i = 0; i < NBGROUPS; i++)
    {
      panel_group_set_style (sp->groups[i], style);
    }
}

void
side_panel_set_popup_size (SidePanel * sp, int size)
{
  int i;

  for (i = 0; i < NBGROUPS; i++)
    {
      panel_group_set_popup_size (sp->groups[i], size);
    }
}

void
side_panel_set_groups (SidePanel * sp, int n)
{
  int i;

  for (i = 0; i < NBGROUPS; i++)
    {
      if (i < n)
	gtk_widget_show (sp->groups[i]->vbox);
      else
	gtk_widget_hide (sp->groups[i]->vbox);
    }
}

void
side_panel_free (SidePanel * sp)
{
  int i;

  for (i = 0; i < NBGROUPS; i++)
    {
      panel_group_free (sp->groups[i]);
    }
  
  g_free (sp);
}

/*****************************************************************************/

PanelGroup *
panel_group_new (int index)
{
  PanelGroup *pg = g_new (PanelGroup, 1);

  pg->index = index;
  pg->vbox = NULL;

  /* These are filled in by the get_settings () function;
     this is a bit different than for the other structures */
  pg->handle = NULL;
  pg->popup = NULL;
  pg->type = -1;
  pg->eventbox = NULL;
  pg->item = NULL;
  pg->module = NULL;

  return pg;
}

void
create_panel_group (PanelGroup * pg, int side)
{
  pg->vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (pg->vbox);

  if (pg->index == 0)
    {
      pg->handle = move_handle_new (side);
      gtk_box_pack_start (GTK_BOX (pg->vbox), pg->handle->hbox, 
			  FALSE, FALSE, 0);
    }
  else
    {
      create_panel_popup (pg->popup);
      gtk_box_pack_start (GTK_BOX (pg->vbox), pg->popup->button, 
			  FALSE, FALSE, 0);
    }

  pg->eventbox = gtk_event_box_new ();
  gtk_widget_show (pg->eventbox);
  gtk_box_pack_start (GTK_BOX (pg->vbox), pg->eventbox, 
		      FALSE, FALSE, 0);

  if (pg->type == ICON)
    {
      create_panel_item (pg->item);
      gtk_container_add (GTK_CONTAINER (pg->eventbox), pg->item->button);
      
      g_signal_connect (pg->item->button, "button-press-event",
			G_CALLBACK (item_on_panel_press_cb), pg);

    }
  else
    {
      create_panel_module (pg->module, pg->eventbox);
      
      if (pg->module->run)
	pg->module->run (pg->module->data);
      else
	{
	  g_printerr ("No run function: %s\n", pg->module->name);
	  
	  panel_module_free (pg->module);
	  pg->item = panel_item_new (NULL);
	  pg->item->tooltip = 
		      g_strdup (_("Click mouse button 3 to change item"));
	  pg->type = ICON;
	  pg->item->id = UNKNOWN_ICON;
	  create_panel_item (pg->item);
	  gtk_container_add (GTK_CONTAINER (pg->eventbox), pg->item->button);
	  
	  g_signal_connect (pg->item->button, "button-press-event",
			    G_CALLBACK (item_on_panel_press_cb), pg);

	}

      if (pg->module->main)
	g_signal_connect (pg->module->main, "button-press-event",
			  G_CALLBACK (item_on_panel_press_cb), pg);
    }

  /* signals */
  g_signal_connect (pg->eventbox, "button-press-event", 
		    G_CALLBACK (group_button_press_cb), pg);
}

void
panel_group_set_size (PanelGroup * pg, int size)
{
  if (pg->index == 0)
    move_handle_set_size (pg->handle, size);
  else
    panel_popup_set_size (pg->popup, size);

  if (pg->type == ICON)
    panel_item_set_size (pg->item, size);
  else
    panel_module_set_size (pg->module, size);
}

void
panel_group_set_style (PanelGroup * pg, int style)
{
  if (pg->index == 0)
    move_handle_set_style (pg->handle, style);
  else
    panel_popup_set_style (pg->popup, style);

  if (pg->type == ICON)
    panel_item_set_style (pg->item, style);
  else
    panel_module_set_style (pg->module, style);
}

void
panel_group_set_popup_size (PanelGroup * pg, int size)
{
  if (pg->index != 0)
    panel_popup_set_popup_size (pg->popup, size);
}

void
panel_group_free (PanelGroup * pg)
{
  if (pg->index == 0)
    /* handle is all gtk widgets that don't have to be freed */
    g_free (pg->handle);
  else
    panel_popup_free (pg->popup);

  if (pg->type == ICON)
    panel_item_free (pg->item);
  else
    panel_module_free (pg->module);
  
  g_free (pg);
}

/*****************************************************************************/

MoveHandle *
move_handle_new (int side)
{
  GtkWidget *im;
  GdkPixbuf *pb;

  MoveHandle *mh = g_new (MoveHandle, 1);
  
  mh->hbox = gtk_hbox_new (FALSE, 0);

  mh->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (mh->button), GTK_RELIEF_NONE);
  gtk_widget_set_size_request (mh->button, 16, MEDIUM_TOPHEIGHT);
  
  panel_set_tooltip (mh->button, _("Iconify panel"));
  g_signal_connect_swapped (mh->button, "clicked", 
			    G_CALLBACK (iconify_cb), NULL);
  
  pb = get_pixbuf_from_id (ICONIFY_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  gtk_widget_set_name (im, "gxfce_color1");
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (mh->button), im);
  
  mh->eventbox = gtk_event_box_new ();

  mh->handle_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (mh->handle_frame), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (mh->eventbox), mh->handle_frame);
  
  pb = get_pixbuf_from_id (HANDLE_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  gtk_widget_set_name (im, "gxfce_color1");
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (mh->handle_frame), im);
  
  
  if (side == LEFT)
    {
      gtk_box_pack_start (GTK_BOX (mh->hbox), mh->button, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (mh->hbox), mh->eventbox, FALSE, FALSE, 0);
    }
  else
    {
      gtk_box_pack_end (GTK_BOX (mh->hbox), mh->button, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (mh->hbox), mh->eventbox, FALSE, FALSE, 0);
    }

  panel_set_as_move_handle (mh->eventbox);
    
  gtk_widget_show_all (mh->hbox);
  return mh;
}

void move_handle_set_size (MoveHandle * mh, int size)
{
  int h = top_height (size);
  int w = icon_size (size);
  gtk_widget_set_size_request (mh->eventbox, w + 8 - 16, h);
  gtk_widget_set_size_request (mh->button, 16, h);
}

void move_handle_set_style (MoveHandle * mh, int style)
{
  if (style == OLD_STYLE)
    {
      gtk_frame_set_shadow_type (GTK_FRAME (mh->handle_frame), GTK_SHADOW_OUT);
      gtk_widget_set_name (mh->handle_frame, "gxfce_color1");
      
      gtk_widget_set_name (mh->eventbox, "gxfce_color1");
      
      gtk_button_set_relief (GTK_BUTTON (mh->button), GTK_RELIEF_NORMAL);
      gtk_widget_set_name (mh->button, "gxfce_color1");
    }
  else
    {
      gtk_frame_set_shadow_type (GTK_FRAME (mh->handle_frame), GTK_SHADOW_NONE);
      gtk_widget_set_name (mh->handle_frame, "gxfce_color7");
      
      gtk_widget_set_name (mh->eventbox, "gxfce_color7");
      
      gtk_button_set_relief (GTK_BUTTON (mh->button), GTK_RELIEF_NONE);
      gtk_widget_set_name (mh->button, "gxfce_color7");
    }
}
