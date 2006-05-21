/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 The Xfce Development Team
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

    /* Settings */
    int mode;
    gboolean military;
    gboolean ampm;
    gboolean secs;

    gboolean show_frame;
}
Clock;

static void clock_properties_dialog (XfcePanelPlugin *plugin, 
                                     Clock *clock);

static void clock_construct (XfcePanelPlugin *plugin);


/* -------------------------------------------------------------------- *
 *                               Clock                                  *
 * -------------------------------------------------------------------- */

static gboolean 
clock_date_tooltip (Clock *clock)
{
    time_t ticks;
    struct tm *tm;
    static gint mday = -1;
    char date_s[255];
    char *utf8date = NULL;

    ticks = time (0);
    tm = localtime (&ticks);

    if (mday != tm->tm_mday)
    {
        mday = tm->tm_mday;

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
        gtk_container_set_border_width (GTK_CONTAINER (clock->frame), 2);
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
    
    g_source_remove (clock->timeout_id);
    g_object_unref (clock->tips);
    g_free (clock);
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
    show_frame = TRUE;
    
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
            show_frame = xfce_rc_read_bool_entry (rc, "show_frame", TRUE);
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

/* Create widgets and connect to signals */

static void 
clock_construct (XfcePanelPlugin *plugin)
{
    Clock *clock = g_new0 (Clock, 1);

    clock->plugin = plugin;

    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (clock_set_size), clock);
    
    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (clock_free_data), clock);
    
    g_signal_connect (plugin, "save", 
                      G_CALLBACK (clock_write_rc_file), clock);
    
    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (clock_properties_dialog), clock);

    clock->frame = gtk_frame_new (NULL);
    gtk_widget_show (clock->frame);
    gtk_container_add (GTK_CONTAINER (plugin), clock->frame);

    clock->clock = xfce_clock_new ();
    gtk_widget_show (clock->clock);
    gtk_container_add (GTK_CONTAINER (clock->frame), clock->clock);

    clock_read_rc_file (plugin, clock);
    
    clock->tips = gtk_tooltips_new ();
    g_object_ref (clock->tips);
    gtk_object_sink (GTK_OBJECT (clock->tips));
        
    clock_date_tooltip (clock);

    clock->timeout_id = 
        g_timeout_add (60000, (GSourceFunc) clock_date_tooltip, clock);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
clock_show_frame_toggled (GtkToggleButton *cb, Clock *clock)
{
    clock->show_frame = gtk_toggle_button_get_active (cb);

    gtk_frame_set_shadow_type (GTK_FRAME (clock->frame), clock->show_frame ?
                               GTK_SHADOW_IN : GTK_SHADOW_NONE);
}

static void
clock_military_toggled (GtkToggleButton *cb, Clock *clock)
{
    clock->military = gtk_toggle_button_get_active (cb);
    xfce_clock_show_military (XFCE_CLOCK (clock->clock), clock->military);
    
    clock_update_size (clock, 
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (clock->plugin)));
}

static void
clock_ampm_toggled (GtkToggleButton *cb, Clock *clock)
{
    clock->ampm = gtk_toggle_button_get_active(cb);
    xfce_clock_show_ampm (XFCE_CLOCK (clock->clock), clock->ampm);
    
    clock_update_size (clock, 
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (clock->plugin)));
}

static void
clock_secs_toggled (GtkToggleButton *cb, Clock *clock)
{
    clock->secs = gtk_toggle_button_get_active(cb);
    xfce_clock_show_secs (XFCE_CLOCK (clock->clock), clock->secs);
    
    clock_update_size (clock, 
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (clock->plugin)));
}

static void
clock_mode_changed (GtkComboBox *cb, Clock *clock)
{
    clock->mode = gtk_combo_box_get_active(cb);
    xfce_clock_set_mode (XFCE_CLOCK (clock->clock), clock->mode);
    
    clock_update_size (clock, 
            xfce_panel_plugin_get_size (XFCE_PANEL_PLUGIN (clock->plugin)));
}

static void
clock_dialog_response (GtkWidget *dlg, int reponse, 
                       Clock *clock)
{
    g_object_set_data (G_OBJECT (clock->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (clock->plugin);
    clock_write_rc_file (clock->plugin, clock);
}

static void
clock_properties_dialog (XfcePanelPlugin *plugin, Clock *clock)
{
    GtkWidget *dlg, *frame, *bin, *vbox, *cb;

    xfce_panel_plugin_block_menu (plugin);
    
    dlg = xfce_titled_dialog_new_with_buttons (_("Clock"),
                GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");
    
    g_signal_connect (dlg, "response", G_CALLBACK (clock_dialog_response),
                      clock);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);
    
    frame = xfce_create_framebox (_("Appearance"), &bin);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
                        FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (bin), vbox);

    cb = gtk_combo_box_new_text ();
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);

    /* Keep order in sync with XfceClockMode */
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Analog"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("Digital"));
    gtk_combo_box_append_text (GTK_COMBO_BOX (cb), _("LED"));
    gtk_combo_box_set_active (GTK_COMBO_BOX (cb), clock->mode);
    g_signal_connect (cb, "changed", 
            G_CALLBACK (clock_mode_changed), clock);

    cb = gtk_check_button_new_with_mnemonic (_("Show _frame"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->show_frame);
    g_signal_connect (cb, "toggled", G_CALLBACK (clock_show_frame_toggled), 
                      clock);
    
    frame = xfce_create_framebox (_("Clock Options"), &bin);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame,
                        FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (bin), vbox);

    cb = gtk_check_button_new_with_mnemonic (_("Use 24-_hour clock"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->military);
    g_signal_connect (cb, "toggled", G_CALLBACK (clock_military_toggled), 
            clock);

    cb = gtk_check_button_new_with_mnemonic (_("Show AM/PM"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->ampm);
    g_signal_connect (cb, "toggled", G_CALLBACK (clock_ampm_toggled), 
            clock);

    cb = gtk_check_button_new_with_mnemonic (_("Display seconds"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
            clock->secs);
    g_signal_connect (cb, "toggled", G_CALLBACK (clock_secs_toggled), 
            clock);

    gtk_widget_show (dlg);
}

