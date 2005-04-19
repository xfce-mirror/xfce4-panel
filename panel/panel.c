/*  $Id$
 *
 *  Copyright 2002-2005 Jasper Huijsmans (jasper@xfce.org)
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

#include "xfce-panel-window.h"
#include "xfce-itembar.h"
#include "xfce-item.h"

#include "xfce.h"
#include "controls.h"
#include "controls_dialog.h"
#include "add-control-dialog.h"
#include "item-control.h"
#include "settings.h"
#include "mcs_client.h"
#include "icons.h"

#define HIDE_TIMEOUT	500
#define UNHIDE_TIMEOUT	100
#define HIDDEN_SIZE	3
#define HANDLE_WIDTH	10
#define SNAP_WIDTH	25
#define OPAQUE          0xffffffff


/* typedefs *
 * -------- */

struct _PanelPrivate
{
    Settings settings;

    gboolean full_width;

    GdkScreen *screen;
    int monitor;
    GdkRectangle monitor_geometry;

    int side;
    XfcePositionState pos_state;
    int offset;
    int block_resize;

    GtkRequisition req;

    gboolean is_created;
    int block_autohide;

    GtkArrowType popup_arrow_type;
    GSList *controls;
};


/* static globals *
 * -------------- */
static guint transparency = 0xc0000000;


/*  exported globals
 *  ---------------- 
 *  FIXME These should be replaced with accessor functions */

G_MODULE_EXPORT /* EXPORT:settings */
Settings settings = {0};
G_MODULE_EXPORT /* EXPORT:panel */
Panel panel = {0};

/* These sizes are exported to all other modules.
 * Arrays are indexed by symbolic sizes TINY, SMALL, MEDIUM, LARGE
 * (see global.h). */
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

/* defined in controls.c */
extern void control_class_unref (ControlClass * cclass);


/* transparency *
 * ------------ */

static void
get_opacity_setting (void)
{
    char *file = 
        xfce_resource_lookup (XFCE_RESOURCE_CONFIG,
                              "xfce4" G_DIR_SEPARATOR_S "transparency");

    if (file)
    {
        FILE *fp;
        
        if ((fp = fopen (file, "r")) != NULL)
        {
            char line[255];
            guint trans;

            while (fgets(line, sizeof (line), fp))
            {
                if (sscanf (line, "panel = %u", &trans) > 0)
                {
                    trans = CLAMP (trans, 0, 80);

                    transparency = OPAQUE - rint ((double)trans * OPAQUE / 100);

                    DBG ("transparency: %u\n", trans);
                    DBG ("opacity: 0x%x\n", transparency);

                    break;
                }
            }

            fclose (fp);
        }
        
        g_free (file);
    }
}

static void
set_translucent (Panel * p, gboolean translucent)
{
    static gboolean initialized = FALSE;
    guint opacity;

    if (G_UNLIKELY (!initialized))
    {
        get_opacity_setting ();
        
        initialized = TRUE;
    }

    if (transparency == OPAQUE)
        return;
    
    opacity = (translucent ? transparency : OPAQUE);
    gdk_error_trap_push ();

    gdk_property_change (p->toplevel->window,
			 gdk_atom_intern ("_NET_WM_WINDOW_OPACITY", FALSE),
			 gdk_atom_intern ("CARDINAL", FALSE), 32,
			 GDK_PROP_MODE_REPLACE, (guchar *) & opacity, 1L);

    gdk_error_trap_pop ();

    gtk_widget_queue_draw (p->toplevel);
}


/* positioning and position related functions *
 * ------------------------------------------ */

