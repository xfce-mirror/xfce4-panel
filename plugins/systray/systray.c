/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <time.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/xfce.h>
#include <panel/settings.h>
#include <panel/plugins.h>

#define BORDER 8

typedef struct
{
    int orientation;
    int size;
    
    GtkWidget *eventbox;
    GtkWidget *frame;
    GtkWidget *box;
    
    /* systray */
    GdkScreen *screen;
    XfceSystemTray *tray;
    gboolean tray_registered;

    GtkWidget *iconbox;

    /* time */
    GtkTooltips *timetips;
    GtkWidget *timebox;
    GtkWidget *timelabel;
    int time_id;
    
    gboolean show_time;
    gboolean before_icons;
    
    int mday;
    int min;
}
Systray;

/* border width */
static int systray_border [] = { 1, 2, 3, 4 };


/* prototypes */
static GtkWidget *systray_create_options_widget (Systray *systray);

static void systray_read_xml (Systray *systray, xmlNodePtr node);

static void systray_write_xml (Systray *systray, xmlNodePtr node);


/* tray */
static gboolean
register_tray (Systray * systray)
{
    GError *error = NULL;
    Screen *scr = GDK_SCREEN_XSCREEN (systray->screen);

    if (xfce_system_tray_check_running (scr))
    {
        xfce_info (_("There is already a system tray running on this "
                     "screen"));
        return FALSE;
    }
    else if (!xfce_system_tray_register (systray->tray, scr, &error))
    {
        xfce_err (_("Unable to register system tray: %s"), error->message);
        g_error_free (error);

        return FALSE;
    }

    return TRUE;
}

static void
icon_docked (XfceSystemTray * tray, GtkWidget * icon, Systray * systray)
{
    if (systray->tray_registered)
    {
        gtk_widget_hide (systray->iconbox);
        gtk_box_pack_start (GTK_BOX (systray->iconbox), icon, 
                            FALSE, FALSE, 0);
        gtk_widget_show (icon);
        gtk_widget_show (systray->iconbox);
    }
}

static void
icon_undocked (XfceSystemTray * tray, GtkWidget * icon, Systray *systray)
{
    if (systray->tray_registered)
    {
        gtk_widget_hide (systray->iconbox);
        gtk_container_remove (GTK_CONTAINER (systray->iconbox), icon);
        gtk_widget_show (systray->iconbox);
    }
}

static void
message_new (XfceSystemTray * tray, GtkWidget * icon, glong id,
             glong timeout, const gchar * text)
{
    g_print ("++ notification: %s\n", text);
}

static void
systray_start (Systray *systray)
{
    if (!systray->tray_registered)
    {
        systray->tray_registered = register_tray (systray);
    }
}

static void
systray_stop (Systray *systray)
{
    if (systray->tray_registered)
    {
        xfce_system_tray_unregister (systray->tray);
        systray->tray_registered = FALSE;
    }
}

/* time */
static gboolean
time_timeout (Systray *systray)
{
    time_t ticks;
    struct tm *tm;
    char date_s[255];

    ticks = time (0);
    tm = localtime (&ticks);

    if (tm->tm_mday != systray->mday)
    {
        systray->mday = tm->tm_mday;
        /* Use format characters from strftime(3)
         * to get the proper string for your locale.
         * I used these:
         * %A  : full weekday name
         * %d  : day of the month
         * %B  : full month name
         * %Y  : four digit year
         */
        strftime (date_s, 255, _("%A %d %B %Y"), tm);

        /* Conversion to utf8
         * patch by Oliver M. Bolzer <oliver@fakeroot.net>
         */
        if (!g_utf8_validate (date_s, -1, NULL))
        {
            char *utf8date;
            
            utf8date = g_locale_to_utf8 (date_s, -1, NULL, NULL, NULL);
            gtk_tooltips_set_tip (systray->timetips, systray->timebox, 
                                  utf8date, NULL);
            g_free (utf8date);
        }
        else
        {
            gtk_tooltips_set_tip (systray->timetips, systray->timebox, 
                                  date_s, NULL);
        }
    }

    if (tm->tm_min != systray->min)
    {
        systray->min = tm->tm_min;
        
        strftime (date_s, 255, _("%H:%M"), tm);

        if (!g_utf8_validate (date_s, -1, NULL))
        {
            char *utf8date;
            
            utf8date = g_locale_to_utf8 (date_s, -1, NULL, NULL, NULL);
            gtk_label_set_text (GTK_LABEL (systray->timelabel), utf8date);
            g_free (utf8date);
        }
        else
        {
            gtk_label_set_text (GTK_LABEL (systray->timelabel), date_s);
        }
    }
    
    return TRUE;
}

