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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <libxfce4util/i18n.h>
#include <libxfcegui4/xfce_clock.h>

#include "global.h"
#include "debug.h"

#include "controls.h"
#include "icons.h"

#include "plugins.h"

#define BORDER 6

/* Clock tooltip - From xfce3 */
/* FIXME: Find another place for that */
static char *day_names[] = {
    N_("Sunday"),
    N_("Monday"),
    N_("Tuesday"),
    N_("Wednesday"),
    N_("Thursday"),
    N_("Friday"),
    N_("Saturday")
};

static char *month_names[] = {
    N_("January"),
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
*/
typedef struct
{
    GtkWidget *eventbox;
    GtkWidget *clock;           /* our XfceClock widget */

    int timeout_id;             /* update the date tooltip */
}
t_clock;

typedef struct
{
    /* the clock */
    t_clock *clock;

    /* backup */
    int mode;
    int military;
    int ampm;
    int secs;

    /* controls dialog */
    GtkWidget *dialog;
    GtkWidget *revert;

    /* clock options */
    GtkWidget *type_menu;

    GtkWidget *twentyfour_rb;
    GtkWidget *twelve_rb;
    GtkWidget *ampm_rb;

    GtkWidget *seconds_cb;
}
ClockDialog;

/* creation and destruction */
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

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

    ticks = time (0);
    tm = localtime (&ticks);
    if ((mday != tm->tm_mday) || (wday != tm->tm_wday) || (mon != tm->tm_mon) ||
        (year != tm->tm_year))
    {
        mday = tm->tm_mday;
        wday = tm->tm_wday;
        mon = tm->tm_mon;
        year = tm->tm_year;
        snprintf (date_s, 255, "%s, %u %s %u", _(day_names[wday]), mday,
                  _(month_names[mon]), year + 1900);
        add_tooltip (widget, _(date_s));
    }
    return TRUE;
}

static t_clock *
clock_new (void)
{
    t_clock *clock = g_new (t_clock, 1);

    clock->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (clock->eventbox, "xfce_clock");
    gtk_widget_show (clock->eventbox);

    clock->clock = xfce_clock_new ();
    gtk_widget_show (clock->clock);
    gtk_container_add (GTK_CONTAINER (clock->eventbox), clock->clock);

    /* Add tooltip to show the current date */
    clock_date_tooltip (clock->eventbox);

    clock->timeout_id =
        g_timeout_add (60000, (GSourceFunc) clock_date_tooltip,
                       clock->eventbox);

    return clock;
}

static void
clock_free (Control * control)
{
    t_clock *clock = control->data;

    g_return_if_fail (clock != NULL);

    if (clock->timeout_id)
        g_source_remove (clock->timeout_id);

    g_free (clock);
}

static void
clock_attach_callback (Control * control, const char *signal,
                       GCallback callback, gpointer data)
{
    t_clock *clock = control->data;

    g_signal_connect (clock->eventbox, signal, callback, data);
}

/* panel preferences */
void
update_clock_size (XfceClock * clock, int size)
{
    if ((xfce_clock_get_mode (clock) == XFCE_CLOCK_LEDS) ||
        (xfce_clock_get_mode (clock) == XFCE_CLOCK_DIGITAL))
    {
        gtk_widget_set_size_request (GTK_WIDGET (clock), -1, -1);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (clock), icon_size[size],
                                     icon_size[size]);
    }
    gtk_widget_queue_resize (GTK_WIDGET (clock));
}

void
clock_set_size (Control * control, int size)
{
    t_clock *clock = (t_clock *) control->data;
    XfceClock *tmp = XFCE_CLOCK (clock->clock);

    switch (size)
    {
        case 0:
            xfce_clock_set_led_size (tmp, DIGIT_SMALL);
            break;
        case 1:
            xfce_clock_set_led_size (tmp, DIGIT_MEDIUM);
            break;
        case 2:
            xfce_clock_set_led_size (tmp, DIGIT_LARGE);
            break;
        default:
            xfce_clock_set_led_size (tmp, DIGIT_HUGE);
    }
    update_clock_size (tmp, size);
}

/* Write the configuration at exit */
void
clock_write_config (Control * control, xmlNodePtr parent)
{
    xmlNodePtr root;
    char value[MAXSTRLEN + 1];

    t_clock *cl = (t_clock *) control->data;
    XfceClock *clock = XFCE_CLOCK (cl->clock);

    /* I use my own node even if we only have 4 settings
       It's safer and easier to check
     */
    root = xmlNewTextChild (parent, NULL, "XfceClock", NULL);
    g_snprintf (value, 2, "%d", clock->mode);
    xmlSetProp (root, "Clock_type", value);
    g_snprintf (value, 2, "%d", clock->military_time);
    xmlSetProp (root, "Toggle_military", value);
    g_snprintf (value, 2, "%d", clock->display_am_pm);
    xmlSetProp (root, "Toggle_am_pm", value);
    g_snprintf (value, 2, "%d", clock->display_secs);
    xmlSetProp (root, "Toggle_secs", value);
}

