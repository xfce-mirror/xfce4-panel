#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <gtk/gtk.h>

#include "xfce.h"
#include "builtins.h"
#include "callbacks.h"
#include "icons.h"

extern GtkTooltips *tooltips;

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

/* clock module */

typedef struct
{
  GtkWidget *eventbox;
  int size;
  
  gboolean show_colon;
  int interval;
  int timeout;
  
  GtkWidget *frame;
  GtkWidget *label;
}
t_clock;

/* "There can be only one" */
gboolean panel_has_clock = FALSE;

int secs_in_day = 24 * 60 * 60;

static gboolean
adjust_time (t_clock *clock)
{
  GTimeVal tv;
  int today;
  int hrs, mins;
  char *text, *markup;
  
  g_get_current_time (&tv);
  
  today = tv.tv_sec % secs_in_day;
  hrs = today / 3600;
  mins = (today % 3600) / 60;
  
  text = g_strdup_printf ("%.2d%c%.2d", 
			  hrs + 2, clock->show_colon ? ':' : ' ', mins);
  
  switch (clock->size)
  {
    case SMALL:
      markup = g_strconcat ("<tt><span size=\"smaller\">", 
			    text, "</span></tt>", NULL);
      break;
    case LARGE:
      markup = g_strconcat ("<tt><span size=\"larger\">", 
			    text, "</span></tt>", NULL);
      break;
    default:
      markup = g_strconcat ("<tt>", text, "</tt>", NULL);
      break;
  }
  clock->show_colon = !clock->show_colon;
  
  gtk_label_set_markup (GTK_LABEL (clock->label), markup);
  
  g_free (text);
  g_free (markup);
  
  return TRUE;
}

t_clock *
clock_init (GtkWidget *eventbox)
{
  t_clock *clock = g_new (t_clock, 1);
  panel_has_clock = TRUE;
  
  clock->size = MEDIUM;
  clock->interval = 1000; /* 1 sec */
  clock->timeout = 0;
  
  clock->frame = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER (clock->frame), 4);
  gtk_frame_set_shadow_type (GTK_FRAME (clock->frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (eventbox), clock->frame);
  
  clock->eventbox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (clock->frame), clock->eventbox);
  
  clock->label = gtk_label_new (NULL);
  gtk_container_add (GTK_CONTAINER (clock->eventbox), clock->label);
  
  gtk_widget_show_all (clock->frame);
  
  adjust_time (clock);
  
  return clock;
}

void
clock_run (t_clock *clock)
{
  clock->timeout = g_timeout_add (clock->interval, 
				  (GSourceFunc) adjust_time, clock);
}

void
clock_stop (t_clock *clock)
{
  panel_has_clock = FALSE;
  
  g_source_remove (clock->timeout);
  
  gtk_widget_destroy (clock->frame);
  g_free (clock);
}

void
clock_set_size (t_clock *clock, int size)
{
  int s = icon_size (size);
  
  clock->size = size;
  
  gtk_widget_set_size_request (clock->frame, s, s);
}

void
clock_set_style (t_clock *clock, int style)
{
  if (style == OLD_STYLE)
    {
      gtk_widget_set_name (clock->frame, "gxfce_color2");
      gtk_widget_set_name (clock->label, "gxfce_color2");
      gtk_widget_set_name (clock->eventbox, "gxfce_color2");
    }
  else
    {
      gtk_widget_set_name (clock->frame, "gxfce_color4");
      gtk_widget_set_name (clock->label, "gxfce_color4");
      gtk_widget_set_name (clock->eventbox, "gxfce_color4");
    }
}

void
clock_configure (void)
{
  
}

gboolean
create_clock_module (PanelModule *pm)
{
  if (panel_has_clock)
    return FALSE;
  
  pm->init = (gpointer) clock_init;
  pm->run = (gpointer) clock_run;
  pm->stop = (gpointer) clock_stop;
  pm->set_size = (gpointer) clock_set_size;
  pm->set_style = (gpointer) clock_set_style;
  pm->configure = (gpointer) clock_configure;

  pm->data = (gpointer) clock_init (pm->eventbox);
  
  return TRUE;
}

/*****************************************************************************/

/* trashcan module */

typedef struct
{
  char *dirname;
  char *command;
  
  int interval;
  gboolean stop;
  int size;
  
  gboolean empty;
  int changed;
  
  GdkPixbuf *current_pb;
  GdkPixbuf *empty_pb;
  GdkPixbuf *full_pb;
  
  GtkWidget *button;
  GtkWidget *image;
}
t_trash;

static void
free_trash (t_trash *trash)
{
  g_free (trash->dirname);
  g_free (trash->command);
  
  g_object_unref (trash->empty_pb);
  g_object_unref (trash->full_pb);
}

static void
trash_set_scaled_image (t_trash *trash)
{
  GdkPixbuf *pb;
  int width, height, bw;
  
  bw = trash->size == SMALL ? 0 : 2;
  width = height = icon_size (trash->size);
  
  gtk_container_set_border_width (GTK_CONTAINER (trash->button), bw);
  gtk_widget_set_size_request (trash->button, width + 4, height + 4);
    
  pb = gdk_pixbuf_scale_simple (trash->current_pb, 
				width - 2 * bw, 
				height - 2 * bw, 
				GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (trash->image), pb);
  g_object_unref (pb);
}

