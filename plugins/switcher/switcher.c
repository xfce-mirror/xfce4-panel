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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <libxfcegui4/libxfcegui4.h>

#include "xfce.h"
#include "popup.h"
#include "settings.h"
#include "plugins.h"
#include "mcs_client.h"

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
    
    gboolean show_minibuttons;
    gboolean show_names;
    gboolean graphical;
    
    GtkWidget *frame;
    GtkWidget *box;
    
    GtkWidget *separators[2];
    GtkWidget *minitables[2];
    GtkWidget *minibuttons[4];

    /* traditional switcher */
    CdePager *cde_pager;

    /* graphical pager */
    GtkWidget *netk_pager;
    
    /* callback(s) we have to save for new buttons */
    GList *callbacks;
}
t_switcher;

/*  callback structure
 *  ------------------
 *  we want to keep track of callbacks to be able to 
 *  add them to new desktop buttons
*/
SignalCallback *signal_callback_new(const char *signal, 
				    GCallback callback, gpointer data)
{
    SignalCallback *sc = g_new0(SignalCallback, 1);

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
static void screen_button_update_label(ScreenButton *sb, const char *name)
{
    char  *markup;

    switch (settings.size)
    {
	case TINY:
	    markup = g_strconcat("<span size=\"smaller\">",
		    		 name, "</span>", NULL);
	    break;
	case LARGE:
	    markup = g_strconcat("<span size=\"larger\">",
		    		 name, "</span>", NULL);
	    break;
	default:
	    markup = g_strdup(name);
    }

    gtk_label_set_markup(GTK_LABEL(sb->label), markup);
    g_free(markup);
}

static gboolean
screen_button_pressed_cb(GtkButton * b, GdkEventButton * ev, ScreenButton * sb)
{
    hide_current_popup_menu();

    if (ev->button == 1)
    {
	netk_workspace_activate(sb->workspace);
	return TRUE;
    }
        
    if(ev->button == 3 && disable_user_config)
    {
	return TRUE;
    }
    
    return FALSE;
}

/* settings */
static void screen_button_update_size(ScreenButton * sb)
{
    int w, h;

    /* TODO:
     * calculation of height is very arbitrary. I just put here what looks
     * good on my screen. Should probably be something a little more
     * intelligent. */

    /* don't let screen buttons get too large in vertical mode */
    if(settings.orientation == VERTICAL && settings.size > SMALL)
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
    gtk_widget_set_size_request(sb->button, w, h);

    screen_button_update_label(sb, gtk_label_get_text(GTK_LABEL(sb->label)));
}

/* creation and destruction */
#if 0
static void screen_button_modify_style(ScreenButton *sb)
{
    GtkStyle *style;
    GtkWidget *w;

    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize(w);
    
    style = gtk_widget_get_style(w);

    gtk_widget_modify_bg(sb->button, GTK_STATE_PRELIGHT, 
	    &(style->base[GTK_STATE_SELECTED]));
    gtk_widget_modify_fg(sb->label, GTK_STATE_PRELIGHT, 
	    &(style->fg[GTK_STATE_SELECTED]));
    
    gtk_widget_modify_bg(sb->button, GTK_STATE_ACTIVE, 
	    &(style->base[GTK_STATE_SELECTED]));
    gtk_widget_modify_fg(sb->label, GTK_STATE_ACTIVE, 
	    &(style->fg[GTK_STATE_SELECTED]));

    gtk_widget_modify_bg(sb->button, GTK_STATE_NORMAL, 
	    &(style->black));
    
    gtk_widget_destroy(w);
}
#endif

static void ws_name_changed(NetkWorkspace *ws, ScreenButton *sb)
{
    const char *name = netk_workspace_get_name(ws);

    gtk_label_set_text(GTK_LABEL(sb->label), name);
}

ScreenButton *create_screen_button(int index, const char *name, 
				   NetkScreen *screen)
{
    const char *realname;
    ScreenButton *sb = g_new0(ScreenButton, 1);

    sb->index = index;

    sb->workspace = netk_screen_get_workspace(screen, index);
    realname = netk_workspace_get_name(sb->workspace);

    if (!realname || !strlen(realname))
	realname = name;
    
    sb->cb_id = g_signal_connect(sb->workspace, "name-changed", 
	    	     		 G_CALLBACK(ws_name_changed), sb);
    
    sb->frame = gtk_alignment_new(0,0,1,1);/* gtk_frame_new(NULL);*/
    gtk_widget_show(sb->frame);
    
    sb->button = gtk_toggle_button_new();
    gtk_button_set_relief(GTK_BUTTON(sb->button), GTK_RELIEF_HALF);
    gtk_widget_set_name(sb->button, screen_class[sb->index % 4]);
    gtk_widget_show(sb->button);
    gtk_container_add(GTK_CONTAINER(sb->frame), sb->button);

    sb->label = gtk_label_new(realname);
/*    gtk_misc_set_alignment(GTK_MISC(sb->label), 0.1, 0.5);*/
    gtk_widget_show(sb->label); 
    gtk_container_add(GTK_CONTAINER(sb->button), sb->label);

    screen_button_update_size(sb);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->button), sb->index == 0);

    g_signal_connect(sb->button, "button-press-event",
                     G_CALLBACK(screen_button_pressed_cb), sb);

    return sb;
}