static void
update_partial_struts (Panel * p)
{
    gulong data[12] = { 0, };

    if (!p->priv->settings.autohide)
    {
	int w, h, x, y;

	x = p->position.x;
	y = p->position.y;
	w = p->toplevel->allocation.width;
	h = p->toplevel->allocation.height;

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
    GSList *l;

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

    p->priv->popup_arrow_type = type;

    for (l = p->priv->controls; l != NULL; l = l->next)
    {
        Control *control = l->data;

        item_control_set_arrow_direction (control, type);
    }
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
    int xcenter, ycenter, snapwidth;
    gboolean show_sides;

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
        snapwidth = MIN (SNAP_WIDTH, (p->priv->monitor_geometry.width 
                                      - p->priv->req.width) / 2);
        
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
	if (*x > xcenter - snapwidth && *x < xcenter + snapwidth)
	{
	    *x = xcenter;

	    p->priv->pos_state = XFCE_POS_STATE_CENTER;
	}
	/* right edge */
	else if (*x + p->priv->req.width > p->priv->monitor_geometry.x
		 + p->priv->monitor_geometry.width - snapwidth)
	{
	    *x = p->priv->monitor_geometry.x
		+ p->priv->monitor_geometry.width - p->priv->req.width;

	    p->priv->pos_state = XFCE_POS_STATE_END;
	}

	/* left edge */
	if (*x < p->priv->monitor_geometry.x + snapwidth)
	{
	    *x = p->priv->monitor_geometry.x;

	    p->priv->pos_state = XFCE_POS_STATE_START;
	}

	p->priv->offset = *x - p->priv->monitor_geometry.x;
    }
    else
    {
        snapwidth = MIN (SNAP_WIDTH, (p->priv->monitor_geometry.height 
                                      - p->priv->req.height) / 2);
        
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
	if (*y > ycenter - snapwidth && *y < ycenter + snapwidth)
	{
	    *y = ycenter;

	    p->priv->pos_state = XFCE_POS_STATE_CENTER;
	}
	/* bottom edge */
	else if (*y + p->priv->req.height > p->priv->monitor_geometry.y
		 + p->priv->monitor_geometry.height - snapwidth)
	{
	    *y = p->priv->monitor_geometry.y
		+ p->priv->monitor_geometry.height - p->priv->req.height;

	    p->priv->pos_state = XFCE_POS_STATE_END;
	}

	/* top edge */
	if (*y < p->priv->monitor_geometry.y + snapwidth)
	{
	    *y = p->priv->monitor_geometry.y;

	    p->priv->pos_state = XFCE_POS_STATE_START;
	}

	p->priv->offset = *y - p->priv->monitor_geometry.y;
    }

    show_sides = !p->priv->full_width;
    
    switch (p->priv->side)
    {
        case TOP:
            xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (p->toplevel), 
                                               FALSE, TRUE, 
                                               show_sides, show_sides);
            break;
        case BOTTOM:
            xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (p->toplevel), 
                                               TRUE, FALSE, 
                                               show_sides, show_sides);
            break;
        case LEFT:
            xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (p->toplevel), 
                                               show_sides, show_sides, 
                                               FALSE, TRUE);
            break;
        case RIGHT:
            xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (p->toplevel), 
                                               show_sides, show_sides, 
                                               TRUE, FALSE);
            break;
        default:
            xfce_panel_window_set_show_border (XFCE_PANEL_WINDOW (p->toplevel), 
                                               TRUE, TRUE, TRUE, TRUE);
    }
}

/* move / resize */
static void
panel_move_func (XfcePanelWindow * panel_window, Panel * panel, int *x, int *y)
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
panel_set_coordinates (Panel *p)
{
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

        if (p->priv->full_width)
        {
            p->position.y = p->priv->monitor_geometry.y;
        }
        else
        {
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
                        + p->priv->monitor_geometry.height 
                        - p->priv->req.height;
                    break;
                default:
                    p->position.y = p->priv->monitor_geometry.y 
                        + p->priv->offset;
            }
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

        if (p->priv->full_width)
        {
            p->position.x = p->priv->monitor_geometry.x;
        }
        else
        {
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
                        + p->priv->monitor_geometry.width 
                        - p->priv->req.width;
                    break;
                default:
                    p->position.x = p->priv->monitor_geometry.x 
                        + p->priv->offset;
            }
        }
    }

    DBG ("\n"
         " ++ position: %d, %d\n"
         "    monitor: %d\n"
         "    side: %s\n"
         "    state: %s\n",
         p->position.x, p->position.y,
         p->priv->monitor,
	 p->priv->side == LEFT ? "left" :
            p->priv->side == RIGHT ? "right" :
                p->priv->side == TOP ? "top" : "bottom",
	 p->priv->pos_state == XFCE_POS_STATE_CENTER ? "center" :
             p->priv->pos_state == XFCE_POS_STATE_START ? "start" :
                 p->priv->pos_state == XFCE_POS_STATE_END ? "end" : "none");
}

