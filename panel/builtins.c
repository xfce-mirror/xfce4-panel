/*  builtins.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
 *                     Xavier Maillard (zedek@fxgsproject.org)
 *                     Olivier Fourdan
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "builtins.h"
#include "controls.h"
#include "iconbutton.h"
#include "xfce_support.h"
#include "icons.h"
#include "iconbutton.h"
#include "callbacks.h"
#include "xfce.h"
#include "xfce_clock.h"

#include "debug.h"

/* panel control configuration
   Global widget used in all the module configuration
   to revert the settings
*/
GtkWidget *revert_button;


/* Clock tooltip - From xfce3 */
/* FIXME: Find another place for that */
static char *day_names[] = { N_("Sunday"),
    N_("Monday"),
    N_("Tuesday"),
    N_("Wednesday"),
    N_("Thursday"),
    N_("Friday"),
    N_("Saturday")
};

static char *month_names[] = { N_("January"),
    N_("February"),
    N_("March"),
    N_("April"),
    N_("May"),
    N_("June"),
    N_("July"),
    N_("August"),
    N_("September"),
    N_("October"),
    N_("November"),
    N_("December")
};

/*  Clock module
 *  ------------
 *  To be replaced by the 'real clock widget'
 *  FIXME: do some cleanup in the code (zeDek)
 *         add user config
 *         check for user config to create our clock (type,
 *         size,etc...)
 *
 *
*/

/* Our real clock */
typedef struct
{
    GtkWidget *frame;
    GtkWidget *eventbox;
    GtkWidget *clock;           /* our XfceClock widget */
}
t_clock;


/* Our static backup clock structure used to store/retrieve infos
   in the configuration dialog
*/
struct ClockBackup
{
    gint i_mode;
    gint b_military;
    gint b_ampm;
    gint b_secs;
};

static struct ClockBackup backup;

/* I know there is a better way but :) */
/* Global widget to be able to check for their state wherever
   we need them (actually usefull for the configuration dialog
*/
/* FIXME: Try to do it without global var */
static GtkWidget *ampmbutton, *secsbutton, *checkbutton, *om;


static t_clock *clock_new(void)
{
    t_clock *clock = g_new(t_clock, 1);
    
    clock->clock = xfce_clock_new();
    g_object_ref(G_OBJECT(clock->clock));

    clock->frame = gtk_frame_new(NULL);
    gtk_container_set_border_width(GTK_CONTAINER(clock->frame), 0);

    gtk_widget_show(clock->frame);

    clock->eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(clock->frame), clock->eventbox);
    gtk_widget_show(clock->eventbox);


    gtk_container_add(GTK_CONTAINER(clock->eventbox), clock->clock);

    gtk_widget_show(clock->clock);

    return clock;
}

#if 0
static gboolean adjust_time(PanelControl * pc)
{
    time_t t;
    struct tm *tm;
    int hrs, mins;
    char *text, *markup;

    t_clock *clock = (t_clock *) pc->data;

    t = time(0);
    tm = localtime(&t);

    hrs = tm->tm_hour;
    mins = tm->tm_min;

    text =
        g_strdup_printf("%.2d%c%.2d", hrs, clock->show_colon ? ':' : ' ', mins);

    switch (settings.size)
    {
        case TINY:
            markup =
                g_strconcat("<tt><span size=\"x-small\">", text, "</span></tt>",
                            NULL);
            break;
        case SMALL:
            markup =
                g_strconcat("<tt><span size=\"small\">", text, "</span></tt>",
                            NULL);
            break;
        case LARGE:
            markup =
                g_strconcat("<tt><span size=\"large\">", text, "</span></tt>",
                            NULL);
            break;
        default:
            markup =
                g_strconcat("<tt><span size=\"medium\">", text, "</span></tt>",
                            NULL);
            break;
    }

    clock->show_colon = !clock->show_colon;

    gtk_label_set_markup(GTK_LABEL(clock->label), markup);

    g_free(text);
    g_free(markup);

    return TRUE;
}
#endif

