/* vim: set expandtab ts=8 sw=4: */
/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
 *  Copyright © 2006 Jani Monoses <jani@ubuntu.com> 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

#define SHOW_DESKTOP_ICON_NAME  "gnome-fs-desktop"
#define TIP_ACTIVE              _("Restore hidden windows")
#define TIP_INACTIVE            _("Hide windows and show desktop")

typedef struct
{
    XfcePanelPlugin *plugin;

    GtkWidget *button;
    GtkWidget *image;

    GtkTooltips *tooltips;
    WnckScreen *screen;
    int wnck_id;
    int screen_id;
    int style_id;

    guint showing:1;
    guint updating:1;
}
ShowDesktopData;

/* Panel Plugin Interface */

static void showdesktop_construct (XfcePanelPlugin * plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (showdesktop_construct);


/* internal functions */

static gboolean
showdesktop_set_size (XfcePanelPlugin *plugin, int size, ShowDesktopData *sdd)
{
    GdkPixbuf *pb;
    int width = size - 2 - 2 * MAX (sdd->button->style->xthickness,
                                    sdd->button->style->ythickness);
    
    pb = xfce_themed_icon_load (SHOW_DESKTOP_ICON_NAME, width);
    if (pb) 
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (sdd->image), pb);
        g_object_unref (pb);
    }
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, size);

    return TRUE;
}

static void
showdesktop_free_data (XfcePanelPlugin * plugin, ShowDesktopData * sdd)
{
    if (sdd->wnck_id)
        g_signal_handler_disconnect (sdd->screen, sdd->wnck_id);
    
    if (sdd->screen_id)
        g_signal_handler_disconnect (plugin, sdd->screen_id);
    
    if (sdd->style_id)
        g_signal_handler_disconnect (plugin, sdd->style_id);
    
    sdd->wnck_id = sdd->screen_id = sdd->style_id = 0;
    gtk_object_sink (GTK_OBJECT (sdd->tooltips));
    panel_slice_free (ShowDesktopData, sdd);
}

static void
update_button_display (ShowDesktopData * sdd)
{
    gtk_tooltips_set_tip (sdd->tooltips, sdd->button,
                          sdd->showing ? TIP_ACTIVE : TIP_INACTIVE, NULL);
}

static void
update_button (ShowDesktopData * sdd)
{
    if (sdd->updating)
        return;

    sdd->updating = TRUE;
    
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sdd->button),
                                  sdd->showing);
    update_button_display (sdd);
    
    sdd->updating = FALSE;
}

/* create widgets and connect to signals */
static void
button_toggled (GtkWidget * button, ShowDesktopData * sdd)
{
    gboolean active;

    if (sdd->updating)
        return;

    active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

    wnck_screen_toggle_showing_desktop (sdd->screen, active);

    sdd->showing = active;
    update_button_display (sdd);
}

static void
showing_desktop_changed (WnckScreen * screen, ShowDesktopData * sdd)
{
    sdd->showing = wnck_screen_get_showing_desktop (screen);
    update_button (sdd);
}

static void
showdesktop_screen_changed (XfcePanelPlugin *plugin, GdkScreen *screen,
                            ShowDesktopData *sdd)
{
    if (sdd->wnck_id)
    {
        g_signal_handler_disconnect (sdd->screen, sdd->wnck_id);
        sdd->wnck_id = 0;
    }

    screen = gtk_widget_get_screen (GTK_WIDGET (plugin));

    if (screen)
    {
        sdd->screen = wnck_screen_get (gdk_screen_get_number (screen));
        
        sdd->wnck_id = 
                g_signal_connect (sdd->screen, "showing_desktop_changed",
                                  G_CALLBACK (showing_desktop_changed), sdd);

        sdd->showing = wnck_screen_get_showing_desktop (sdd->screen);
        update_button (sdd);
        showdesktop_set_size (plugin, xfce_panel_plugin_get_size (plugin), sdd);
    }
}

static void
showdesktop_style_set (XfcePanelPlugin *plugin, gpointer ignored,
                       ShowDesktopData *sdd)
{
    showdesktop_set_size (plugin, xfce_panel_plugin_get_size (plugin), sdd);
}

static void
showdesktop_construct (XfcePanelPlugin * plugin)
{
    ShowDesktopData *sdd = panel_slice_new0 (ShowDesktopData);

    sdd->plugin = plugin;

    sdd->tooltips = gtk_tooltips_new ();

    sdd->image = gtk_image_new ();

    sdd->button = xfce_create_panel_toggle_button ();
    gtk_container_add (GTK_CONTAINER (sdd->button), GTK_WIDGET (sdd->image));

    gtk_button_set_relief (GTK_BUTTON (sdd->button), GTK_RELIEF_NONE);
    gtk_widget_show_all (sdd->button);

    gtk_container_add (GTK_CONTAINER (plugin), sdd->button);
    xfce_panel_plugin_add_action_widget (plugin, sdd->button);
    
    g_signal_connect (sdd->button, "toggled", 
                      G_CALLBACK (button_toggled), sdd);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (showdesktop_free_data), sdd);

    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (showdesktop_set_size), sdd);

    update_button (sdd);

    showdesktop_screen_changed (plugin, gtk_widget_get_screen (sdd->button),
                                sdd);

    sdd->screen_id = 
        g_signal_connect (plugin, "screen-changed", 
                          G_CALLBACK (showdesktop_screen_changed), sdd);

    sdd->style_id = 
        g_signal_connect (plugin, "style-set",
                          G_CALLBACK (showdesktop_style_set), sdd);
}
