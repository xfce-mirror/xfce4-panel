/*  xfce4
 *
 *  Copyright (C) 2002-2004 Jasper Huijsmans (jasper@xfce.org)
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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libnetk.h>
#include <libxfcegui4/xinerama.h>
#include <libxfcegui4/icons.h>

#include "xfce.h"
#include "groups.h"
#include "controls.h"
#include "controls_dialog.h"
#include "add-control-dialog.h"
#include "popup.h"
#include "settings.h"
#include "mcs_client.h"

#define HIDE_TIMEOUT   500
#define UNHIDE_TIMEOUT 100
#define HIDDEN_SIZE 3

#define HANDLE_WIDTH 10

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

/* original screen sizes, used if screen is not the same
 * size as in previous session */
static int old_screen_width = 0;
static int old_screen_height = 0;

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
    DesktopMargins margins = { 0 };

    g_return_if_fail (dpy != NULL);
    
    netk_get_desktop_margins (xscreen, &margins);
    
    gtk_window_get_position (GTK_WINDOW (p->toplevel), &x, &y);
    x += p->toplevel->allocation.width / 2;
    y += p->toplevel->allocation.height / 2;

    xinerama_scr.xmin = MAX (MyDisplayX (x, y), margins.left);
    xinerama_scr.xmax = MIN (MyDisplayMaxX (dpy, scr, x, y), 
	    		     screen_width - margins.right);
    xinerama_scr.ymin = MAX (MyDisplayY (x, y), margins.top);
    xinerama_scr.ymax = MIN (MyDisplayMaxY (dpy, scr, x, y), 
	    		     screen_height - margins.bottom);

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
 *   
 * Assumes panel_req and xinerama_scr are up-to-date.
 **/
static void
update_position (Panel * p, int *x, int *y)
{
    int xcenter, ycenter;
    
    if (!p || !p->toplevel)
	return;

    DBG ("desired position: %d,%d", *x, *y);

    xcenter = xinerama_scr.xmin +
	(xinerama_scr.xmax - xinerama_scr.xmin) / 2 -
	panel_req.width / 2;

    ycenter = xinerama_scr.ymin +
	(xinerama_scr.ymax - xinerama_scr.ymin) / 2 -
	panel_req.height / 2;

    if (settings.orientation == HORIZONTAL)
    {
	if (*y < ycenter)
	{
	    *y = xinerama_scr.ymin;
	}
	else
	{
	    *y = xinerama_scr.ymax - panel_req.height;
	}

	if (xcenter > xinerama_scr.xmin &&
	    *x > xcenter - 20 && *x < xcenter + 20)
	{
	    *x = xcenter;
	}
	else if (*x + panel_req.width > xinerama_scr.xmax - 20)
	{
	    *x = xinerama_scr.xmax - panel_req.width;
	}
	else if (*x < xinerama_scr.xmin + 20)
	{
	    *x = xinerama_scr.xmin;
	}

	if (*x < xinerama_scr.xmin)
	{
	    *x = xinerama_scr.xmin;
	}
    }
    else
    {
	if (*x < xcenter)
	{
	    *x = xinerama_scr.xmin;
	}
	else
	{
	    *x = xinerama_scr.xmax - panel_req.width;
	}

	if (ycenter > xinerama_scr.ymin &&
	    *y > ycenter - 20 && *y < ycenter + 20)
	{
	    *y = ycenter;
	}
	else if (*y + panel_req.height > xinerama_scr.ymax - 20)
	{
	    *y = xinerama_scr.ymax - panel_req.height;
	}
	else if (*y < xinerama_scr.ymin + 20)
	{
	    *y = xinerama_scr.ymin;
	}

	if (*y < xinerama_scr.ymin)
	{
	    *y = xinerama_scr.ymin;
	}
    }

    DBG ("new position: %d,%d", *x, *y);
}

static void
panel_move_func (GtkWidget *win, int *x, int *y, Panel *panel)
{
    static int num_screens = 0;

    if (G_UNLIKELY(num_screens == 0))
	num_screens = xineramaGetHeads ();

    if (G_UNLIKELY(num_screens > 1))
    {
	int xcenter, ycenter;

	xcenter = *x + panel_req.width / 2;
	ycenter = *y + panel_req.height / 2;
	
	if (xcenter < xinerama_scr.xmin || ycenter < xinerama_scr.ymax
	    || xcenter > xinerama_scr.xmax || ycenter > xinerama_scr.ymax)
	{
	    update_xinerama_coordinates (panel);
	}
    }
    
    /* check if xinerama screen must be updated */
    update_position (panel, x, y);
}

static void
panel_reallocate (Panel * p, GtkRequisition * previous)
{
    GtkRequisition new;
    int xold, yold, xnew, ynew;

    gtk_widget_size_request (p->toplevel, &new);

    xold = p->position.x;
    yold = p->position.y;

    xnew = xold - (new.width - previous->width) / 2;
    ynew = yold - (new.height - previous->height) / 2;
    
    panel_req = new;

    update_position (p, &xnew, &ynew);

    if (xnew != xold || ynew != yold)
    {
	gtk_window_move (GTK_WINDOW (p->toplevel), xnew, ynew);

	p->position.x = xnew;
	p->position.y = ynew;

	/* Need to save position here... */
	write_panel_config ();
    }
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
    {N_("/Xfce Panel"), NULL, NULL, 0, "<Title>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/Add _new item"), NULL, run_add_control_dialog, 0, "<Item>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Properties..."), NULL, edit_prefs, 0, "<Item>"},
    {N_("/_About Xfce"), NULL, do_info, 0, "<Item>"},
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
    static GtkItemFactory *ifactory;

    if (!menu)
    {
	ifactory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);

	gtk_item_factory_set_translate_func (ifactory,
					     (GtkTranslateFunc)
					     translate_menu, NULL, NULL);
	gtk_item_factory_create_items (ifactory, G_N_ELEMENTS (panel_items),
				       panel_items, NULL);

	menu = GTK_MENU (gtk_item_factory_get_widget (ifactory, "<popup>"));
    }

    /* the third item, keep in sync with factory 
    item = GTK_MENU_SHELL (menu)->children->next->next->data;
    
    submenu = get_controls_submenu();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);*/

    return menu;
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
    xfce_movehandler_set_move_func (XFCE_MOVEHANDLER (mh),
	    			     (XfceMoveFunc) panel_move_func, p);
    gtk_widget_show (mh);

    gtk_widget_set_name (mh, "xfce_panel");

    g_signal_connect (mh, "button-press-event",
		      G_CALLBACK (handler_pressed_cb), p);

    g_signal_connect_swapped (mh, "move-start", 
	    		      G_CALLBACK (update_xinerama_coordinates), p);

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
    else
	DBG ("already hidden");

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

    gtk_window_set_title (window, _("Xfce Panel"));
    gtk_window_set_decorated (window, FALSE);
    gtk_window_set_resizable (window, FALSE);
    gtk_window_stick (window);

    pb = get_panel_pixbuf ();
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    g_signal_connect (w, "delete-event", G_CALLBACK (panel_delete_cb), p);

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

    if (old_screen_width > 0)
    {
	double xscale, yscale;

	gtk_widget_size_request (panel.toplevel, &panel_req);
	
	xscale = (double) x / (double) (old_screen_width - panel_req.width);
	yscale = (double) y / (double) (old_screen_height - panel_req.height);
	
	x = rint (xscale * (screen_width - panel_req.width));
	y = rint (yscale * (screen_height - panel_req.height));

	old_screen_width = old_screen_height = 0;
    }
	
    panel.position.x = x;
    panel.position.y = y;
    gtk_window_move (GTK_WINDOW (panel.toplevel), x, y);

    gtk_widget_size_request (panel.toplevel, &panel_req);
    update_xinerama_coordinates (&panel);

    panel.position.x = x;
    panel.position.y = y;
    panel_set_position ();

    gtk_widget_show (panel.toplevel);
    set_window_layer (panel.toplevel, settings.layer);
    set_window_skip (panel.toplevel);

    /* this must be before set_autohide() and after set_position()
     * otherwise the initial position will be messed up */
    panel_created = TRUE;

    if (hidden)
	panel_set_autohide (TRUE);

#if GTK_CHECK_VERSION(2,2,0)
    g_signal_connect (G_OBJECT (gdk_screen_get_default ()), "size-changed",
		      G_CALLBACK (screen_size_changed), &panel);
#endif

    g_signal_connect (GTK_WINDOW (panel.toplevel), "size-allocate", 
	    	      G_CALLBACK (panel_allocate_cb), &panel);

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
	xfce_set_icon_theme(theme);

#if GTK_CHECK_VERSION(2, 4, 0)
    gtk_settings_set_string_property (gtk_settings_get_default (),
    		"gtk-icon-theme-name", theme, "panel.c:1108");
#endif

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
	settings.orientation = (int) strtol (value, NULL, 0);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "layer");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	if (n >= ABOVE && n <= BELOW)
	    settings.layer = n;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "size");

    if (value)
    {
	settings.size = (int) strtol (value, NULL, 0);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "popupposition");

    if (value)
    {
	settings.popup_position = (int) strtol (value, NULL, 0);
	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "icontheme");

    if (value)
	settings.theme = g_strdup (value);

    g_free (value);

    value = xmlGetProp (node, (const xmlChar *) "groups");

    if (value)
    {
	settings.num_groups = (int) strtol (value, NULL, 0);
	g_free (value);
    }
    else
    {
	value = xmlGetProp (node, (const xmlChar *) "left");

	if (value)
	{
	    settings.num_groups = (int) strtol (value, NULL, 0);
	    g_free (value);

	    value = xmlGetProp (node, (const xmlChar *) "right");

	    if (value)
	    {
		settings.num_groups += (int) strtol (value, NULL, 0);
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
		panel.position.x = (int) strtol (value, NULL, 0);

	    g_free (value);

	    value = xmlGetProp (child, (const xmlChar *) "y");

	    if (value)
		panel.position.y = (int) strtol (value, NULL, 0);

	    g_free (value);

	    if (panel.position.x < 0 || panel.position.y < 0)
		break;

	    value = xmlGetProp (child, (const xmlChar *) "screenwidth");

	    if (value)
		w = (int) strtol (value, NULL, 0);
	    else
		break;

	    value = xmlGetProp (child, (const xmlChar *) "screenheight");

	    if (value)
		h = (int) strtol (value, NULL, 0);
	    else
		break;

	    /* this doesn't actually work completely, we need to
	     * save the panel width as well to do it right */
	    if (w != screen_width || h != screen_height)
	    {
		old_screen_width = w;
		old_screen_height = h;
	    }
	}
    }

    /* check the values */
    if (settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
	settings.orientation = HORIZONTAL;
    if (settings.size < TINY || settings.size > LARGE)
	settings.size = SMALL;
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
