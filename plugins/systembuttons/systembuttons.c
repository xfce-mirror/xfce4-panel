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

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_iconbutton.h>
#include <libxfcegui4/xfce-icontheme.h>

#include <panel/mcs_client.h>
#include <panel/plugins.h>
#include <panel/xfce.h>


/* icons *
 * ----- */

enum
{
    MINILOCK_ICON,
    MINIINFO_ICON,
    MINIPALET_ICON,
    MINIPOWER_ICON,
    MINIBUTTONS
};

/* TODO: add kde names */
static char *minibutton_names[][3] = {
    {"xfce-system-lock", "gnome-lockscreen", NULL},
    {"xfce-system-info", "gnome-info", NULL},
    {"xfce-system-settings", "gnome-settings", NULL},
    {"xfce-system-exit", "gnome-logout", NULL},
};

static GdkPixbuf *
get_minibutton_pixbuf (int id)
{
    GdkPixbuf *pb;

    pb = themed_pixbuf_from_name_list (minibutton_names[id], 16);

    if (!pb)
	return get_pixbuf_by_id (UNKNOWN_ICON);

    return pb;
}


/* callbacks */
static void
mini_lock_cb (void)
{
    exec_cmd ("xflock4", FALSE, FALSE);
}

static void
mini_info_cb (void)
{
    exec_cmd ("xfce4-about", FALSE, FALSE);
}

static void
mini_palet_cb (void)
{
    if (disable_user_config)
    {
	xfce_info (_("Access to the configuration system has been disabled."
		     "\n\n"
		     "Ask your system administrator for more information"));
	return;
    }

    mcs_dialog ("all");
}

static void
mini_power_cb (GtkButton * b, GdkEventButton * ev, gpointer data)
{
    quit (FALSE);
}

/*  typedefs */
typedef struct
{
    const char *signal;
    GCallback callback;
    gpointer data;
}
SignalCallback;

enum
{ LOCK, EXIT, SETUP, INFO };

typedef struct
{
    int button_types[2];   /* 0, 1, 2 or 3 for lock, exit, setup and info */
    gboolean hide_two;

    GtkWidget *box;
    GtkWidget *buttons[2];
    GList *callbacks;	   /* save callbacks for when we change buttons */
    int cb_ids[2];
}
t_systembuttons;

typedef struct
{
    Control *control;
    t_systembuttons *sb;

    GtkWidget *option_menus[2];
    GtkWidget *label_two;
    GtkWidget *hide_checkbutton;
}
t_systembuttons_dialog;

/* change buttons */
static void
button_update_image (GtkWidget * button, int type)
{
    GdkPixbuf *pb = NULL;

    switch (type)
    {
	case LOCK:
	    pb = get_minibutton_pixbuf (MINILOCK_ICON);
	    break;
	case EXIT:
	    pb = get_minibutton_pixbuf (MINIPOWER_ICON);
	    break;
	case SETUP:
	    pb = get_minibutton_pixbuf (MINIPALET_ICON);
	    break;
	case INFO:
	    pb = get_minibutton_pixbuf (MINIINFO_ICON);
	    break;
    }

    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (button), pb);

    g_object_unref (pb);
}

static void
button_set_tip (GtkWidget * button, int type)
{
    static const char *tips[4];
    static gboolean need_init = TRUE;

    if (need_init)
    {
	tips[0] = _("Lock the screen");
	tips[1] = _("Exit");
	tips[2] = _("Setup");
	tips[3] = _("Info");
    }

    add_tooltip (button, tips[type]);
}

/* creation and destruction */
static int
connect_callback (GtkWidget * button, int type)
{
    GCallback callback = NULL;

    switch (type)
    {
	case LOCK:
	    callback = G_CALLBACK (mini_lock_cb);
	    break;
	case EXIT:
	    callback = G_CALLBACK (mini_power_cb);
	    break;
	case SETUP:
	    callback = G_CALLBACK (mini_palet_cb);
	    break;
	case INFO:
	    callback = G_CALLBACK (mini_info_cb);
	    break;
    }

    return g_signal_connect (button, "clicked", callback, NULL);
}

static void
create_systembutton (t_systembuttons * sb, int n, int type)
{
    GtkWidget *button;

    button = xfce_iconbutton_new ();
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_widget_show (button);

    button_set_tip (button, type);
    button_update_image (button, type);

    sb->cb_ids[n] = connect_callback (button, type);
    sb->buttons[n] = button;

    if (G_UNLIKELY (disable_user_config && type == SETUP))
	gtk_widget_set_sensitive (button, FALSE);
}

static void
systembuttons_change_type (t_systembuttons * sb, int n, int type)
{
    GtkWidget *button;

    sb->button_types[n] = type;

    button = sb->buttons[n];

    g_signal_handler_disconnect (sb->buttons[n], sb->cb_ids[n]);

    sb->cb_ids[n] = connect_callback (button, type);
    button_set_tip (button, type);
    button_update_image (button, type);

    if (G_UNLIKELY (disable_user_config && type == SETUP))
	gtk_widget_set_sensitive (button, FALSE);
    else
	gtk_widget_set_sensitive (button, TRUE);
}