void clock_free(PanelControl * pc)
{
    t_clock *clock = (t_clock *) pc->data;
    g_return_if_fail( clock != NULL );
    if(GTK_IS_WIDGET(clock->clock))
        gtk_widget_destroy(clock->clock);
    if(GTK_IS_WIDGET(clock->eventbox))
        gtk_widget_destroy(clock->eventbox);
    if(GTK_IS_WIDGET(clock->frame))
        gtk_widget_destroy(clock->frame);
    g_free(clock);
}

void update_clock_size(XfceClock *clock, int size)
{
    if ((xfce_clock_get_mode(clock) == XFCE_CLOCK_LEDS) || (xfce_clock_get_mode(clock) == XFCE_CLOCK_DIGITAL))
    {
        gtk_widget_set_size_request(GTK_WIDGET(clock), -1, -1);
    }
    else
    {
        gtk_widget_set_size_request(GTK_WIDGET(clock), icon_size[size], icon_size[size]);
    }
    gtk_widget_queue_resize (GTK_WIDGET(clock));
}

void clock_set_size(PanelControl * pc, int size)
{
    int s = icon_size[size];
    int w = s;
    t_clock *clock = (t_clock *) pc->data;
    XfceClock *tmp = XFCE_CLOCK(clock->clock);
    switch (size)
    {
        case 0:
          xfce_clock_set_led_size(tmp, DIGIT_SMALL);
	  break;
	case 1:
          xfce_clock_set_led_size(tmp, DIGIT_MEDIUM);
	  break;
	case 2:
          xfce_clock_set_led_size(tmp, DIGIT_LARGE);
	  break;
	default:
          xfce_clock_set_led_size(tmp, DIGIT_HUGE);
    }
    update_clock_size(tmp, size);
}

/* FIXME: have to have a look into it as I don't really know if that
   still works :). At least it compiles.
*/
void clock_set_style(PanelControl * pc, int style)
{
    t_clock *clock = (t_clock *) pc->data;

    if(style == OLD_STYLE)
    {
        gtk_widget_set_name(clock->frame, "gxfce_color2");
	gtk_widget_set_name(clock->clock, "gxfce_color2");
        gtk_widget_set_name(clock->eventbox, "gxfce_color2");
	if(XFCE_CLOCK(clock->clock)->mode == XFCE_CLOCK_ANALOG)
            gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_NONE);
	else
	    gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_IN);
    }
    else
    {
        gtk_widget_set_name(clock->frame, "gxfce_color4");
	gtk_widget_set_name(clock->clock, "gxfce_color4");
        gtk_widget_set_name(clock->eventbox, "gxfce_color4");
	gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_NONE);
    }
}

gboolean
clock_date_tooltip (GtkWidget * widget)
{
    time_t ticks;
    struct tm *tm;
    static gint mday = -1;
    static gint wday = -1;
    static gint mon = -1;
    static gint year = -1;
    char date_s[255];

    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

    ticks = time(0);
    tm = localtime(&ticks);
    if((mday != tm->tm_mday) || (wday != tm->tm_wday) || (mon != tm->tm_mon) ||
       (year != tm->tm_year))
    {
        mday = tm->tm_mday;
        wday = tm->tm_wday;
        mon = tm->tm_mon;
        year = tm->tm_year;
        snprintf(date_s, 255, "%s, %u %s %u", _(day_names[wday]), mday,
                 _(month_names[mon]), year + 1900);
        add_tooltip(widget, _(date_s));
    }
    return TRUE;
}


/* Update the widgets' state of the configuration dialog to
    reflect change
*/
static void clock_update_options_box(t_clock *clock)
{

    gtk_option_menu_set_history(GTK_OPTION_MENU(om),XFCE_CLOCK( clock->clock )->mode);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), XFCE_CLOCK( clock->clock )->military_time);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ampmbutton), XFCE_CLOCK( clock->clock )->display_am_pm);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(secsbutton), XFCE_CLOCK( clock->clock )->display_secs);

}