static void
panel_resize_func (XfcePanelWindow * panel_window, Panel *p, 
                   GtkAllocation * old, GtkAllocation * new, int *x, int *y)
{
    if (p->hidden || !old || !new || p->priv->block_resize)
        return;

    if (p->priv->settings.orientation == HORIZONTAL)
        p->priv->offset -= (new->width - old->width) / 2;
    else
        p->priv->offset -= (new->height - old->height) / 2;

    p->priv->req.width = new->width;
    p->priv->req.height = new->height;
    
    panel_set_coordinates (p);

    *x = p->position.x;
    *y = p->position.y;
}

static void
panel_set_position (Panel * p)
{
    if (!p->priv->is_created || p->hidden)
	return;

    panel_set_coordinates (p);

    gtk_window_move (GTK_WINDOW (p->toplevel), p->position.x, p->position.y);

    update_arrow_direction (p);

    write_panel_config ();

    update_partial_struts (p);
}

static void
panel_center (int side)
{
    panel.priv->side = side;
    panel.priv->pos_state = XFCE_POS_STATE_CENTER;
    panel.priv->offset = 0;

    panel_set_position (&panel);
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
    exec_cmd ("xfhelp4 xfce4-panel.html", FALSE, FALSE);
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
     * keep in sync with menu changes. */
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

/* defined in controls.c, must be set to NULL to indicate the menu
 * is not being popped up from a panel item */
/* FIXME ugly */
extern Control *popup_control;

static gboolean
window_pressed_cb (GtkWidget * w, GdkEventButton * event)
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
window_move_end_cb (GtkWidget * w, int x, int y, Panel * p)
{
    p->position.x = x;
    p->position.y = y;

    DBG ("move end: %d,%d", p->position.x, p->position.y);

    /* FIXME do we need this? */
    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);

    write_panel_config ();

    update_partial_struts (p);
}

static void
window_move_start (Panel * p)
{
    /* FIXME do we need this? */
    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);
}

/*  Autohide
 *  --------
*/
/* FIXME even more ugly */
extern PanelPopup *open_popup;

static void
panel_set_hidden (Panel * p, gboolean hide)
{
    static int recursive = 0;
    int x, y, w, h;
    
    while (recursive > 0)
        g_usleep (1000);

    if (hide == p->hidden)
        return;
    
    recursive++;
    
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

    p->priv->block_resize++;
    
    if (hide)
    {
	gtk_widget_hide (p->group_box);
        xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (p->toplevel),
                                            XFCE_HANDLE_STYLE_NONE);
	gtk_widget_set_size_request (p->toplevel, w, h);
    }
    else
    {
	gtk_widget_show (p->group_box);

        panel_set_full_width (p->priv->full_width);
    }

    p->hidden = hide;

    DBG ("%s: (%d,%d) %dx%d\n", hide ? "hide" : "unhide", x, y, w, h);
    gdk_window_move_resize (p->toplevel->window, x, y, w, h);
    
    p->priv->block_resize--;
    
    recursive--;
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
        gtk_widget_queue_resize (p->toplevel);
    }

    return FALSE;
}

static gboolean
panel_enter (GtkWindow * w, GdkEventCrossing * event, Panel * p)
{
    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        set_translucent (p, FALSE);

        if (settings.autohide)
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
    }

    return FALSE;
}