static void
arrange_systembuttons (t_systembuttons * sb, int orientation)
{

    if (sb->box)
    {
	gtk_container_remove (GTK_CONTAINER (sb->box), sb->buttons[0]);
	gtk_container_remove (GTK_CONTAINER (sb->box), sb->buttons[1]);

	gtk_widget_destroy (sb->box);
    }

    if ((orientation == HORIZONTAL && settings.size <= SMALL)
	|| (orientation == VERTICAL && settings.size > SMALL))
    {
	sb->box = gtk_hbox_new (TRUE, 0);
    }
    else
    {
	sb->box = gtk_vbox_new (TRUE, 0);
    }

    gtk_widget_show (sb->box);

    gtk_widget_show (sb->buttons[0]);

    if (!sb->hide_two)
	gtk_widget_show (sb->buttons[1]);
    else
	gtk_widget_hide (sb->buttons[1]);

    gtk_box_pack_start (GTK_BOX (sb->box), sb->buttons[0], TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (sb->box), sb->buttons[1], TRUE, TRUE, 0);
}

static t_systembuttons *
systembuttons_new (void)
{
    t_systembuttons *sb = g_new0 (t_systembuttons, 1);

    sb->button_types[0] = 0;
    create_systembutton (sb, 0, 0);

    sb->button_types[1] = 1;
    create_systembutton (sb, 1, 1);

    g_object_ref (sb->buttons[0]);
    g_object_ref (sb->buttons[1]);

    arrange_systembuttons (sb, settings.orientation);

    return sb;
}

static void
systembuttons_free (Control * control)
{
    t_systembuttons *sb = control->data;

    g_list_foreach (sb->callbacks, (GFunc) g_free, NULL);
    g_list_free (sb->callbacks);

    g_free (sb);
}

static void
systembuttons_attach_callback (Control * control, const char *signal,
			       GCallback callback, gpointer data)
{
    t_systembuttons *sb = (t_systembuttons *) control->data;
    SignalCallback *cb;

    cb = g_new0 (SignalCallback, 1);
    cb->signal = signal;
    cb->callback = callback;
    cb->data = data;

    sb->callbacks = g_list_append (sb->callbacks, cb);

    g_signal_connect (sb->buttons[0], signal, callback, data);
    g_signal_connect (sb->buttons[1], signal, callback, data);
}

/* global prefs */
static void
systembuttons_set_size (Control * control, int size)
{
    int s, w, h, n;
    t_systembuttons *sb = control->data;

    arrange_systembuttons (sb, settings.orientation);
    gtk_container_add (GTK_CONTAINER (control->base), sb->box);

    s = w = h = icon_size[size] + border_width;

    n = sb->hide_two ? 1 : 2;

    if (settings.orientation == HORIZONTAL)
    {
	if (settings.size > SMALL)
	    w = s / 2;
	else
	    w = 0.75 * n * s;
    }
    else
    {
	if (settings.size > SMALL)
	    h = s / 2;
	else
	    h = 0.75 * n * s;
    }

    gtk_widget_set_size_request (sb->box, w, h);
}

static void
systembuttons_set_orientation (Control * control, int orientation)
{
    t_systembuttons *sb = control->data;

    arrange_systembuttons (sb, orientation);
    gtk_container_add (GTK_CONTAINER (control->base), sb->box);
}

static void
systembuttons_set_theme (Control * control, const char *theme)
{
    t_systembuttons *sb;

    sb = control->data;

    button_update_image (sb->buttons[0], sb->button_types[0]);
    button_update_image (sb->buttons[1], sb->button_types[1]);
}

/* settings */
static void
systembuttons_read_config (Control * control, xmlNodePtr node)
{
    xmlChar *value;
    int n;
    t_systembuttons *sb = (t_systembuttons *) control->data;

    value = xmlGetProp (node, (const xmlChar *) "button1");

    if (value)
    {
	n = atoi (value);
	g_free (value);

	if (n != sb->button_types[0])
	    systembuttons_change_type (sb, 0, n);
    }

    value = xmlGetProp (node, (const xmlChar *) "button2");

    if (value)
    {
	n = atoi (value);
	g_free (value);

	if (n != sb->button_types[1])
	    systembuttons_change_type (sb, 1, n);
    }

    value = xmlGetProp (node, (const xmlChar *) "showtwo");

    if (value)
    {
	n = atoi (value);
	g_free (value);

	if (n == 0)
	{
	    sb->hide_two = TRUE;
	    gtk_widget_hide (sb->buttons[1]);
	}
	else
	{
	    sb->hide_two = FALSE;
	    gtk_widget_show (sb->buttons[1]);
	}
    }

    /* Try to resize the systembuttons to fit the user settings */
    systembuttons_set_size (control, settings.size);
}

