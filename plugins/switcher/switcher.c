/*  $Id$
 *
 *  Copyright 2002 Jasper Huijsmans (jasper@xfce.org)
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/xfce.h>
#include <panel/popup.h>
#include <panel/settings.h>
#include <panel/plugins.h>

#if 0
#include <panel/mcs_client.h>
#endif

static int screen_button_width[] = { 35, 45, 80, 80 };

typedef struct
{
    int index;
    NetkWorkspace *workspace;

    int cb_id;

    GtkWidget *frame;
    GtkWidget *button;
    GtkWidget *label;
}
ScreenButton;

typedef struct
{
    const char *signal;
    GCallback callback;
    gpointer data;
}
SignalCallback;

typedef struct
{
    NetkScreen *screen;

    GtkWidget *box;
    GtkWidget *buttonboxes[2];
    GList *buttons;
}
CdePager;

typedef struct
{
    NetkScreen *screen;
    GPtrArray *screen_names;

    /* callback id's */
    int ws_changed_id;
    int ws_created_id;
    int ws_destroyed_id;

    GtkWidget *frame;
    GtkWidget *box;

/*    GtkWidget *separators[2];*/

    /* traditional switcher */
    CdePager *cde_pager;

    /* callback(s) we have to save for new buttons and for reorientation */
    GList *callbacks;
}
t_switcher;

/*  callback structure
 *  ------------------
 *  we want to keep track of callbacks to be able to 
 *  add them to new desktop buttons
*/
SignalCallback *
signal_callback_new (const char *signal, GCallback callback, gpointer data)
{
    SignalCallback *sc = g_new0 (SignalCallback, 1);

    sc->signal = signal;
    sc->callback = callback;
    sc->data = data;

    return sc;
}

/*  screenbuttons
 *  -------------
 *  traditional xfce desktop buttons
*/

/*  color names used for screen buttons
 *  may be used in gtk themes */
static char *screen_class[] = {
    "xfce_button1",
    "xfce_button2",
    "xfce_button3",
    "xfce_button4",
};

/*  callbacks */
static void
screen_button_update_label (ScreenButton * sb, const char *name)
{
    char *markup;

    switch (settings.size)
    {
	case TINY:
	    markup = g_strconcat ("<span size=\"smaller\">",
				  name, "</span>", NULL);
	    break;
	case LARGE:
	    markup = g_strconcat ("<span size=\"larger\">",
				  name, "</span>", NULL);
	    break;
	default:
	    markup = g_strdup (name);
    }

    gtk_label_set_markup (GTK_LABEL (sb->label), markup);
    g_free (markup);
}

static gboolean
screen_button_pressed_cb (GtkButton * b, GdkEventButton * ev,
			  ScreenButton * sb)
{
    hide_current_popup_menu ();

    if (ev->button == 1)
    {
	netk_workspace_activate (sb->workspace);
	return TRUE;
    }

    if (ev->button == 3 && disable_user_config)
    {
	return TRUE;
    }

    return FALSE;
}

/* settings */
static void
screen_button_update_size (ScreenButton * sb)
{
    int w, h;

    /* TODO:
     * calculation of height is very arbitrary. I just put here what looks
     * good on my screen. Should probably be something a little more
     * intelligent. */

    /* don't let screen buttons get too large in vertical mode */
    if (settings.orientation == VERTICAL && settings.size > SMALL)
	w = screen_button_width[MEDIUM] * 3 / 4;
    else
	w = screen_button_width[settings.size];

    switch (settings.size)
    {
	case TINY:
	    h = icon_size[TINY];
	    break;
	case SMALL:
	    h = -1;
	    break;
	case LARGE:
	    h = (top_height[LARGE] + icon_size[LARGE]) / 2 - 6;
	    break;
	default:
	    h = (top_height[MEDIUM] + icon_size[MEDIUM]) / 2 - 5;
	    break;
    }

    h = -1;
    gtk_widget_set_size_request (sb->button, w, h);

    screen_button_update_label (sb,
				gtk_label_get_text (GTK_LABEL (sb->label)));
}

/* creation and destruction */
static void
ws_name_changed (NetkWorkspace * ws, ScreenButton * sb)
{
    const char *name = netk_workspace_get_name (ws);

    gtk_label_set_text (GTK_LABEL (sb->label), name);
}

