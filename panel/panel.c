/*  xfce4
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
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

/*
    04/11/2003 - Erik Touve - etouve@earthlink.net
    Autohide code added.  Put Panel behind a struct.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GDK_MULTIHEAD_SAFE
#undef GDK_MULTIHEAD_SAFE
#endif

#include <math.h>

#include <X11/Xlib.h>

#include <libxfce4util/debug.h>
#include <libxfce4util/i18n.h>
#include <libxfcegui4/libnetk.h>
#include <libxfcegui4/xinerama.h>

#include "xfce.h"
#include "groups.h"
#include "controls.h"
#include "controls_dialog.h"
#include "popup.h"
#include "settings.h"
#include "mcs_client.h"

#define HIDE_TIMEOUT   500
#define UNHIDE_TIMEOUT 100
#define HIDDEN_SIZE 5

/* TODO: I didn't actually check this ... -- Jasper */
#define FRAME_IPADDING 1

/* globals */

Settings settings;
Panel panel;

/*  Panel dimensions 
 *  ----------------
 *  These sizes are exported to all other modules.
 *  Arrays are indexed by symbolic sizes TINY, SMALL, MEDIUM, LARGE
 *  (see global.h).
*/
int icon_size[] = { 24, 30, 45, 60 };

int border_width = 4;

int top_height[] = { 14, 16, 18, 20 };

int popup_icon_size[] = { 22, 26, 26, 32 };

/* lock settings update when panel is not yet (re)build */

static gboolean panel_created = FALSE;

/* screen properties */

static Display *dpy = NULL;
Screen *xscreen = NULL;
static int scr = 0;
static int screen_width = 0;
static int screen_height = 0;

/* (re)positioning */

struct XineramaScreen
{
    int xmin, ymin;
    int xmax, ymax;
};

struct XineramaScreen xinerama_scr = { 0 };

typedef enum
{
    FLOATING,
    EDGE,
    CENTER,
    CORNER
}
XfcePositionState;

static XfcePositionState pos_state = FLOATING;

static GtkRequisition panel_req = { 0 };

/* prototypes */

static void update_arrow_direction (int x, int y);
static void update_xinerama_coordinates (Panel * p);
static void update_position (Panel * p, int *x, int *y);
static void panel_reallocate (Panel * p, GtkRequisition * previous);
static void panel_set_hidden (Panel * p, gboolean hide);
static void init_settings ();


/**
 * update_arrow_direction
 * 
 * adjust arrow direction based on which quarter of the screen we
 * are (Xinerama aware, of course ;) 
 **/
static void
update_arrow_direction (int x, int y)
{
    int center;
    GtkArrowType type;

    if (settings.orientation == HORIZONTAL)
    {
	center = xinerama_scr.ymin +
	    (xinerama_scr.ymax - xinerama_scr.ymin) / 2 -
	    panel_req.height / 2;

	if (y > center)
	    type = GTK_ARROW_UP;
	else
	    type = GTK_ARROW_DOWN;
    }
    else
    {
	center = xinerama_scr.xmin +
	    (xinerama_scr.xmax - xinerama_scr.xmin) / 2 - panel_req.width / 2;

	if (x > center)
	    type = GTK_ARROW_LEFT;
	else
	    type = GTK_ARROW_RIGHT;
    }

    groups_set_arrow_direction (type);
}

static void
update_xinerama_coordinates (Panel * p)
{
    int x, y;

    g_return_if_fail (dpy != NULL);

    gtk_window_get_position (GTK_WINDOW (p->toplevel), &x, &y);
    x += p->toplevel->allocation.width / 2;
    y += p->toplevel->allocation.height / 2;

    xinerama_scr.xmin = MyDisplayX (x, y);
    xinerama_scr.xmax = MyDisplayMaxX (dpy, scr, x, y);
    xinerama_scr.ymin = MyDisplayY (x, y);
    xinerama_scr.ymax = MyDisplayMaxY (dpy, scr, x, y);

    DBG ("screen: %d,%d+%dx%d",
	 xinerama_scr.xmin, xinerama_scr.ymin,
	 xinerama_scr.xmax - xinerama_scr.xmin,
	 xinerama_scr.ymax - xinerama_scr.ymin);
}