/* Read the configuration file at init */
void
clock_read_config (Control * control, xmlNodePtr node)
{
    xmlChar *value;

    t_clock *cl = (t_clock *) control->data;

    if (!node || !node->children)
        return;

    node = node->children;

    /* Leave if we can't find the node XfceClock */
    if (!xmlStrEqual (node->name, "XfceClock"))
        return;

    if ((value = xmlGetProp (node, (const xmlChar *)"Clock_type")))
    {
        XFCE_CLOCK (cl->clock)->mode = atoi (value);
        g_free (value);
    }

    if ((value = xmlGetProp (node, (const xmlChar *)"Toggle_military")))
    {
        XFCE_CLOCK (cl->clock)->military_time = atoi (value);
        g_free (value);
    }
    if ((value = xmlGetProp (node, (const xmlChar *)"Toggle_am_pm")))
    {
        XFCE_CLOCK (cl->clock)->display_am_pm = atoi (value);
        g_free (value);
    }
    if ((value = xmlGetProp (node, (const xmlChar *)"Toggle_secs")))
    {
        XFCE_CLOCK (cl->clock)->display_secs = atoi (value);
        g_free (value);
    }

    /* Try to resize the clock to fit the user settings */
    clock_set_size (control, settings.size);
}

/*  Clock options dialog
 *  --------------------
*/
static void
make_sensitive (GtkWidget * w)
{
    gtk_widget_set_sensitive (w, TRUE);
}

static void
make_insensitive (GtkWidget * w)
{
    gtk_widget_set_sensitive (w, FALSE);
}

/* clock type */
static void
clock_type_changed (GtkOptionMenu * om, ClockDialog * cd)
{
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    xclock->mode = gtk_option_menu_get_history (om);

    xfce_clock_set_mode (xclock, xclock->mode);
    update_clock_size (xclock, settings.size);

    if (xclock->mode == XFCE_CLOCK_ANALOG)
    {
        make_insensitive (cd->twentyfour_rb);
        make_insensitive (cd->twelve_rb);
        make_insensitive (cd->ampm_rb);
    }
    else
    {
        make_sensitive (cd->twentyfour_rb);
        make_sensitive (cd->twelve_rb);
        make_sensitive (cd->ampm_rb);
    }

    gtk_widget_set_sensitive (cd->revert, TRUE);
}

static void
add_type_box (GtkWidget * vbox, GtkSizeGroup * sg, ClockDialog * cd)
{
    GtkWidget *hbox, *label, *menu, *mi, *om;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    label = gtk_label_new (_("Clock type:"));
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    om = cd->type_menu = gtk_option_menu_new ();
    gtk_widget_show (om);
    gtk_box_pack_start (GTK_BOX (hbox), om, FALSE, FALSE, 0);

    menu = gtk_menu_new ();
    gtk_option_menu_set_menu (GTK_OPTION_MENU (om), menu);

    mi = gtk_menu_item_new_with_label (_("Analog"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_menu_item_new_with_label (_("Digital"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    mi = gtk_menu_item_new_with_label (_("LED"));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

    gtk_option_menu_set_history (GTK_OPTION_MENU (om),
                                 XFCE_CLOCK (cd->clock->clock)->mode);

    g_signal_connect (om, "changed", G_CALLBACK (clock_type_changed), cd);
}

/* hour mode */
static void
set_24hr_mode (GtkToggleButton * tb, ClockDialog * cd)
{
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    if (!gtk_toggle_button_get_active (tb))
        return;

    xfce_clock_show_military (xclock, 1);

    update_clock_size (xclock, settings.size);
    gtk_widget_set_sensitive (cd->revert, TRUE);
}

static void
set_12hr_mode (GtkToggleButton * tb, ClockDialog * cd)
{
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    if (!gtk_toggle_button_get_active (tb))
        return;

    xfce_clock_show_military (xclock, 0);
    xfce_clock_show_ampm (xclock, 0);

    update_clock_size (xclock, settings.size);
    gtk_widget_set_sensitive (cd->revert, TRUE);
}

static void
set_ampm_mode (GtkToggleButton * tb, ClockDialog * cd)
{
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    if (!gtk_toggle_button_get_active (tb))
        return;

    xfce_clock_show_military (xclock, 0);
    xfce_clock_show_ampm (xclock, 1);

    update_clock_size (xclock, settings.size);
    gtk_widget_set_sensitive (cd->revert, TRUE);
}

static void
add_hour_mode_box (GtkWidget * vbox, GtkSizeGroup * sg, ClockDialog * cd)
{
    GtkWidget *hbox, *label, *rb1, *rb2, *rb3;
    XfceClock *xclock;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Hour mode:"));
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    rb1 = cd->twentyfour_rb =
        gtk_radio_button_new_with_label (NULL, _("24 hour"));
    gtk_widget_show (rb1);
    gtk_box_pack_start (GTK_BOX (hbox), rb1, FALSE, FALSE, 0);

    rb2 = cd->twelve_rb =
        gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (rb1),
                                                     _("12 hour"));
    gtk_widget_show (rb2);
    gtk_box_pack_start (GTK_BOX (hbox), rb2, FALSE, FALSE, 0);

    rb3 = cd->ampm_rb =
        gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (rb1),
                                                     _("AM/PM"));
    gtk_widget_show (rb3);
    gtk_box_pack_start (GTK_BOX (hbox), rb3, FALSE, FALSE, 0);

    xclock = XFCE_CLOCK (cd->clock->clock);

    if (xclock->military_time)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb1), TRUE);
    else if (xclock->display_am_pm)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb3), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rb2), TRUE);

    g_signal_connect (rb1, "toggled", G_CALLBACK (set_24hr_mode), cd);
    g_signal_connect (rb2, "toggled", G_CALLBACK (set_12hr_mode), cd);
    g_signal_connect (rb3, "toggled", G_CALLBACK (set_ampm_mode), cd);
}