/* Update the clock size whenever a config settings has changed */
static void update_size(GtkToggleButton * tb, PanelControl* pc)
{
    clock_set_size(pc,settings.size);
}
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static void clock_type_changed(GtkOptionMenu * omi, t_clock * clock)
{
    XFCE_CLOCK(clock->clock)->mode = gtk_option_menu_get_history(omi);

    /* No frame *please* (OF.) */
#if 0
    /* Cosmetic change : in analog mode 3D mode is not cute */
    if(XFCE_CLOCK(clock->clock)->mode == XFCE_CLOCK_ANALOG)
        gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_NONE);
    else
	gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_IN);
#else
    gtk_frame_set_shadow_type(GTK_FRAME(clock->frame), GTK_SHADOW_NONE);
#endif

    xfce_clock_set_mode(XFCE_CLOCK(clock->clock), XFCE_CLOCK(clock->clock)->mode);
    gtk_widget_queue_resize (GTK_WIDGET(clock->clock));
     /* Make the revert_button sensitive to get our initial value back
      */
     gtk_widget_set_sensitive( revert_button, TRUE );
}


static GtkWidget *create_clock_type_option_menu(t_clock * clock, PanelControl* pc)
{
    GtkWidget *menu, *mi, *omi;

    (void) omi;

    om = gtk_option_menu_new();

    menu = gtk_menu_new();
    gtk_widget_show(menu);

    mi = gtk_menu_item_new_with_label(_("Analog"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("Digital"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    mi = gtk_menu_item_new_with_label(_("LED"));
    gtk_widget_show(mi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(om), menu);

    gtk_option_menu_set_history(GTK_OPTION_MENU(om),
                                XFCE_CLOCK(clock->clock)->mode);

    g_signal_connect(om, "changed", G_CALLBACK(clock_type_changed), clock);
    g_signal_connect(om, "changed", G_CALLBACK(update_size), pc);
    return om;
}

static void clock_hour_mode_changed(GtkToggleButton * tb, t_clock * clock)
{
    XFCE_CLOCK(clock->clock)->military_time = gtk_toggle_button_get_active(tb);
    if(XFCE_CLOCK(clock->clock)->military_time == 1)
    {
        /* Disable ampm mode when we use 24h mode */
        gtk_widget_set_sensitive(ampmbutton, FALSE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ampmbutton), FALSE);
    }
    else
        gtk_widget_set_sensitive(ampmbutton, TRUE);


    xfce_clock_show_military(XFCE_CLOCK(clock->clock),
                             XFCE_CLOCK(clock->clock)->military_time);
     /* Make the revert_button sensitive to get our initial value back
      */
     gtk_widget_set_sensitive( revert_button, TRUE );
     gtk_widget_set_size_request(clock->frame,-1, -1);
     gtk_widget_queue_resize (GTK_WIDGET(clock->clock));
}

static GtkWidget *create_clock_24hrs_button(t_clock * clock, PanelControl* pc)
{
    GtkWidget *cb;
    cb = gtk_check_button_new();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                                 XFCE_CLOCK(clock->clock)->military_time);


    xfce_clock_show_military(XFCE_CLOCK(clock->clock),
			     XFCE_CLOCK(clock->clock)->military_time);

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_hour_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), pc);

    return cb;
}

static void clock_secs_mode_changed(GtkToggleButton * tb, t_clock * clock)
{
    XfceClock *tmp = XFCE_CLOCK(clock->clock);
    XFCE_CLOCK(clock->clock)->display_secs = gtk_toggle_button_get_active(tb);

    xfce_clock_show_secs(XFCE_CLOCK(clock->clock),
			 XFCE_CLOCK(clock->clock)->display_secs);

     /* Make the revert_button sensitive to get our initial value back
      */
     gtk_widget_set_sensitive( revert_button, TRUE );
     gtk_widget_set_size_request(clock->frame, -1, -1);
     gtk_widget_queue_resize (GTK_WIDGET(clock->clock));
}