void screen_button_pack(ScreenButton * sb, GtkBox *box)
{
    gtk_box_pack_start(box, sb->frame, TRUE, TRUE, 0);
}

void screen_button_free(ScreenButton * sb)
{
    g_signal_handler_disconnect(sb->workspace, sb->cb_id);
    g_free(sb);
}

/*  Traditional pager
 *  -----------------
 *  it's not completely stand-alone now, but perhaps we should make it a
 *  separate widget (?)
*/
static void cde_pager_update_size(CdePager *pager)
{
    GList *li;
    
    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;
	
	screen_button_update_size(sb);
    }

    /* check if we can rearrange the buttons (== horizontal mode) */
    if (!pager->buttonboxes[1])
	return;

    if (settings.size > SMALL)
	gtk_widget_show(pager->buttonboxes[1]);
    else
	gtk_widget_hide(pager->buttonboxes[1]);
    
    /* remove the buttons ... */
    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	g_object_ref(sb->frame);
	gtk_container_remove(GTK_CONTAINER(sb->frame->parent), sb->frame);
    }

    /* ... and put them back again */
    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;
	
	if (settings.size <= SMALL || sb->index % 2 == 0)
	{
	    gtk_box_pack_start(GTK_BOX(pager->buttonboxes[0]), sb->frame,
		    	       TRUE, TRUE, 0);
	}
	else
	{
	    gtk_box_pack_start(GTK_BOX(pager->buttonboxes[1]), sb->frame,
		    	       TRUE, TRUE, 0);
	}

	g_object_unref(sb->frame);
    }
}

static void cde_pager_attach_callback(CdePager *pager, SignalCallback *sc)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	g_signal_connect(sb->button, sc->signal, sc->callback, sc->data);
    }
}

static void cde_pager_add_button(CdePager *pager, GList *callbacks, 
				 GPtrArray *names)
{
    ScreenButton *sb;
    int i, index, active;
    GList *li;
    char *name;

    index = g_list_length(pager->buttons);
    active = 
	netk_workspace_get_number(
		netk_screen_get_active_workspace(pager->screen));
    
    for (i = names->len; i < index + 1; i++)
    {
	char tmp[3];

	sprintf(tmp, "%d", i+1);

	g_ptr_array_add(names, g_strdup(tmp));
    }
    
    name = g_ptr_array_index(names, index);
    sb = create_screen_button(index, name, pager->screen);

    pager->buttons = g_list_append(pager->buttons, sb);

    if (settings.orientation == VERTICAL || settings.size <= SMALL || 
	    index % 2 == 0)
    {
	screen_button_pack(sb, GTK_BOX(pager->buttonboxes[0]));
    }
    else
    {
	screen_button_pack(sb, GTK_BOX(pager->buttonboxes[1]));
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->button),
				     index == active);

    for (li = callbacks; li; li = li->next)
    {
	SignalCallback *sc = li->data;

	g_signal_connect(sb->button, sc->signal, sc->callback, sc->data);
    }
}

static void cde_pager_remove_button(CdePager *pager)
{
    GList *li;
    ScreenButton *sb;

    li = g_list_last(pager->buttons);
    sb = li->data;

    pager->buttons = g_list_delete_link(pager->buttons, li);
    
    gtk_widget_destroy(sb->frame);

    screen_button_free(sb);
}

