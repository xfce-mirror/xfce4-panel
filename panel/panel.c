/*  $Id$
 *
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <X11/Xlib.h>

#include <gmodule.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libnetk.h>

#include "xfce.h"
#include "groups.h"
#include "controls.h"
#include "controls_dialog.h"
#include "add-control-dialog.h"
#include "popup.h"
#include "settings.h"
#include "mcs_client.h"
#include "icons.h"

#define HIDE_TIMEOUT	500
#define UNHIDE_TIMEOUT	100
#define HIDDEN_SIZE	3
#define HANDLE_WIDTH	10
#define SNAP_WIDTH	25


/* typedefs *
 * -------- */

struct _PanelPrivate
{
    Settings settings;

    GdkScreen *screen;
    int monitor;
    GdkRectangle monitor_geometry;

    int side;
    XfcePositionState pos_state;
    int offset;

    GtkRequisition req;

    gboolean is_created;
    int block_autohide;
};


/* exported globals *
 * ---------------- */

/* FIXME: get rid of these */
G_MODULE_EXPORT /* EXPORT:settings */
Settings settings;
G_MODULE_EXPORT /* EXPORT:panel */
Panel panel;

/**
 * These sizes are exported to all other modules.
 * Arrays are indexed by symbolic sizes TINY, SMALL, MEDIUM, LARGE
 * (see global.h).
 **/
G_MODULE_EXPORT /* EXPORT:icon_size */
int icon_size[] = { 24, 30, 45, 60 };

G_MODULE_EXPORT /* EXPORT:border_width */
int border_width = 4;

G_MODULE_EXPORT /* EXPORT:top_height */
int top_height[] = { 14, 16, 18, 20 };

G_MODULE_EXPORT /* EXPORT:popup_icon_size */
int popup_icon_size[] = { 22, 26, 26, 32 };

/* static prototypes *
 * ----------------- */

static void update_partial_struts (Panel * p);

static void update_arrow_direction (Panel * p);

static void update_xinerama_coordinates (Panel * p, int x, int y);

static void restrict_position (Panel * p, int *x, int *y);

static void panel_set_position (Panel * p);

static void panel_set_hidden (Panel * p, gboolean hide);

static void init_settings (Panel * p);


/* positioning and position related functions *
 * ------------------------------------------ */

