/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 - 2007 The Xfce Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Authors:
 *      Jasper Huijsmans <jasper@xfce.org>
 *      Jannis Pohlmann <info@sten-net.de>
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

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget *frame;
    GtkWidget *clock;
    GtkTooltips *tips;

    int timeout_id;
    int reschedule_id;
    int screen_changed_id;

    /* Settings */
    int mode;
    gboolean military;
    gboolean ampm;
    gboolean secs;

    gboolean show_frame;
    gint mday;
}
Clock;

typedef struct
{
    Clock *clock;

    GtkWidget *cb_mode;
    GtkWidget *cb_frame;
    GtkWidget *cb_military;
    GtkWidget *cb_ampm;
    GtkWidget *cb_secs;
}
ClockDialog;

static void clock_properties_dialog (XfcePanelPlugin *plugin,
                                     Clock *clock);

static void clock_construct (XfcePanelPlugin *plugin);


/* -------------------------------------------------------------------- *
 *                               Clock                                  *
 * -------------------------------------------------------------------- */
 
static gboolean
clock_reschedule_callback (Clock *clock)
{
    g_return_val_if_fail (clock->secs == FALSE, FALSE);
    
    /* update the clock every minute */
    xfce_clock_set_interval (XFCE_CLOCK (clock->clock), 60 * 1000);
    
    return FALSE;
}

static void
clock_reschedule_callback_destroy (Clock *clock)
{
    clock->reschedule_id = 0;
}

static void
clock_reschedule (Clock *clock)
{
    time_t ticks;
    struct tm *tm;
    gint interval;
    
    /* stop running reschudule */
    if (clock->reschedule_id)
        g_source_remove (clock->reschedule_id);
        
    /* update every second if seconds are displayed or the led clock is used */
    if (clock->secs || clock->mode == XFCE_CLOCK_LEDS)
    {
    	/* update every second */
    	xfce_clock_set_interval (XFCE_CLOCK (clock->clock), 1000);
    }
    else
    {
        /* get the time */
        time (&ticks);
        tm = localtime (&ticks);
    
        /* get the interval up to the next minute */
        interval = ((60 - tm->tm_sec) * 1000) + 500;
        
        /* set the new clock timeout (for this minute) */
        xfce_clock_set_interval (XFCE_CLOCK (clock->clock), interval);
        
        /* start a new reschedule event */
        clock->reschedule_id = 
            g_timeout_add_full (G_PRIORITY_LOW, interval, (GSourceFunc)clock_reschedule_callback, 
                                clock, (GDestroyNotify)clock_reschedule_callback_destroy);
    }
}

static gboolean
clock_date_tooltip (Clock *clock)
{
    time_t ticks;
    struct tm *tm;
    char date_s[255];
    char *utf8date = NULL;

    ticks = time (0);
    tm = localtime (&ticks);

    if (clock->mday != tm->tm_mday)
    {
        clock->mday = tm->tm_mday;

        /* Use format characters from strftime(3)
	 * to get the proper string for your locale.
	 * I used these:
	 * %A  : full weekday name
	 * %d  : day of the month
	 * %B  : full month name
	 * %Y  : four digit year
	 */
        strftime(date_s, 255, _("%A %d %B %Y"), tm);

        /* Conversion to utf8
         * Patch by Oliver M. Bolzer <oliver@fakeroot.net>
         */
        if (!g_utf8_validate(date_s, -1, NULL))
        {
            utf8date = g_locale_to_utf8(date_s, -1, NULL, NULL, NULL);
        }

        if (utf8date)
        {
            gtk_tooltips_set_tip (clock->tips, GTK_WIDGET (clock->plugin),
                                  utf8date, NULL);
            g_free (utf8date);
        }
        else
        {
            gtk_tooltips_set_tip (clock->tips, GTK_WIDGET (clock->plugin),
                                  date_s, NULL);
        }
    }

    return TRUE;
}

static void
clock_update_size (Clock *clock, int size)
{
    XfceClock *clk = XFCE_CLOCK (clock->clock);

    g_return_if_fail (clk != NULL);
    g_return_if_fail (GTK_IS_WIDGET (clk));

    /* keep in sync with systray */
    if (size > 26)
    {
        gtk_container_set_border_width (GTK_CONTAINER (clock->frame), 1);
        size -= 3;
    }
    else
    {
        gtk_container_set_border_width (GTK_CONTAINER (clock->frame), 0);
        size -= 1;
    }

    /* Replaced old 4-stage switch; Perhaps DIGIT_*_HEIGHT
     * should be moved from xfce_clock.c to xfce_clock.h so we
     * can use them here (e.g. 10*2 => DIGIT_SMALL_HEIGHT*2)
     * */

    if (xfce_panel_plugin_get_orientation (clock->plugin)
            == GTK_ORIENTATION_HORIZONTAL)
    {
        if (size <= 10*2)
        {
            xfce_clock_set_led_size (clk, DIGIT_SMALL);
        }
        else if (size <= 14*2)
        {
            xfce_clock_set_led_size (clk, DIGIT_MEDIUM);
        }
        else if (size <= 20*2)
        {
            xfce_clock_set_led_size (clk, DIGIT_LARGE);
        }
        else
        {
            xfce_clock_set_led_size (clk, DIGIT_HUGE);
        }
    }
    else
    {
        /* Always use small size in vertical mode */
        xfce_clock_set_led_size (clk, DIGIT_SMALL);
    }

    if ((xfce_clock_get_mode (clk) == XFCE_CLOCK_LEDS) ||
        (xfce_clock_get_mode (clk) == XFCE_CLOCK_DIGITAL))
    {
        gtk_widget_set_size_request (GTK_WIDGET (clk), -1, -1);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (clk), size, size);
    }

    gtk_widget_queue_resize (GTK_WIDGET (clk));
}


