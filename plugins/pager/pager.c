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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GDK_MULTIHEAD_SAFE
#undef GDK_MULTIHEAD_SAFE
#endif

#include <libxfce4util/i18n.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/xfce.h>
#include <panel/popup.h>
#include <panel/settings.h>
#include <panel/plugins.h>
#include <panel/mcs_client.h>

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

    int ws_created_id;
    int ws_destroyed_id;

    GtkWidget *base;
    GtkWidget *box;

    GtkWidget *separators[2];

    /* graphical pager */
    GtkWidget *netk_pager;

    /* callback(s) we have to save for reorientation */
    GList *callbacks;
}
t_pager;

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

/*  Graphical pager
 *  ---------------
*/
static void
netk_pager_update_size (GtkWidget * pager, NetkScreen * screen)
{
    int s = icon_size[settings.size];
    int count = netk_screen_get_workspace_count (screen);
    int w;

    if (settings.orientation == HORIZONTAL)
    {
	w = count * (int) ((double) (gdk_screen_width () * s) /
			   (double) (gdk_screen_height ()));

	gtk_widget_set_size_request (pager, w, s);
    }
    else
    {
	w = count * (int) ((double) (gdk_screen_height () * s) /
			   (double) (gdk_screen_width ()));

	gtk_widget_set_size_request (pager, s, w);
    }
}

GtkWidget *
create_netk_pager (NetkScreen * screen)
{
    GtkWidget *pager;
    GtkOrientation gor = (settings.orientation == VERTICAL) ?
	GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;

    pager = netk_pager_new (screen);
    netk_pager_set_n_rows (NETK_PAGER (pager), 1);
    netk_pager_set_orientation (NETK_PAGER (pager), gor);
    gtk_widget_show (pager);

    netk_pager_update_size (pager, screen);

    return pager;
}

/*  Desktop pager
 *  ----------------
*/

/* static prototypes */
static void arrange_pager (t_pager * pager);

/* settings */
static void
pager_set_size (Control * control, int size)
{
    t_pager *pager;

    gtk_widget_set_size_request (control->base, -1, -1);

    pager = control->data;

    netk_pager_update_size (pager->netk_pager, pager->screen);
}

static void
pager_set_orientation (Control * control, int orientation)
{
    t_pager *pager;

    pager = control->data;

    arrange_pager (pager);
    pager_set_size (control, settings.size);
}

/*  creation, destruction and configuration 
 *  ---------------------------------------
*/
static void
pager_attach_callback (Control * control, const char *signal,
		       GCallback callback, gpointer data)
{
    SignalCallback *sc;
    t_pager *pager;

    pager = control->data;

    sc = signal_callback_new (signal, callback, data);
    pager->callbacks = g_list_append (pager->callbacks, sc);

    g_signal_connect (pager->netk_pager, signal, callback, data);
}

static void
arrange_pager (t_pager * pager)
{
    GList *li;
    gboolean vertical = settings.orientation == VERTICAL;

    /* destroy existing widgets */
    if (pager->box)
    {
	gtk_widget_destroy (pager->box);
    }

    /* then create new ones */
    if (vertical)
    {
	pager->box = gtk_vbox_new (FALSE, 1);
	pager->separators[0] = gtk_hseparator_new ();
	pager->separators[1] = gtk_hseparator_new ();
    }
    else
    {
	pager->box = gtk_hbox_new (FALSE, 0);
	pager->separators[0] = gtk_vseparator_new ();
	pager->separators[1] = gtk_vseparator_new ();
    }

    pager->netk_pager = create_netk_pager (pager->screen);

    /* show the widgets */
    gtk_widget_show (pager->box);

    gtk_widget_show (pager->separators[0]);
    gtk_widget_show (pager->separators[1]);

    /* packing the widgets */
    gtk_container_add (GTK_CONTAINER (pager->base), pager->box);

    gtk_box_pack_start (GTK_BOX (pager->box), pager->separators[0], TRUE,
			TRUE, 2);

    {
	GtkWidget *align;

	align = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), pager->netk_pager);

	gtk_box_pack_start (GTK_BOX (pager->box), align, TRUE, TRUE, 2);
    }

    gtk_box_pack_start (GTK_BOX (pager->box), pager->separators[1], TRUE,
			TRUE, 2);

    /* attach callbacks */
    for (li = pager->callbacks; li; li = li->next)
    {
	SignalCallback *cb = li->data;

	g_signal_connect (pager->netk_pager,
			  cb->signal, cb->callback, cb->data);
    }
}

/* callbacks */
static void
pager_screen_created (NetkScreen * screen, NetkWorkspace * ws,
		      t_pager * pager)
{
    netk_pager_update_size (pager->netk_pager, pager->screen);
}

static void
pager_screen_destroyed (NetkScreen * screen, NetkWorkspace * ws,
			t_pager * pager)
{
    netk_pager_update_size (pager->netk_pager, pager->screen);
}

t_pager *
pager_new (NetkScreen * screen)
{
    t_pager *pager = g_new0 (t_pager, 1);

    pager->screen = screen;

    pager->base = gtk_alignment_new (0.5, 0.5, 0.8, 0.8);
    gtk_widget_show (pager->base);

    /* this creates all widgets */
    arrange_pager (pager);

    pager->ws_created_id =
	g_signal_connect (pager->screen, "workspace-created",
			  G_CALLBACK (pager_screen_created), pager);

    pager->ws_destroyed_id =
	g_signal_connect (pager->screen, "workspace-destroyed",
			  G_CALLBACK (pager_screen_destroyed), pager);

    return pager;
}

static void
pager_free (Control * control)
{
    GList *li;
    t_pager *pager;

    pager = control->data;

    g_signal_handler_disconnect (pager->screen, pager->ws_created_id);
    g_signal_handler_disconnect (pager->screen, pager->ws_destroyed_id);

    for (li = pager->callbacks; li; li = li->next)
	g_free (li->data);

    g_list_free (pager->callbacks);

    g_free (pager);
}

/*  Switcher panel control
 *  ----------------------
*/
gboolean
create_pager_control (Control * control)
{
    t_pager *pager;
    NetkScreen *screen;

    screen = netk_screen_get_default ();

    netk_screen_force_update (screen);
    pager = pager_new (screen);
    netk_screen_force_update (screen);

    gtk_container_add (GTK_CONTAINER (control->base), pager->base);

    control->data = pager;
    control->with_popup = FALSE;

    pager_set_size (control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    cc->name = "pager";
    cc->caption = _("Graphical pager");

    cc->create_control = (CreateControlFunc) create_pager_control;

    cc->free = pager_free;
    cc->attach_callback = pager_attach_callback;

    cc->set_orientation = pager_set_orientation;
    cc->set_size = pager_set_size;
}

XFCE_PLUGIN_CHECK_INIT