static void
set_opacity (Panel * p, gboolean translucent)
{
    guint opacity;

    opacity = (translucent ? 0xc0000000 : 0xffffffff);
    gdk_error_trap_push ();

    gdk_property_change (p->toplevel->window,
			 gdk_atom_intern ("_NET_WM_WINDOW_OPACITY", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & opacity, 1L);

    gdk_error_trap_pop ();
    
}

static void
update_partial_struts (Panel * p)
{
    gulong data[12] = { 0, };

    if (!p->priv->settings.autohide && p->priv->settings.layer == 0)
    {
	int w, h, x, y;

	x = p->position.x;
	y = p->position.y;
	w = p->priv->req.width;
	h = p->priv->req.height;

	switch (p->priv->side)
	{
	    case LEFT:
		data[0] = w;	        /* left           */

		data[4] = y;	        /* left_start_y   */
		data[5] = y + h;	/* left_end_y     */
		break;
	    case RIGHT:
		data[1] = w;	        /* right          */

		data[6] = y;	        /* right_start_y  */
		data[7] = y + h;	/* right_end_y    */
		break;
	    case TOP:
		data[2] = h;	        /* top            */

		data[8] = x;	        /* top_start_x    */
		data[9] = x + w;	/* top_end_x      */
		break;
	    default:
		data[3] = h;	        /* bottom         */

		data[10] = x;	        /* bottom_start_x */
		data[11] = x + w;	/* bottom_end_x   */
	}
    }

    DBG (" ** struts: "
	 "%ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n",
	 data[0], data[1], data[2], data[3], data[4], data[5],
	 data[6], data[7], data[8], data[9], data[10], data[11]);

    gdk_error_trap_push ();

    gdk_property_change (p->toplevel->window,
			 gdk_atom_intern ("_NET_WM_STRUT_PARTIAL", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & data, 12);

    gdk_error_trap_pop ();
}

static void
update_arrow_direction (Panel * p)
{
    GtkArrowType type;

    switch (p->priv->side)
    {
	case LEFT:
	    type = GTK_ARROW_RIGHT;
	    break;
	case RIGHT:
	    type = GTK_ARROW_LEFT;
	    break;
	case TOP:
	    type = GTK_ARROW_DOWN;
	    break;
	default:
	    type = GTK_ARROW_UP;
    }

    groups_set_arrow_direction (type);
}

static void
update_xinerama_coordinates (Panel * p, int x, int y)
{
    int monitor;

    monitor = gdk_screen_get_monitor_at_point (p->priv->screen, x, y);

    if (monitor != p->priv->monitor)
    {
	p->priv->monitor = monitor;

	gdk_screen_get_monitor_geometry (p->priv->screen, p->priv->monitor,
					 &(p->priv->monitor_geometry));

	DBG ("\n  monitor: %d\n  geometry: %d,%d %dx%d",
	     p->priv->monitor,
	     p->priv->monitor_geometry.x,
	     p->priv->monitor_geometry.y,
	     p->priv->monitor_geometry.width,
	     p->priv->monitor_geometry.height);
    }
}

static void
restrict_position (Panel * p, int *x, int *y)
{
    int xcenter, ycenter;

    if (!p || !p->toplevel)
	return;

    DBG ("\n  desired position: %d,%d\n  size: %d x %d", *x, *y,
	 p->priv->req.width, p->priv->req.height);

    xcenter = p->priv->monitor_geometry.x
	+ p->priv->monitor_geometry.width / 2 - p->priv->req.width / 2;

    ycenter = p->priv->monitor_geometry.y
	+ p->priv->monitor_geometry.height / 2 - p->priv->req.height / 2;

    p->priv->pos_state = XFCE_POS_STATE_NONE;
    p->priv->offset = 0;

    if (p->priv->settings.orientation == HORIZONTAL)
    {
	if (*y < ycenter)
	{
	    *y = p->priv->monitor_geometry.y;

	    p->priv->side = TOP;
	}
	else
	{
	    *y = p->priv->monitor_geometry.y
		+ p->priv->monitor_geometry.height - p->priv->req.height;

	    p->priv->side = BOTTOM;
	}

	/* center */
	if (*x > xcenter - SNAP_WIDTH && *x < xcenter + SNAP_WIDTH)
	{
	    *x = xcenter;

	    p->priv->pos_state = XFCE_POS_STATE_CENTER;
	}
	/* right edge */
	else if (*x + p->priv->req.width > p->priv->monitor_geometry.x
		 + p->priv->monitor_geometry.width - SNAP_WIDTH)
	{
	    *x = p->priv->monitor_geometry.x
		+ p->priv->monitor_geometry.width - p->priv->req.width;

	    p->priv->pos_state = XFCE_POS_STATE_END;
	}

	/* left edge */
	if (*x < p->priv->monitor_geometry.x + SNAP_WIDTH)
	{
	    *x = p->priv->monitor_geometry.x;

	    p->priv->pos_state = XFCE_POS_STATE_START;
	}

	p->priv->offset = *x - p->priv->monitor_geometry.x;
    }
    else
    {
	if (*x < xcenter)
	{
	    *x = p->priv->monitor_geometry.x;

	    p->priv->side = LEFT;
	}
	else
	{
	    *x = p->priv->monitor_geometry.x
		+ p->priv->monitor_geometry.width - p->priv->req.width;

	    p->priv->side = RIGHT;
	}

	/* center */
	if (*y > ycenter - SNAP_WIDTH && *y < ycenter + SNAP_WIDTH)
	{
	    *y = ycenter;

	    p->priv->pos_state = XFCE_POS_STATE_CENTER;
	}
	/* bottom edge */
	else if (*y + p->priv->req.height > p->priv->monitor_geometry.y
		 + p->priv->monitor_geometry.height - SNAP_WIDTH)
	{
	    *y = p->priv->monitor_geometry.y
		+ p->priv->monitor_geometry.height - p->priv->req.height;

	    p->priv->pos_state = XFCE_POS_STATE_END;
	}

	/* top edge */
	if (*y < p->priv->monitor_geometry.y + SNAP_WIDTH)
	{
	    *y = p->priv->monitor_geometry.y;

	    p->priv->pos_state = XFCE_POS_STATE_START;
	}

	p->priv->offset = *y - p->priv->monitor_geometry.y;
    }

    DBG ("++ position: %d, %d\n", p->position.x, p->position.y);
    DBG ("   monitor: %d\n", p->priv->monitor);
    DBG ("   side: %s\n",
	 p->priv->side == LEFT ? "left" :
	 p->priv->side == RIGHT ? "right" :
	 p->priv->side == TOP ? "top" : "bottom");
    DBG ("   state: %s\n",
	 p->priv->pos_state == XFCE_POS_STATE_CENTER ? "center" :
	 p->priv->pos_state == XFCE_POS_STATE_START ? "start" :
	 p->priv->pos_state == XFCE_POS_STATE_END ? "end" : "none");
}

static void
panel_move_func (GtkWidget * win, int *x, int *y, Panel * panel)
{
    static int num_screens = 0;
    int side;

    if (G_UNLIKELY (num_screens == 0))
    {
	num_screens = gdk_screen_get_n_monitors (panel->priv->screen);
    }

    if (G_UNLIKELY (num_screens > 1))
    {
	int xcenter, ycenter;

	xcenter = *x + panel->priv->req.width / 2;
	ycenter = *y + panel->priv->req.height / 2;

	update_xinerama_coordinates (panel, xcenter, ycenter);
    }

    side = panel->priv->side;

    restrict_position (panel, x, y);

    if (side != panel->priv->side)
	update_arrow_direction (panel);
}

static void
panel_set_position (Panel * p)
{
    if (!p->priv->is_created || p->hidden)
	return;

    gtk_widget_size_request (p->toplevel, &(p->priv->req));

    if (p->priv->settings.orientation == VERTICAL)
    {
	if (p->priv->side == LEFT)
	{
	    p->position.x = p->priv->monitor_geometry.x;
	}
	else
	{
	    p->position.x = p->priv->monitor_geometry.x
		+ p->priv->monitor_geometry.width - p->priv->req.width;
	}

	switch (p->priv->pos_state)
	{
	    case XFCE_POS_STATE_CENTER:
		p->position.y = p->priv->monitor_geometry.y
		    + (p->priv->monitor_geometry.height
		       - p->priv->req.height) / 2;
		break;
	    case XFCE_POS_STATE_START:
		p->position.y = p->priv->monitor_geometry.y;
		break;
	    case XFCE_POS_STATE_END:
		p->position.y = p->priv->monitor_geometry.y
		    + p->priv->monitor_geometry.height - p->priv->req.height;
		break;
	    default:
		p->position.y = p->priv->monitor_geometry.y + p->priv->offset;
	}
    }
    else
    {
	if (p->priv->side == TOP)
	{
	    p->position.y = p->priv->monitor_geometry.y;
	}
	else
	{
	    p->position.y = p->priv->monitor_geometry.y
		+ p->priv->monitor_geometry.height - p->priv->req.height;
	}

	switch (p->priv->pos_state)
	{
	    case XFCE_POS_STATE_CENTER:
		p->position.x = p->priv->monitor_geometry.x
		    + (p->priv->monitor_geometry.width
		       - p->priv->req.width) / 2;
		break;
	    case XFCE_POS_STATE_START:
		p->position.x = p->priv->monitor_geometry.x;
		break;
	    case XFCE_POS_STATE_END:
		p->position.x = p->priv->monitor_geometry.x
		    + p->priv->monitor_geometry.width - p->priv->req.width;
		break;
	    default:
		p->position.x = p->priv->monitor_geometry.x + p->priv->offset;
	}
    }

    DBG (" ++ position: %d, %d\n", p->position.x, p->position.y);
    DBG ("    monitor: %d\n", p->priv->monitor);
    DBG ("    side: %s\n",
	 p->priv->side == LEFT ? "left" :
	 p->priv->side == RIGHT ? "right" :
	 p->priv->side == TOP ? "top" : "bottom");
    DBG ("    state: %s\n",
	 p->priv->pos_state == XFCE_POS_STATE_CENTER ? "center" :
	 p->priv->pos_state == XFCE_POS_STATE_START ? "start" :
	 p->priv->pos_state == XFCE_POS_STATE_END ? "end" : "none");

    gtk_window_move (GTK_WINDOW (p->toplevel), p->position.x, p->position.y);

    update_arrow_direction (p);

    write_panel_config ();

    update_partial_struts (p);
}

static void
screen_size_changed (GdkScreen * screen, Panel * p)
{
    GdkRectangle r;
    double scale;

    r = p->priv->monitor_geometry;

    gdk_screen_get_monitor_geometry (screen, p->priv->monitor,
				     &(p->priv->monitor_geometry));

    if (p->priv->settings.orientation == VERTICAL)
    {
	scale = (double) (p->position.y - r.y) / (double) r.height;

	p->priv->offset = rint (scale * p->priv->monitor_geometry.height);
    }
    else
    {
	scale = (double) (p->position.x - r.x) / (double) r.width;

	p->priv->offset = rint (scale * p->priv->monitor_geometry.width);
    }

    panel_set_position (p);
}

/*  Move handle menu
 *  ----------------
*/
static void
run_add_control_dialog (void)
{
    add_control_dialog (&panel, -1);
}

static void
edit_prefs (void)
{
    mcs_dialog (NULL);
}

static void
lock_screen (void)
{
    exec_cmd ("xflock4", FALSE, FALSE);
}

static void
restart_panel (void)
{
    restart ();
}

static void
exit_panel (void)
{
    quit (FALSE);
}

static void
do_info (void)
{
    exec_cmd ("xfce4-about", FALSE, FALSE);
}

static void
do_help (void)
{
    exec_cmd ("xfhelp4", FALSE, FALSE);
}

static GtkMenu *
get_handle_menu (void)
{
    static GtkWidget *menu = NULL;

    if (!menu)
    {
	GtkWidget *mi;

        xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

	menu = gtk_menu_new ();

	mi = gtk_menu_item_new_with_label (_("Xfce Panel"));
	gtk_widget_show (mi);
	gtk_widget_set_sensitive (mi, FALSE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("Add _new item"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (run_add_control_dialog),
			  NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Properties..."));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (edit_prefs), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_About Xfce"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (do_info), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (do_help), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Lock screen"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (lock_screen), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("_Restart"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (restart_panel), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_widget_show (mi);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_menu_item_new_with_mnemonic (_("E_xit"));
	gtk_widget_show (mi);
	g_signal_connect (mi, "activate", G_CALLBACK (exit_panel), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

    g_assert (menu != NULL);

    /* when XFCE_DISABLE_USER_CONFIG is set, hide 3rd, 4th and 5th item;
     * keep in sync with factory. */
    if (G_UNLIKELY (disable_user_config))
    {
	GList *l;
	GtkWidget *item;

	l = g_list_nth (GTK_MENU_SHELL (menu)->children, 2);
	item = l->data;
	gtk_widget_hide (item);

	l = l->next;
	item = l->data;
	gtk_widget_hide (item);

	l = l->next;
	item = l->data;
	gtk_widget_hide (item);
    }

    return GTK_MENU (menu);
}

/*  Move handles
 *  ------------
*/
static void
handle_set_size (GtkWidget * mh, int size)
{
    if (settings.orientation == VERTICAL)
	gtk_widget_set_size_request (mh, -1, HANDLE_WIDTH);
    else
	gtk_widget_set_size_request (mh, HANDLE_WIDTH, -1);
}

/* defined in controls.c, must be set to NULL to indicate the menu
 * is not being popped up from a panel item */
extern Control *popup_control;

static gboolean
handler_pressed_cb (GtkWidget * h, GdkEventButton * event)
{
    hide_current_popup_menu ();

    if (event->button == 3 ||
	(event->button == 1 && event->state & GDK_SHIFT_MASK))
    {
	GtkMenu *menu;

	popup_control = NULL;
	menu = get_handle_menu ();

	panel_register_open_menu (GTK_WIDGET (menu));

	gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button,
			event->time);

	return TRUE;
    }

    /* let default handler run */
    return FALSE;
}

static void
handler_move_end_cb (GtkWidget * h, Panel * p)
{
    int x, y;

    gtk_window_get_position (GTK_WINDOW (panel.toplevel),
			     &p->position.x, &p->position.y);

    DBG ("move end: %d,%d", p->position.x, p->position.y);

    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);

    x = p->position.x;
    y = p->position.y;

    restrict_position (p, &x, &y);

    if (x != p->position.x || y != p->position.y)
    {
	gtk_window_move (GTK_WINDOW (p->toplevel), x, y);
	p->position.x = x;
	p->position.y = y;

	update_arrow_direction (p);
    }

    write_panel_config ();

    update_partial_struts (p);
}

static void
handler_move_start (Panel * p)
{
    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);
}

G_MODULE_EXPORT /* EXPORT:handle_new */
GtkWidget *
handle_new (Panel * p)
{
    GtkWidget *mh;

    mh = xfce_movehandler_new (panel.toplevel);
    xfce_movehandler_set_move_func (XFCE_MOVEHANDLER (mh),
				    (XfceMoveFunc) panel_move_func, p);
    gtk_widget_show (mh);

    gtk_widget_set_name (mh, "xfce_panel");

    g_signal_connect (mh, "button-press-event",
		      G_CALLBACK (handler_pressed_cb), p);

    g_signal_connect_swapped (mh, "move-start",
			      G_CALLBACK (handler_move_start), p);

    g_signal_connect (mh, "move-end", G_CALLBACK (handler_move_end_cb), p);

    /* protect against destruction when removed from box */
    g_object_ref (mh);

    handle_set_size (mh, settings.size);

    return mh;
}

/*  Autohide
 *  --------
*/
extern PanelPopup *open_popup;

static void
panel_set_hidden (Panel * p, gboolean hide)
{
    int x, y, w, h;

    /* set flag before moving when hiding */
    if (hide)
	p->hidden = hide;

    x = p->position.x;
    y = p->position.y;
    w = p->priv->req.width;
    h = p->priv->req.height;

    if (hide)
    {
	/* Depending on orientation, resize */
	switch (p->priv->side)
	{
	    case LEFT:
		x = p->priv->monitor_geometry.x;
		w = HIDDEN_SIZE;
		break;
	    case RIGHT:
		x = p->priv->monitor_geometry.x
		    + p->priv->monitor_geometry.width - HIDDEN_SIZE;
		w = HIDDEN_SIZE;
		break;
	    case TOP:
		y = p->priv->monitor_geometry.y;
		h = HIDDEN_SIZE;
		break;
	    default:
		y = p->priv->monitor_geometry.y
		    + p->priv->monitor_geometry.height - HIDDEN_SIZE;
		h = HIDDEN_SIZE;
	}
    }

    if (hide)
    {
	gtk_widget_hide (p->main_frame);
	gtk_widget_set_size_request (p->toplevel, w, h);
    }
    else
    {
	gtk_widget_set_size_request (p->toplevel, -1, -1);
	gtk_widget_show (p->main_frame);
    }

    DBG ("%s: (%d,%d) %dx%d", hide ? "hide" : "unhide", x, y, w, h);

    /* this seems to be necessary to be able to move the window ... */
    while (gtk_events_pending ())
	gtk_main_iteration_do (FALSE);

    gdk_window_move_resize (p->toplevel->window, x, y, w, h);

    /* set flag after moving when unhiding */
    if (!hide)
	p->hidden = hide;
}

static gboolean
panel_hide_timeout (Panel * p)
{
    /* if popup is open, keep trying */
    if (open_popup || p->priv->block_autohide > 0)
	return TRUE;

    if (!p->hidden)
    {
	panel_set_hidden (p, TRUE);
    }
    else
	DBG ("already hidden");

    return FALSE;
}

static gboolean
panel_unhide_timeout (Panel * p)
{
    if (p->hidden)
    {
	panel_set_hidden (p, FALSE);
    }

    return FALSE;
}

static gboolean
panel_enter (GtkWindow * w, GdkEventCrossing * event, Panel * p)
{
    if (!(settings.autohide))
	return FALSE;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
	if (p->hide_timeout)
	{
	    g_source_remove (p->hide_timeout);
	    p->hide_timeout = 0;
	}

	if (!p->unhide_timeout)
	{
	    p->unhide_timeout =
		g_timeout_add (UNHIDE_TIMEOUT,
			       (GSourceFunc) panel_unhide_timeout, p);
	}
    }

    return FALSE;
}

static gboolean
panel_leave (GtkWidget * w, GdkEventCrossing * event, Panel * p)
{
    if (!(settings.autohide))
	return FALSE;

    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
	if (p->unhide_timeout)
	{
	    g_source_remove (p->unhide_timeout);
	    p->unhide_timeout = 0;
	}

	if (!p->hide_timeout)
	{
	    p->hide_timeout =
		g_timeout_add (HIDE_TIMEOUT, (GSourceFunc) panel_hide_timeout,
			       p);
	}
    }

    return FALSE;
}

/* unhide when receiving drag data */
static GtkTargetEntry entry[] = {
    {"text/uri-list", 0, 0},
    {"STRING", 0, 1}
};

static gboolean
drag_motion (GtkWidget * widget, GdkDragContext * context,
	     int x, int y, guint time, Panel * p)
{
    if (!p->hidden)
	return TRUE;

    if (p->hide_timeout)
    {
	g_source_remove (p->hide_timeout);
	p->hide_timeout = 0;
    }

    if (!p->unhide_timeout)
    {
	p->unhide_timeout =
	    g_timeout_add (UNHIDE_TIMEOUT,
			   (GSourceFunc) panel_unhide_timeout, p);
    }

    return TRUE;
}

/*  Panel framework
 *  ---------------
*/
static gboolean
panel_delete_cb (GtkWidget * window, GdkEvent * ev, gpointer data)
{
    quit (FALSE);

    return TRUE;
}

static void
panel_allocate_cb (GtkWidget * window, GtkAllocation * allocation, Panel * p)
{
    if (p->hidden)
	return;

    if (allocation->width != p->priv->req.width
	|| allocation->height != p->priv->req.height)
    {
	panel_set_position (p);
    }
}

static GtkWidget *
create_panel_window (Panel * p)
{
    GtkWidget *w;
    GtkWindow *window;
    GdkPixbuf *pb;

    w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    window = GTK_WINDOW (w);

    gtk_window_set_title (window, _("Xfce Panel"));
    gtk_window_set_decorated (window, FALSE);
    gtk_window_set_resizable (window, FALSE);
    gtk_window_stick (window);

    gtk_window_set_skip_taskbar_hint (window, TRUE);
    gtk_window_set_skip_pager_hint (window, TRUE);

    gtk_window_set_gravity (window, GDK_GRAVITY_STATIC);

    pb = get_panel_pixbuf ();
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    g_signal_connect (w, "delete-event", G_CALLBACK (panel_delete_cb), p);

    g_object_set_data (G_OBJECT (w), "panel", p);

    return w;
}

static void
create_panel_framework (Panel * p)
{
    gboolean vertical = (settings.orientation == VERTICAL);

    /* main frame */
    p->main_frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (p->main_frame), GTK_SHADOW_OUT);
    gtk_container_set_border_width (GTK_CONTAINER (p->main_frame), 0);
    gtk_widget_show (p->main_frame);
    gtk_container_add (GTK_CONTAINER (p->toplevel), p->main_frame);

    /* create all widgets that depend on orientation */
    if (vertical)
    {
	p->panel_box = gtk_vbox_new (FALSE, 0);
	p->group_box = gtk_vbox_new (FALSE, 0);
    }
    else
    {
	p->panel_box = gtk_hbox_new (FALSE, 0);
	p->group_box = gtk_hbox_new (FALSE, 0);
    }

    /* show them */
    gtk_widget_show (p->panel_box);
    gtk_widget_show (p->group_box);

    /* create handles */
    p->handles[LEFT] = handle_new (p);
    p->handles[RIGHT] = handle_new (p);

    /* pack the widgets into the main frame */
    gtk_container_add (GTK_CONTAINER (p->main_frame), p->panel_box);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->handles[LEFT], FALSE,
			FALSE, 0);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->group_box, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->handles[RIGHT], FALSE,
			FALSE, 0);
}

