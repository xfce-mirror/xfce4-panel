#include <gtk/gtk.h>

#include "xfce.h"
#include "central.h"
#include "callbacks.h"
#include "icons.h"

/*****************************************************************************/
/* colors used for screen buttons in OLD_STYLE */
static char *screen_class[] = { 
  "gxfce_color2",
  "gxfce_color5",
  "gxfce_color4",
  "gxfce_color6"
};

CentralPanel *
central_panel_new (void)
{
  int i;
  CentralPanel *cp = g_new (CentralPanel, 1);

  cp->frame = NULL;
  cp->hbox = NULL;

  cp->left_table = NULL;
  cp->left_buttons[0] = NULL;
  cp->left_buttons[1] = NULL;

  cp->right_table = NULL;
  cp->right_buttons[0] = NULL;
  cp->right_buttons[1] = NULL;

  cp->desktop_table = NULL;

  for (i = 0; i < NBSCREENS; i++)
    {
      cp->screen_buttons[i] = NULL;
      cp->screen_names[i] = NULL;
    }
    
  cp->lock_command = NULL;
  cp->xmlnode = NULL;

  return cp;
}

static void
create_left_table (CentralPanel * cp)
{
  GtkWidget *im;
  GdkPixbuf *pb;

  cp->left_table = gtk_table_new (2, 2, FALSE);

  cp->left_buttons[0] = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (cp->left_buttons[0]), GTK_RELIEF_NONE);
  cp->left_buttons[1] = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (cp->left_buttons[1]), GTK_RELIEF_NONE);

  pb = get_pixbuf_from_id (MINILOCK_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (cp->left_buttons[0]), im);

  pb = get_pixbuf_from_id (MINIINFO_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (cp->left_buttons[1]), im);

  /* by default put buttons in 1st column in 2 rows */
  gtk_table_attach (GTK_TABLE (cp->left_table), cp->left_buttons[0],
		    0, 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (cp->left_table), cp->left_buttons[1],
		    0, 1, 1, 2, GTK_SHRINK, GTK_EXPAND, 0, 0);

  gtk_widget_show_all (cp->left_table);
}

static void
create_right_table (CentralPanel * cp)
{
  GtkWidget *im;
  GdkPixbuf *pb;

  cp->right_table = gtk_table_new (2, 2, FALSE);

  cp->right_buttons[0] = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (cp->right_buttons[0]), GTK_RELIEF_NONE);
  cp->right_buttons[1] = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (cp->right_buttons[1]), GTK_RELIEF_NONE);

  pb = get_pixbuf_from_id (MINIPALET_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (cp->right_buttons[0]), im);

  pb = get_pixbuf_from_id (MINIPOWER_ICON);
  im = gtk_image_new_from_pixbuf (pb);
  g_object_unref (pb);
  gtk_container_add (GTK_CONTAINER (cp->right_buttons[1]), im);

  /* by default put buttons in 1st column in 2 rows */
  gtk_table_attach (GTK_TABLE (cp->right_table), cp->right_buttons[0],
		    0, 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 0, 0);
  gtk_table_attach (GTK_TABLE (cp->right_table), cp->right_buttons[1],
		    0, 1, 1, 2, GTK_SHRINK, GTK_EXPAND, 0, 0);

  gtk_widget_show_all (cp->right_table);
}

static void
create_desktop_table (CentralPanel * cp)
{
  int i;

  cp->desktop_table = gtk_table_new (2, NBSCREENS, FALSE);

  for (i = 0; i < NBSCREENS; i++)
    {
      ScreenButton *sb = cp->screen_buttons[i] = screen_button_new ();

      sb->frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (sb->frame), GTK_SHADOW_IN);

      sb->button = gtk_toggle_button_new ();
      gtk_button_set_relief (GTK_BUTTON (sb->button), GTK_RELIEF_HALF);
      gtk_widget_set_size_request (sb->button, SCREEN_BUTTON_WIDTH, -1);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->button), i == 0);
      gtk_container_add (GTK_CONTAINER (sb->frame), sb->button);

      sb->label = gtk_label_new (cp->screen_names[i]);
      gtk_misc_set_alignment (GTK_MISC (sb->label), 0.1, 0.5);
      gtk_container_add (GTK_CONTAINER (sb->button), sb->label);

      /* pack buttons alternating first and second row */
      if ((i % 2) == 0)
	{
	  gtk_table_attach (GTK_TABLE (cp->desktop_table), sb->frame,
			    i, i + 1, 0, 1, GTK_SHRINK, GTK_EXPAND, 0, 0);
	}
      else
	{
	  int pos = i - 1;

	  gtk_table_attach (GTK_TABLE (cp->desktop_table), sb->frame,
			    pos, pos + 1, 1, 2, GTK_SHRINK, GTK_EXPAND, 0, 0);
	}
	
      /* signals */
      /* we need the callback id to be able to block the handler to 
	 prevent a race condition when changing screens */
      sb->callback_id = 
	g_signal_connect_swapped (sb->button, "clicked", 
				  G_CALLBACK (change_current_desktop), 
				  GINT_TO_POINTER (i));
	
      g_signal_connect (sb->button, "button-press-event",
			G_CALLBACK (screen_button_pressed_cb), sb);
    }

  gtk_widget_show_all (cp->desktop_table);
}