static void cde_pager_set_active(CdePager *pager, NetkWorkspace *ws)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sb->button),
				     sb->workspace == ws);
    }
}

CdePager *create_cde_pager(NetkScreen *screen, GPtrArray *names)
{
    int i, n;

    CdePager *pager = g_new0(CdePager, 1);

    pager->screen = screen;
    
    n = netk_screen_get_workspace_count(screen);

    if (settings.orientation == HORIZONTAL)
    {
	GtkWidget *spacer;

 	pager->box = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(pager->box);

	spacer = gtk_alignment_new(0,0,0,0);
	gtk_widget_show(spacer);
	gtk_box_pack_start(GTK_BOX(pager->box), spacer,
			   TRUE, TRUE, 0);
	
	pager->buttonboxes[0] = gtk_hbox_new(TRUE, 2);
	gtk_widget_show(pager->buttonboxes[0]);
	gtk_box_pack_start(GTK_BOX(pager->box), pager->buttonboxes[0],
			   FALSE, FALSE, 2);
	
	pager->buttonboxes[1] = gtk_hbox_new(TRUE, 2);
	gtk_widget_show(pager->buttonboxes[1]);
	gtk_box_pack_start(GTK_BOX(pager->box), pager->buttonboxes[1],
			   FALSE, FALSE, 0);
	
	spacer = gtk_alignment_new(0,0,0,0);
	gtk_widget_show(spacer);
	gtk_box_pack_start(GTK_BOX(pager->box), spacer,
			   TRUE, TRUE, 0);
    }
    else
    {
 	pager->box = pager->buttonboxes[0] = gtk_vbox_new(TRUE, 2);
	gtk_widget_show(pager->box);
    }

    for (i = 0; i < n; i++)
    {
	/* no callbacks yet */
	cde_pager_add_button(pager, NULL, names);
    }

    return pager;
}

static void cde_pager_free(CdePager *pager)
{
    GList *li;

    for (li = pager->buttons; li; li = li->next)
    {
	ScreenButton *sb = li->data;

	screen_button_free(sb);
    }
    
    g_list_free(pager->buttons);
    g_free(pager);
}

/*  Graphical pager
 *  ---------------
*/
static void netk_pager_update_size(GtkWidget *pager, NetkScreen *screen)
{
    int s =icon_size[settings.size];
    int count = netk_screen_get_workspace_count(screen);
    int w;

    if (settings.orientation == HORIZONTAL)
    {
	w = count * (int)((double)(gdk_screen_width() * s) / 
			    (double)(gdk_screen_height()));
	
	gtk_widget_set_size_request(pager, w, s);	
    }
    else
    {
	w =  count * (int)((double)(gdk_screen_height() * s) / 
			    (double)(gdk_screen_width()));
	
	gtk_widget_set_size_request(pager, s, w);
    }
}

GtkWidget *create_netk_pager(NetkScreen *screen)
{
    GtkWidget *pager;
    GtkOrientation gor = (settings.orientation == VERTICAL) ? 
			 GTK_ORIENTATION_VERTICAL : 
			 GTK_ORIENTATION_HORIZONTAL;
    
    pager = netk_pager_new(screen);
    netk_pager_set_n_rows(NETK_PAGER(pager), 1);
    netk_pager_set_orientation(NETK_PAGER(pager), gor);
    gtk_widget_show(pager);

    netk_pager_update_size(pager, screen);
    
    return pager;
}

/*  Mini buttons
 *  ------------
*/
static void mini_lock_cb(void)
{
    hide_current_popup_menu();

    exec_cmd("xflock", FALSE);
}

extern void info_panel_dialog(void);

static void mini_info_cb(void)
{
    hide_current_popup_menu();

    info_panel_dialog();
}

static void mini_palet_cb(void)
{
    hide_current_popup_menu();

    if(disable_user_config)
    {
        show_info(_("Access to the configuration system has been disabled.\n\n"
                    "Ask your system administrator for more information"));
        return;
    }

    mcs_dialog(NULL);
}

static void mini_power_cb(GtkButton * b, GdkEventButton * ev, gpointer data)
{
    hide_current_popup_menu();

    quit(FALSE);
}