ScreenButton *
create_screen_button (int index, const char *name, NetkScreen * screen)
{
    const char *realname;
    ScreenButton *sb = g_new0 (ScreenButton, 1);

    sb->index = index;

    sb->workspace = netk_screen_get_workspace (screen, index);
    realname = netk_workspace_get_name (sb->workspace);

    if (!realname || !strlen (realname))
	realname = name;

    sb->cb_id = g_signal_connect (sb->workspace, "name-changed",
				  G_CALLBACK (ws_name_changed), sb);

    sb->frame = gtk_alignment_new (0, 0, 1, 1);	/* gtk_frame_new(NULL); */
    gtk_widget_show (sb->frame);

    sb->button = gtk_toggle_button_new ();
    gtk_button_set_relief (GTK_BUTTON (sb->button), GTK_RELIEF_HALF);
    gtk_widget_set_name (sb->button, screen_class[sb->index % 4]);
    gtk_widget_show (sb->button);
    gtk_container_add (GTK_CONTAINER (sb->frame), sb->button);

    sb->label = gtk_label_new (realname);
/*    gtk_misc_set_alignment(GTK_MISC(sb->label), 0.1, 0.5);*/
    gtk_widget_show (sb->label);
    gtk_container_add (GTK_CONTAINER (sb->button), sb->label);

    screen_button_update_size (sb);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->button),
				  sb->index == 0);

    g_signal_connect (sb->button, "button-press-event",
		      G_CALLBACK (screen_button_pressed_cb), sb);

    return sb;
}

void
screen_button_pack (ScreenButton * sb, GtkBox * box)
{
    gtk_box_pack_start (box, sb->frame, TRUE, TRUE, 0);
}

void
screen_button_free (ScreenButton * sb)
{
    g_signal_handler_disconnect (sb->workspace, sb->cb_id);
    g_free (sb);
}

/*  Traditional pager
 *  -----------------
 *  it's not completely stand-alone now, but perhaps we should make it a
 *  separate widget (?)
*/
static void
cde_pager_update_size (CdePager * pager)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	screen_button_update_size (sb);
    }

    /* check if we can rearrange the buttons (== horizontal mode) */
    if (!pager->buttonboxes[1])
	return;

    if (settings.size > SMALL)
	gtk_widget_show (pager->buttonboxes[1]);
    else
	gtk_widget_hide (pager->buttonboxes[1]);

    /* remove the buttons ... */
    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	g_object_ref (sb->frame);
	gtk_container_remove (GTK_CONTAINER (sb->frame->parent), sb->frame);
    }

    /* ... and put them back again */
    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	if (settings.size <= SMALL || sb->index % 2 == 0)
	{
	    gtk_box_pack_start (GTK_BOX (pager->buttonboxes[0]), sb->frame,
				TRUE, TRUE, 0);
	}
	else
	{
	    gtk_box_pack_start (GTK_BOX (pager->buttonboxes[1]), sb->frame,
				TRUE, TRUE, 0);
	}

	g_object_unref (sb->frame);
    }
}

static void
cde_pager_attach_callback (CdePager * pager, SignalCallback * sc)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	g_signal_connect (sb->button, sc->signal, sc->callback, sc->data);
    }
}

static void
cde_pager_add_button (CdePager * pager, GList * callbacks, GPtrArray * names)
{
    ScreenButton *sb;
    int i, index, active;
    GList *li;
    char *name;

    index = g_list_length (pager->buttons);
    active =
	netk_workspace_get_number (netk_screen_get_active_workspace
				   (pager->screen));

    for (i = names->len; i < index + 1; i++)
    {
	char tmp[3];

	sprintf (tmp, "%d", i + 1);

	g_ptr_array_add (names, g_strdup (tmp));
    }

    name = g_ptr_array_index (names, index);
    sb = create_screen_button (index, name, pager->screen);

    pager->buttons = g_list_append (pager->buttons, sb);

    if (settings.orientation == VERTICAL || settings.size <= SMALL ||
	index % 2 == 0)
    {
	screen_button_pack (sb, GTK_BOX (pager->buttonboxes[0]));
    }
    else
    {
	screen_button_pack (sb, GTK_BOX (pager->buttonboxes[1]));
    }

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->button),
				  index == active);

    for (li = callbacks; li; li = li->next)
    {
	SignalCallback *sc = li->data;

	g_signal_connect (sb->button, sc->signal, sc->callback, sc->data);
    }
}

