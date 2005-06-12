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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

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

#include <libxfce4util/libxfce4util.h>

#include <panel/global.h>
#include <panel/controls.h>
#include <panel/icons.h>
#include <panel/plugins.h>

#define BORDER  8


typedef struct
{
    GtkWidget *eventbox;
    
    GtkWidget *table;

    GtkWidget *hrlabel;
    GtkWidget *minlabel;
    GtkWidget *ampmlabel;

    GtkTooltips *tips;

    int size;

    int timeout_id;
    
    int mday;
    int min;
    int hr;
}
AnotherClock;

/* start xfcalendar or send a client message
 * NOTE: keep message format in sync with xfcalendar */

static gboolean
popup_xfcalendar (GtkWidget * widget)
{
    GdkAtom atom;
    Window xwindow;

    /* send message to xfcalendar if it is running */
    atom = gdk_atom_intern ("_XFCE_CALENDAR_RUNNING", FALSE);
    if ((xwindow = XGetSelectionOwner (GDK_DISPLAY (),
				       gdk_x11_atom_to_xatom (atom))) != None)
    {
	char msg[20];
	const char *fmt = "%lx:%s";
	Window xid = GDK_WINDOW_XID (widget->window);
	GdkEventClient gev;

	/* popup in the same direction as menus */
	switch (groups_get_arrow_direction ())
	{
	    case GTK_ARROW_UP:
		sprintf (msg, fmt, xid, "up");
		break;
	    case GTK_ARROW_DOWN:
		sprintf (msg, fmt, xid, "down");
		break;
	    case GTK_ARROW_LEFT:
		sprintf (msg, fmt, xid, "left");
		break;
	    case GTK_ARROW_RIGHT:
		sprintf (msg, fmt, xid, "right");
		break;
	    default:
		return FALSE;
	}

	gev.type = GDK_CLIENT_EVENT;
	gev.window = widget->window;
	gev.send_event = TRUE;
	gev.message_type =
	    gdk_atom_intern ("_XFCE_CALENDAR_TOGGLE_HERE", FALSE);
	gev.data_format = 8;
	strcpy (gev.data.b, msg);

	gdk_event_send_client_message ((GdkEvent *) & gev,
				       (GdkNativeWindow) xwindow);
	gdk_flush ();

	return TRUE;
    }
    else 
    {
	exec_cmd_silent ("xfcalendar", FALSE, FALSE);
        exec_cmd_silent ("xfcalendar", FALSE, FALSE); /* actually only RAISE */
    }

    return FALSE;
}

static gboolean
on_button_press_event_cb (GtkWidget * widget,
			  GdkEventButton * event, Control * control)
{
    if (event->button == 1)
    {
	return popup_xfcalendar (control->base);
    }

    return FALSE;
}

/* creation and destruction */
static gboolean
anotherclock_timeout (AnotherClock *ac)
{
    time_t ticks;
    struct tm *tm;
    char date_s[255];

    ticks = time (0);
    tm = localtime (&ticks);

    if (tm->tm_mday != ac->mday)
    {
        ac->mday = tm->tm_mday;
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
            gtk_tooltips_set_tip (ac->tips, ac->eventbox, utf8date, NULL);
            g_free (utf8date);
        }
        else
        {
            gtk_tooltips_set_tip (ac->tips, ac->eventbox, date_s, NULL);
        }
    }

    if (tm->tm_min != ac->min)
    {
        char *markup = NULL, *size = NULL;
        
        ac->min = tm->tm_min;
        
        switch (ac->size)
        {
            case 0:
                size = "x-small";
                break;
            case 1:
                size = "small";
                break;
            case 2:
                size = "small";
                break;
            default:
                size = "medium";
        }
        
        markup = g_strdup_printf ("<span size=\"%s\">%02d</span>", 
                                  size, ac->min);
        
        gtk_label_set_markup (GTK_LABEL (ac->minlabel), markup);

        g_free (markup);
    }
    
    if (tm->tm_hour != ac->hr)
    {
        char *markup = NULL, *size = NULL;
        
        ac->hr = tm->tm_hour;
        
        switch (ac->size)
        {
            case 0:
                size = "large";
                break;
            case 1:
                size = "x-large";
                break;
            case 2:
                size = "x-large";
                break;
            default:
                size = "xx-large";
        }
        
        markup = g_strdup_printf ("<span size=\"%s\">%d</span>", 
                                  size, ac->hr);
        
        gtk_label_set_markup (GTK_LABEL (ac->hrlabel), markup);

        g_free (markup);
    }
    
    return TRUE;
}