static void create_minibuttons(t_switcher *sw)
{
    GdkPixbuf *pb[4];
    GtkWidget *button;
    int i;
    GList *li;

    pb[0] = get_minibutton_pixbuf(MINILOCK_ICON);
    pb[1] = get_minibutton_pixbuf(MINIINFO_ICON);
    pb[2] = get_minibutton_pixbuf(MINIPALET_ICON);
    pb[3] = get_minibutton_pixbuf(MINIPOWER_ICON);

    for(i = 0; i < 4; i++)
    {
	int size = minibutton_size[settings.size];

        sw->minibuttons[i] = button = xfce_iconbutton_new_from_pixbuf(pb[i]);
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_widget_show(button);
	gtk_widget_set_size_request(button,size,size);

	g_object_unref(pb[i]);

	for (li = sw->callbacks; li; li = li->next)
	{
	    SignalCallback *sc = li->data;

	    g_signal_connect(sw->minibuttons[i], sc->signal,
		    	     sc->callback, sc->data);
	}
    }
    
    /* fixed tooltips, since the user can't change the icon anyway */
    add_tooltip(sw->minibuttons[0], _("Lock the screen"));
    add_tooltip(sw->minibuttons[1], _("Info..."));
    add_tooltip(sw->minibuttons[2], _("Setup..."));
    add_tooltip(sw->minibuttons[3], _("Exit"));

    /* signals */
    g_signal_connect(sw->minibuttons[0], "clicked", 
		     G_CALLBACK(mini_lock_cb), NULL);

    g_signal_connect(sw->minibuttons[1], "clicked", 
		     G_CALLBACK(mini_info_cb), NULL);

    g_signal_connect(sw->minibuttons[2], "clicked",
                     G_CALLBACK(mini_palet_cb), NULL);

    g_signal_connect(sw->minibuttons[3], "clicked",
                     G_CALLBACK(mini_power_cb), NULL);
}

void add_minibuttons(t_switcher *sw)
{
    create_minibuttons(sw); 
    
    gtk_table_attach(GTK_TABLE(sw->minitables[0]), 
		     sw->minibuttons[0],
		     0, 1, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0); 
    
    gtk_table_attach(GTK_TABLE(sw->minitables[1]), 
		     sw->minibuttons[2], 
		     0, 1, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
    
    /* find if we want the minibuttons horizontally or vertically */
    if((settings.orientation == VERTICAL && settings.size > SMALL) ||
       (settings.orientation == HORIZONTAL && settings.size <= SMALL))
    {
	gtk_table_attach(GTK_TABLE(sw->minitables[0]),
			 sw->minibuttons[1],
			 1, 2, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(sw->minitables[1]),
			 sw->minibuttons[3],
			 1, 2, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
    }
    else
    {
	gtk_table_attach(GTK_TABLE(sw->minitables[0]),
			 sw->minibuttons[1],
			 0, 1, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(sw->minitables[1]),
			 sw->minibuttons[3],
			 0, 1, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
    }
}

/*  Desktop switcher
 *  ----------------
*/

/* static prototypes */
static void arrange_switcher(t_switcher *sw);

/* settings */
static void switcher_set_size(Control *control, int size)
{
    int i;
    t_switcher *sw;
    int s;

    gtk_widget_set_size_request(control->base, -1, -1);

    sw = control->data;

    s = minibutton_size[size];
    
    for (i = 0; i < 4; i++)
    {
	gtk_widget_destroy(sw->minibuttons[i]);
    }

    add_minibuttons(sw);

    if (sw->graphical)
    {
	netk_pager_update_size(sw->netk_pager, sw->screen);
    }
    else
    {
	cde_pager_update_size(sw->cde_pager);
    }
}

static void switcher_set_style(Control *control, int style)
{
    t_switcher *sw;

    sw = control->data;

    if (style == OLD_STYLE)
    {
	gtk_widget_hide(sw->separators[0]);
	gtk_widget_hide(sw->separators[1]);

	gtk_frame_set_shadow_type(GTK_FRAME(sw->frame), GTK_SHADOW_OUT);
    }
    else
    {
	gtk_widget_show(sw->separators[0]);
	gtk_widget_show(sw->separators[1]);

 	gtk_frame_set_shadow_type(GTK_FRAME(sw->frame), GTK_SHADOW_NONE);
    }
}

static void switcher_set_theme(Control *control, const char *theme)
{
    GdkPixbuf *pb [4];
    t_switcher *sw;
    int i;

    sw = control->data;

    pb[0] = get_minibutton_pixbuf(MINILOCK_ICON);
    pb[1] = get_minibutton_pixbuf(MINIINFO_ICON);
    pb[2] = get_minibutton_pixbuf(MINIPALET_ICON);
    pb[3] = get_minibutton_pixbuf(MINIPOWER_ICON);

    for (i = 0; i < 4; i++)
    {
 	xfce_iconbutton_set_pixbuf(XFCE_ICONBUTTON(sw->minibuttons[i]),
				   pb[i]);
 	g_object_unref(pb[i]);
    }
    
     switcher_set_size(control, settings.size);
}

static void switcher_set_orientation(Control *control, int orientation)
{
    t_switcher *sw;

    sw = control->data;

    arrange_switcher(sw);
    switcher_set_style(control, settings.style);
    switcher_set_size(control, settings.size);
}

/*  creation, destruction and configuration 
 *  ---------------------------------------
*/
static void switcher_read_config(Control *control, xmlNodePtr node)
{
    xmlChar *value;
    int i;
    t_switcher *sw;

    if(!node)
	return;

    sw = control->data;
    
    /* properties */
    value = xmlGetProp(node, "showminibuttons");

    if (value)
    {
	i = atoi(value);
	g_free(value);

	if (i == 0)
	{
	    sw->show_minibuttons = FALSE;

	    gtk_widget_hide(sw->minitables[0]);
	    gtk_widget_hide(sw->minitables[1]);
	}
    }

    value = xmlGetProp(node, "graphical");

    if (value)
    {
	i = atoi(value);
	g_free(value);

	if (i == 1)
	{
	    GtkWidget *align;
	    
	    sw->netk_pager = create_netk_pager(sw->screen);
	    sw->graphical = TRUE;

	    gtk_widget_destroy(sw->cde_pager->box);
	    cde_pager_free(sw->cde_pager);
	    sw->cde_pager = NULL;

	    align = gtk_alignment_new(0.5, 0.5, 0, 0);
	    gtk_widget_show(align);
	    gtk_container_add(GTK_CONTAINER(align), sw->netk_pager);
	    
	    gtk_box_pack_start(GTK_BOX(sw->box), align, 
		    	       TRUE, TRUE, 2);
	    gtk_box_reorder_child(GTK_BOX(sw->box), align, 2);
	}
    }
}

static void switcher_write_config(Control *control, xmlNodePtr node)
{
    char prop[3];
    t_switcher *sw;

    sw = control->data;
    
    snprintf(prop, 3, "%d", sw->show_minibuttons ? 1 : 0);
    xmlSetProp(node, "showminibuttons", prop);
    
    snprintf(prop, 3, "%d", sw->graphical ? 1 : 0);
    xmlSetProp(node, "graphical", prop);
}

static void switcher_attach_callback(Control *control, const char *signal, 
				     GCallback callback, gpointer data)
{
    SignalCallback *sc;
    t_switcher *sw;
    int i;

    sw = control->data;

    sc = signal_callback_new(signal, callback, data);
    sw->callbacks = g_list_append(sw->callbacks, sc);

    if (sw->graphical)
    {
	g_signal_connect(sw->netk_pager, signal, callback, data);
    }
    else
    {
	cde_pager_attach_callback(sw->cde_pager, sc);
    }

    for (i = 0; i < 4; i++)
    {
	g_signal_connect(sw->minibuttons[i], signal, callback, data);
    }
}

static void arrange_switcher(t_switcher *sw)
{
    GList *li;
    gboolean vertical = settings.orientation == VERTICAL;

    /* destroy existing widgets */
    if (sw->box)
    {
	if (!sw->graphical)
	    cde_pager_free(sw->cde_pager);
	
	gtk_widget_destroy(sw->box);
    }

    /* then create new ones */
    if (vertical)
    {
	sw->box = gtk_vbox_new(FALSE, 1);
	sw->separators[0] = gtk_hseparator_new();
	sw->separators[1] = gtk_hseparator_new();
    }
    else
    {
	sw->box = gtk_hbox_new(FALSE, 0);
	sw->separators[0] = gtk_vseparator_new();
	sw->separators[1] = gtk_vseparator_new();
    }

    if (sw->graphical)
    {
	sw->netk_pager = create_netk_pager(sw->screen);
    }
    else
    {
	sw->cde_pager = create_cde_pager(sw->screen, sw->screen_names);
    }

    sw->minitables[0] = gtk_table_new(2,2,FALSE);
    sw->minitables[1] = gtk_table_new(2,2,FALSE);
    add_minibuttons(sw);

    /* show the widgets */
    gtk_widget_show(sw->box);

    if (sw->show_minibuttons)
    {
	gtk_widget_show(sw->minitables[0]);
	gtk_widget_show(sw->minitables[1]);
    }
    
    if (settings.style == NEW_STYLE)
    {
	gtk_widget_show(sw->separators[0]);
	gtk_widget_show(sw->separators[1]);
    }

    /* packing the widgets */
    gtk_container_add(GTK_CONTAINER(sw->frame),sw->box);
    
    gtk_box_pack_start(GTK_BOX(sw->box), sw->separators[0], TRUE, TRUE, 2);

    gtk_box_pack_start(GTK_BOX(sw->box), sw->minitables[0], TRUE, TRUE, 0);
    
    if (sw->graphical)
    {
	GtkWidget *align;

	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(align);
	gtk_container_add(GTK_CONTAINER(align), sw->netk_pager);
	
	gtk_box_pack_start(GTK_BOX(sw->box), align, 
		    	       TRUE, TRUE, 2);
    }
    else
    {
	gtk_box_pack_start(GTK_BOX(sw->box), sw->cde_pager->box, 
			   TRUE, TRUE, 2);
    }
    
    gtk_box_pack_start(GTK_BOX(sw->box), sw->minitables[1], TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(sw->box), sw->separators[1], TRUE, TRUE, 2);

    /* attach callbacks */
    for (li = sw->callbacks; li; li = li->next)
    {
	SignalCallback *cb = li->data;
	
	if (sw->graphical)
	{
	    g_signal_connect(sw->netk_pager, 
		    	     cb->signal, cb->callback, cb->data);
	}
	else
	{
	    cde_pager_attach_callback(sw->cde_pager, cb);
	}
    }
}

/* callbacks */
static void switcher_set_current_screen(NetkScreen *screen, t_switcher *sw)
{
    if (!sw->graphical)
    {
	NetkWorkspace *ws;

	ws = netk_screen_get_active_workspace(sw->screen);

	cde_pager_set_active(sw->cde_pager, ws);
    }
}

static void switcher_screen_created(NetkScreen *screen, NetkWorkspace *ws,
				    t_switcher *sw)
{
    if (sw->graphical)
	netk_pager_update_size(sw->netk_pager, sw->screen);
    else
	cde_pager_add_button(sw->cde_pager, sw->callbacks, sw->screen_names);
}

static void switcher_screen_destroyed(NetkScreen *screen, NetkWorkspace *ws,
				      t_switcher *sw)
{
    if (sw->graphical)
	netk_pager_update_size(sw->netk_pager, sw->screen);
    else
	cde_pager_remove_button(sw->cde_pager);
}

t_switcher *switcher_new(NetkScreen *screen)
{
    t_switcher *sw = g_new0(t_switcher, 1);

    sw->screen = screen;

    sw->screen_names = 
	g_ptr_array_sized_new(netk_screen_get_workspace_count(screen));

    sw->show_minibuttons = TRUE;
    sw->show_names = TRUE;
    sw->graphical = FALSE;
    
    sw->frame = gtk_frame_new(NULL);
    gtk_widget_show(sw->frame);

    /* this creates all widgets */
    arrange_switcher(sw);    

    sw->ws_changed_id =
	g_signal_connect(sw->screen, "active-workspace-changed", 
		         G_CALLBACK(switcher_set_current_screen), sw);
    
    sw->ws_created_id =
	g_signal_connect(sw->screen, "workspace-created",
		         G_CALLBACK(switcher_screen_created), sw);
    
    sw->ws_destroyed_id =
	g_signal_connect(sw->screen, "workspace-destroyed",
		         G_CALLBACK(switcher_screen_destroyed), sw);

    return sw;
}

static void switcher_free(Control *control)
{
    GList *li;
    t_switcher *sw;

    sw = control->data;
    
    g_signal_handler_disconnect(sw->screen, sw->ws_changed_id);
    g_signal_handler_disconnect(sw->screen, sw->ws_created_id);
    g_signal_handler_disconnect(sw->screen, sw->ws_destroyed_id);
    
    if (!sw->graphical)
	cde_pager_free(sw->cde_pager);

    for (li = sw->callbacks; li; li = li->next)
	g_free(li->data);

    g_list_free(sw->callbacks);

    g_free(sw);
} 

/*  configuration dialog 
 *  --------------------
*/
typedef struct 
{
    t_switcher *sw;
    
    gboolean backup_show_minibuttons;
    gboolean backup_show_names;
    gboolean backup_graphical;

    GtkWidget *dialog;
    GtkWidget *revert;

    GtkWidget *mini_checkbutton;
    GtkWidget *names_checkbutton;
    GtkWidget *graphical_checkbutton;
}
t_switcher_dialog;

static void switcher_dialog_done(GtkWidget *b, t_switcher_dialog *sd)
{
    g_free(sd);
}

static void workspace_dialog(GtkWidget *b, t_switcher_dialog *sd)
{
    g_spawn_command_line_async("xfce-setting-show workspaces", NULL);
}

static void show_minibuttons_changed(GtkToggleButton *tb, t_switcher_dialog *sd)
{
    t_switcher *sw = sd->sw;
    gboolean show;

    show = gtk_toggle_button_get_active(tb);
    sw->show_minibuttons = show;
    
    if(show)
    {
        gtk_widget_show(sw->minitables[0]);
        gtk_widget_show(sw->minitables[1]);
    }
    else
    {
        gtk_widget_hide(sw->minitables[0]);
        gtk_widget_hide(sw->minitables[1]);
    }

    gtk_widget_set_sensitive(sd->revert, TRUE);
}

#if 0
static void show_names_changed(GtkToggleButton *tb, t_switcher_dialog *sd)
{
    t_switcher *sw = sd->sw;
    gboolean show;

    show = gtk_toggle_button_get_active(tb);
    sw->show_names = show;
    
    if(show)
    {
    }
    else
    {
    }

    gtk_widget_set_sensitive(sd->revert, TRUE);
}
#endif

static void graphical_changed(GtkToggleButton *tb, t_switcher_dialog *sd)
{
    t_switcher *sw = sd->sw;
    gboolean graphical;
    GList *li;
    
    graphical = gtk_toggle_button_get_active(tb);

    if (sw->graphical)
    {
	GtkWidget *align  = sw->netk_pager->parent;
	gtk_widget_destroy(sw->netk_pager);
	gtk_widget_destroy(align);
	sw->netk_pager = NULL;
    }
    else
    {
	gtk_widget_destroy(sw->cde_pager->box);
	cde_pager_free(sw->cde_pager);
	sw->cde_pager = NULL;
    }
    
    sw->graphical = graphical;
    
    if (graphical)
    {
	GtkWidget *align;
	
	sw->netk_pager = create_netk_pager(sw->screen);

	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(align);
	gtk_container_add(GTK_CONTAINER(align), sw->netk_pager);
	
	gtk_box_pack_start(GTK_BOX(sw->box), align, 
			   TRUE, TRUE, 2);
	gtk_box_reorder_child(GTK_BOX(sw->box), align, 2);
    }
    else
    {
	sw->cde_pager = create_cde_pager(sw->screen, sw->screen_names);
	
	gtk_box_pack_start(GTK_BOX(sw->box), sw->cde_pager->box, 
			   TRUE, TRUE, 2);
	gtk_box_reorder_child(GTK_BOX(sw->box), sw->cde_pager->box, 2);
    }

    for (li = sw->callbacks; li; li = li->next)
    {
	SignalCallback *cb = li->data;
	
	if (sw->graphical)
	{
	    g_signal_connect(sw->netk_pager, 
		    	     cb->signal, cb->callback, cb->data);
	}
	else
	{
	    cde_pager_attach_callback(sw->cde_pager, cb);
	}
    }

    gtk_widget_set_sensitive(sd->revert, TRUE);
}

static void switcher_revert(GtkWidget *b, t_switcher_dialog *sd)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->mini_checkbutton), 
	    			 sd->backup_show_minibuttons);
/*    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->names_checkbutton), 
	    			 sd->backup_show_names);
*/

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->graphical_checkbutton), 
	    			 sd->backup_graphical);
}  

static void switcher_add_options(Control *control, GtkContainer *container, 
				 GtkWidget *revert, GtkWidget *done)
{
    GtkWidget *vbox, *hbox, *label, *button;
    GtkSizeGroup *sg;
    t_switcher_dialog *sd;

    sd = g_new0(t_switcher_dialog, 1);
    
    sd->sw = control->data;

    sd->backup_show_minibuttons = sd->sw->show_minibuttons;
    sd->backup_show_names = sd->sw->show_names;
    sd->backup_graphical = sd->sw->graphical;
    
    sd->dialog = gtk_widget_get_toplevel(revert);
    sd->revert = revert;
    
    sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    
    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);
    gtk_container_add(container, vbox);

    /* show minibuttons ? */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Workspace options:"));
    gtk_widget_show(label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg,label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    button = gtk_button_new_from_stock(GTK_STOCK_PROPERTIES);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    
    g_signal_connect(button, "clicked", G_CALLBACK(workspace_dialog), sd);

    /* show minibuttons ? */
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Show minibuttons"));
    gtk_widget_show(label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg,label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    sd->mini_checkbutton = gtk_check_button_new();
    gtk_widget_show(sd->mini_checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), sd->mini_checkbutton, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->mini_checkbutton), 
	    			 sd->sw->show_minibuttons);
    g_signal_connect(sd->mini_checkbutton, "toggled", 
	    	     G_CALLBACK(show_minibuttons_changed), sd);

    /* show desktop names ? 
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Use desktop names:"));
    gtk_widget_show(label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg,label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    sd->names_checkbutton = gtk_check_button_new();
    gtk_widget_show(sd->names_checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), sd->names_checkbutton, FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->names_checkbutton), 
	    			 sd->sw->show_names);
    g_signal_connect(sd->names_checkbutton, "toggled", 
	    	     G_CALLBACK(show_names_changed), sd);
*/
    /* show graphical pager ?*/ 
    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("Use graphical pager"));
    gtk_widget_show(label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_size_group_add_widget(sg,label);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    sd->graphical_checkbutton = gtk_check_button_new();
    gtk_widget_show(sd->graphical_checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), sd->graphical_checkbutton, 
    		       FALSE, FALSE, 0);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sd->graphical_checkbutton), 
	    			 sd->sw->graphical);
    g_signal_connect(sd->graphical_checkbutton, "toggled", 
	    	     G_CALLBACK(graphical_changed), sd);

    g_signal_connect(revert, "clicked", G_CALLBACK(switcher_revert), sd);
    g_signal_connect(done, "clicked", G_CALLBACK(switcher_dialog_done), sd);
}

/*  Switcher panel control
 *  ----------------------
*/
gboolean create_switcher_control(Control *control)
{
    t_switcher *sw;
    NetkScreen *screen;

    screen = netk_screen_get_default();

    netk_screen_force_update(screen);
    sw = switcher_new(screen);
    netk_screen_force_update(screen);

    gtk_container_add(GTK_CONTAINER(control->base), sw->frame);
    
    control->data = sw;
    control->with_popup = FALSE;
    
    switcher_set_style(control, settings.style);
    switcher_set_size(control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc)
{
    cc->name = "switcher";
    cc->caption = _("Desktop switcher");
    
    cc->create_control = (CreateControlFunc) create_switcher_control;

    cc->free = switcher_free;
    cc->read_config = switcher_read_config;
    cc->write_config = switcher_write_config;
    cc->attach_callback = switcher_attach_callback;
    
    cc->add_options = switcher_add_options;

    cc->set_orientation = switcher_set_orientation;
    cc->set_size = switcher_set_size;
    cc->set_style = switcher_set_style;
    cc->set_theme = switcher_set_theme;
}


XFCE_PLUGIN_CHECK_INIT