static void
cde_pager_remove_button (CdePager * pager)
{
    GList *li;
    ScreenButton *sb;

    li = g_list_last (pager->buttons);
    sb = li->data;

    pager->buttons = g_list_delete_link (pager->buttons, li);

    gtk_widget_destroy (sb->frame);

    screen_button_free (sb);
}

static void
cde_pager_set_active (CdePager * pager, NetkWorkspace * ws)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sb->button),
				      sb->workspace == ws);
    }
}

CdePager *
create_cde_pager (NetkScreen * screen, GPtrArray * names)
{
    int i, n;

    CdePager *pager = g_new0 (CdePager, 1);

    pager->screen = screen;

    n = netk_screen_get_workspace_count (screen);

    if (settings.orientation == HORIZONTAL)
    {
	GtkWidget *spacer;

	pager->box = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (pager->box);

	spacer = gtk_alignment_new (0, 0, 0, 0);
	gtk_widget_show (spacer);
	gtk_box_pack_start (GTK_BOX (pager->box), spacer, TRUE, TRUE, 0);

	pager->buttonboxes[0] = gtk_hbox_new (TRUE, 2);
	gtk_widget_show (pager->buttonboxes[0]);
	gtk_box_pack_start (GTK_BOX (pager->box), pager->buttonboxes[0],
			    FALSE, FALSE, 2);

	pager->buttonboxes[1] = gtk_hbox_new (TRUE, 2);
	gtk_widget_show (pager->buttonboxes[1]);
	gtk_box_pack_start (GTK_BOX (pager->box), pager->buttonboxes[1],
			    FALSE, FALSE, 0);

	spacer = gtk_alignment_new (0, 0, 0, 0);
	gtk_widget_show (spacer);
	gtk_box_pack_start (GTK_BOX (pager->box), spacer, TRUE, TRUE, 0);
    }
    else
    {
	pager->box = pager->buttonboxes[0] = gtk_vbox_new (TRUE, 2);
	gtk_widget_show (pager->box);
    }

    for (i = 0; i < n; i++)
    {
	/* no callbacks yet */
	cde_pager_add_button (pager, NULL, names);
    }

    return pager;
}

static void
cde_pager_free (CdePager * pager)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	screen_button_free (sb);
    }

    g_list_free (pager->buttons);
    g_free (pager);
}

/*  Desktop switcher
 *  ----------------
*/

/* static prototypes */
static void arrange_switcher (t_switcher * sw);

/* settings */
static void
switcher_set_size (Control * control, int size)
{
    t_switcher *sw;

    gtk_widget_set_size_request (control->base, -1, -1);

    sw = control->data;

    cde_pager_update_size (sw->cde_pager);
}

static void
switcher_set_orientation (Control * control, int orientation)
{
    t_switcher *sw;

    sw = control->data;

    arrange_switcher (sw);
    switcher_set_size (control, settings.size);
}

/*  creation, destruction and configuration 
 *  ---------------------------------------
*/
static void
switcher_attach_callback (Control * control, const char *signal,
			  GCallback callback, gpointer data)
{
    SignalCallback *sc;
    t_switcher *sw;

    sw = control->data;

    sc = signal_callback_new (signal, callback, data);
    sw->callbacks = g_list_append (sw->callbacks, sc);

    cde_pager_attach_callback (sw->cde_pager, sc);
}