static void
systray_set_show_time (Systray *systray, gboolean show)
{
    if (show == systray->show_time)
        return;

    systray->show_time = show;
    
    if (show)
    {
        if (!systray->timetips)
            systray->timetips = gtk_tooltips_new ();
        
        systray->timebox = gtk_event_box_new ();
        gtk_widget_show (systray->timebox);
        gtk_box_pack_start (GTK_BOX (systray->box), systray->timebox,
                            FALSE, FALSE, 0);
        if (systray->before_icons)
            gtk_box_reorder_child (GTK_BOX (systray->box), 
                                   systray->timebox, 0);
        
        systray->timelabel = gtk_label_new (NULL);
        gtk_widget_show (systray->timelabel);
        gtk_container_add (GTK_CONTAINER (systray->timebox), 
                           systray->timelabel);

        systray->mday = -1;
        systray->min = -1;

        time_timeout (systray);
        systray->time_id = g_timeout_add (1000, (GSourceFunc) time_timeout,
                                          systray);
    }
    else
    {
        g_source_remove (systray->time_id);
        systray->time_id = 0;

        gtk_widget_destroy (systray->timebox);
        systray->timebox = NULL;
        systray->timelabel = NULL;
    }
}

static void
systray_set_before_icons (Systray *systray, gboolean before_icons)
{
    if (before_icons == systray->before_icons)
        return;

    systray->before_icons = before_icons;

    if (systray->show_time)
    {
        gtk_box_reorder_child (GTK_BOX (systray->box), systray->timebox,
                               before_icons ? 0 : 2);
    }
}

/* plugin */
static void
systray_set_size (Control *control, int size)
{
    Systray *systray = control->data;
    
    if (size != systray->size)
    {
        systray->size = size;

        gtk_container_set_border_width (GTK_CONTAINER (systray->frame),
                                        systray_border [size]);
        
        gtk_container_set_border_width (GTK_CONTAINER (systray->iconbox),
                                        systray_border [size]);
        
        if (systray->orientation == HORIZONTAL)
        {
            gtk_widget_set_size_request (systray->eventbox,
                                         -1, icon_size [size] + border_width);
        }
        else
        {
            gtk_widget_set_size_request (systray->eventbox,
                                         icon_size [size] + border_width, -1);
        }
    }
}

static void
systray_set_orientation (Control *control, int orientation)
{
    Systray *systray = control->data;

    if (orientation != systray->orientation)
    {
        int size = systray->size;
        
        systray->orientation = orientation;

        systray_stop (systray);

        if (systray->show_time)
        {
            g_object_ref (systray->timebox);
            gtk_container_remove (GTK_CONTAINER (systray->box), 
                                  systray->timebox);
        }
        
        gtk_widget_destroy (systray->box);

        if (orientation == HORIZONTAL)
        {
            systray->box = gtk_hbox_new (FALSE, 0);
            systray->iconbox = gtk_hbox_new (TRUE, 3);
        }
        else
        {
            systray->box = gtk_hbox_new (FALSE, 0);
            systray->iconbox = gtk_vbox_new (TRUE, 3);
        }

        gtk_widget_show (systray->box);
        gtk_widget_show (systray->iconbox);

        gtk_container_add (GTK_CONTAINER (systray->frame), systray->box);
        
        if (systray->show_time && systray->before_icons)
        {
            gtk_box_pack_start (GTK_BOX (systray->box), systray->timebox,
                                FALSE, FALSE, 0);
            g_object_unref (systray->timebox);
        }

        gtk_box_pack_start (GTK_BOX (systray->box), systray->iconbox,
                            TRUE, TRUE, 0);
        
        if (systray->show_time && !systray->before_icons)
        {
            gtk_box_pack_start (GTK_BOX (systray->box), systray->timebox,
                                FALSE, FALSE, 0);
            g_object_unref (systray->timebox);
        }

        /* force update */
        systray->size = -1;
        systray_set_size (control, size);

        systray_start (systray);
    }
}

static void
systray_create_options (Control *control, GtkContainer *container, 
                        GtkWidget *close)
{
    Systray *systray = control->data;

    gtk_container_add (container, systray_create_options_widget (systray));
}

static void
systray_read_config (Control *control, xmlNodePtr node)
{
    Systray *systray = control->data;

    systray_read_xml (systray, node);
}

static void
systray_write_config (Control *control, xmlNodePtr node)
{
    Systray *systray = control->data;

    systray_write_xml (systray, node);
}

static void
systray_attach_callback (Control * control, const char *signal,
			GCallback callback, gpointer data)
{
    Systray *systray = control->data;

    g_signal_connect (systray->eventbox, signal, callback, data);
    g_signal_connect (systray->frame, signal, callback, data);
}

