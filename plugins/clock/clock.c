/*  clock.c
 *
 *  Copyright (C) 2002 Jasper Huijsmans(huysmans@users.sourceforge.net)
 *                     Xavier Maillard (zedek@fxgsproject.org)
 *                     Olivier Fourdan (fourdan@xfce.org)
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

#include <libxfcegui4/xfce_clock.h>

#include "global.h"
#include "debug.h"

#include "controls.h"
#include "icons.h"

#include "plugins.h"

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

/* panel control configuration
   Global widget used in all the module configuration
   to revert the settings
*/
static GtkWidget *revert_button;

static void set_tip(GtkWidget *widget, const char *tip)
{
    static GtkTooltips *tooltips = NULL;

    if (!tooltips)
    {
	tooltips = gtk_tooltips_new();
    }

    gtk_tooltips_set_tip(tooltips, widget, tip, NULL);
}


/*  Clock module
 *  ------------
*/

/* Our real clock */
typedef struct
{
    GtkWidget *frame;
    GtkWidget *eventbox;
    GtkWidget *clock;           /* our XfceClock widget */

    int timeout_id; /* update the date tooltip */
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

static void clock_free(Control * control)
{
    t_clock *clock = (t_clock *) control->data;
    g_return_if_fail( clock != NULL );
    
    if(GTK_IS_WIDGET(clock->clock))
        gtk_widget_destroy(clock->clock);
    if(GTK_IS_WIDGET(clock->eventbox))
        gtk_widget_destroy(clock->eventbox);
    if(GTK_IS_WIDGET(clock->frame))
        gtk_widget_destroy(clock->frame);

    if (clock->timeout_id)
	g_source_remove(clock->timeout_id);
    
    g_free(clock);
}

static void clock_attach_callback(Control * control, const char *signal,
				  GCallback callback, gpointer data)
{
    t_clock *clock = (t_clock *) control->data;

    g_signal_connect(clock->eventbox, signal, callback, data);
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

void clock_set_size(Control * control, int size)
{
    int s = icon_size[size];
    t_clock *clock = (t_clock *) control->data;
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

void clock_set_style(Control * control, int style)
{
    t_clock *clock = (t_clock *) control->data;

    if(style == OLD_STYLE)
    {
        gtk_widget_set_name(clock->frame, "gxfce_color2");
	gtk_widget_set_name(clock->clock, "gxfce_color2");
        gtk_widget_set_name(clock->eventbox, "gxfce_color2");
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
static void update_size(GtkToggleButton * tb, Control* control)
{
    clock_set_size(control,settings.size);
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


static GtkWidget *create_clock_type_option_menu(t_clock * clock, Control* control)
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
    g_signal_connect(om, "changed", G_CALLBACK(update_size), control);
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

static GtkWidget *create_clock_24hrs_button(t_clock * clock, Control* control)
{
    GtkWidget *cb;
    cb = gtk_check_button_new();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                                 XFCE_CLOCK(clock->clock)->military_time);


    xfce_clock_show_military(XFCE_CLOCK(clock->clock),
			     XFCE_CLOCK(clock->clock)->military_time);

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_hour_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), control);

    return cb;
}

static void clock_secs_mode_changed(GtkToggleButton * tb, t_clock * clock)
{
    XFCE_CLOCK(clock->clock)->display_secs = gtk_toggle_button_get_active(tb);

    xfce_clock_show_secs(XFCE_CLOCK(clock->clock),
			 XFCE_CLOCK(clock->clock)->display_secs);

     /* Make the revert_button sensitive to get our initial value back
      */
     gtk_widget_set_sensitive( revert_button, TRUE );
     gtk_widget_set_size_request(clock->frame, -1, -1);
     gtk_widget_queue_resize (GTK_WIDGET(clock->clock));
}

static GtkWidget *create_clock_secs_button(t_clock * clock, Control* control)
{
    GtkWidget *cb;

    cb = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
                                 XFCE_CLOCK(clock->clock)->display_secs);


    xfce_clock_show_secs(XFCE_CLOCK(clock->clock),
			 XFCE_CLOCK(clock->clock)->display_secs);

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_secs_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), control);

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

static GtkWidget *create_clock_ampm_button(t_clock * clock, Control* control)
{
    GtkWidget *cb;
    cb = gtk_check_button_new();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb),
				 XFCE_CLOCK(clock->clock)->display_am_pm &&
				 ! XFCE_CLOCK(clock->clock)->military_time);

    gtk_widget_set_sensitive(cb, ! XFCE_CLOCK(clock->clock)->military_time);
    xfce_clock_show_ampm( XFCE_CLOCK(clock->clock), XFCE_CLOCK(clock->clock)->display_am_pm );

    g_signal_connect(cb, "toggled", G_CALLBACK(clock_ampm_mode_changed), clock);
    g_signal_connect(cb, "toggled", G_CALLBACK(update_size), control);
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
void clock_write_config(Control *control, xmlNodePtr parent)
{
    xmlNodePtr root;
    char value[MAXSTRLEN+1];

    t_clock *cl = (t_clock *) control->data;
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
void clock_read_config(Control *control, xmlNodePtr node)
{
    xmlChar *value;

    t_clock *cl = (t_clock *)control->data;

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
    if ((value = xmlGetProp(node, (const xmlChar *)"Clock_type")))
    {
	XFCE_CLOCK(cl->clock)->mode = atoi(value);
        g_free(value);
    }
    gtk_frame_set_shadow_type(GTK_FRAME(cl->frame), GTK_SHADOW_NONE);
#endif

    if ((value = xmlGetProp(node, (const xmlChar *)"Toggle_military")))
    {
	XFCE_CLOCK(cl->clock)->military_time = atoi(value);
	g_free(value);
    }
    if ((value = xmlGetProp(node, (const xmlChar *)"Toggle_am_pm")))
    {
	XFCE_CLOCK(cl->clock)->display_am_pm = atoi(value);
        g_free(value);
    }
    if ((value = xmlGetProp(node, (const xmlChar *)"Toggle_secs")))
    {
	XFCE_CLOCK(cl->clock)->display_secs = atoi(value);
        g_free(value);
    }

    /* Try to resize the clock to fit the user settings */
    clock_set_size(control, settings.size);
}

static void clock_apply_configuration(Control * control)
{
    t_clock *cl = (t_clock*) control->data;

    XFCE_CLOCK(cl->clock)->display_am_pm = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ampmbutton));
    XFCE_CLOCK(cl->clock)->display_secs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(secsbutton));
    XFCE_CLOCK(cl->clock)->military_time = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton));
    XFCE_CLOCK(cl->clock)->mode = gtk_option_menu_get_history(GTK_OPTION_MENU(om));

    /* Clean the backup whenever we confirm our changes */
    /* Actually that does nothing but it's to be consistent with the
       API */
    clock_clean_backup();
}

