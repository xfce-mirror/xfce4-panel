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

/*  systembuttons.c : xfce4 panel plugin that shows one or two 
 *  'system buttons'. These are the same buttons also present on 
 *  both sides of the switcher plugin: 
 *  lock, exit, setup and info.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_iconbutton.h>

#include <panel/mcs_client.h>
#include <panel/plugins.h>
#include <panel/xfce.h>

#if 0
#include "mcs_client.h"
#endif

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
	xfce_info (_
		   ("Access to the configuration system has been disabled.\n\n"
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
    gboolean show_two;
    int button_types[2];	/* 0, 1, 2 or 3 for lock, exit, setup and info */

    GtkWidget *box;
    GtkWidget *buttonbox;
    GtkWidget *align[2];	/* containers for the actual buttons */

    GList *callbacks;		/* save callbacks for when we change buttons */
}
t_systembuttons;

typedef struct
{
    int backup_show_two;
    int backup_button_types[2];

    Control *control;
    t_systembuttons *sb;

    GtkWidget *two_checkbutton;
    GtkWidget *option_menu_hbox[2];
    GtkWidget *option_menus[2];
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
static GtkWidget *
create_systembutton (int type)
{
    GtkWidget *button;
    GCallback callback = NULL;

    button = xfce_iconbutton_new ();
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    gtk_widget_show (button);

    button_set_tip (button, type);
    button_update_image (button, type);

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

    g_signal_connect (button, "clicked", callback, NULL);

    return button;
}

static void
systembuttons_change_type (t_systembuttons * sb, int n, int type)
{
    GtkWidget *button;
    GList *li;

    sb->button_types[n] = type;

    button = gtk_bin_get_child (GTK_BIN (sb->align[n]));
    gtk_widget_destroy (button);

    button = create_systembutton (type);
    gtk_container_add (GTK_CONTAINER (sb->align[n]), button);

    for (li = sb->callbacks; li; li = li->next)
    {
	SignalCallback *cb = li->data;

	g_signal_connect (button, cb->signal, cb->callback, cb->data);
    }
}

static void
arrange_systembuttons (t_systembuttons * sb, int orientation)
{
    GtkWidget *align;
    int spacing;

    if (sb->box)
    {
	gtk_container_remove (GTK_CONTAINER (sb->buttonbox), sb->align[0]);
	gtk_container_remove (GTK_CONTAINER (sb->buttonbox), sb->align[1]);

	gtk_widget_destroy (sb->box);
    }

    /* we need some extra spacing when the panel size <= SMALL
     * hence the strange code below :) */
    if ((orientation == HORIZONTAL && settings.size > SMALL) ||
	(orientation == VERTICAL && settings.size <= SMALL))
    {
	if (settings.size > SMALL)
	{
	    sb->box = gtk_vbox_new (TRUE, 0);
	    sb->buttonbox = sb->box;
	}
	else
	{
	    sb->box = gtk_vbox_new (FALSE, 0);
	    sb->buttonbox = gtk_vbox_new (TRUE, 0);
	}
    }
    else
    {
	if (settings.size > SMALL)
	{
	    sb->box = gtk_hbox_new (FALSE, 0);
	    sb->buttonbox = sb->box;
	}
	else
	{
	    sb->box = gtk_hbox_new (FALSE, 0);
	    sb->buttonbox = gtk_hbox_new (TRUE, 0);
	}
    }

    gtk_widget_show (sb->box);
    gtk_widget_show (sb->buttonbox);

    spacing = border_width / 2;

    if (settings.size <= SMALL)
    {
	align = gtk_alignment_new (0, 0, 0, 0);
	gtk_widget_set_size_request (align, spacing, spacing);
	gtk_widget_show (align);
	gtk_box_pack_start (GTK_BOX (sb->box), align, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (sb->box), sb->buttonbox, TRUE, TRUE, 0);
    }

    gtk_box_pack_start (GTK_BOX (sb->buttonbox), sb->align[0], TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (sb->buttonbox), sb->align[1], TRUE, TRUE, 0);

    if (settings.size <= SMALL)
    {
	align = gtk_alignment_new (0, 0, 0, 0);
	gtk_widget_set_size_request (align, spacing, spacing);
	gtk_widget_show (align);
	gtk_box_pack_start (GTK_BOX (sb->box), align, FALSE, FALSE, 0);
    }
}

static t_systembuttons *
systembuttons_new (void)
{
    GtkWidget *button;
    t_systembuttons *sb = g_new0 (t_systembuttons, 1);

    sb->show_two = TRUE;

    sb->align[0] = gtk_alignment_new (0, 0, 1, 1);
    gtk_widget_show (sb->align[0]);

    sb->button_types[0] = 0;
    button = create_systembutton (0);
    gtk_container_add (GTK_CONTAINER (sb->align[0]), button);

    sb->align[1] = gtk_alignment_new (0, 0, 1, 1);
    gtk_widget_show (sb->align[1]);

    sb->button_types[1] = 1;
    button = create_systembutton (1);
    gtk_container_add (GTK_CONTAINER (sb->align[1]), button);

    g_object_ref (sb->align[0]);
    g_object_ref (sb->align[1]);

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
    GtkWidget *button;

    cb = g_new0 (SignalCallback, 1);
    cb->signal = signal;
    cb->callback = callback;
    cb->data = data;

    sb->callbacks = g_list_append (sb->callbacks, cb);

    button = gtk_bin_get_child (GTK_BIN (sb->align[0]));
    g_signal_connect (button, signal, callback, data);

    button = gtk_bin_get_child (GTK_BIN (sb->align[1]));
    g_signal_connect (button, signal, callback, data);
}

/* global prefs */
static void
systembuttons_set_size (Control * control, int size)
{
    int s1, s2, n;
    t_systembuttons *sb = control->data;

    arrange_systembuttons (sb, settings.orientation);
    gtk_container_add (GTK_CONTAINER (control->base), sb->box);

    n = sb->show_two ? 2 : 1;
    s1 = icon_size[size] + border_width;
    s2 = s1 * 0.75;

    if ((settings.orientation == VERTICAL && size <= SMALL) ||
	(settings.orientation == HORIZONTAL && size > SMALL))
    {
	if (size > SMALL)
	    gtk_widget_set_size_request (sb->buttonbox, s2, s1);
	else
	    gtk_widget_set_size_request (sb->buttonbox, s2, s1 * n * 0.75);
    }
    else
    {
	if (size > SMALL)
	    gtk_widget_set_size_request (sb->buttonbox, s1, s2);
	else
	    gtk_widget_set_size_request (sb->buttonbox, s1 * n * 0.75, s2);
    }
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
    GtkWidget *button;
    t_systembuttons *sb;

    sb = control->data;

    button = gtk_bin_get_child (GTK_BIN (sb->align[0]));
    button_update_image (button, sb->button_types[0]);

    button = gtk_bin_get_child (GTK_BIN (sb->align[1]));
    button_update_image (button, sb->button_types[1]);
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
	    sb->show_two = FALSE;
	    gtk_widget_hide (sb->align[1]);
	}
	else
	{
	    sb->show_two = TRUE;
	    gtk_widget_show (sb->align[1]);
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

    sprintf (prop, "%d", sb->show_two ? 1 : 0);

    xmlSetProp (node, (const xmlChar *) "showtwo", prop);
}

/* options dialog */
static void
checkbutton_changed (GtkToggleButton * tb, t_systembuttons_dialog * sbd)
{
    gboolean show_two;

    show_two = gtk_toggle_button_get_active (tb);

    if (show_two != sbd->sb->show_two)
    {
	sbd->sb->show_two = show_two;

	if (show_two)
	{
	    gtk_widget_show (sbd->sb->align[1]);
	    gtk_widget_show (sbd->option_menu_hbox[1]);
	}
	else
	{
	    gtk_widget_hide (sbd->sb->align[1]);
	    gtk_widget_hide (sbd->option_menu_hbox[1]);
	}

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

#if 0
static void
systembuttons_revert_configuration (t_systembuttons_dialog * sbd)
{
    GtkOptionMenu *om;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sbd->two_checkbutton),
				  sbd->backup_show_two);

    om = GTK_OPTION_MENU (sbd->option_menus[0]);
    gtk_option_menu_set_history (om, sbd->backup_button_types[0]);

    om = GTK_OPTION_MENU (sbd->option_menus[1]);
    gtk_option_menu_set_history (om, sbd->backup_button_types[1]);
}
#endif

void
systembuttons_add_options (Control * control, GtkContainer * container,
			   GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label, *om = NULL;
    const char *names[4];
    int i, j;
    t_systembuttons *sb;
    t_systembuttons_dialog *sbd;
    GtkSizeGroup *sg1, *sg2;

    sg1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    sg2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    /* initialize dialog structure */
    sb = control->data;
    sbd = g_new0 (t_systembuttons_dialog, 1);

    sbd->backup_show_two = sb->show_two;
    sbd->backup_button_types[0] = sb->button_types[0];
    sbd->backup_button_types[1] = sb->button_types[1];

    sbd->control = control;
    sbd->sb = sb;

    /* add widgets */
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    /* show two checkbutton */
    hbox = gtk_hbox_new (FALSE, 4);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Show two buttons:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg1, label);

    sbd->two_checkbutton = gtk_check_button_new ();
    gtk_widget_show (sbd->two_checkbutton);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sbd->two_checkbutton),
				  sbd->sb->show_two);
    gtk_box_pack_start (GTK_BOX (hbox), sbd->two_checkbutton, FALSE, FALSE,
			0);

    g_signal_connect (sbd->two_checkbutton, "toggled",
		      G_CALLBACK (checkbutton_changed), sbd);

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
    hbox = sbd->option_menu_hbox[0] = gtk_hbox_new (FALSE, 4);
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

    hbox = sbd->option_menu_hbox[1] = gtk_hbox_new (FALSE, 4);
    if (sb->show_two)
	gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Button 2:"));
    gtk_widget_show (label);
    gtk_misc_set_alignment (GTK_MISC (label), 1, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg1, label);

    om = sbd->option_menus[1];
    gtk_widget_show (om);
    gtk_box_pack_start (GTK_BOX (hbox), om, FALSE, FALSE, 0);
    gtk_size_group_add_widget (sg2, om);

    /* dialog buttons */
    g_signal_connect_swapped (done, "clicked", G_CALLBACK (g_free), sbd);
}

/* panel control */
gboolean
create_systembuttons_control (Control * control)
{
    t_systembuttons *sb = systembuttons_new ();

    gtk_container_add (GTK_CONTAINER (control->base), sb->box);

    control->data = (gpointer) sb;
    control->with_popup = FALSE;

    systembuttons_set_size (control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
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
    cc->add_options = systembuttons_add_options;
}

XFCE_PLUGIN_CHECK_INIT