static GtkWidget *create_clock_secs_button(t_clock * clock, PanelControl* pc)
{
    GtkWidget *cb;

    cb = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                                 XFCE_CLOCK(clock->clock)->display_secs);


    xfce_clock_show_secs(XFCE_CLOCK(clock->clock),
			 XFCE_CLOCK(clock->clock)->display_secs);

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_secs_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), pc);

    return cb;
}

static void clock_ampm_mode_changed(GtkToggleButton * tb, t_clock * clock)
{
    XFCE_CLOCK(clock->clock)->display_am_pm = gtk_toggle_button_get_active(tb);


    xfce_clock_show_ampm( XFCE_CLOCK(clock->clock),
			  XFCE_CLOCK(clock->clock)->display_am_pm);

     /* Make the revert_button sensitive to get our initial value back
	*/
    gtk_widget_set_sensitive( revert_button, TRUE );
    gtk_widget_set_size_request(clock->frame,-1, -1);
    gtk_widget_queue_resize (GTK_WIDGET(clock->clock));
}

static GtkWidget *create_clock_ampm_button(t_clock * clock, PanelControl* pc)
{
    GtkWidget *cb;
    cb = gtk_check_button_new();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
				 XFCE_CLOCK(clock->clock)->display_am_pm &&
				 ! XFCE_CLOCK(clock->clock)->military_time);

    gtk_widget_set_sensitive(cb, ! XFCE_CLOCK(clock->clock)->military_time);
    xfce_clock_show_ampm( XFCE_CLOCK(clock->clock), XFCE_CLOCK(clock->clock)->display_am_pm );

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_ampm_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), pc);
    return cb;
}


/* Store old clock infos to be retrieved later on */
static void clock_create_backup(t_clock *cl)
{
    XfceClock *tmp = XFCE_CLOCK( cl->clock );
    g_return_if_fail( tmp != NULL );

    backup.i_mode = tmp->mode;
    backup.b_military = tmp->military_time;
    backup.b_ampm = tmp->display_am_pm;
    backup.b_secs = tmp->display_secs;
}

/* Delete backup */
static void clock_clean_backup(void)
{
    /* Nothing to do here */

}

/* Get back our clock structure
   FIXME: use this whenever an entry in the configuration dialog has
   changed
*/
static void clock_revert(t_clock *clock)
{
    XFCE_CLOCK( clock->clock )->mode = backup.i_mode;
    XFCE_CLOCK( clock->clock )->military_time = backup.b_military;
    XFCE_CLOCK( clock->clock )->display_am_pm = backup.b_ampm;
    XFCE_CLOCK( clock->clock )->display_secs = backup.b_secs;

    clock_update_options_box(clock);
}

/* Write the configuration at exit */
void clock_write_config(PanelControl *pc, xmlNodePtr parent)
{
    xmlNodePtr root, node;
    char value[MAXSTRLEN+1];

    t_clock *cl = (t_clock *) pc->data;
    XfceClock *clock = XFCE_CLOCK(cl->clock);

    /* I use my own node even if we only have 4 settings
       It's safer and easier to check
    */
    root = xmlNewTextChild(parent, NULL, "XfceClock", NULL);
    g_snprintf(value,2,"%d", clock->mode);
    xmlSetProp(root, "Clock_type", value);
    g_snprintf(value,2,"%d", clock->military_time);
    xmlSetProp(root, "Toggle_military", value);
    g_snprintf(value,2,"%d", clock->display_am_pm);
    xmlSetProp(root, "Toggle_am_pm",value);
    g_snprintf(value,2,"%d", clock->display_secs);
    xmlSetProp(root,"Toggle_secs",value);
}