void clock_add_options(Control * control, GtkContainer * container,
                       GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox, *om, *hbox, *label;

    GtkSizeGroup *sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    t_clock *clock = (t_clock *) control->data;

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

    om = create_clock_type_option_menu(clock,control);
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

    checkbutton = create_clock_24hrs_button(clock,control);
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

    ampmbutton = create_clock_ampm_button(clock,control);
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

    secsbutton = create_clock_secs_button(clock,control);
    gtk_widget_show(secsbutton);
    gtk_box_pack_start(GTK_BOX(hbox), secsbutton, FALSE, FALSE, 0);

    /* signals */
    g_signal_connect_swapped(revert, "clicked",
                             G_CALLBACK(clock_revert), clock);
    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(clock_clean_backup), NULL);
    g_signal_connect_swapped(done, "clicked",
                             G_CALLBACK(clock_apply_configuration), control);

    gtk_container_add(container, vbox);

}

/*  create clock panel control
*/
gboolean create_clock_control(Control * control)
{
    t_clock *clock = clock_new();

    gtk_container_add(GTK_CONTAINER(control->base), clock->frame);

    control->data = (gpointer) clock;

    /* Add tooltip to show up the current date */
    clock_date_tooltip (clock->eventbox);

    clock->timeout_id = 
	g_timeout_add(60000, (GSourceFunc)clock_date_tooltip, clock->eventbox);
    
    gtk_widget_set_size_request(control->base, -1, -1);
    clock_set_size(control, settings.size); 

    return TRUE;
}

G_MODULE_EXPORT void xfce_control_class_init(ControlClass *cc)
{
    cc->name = "clock";
    cc->caption = _("Xfce clock");
    
    cc->create_control = (CreateControlFunc) create_clock_control;

    cc->free = clock_free;
    cc->read_config = clock_read_config;
    cc->write_config = clock_write_config;
    cc->attach_callback = clock_attach_callback;

    cc->add_options = clock_add_options;

    cc->set_size = clock_set_size;
    cc->set_style = clock_set_style;
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT

