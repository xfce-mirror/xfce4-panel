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

int popup_icon_size[] = { 20, 24, 24, 32 };

/* lock settings update when panel is not yet (re)build */
static gboolean panel_created = FALSE;

/* screen properties */
static Display *dpy = NULL;
static int scr = 0;
static int screen_width = 0;
static int screen_height = 0;

/*  Move handle menu
 *  ----------------
*/
static void
edit_prefs (void)
{
    mcs_dialog (NULL);
}

static void
settings_mgr (void)
{
    mcs_dialog ("all");
}

static void
add_new (void)
{
    Control *control;

    panel_add_control ();
    panel_set_position ();

    control = groups_get_control (settings.num_groups - 1);

    controls_dialog (control);
}

static void
lock_screen (void)
{
    exec_cmd ("xflock4", FALSE, FALSE);
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
    {N_("/Add _new item"), NULL, add_new, 0, "<Item>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Properties..."), NULL, edit_prefs, 0, "<Item>"},
    {N_("/_Settings manager"), NULL, settings_mgr, 0, "<Item>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_About XFce"), NULL, do_info, 0, "<Item>"},
    {N_("/_Help"), NULL, do_help, 0, "<Item>"},
    {"/sep", NULL, NULL, 0, "<Separator>"},
    {N_("/_Lock screen"), NULL, lock_screen, 0, "<Item>"},
    {N_("/E_xit"), NULL, exit_panel, 0, "<Item>"},
};

static const char *
translate_menu (const char *msg)
{
    return gettext (msg);
}