/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (clock_construct);


/* Interface Implementation */

static gboolean
clock_set_size (XfcePanelPlugin *plugin, int size, Clock *clock)
{
    clock_update_size(clock, size);

    return TRUE;
}

static void
clock_free_data (XfcePanelPlugin *plugin, Clock *clock)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);
        
    if (clock->reschedule_id)
        g_source_remove (clock->reschedule_id);
        
    g_signal_handler_disconnect (plugin, clock->screen_changed_id);

    g_source_remove (clock->timeout_id);
    g_object_unref (G_OBJECT (clock->tips));
    panel_slice_free (Clock, clock);
}

static void
clock_read_rc_file (XfcePanelPlugin *plugin, Clock* clock)
{
    char *file;
    XfceRc *rc;
    int mode;
    gboolean military, ampm, secs, show_frame;

    mode = XFCE_CLOCK_DIGITAL;
    military = TRUE;
    ampm = FALSE;
    secs = FALSE;
    show_frame = FALSE;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            mode = xfce_rc_read_int_entry (rc, "mode", XFCE_CLOCK_DIGITAL);
            military = xfce_rc_read_bool_entry (rc, "military", TRUE);
            ampm = xfce_rc_read_bool_entry (rc, "ampm", FALSE);
            secs = xfce_rc_read_bool_entry (rc, "secs", FALSE);
            show_frame = xfce_rc_read_bool_entry (rc, "show_frame", FALSE);
            xfce_rc_close (rc);
        }
    }

    clock->mode = mode;
    clock->military = military;
    clock->ampm = ampm;
    clock->secs = secs;

    xfce_clock_set_mode (XFCE_CLOCK (clock->clock), mode);
    xfce_clock_show_military (XFCE_CLOCK (clock->clock), military);
    xfce_clock_show_ampm (XFCE_CLOCK (clock->clock), ampm);
    xfce_clock_show_secs (XFCE_CLOCK (clock->clock), secs);

    clock->show_frame = show_frame;
    gtk_frame_set_shadow_type (GTK_FRAME (clock->frame),
                               show_frame ? GTK_SHADOW_IN : GTK_SHADOW_NONE);

    clock_update_size (clock,
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (plugin)));
}

static void
clock_write_rc_file (XfcePanelPlugin *plugin, Clock *clock)
{
    char *file;
    XfceRc *rc;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "mode", clock->mode);
    xfce_rc_write_bool_entry (rc, "military", clock->military);
    xfce_rc_write_bool_entry (rc, "ampm", clock->ampm);
    xfce_rc_write_bool_entry (rc, "secs", clock->secs);
    xfce_rc_write_bool_entry (rc, "show_frame", clock->show_frame);

    xfce_rc_close (rc);
}

static void
clock_screen_changed (GtkWidget *plugin, GdkScreen *screen,
                      Clock *clock)
{
	/* return when the widget is not visible (when moving the plugin) */
	if (GTK_IS_INVISIBLE (clock->clock) == FALSE)
	    return;
	
    gtk_widget_destroy (clock->clock);

    clock->clock = xfce_clock_new ();
    gtk_widget_show (clock->clock);
    gtk_container_add (GTK_CONTAINER (clock->frame), clock->clock);

    clock->mday = -1;
    
    clock_read_rc_file (clock->plugin, clock);
    
    /* set the clock timeout */
    clock_reschedule (clock);
}

/* Create widgets and connect to signals */

static void
clock_construct (XfcePanelPlugin *plugin)
{
    Clock *clock = panel_slice_new0 (Clock);

    clock->plugin = plugin;
    clock->reschedule_id = 0;

    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (clock_set_size), clock);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (clock_free_data), clock);

    g_signal_connect (plugin, "save",
                      G_CALLBACK (clock_write_rc_file), clock);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (clock_properties_dialog), clock);
                      
    clock->screen_changed_id =
        g_signal_connect (plugin, "screen-changed",
                          G_CALLBACK (clock_screen_changed), clock);

    clock->frame = gtk_frame_new (NULL);
    gtk_widget_show (clock->frame);
    gtk_container_add (GTK_CONTAINER (plugin), clock->frame);

    clock->clock = xfce_clock_new ();
    gtk_widget_show (clock->clock);
    gtk_container_add (GTK_CONTAINER (clock->frame), clock->clock);

    clock_read_rc_file (plugin, clock);
    clock->mday = -1;

    clock->tips = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (clock->tips));
    gtk_object_sink (GTK_OBJECT (clock->tips));

    clock_date_tooltip (clock);
    
    /* set the clock timeout */
    clock_reschedule (clock);

    /* update the tooltip every minute */
    clock->timeout_id =
        g_timeout_add (60 * 1000, (GSourceFunc) clock_date_tooltip, clock);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
clock_set_sensitive (ClockDialog *cd)
{
    if (cd->clock->mode == XFCE_CLOCK_ANALOG)
    {
	gtk_widget_set_sensitive (cd->cb_military, FALSE);
	gtk_widget_set_sensitive (cd->cb_ampm, FALSE);
    }
    else
    {
	gtk_widget_set_sensitive (cd->cb_military, TRUE);

	if (cd->clock->military)
	    gtk_widget_set_sensitive (cd->cb_ampm, FALSE);
        else
	    gtk_widget_set_sensitive (cd->cb_ampm, TRUE);
    }
}

static void
clock_button_toggled (GtkWidget *cb, ClockDialog *cd)
{
    gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cb));

    if (cb == cd->cb_frame)
    {
	cd->clock->show_frame = active;
	gtk_frame_set_shadow_type (GTK_FRAME (cd->clock->frame),
	                           active ? GTK_SHADOW_IN : GTK_SHADOW_NONE);
    }
    else if (cb == cd->cb_military)
    {
	cd->clock->military = active;
	xfce_clock_show_military (XFCE_CLOCK (cd->clock->clock),
	                          active);

	clock_set_sensitive (cd);
    }
    else if (cb == cd->cb_ampm)
    {
	cd->clock->ampm = active;
	xfce_clock_show_ampm (XFCE_CLOCK (cd->clock->clock),
	                      active);
    }
    else if (cb == cd->cb_secs)
    {
	cd->clock->secs = active;
	xfce_clock_show_secs (XFCE_CLOCK (cd->clock->clock),
	                      active);
	
	/* set the clock timeout */
        clock_reschedule (cd->clock);
    }

    clock_update_size (cd->clock,
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (cd->clock->plugin)));
}

static void
clock_mode_changed (GtkComboBox *cb, ClockDialog *cd)
{
    cd->clock->mode = gtk_combo_box_get_active(cb);
    xfce_clock_set_mode (XFCE_CLOCK (cd->clock->clock), cd->clock->mode);

    clock_set_sensitive (cd);

    clock_update_size (cd->clock,
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (cd->clock->plugin)));
}

static void
clock_dialog_response (GtkWidget *dlg, int reponse,
                       ClockDialog *cd)
{
    g_object_set_data (G_OBJECT (cd->clock->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (cd->clock->plugin);
    clock_write_rc_file (cd->clock->plugin, cd->clock);

    g_free (cd);
}

static void
clock_properties_dialog (XfcePanelPlugin *plugin, Clock *clock)
{
    GtkWidget *dlg, *frame, *bin, *vbox, *cb;
    ClockDialog *cd;

    xfce_panel_plugin_block_menu (plugin);

    cd = g_new0 (ClockDialog, 1);
    cd->clock = clock;

    dlg = xfce_titled_dialog_new_with_buttons (_("Clock"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    frame = xfce_create_framebox (_("Appearance"), &bin);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
                        FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (bin), vbox);

    cd->cb_mode = cb = gtk_combo_box_new_text ();
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);

    /* Keep order in sync with XfceClockMode */
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Analog"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Digital"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("LED"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (cb), clock->mode);
    g_signal_connect (cb, "changed",
            G_CALLBACK (clock_mode_changed), cd);

    cd->cb_frame = cb = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->show_frame);
    g_signal_connect (cb, "toggled",
            G_CALLBACK (clock_button_toggled), cd);

    frame = xfce_create_framebox (_("Clock Options"), &bin);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
                        FALSE, FALSE, 0);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_add (GTK_CONTAINER (bin), vbox);

    cd->cb_military = cb = gtk_check_button_new_with_mnemonic (_("Use 24-_hour clock"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->military);
    g_signal_connect (cb, "toggled",
            G_CALLBACK (clock_button_toggled), cd);

    cd->cb_ampm = cb = gtk_check_button_new_with_mnemonic (_("Show AM/PM"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->ampm);
    g_signal_connect (cb, "toggled",
            G_CALLBACK (clock_button_toggled), cd);

    cd->cb_secs = cb = gtk_check_button_new_with_mnemonic (_("Display seconds"));
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->secs);
    g_signal_connect (cb, "toggled",
            G_CALLBACK (clock_button_toggled), cd);

    clock_set_sensitive (cd);

    g_signal_connect (dlg, "response",
            G_CALLBACK (clock_dialog_response), cd);

    gtk_widget_show_all (dlg);
}