static gboolean
panel_leave (GtkWidget * w, GdkEventCrossing * event, Panel * p)
{
    if (event->detail != GDK_NOTIFY_INFERIOR)
    {
        set_translucent (p, TRUE);

        if (settings.autohide)
        {
            if (p->unhide_timeout)
            {
                g_source_remove (p->unhide_timeout);
                p->unhide_timeout = 0;
            }

            if (!p->hide_timeout)
            {
                p->hide_timeout =
                    g_timeout_add (HIDE_TIMEOUT, 
                                   (GSourceFunc) panel_hide_timeout, p);
            }
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

static GtkWidget *
create_panel_window (Panel * p)
{
    GtkWidget *w;
    GtkWindow *window;
    GdkPixbuf *pb;

    w = xfce_panel_window_new ();
    window = GTK_WINDOW (w);

    gtk_window_set_title (window, _("Xfce Panel"));

    pb = get_panel_pixbuf ();
    gtk_window_set_icon (window, pb);
    g_object_unref (pb);

    g_signal_connect (w, "delete-event", G_CALLBACK (panel_delete_cb), p);

    g_signal_connect (w, "button-press-event",
		      G_CALLBACK (window_pressed_cb), p);

    g_signal_connect_swapped (w, "move-start",
			      G_CALLBACK (window_move_start), p);

    g_signal_connect (w, "move-end", G_CALLBACK (window_move_end_cb), p);
    g_object_set_data (G_OBJECT (w), "panel", p);

    xfce_panel_window_set_move_function (XFCE_PANEL_WINDOW (w),
            (XfcePanelWindowMoveFunc) panel_move_func, p);
    
    xfce_panel_window_set_resize_function (XFCE_PANEL_WINDOW (w),
            (XfcePanelWindowResizeFunc) panel_resize_func, p);
    
    return w;
}

static void
create_panel_framework (Panel * p)
{
    GtkOrientation orientation = 
        (p->priv->settings.orientation == VERTICAL) ?
                GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;

    xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (p->toplevel),
                                       orientation);

    p->group_box = xfce_itembar_new (orientation);
    gtk_widget_show (p->group_box);
    gtk_container_add (GTK_CONTAINER (p->toplevel), p->group_box);
}

/* housekeeping for panel controls */

G_MODULE_EXPORT /* EXPORT:panel_cleanup */
void
panel_cleanup (void)
{
    GSList *l;

    for (l = panel.priv->controls; l != NULL; l = l->next)
    {
        Control *control = l->data;

        control_free (control);
    }

    control_class_list_cleanup ();

    g_slist_free (panel.priv->controls);
    panel.priv->controls = NULL;

    g_free (panel.priv);
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

    icons_init ();
    
    /* toplevel window */
    p->toplevel = create_panel_window (p);

    g_object_add_weak_pointer (G_OBJECT (p->toplevel),
			       (gpointer *) & (p->toplevel));

    p->priv->screen = gtk_widget_get_screen (p->toplevel);
    p->priv->monitor = -1;

    /* fill in the 'settings' structure */
    init_settings (p);
    get_global_prefs ();

    /* If there is a settings manager it takes precedence */
    mcs_watch_xfce_channel ();

    /* backwards compat */
    p->priv->settings = settings;

    /* panel framework */
    create_panel_framework (p);

    control_class_list_init ();

    /* read and apply configuration 
     * This function creates the panel items and popup menus */
    get_panel_config ();

    if (!GTK_WIDGET_REALIZED (p->toplevel))
        gtk_widget_realize (p->toplevel);

    p->priv->block_resize++;
    p->priv->is_created = TRUE;
    
    panel_set_position (p);

    DBG (" ++ position: %d, %d\n", panel.position.x, panel.position.y);

    update_xinerama_coordinates (p, p->position.x + p->priv->req.width / 2,
				 p->position.y + p->priv->req.height / 2);

    gtk_widget_show (p->toplevel);

    panel_set_full_width (p->priv->full_width);

    /* recalculate pos_state for old API where only x and y coordinates are
     * read from the config file */
    restrict_position (p, &(p->position.x), &(p->position.y));
    gtk_window_move (GTK_WINDOW (p->toplevel), p->position.x, p->position.y);

    p->priv->block_resize--;

    if (p->priv->settings.autohide)
	panel_set_autohide (TRUE);
    
    set_translucent (p, TRUE);

    update_partial_struts (p);

    update_arrow_direction (p);

    /* Connect signalers to window for autohide */
    g_signal_connect (p->toplevel, "enter-notify-event",
		      G_CALLBACK (panel_enter), p);

    g_signal_connect (p->toplevel, "leave-notify-event",
		      G_CALLBACK (panel_leave), p);

    gtk_drag_dest_set (p->toplevel, GTK_DEST_DEFAULT_ALL, entry, 2,
		       GDK_ACTION_COPY);

    g_signal_connect (p->toplevel, "drag-motion",
		      G_CALLBACK (drag_motion), p);

    g_signal_connect (p->priv->screen, "size-changed",
		      G_CALLBACK (screen_size_changed), p);
}

/* find or act on specific groups */

G_MODULE_EXPORT /* EXPORT:panel_move_control */
void
panel_move_control (int from, int to)
{
    int i;
    GSList *li;
    Control *control;

    if (from < 0 || from >= g_slist_length (panel.priv->controls))
	return;

    li = g_slist_nth (panel.priv->controls, from);
    control = li->data;

    xfce_itembar_reorder_child (XFCE_ITEMBAR (panel.group_box), 
                                control->base, to);

    panel.priv->controls = g_slist_delete_link (panel.priv->controls, li);
    panel.priv->controls = g_slist_insert (panel.priv->controls, control, to);

    for (i = 0, li = panel.priv->controls; li; i++, li = li->next)
    {
	control = li->data;

	control->index = i;
    }

    write_panel_config ();
}

G_MODULE_EXPORT /* EXPORT:panel_remove_control */
void
panel_remove_control (int index)
{
    int i;
    GSList *li;
    Control *control;

    li = g_slist_nth (panel.priv->controls, index);
    
    /* Paranoid, should not happen here */
    if (!li)
	return;

    control = li->data;

    DBG ("unref class %s", control->cclass->caption);
    control_class_unref (control->cclass);

    control_unpack (control);

    panel.priv->controls = g_slist_delete_link (panel.priv->controls, li);
    control_free (control);

    settings.num_groups--;

    for (i = 0, li = panel.priv->controls; li != NULL; i++, li = li->next)
    {
	control = li->data;

	control->index = i;
    }

    write_panel_config ();
}

G_MODULE_EXPORT /* EXPORT:panel_add_control */
void
panel_add_control (Control * control, int index)
{
    int len;

    len = g_slist_length (panel.priv->controls);

    if (index < 0 || index > len)
	index = len;

    control->index = index;
    control_pack (control, panel.group_box);
    panel.priv->controls = g_slist_append (panel.priv->controls, control);

    if (index >= 0 && index < len)
	panel_move_control (len, index);

    if (control->with_popup)
    {
	item_control_show_popup (control, TRUE);
    }

    settings.num_groups++;

    write_panel_config ();
}

G_MODULE_EXPORT /* EXPORT:panel_get_n_controls */
int
panel_get_n_controls (void)
{
    return g_slist_length (panel.priv->controls);
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
    GSList *l;

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

    panel.priv->block_resize++;

    gtk_widget_hide (panel.toplevel);

    xfce_panel_window_set_orientation (XFCE_PANEL_WINDOW (panel.toplevel),
                                       orientation == VERTICAL ?
                                                GTK_ORIENTATION_VERTICAL :
                                                GTK_ORIENTATION_HORIZONTAL);
    
    xfce_itembar_set_orientation (XFCE_ITEMBAR (panel.group_box),
                                  orientation == VERTICAL ?
                                        GTK_ORIENTATION_VERTICAL :
                                        GTK_ORIENTATION_HORIZONTAL);
    
    for (l = panel.priv->controls; l != NULL; l = l->next)
    {
        Control *control = l->data;

        xfce_item_set_orientation (XFCE_ITEM (control->base), 
                                   orientation == VERTICAL ?
                                        GTK_ORIENTATION_VERTICAL :
                                        GTK_ORIENTATION_HORIZONTAL);
    }
    
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

    gtk_widget_size_request (panel.toplevel, &panel.priv->req);

    /* calls panel_set_position () */
    if (orientation == HORIZONTAL)
	panel_center (BOTTOM);
    else
	panel_center (RIGHT);

    gtk_widget_show (panel.toplevel);

    panel_set_position (&panel);

    if (hidden)
	panel_set_autohide (TRUE);

    set_translucent (&panel, TRUE);

    update_partial_struts (&panel);

    panel.priv->block_resize--;
}

G_MODULE_EXPORT /* EXPORT:panel_set_size */
void
panel_set_size (int size)
{
    GSList *l;
    
    panel.priv->settings.size = size;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    hide_current_popup_menu ();

    for (l = panel.priv->controls; l != NULL; l = l->next)
    {
        Control *control = l->data;

        control_set_size (control, size);
    }

    /* this will also resize the icons */
    panel_set_theme (panel.priv->settings.theme);
}

G_MODULE_EXPORT /* EXPORT:panel_set_popup_position */
void
panel_set_popup_position (int position)
{
    GSList *l;
    
    panel.priv->settings.popup_position = position;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    hide_current_popup_menu ();

    for (l = panel.priv->controls; l != NULL; l = l->next)
    {
        Control *control = l->data;

        item_control_set_popup_position (control, position);
    }

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
        GSList *l;

        for (l = panel.priv->controls; l != NULL; l = l->next)
        {
            Control *control = l->data;

            control_set_theme (control, theme);
        }
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

G_MODULE_EXPORT /* EXPORT:panel_set_autohide */
void
panel_set_autohide (gboolean hide)
{
    panel.priv->settings.autohide = hide;

    /* backwards compat */
    settings = panel.priv->settings;

    if (!panel.priv->is_created)
	return;

    panel_set_hidden (&panel, hide);
    gtk_widget_queue_resize (panel.toplevel);

    update_partial_struts (&panel);
}

G_MODULE_EXPORT /* EXPORT:panel_set_full_width */
void 
panel_set_full_width (gboolean full_width)
{
    panel.priv->full_width = full_width;

    xfce_panel_window_set_handle_style (XFCE_PANEL_WINDOW (panel.toplevel),
                                        full_width ?
                                        XFCE_HANDLE_STYLE_NONE :
                                        XFCE_HANDLE_STYLE_BOTH);

    if (panel.priv->is_created)
    {
        XfcePanelWindow *window = XFCE_PANEL_WINDOW (panel.toplevel);
        
        switch (panel.priv->side)
        {
            case TOP:
                xfce_panel_window_set_show_border (window,
                                                   FALSE, TRUE, 
                                                   !full_width, !full_width);
                break;
            case BOTTOM:
                xfce_panel_window_set_show_border (window, 
                                                   TRUE, FALSE, 
                                                   !full_width, !full_width);
                break;
            case LEFT:
                xfce_panel_window_set_show_border (window, 
                                                   !full_width, !full_width, 
                                                   FALSE, TRUE);
                break;
            case RIGHT:
                xfce_panel_window_set_show_border (window, 
                                                   !full_width, !full_width, 
                                                   TRUE, FALSE);
                break;
            default:
                xfce_panel_window_set_show_border (window, 
                                                   TRUE, TRUE, TRUE, TRUE);
        }

        if (full_width)
        {
            GdkRectangle *r = &panel.priv->monitor_geometry;
            if (panel.priv->settings.orientation == HORIZONTAL)
                gtk_widget_set_size_request (panel.toplevel, r->width, -1);
            else
                gtk_widget_set_size_request (panel.toplevel, -1, r->height);
        }
        else
        {
            gtk_widget_set_size_request (panel.toplevel, -1, -1);
            panel_center (panel.priv->side);
        }
        
    }
}

/*  Global preferences
 *  ------------------
*/
static void
init_settings (Panel * p)
{
    p->priv->settings.orientation = HORIZONTAL;

    p->priv->settings.size = SMALL;
    p->priv->settings.popup_position = RIGHT;

    p->priv->settings.autohide = FALSE;

    p->priv->settings.theme = NULL;

    p->priv->full_width = FALSE;

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

    value = xmlGetProp (node, (const xmlChar *) "size");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

        settings.size = CLAMP (n, TINY, LARGE);

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "popupposition");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

        settings.popup_position = CLAMP (n, LEFT, BOTTOM);

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

                panel.priv->monitor = CLAMP (n, 0, num_screens);

		g_free (value);

		value = xmlGetProp (child, (const xmlChar *) "side");

		if (value)
		{
		    n = (int) strtol (value, NULL, 0);

                    panel.priv->side = CLAMP (n, LEFT, BOTTOM);

		    g_free (value);
		}

		value = xmlGetProp (child, (const xmlChar *) "posstate");

		if (value)
		{
		    n = (int) strtol (value, NULL, 0);

                    panel.priv->pos_state = CLAMP (n, XFCE_POS_STATE_NONE,
                                                   XFCE_POS_STATE_END);

		    g_free (value);
		}

		if (panel.priv->pos_state != XFCE_POS_STATE_CENTER)
		{
		    value = xmlGetProp (child, (const xmlChar *) "offset");

		    if (value)
		    {
                        n = (int) strtol (value, NULL, 0);
                        
                        if (n > 0)
                            panel.priv->offset = n;

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

    snprintf (value, 2, "%d", settings.size);
    xmlSetProp (node, "size", value);

    snprintf (value, 2, "%d", settings.popup_position);
    xmlSetProp (node, "popupposition", value);

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

G_MODULE_EXPORT /* EXPORT:old_groups_set_from_xml */
void
old_groups_set_from_xml (int side, xmlNodePtr node)
{
    static int last_group = 0;
    xmlNodePtr child;
    int i;
    GSList *li;
    Control *control;

    if (side == LEFT)
	last_group = 0;

    li = g_slist_nth (panel.priv->controls, last_group);

    /* children are "Group" nodes */
    if (node)
	node = node->children;

    for (i = last_group; /*i < settings.num_groups || */ li || node; i++)
    {
	gboolean control_created = FALSE;

	if (side == LEFT && !node)
	    break;

	if (li)
	{
	    control = li->data;
	}
	else
	{
	    control = control_new (i);
	    control_pack (control, panel.group_box);
            panel.priv->controls = g_slist_append (panel.priv->controls,
                                                   control);
	}

	if (node)
	{
            xmlNodePtr popup_node = NULL;
            
	    for (child = node->children; child; child = child->next)
	    {
		/* create popup items and panel control */
		if (xmlStrEqual (child->name, (const xmlChar *) "Popup"))
		{
		    popup_node = child;
		}
		else if (xmlStrEqual
			 (child->name, (const xmlChar *) "Control"))
		{
		    control_set_from_xml (control, child);
		    control_created = TRUE;
		}
	    }

            if (control_created && popup_node)
                item_control_add_popup_from_xml (control, popup_node);
	}

	if (!control_created)
	    control_set_from_xml (control, NULL);

	if (node)
	    node = node->next;

	if (li)
	    li = li->next;

	last_group++;
    }
}

G_MODULE_EXPORT /* EXPORT:groups_set_from_xml */
void
groups_set_from_xml (xmlNodePtr node)
{
    int i;

    /* children are "Group" nodes */
    if (node)
	node = node->children;

    for (i = 0; node; i++, node = node->next)
    {
	gboolean control_created = FALSE;
	xmlNodePtr child, popup_node = NULL;
        Control *control;

	control = control_new (i);
	control_pack (control, panel.group_box);
	panel.priv->controls = g_slist_append (panel.priv->controls, control);

	for (child = node->children; child; child = child->next)
	{
	    /* create popup items and panel control */
	    if (xmlStrEqual (child->name, (const xmlChar *) "Popup"))
	    {
		popup_node = child;
	    }
	    else if (xmlStrEqual (child->name, (const xmlChar *) "Control"))
	    {
		control_created =
		    control_set_from_xml (control, child);

                if (control_created && popup_node)
                    item_control_add_popup_from_xml (control, popup_node);
	    }
	}

	if (!control_created)
	{
	    panel.priv->controls = g_slist_remove (panel.priv->controls,
                                                   control);
	    control_unpack (control);
	    control_free (control);
	}
    }
}

G_MODULE_EXPORT /* EXPORT:groups_write_xml */
void
groups_write_xml (xmlNodePtr root)
{
    xmlNodePtr node, child;
    GSList *li;
    Control *control;

    node = xmlNewTextChild (root, NULL, "Groups", NULL);

    for (li = panel.priv->controls; li; li = li->next)
    {
	control = li->data;

	child = xmlNewTextChild (node, NULL, "Group", NULL);

        /* special case for launchers */
        item_control_write_popup_xml (control, child);
	control_write_xml (control, child);
    }
}

/* for menus, to prevent problems with autohide */

static void
menu_destroyed (GtkWidget * menu, Panel * p)
{
    panel_unblock_autohide (p);

    if (p->priv->block_autohide == 0 
        && gdk_window_at_pointer (NULL, NULL) != p->toplevel->window)
    {
        if (p->priv->settings.autohide)
        {
            GdkEvent *ev = gdk_event_new (GDK_LEAVE_NOTIFY);

            ((GdkEventCrossing *) ev)->time = GDK_CURRENT_TIME;
            ((GdkEventCrossing *) ev)->detail = GDK_NOTIFY_NONLINEAR;

            gtk_widget_event (p->toplevel, ev);

            gdk_event_free (ev);
        }
        
        set_translucent (p, TRUE);
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

G_MODULE_EXPORT /* EXPORT:panel_get_arrow_direction */
GtkArrowType
panel_get_arrow_direction (Panel *p)
{
    return panel.priv->popup_arrow_type;
}

/* deprecated functions */
#ifndef XFCE_DISABLE_DEPRECATED
G_MODULE_EXPORT /* EXPORT:groups_get_arrow_direction */
GtkArrowType
groups_get_arrow_direction (void)
{
    return panel_get_arrow_direction (&panel);
}
#endif
