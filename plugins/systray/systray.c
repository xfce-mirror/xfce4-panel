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
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <panel/xfce.h>
#include <panel/settings.h>
#include <panel/plugins.h>

#define BORDER 6

typedef struct
{
    XfceSystemTray *tray;
    gboolean tray_registered;

    int orientation;
    int size;
    
    GdkScreen *screen;
    
    GtkWidget *eventbox;
    GtkWidget *frame;
    GtkWidget *iconbox;
}
Systray;

/* border width */
static int systray_border [] = { 1, 2, 3, 4 };


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
        gtk_box_pack_start (GTK_BOX (systray->iconbox), icon, 
                            FALSE, FALSE, 0);
        gtk_widget_show (icon);
        gtk_widget_queue_resize (icon);
    }
}

static void
icon_undocked (XfceSystemTray * tray, GtkWidget * icon, GtkBox * iconbox)
{
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
            gtk_widget_set_size_request (systray->iconbox,
                                         -1, icon_size [size] + border_width);
        }
        else
        {
            gtk_widget_set_size_request (systray->iconbox,
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

        gtk_widget_destroy (systray->iconbox);

        if (orientation == HORIZONTAL)
            systray->iconbox = gtk_hbox_new (TRUE, 3);
        else
            systray->iconbox = gtk_vbox_new (TRUE, 3);

        gtk_widget_show (systray->iconbox);
        gtk_container_add (GTK_CONTAINER (systray->frame), systray->iconbox);

        /* force update */
        systray->size = -1;
        systray_set_size (control, size);

        systray_start (systray);
    }
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
    
    systray->iconbox = gtk_hbox_new (TRUE, 3);
    gtk_widget_show (systray->iconbox);
    gtk_container_set_border_width (GTK_CONTAINER (systray->iconbox), 
                                    systray_border [systray->size]);
    gtk_container_add (GTK_CONTAINER (systray->frame), systray->iconbox);

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
    cc->caption = _("Notification Area");

    cc->create_control = (CreateControlFunc) create_systray_control;

    cc->attach_callback = systray_attach_callback;
    cc->free = systray_free;

    cc->set_size = systray_set_size;
    cc->set_orientation = systray_set_orientation;

    /* Additional API calls */
    control_class_set_unique (cc, TRUE);
}

/* Macro that checks panel API version */
XFCE_PLUGIN_CHECK_INIT