/* Read the configuration file at init */
void clock_read_config(PanelControl *pc, xmlNodePtr node)
{
    xmlChar *value;
    int n;

    t_clock *cl = (t_clock *)pc->data;

    if(!node || !node->children)
        return;

    node = node->children;

    /* Leave if we can't find the node XfceClock */
    if(!xmlStrEqual(node->name, "XfceClock"))
        return;

    /* No frame *please* (OF.) */
#if 0	
    if ( value = xmlGetProp(node, (const xmlChar *)"Clock_type"))
    {
	XFCE_CLOCK(cl->clock)->mode = atoi(value);
	if(xfce_clock_get_mode(XFCE_CLOCK(cl->clock)) == XFCE_CLOCK_ANALOG )
	    gtk_frame_set_shadow_type(GTK_FRAME(cl->frame), GTK_SHADOW_NONE);
	else
	    gtk_frame_set_shadow_type(GTK_FRAME(cl->frame), GTK_SHADOW_IN);
        g_free(value);
    }
#else
    if (value = xmlGetProp(node, (const xmlChar *)"Clock_type"))
    {
	XFCE_CLOCK(cl->clock)->mode = atoi(value);
        g_free(value);
    }
    gtk_frame_set_shadow_type(GTK_FRAME(cl->frame), GTK_SHADOW_NONE);
#endif

    if ( value = xmlGetProp(node, (const xmlChar *)"Toggle_military"))
    {
	XFCE_CLOCK(cl->clock)->military_time = atoi(value);
	g_free(value);
    }
    if ( value = xmlGetProp(node, (const xmlChar *)"Toggle_am_pm"))
    {
	XFCE_CLOCK(cl->clock)->display_am_pm = atoi(value);
        g_free(value);
    }
    if ( value = xmlGetProp(node, (const xmlChar *)"Toggle_secs"))
    {
	XFCE_CLOCK(cl->clock)->display_secs = atoi(value);
        g_free(value);
    }

    /* Try to resize the clock to fit the user settings */
    clock_set_size(pc, settings.size);
}

static void clock_apply_configuration(PanelControl * pc)
{
    t_clock *cl = (t_clock*) pc->data;

    XFCE_CLOCK(cl->clock)->display_am_pm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ampmbutton));
    XFCE_CLOCK(cl->clock)->display_secs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(secsbutton));
    XFCE_CLOCK(cl->clock)->military_time = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton));
    XFCE_CLOCK(cl->clock)->mode = gtk_option_menu_get_history(GTK_OPTION_MENU(om));

    /* Clean the backup whenever we confirm our changes */
    /* Actually that does nothing but it's to be consistent with the
       API */
    clock_clean_backup();
}

void clock_add_options(PanelControl * pc, GtkContainer * container,
                       GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *om, *hbox, *label;

    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    t_clock *clock = (t_clock *) pc->data;

    /* Make a backup copy of our current settings */
    clock_create_backup(clock);

    revert_button = revert;

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Clock type:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    om = create_clock_type_option_menu(clock,pc);
    gtk_widget_show(om);
    gtk_box_pack_start(GTK_BOX(hbox), om, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("24 hour clock:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    checkbutton = create_clock_24hrs_button(clock,pc);
    gtk_widget_show(checkbutton);
    gtk_box_pack_start(GTK_BOX(hbox), checkbutton, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Toggle AM PM mode :"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    ampmbutton = create_clock_ampm_button(clock,pc);
    gtk_widget_show(ampmbutton);
    gtk_box_pack_start(GTK_BOX(hbox), ampmbutton, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Show seconds:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    secsbutton = create_clock_secs_button(clock,pc);
    gtk_widget_show(secsbutton);
    gtk_box_pack_start(GTK_BOX(hbox), secsbutton, FALSE, FALSE, 0);

    /* signals */
    g_signal_connect_swapped(revert, "clicked",
                             G_CALLBACK(clock_revert), clock);
    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(clock_clean_backup), NULL);
    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(clock_apply_configuration), pc);

    gtk_container_add(container, vbox);

}

/*  create clock panel control
*/
void create_clock(PanelControl * pc)
{
    t_clock *clock = clock_new();

    gtk_container_add(GTK_CONTAINER(pc->base), clock->frame);

    pc->caption = g_strdup(_("XFce clock"));
    pc->data = (gpointer) clock;
    pc->main = clock->frame;

    pc->interval = 1000;        /* 1 sec */

    pc->free = (gpointer) clock_free;

    pc->read_config = clock_read_config;
    pc->write_config = clock_write_config;

    pc->set_size = (gpointer) clock_set_size;
    pc->set_style = (gpointer) clock_set_style;

    pc->add_options = (gpointer) clock_add_options;

    /* Add tooltip to show up the current date */
    /*
      FIXME: this is a bug in fact. The tooltipe never gets updated.
       This is a problem if the panel is not reloaded once a day
    because the current day may be totally wrong. I know the way to
    fix it but as it's not really critical, I let it down for the
    moment
    */
    clock_date_tooltip (clock->eventbox);

}

/* -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- */
/* -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- */

/*  Trash module
 *  ------------
*/
typedef struct
{
    char *dirname;
    char *command;
    gboolean in_terminal;

    gboolean empty;

    GdkPixbuf *empty_pb;
    GdkPixbuf *full_pb;

    IconButton *button;
}
t_trash;

static void trash_run(t_trash * trash)
{
    exec_cmd(trash->command, trash->in_terminal);
}

void trash_dropped(GtkWidget * widget, GList * drop_data, gpointer data)
{
    t_trash *trash = (t_trash *) data;
    char *cmd;
    GList *li;

    for(li = drop_data; li && li->data; li = li->next)
    {
        cmd = g_strconcat(trash->command, " ", (char *)li->data, NULL);

        exec_cmd_silent(cmd, FALSE);

        g_free(cmd);
    }
}

static t_trash *trash_new(void)
{
    t_trash *trash = g_new(t_trash, 1);
    const char *home = g_getenv("HOME");
    GtkWidget *b;

    trash->dirname = g_strconcat(home, "/.xfce/trash", NULL);

    trash->empty = TRUE;

    trash->command = g_strdup("xftrash");
    trash->in_terminal = FALSE;

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    trash->button = icon_button_new(trash->empty_pb);

    b = icon_button_get_button(trash->button);

    add_tooltip(b, _("Trashcan: 0 files"));

    /* signals */
    dnd_set_drag_dest(b);
    dnd_set_callback(b, DROP_CALLBACK(trash_dropped), trash);

    g_signal_connect_swapped(b, "clicked", G_CALLBACK(trash_run), trash);

    return trash;
}

static gboolean check_trash(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    GDir *dir;
    const char *file;
    char text[MAXSTRLEN];
    gboolean changed = FALSE;

    if(!trash->dirname)
        return TRUE;
    dir = g_dir_open(trash->dirname, 0, NULL);

    if(dir)
        file = g_dir_read_name(dir);

    if(!dir || !file)
    {
        if(!trash->empty)
        {
            trash->empty = TRUE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->empty_pb);
            add_tooltip(icon_button_get_button(trash->button),
                        _("Trashcan: 0 files"));
        }
    }
    else
    {
        struct stat s;
        int number = 0;
        int size = 0;
        char *cwd = g_get_current_dir();

        chdir(trash->dirname);

        if(trash->empty)
        {
            trash->empty = FALSE;
            changed = TRUE;
            icon_button_set_pixbuf(trash->button, trash->full_pb);
        }

        while(file)
        {
            number++;

            stat(file, &s);
            size += s.st_size;

            file = g_dir_read_name(dir);
        }

        chdir(cwd);
        g_free(cwd);

        if(size < 1024)
            sprintf(text, _("Trashcan: %d files / %d B"), number, size);
        else if(size < 1024 * 1024)
            sprintf(text, _("Trashcan: %d files / %d KB"), number, size / 1024);
        else
            sprintf(text, _("Trashcan: %d files / %d MB"), number,
                    size / (1024 * 1024));

        add_tooltip(icon_button_get_button(trash->button), text);
    }

    if(dir)
        g_dir_close(dir);

    return TRUE;
}

static void trash_free(PanelControl * pc)
{
    t_trash *trash = (t_trash *) pc->data;

    g_free(trash->dirname);

    icon_button_free(trash->button);

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    g_free(trash);
}

static void trash_set_size(PanelControl * pc, int size)
{
    t_trash *trash = (t_trash *) pc->data;

    icon_button_set_size(trash->button, size);
}

static void trash_set_theme(PanelControl * pc, const char *theme)
{
    t_trash *trash = (t_trash *) pc->data;

    g_object_unref(trash->empty_pb);
    g_object_unref(trash->full_pb);

    trash->empty_pb = get_module_pixbuf(TRASH_EMPTY_ICON);
    trash->full_pb = get_module_pixbuf(TRASH_FULL_ICON);

    if(trash->empty)
        icon_button_set_pixbuf(trash->button, trash->empty_pb);
    else
        icon_button_set_pixbuf(trash->button, trash->full_pb);
}

/*  create trash panel control
*/
void create_trash(PanelControl * pc)
{
    t_trash *trash = trash_new();
    GtkWidget *b = icon_button_get_button(trash->button);

    gtk_container_add(GTK_CONTAINER(pc->base), b);

    pc->caption = g_strdup(_("Trash can"));
    pc->data = (gpointer) trash;
    pc->main = b;

    pc->interval = 2000;        /* 2 sec */
    pc->update = (gpointer) check_trash;

    pc->free = (gpointer) trash_free;

    pc->set_size = (gpointer) trash_set_size;
    pc->set_theme = (gpointer) trash_set_theme;
}

/*  Exit module
 *  -----------
*/
typedef struct
{
    GtkWidget *vbox;

    GtkWidget *exit_button;
    GtkWidget *lock_button;
}
t_exit;

static t_exit *exit_new(void)
{
    GtkWidget *im;
    t_exit *exit = g_new(t_exit, 1);

    exit->vbox = gtk_vbox_new(TRUE, 0);
    gtk_widget_show(exit->vbox);

    exit->lock_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(exit->lock_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->lock_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->lock_button, TRUE, TRUE, 0);

    im = gtk_image_new();
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(exit->lock_button), im);

    exit->exit_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(exit->exit_button), GTK_RELIEF_NONE);
    gtk_widget_show(exit->exit_button);
    gtk_box_pack_start(GTK_BOX(exit->vbox), exit->exit_button, TRUE, TRUE, 0);

    im = gtk_image_new();
    gtk_widget_show(im);
    gtk_container_add(GTK_CONTAINER(exit->exit_button), im);

    add_tooltip(exit->lock_button, _("Lock the screen"));
    add_tooltip(exit->exit_button, _("Exit"));

    g_signal_connect_swapped(exit->lock_button, "clicked",
                             G_CALLBACK(mini_lock_cb), NULL);

    g_signal_connect_swapped(exit->exit_button, "clicked", G_CALLBACK(quit),
                             NULL);

    return exit;
}

static void exit_free(PanelControl * pc)
{
    t_exit *exit = (t_exit *) pc->data;

    if(GTK_IS_WIDGET(pc->main))
        gtk_widget_destroy(pc->main);

    g_free(exit);
}

static void exit_set_size(PanelControl * pc, int size)
{
    t_exit *exit = (t_exit *) pc->data;
    GdkPixbuf *pb1, *pb2;
    GtkWidget *im;
    int width, height;

    width = icon_size[size];
    height = icon_size[size] / 2;

    gtk_widget_set_size_request(exit->exit_button, width + 4, height + 2);
    gtk_widget_set_size_request(exit->lock_button, width + 4, height + 2);

    pb1 = get_system_pixbuf(MINIPOWER_ICON);
    im = gtk_bin_get_child(GTK_BIN(exit->exit_button));
    pb2 = get_scaled_pixbuf(pb1, height - 4);
    gtk_image_set_from_pixbuf(GTK_IMAGE(im), pb2);

    g_object_unref(pb1);
    g_object_unref(pb2);

    pb1 = get_system_pixbuf(MINILOCK_ICON);
    im = gtk_bin_get_child(GTK_BIN(exit->lock_button));
    pb2 = get_scaled_pixbuf(pb1, height - 4);
    gtk_image_set_from_pixbuf(GTK_IMAGE(im), pb2);

    g_object_unref(pb1);
    g_object_unref(pb2);
}

static void exit_set_theme(PanelControl * pc, const char *theme)
{
    exit_set_size(pc, settings.size);
}

GtkWidget *lock_entry;
GtkWidget *exit_entry;

static void exit_apply_configuration(PanelControl * pc)
{
    const char *tmp;

    tmp = gtk_entry_get_text(GTK_ENTRY(lock_entry));

    g_free(settings.lock_command);

    if(tmp && *tmp)
        settings.lock_command = g_strdup(tmp);
    else
        settings.lock_command = NULL;

    tmp = gtk_entry_get_text(GTK_ENTRY(exit_entry));

    g_free(settings.exit_command);

    if(tmp && *tmp)
        settings.exit_command = g_strdup(tmp);
    else
        settings.exit_command = NULL;
}

void exit_add_options(PanelControl * pc, GtkContainer * container,
                      GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label;
    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Lock command:"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    lock_entry = gtk_entry_new();
    if(settings.lock_command)
        gtk_entry_set_text(GTK_ENTRY(lock_entry), settings.lock_command);
    gtk_widget_show(lock_entry);
    gtk_box_pack_start(GTK_BOX(hbox), lock_entry, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new(_("Exit command"));
    gtk_widget_show(label);
    gtk_size_group_add_widget(sg, label);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    exit_entry = gtk_entry_new();
    if(settings.exit_command)
        gtk_entry_set_text(GTK_ENTRY(exit_entry), settings.exit_command);
    gtk_widget_show(exit_entry);
    gtk_box_pack_start(GTK_BOX(hbox), exit_entry, FALSE, FALSE, 0);

    gtk_container_add(container, vbox);

    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(exit_apply_configuration), pc);
}

void create_exit(PanelControl * pc)
{
    t_exit *exit = exit_new();

    gtk_container_add(GTK_CONTAINER(pc->base), exit->vbox);

    pc->caption = g_strdup(_("Exit/Lock"));
    pc->data = (gpointer) exit;
    pc->main = exit->exit_button;

    pc->interval = 0;
    pc->update = NULL;

    pc->free = (gpointer) exit_free;

    pc->set_size = (gpointer) exit_set_size;
    pc->set_theme = (gpointer) exit_set_theme;

    pc->add_options = (gpointer) exit_add_options;

    exit_set_size(pc, settings.size);
}

/*  Config module
 *  -------------
*/
static IconButton *config_new(void)
{
    IconButton *button;
    GtkWidget *b;
    GdkPixbuf *pb = NULL;

    pb = get_system_pixbuf(MINIPALET_ICON);
    button = icon_button_new(pb);
    g_object_unref(pb);

    b = icon_button_get_button(button);

    add_tooltip(b, _("Setup..."));

    g_signal_connect_swapped(b, "clicked", G_CALLBACK(mini_palet_cb), NULL);

    return button;
}

static void config_free(PanelControl * pc)
{
    IconButton *b = (IconButton *) pc->data;

    icon_button_free(b);
}

static void config_set_size(PanelControl * pc, int size)
{
    IconButton *b = (IconButton *) pc->data;

    icon_button_set_size(b, size);
}

static void config_set_theme(PanelControl * pc, const char *theme)
{
    GdkPixbuf *pb;
    IconButton *b = (IconButton *) pc->data;

    pb = get_system_pixbuf(MINIPALET_ICON);
    icon_button_set_pixbuf(b, pb);
    g_object_unref(pb);
}

void create_config(PanelControl * pc)
{
    IconButton *config = config_new();
    GtkWidget *b = icon_button_get_button(config);

    gtk_container_add(GTK_CONTAINER(pc->base), b);
    pc->caption = g_strdup(_("Setup"));
    pc->data = (gpointer) config;
    pc->main = b;

    pc->interval = 0;
    pc->update = NULL;

    pc->free = (gpointer) config_free;

    pc->set_size = (gpointer) config_set_size;
    pc->set_theme = (gpointer) config_set_theme;
}