void
add_central_panel (XfcePanel * p)
{
  create_central_panel (p->central_panel);

  gtk_box_pack_start (GTK_BOX (p->hbox), p->central_panel->frame,
		      TRUE, TRUE, 0);
}

void
create_central_panel (CentralPanel * cp)
{
  cp->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (cp->frame), GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (cp->frame), 1);

  cp->hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (cp->frame), cp->hbox);

  create_left_table (cp);
  gtk_box_pack_start (GTK_BOX (cp->hbox), cp->left_table, FALSE, FALSE, 0);

  create_desktop_table (cp);
  gtk_box_pack_start (GTK_BOX (cp->hbox), cp->desktop_table, TRUE, TRUE, 0);

  create_right_table (cp);
  gtk_box_pack_start (GTK_BOX (cp->hbox), cp->right_table, FALSE, FALSE, 0);

  gtk_widget_show (cp->hbox);
  gtk_widget_show (cp->frame);
  
  /* signals */
  /* left minibuttons */
  if (!cp->lock_command)
    cp->lock_command = g_strdup ("xflock");
  /* fixed tooltip, since the user can't change the icon anyway */
  panel_set_tooltip (cp->left_buttons[0], _("Lock the screen"));
  g_signal_connect_swapped (cp->left_buttons[0], "clicked",
		    G_CALLBACK (mini_lock_cb), cp->lock_command);
  
  panel_set_tooltip (cp->left_buttons[1], _("Info..."));
  g_signal_connect_swapped (cp->left_buttons[1], "clicked",
			    G_CALLBACK (mini_info_cb), NULL);
			    
  /* right minibuttons */
  panel_set_tooltip (cp->right_buttons[0], _("Setup..."));
  g_signal_connect_swapped (cp->right_buttons[0], "clicked",
			    G_CALLBACK (mini_palet_cb), NULL);

  panel_set_tooltip (cp->right_buttons[1], 
		     _("Mouse 1: Exit\nMouse 3: Restart"));
  /* make left _and_ right button work */
  g_signal_connect_swapped (cp->right_buttons[1], "button-press-event",
			    G_CALLBACK (gtk_button_pressed), 
			    cp->right_buttons[1]);
  g_signal_connect (cp->right_buttons[1], "button-release-event",
		    G_CALLBACK (mini_power_cb), NULL);
}

static int
get_screen_button_height (int size)
{
  if (size == SMALL)
    return SMALL_PANEL_ICONS;
  else if (size == LARGE)
    return (LARGE_TOPHEIGHT + LARGE_PANEL_ICONS) / 2 - 6;
  else
    return (MEDIUM_TOPHEIGHT + MEDIUM_PANEL_ICONS) / 2 - 5;    
}

static int
get_screen_button_width (int size)
{
  if (size == SMALL)
    return SCREEN_BUTTON_WIDTH / 2;
  else
    return SCREEN_BUTTON_WIDTH;
}