G_MODULE_EXPORT /* EXPORT:panel_cleanup */
void
panel_cleanup (void)
{
    groups_cleanup ();
}

G_MODULE_EXPORT /* EXPORT:create_panel */
void
create_panel (void)
{
    Panel *p;

    /* FIXME: get rid of global panel variable */
    p = &panel;

    p->toplevel = NULL;

    p->position.x = -1;
    p->position.y = -1;

    p->hidden = FALSE;
    p->hide_timeout = 0;
    p->unhide_timeout = 0;

    p->priv = g_new0 (PanelPrivate, 1);

    /* toplevel window */
    p->toplevel = create_panel_window (p);

    g_object_add_weak_pointer (G_OBJECT (p->toplevel),
			       (gpointer *) & (p->toplevel));

    /* Connect signalers to window for autohide */
    g_signal_connect (p->toplevel, "enter-notify-event",
		      G_CALLBACK (panel_enter), p);

    g_signal_connect (p->toplevel, "leave-notify-event",
		      G_CALLBACK (panel_leave), p);

    gtk_drag_dest_set (p->toplevel, GTK_DEST_DEFAULT_ALL, entry, 2,
		       GDK_ACTION_COPY);

    g_signal_connect (p->toplevel, "drag-motion",
		      G_CALLBACK (drag_motion), p);

    p->priv->screen = gtk_widget_get_screen (p->toplevel);
    p->priv->monitor = -1;

    /* fill in the 'settings' structure */
    init_settings (p);
    get_global_prefs ();

    /* If there is a settings manager it takes precedence */
    mcs_watch_xfce_channel ();

    icon_theme_init ();
    
    /* backwards compat */
    p->priv->settings = settings;

    /* panel framework */
    create_panel_framework (p);

    groups_init (GTK_BOX (p->group_box));

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config ();

    if (!GTK_WIDGET_REALIZED (p->toplevel))
	gtk_widget_realize (p->toplevel);

    p->priv->is_created = TRUE;

    panel_set_position (p);

    DBG (" ++ position: %d, %d\n", panel.position.x, panel.position.y);

    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);

    gtk_widget_show (p->toplevel);

    /* set layer on visible window */
    panel_set_layer (p->priv->settings.layer);

    /* size sometimes changes after showing toplevel */
    panel_set_position (p);

    /* recalculate pos_state for old API where only x and y coordinates are
     * read from the config file */
    restrict_position (p, &(p->position.x), &(p->position.y));
    gtk_window_move (GTK_WINDOW (p->toplevel), p->position.x, p->position.y);

    if (p->priv->settings.autohide)
	panel_set_autohide (TRUE);

    update_partial_struts (p);

    update_arrow_direction (p);

    /* auto resize functions */
    g_signal_connect (p->toplevel, "size-allocate",
		      G_CALLBACK (panel_allocate_cb), p);

    g_signal_connect (p->priv->screen, "size-changed",
		      G_CALLBACK (screen_size_changed), p);
}