/**
 * update_position
 * 
 * - keep inside the screen
 * - update position status (see above)
 * - if on the edge, move slightly over the edge by the width of the shadow
 *   to take advantage of Fitts law: putting the mouse pointer on the edge
 *   will hit the buttons, instead of the window border.
 *   
 * Assumes panel_req and xinerama_scr are up-to-date.
 **/
static void
update_position (Panel * p, int *x, int *y)
{
    int xthick, ythick;

    if (!p || !p->toplevel)
	return;

    xthick = p->toplevel->style->xthickness + FRAME_IPADDING;
    ythick = p->toplevel->style->ythickness + FRAME_IPADDING;

    pos_state = FLOATING;

    DBG ("desired position: %d,%d", *x, *y);

    /* use an 'error' margin of 2 */
    if (settings.orientation == HORIZONTAL)
    {
	if (*y + panel_req.height > xinerama_scr.ymax - 2)
	{
	    *y = xinerama_scr.ymax - panel_req.height + ythick;
	    pos_state = EDGE;
	}

	if (*y < xinerama_scr.ymin + 2)
	{
	    *y = xinerama_scr.ymin - ythick;
	    pos_state = EDGE;
	}

	if (pos_state == EDGE)
	{
	    if (*x + panel_req.width > xinerama_scr.xmax - 2)
	    {
		*x = xinerama_scr.xmax - panel_req.width + xthick;
		pos_state = CORNER;
	    }

	    if (*x < xinerama_scr.xmin + 2)
	    {
		*x = xinerama_scr.xmin - xthick;
		pos_state = CORNER;
	    }
	}

	if (pos_state == EDGE)
	{
	    int xcenter = xinerama_scr.xmin +
		(xinerama_scr.xmax - xinerama_scr.xmin) / 2 -
		panel_req.width / 2;

	    if (xcenter > xinerama_scr.xmin - xthick &&
		*x > xcenter - 2 && *x < xcenter + 2)
	    {
		*x = xcenter;
		pos_state = CENTER;
	    }
	}
    }
    else
    {
	if (*x + panel_req.width > xinerama_scr.xmax - 2)
	{
	    *x = xinerama_scr.xmax - panel_req.width + xthick;
	    pos_state = EDGE;
	}

	if (*x < xinerama_scr.xmin + 2)
	{
	    *x = xinerama_scr.xmin - xthick;
	    pos_state = EDGE;
	}

	if (pos_state == EDGE)
	{
	    if (*y + panel_req.height > xinerama_scr.ymax - 2)
	    {
		*y = xinerama_scr.ymax - panel_req.height + ythick;
		pos_state = CORNER;
	    }

	    if (*y < xinerama_scr.ymin + 2)
	    {
		*y = xinerama_scr.ymin - ythick;
		pos_state = CORNER;
	    }
	}

	if (pos_state == EDGE)
	{
	    int ycenter = xinerama_scr.ymin +
		(xinerama_scr.ymax - xinerama_scr.ymin) / 2 -
		panel_req.height / 2;

	    if (ycenter > xinerama_scr.ymin - ythick &&
		*y > ycenter - 2 && *y < ycenter + 2)
	    {
		*y = ycenter;
		pos_state = CENTER;
	    }
	}
    }

    DBG ("new position: %d,%d", *x, *y);
}