static void
arrange_switcher (t_switcher * sw)
{
    GList *li;
    gboolean vertical = settings.orientation == VERTICAL;

    /* destroy existing widgets */
    if (sw->box)
    {
	cde_pager_free (sw->cde_pager);

	gtk_widget_destroy (sw->box);
    }

    /* then create new ones */
    if (vertical)
    {
	sw->box = gtk_vbox_new (FALSE, 1);
/*	sw->separators[0] = gtk_hseparator_new ();
	sw->separators[1] = gtk_hseparator_new ();*/
    }
    else
    {
	sw->box = gtk_hbox_new (FALSE, 0);
/*	sw->separators[0] = gtk_vseparator_new ();
	sw->separators[1] = gtk_vseparator_new ();*/
    }

    sw->cde_pager = create_cde_pager (sw->screen, sw->screen_names);

    /* show the widgets */
    gtk_widget_show (sw->box);

/*    gtk_widget_show (sw->separators[0]);
    gtk_widget_show (sw->separators[1]);
*/
    /* packing the widgets */
    gtk_container_add (GTK_CONTAINER (sw->frame), sw->box);

/*    gtk_box_pack_start (GTK_BOX (sw->box), sw->separators[0], TRUE, TRUE, 2);
*/
    gtk_box_pack_start (GTK_BOX (sw->box), sw->cde_pager->box, TRUE, TRUE, 2);

/*    gtk_box_pack_start (GTK_BOX (sw->box), sw->separators[1], TRUE, TRUE, 2);
*/
    /* attach callbacks */
    for (li = sw->callbacks; li; li = li->next)
    {
	SignalCallback *cb = li->data;

	cde_pager_attach_callback (sw->cde_pager, cb);
    }
}

/* callbacks */
static void
switcher_set_current_screen (NetkScreen * screen, t_switcher * sw)
{
    NetkWorkspace *ws;

    ws = netk_screen_get_active_workspace (sw->screen);

    cde_pager_set_active (sw->cde_pager, ws);
}

static void
switcher_screen_created (NetkScreen * screen, NetkWorkspace * ws,
			 t_switcher * sw)
{
    cde_pager_add_button (sw->cde_pager, sw->callbacks, sw->screen_names);
}

static void
switcher_screen_destroyed (NetkScreen * screen, NetkWorkspace * ws,
			   t_switcher * sw)
{
    cde_pager_remove_button (sw->cde_pager);
}

t_switcher *
switcher_new (NetkScreen * screen)
{
    t_switcher *sw = g_new0 (t_switcher, 1);

    sw->screen = screen;

    sw->screen_names =
	g_ptr_array_sized_new (netk_screen_get_workspace_count (screen));

    sw->frame = gtk_alignment_new (0.5, 0.5, 0.8, 0.8);
    gtk_widget_show (sw->frame);

    /* this creates all widgets */
    arrange_switcher (sw);

    sw->ws_changed_id =
	g_signal_connect (sw->screen, "active-workspace-changed",
			  G_CALLBACK (switcher_set_current_screen), sw);

    sw->ws_created_id =
	g_signal_connect (sw->screen, "workspace-created",
			  G_CALLBACK (switcher_screen_created), sw);

    sw->ws_destroyed_id =
	g_signal_connect (sw->screen, "workspace-destroyed",
			  G_CALLBACK (switcher_screen_destroyed), sw);

    return sw;
}

static void
switcher_free (Control * control)
{
    GList *li;
    t_switcher *sw;

    sw = control->data;

    g_signal_handler_disconnect (sw->screen, sw->ws_changed_id);
    g_signal_handler_disconnect (sw->screen, sw->ws_created_id);
    g_signal_handler_disconnect (sw->screen, sw->ws_destroyed_id);

    cde_pager_free (sw->cde_pager);

    for (li = sw->callbacks; li; li = li->next)
	g_free (li->data);

    g_list_free (sw->callbacks);

    g_free (sw);
}

/*  Switcher panel control
 *  ----------------------
*/
gboolean
create_switcher_control (Control * control)
{
    t_switcher *sw;
    NetkScreen *screen;

    screen = netk_screen_get_default ();

    netk_screen_force_update (screen);
    sw = switcher_new (screen);
    netk_screen_force_update (screen);

    gtk_container_add (GTK_CONTAINER (control->base), sw->frame);

    control->data = sw;
    control->with_popup = FALSE;

    switcher_set_size (control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    cc->name = "switcher";
    cc->caption = _("Desktop switcher");

    cc->create_control = (CreateControlFunc) create_switcher_control;

    cc->free = switcher_free;
    cc->attach_callback = switcher_attach_callback;

    cc->set_orientation = switcher_set_orientation;
    cc->set_size = switcher_set_size;
}


XFCE_PLUGIN_CHECK_INIT