/*  Panel settings
 *  --------------
*/
G_MODULE_EXPORT /* EXPORT:panel_set_orientation */
void
panel_set_orientation (int orientation)
{
    gboolean hidden;
    int pos;

    panel.priv->settings.orientation = orientation;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    hide_current_popup_menu ();

    hidden = settings.autohide;
    if (hidden)
    {
	panel_set_autohide (FALSE);
    }

    gtk_widget_hide (panel.toplevel);

    /* change popup position to make it look better 
     * done here, because it's needed for size calculation 
     * the setttings dialog will also change, we just 
     * anticipate that ;-)
     */
    pos = settings.popup_position;
    switch (pos)
    {
	case LEFT:
	    pos = TOP;
	    break;
	case RIGHT:
	    pos = BOTTOM;
	    break;
	case TOP:
	    pos = LEFT;
	    break;
	case BOTTOM:
	    pos = RIGHT;
	    break;
    }
    panel_set_popup_position (pos);

    /* save panel controls */
    groups_unpack ();

    /* no need to recreate the window */
    gtk_widget_destroy (panel.main_frame);
    create_panel_framework (&panel);

    groups_pack (GTK_BOX (panel.group_box));
    groups_set_orientation (orientation);

    /* prevent 'size-allocate' handler from wrongly adjusting 
     * the position */
    panel.priv->req.width = panel.priv->req.height = 0;

    panel_set_size (settings.size);

    gtk_widget_size_request (panel.toplevel, &panel.priv->req);

    /* calls panel_set_position () */
    if (orientation == HORIZONTAL)
	panel_center (BOTTOM);
    else
	panel_center (RIGHT);

    gtk_widget_show_now (panel.toplevel);

    /* size sometimes changes after showing */
    panel_set_position (&panel);

    gtk_window_stick (GTK_WINDOW (panel.toplevel));

    panel_set_layer (panel.priv->settings.layer);

    if (hidden)
	panel_set_autohide (TRUE);

    update_partial_struts (&panel);
}

G_MODULE_EXPORT /* EXPORT:panel_set_layer */
void
panel_set_layer (int layer)
{
    panel.priv->settings.layer = layer;

    /* backwards compat */
    settings = panel.priv->settings;

    if (panel.priv->is_created)
    {
	set_window_layer (panel.toplevel, layer);

	if (layer == ABOVE)
	    gtk_window_present (GTK_WINDOW (panel.toplevel));

	update_partial_struts (&panel);
        set_opacity (&panel, (layer == ABOVE));

#if 0
	/* dock type hint */
	gboolean autohide = panel.priv->settings.autohide;

	if (autohide)
	    panel_set_autohide (FALSE);

	gtk_widget_hide (panel.toplevel);
	gtk_widget_unrealize (panel.toplevel);

	gtk_window_set_type_hint (GTK_WINDOW (panel.toplevel),
				  layer == 0 ? GDK_WINDOW_TYPE_HINT_DOCK
				  : GDK_WINDOW_TYPE_HINT_NORMAL);

	/* position and properties are lost by unrealizing */
	gtk_window_move (GTK_WINDOW (panel.toplevel),
			 panel.position.x, panel.position.y);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (panel.toplevel), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (panel.toplevel), TRUE);
	gtk_window_stick (GTK_WINDOW (panel.toplevel));

	/* make sure it is really shown before setting struts */
	gtk_widget_show_now (panel.toplevel);

	if (autohide)
	    panel_set_autohide (TRUE);

#endif
    }
}

G_MODULE_EXPORT /* EXPORT:panel_set_size */
void
panel_set_size (int size)
{
    panel.priv->settings.size = size;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    hide_current_popup_menu ();

    groups_set_size (size);
    handle_set_size (panel.handles[LEFT], size);
    handle_set_size (panel.handles[RIGHT], size);

    /* this will also resize the icons */
    panel_set_theme (panel.priv->settings.theme);
}

G_MODULE_EXPORT /* EXPORT:panel_set_popup_position */
void
panel_set_popup_position (int position)
{
    panel.priv->settings.popup_position = position;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    hide_current_popup_menu ();

    groups_set_popup_position (position);
    update_arrow_direction (&panel);

    /* this is necessary to get the right proportions */
    panel_set_size (settings.size);
}

G_MODULE_EXPORT /* EXPORT:panel_set_theme */
void
panel_set_theme (const char *theme)
{
    g_free (panel.priv->settings.theme);

    if (theme)
        panel.priv->settings.theme = g_strdup (theme);
    else
        panel.priv->settings.theme = NULL;

    /* backwards compat */
    settings = panel.priv->settings;

    if (panel.priv->is_created)
    {
	groups_set_theme (theme);
    }
}

G_MODULE_EXPORT /* EXPORT:panel_set_settings */
void
panel_set_settings (void)
{
    panel_set_size (settings.size);
    panel_set_popup_position (settings.popup_position);

    panel_set_theme (settings.theme);
}

G_MODULE_EXPORT /* EXPORT:panel_center */
void
panel_center (int side)
{
    panel.priv->side = side;
    panel.priv->pos_state = XFCE_POS_STATE_CENTER;
    panel.priv->offset = 0;

    panel_set_position (&panel);
}

G_MODULE_EXPORT /* EXPORT:panel_set_autohide */
void
panel_set_autohide (gboolean hide)
{
    panel.priv->settings.autohide = hide;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    if (hide)
    {
	DBG ("add hide timeout");

	panel.hide_timeout = g_timeout_add (1000,
					    (GSourceFunc) panel_hide_timeout,
					    &panel);
    }
    else
    {
	panel_set_hidden (&panel, hide);
    }

    update_partial_struts (&panel);
}

/*  Global preferences
 *  ------------------
*/
static void
init_settings (Panel * p)
{
    p->priv->settings.orientation = HORIZONTAL;
    p->priv->settings.layer = ABOVE;

    p->priv->settings.size = SMALL;
    p->priv->settings.popup_position = RIGHT;

    p->priv->settings.autohide = FALSE;

    p->priv->settings.theme = NULL;

    /* backwards compat */
    settings = p->priv->settings;
}

G_MODULE_EXPORT /* EXPORT:panel_parse_xml */
void
panel_parse_xml (xmlNodePtr node)
{
    xmlChar *value;
    xmlNodePtr child;
    int n;

    /* properties */
    value = xmlGetProp (node, (const xmlChar *) "orientation");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	settings.orientation = (n == VERTICAL) ? VERTICAL : HORIZONTAL;
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "layer");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	settings.layer = n == 1 ? 1 : 0;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "size");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	if (n >= TINY && n <= LARGE)
	    settings.size = n;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "popupposition");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	if (n >= LEFT && n <= BOTTOM)
	    settings.popup_position = n;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "icontheme");

    if (value)
    {
	settings.theme = g_strdup (value);

	g_free (value);
    }

    panel.priv->monitor = -1;
    
    /* child nodes */
    for (child = node->children; child; child = child->next)
    {
	if (xmlStrEqual (child->name, (const xmlChar *) "Position"))
	{
	    /* new API */
	    value = xmlGetProp (child, (const xmlChar *) "monitor");

	    if (value)
	    {
		int num_screens =
		    gdk_screen_get_n_monitors (panel.priv->screen);

		n = (int) strtol (value, NULL, 0);

		if (n >= 0 && n < num_screens)
		    panel.priv->monitor = n;

		g_free (value);

		value = xmlGetProp (child, (const xmlChar *) "side");

		if (value)
		{
		    n = (int) strtol (value, NULL, 0);

		    if (n >= LEFT && n <= BOTTOM)
			panel.priv->side = n;

		    g_free (value);
		}

		value = xmlGetProp (child, (const xmlChar *) "posstate");

		if (value)
		{
		    n = (int) strtol (value, NULL, 0);

		    if (n > XFCE_POS_STATE_NONE && n <= XFCE_POS_STATE_END)
			panel.priv->pos_state = n;

		    g_free (value);
		}

		if (panel.priv->pos_state != XFCE_POS_STATE_CENTER)
		{
		    value = xmlGetProp (child, (const xmlChar *) "offset");

		    if (value)
		    {
			panel.priv->offset = (int) strtol (value, NULL, 0);

			g_free (value);
		    }
		}

		gdk_screen_get_monitor_geometry (panel.priv->screen,
						 panel.priv->monitor,
						 &(panel.priv->
						   monitor_geometry));
	    }
	    else
	    {
		/* old API */
		value = xmlGetProp (child, (const xmlChar *) "x");

		if (value)
		{
		    panel.position.x = (int) strtol (value, NULL, 0);

		    g_free (value);
		}

		value = xmlGetProp (child, (const xmlChar *) "y");

		if (value)
		{
		    panel.position.y = (int) strtol (value, NULL, 0);

		    g_free (value);
		}

		if (panel.position.x != -1 && panel.position.y != -1)
		{
		    panel.priv->monitor =
			gdk_screen_get_monitor_at_point (panel.priv->screen,
							 panel.position.x,
							 panel.position.y);

		    gdk_screen_get_monitor_geometry (panel.priv->screen,
						     panel.priv->monitor,
						     &(panel.priv->
						       monitor_geometry));

		    panel.priv->pos_state = XFCE_POS_STATE_NONE;

		    if (panel.priv->settings.orientation == HORIZONTAL)
		    {
			if (panel.position.y < panel.priv->monitor_geometry.y
			    + panel.priv->monitor_geometry.height / 2)
			{
			    panel.priv->side = TOP;
			}
			else
			{
			    panel.priv->side = BOTTOM;
			}

			panel.priv->offset = panel.position.x -
			    panel.priv->monitor_geometry.x;
		    }
		    else
		    {
			if (panel.position.y < panel.priv->monitor_geometry.x
			    + panel.priv->monitor_geometry.width / 2)
			{
			    panel.priv->side = LEFT;
			}
			else
			{
			    panel.priv->side = RIGHT;
			}

			panel.priv->offset = panel.position.y -
			    panel.priv->monitor_geometry.y;
		    }
		}
	    }
	}
    }

    if (panel.priv->monitor == -1)
    {
	DBG (" ++ Center panel on 1st monitor\n");

	panel.priv->monitor = 0;
	panel.priv->side = BOTTOM;
	panel.priv->pos_state = XFCE_POS_STATE_CENTER;

	gdk_screen_get_monitor_geometry (panel.priv->screen,
					 panel.priv->monitor,
					 &(panel.priv->monitor_geometry));
    }
}