void
central_panel_set_size (CentralPanel * cp, int size)
{
  GtkRequisition req;
  GList *child;
  GtkTableChild *tc;
  int hpos, vpos;
  int h, w;

  /* left table 
     when SMALL put second button in first column first row
     else put second button in second column second row
   */
  for (child = GTK_TABLE (cp->left_table)->children;
       child && child->data; child = child->next)
    {
      tc = (GtkTableChild *) child->data;
      
      if (tc->widget == cp->left_buttons [1])
	break;
    }

  if (size == SMALL)
    {
      hpos = 1;
      vpos = 0;
    }
  else
    {
      hpos = 0;
      vpos = 1;
    }
  
  tc->left_attach = hpos;
  tc->right_attach = hpos + 1;
  tc->top_attach = vpos;
  tc->bottom_attach = vpos + 1;

  /* right table
     when SMALL put second button in second column first row
     else put second button in first column second row
   */
  for (child = GTK_TABLE (cp->right_table)->children;
       child && child->data; child = child->next)
    {
      tc = (GtkTableChild *) child->data;
      
      if (tc->widget == cp->right_buttons [1])
	break;
    }
   
  if (size == SMALL)
    {
      hpos = 1;
      vpos = 0;
    }
  else
    {
      hpos = 0;
      vpos = 1;
    }
  
  tc->left_attach = hpos;
  tc->right_attach = hpos + 1;
  tc->top_attach = vpos;
  tc->bottom_attach = vpos + 1;

  /* desktop table */
  h = get_screen_button_height (size);
  w = get_screen_button_width (size);

  for (child = GTK_TABLE (cp->desktop_table)->children;
       child && child->data; child = child->next)
    {
      ScreenButton *sb;
      int i;

      for (i = 0; i < NBSCREENS; i++)
	{
	  sb = cp->screen_buttons[i];
	  gtk_widget_set_size_request (sb->button, w, h);
	}
	
      tc = (GtkTableChild *) child->data;

      for (i = 0; i < NBSCREENS; i++)
	{
	  sb = cp->screen_buttons[i];
	  
	  if (tc->widget == sb->frame)
	    break;
	}
	
      if ((i % 2)==0)
	{
	  if (size == SMALL)
	    {
	      hpos = i;
	      vpos = 0;
	    }
	  else
	    {
	      hpos = i / 2;
	      vpos = 0;
	    }
	}
      else
	{
	  if (size == SMALL)
	    {
	      hpos = i;
	      vpos = 0;
	    }
	  else
	    {
	      hpos = (i - 1) / 2;
	      vpos = 1;
	    }
	}
      
      tc->left_attach = hpos;
      tc->right_attach = hpos + 1;
      tc->top_attach = vpos;
      tc->bottom_attach = vpos + 1;
    }
    
  gtk_widget_set_size_request (cp->left_buttons[0], -1, -1);
  gtk_widget_set_size_request (cp->left_buttons[1], -1, -1);
  gtk_widget_set_size_request (cp->right_buttons[0], -1, -1);
  gtk_widget_set_size_request (cp->right_buttons[1], -1, -1);

  /* largest minibutton */
  gtk_widget_size_request (cp->right_buttons[0], &req);
    
  w = req.width;
  h = req.height;
  
  gtk_widget_set_size_request (cp->left_buttons[0], w, h);
  gtk_widget_set_size_request (cp->left_buttons[1], w, h);
  gtk_widget_set_size_request (cp->right_buttons[1], w, h);
}

void
central_panel_set_style (CentralPanel * cp, int style)
{
  int i;
  
  if (style == OLD_STYLE)
    {
      gtk_frame_set_shadow_type (GTK_FRAME (cp->frame), GTK_SHADOW_OUT);
      
      for (i = 0; i < NBSCREENS; i++)
	{
	  gtk_widget_set_name (cp->screen_buttons[i]->button, 
			       screen_class[i % 4]);
	}
    }
  else
    {
      gtk_frame_set_shadow_type (GTK_FRAME (cp->frame), GTK_SHADOW_ETCHED_IN);
      
      for (i = 0; i < NBSCREENS; i++)
	{
	  gtk_widget_set_name (cp->screen_buttons[i]->button, 
			       "gxfce_color4");
	}
    }
}

void
central_panel_set_screens (CentralPanel * cp, int n)
{
  int i;

  for (i = 0; i < NBSCREENS; i++)
    {
      ScreenButton *sb = cp->screen_buttons[i];

      if (i < n)
	gtk_widget_show (sb->frame);
      else
	gtk_widget_hide (sb->frame);
    }
}

void
central_panel_free (CentralPanel * cp)
{
  int i;

  /* the gtk widgets are destroyed by gtk,
     so we only have to get rid of our own
     structures.
   */

  for (i = 0; i < NBSCREENS; i++)
    screen_button_free (cp->screen_buttons[i]);

  g_free (cp);
}

void 
central_panel_set_current (CentralPanel * cp, int n)
{
  int i;
  
  for (i = 0; i < NBSCREENS; i++)
    {
      ScreenButton *sb = cp->screen_buttons[i];
      
      g_signal_handler_block (sb->button, sb->callback_id);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->button), i == n);
      g_signal_handler_unblock (sb->button, sb->callback_id);
    }
}

/*****************************************************************************/

ScreenButton *
screen_button_new (void)
{
  ScreenButton *sb = g_new (ScreenButton, 1);

  sb->frame = NULL;
  sb->button = NULL;
  sb->label = NULL;
  sb->callback_id = 0;
  
  return sb;
}

void
screen_button_free (ScreenButton * sb)
{
  /* the gtk widgets are destroyed by gtk,
     so we only have to get rid of our own
     structures.
   */

  g_free (sb);
}