static GtkMenu *
create_handle_menu (void)
{
    static GtkMenu *menu = NULL;
    GtkItemFactory *ifactory;

    if (!menu)
    {
        ifactory = gtk_item_factory_new (GTK_TYPE_MENU, "<popup>", NULL);

        gtk_item_factory_set_translate_func (ifactory,
                                             (GtkTranslateFunc) translate_menu,
                                             NULL, NULL);
        gtk_item_factory_create_items (ifactory, G_N_ELEMENTS (panel_items),
                                       panel_items, NULL);
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
handler_pressed_cb (GtkWidget * h, GdkEventButton * event, GtkMenu * menu)
{
    hide_current_popup_menu ();

    if (event->button == 3 ||
        (event->button == 1 && event->state & GDK_SHIFT_MASK))
    {
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, event->button,
                        event->time);
        return TRUE;
    }

    /* let default handler run */
    return FALSE;
}

static gboolean
handler_released_cb (GtkWidget * h, GdkEventButton * event, gpointer data)
{
    gtk_window_get_position (GTK_WINDOW (panel.toplevel), &panel.position.x, &panel.position.y);
    /* let default handler run */
    return FALSE;
}

GtkWidget *
handle_new (void)
{
    GtkWidget *mh;
    GtkMenu *menu;

    mh = xfce_movehandler_new (panel.toplevel);
    gtk_widget_show (mh);

    gtk_widget_set_name (mh, "xfce_panel");

    menu = create_handle_menu ();

    g_signal_connect (mh, "button-press-event",
                      G_CALLBACK (handler_pressed_cb), menu);

    g_signal_connect (mh, "button-release-event",
                      G_CALLBACK (handler_released_cb), NULL);

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
    GtkRequisition req;
    int x, y;
    Position pos;
    static int minx = 0, maxx = 0, miny = 0, maxy = 0, centerx = 0, centery=0;
    
    if (!p->hidden)
    {
        gtk_window_get_position (GTK_WINDOW (p->toplevel), &(p->position.x),
                                 &(p->position.y));
    }

    /* Get the size */
    gtk_widget_size_request (p->toplevel, &req);
    pos = p->position;

    /* xinerama aware screen coordinates */
    if (pos.x < minx || pos.x > maxx || pos.y < miny || pos.y > maxy)
    {
	minx = MyDisplayX(pos.x, pos.y);
	maxx = MyDisplayMaxX(dpy, scr, pos.x, pos.y);
	miny = MyDisplayY(pos.x, pos.y);
	maxy = MyDisplayMaxY(dpy, scr, pos.x, pos.y);

	centerx = minx + (maxx - minx) / 2;
	centery = miny + (maxy - miny) / 2;
    }

    /* Handle the resize */
    if (hide)
    {
        /* Depending on orientation, resize */
        if (settings.orientation == VERTICAL)
        {
            if (pos.x < centerx)
                x = pos.x;
            else
                x = pos.x + req.width - HIDDEN_SIZE;

            y = pos.y;

            req.width = HIDDEN_SIZE;
        }
        else
        {
            if (pos.y < centery)
                y = pos.y;
            else
                y = pos.y + req.height - HIDDEN_SIZE;

            x = pos.x;

            req.height = HIDDEN_SIZE;
        }
    }
    else
    {
        req.width = -1;
        req.height = -1;
        x = pos.x;
        y = pos.y;
    }

/*    g_print("(x=%d,y=%d) => (x=%d,y=%d)\n", pos.x, pos.y, x, y);*/
    if (hide)
        gtk_widget_hide (p->main_frame);
    else
        gtk_widget_show (p->main_frame);

    p->hidden = hide;

    gtk_widget_set_size_request (p->toplevel, req.width, req.height);
    
    while(gtk_events_pending())
	gtk_main_iteration();

    /* gtk_window_present(GTK_WINDOW(p->toplevel)); */
    gtk_window_move (GTK_WINDOW (p->toplevel), x, y);
}

gboolean
panel_hide_timeout (Panel * p)
{
    /* keep trying while a subpanel is open */
    if (open_popup)
        return TRUE;

    if (!p->hidden)
	panel_set_hidden (p, TRUE);

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
panel_enter (GtkWindow * w, GdkEventCrossing * event, gpointer data)
{
    Panel *p = (Panel *) data;

    if (!(settings.autohide))
    {
        return FALSE;
    }

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
panel_leave (GtkWidget * w, GdkEventCrossing * event, gpointer data)
{
    Panel *p = (Panel *) data;

    if (!(settings.autohide))
    {
        return FALSE;
    }

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

/*  Panel framework
 *  ---------------
*/
static gboolean
panel_delete_cb (GtkWidget * window, GdkEvent * ev, gpointer data)
{
    quit (FALSE);

    return TRUE;
}

static GtkWidget *
create_panel_window (void)
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

    pb = get_system_pixbuf (XFCE_ICON);
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    g_signal_connect (w, "delete-event", G_CALLBACK (panel_delete_cb), NULL);

    return w;
}

static void
create_panel_framework (Panel * p)
{
    gboolean vertical = (settings.orientation == VERTICAL);

    /* toplevel window */
    if (!p->toplevel)
    {
        p->toplevel = create_panel_window ();
        g_object_add_weak_pointer (G_OBJECT (p->toplevel),
                                   (gpointer *) & (p->toplevel));
    }

    /* this is necessary after a SIGHUP */
    gtk_window_stick (GTK_WINDOW (p->toplevel));

    /* Connect signalers to window for autohide */
    g_signal_connect (GTK_WINDOW (p->toplevel), "enter-notify-event",
                      G_CALLBACK (panel_enter), &panel);

    g_signal_connect (GTK_WINDOW (p->toplevel), "leave-notify-event",
                      G_CALLBACK (panel_leave), &panel);

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
    p->handles[LEFT] = handle_new ();
    p->handles[RIGHT] = handle_new ();

    /* pack the widgets into the main frame */
    gtk_container_add (GTK_CONTAINER (p->main_frame), p->panel_box);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->handles[LEFT], FALSE, FALSE,
                        0);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->group_box, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (p->panel_box), p->handles[RIGHT], FALSE, FALSE,
                        0);
}

void
panel_cleanup (void)
{
    groups_cleanup ();
}

static void init_settings ();

void
create_panel (void)
{
    static gboolean need_init = TRUE;
    int x, y;

    /* necessary for initial settings to do the right thing */
    panel_created = FALSE;

    if (need_init)
    {
        /* set for later use */
	/* TODO: for gtk 2.2 we could use gtk_widget_get_display() et al. */
	dpy = gdk_display;
	scr = DefaultScreen(dpy); 
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

    /* panel framework */
    create_panel_framework (&panel);

    /* FIXME
     * somehow the position gets set differently in the code below
     * we just save it here and restore it before reading the config
     * file */
    x = panel.position.x;
    y = panel.position.y;

    groups_init (GTK_BOX (panel.group_box));

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config ();

    panel.position.x = x;
    panel.position.y = y;
    panel_set_position ();

    gtk_widget_show (panel.toplevel);
    set_window_layer (panel.toplevel, settings.layer);
    set_window_skip (panel.toplevel);

    /* this must be before set_autohide() and after set_position()
     * otherwise the initial position will be messed up */
    panel_created = TRUE;

    panel_set_autohide (settings.autohide);
}

void
panel_add_control (void)
{
    settings.num_groups++;

    groups_set_num_groups (settings.num_groups);
}

/*  Panel settings
 *  --------------
*/
void
panel_set_orientation (int orientation)
{
    gboolean hidden;
    
    settings.orientation = orientation;

    if (!panel_created)
        return;

    hidden = settings.autohide;
    if (hidden)
	panel_set_autohide(FALSE);
    
    hide_current_popup_menu ();

    /* save panel controls */
    groups_unpack ();

    /* no need to recreate the window */
    gtk_widget_destroy (panel.main_frame);
    create_panel_framework (&panel);

    groups_pack (GTK_BOX (panel.group_box));
    groups_set_orientation (orientation);

    while(gtk_events_pending())
	gtk_main_iteration();

    panel.position.x = panel.position.y = -1;
    panel_set_position ();
    
    if (hidden)
	panel_set_autohide(TRUE);
}

void
panel_set_layer (int layer)
{
    settings.layer = layer;

    if (!panel_created)
        return;

    set_window_layer (panel.toplevel, layer);
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

    panel_set_position ();
}

void
panel_set_popup_position (int position)
{
    settings.popup_position = position;

    if (!panel_created)
        return;

    hide_current_popup_menu ();

    groups_set_popup_position (position);

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

/* FIXME: this should probably NOT be a global option.
 * Just add and remove controls based on the config file */
void
panel_set_num_groups (int n)
{
    settings.num_groups = n;

    if (!panel_created)
        return;

    groups_set_num_groups (n);
}

void
panel_set_settings (void)
{
    panel_set_size (settings.size);
    panel_set_popup_position (settings.popup_position);

    panel_set_theme (settings.theme);

    panel_set_num_groups (settings.num_groups);
}

void
panel_center (int side)
{
    GtkRequisition req;
    int w, h;
    DesktopMargins margins;
    Screen *xscreen;
    gboolean hidden;

    w = screen_width;
    h = screen_height;

    hidden = settings.autohide;
    if (hidden)
    {
	panel_set_autohide(FALSE);

	while(gtk_events_pending())
	    gtk_main_iteration();
    }
    
    gtk_widget_size_request (panel.toplevel, &req);

    xscreen = DefaultScreenOfDisplay (GDK_DISPLAY ());
    netk_get_desktop_margins (xscreen, &margins);

    switch (side)
    {
        case LEFT:
            panel.position.x = margins.left;
            panel.position.y = h / 2 - req.height / 2;
            break;
        case RIGHT:
            panel.position.x = w - req.width - margins.right;
            panel.position.y = h / 2 - req.height / 2;
            break;
        case TOP:
            panel.position.x = w / 2 - req.width / 2;
            panel.position.y = margins.top;
            break;
        default:
            panel.position.x = w / 2 - req.width / 2;
            panel.position.y = h - req.height - margins.bottom;
    }

    if (panel.position.x < 0)
        panel.position.x = 0;

    if (panel.position.y < 0)
        panel.position.y = 0;

    panel_set_position ();

    if (hidden)
	panel_set_autohide(TRUE);
}

void
panel_set_position (void)
{
    if (panel.position.x == -1 && panel.position.y == -1)
    {
        if (settings.orientation == HORIZONTAL)
            panel_center (BOTTOM);
        else
            panel_center (LEFT);
    }
    else
    {
        GtkRequisition req;
        int w, h;
	gboolean hidden;
    
	hidden = settings.autohide;
	if (hidden)
	{
	    panel_set_autohide(FALSE);

	    while(gtk_events_pending())
		gtk_main_iteration();
	}
	    
        w = screen_width;
        h = screen_height;

        gtk_widget_size_request (panel.toplevel, &req);

        if (panel.position.x + req.width > w && req.width <= w)
            panel.position.x = w - req.width;

        if (panel.position.y + req.height > h && req.height <= h)
            panel.position.y = h - req.height;

        if (panel.position.x < 0)
            panel.position.x = 0;

        if (panel.position.y < 0)
            panel.position.y = 0;

        DBG ("position: (%d, %d)\n", panel.position.x, panel.position.y);

        gtk_window_move (GTK_WINDOW (panel.toplevel), panel.position.x,
                         panel.position.y);
	
	if (hidden)
	    panel_set_autohide(TRUE);
    }
}

void
panel_set_autohide (gboolean hide)
{
    settings.autohide = hide;

    if (!panel_created)
	return;

    panel_set_hidden (&panel, hide);
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
    value = xmlGetProp (node, (const xmlChar *)"orientation");

    if (value)
    {
        settings.orientation = atoi (value);
        g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *)"layer");

    if (value)
    {
        n = atoi (value);

        if (n >= ABOVE && n <= BELOW)
            settings.layer = n;

        g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *)"size");

    if (value)
    {
        settings.size = atoi (value);
        g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *)"popupposition");

    if (value)
    {
        settings.popup_position = atoi (value);
        g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *)"icontheme");

    if (value)
        settings.theme = g_strdup (value);

    g_free (value);

    value = xmlGetProp (node, (const xmlChar *)"groups");

    if (value)
    {
        settings.num_groups = atoi (value);
        g_free (value);
    }
    else
    {
        value = xmlGetProp (node, (const xmlChar *)"left");

        if (value)
        {
            settings.num_groups = atoi (value);
            g_free (value);

            value = xmlGetProp (node, (const xmlChar *)"right");

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

        if (xmlStrEqual (child->name, (const xmlChar *)"Position"))
        {
            value = xmlGetProp (child, (const xmlChar *)"x");

            if (value)
                panel.position.x = atoi (value);

            g_free (value);

            value = xmlGetProp (child, (const xmlChar *)"y");

            if (value)
                panel.position.y = atoi (value);

            g_free (value);

            if (panel.position.x < 0 || panel.position.y < 0)
                break;

            value = xmlGetProp (child, (const xmlChar *)"screenwidth");

            if (value)
                w = atoi (value);
            else
                break;

            value = xmlGetProp (child, (const xmlChar *)"screenheight");

            if (value)
                h = atoi (value);
            else
                break;

            /* this doesn't actually work completely, we need to
             * save the panel width as well to do it right */
            if (w != screen_width || h != screen_height)
            {
                panel.position.x =
                    (int)((double)(panel.position.x * screen_width) /
                          (double)w);
                panel.position.y =
                    (int)((double)(panel.position.y * screen_height) /
                          (double)h);
            }
        }
    }

    /* check the values */
    if (settings.orientation < HORIZONTAL || settings.orientation > VERTICAL)
        settings.orientation = HORIZONTAL;
    if (settings.size < TINY || settings.size > LARGE)
        settings.size = SMALL;
    if (settings.num_groups < 1 || settings.num_groups > 2 * NBGROUPS)
        settings.num_groups = 10;
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