G_MODULE_EXPORT /* EXPORT:panel_write_xml */
void
panel_write_xml (xmlNodePtr root)
{
    xmlNodePtr node;
    xmlNodePtr child;
    char value[MAXSTRLEN + 1];

    node = xmlNewTextChild (root, NULL, "Panel", NULL);

    snprintf (value, 2, "%d", settings.orientation);
    xmlSetProp (node, "orientation", value);

    snprintf (value, 2, "%d", settings.layer);
    xmlSetProp (node, "layer", value);

    snprintf (value, 2, "%d", settings.size);
    xmlSetProp (node, "size", value);

    snprintf (value, 2, "%d", settings.popup_position);
    xmlSetProp (node, "popupposition", value);

    if (settings.theme)
	xmlSetProp (node, "icontheme", settings.theme);

    child = xmlNewTextChild (node, NULL, "Position", NULL);

    snprintf (value, 5, "%d", panel.priv->monitor);
    xmlSetProp (child, "monitor", value);

    snprintf (value, 5, "%d", panel.priv->side);
    xmlSetProp (child, "side", value);

    snprintf (value, 5, "%d", panel.priv->pos_state);
    xmlSetProp (child, "posstate", value);

    if (panel.priv->pos_state == XFCE_POS_STATE_NONE)
    {
	snprintf (value, 5, "%d", panel.priv->offset);
	xmlSetProp (child, "offset", value);
    }

    snprintf (value, 5, "%d", panel.position.x);
    xmlSetProp (child, "x", value);

    snprintf (value, 5, "%d", panel.position.y);
    xmlSetProp (child, "y", value);
}

/* for menus, to prevent problems with autohide */

static void
menu_destroyed (GtkWidget * menu, Panel * p)
{
    panel_unblock_autohide (p);

    if (p->priv->settings.autohide
	&& gdk_window_at_pointer (NULL, NULL) != p->toplevel->window)
    {
	GdkEvent *ev = gdk_event_new (GDK_LEAVE_NOTIFY);

	((GdkEventCrossing *) ev)->time = GDK_CURRENT_TIME;
	((GdkEventCrossing *) ev)->detail = GDK_NOTIFY_NONLINEAR;

	gtk_widget_event (p->toplevel, ev);

	gdk_event_free (ev);
    }
}

G_MODULE_EXPORT /* EXPORT:panel_register_open_menu */
void
panel_register_open_menu (GtkWidget * menu)
{
    g_return_if_fail (GTK_IS_WIDGET (menu));
 
    panel_block_autohide (&panel);
    g_signal_connect (menu, "deactivate", G_CALLBACK (menu_destroyed),
		      &panel);
}

G_MODULE_EXPORT /* EXPORT:panel_block_autohide */
void
panel_block_autohide (Panel * p)
{
    p->priv->block_autohide++;
}

G_MODULE_EXPORT /* EXPORT:panel_unblock_autohide */
void
panel_unblock_autohide (Panel * p)
{
    if (p->priv->block_autohide > 0)
	p->priv->block_autohide--;
}

G_MODULE_EXPORT /* EXPORT:panel_get_side */
int
panel_get_side (void)
{
    return panel.priv->side;
}