static gboolean
check_trash (t_trash *trash)
{
  GDir *dir = g_dir_open (trash->dirname, 0, NULL);
  const char *file;
  char text[MAXSTRLEN];
  
  if (dir)
    file = g_dir_read_name (dir);
  
  if (!dir || !file)
    {
      if (dir)
	g_dir_close (dir);
      
      if (!trash->empty)
	{
	  trash->current_pb = trash->empty_pb;
	  trash->empty = TRUE;
	  trash->changed = TRUE;
	  gtk_tooltips_set_tip (tooltips, trash->button, 
				_("Trashcan: 0 files"), NULL);
	}
    }
  else
    {
      struct stat s;
      int number = 0;
      int size = 0;
      char *cwd = g_get_current_dir ();
  
      chdir (trash->dirname);
  
	
      if (trash->empty)
	{
	  trash->current_pb = trash->full_pb;
	  trash->empty = FALSE;
	  trash->changed = TRUE;
	}
	
      while (file)
	{
	  number++;
	  
	  stat (file, &s);
	  size += s.st_size;
	  	  
	  file = g_dir_read_name (dir);
	}
	
      chdir (cwd);
      g_free (cwd);
      g_dir_close (dir);
	
      sprintf (text, _("Trashcan: %d files / %d kb"), number, size / 1024);
      
      gtk_tooltips_set_tip (tooltips, trash->button, 
			    text, NULL);
    }

  if (trash->changed)
    {
      trash_set_scaled_image (trash);
      trash->changed = FALSE;
    }
  
  if (trash->stop)
    {
      free_trash (trash);
      return FALSE;
    }
  else
    return TRUE;
}

void
trash_drop_cb (GtkWidget * widget, GdkDragContext * context, 
	       gint x, gint y, GtkSelectionData * data, 
	       guint info, guint time, gpointer cbdata)
{
  GList *fnames, *fnp;
  guint count;
  char *execute, *cmd;
  
  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);
  
  if (count > 0)
  {
    execute = (char *) g_malloc (MAXSTRLEN);
    cmd = (char *) cbdata;
    
    strcpy (execute, cmd);
    
    for (fnp = fnames; fnp; fnp = fnp->next, count--)
    {
      strcat (execute, " ");
      strncat (execute, (char *) (fnp->data), MAXSTRLEN - 1);
    }
    
    exec_cmd (execute);
    g_free (execute);
  }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE), time);
}

t_trash *
trash_init (GtkWidget *eventbox)
{
  const char *home = g_getenv ("HOME");
  t_trash *trash = g_new (t_trash, 1);
  
  trash->dirname = g_strconcat (home, "/.xfce/trash", NULL);
  trash->command = g_strdup ("xftrash");
  
  trash->interval = 2000; /* 2 sec */
  trash->stop = FALSE;
  trash->size = MEDIUM;
  
  trash->empty = TRUE;
  trash->changed = FALSE;
  
  trash->empty_pb = get_trashcan_pixbuf (TRASH_EMPTY_ICON);
  trash->full_pb = get_trashcan_pixbuf (TRASH_FULL_ICON);
  
  trash->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (trash->button), GTK_RELIEF_NONE);
  trash->image = gtk_image_new ();
  
  trash->current_pb = trash->empty_pb;
  trash_set_scaled_image (trash);
  
  gtk_container_add (GTK_CONTAINER (trash->button), trash->image);
  gtk_container_add (GTK_CONTAINER (eventbox), trash->button);
  
  gtk_widget_show_all (trash->button);
  
  gtk_tooltips_set_tip (tooltips, trash->button, 
			_("Trashcan: 0 files"), NULL);
			
  check_trash (trash);
  
  /* signals */
  if (trash->command)
    {
      g_signal_connect_swapped (trash->button, "clicked",
				G_CALLBACK (exec_cmd), trash->command);
      
      gtk_drag_dest_set (trash->button, GTK_DEST_DEFAULT_ALL, 
			 target_table, n_targets, GDK_ACTION_COPY);
      
      g_signal_connect (trash->button, "drag_data_received",
			G_CALLBACK (trash_drop_cb), trash->command);
    }
  
  return trash;
}

void
trash_run (t_trash *trash)
{
  g_timeout_add (trash->interval, (GSourceFunc) check_trash, trash);
}

void
trash_stop (t_trash *trash)
{
  trash->stop = TRUE;
}

void trash_set_size (t_trash *trash, int size)
{
  trash->size = size;
  trash_set_scaled_image (trash);
}

gboolean
create_trash_module (PanelModule *pm)
{
  pm->init = (gpointer) trash_init;
  pm->run = (gpointer) trash_run;
  pm->stop = (gpointer) trash_stop;
  pm->set_size = (gpointer) trash_set_size;
  
  pm->data = (gpointer) trash_init (pm->eventbox);
  
  return TRUE;
}