static AnotherClock *
anotherclock_new (void)
{
    AnotherClock *ac = g_new (AnotherClock, 1);
    GtkWidget *align;
    
    ac->mday = ac->min = ac->hr = -1;

    ac->size = 1;

    ac->tips = gtk_tooltips_new ();
    g_object_ref (ac->tips);
    gtk_object_sink (GTK_OBJECT (ac->tips));

    ac->eventbox = gtk_event_box_new ();
    gtk_widget_set_name (ac->eventbox, "xfce_panel");
    gtk_widget_show (ac->eventbox);

    align = gtk_alignment_new (0.5, 0.5, 0, 0);
    gtk_widget_show (align);
    gtk_container_add (GTK_CONTAINER (ac->eventbox), align);

    ac->table = gtk_table_new (2, 2, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (ac->table), 2);
    gtk_widget_show (ac->table);
    gtk_container_add (GTK_CONTAINER (align), ac->table);

    ac->hrlabel = gtk_label_new (NULL);
    gtk_widget_show (ac->hrlabel);
    gtk_label_set_use_markup (GTK_LABEL (ac->hrlabel), TRUE);
    gtk_table_attach (GTK_TABLE (ac->table), ac->hrlabel,
                      0, 1, 0, 2, GTK_FILL, GTK_FILL, 0, 0);
    
    ac->minlabel = gtk_label_new (NULL);
    gtk_widget_show (ac->minlabel);
    gtk_label_set_use_markup (GTK_LABEL (ac->minlabel), TRUE);
    gtk_table_attach (GTK_TABLE (ac->table), ac->minlabel,
                      1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
    
    /* Add tooltip to show the current date */
    anotherclock_timeout (ac);

    ac->timeout_id =
	g_timeout_add (1000, (GSourceFunc) anotherclock_timeout, ac);

    /* callback for calendar popup */
    g_signal_connect (G_OBJECT (ac->eventbox), "button-press-event",
		      G_CALLBACK (on_button_press_event_cb), ac);

    return ac;
}

static void
anotherclock_free (Control * control)
{
    AnotherClock *ac = control->data;

    g_return_if_fail (ac != NULL);

    if (ac->timeout_id)
    {
	g_source_remove (ac->timeout_id);
        ac->timeout_id = 0;
    }

    g_object_unref (ac->tips);

    g_free (ac);
}

static void
anotherclock_attach_callback (Control * control, const char *signal,
		       GCallback callback, gpointer data)
{
    AnotherClock *ac = control->data;

    g_signal_connect (ac->eventbox, signal, callback, data);
}

/* panel preferences */
static void
anotherclock_set_size (Control * control, int size)
{
    AnotherClock *ac = control->data;

    ac->size = size;

    ac->min = ac->hr = -1;

    anotherclock_timeout (ac);
}

/* Write the configuration at exit */
static void
anotherclock_write_config (Control * control, xmlNodePtr parent)
{
}

/* Read the configuration file at init */
static void
anotherclock_read_config (Control * control, xmlNodePtr node)
{
}


/*  Clock panel control
 *  -------------------
*/
static gboolean
create_anotherclock_control (Control * control)
{
    AnotherClock *ac = anotherclock_new ();

    gtk_container_add (GTK_CONTAINER (control->base), ac->eventbox);

    control->data = (gpointer) ac;
    control->with_popup = FALSE;

    gtk_widget_set_size_request (control->base, -1, -1);

    return TRUE;
}

/* calculate position of calendar popup */

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "anotherclock";
    cc->caption = _("Another Clock");

    cc->create_control = (CreateControlFunc) create_anotherclock_control;

    cc->free = anotherclock_free;
    cc->read_config = anotherclock_read_config;
    cc->write_config = anotherclock_write_config;
    cc->attach_callback = anotherclock_attach_callback;

    cc->set_size = anotherclock_set_size;

    control_class_set_unique (cc, TRUE);
}

/* macro defined in plugins.h */
XFCE_PLUGIN_CHECK_INIT