static void
systembuttons_write_config (Control * control, xmlNodePtr node)
{
    char prop[3];
    t_systembuttons *sb = control->data;

    sprintf (prop, "%d", sb->button_types[0]);

    xmlSetProp (node, (const xmlChar *) "button1", prop);

    sprintf (prop, "%d", sb->button_types[1]);

    xmlSetProp (node, (const xmlChar *) "button2", prop);

    sprintf (prop, "%d", sb->hide_two ? 0 : 1);

    xmlSetProp (node, (const xmlChar *) "showtwo", prop);
}

/* options dialog */
static void
checkbutton_changed (GtkToggleButton * tb, t_systembuttons_dialog * sbd)
{
    gboolean hide_two;

    hide_two = gtk_toggle_button_get_active (tb);

    if (hide_two != sbd->sb->hide_two)
    {
	sbd->sb->hide_two = hide_two;

        if (!hide_two)
            gtk_widget_show (sbd->sb->buttons[1]);
        else
            gtk_widget_hide (sbd->sb->buttons[1]);
        
        gtk_widget_set_sensitive (sbd->label_two, !hide_two);
        gtk_widget_set_sensitive (sbd->option_menus[1], !hide_two);

	systembuttons_set_size (sbd->control, settings.size);
    }
}

static void
buttons_changed (GtkOptionMenu * om, t_systembuttons_dialog * sbd)
{
    int i;
    int n = gtk_option_menu_get_history (om);

    i = (GTK_WIDGET (om) == sbd->option_menus[0]) ? 0 : 1;

    if (n == sbd->sb->button_types[i])
	return;

    systembuttons_change_type (sbd->sb, i, n);
}

void
systembuttons_create_options (Control * control, GtkContainer * container,
			      GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label, *om = NULL;
    const char *names[4];
    int i, j;
    t_systembuttons *sb;
    t_systembuttons_dialog *sbd;
    GtkSizeGroup *sg1, *sg2;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    sg1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    sg2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* initialize dialog structure */
    sb = control->data;
    sbd = g_new0 (t_systembuttons_dialog, 1);

    sbd->control = control;
    sbd->sb = sb;

    /* add widgets */
    vbox = gtk_vbox_new (FALSE, 6);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    /* set names to use in option menus */
    names[0] = _("Lock");
    names[1] = _("Exit");
    names[2] = _("Setup");
    names[3] = _("Info");

    /* two identical option menus */
    for (i = 0; i < 2; i++)
    {
	GtkWidget *menu;
	GtkWidget *mi;

	om = sbd->option_menus[i] = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (om), menu);

	for (j = 0; j < 4; j++)
	{
	    mi = gtk_menu_item_new_with_label (names[j]);
	    gtk_widget_show (mi);
	    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (om),
				     sb->button_types[i]);
	g_signal_connect (om, "changed", G_CALLBACK (buttons_changed), sbd);
    }

    /* buttons option menus */
    hbox = gtk_hbox_new (FALSE, 6);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Button 1:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg1, label);

    om = sbd->option_menus[0];
    gtk_widget_show (om);
    gtk_box_pack_start (GTK_BOX (hbox), om, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg2, om);

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = sbd->label_two = gtk_label_new (_("Button 2:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg1, label);
    gtk_widget_set_sensitive (label, !sb->hide_two);

    om = sbd->option_menus[1];
    gtk_widget_show (om);
    gtk_box_pack_start (GTK_BOX (hbox), om, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg2, om);
    gtk_widget_set_sensitive (om, !sb->hide_two);

    /* checkbox */
    sbd->hide_checkbutton = gtk_check_button_new_with_mnemonic (_("_Hide"));
    gtk_widget_show (sbd->hide_checkbutton);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sbd->hide_checkbutton),
				  sbd->sb->hide_two);
    gtk_box_pack_start (GTK_BOX (hbox), sbd->hide_checkbutton, FALSE, FALSE,
			0);

    g_signal_connect (sbd->hide_checkbutton, "toggled",
		      G_CALLBACK (checkbutton_changed), sbd);

    /* dialog buttons */
    g_signal_connect_swapped (done, "clicked", G_CALLBACK (g_free), sbd);
}

/* panel control */
gboolean
create_systembuttons_control (Control * control)
{
    t_systembuttons *sb = systembuttons_new ();

    gtk_widget_set_size_request (control->base, -1, -1);
    gtk_container_add (GTK_CONTAINER (control->base), sb->box);
    gtk_container_set_border_width (GTK_CONTAINER (control->base), 0);

    control->data = (gpointer) sb;
    control->with_popup = FALSE;

    systembuttons_set_size (control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "systembuttons";
    cc->caption = _("System buttons");

    cc->create_control = (CreateControlFunc) create_systembuttons_control;

    cc->free = systembuttons_free;
    cc->read_config = systembuttons_read_config;
    cc->write_config = systembuttons_write_config;
    cc->attach_callback = systembuttons_attach_callback;

    cc->set_size = systembuttons_set_size;
    cc->set_orientation = systembuttons_set_orientation;
    cc->set_theme = systembuttons_set_theme;
    cc->create_options = systembuttons_create_options;
}

XFCE_PLUGIN_CHECK_INIT
