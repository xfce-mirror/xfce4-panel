#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "xfce.h"
#include "callbacks.h"
#include "dialogs.h"
#include "central.h"
#include "side.h"
#include "item.h"
#include "popup.h"
#include "icons.h"

static PanelPopup *open_popup = NULL;

void hide_current_popup_menu (void);

/*****************************************************************************/
/* useful utility functions */
void
exec_cmd (const char *cmd)
{
  GError *error = NULL;		/* this must be NULL to prevent crash :( */

  if (!g_spawn_command_line_async (cmd, &error))
    {
      GtkWidget *dialog;
      char *msg;

      msg = g_strcompress (error->message);

      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_OK, msg);
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      g_free (msg);
    }
}

/*** the next three routines are taken straight from gnome-libs so that the
     gtk-only version can receive drag and drops as well ***/
/**
 * gnome_uri_list_free_strings:
 * @list: A GList returned by gnome_uri_list_extract_uris() or gnome_uri_list_extract_filenames()
 *
 * Releases all of the resources allocated by @list.
 */
void
gnome_uri_list_free_strings (GList * list)
{
  g_list_foreach (list, (GFunc) g_free, NULL);
  g_list_free (list);
}


/**
 * gnome_uri_list_extract_uris:
 * @uri_list: an uri-list in the standard format.
 *
 * Returns a GList containing strings allocated with g_malloc
 * that have been splitted from @uri-list.
 */
GList *
gnome_uri_list_extract_uris (const gchar * uri_list)
{
  const gchar *p, *q;
  gchar *retval;
  GList *result = NULL;

  g_return_val_if_fail (uri_list != NULL, NULL);

  p = uri_list;

  /* We don't actually try to validate the URI according to RFC
   * 2396, or even check for allowed characters - we just ignore
   * comments and trim whitespace off the ends.  We also
   * allow LF delimination as well as the specified CRLF.
   */
  while (p)
    {
      if (*p != '#')
	{
	  while (isspace ((int) (*p)))
	    p++;

	  q = p;
	  while (*q && (*q != '\n') && (*q != '\r'))
	    q++;

	  if (q > p)
	    {
	      q--;
	      while (q > p && isspace ((int) (*q)))
		q--;

	      retval = (char *) g_malloc (q - p + 2);
	      strncpy (retval, p, q - p + 1);
	      retval[q - p + 1] = '\0';

	      result = g_list_prepend (result, retval);
	    }
	}
      p = strchr (p, '\n');
      if (p)
	p++;
    }

  return g_list_reverse (result);
}


/**
 * gnome_uri_list_extract_filenames:
 * @uri_list: an uri-list in the standard format
 *
 * Returns a GList containing strings allocated with g_malloc
 * that contain the filenames in the uri-list.
 *
 * Note that unlike gnome_uri_list_extract_uris() function, this
 * will discard any non-file uri from the result value.
 */
GList *
gnome_uri_list_extract_filenames (const gchar * uri_list)
{
  GList *tmp_list, *node, *result;

  g_return_val_if_fail (uri_list != NULL, NULL);

  result = gnome_uri_list_extract_uris (uri_list);

  tmp_list = result;
  while (tmp_list)
    {
      gchar *s = (char *) tmp_list->data;

      node = tmp_list;
      tmp_list = tmp_list->next;

      if (!strncmp (s, "file:", 5))
	{
	  /* added by Jasper Huijsmans
	     remove leading multiple slashes */
	  if (!strncmp (s + 5, "///", 3))
	    node->data = g_strdup (s + 7);
	  else
	    node->data = g_strdup (s + 5);
	}
      else
	{
	  node->data = g_strdup (s);
	}
      g_free (s);
    }
  return result;
}

/*****************************************************************************/
/* panel */
gboolean
panel_delete_cb (GtkWidget * window, GdkEvent * ev, gpointer data)
{
  GtkWidget *dialog;
  int response;

  hide_current_popup_menu ();

  dialog = gtk_message_dialog_new (GTK_WINDOW (window),
				   GTK_DIALOG_MODAL |
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				   _
				   ("The panel recieved a request from the window manager\n"
				    "to quit. Usually this is by accident. If you want to\n"
				    "continue using the panel choose \'No\'\n\n"
				    "Do you want to quit the panel?"));

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_YES)
    quit ();

  return TRUE;
}

/*****************************************************************************/
/* central panel */
gboolean
screen_button_pressed_cb (GtkButton * b, GdkEventButton * ev,
			  ScreenButton * sb)
{
  hide_current_popup_menu ();

  if (ev->button != 3)
    return FALSE;

  return TRUE;
}

/* temporary functions for testing purposes */
void
switch_size_cb (void)
{
  static int size = MEDIUM;

  size = size == MEDIUM ? SMALL : size == SMALL ? LARGE : MEDIUM;

  panel_set_size (size);
  panel_set_popup_size (size);
}

void
switch_style_cb (void)
{
  static int style = NEW_STYLE;

  style = style == NEW_STYLE ? OLD_STYLE : NEW_STYLE;

  panel_set_style (style);
}
 /**/ void
mini_lock_cb (char *cmd)
{
  hide_current_popup_menu ();

  exec_cmd (cmd);
}

void
mini_info_cb (void)
{
  hide_current_popup_menu ();

  switch_size_cb ();
}

void
mini_palet_cb (void)
{
  hide_current_popup_menu ();

  switch_style_cb ();
}

void
mini_power_cb (GtkButton * b, GdkEventButton * ev, gpointer data)
{
  int w, h;

  hide_current_popup_menu ();

  gdk_window_get_size (ev->window, &w, &h);

  /* we have to check if the pointer is still inside the button */
  if (ev->x < 0 || ev->x > w || ev->y < 0 || ev->y > h)
    {
      gtk_button_released (b);
      return;
    }

  if (ev->button == 1)
    quit ();
  else
    restart ();
}

/*****************************************************************************/
/* side panel */
void
iconify_cb (void)
{
  hide_current_popup_menu ();

  iconify_panel ();
}

gboolean
group_button_press_cb (GtkButton * b, GdkEventButton * ev, PanelGroup * pg)
{
  hide_current_popup_menu ();

  if (ev->button != 3)
    return FALSE;

  return TRUE;
}

/*****************************************************************************/
/* popup */

static void
hide_popup (PanelPopup * pp)
{
  if (open_popup == pp)
    open_popup = NULL;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pp->button), FALSE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (pp->arrow_image), pp->up);

  gtk_widget_hide (pp->window);

  if (pp->detached)
    {
      pp->detached = FALSE;
      gtk_widget_show (pp->tearoff_button);
      gtk_window_set_decorated (GTK_WINDOW (pp->window), FALSE);
    }
}

static void
show_popup (PanelPopup * pp)
{
  GtkRequisition req1, req2;
  GdkWindow *p;
  int xb, yb, xp, yp, x, y;

  if (open_popup)
    hide_popup (open_popup);

  if (!pp->detached)
    open_popup = pp;

  gtk_image_set_from_pixbuf (GTK_IMAGE (pp->arrow_image), pp->down);

  if (!GTK_WIDGET_REALIZED (pp->button))
    gtk_widget_realize (pp->button);

  gtk_widget_size_request (pp->button, &req1);
  xb = pp->button->allocation.x;
  yb = pp->button->allocation.y;

  p = gtk_widget_get_parent_window (pp->button);
  gdk_window_get_root_origin (p, &xp, &yp);

  x = xb + xp + req1.width / 2;
  y = yb + yp;

  if (!GTK_WIDGET_REALIZED (pp->window))
    gtk_widget_realize (pp->window);

  gtk_widget_size_request (pp->window, &req2);

  /* show upwards or downwards ?
   * and make detached menu appear loose from the button */
  if (y < req2.height && gdk_screen_height () - y > req2.height)
    {
      y = y + req1.height;

      if (pp->detached)
	y = y + 30;
    }
  else
    {
      y = y - req2.height;

      if (pp->detached)
	y = y - 15;
    }

  x = x - req2.width / 2;

  if (x < 0)
    x = 0;
  if (x + req2.width > gdk_screen_width ())
    x = gdk_screen_width () - req2.width;

  gtk_window_move (GTK_WINDOW (pp->window), x, y);
  gtk_widget_show (pp->window);
}

void
hide_current_popup_menu (void)
{
  if (open_popup)
    hide_popup (open_popup);
}

void
toggle_popup (GtkWidget * button, PanelPopup * pp)
{
  hide_current_popup_menu ();

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    show_popup (pp);
  else
    hide_popup (pp);
}


void
tearoff_popup (GtkWidget * button, PanelPopup * pp)
{
  open_popup = NULL;
  pp->detached = TRUE;
  gtk_widget_hide (pp->window);
  gtk_widget_hide (pp->tearoff_button);
  gtk_window_set_decorated (GTK_WINDOW (pp->window), TRUE);
  show_popup (pp);
}


gboolean
delete_popup (GtkWidget * window, GdkEvent * ev, PanelPopup * pp)
{
  hide_popup (pp);

  return TRUE;
}

/*****************************************************************************/
/* item */
void
addtomenu_item_drop_cb (GtkWidget * widget, GdkDragContext * context,
			gint x, gint y, GtkSelectionData * data,
			guint info, guint time, PanelPopup * pp)
{
  GList *fnames, *fnp;
  guint count;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);

  if (count > 0)
    {
      for (fnp = fnames; fnp; fnp = fnp->next, count--)
	{
	  char *cmd;
	  PanelItem *pi = panel_item_new (pp);

	  if (!(cmd = g_find_program_in_path ((char *) fnp->data)))
	    continue;

	  pi->in_menu = TRUE;
	  pi->command = cmd;
	  
	  pi->caption = g_path_get_basename (cmd);
	  pi->caption[0] = g_ascii_toupper (pi->caption[0]);
	  
	  create_panel_item (pi);
	  pp->items = g_list_prepend (pp->items, pi);
	  
	  gtk_size_group_add_widget (pp->hgroup, pi->image);
	  gtk_box_pack_start (GTK_BOX (pp->vbox), pi->button, TRUE, TRUE, 0);
	  
	  gtk_box_reorder_child (GTK_BOX (pp->vbox), pi->button, 2);
	  gtk_button_clicked (GTK_BUTTON (pp->button));
	  gtk_button_clicked (GTK_BUTTON (pp->button));
	}
    }

  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE),
		   time);
}

void addtomenu_item_click_cb (GtkButton * b, PanelPopup * pp)
{
  add_item_dialog (pp);
}

void
item_drop_cb (GtkWidget * widget, GdkDragContext * context,
	      gint x, gint y, GtkSelectionData * data,
	      guint info, guint time, char *command)
{
  GList *fnames, *fnp;
  guint count;
  char *execute;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);

  if (count > 0)
    {
      execute = (char *) g_new0 (char, MAXSTRLEN);

      strcpy (execute, command);

      for (fnp = fnames; fnp; fnp = fnp->next, count--)
	{
	  strcat (execute, " ");
	  strncat (execute, (char *) (fnp->data),
		   MAXSTRLEN - strlen (execute));
	}

      exec_cmd (execute);
      g_free (execute);

      hide_current_popup_menu ();
    }
  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE),
		   time);
}

void
item_click_cb (GtkButton * b, char *command)
{
  hide_current_popup_menu ();
  exec_cmd (command);
}

gboolean
item_in_menu_press_cb (GtkButton * b, GdkEventButton * ev, PanelItem * pi)
{
  if (ev->button != 3)
    return FALSE;

  edit_item_dlg (pi);
  
  return TRUE;
}

gboolean
item_on_panel_press_cb (GtkButton * b, GdkEventButton * ev, PanelGroup * pg)
{
  if (ev->button != 3)
    return FALSE;

  hide_current_popup_menu ();

  edit_item_or_module_dlg (pg);
  
  return TRUE;
}

/*****************************************************************************/