static Systray *
systray_new (void)
{
    Systray *systray;

    systray = g_new0 (Systray, 1);

    /* initialize default settings */
    systray->orientation = HORIZONTAL;
    systray->size = SMALL;

    systray->tray = xfce_system_tray_new ();

    systray->eventbox = gtk_event_box_new ();
    gtk_widget_show (systray->eventbox);

    systray->screen = gtk_widget_get_screen (systray->eventbox);
    
    systray->frame = gtk_frame_new (NULL);
    gtk_widget_show (systray->frame);
    gtk_frame_set_shadow_type (GTK_FRAME (systray->frame), GTK_SHADOW_IN);
    gtk_container_set_border_width (GTK_CONTAINER (systray->frame), 
                                    systray_border [systray->size]);
    gtk_container_add (GTK_CONTAINER (systray->eventbox), systray->frame);
    
    /* systray */
    systray->box = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (systray->box);
    gtk_container_set_border_width (GTK_CONTAINER (systray->box), 
                                    systray_border [systray->size]);
    gtk_container_add (GTK_CONTAINER (systray->frame), systray->box);
    
    systray->iconbox = gtk_hbox_new (TRUE, 3);
    gtk_widget_show (systray->iconbox);
    gtk_box_pack_start (GTK_BOX (systray->box), systray->iconbox, 
                        TRUE, TRUE, 0);

    /* time: off by default */
    systray->show_time = FALSE;
    
    g_signal_connect (systray->tray, "icon_docked", G_CALLBACK (icon_docked),
                      systray);
    
    g_signal_connect (systray->tray, "icon_undocked",
                      G_CALLBACK (icon_undocked), systray);
    
    g_signal_connect (systray->tray, "message_new", G_CALLBACK (message_new),
                      systray);
    
    systray_start (systray);
    
    return systray;
}

static void
systray_free (Control * control)
{
    Systray *systray = (Systray *) control->data;

    /* free resources, stop timeouts, etc. */
    systray_stop (systray);

    systray_set_show_time (systray, FALSE);

    /* don't forget to free the memory for the Systray struct itself */
    g_free (systray);
}

static gboolean
create_systray_control (Control * control)
{
    Systray *systray = systray_new ();

    gtk_widget_set_size_request (control->base, -1, -1);
    gtk_container_add (GTK_CONTAINER (control->base), systray->eventbox);

    control->data = systray;

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "systray";
    cc->caption = _("Status Area");

    cc->create_control = (CreateControlFunc) create_systray_control;

    cc->attach_callback = systray_attach_callback;
    cc->free = systray_free;

    cc->read_config = systray_read_config;
    cc->write_config = systray_write_config;
    
    cc->set_size = systray_set_size;
    cc->set_orientation = systray_set_orientation;

    cc->create_options = systray_create_options;

    /* Additional API calls */
    control_class_set_unique (cc, TRUE);
}

/* Macro that checks panel API version */
XFCE_PLUGIN_CHECK_INIT

/* configuration */
static void
toggle_show_time (GtkToggleButton *b, Systray *systray)
{
    systray_set_show_time (systray, gtk_toggle_button_get_active (b));
}

static void
toggle_before_icons (GtkToggleButton *b, Systray *systray)
{
    systray_set_before_icons (systray, gtk_toggle_button_get_active (b));
}

static GtkWidget *
systray_create_options_widget (Systray *systray)
{
    GtkWidget *vbox, *button, *hbox, *align;
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);

    button = gtk_check_button_new_with_mnemonic (_("Show _time"));
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    
    if (systray->show_time)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

    g_signal_connect (button, "toggled", G_CALLBACK (toggle_show_time),
                      systray);
    
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    align = gtk_alignment_new (0,0,0,0);
    gtk_widget_set_size_request (align, 2 * BORDER, -1);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
    
    button = 
        gtk_check_button_new_with_mnemonic (_("Display time before icons"));
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    
    if (systray->before_icons)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

    g_signal_connect (button, "toggled", G_CALLBACK (toggle_before_icons), 
                      systray);
    
    return vbox;
}

static void 
systray_read_xml (Systray *systray, xmlNodePtr node)
{
    xmlChar *value;
    int n;
    
    if (!node)
        return;

    if ((value = xmlGetProp (node, "before")))
    {
        n = (int) strtol (value, NULL, 0);

        systray_set_before_icons (systray, (n == 1));
        
        g_free (value);
    }

    if ((value = xmlGetProp (node, "time")))
    {
        n = (int) strtol (value, NULL, 0);

        systray_set_show_time (systray, (n == 1));
        
        g_free (value);
    }
}

static void 
systray_write_xml (Systray *systray, xmlNodePtr node)
{
    char value[2];
    
    snprintf (value, 2, "%d", systray->show_time);
    xmlSetProp (node, "time", value);
    
    snprintf (value, 2, "%d", systray->before_icons);
    xmlSetProp (node, "before", value);
}