static void
panel_reallocate (Panel * p, GtkRequisition * previous)
{
    GtkRequisition new;
    int xold, yold, xnew, ynew, xcenter, ycenter;

    gtk_widget_size_request (p->toplevel, &new);

    xold = xnew = p->position.x;
    yold = ynew = p->position.y;

    xcenter = xinerama_scr.xmin + (xinerama_scr.xmax - xinerama_scr.xmin) / 2;
    ycenter = xinerama_scr.ymin + (xinerama_scr.ymax - xinerama_scr.ymin) / 2;

    /* initial new coordinates depend on centering */
    if (pos_state == CENTER)
    {
	if (settings.orientation == HORIZONTAL)
	{
	    xnew = xcenter - new.width / 2;
	}
	else
	{
	    ynew = ycenter - new.height / 2;
	}
    }

    /* only right and bottom edge/corner can change */
    /* use an 'error' margin of 2 */
    if (pos_state == EDGE || pos_state == CORNER)
    {
	if (xold + previous->width > xinerama_scr.xmax - 2)
	{
	    xnew = xinerama_scr.xmax - new.width;
	}

	if (yold + previous->height > xinerama_scr.ymax - 2)
	{
	    ynew = xinerama_scr.ymax - new.height;
	}
    }

    panel_req = new;

    update_position (p, &xnew, &ynew);

    DBG ("reallocate: %d,%d+%dx%d -> %d,%d+%dx%d",
	 xold, yold, previous->width, previous->height,
	 xnew, ynew, new.width, new.height);

    if (xnew != xold || ynew != yold)
    {
	gtk_window_move (GTK_WINDOW (p->toplevel), xnew, ynew);

	p->position.x = xnew;
	p->position.y = ynew;

	/* Need to save position here... */
	write_panel_config ();

	/* FIXME: to we need to update the arrow direction here? */
    }
}

/*  Move handle menu
 *  ----------------
*/
static void
edit_prefs (void)
{
    mcs_dialog (NULL);
}

#if 0
static void
settings_mgr (void)
{
    mcs_dialog ("all");
}
#endif

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

static GtkItemFactoryEntry panel_items[] = {
    {N_("/XFce Panel"), NULL, NULL, 0, "<Title>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/Add _new item"), NULL, NULL, 0, "<Branch>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Properties..."), NULL, edit_prefs, 0, "<Item>"},
    {N_("/_About XFce"), NULL, do_info, 0, "<Item>"},
    {N_("/_Help"), NULL, do_help, 0, "<Item>"},
#if 0
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Settings manager"), NULL, settings_mgr, 0, "<Item>"},
#endif
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Lock screen"), NULL, lock_screen, 0, "<Item>"},
    {N_("/_Restart"), NULL, restart_panel, 0, "<Item>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/E_xit"), NULL, exit_panel, 0, "<Item>"},
};

static const char *
translate_menu (const char *msg)
{
#if ENABLE_NLS
    /* ensure we use the panel domain and not that of a plugin */
    return dgettext (GETTEXT_PACKAGE, msg);
#else
    return msg;
#endif
}

static GtkMenu *
get_handle_menu (void)
{
    static GtkMenu *menu = NULL;
    GtkItemFactory *ifactory;

    if (!menu)
    {
	GtkItemFactoryEntry *entries;
	int n_entries;

	ifactory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);

	gtk_item_factory_set_translate_func (ifactory,
					     (GtkTranslateFunc)
					     translate_menu, NULL, NULL);
	gtk_item_factory_create_items (ifactory, G_N_ELEMENTS (panel_items),
				       panel_items, NULL);

	n_entries = get_controls_menu_entries (&entries, "/Add new item");

	gtk_item_factory_create_items (ifactory, n_entries, entries, NULL);

/*	free_controls_menu_entries(entries, n_entries); */

	menu = GTK_MENU (gtk_item_factory_get_widget (ifactory, "<popup>"));
    }

    return menu;
}

/*  Move handles
 *  ------------
*/
static void
handle_set_size (GtkWidget * mh, int size)
{
    int h = (size == TINY) ? top_height[TINY] : top_height[SMALL];
    int w = icon_size[size] + 2 * border_width;
    gboolean vertical = settings.orientation == VERTICAL;

    if (vertical)
	gtk_widget_set_size_request (mh, w, h);
    else
	gtk_widget_set_size_request (mh, h, w);
}

static gboolean
handler_pressed_cb (GtkWidget * h, GdkEventButton * event)
{
    hide_current_popup_menu ();

    if (event->button == 3 ||
	(event->button == 1 && event->state & GDK_SHIFT_MASK))
    {
	GtkMenu *menu;

	menu = get_handle_menu ();

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

    update_xinerama_coordinates (p);

    x = panel.position.x;
    y = panel.position.y;

    update_position (p, &x, &y);

    if (x != p->position.x || y != p->position.y)
    {
	gtk_window_move (GTK_WINDOW (p->toplevel), x, y);
	p->position.x = x;
	p->position.y = y;

	/* Need to save position here... */
	write_panel_config ();
    }

    update_arrow_direction (x, y);
}

GtkWidget *
handle_new (Panel * p)
{
    GtkWidget *mh;

    mh = xfce_movehandler_new (panel.toplevel);
    gtk_widget_show (mh);

    gtk_widget_set_name (mh, "xfce_panel");

    g_signal_connect (mh, "button-press-event",
		      G_CALLBACK (handler_pressed_cb), p);

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
    Position pos = p->position;
    int x, y, w, h;

    x = pos.x;
    y = pos.y;
    w = panel_req.width;
    h = panel_req.height;

    if (hide)
    {
	/* Only hide when the panel is on the edge of the screen 
	 * Thanks to Eduard Rocatello for pointing this out */
	if (pos_state == FLOATING)
	    return;

	/* Depending on orientation, resize */
	if (settings.orientation == VERTICAL)
	{
	    if (pos.x - xinerama_scr.xmin < xinerama_scr.xmax - pos.x)
		x = pos.x;
	    else
		x = pos.x + panel_req.width - HIDDEN_SIZE + 1;

	    w = HIDDEN_SIZE;
	}
	else
	{
	    if (pos.y - xinerama_scr.ymin < xinerama_scr.ymax - pos.y)
		y = pos.y;
	    else
		y = pos.y + panel_req.height - HIDDEN_SIZE + 1;

	    h = HIDDEN_SIZE;
	}
    }
    else			/* unhide */
    {
	w = -1;
	h = -1;
    }

    if (hide)
	gtk_widget_hide (p->main_frame);
    else
	gtk_widget_show (p->main_frame);

    p->hidden = hide;

    gtk_widget_set_size_request (p->toplevel, w, h);

    while (gtk_events_pending ())
	gtk_main_iteration ();

    DBG ("move: %d,%d\n", x, y);

    /* gtk_window_present(GTK_WINDOW(p->toplevel)); */
    gtk_window_move (GTK_WINDOW (p->toplevel), x, y);
}

gboolean
panel_hide_timeout (Panel * p)
{
    /* if popup is open, keep trying */
    if (open_popup)
	return TRUE;

    if (!p->hidden)
    {
	panel_set_hidden (p, TRUE);
    }

    return FALSE;
}

gboolean
panel_unhide_timeout (Panel * p)
{
    if (p->hidden)
	panel_set_hidden (p, FALSE);

    return FALSE;
}

gboolean
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

gboolean
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

    if (panel_req.width != allocation->width ||
	panel_req.height != allocation->height)
    {
	if (!(panel_req.width == 0 && panel_req.height == 0))
	{
	    GtkRequisition prev = panel_req;

	    panel_req.width = allocation->width;
	    panel_req.height = allocation->height;

	    panel_reallocate (p, &prev);
	}
	else
	{
	    panel_req.width = allocation->width;
	    panel_req.height = allocation->height;
	}
    }
}

static void
screen_size_changed (GdkScreen * screen, Panel * p)
{
    double xalign, yalign;
    int width, height, x, y;

    width = xinerama_scr.xmax - xinerama_scr.xmin;
    height = xinerama_scr.ymax - xinerama_scr.ymin;

    xalign = (double) p->position.x / (width - panel_req.width);
    yalign = (double) p->position.y / (height - panel_req.height);

    DBG ("relative position: %.2f x %.2f", xalign, yalign);

    update_xinerama_coordinates (p);

    width = xinerama_scr.xmax - xinerama_scr.xmin;
    height = xinerama_scr.ymax - xinerama_scr.ymin;

    DBG ("new screen size: %dx%d", width, height);

    x = rint (xalign * (width - panel_req.width));
    y = rint (yalign * (height - panel_req.height));

    update_position (p, &x, &y);

    gtk_window_move (GTK_WINDOW (p->toplevel), x, y);

    p->position.x = x;
    p->position.y = y;

    write_panel_config ();
}

static GtkWidget *
create_panel_window (Panel * p)
{
    GtkWidget *w;
    GtkWindow *window;
    GdkPixbuf *pb;

    w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    window = GTK_WINDOW (w);

    gtk_window_set_title (window, _("XFce Panel"));
    gtk_window_set_decorated (window, FALSE);
    gtk_window_set_resizable (window, FALSE);
    gtk_window_stick (window);

    pb = get_panel_pixbuf ();
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    g_signal_connect (w, "delete-event", G_CALLBACK (panel_delete_cb), p);

    g_signal_connect (w, "size-allocate", G_CALLBACK (panel_allocate_cb), p);

#if GTK_CHECK_VERSION(2,2,0)
    g_signal_connect (G_OBJECT (gdk_screen_get_default ()), "size-changed",
		      G_CALLBACK (screen_size_changed), p);
#endif

    return w;
}

static void
create_panel_framework (Panel * p)
{
    gboolean vertical = (settings.orientation == VERTICAL);

    /* toplevel window */
    if (!p->toplevel)
    {
	p->toplevel = create_panel_window (p);
	g_object_add_weak_pointer (G_OBJECT (p->toplevel),
				   (gpointer *) & (p->toplevel));
    }

    /* this is necessary after a SIGHUP */
    gtk_window_stick (GTK_WINDOW (p->toplevel));

    /* Connect signalers to window for autohide */
    g_signal_connect (p->toplevel, "enter-notify-event",
		      G_CALLBACK (panel_enter), p);

    g_signal_connect (p->toplevel, "leave-notify-event",
		      G_CALLBACK (panel_leave), p);

    gtk_drag_dest_set (p->toplevel, GTK_DEST_DEFAULT_ALL, entry, 2,
		       GDK_ACTION_COPY);

    g_signal_connect (p->toplevel, "drag-motion",
		      G_CALLBACK (drag_motion), p);

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

void
panel_cleanup (void)
{
    groups_cleanup ();
}

void
create_panel (void)
{
    static gboolean need_init = TRUE;
    int x, y;
    int hidden;

    /* necessary for initial settings to do the right thing */
    panel_created = FALSE;

    if (need_init)
    {
	/* set for later use */
	/* TODO: for gtk 2.2 we could use gtk_widget_get_display() et al. */
	dpy = gdk_display;
	scr = DefaultScreen (dpy);
	xscreen = DefaultScreenOfDisplay (dpy);
	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();

	panel.toplevel = NULL;

	panel.position.x = -1;
	panel.position.y = -1;

	panel.hidden = FALSE;
	panel.hide_timeout = 0;
	panel.unhide_timeout = 0;

	/* fill in the 'settings' structure */
	init_settings ();
	get_global_prefs ();

	/* If there is a settings manager it takes precedence */
	mcs_watch_xfce_channel ();

	need_init = FALSE;
    }

    /* unhide panel initially */
    hidden = settings.autohide;
    settings.autohide = FALSE;

    /* FIXME
     * somehow the position gets set differently in the code below
     * we just save it here and restore it before reading the config
     * file */
    x = panel.position.x;
    y = panel.position.y;

    /* panel framework */
    create_panel_framework (&panel);

    groups_init (GTK_BOX (panel.group_box));

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config ();

    panel.position.x = x;
    panel.position.y = y;
    gtk_window_move (GTK_WINDOW (panel.toplevel), x, y);

    gtk_widget_show_now (panel.toplevel);
    set_window_layer (panel.toplevel, settings.layer);
    set_window_skip (panel.toplevel);

    gtk_widget_size_request (panel.toplevel, &panel_req);
    update_xinerama_coordinates (&panel);

    /* this must be before set_autohide() and after set_position()
     * otherwise the initial position will be messed up */
    panel_created = TRUE;

    panel.position.x = x;
    panel.position.y = y;
    panel_set_position ();

    if (hidden)
	panel_set_autohide (TRUE);
}

/*  Panel settings
 *  --------------
*/
void
panel_set_orientation (int orientation)
{
    gboolean hidden;
    int pos;

    settings.orientation = orientation;

    if (!panel_created)
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

    /* approximate reset panel requisition to prevent 'size-allocate' 
     * handler from wrongly adjusting the position */
    panel_req.width = panel_req.height = 0;

    /* TODO: find out why position doesn't get properly set in 
     * set_size function */
    panel.position.x = panel.position.y = -1;
    panel_set_size (settings.size);

    gtk_widget_size_request (panel.toplevel, &panel_req);
    if (orientation == HORIZONTAL)
	panel_center (BOTTOM);
    else
	panel_center (LEFT);

    gtk_widget_show_now (panel.toplevel);
    set_window_layer (panel.toplevel, settings.layer);
    set_window_skip (panel.toplevel);

    /* FIXME: the window doesn't always get moved properly by gtk
     * when hidden. */
    gtk_window_move (GTK_WINDOW (panel.toplevel),
		     panel.position.x, panel.position.y);

    if (hidden)
	panel_set_autohide (TRUE);
}

void
panel_set_layer (int layer)
{
    settings.layer = layer;

    if (!panel_created)
	return;

    set_window_layer (panel.toplevel, layer);

    if (layer == ABOVE)
	gtk_window_present (GTK_WINDOW (panel.toplevel));
}

void
panel_set_size (int size)
{
    settings.size = size;

    if (!panel_created)
	return;

    hide_current_popup_menu ();

    groups_set_size (size);
    handle_set_size (panel.handles[LEFT], size);
    handle_set_size (panel.handles[RIGHT], size);
}

void
panel_set_popup_position (int position)
{
    settings.popup_position = position;

    if (!panel_created)
	return;

    hide_current_popup_menu ();

    groups_set_popup_position (position);
    update_arrow_direction (panel.position.x, panel.position.y);

    /* this is necessary to get the right proportions */
    panel_set_size (settings.size);
}

void
panel_set_theme (const char *theme)
{
    g_free (settings.theme);
    settings.theme = g_strdup (theme);

    if (!panel_created)
	return;

    groups_set_theme (theme);
}

void
panel_set_settings (void)
{
    panel_set_size (settings.size);
    panel_set_popup_position (settings.popup_position);

    panel_set_theme (settings.theme);
}

void
panel_center (int side)
{
    int w, h;
    DesktopMargins margins;

    w = screen_width;
    h = screen_height;

    netk_get_desktop_margins (xscreen, &margins);

    switch (side)
    {
	case LEFT:
	    panel.position.x = margins.left;
	    panel.position.y = h / 2 - panel_req.height / 2;
	    break;
	case RIGHT:
	    panel.position.x = w - panel_req.width - margins.right;
	    panel.position.y = h / 2 - panel_req.height / 2;
	    break;
	case TOP:
	    panel.position.x = w / 2 - panel_req.width / 2;
	    panel.position.y = margins.top;
	    break;
	default:
	    panel.position.x = w / 2 - panel_req.width / 2;
	    panel.position.y = h - panel_req.height - margins.bottom;
    }

    if (panel.position.x < 0)
	panel.position.x = 0;

    if (panel.position.y < 0)
	panel.position.y = 0;

    panel_set_position ();
}

void
panel_set_position (void)
{
    gboolean hidden = settings.autohide;

    if (!panel_created)
	return;

    if (hidden)
    {
	DBG ("unhide panel before repositioning\n");

	panel_set_autohide (FALSE);
    }

    gtk_widget_size_request (panel.toplevel, &panel_req);

    if (panel.position.x == -1 && panel.position.y == -1)
    {
	if (settings.orientation == HORIZONTAL)
	    panel_center (BOTTOM);
	else
	    panel_center (LEFT);
    }
    else
    {
	update_position (&panel, &panel.position.x, &panel.position.y);

	DBG ("move: %d,%d", panel.position.x, panel.position.y);

	gtk_window_move (GTK_WINDOW (panel.toplevel), panel.position.x,
			 panel.position.y);

	/* Need to save position here... */
	write_panel_config ();

	update_arrow_direction (panel.position.x, panel.position.y);
    }

    if (hidden)
	panel_set_autohide (TRUE);
}

void
panel_set_autohide (gboolean hide)
{
    settings.autohide = hide;

    if (!panel_created)
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
}

/*  Global preferences
 *  ------------------
*/
static void
init_settings (void)
{
    settings.orientation = HORIZONTAL;
    settings.layer = ABOVE;

    settings.size = SMALL;
    settings.popup_position = RIGHT;

    settings.autohide = FALSE;

    settings.theme = NULL;

    settings.num_groups = 9;
}

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
	settings.orientation = atoi (value);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "layer");

    if (value)
    {
	n = atoi (value);

	if (n >= ABOVE && n <= BELOW)
	    settings.layer = n;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "size");

    if (value)
    {
	settings.size = atoi (value);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "popupposition");

    if (value)
    {
	settings.popup_position = atoi (value);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "icontheme");

    if (value)
	settings.theme = g_strdup (value);

    g_free (value);

    value = xmlGetProp (node, (const xmlChar *) "groups");

    if (value)
    {
	settings.num_groups = atoi (value);
	g_free (value);
    }
    else
    {
	value = xmlGetProp (node, (const xmlChar *) "left");

	if (value)
	{
	    settings.num_groups = atoi (value);
	    g_free (value);

	    value = xmlGetProp (node, (const xmlChar *) "right");

	    if (value)
	    {
		settings.num_groups += atoi (value);
		g_free (value);
	    }
	}
    }

    /* child nodes */
    for (child = node->children; child; child = child->next)
    {
	int w, h;

	if (xmlStrEqual (child->name, (const xmlChar *) "Position"))
	{
	    value = xmlGetProp (child, (const xmlChar *) "x");

	    if (value)
		panel.position.x = atoi (value);

	    g_free (value);

	    value = xmlGetProp (child, (const xmlChar *) "y");

	    if (value)
		panel.position.y = atoi (value);

	    g_free (value);

	    if (panel.position.x < 0 || panel.position.y < 0)
		break;

	    value = xmlGetProp (child, (const xmlChar *) "screenwidth");

	    if (value)
		w = atoi (value);
	    else
		break;

	    value = xmlGetProp (child, (const xmlChar *) "screenheight");

	    if (value)
		h = atoi (value);
	    else
		break;

	    /* this doesn't actually work completely, we need to
	     * save the panel width as well to do it right */
	    if (w != screen_width || h != screen_height)
	    {
		panel.position.x =
		    (int) ((double) (panel.position.x * screen_width) /
			   (double) w);
		panel.position.y =
		    (int) ((double) (panel.position.y * screen_height) /
			   (double) h);
	    }
	}
    }

    /* check the values */
    if (settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
	settings.orientation = HORIZONTAL;
    if (settings.size < TINY || settings.size > LARGE)
	settings.size = SMALL;
/*    if (settings.num_groups < 1 || settings.num_groups > 2 * NBGROUPS)
	settings.num_groups = 2*NBGROUPS; */
}

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

    snprintf (value, 3, "%d", settings.num_groups);
    xmlSetProp (node, "groups", value);

    child = xmlNewTextChild (node, NULL, "Position", NULL);

    snprintf (value, 5, "%d", panel.position.x);
    xmlSetProp (child, "x", value);

    snprintf (value, 5, "%d", panel.position.y);
    xmlSetProp (child, "y", value);

    /* save screen width and hide, so we can use a relative position
     * the user logs in on different computers */
    snprintf (value, 5, "%d", screen_width);
    xmlSetProp (child, "screenwidth", value);

    snprintf (value, 5, "%d", screen_height);
    xmlSetProp (child, "screenheight", value);
}