/* show seconds */
static void
clock_seconds_changed (GtkToggleButton * tb, ClockDialog * cd)
{
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    xfce_clock_show_secs (xclock, gtk_toggle_button_get_active (tb));
    update_clock_size (xclock, settings.size);

    gtk_widget_set_sensitive (cd->revert, TRUE);
}

static void
add_seconds_box (GtkWidget * vbox, GtkSizeGroup * sg, ClockDialog * cd)
{
    GtkWidget *hbox, *label, *cb;
    XfceClock *xclock = XFCE_CLOCK (cd->clock->clock);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Show seconds:"));
    gtk_widget_show (label);
    gtk_size_group_add_widget (sg, label);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    cb = cd->seconds_cb = gtk_check_button_new ();
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), xclock->display_secs);
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (hbox), cb, FALSE, FALSE, 0);

    g_signal_connect (cb, "toggled", G_CALLBACK (clock_seconds_changed), cd);
}

/* backup */
static void
clock_create_backup (ClockDialog * cd)
{
    XfceClock *clock = XFCE_CLOCK (cd->clock->clock);

    g_return_if_fail (clock != NULL);

    cd->mode = clock->mode;
    cd->military = clock->military_time;
    cd->ampm = clock->display_am_pm;
    cd->secs = clock->display_secs;
}

static void
clock_restore_backup (ClockDialog * cd)
{
    GtkToggleButton *tb;

    /* clock type */
    gtk_option_menu_set_history (GTK_OPTION_MENU (cd->type_menu), cd->mode);

    /* hour mode */
    if (cd->military)
        tb = GTK_TOGGLE_BUTTON (cd->twentyfour_rb);
    else if (cd->ampm)
        tb = GTK_TOGGLE_BUTTON (cd->ampm_rb);
    else
        tb = GTK_TOGGLE_BUTTON (cd->twelve_rb);

    gtk_toggle_button_set_active (tb, TRUE);

    /* seconds */
    tb = GTK_TOGGLE_BUTTON (cd->seconds_cb);
    gtk_toggle_button_set_active (tb, cd->secs);
}

/* clock options box */
void
clock_add_options (Control * control, GtkContainer * container,
                   GtkWidget * revert, GtkWidget * done)
{
    GtkWidget *vbox;
    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    ClockDialog *cd;

    cd = g_new0 (ClockDialog, 1);

    cd->clock = control->data;
    cd->dialog = gtk_widget_get_toplevel (revert);
    cd->revert = revert;

    clock_create_backup (cd);

    g_signal_connect_swapped (revert, "clicked",
                              G_CALLBACK (clock_restore_backup), cd);

    g_signal_connect_swapped (cd->dialog, "destroy-event",
                              G_CALLBACK (g_free), cd);

    /* the options box */
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);

    add_type_box (vbox, sg, cd);

    add_hour_mode_box (vbox, sg, cd);

    add_seconds_box (vbox, sg, cd);

    gtk_container_add (container, vbox);
}

/*  Clock panel control
 *  -------------------
*/
gboolean
create_clock_control (Control * control)
{
    t_clock *clock = clock_new ();

    gtk_container_add (GTK_CONTAINER (control->base), clock->eventbox);

    control->data = (gpointer) clock;
    control->with_popup = FALSE;

    gtk_widget_set_size_request (control->base, -1, -1);
    clock_set_size (control, settings.size);

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    cc->name = "clock";
    cc->caption = _("Xfce Clock");

    cc->create_control = (CreateControlFunc) create_clock_control;

    cc->free = clock_free;
    cc->read_config = clock_read_config;
    cc->write_config = clock_write_config;
    cc->attach_callback = clock_attach_callback;

    cc->add_options = clock_add_options;

    cc->set_size = clock_set_size;
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT
