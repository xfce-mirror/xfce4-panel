#include <gtk/gtk.h>

#include "xfce.h"
#include "central.h"
#include "side.h"
#include "popup.h"
#include "item.h"
#include "module.h"
#include "settings.h"
#include "icons.h"
#include "wmhints.h"
#include "move.h"
#include "callbacks.h"

/* the xfce panel */
XfcePanel panel;
GtkTooltips *tooltips;

/*****************************************************************************/
void
panel_init (void)
{
  panel.x = -1;
  panel.y = -1;
  panel.size = -1;
  panel.popup_size = -1;
  panel.style = -1;
  panel.num_screens = -1;
  panel.num_groups_left = -1;
  panel.num_groups_right = -1;

  panel.current_screen = 0;
  panel.window = NULL;
  panel.frame = NULL;
  panel.hbox = NULL;
  panel.central_panel = central_panel_new ();
  panel.left_panel = side_panel_new (LEFT);
  panel.right_panel = side_panel_new (RIGHT);

  get_settings (&panel);

  tooltips = gtk_tooltips_new ();

  watch_root_properties ();
}

static void
panel_set_position (void)
{
  GtkRequisition req;

  if (panel.x == -1 || panel.y == -1)
    {
      gtk_widget_size_request (panel.window, &req);

      panel.x = gdk_screen_width () / 2 - req.width / 2;
      panel.y = gdk_screen_height () - req.height;
    }

  gtk_window_move (GTK_WINDOW (panel.window), panel.x, panel.y);
}

void
create_xfce_panel (void)
{
  GdkPixbuf *pb;

  panel.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (panel.window), _("XFce Main Panel"));
  gtk_window_set_wmclass (GTK_WINDOW (panel.window),
			  _("XFce4 Panel"), _("XFce4 Panel"));
  gtk_window_set_decorated (GTK_WINDOW (panel.window), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (panel.window), FALSE);
  gtk_window_set_default_size (GTK_WINDOW (panel.window), 0, 0);

  gtk_window_stick (GTK_WINDOW (panel.window));

  pb = get_pixbuf_from_id (XFCE_ICON);
  gtk_window_set_icon (GTK_WINDOW (panel.window), pb);
  g_object_unref (pb);

  gtk_window_set_position (GTK_WINDOW (panel.window), GTK_WIN_POS_CENTER);

  panel.frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (panel.frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (panel.window), panel.frame);
  gtk_widget_show (panel.frame);

  panel.hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (panel.frame), panel.hbox);
  gtk_widget_show (panel.hbox);

  add_side_panel (&panel, LEFT);
  add_central_panel (&panel);
  add_side_panel (&panel, RIGHT);

  /* signals */
  g_signal_connect (panel.window, "delete-event",
		    G_CALLBACK (panel_delete_cb), NULL);

  /* apply configuration */
  panel_set_size (panel.size);
  panel_set_style (panel.style);
  panel_set_popup_size (panel.popup_size);

  central_panel_set_screens (panel.central_panel, panel.num_screens);
  panel_set_left_groups (panel.num_groups_left);
  panel_set_right_groups (panel.num_groups_right);

  panel.current_screen = net_current_desktop_get ();
  if (panel.current_screen > 0)
    central_panel_set_current (panel.central_panel, panel.current_screen);

  panel_set_position ();

  gtk_widget_show (panel.window);
  CreateDrawGC (panel.window->window);
}

void
panel_cleanup (void)
{
  side_panel_free (panel.left_panel);
  side_panel_free (panel.right_panel);
  central_panel_free (panel.central_panel);
}

/*****************************************************************************/

void
panel_set_as_move_handle (GtkWidget * widget)
{
  attach_move_callbacks (widget, panel.window);
}

void
set_transient_for_panel (GtkWidget * widget)
{
  if (GTK_IS_WINDOW (widget))
    gtk_window_set_transient_for (GTK_WINDOW (widget),
				  GTK_WINDOW (panel.window));
}

void
iconify_panel (void)
{
  gtk_window_iconify (GTK_WINDOW (panel.window));
}

void
panel_set_tooltip (GtkWidget * widget, const char *tip)
{
  gtk_tooltips_set_tip (tooltips, widget, tip, NULL);
}

int
icon_size (int size)
{
  switch (size)
    {
    case SMALL:
      return SMALL_PANEL_ICONS;
      break;
    case LARGE:
      return LARGE_PANEL_ICONS;
      break;
    default:
      return MEDIUM_PANEL_ICONS;
    }
}

int
popup_icon_size (int size)
{
  switch (size)
    {
    case SMALL:
      return SMALL_POPUP_ICONS;
      break;
    case LARGE:
      return LARGE_POPUP_ICONS;
      break;
    default:
      return MEDIUM_POPUP_ICONS;
    }
}

int
top_height (int size)
{
  switch (size)
    {
    case SMALL:
      return SMALL_TOPHEIGHT;
      break;
    case LARGE:
      return LARGE_TOPHEIGHT;
      break;
    default:
      return MEDIUM_TOPHEIGHT;
    }
}

/*****************************************************************************/
/* xsettings */
void
panel_set_size (int size)
{
  side_panel_set_size (panel.left_panel, size);
  central_panel_set_size (panel.central_panel, size);
  side_panel_set_size (panel.right_panel, size);

  panel.size = size;
}

void
panel_set_style (int style)
{
  side_panel_set_style (panel.left_panel, style);
  central_panel_set_style (panel.central_panel, style);
  side_panel_set_style (panel.right_panel, style);

  panel.style = style;
}

void
panel_set_popup_size (int size)
{
  side_panel_set_popup_size (panel.left_panel, size);
  side_panel_set_popup_size (panel.right_panel, size);

  panel.popup_size = size;
}

void
panel_set_left_groups (int n)
{
  side_panel_set_groups (panel.left_panel, n);

  panel.num_groups_left = n;
}

void
panel_set_right_groups (int n)
{
  side_panel_set_groups (panel.right_panel, n);

  panel.num_groups_right = n;
}

/*****************************************************************************/

static void
xfce_init (int argc, char **argv)
{
  create_atoms ();
  net_wm_support_check ();
  create_pixbufs ();
  set_module_names ();
}

static void
xfce_run (void)
{
  panel_init ();

  create_xfce_panel ();
  gtk_main ();
}

void
quit (void)
{
  gtk_main_quit ();

  panel_cleanup ();
  gtk_widget_destroy (panel.window);
}

void
restart (void)
{
  gtk_main_quit ();

  panel_cleanup ();
  gtk_widget_destroy (panel.window);

  g_printerr ("Restarting panel\n");
  xfce_run ();
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);
  xfce_init (argc, argv);

  xfce_run ();

  return 0;
}

/*****************************************************************************/

void
change_current_desktop (int n)
{
  if (panel.current_screen != n)
    {
      if (n < 0)
	n = 0;

      if (n > panel.num_screens - 1)
	n = panel.num_screens - 1;

      net_current_desktop_set (n);
    }
  else
    central_panel_set_current (panel.central_panel, panel.current_screen);
}

void
change_number_of_desktops (int n)
{
  if (panel.num_screens != n)
    net_number_of_desktops_set (n);
}

void
change_desktop_name (int n, char *name)
{
}

void
current_desktop_changed (int n)
{
  if (panel.current_screen != n)
    {
      central_panel_set_current (panel.central_panel, n);
      panel.current_screen = n;
    }
}

void
number_of_desktops_changed (int n)
{
  if (panel.num_screens != n)
    {
      central_panel_set_screens (panel.central_panel, n);
      panel.num_screens = n;
    }
}

void
desktop_name_changed (int n, char *name)
{
}
