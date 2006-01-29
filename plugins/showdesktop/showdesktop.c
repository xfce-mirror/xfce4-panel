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
#include <libxfcegui4/libxfcegui4.h>

#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
    XfcePanelPlugin *plugin;

    NetkScreen *screen;
    GtkWidget *button;
    GdkPixbuf *icon;
    GtkWidget *image;

    GtkTooltips *tooltips;

    gboolean showing;
    gboolean updating;
}
ShowDesktopData;

/* Panel Plugin Interface */

static void showdesktop_construct (XfcePanelPlugin * plugin);

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (showdesktop_construct);


/* internal functions */

static void
showdesktop_free_data (XfcePanelPlugin * plugin, ShowDesktopData * sdd)
{
    gtk_object_sink (GTK_OBJECT (sdd->tooltips));
    g_free (sdd);
}

#define TIP_ACTIVE _("Restore hidden windows")
#define TIP_INACTIVE _("Hide windows and show desktop")

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

    netk_screen_toggle_showing_desktop (sdd->screen, active);

    sdd->showing = active;
    update_button_display (sdd);
}

static void
showing_desktop_changed (NetkScreen * screen, ShowDesktopData * sdd)
{
    sdd->showing = netk_screen_get_showing_desktop (screen);
    update_button (sdd);
}

#define SHOW_DESKTOP_ICON_NAME "gnome-fs-desktop"

static void
showdesktop_construct (XfcePanelPlugin * plugin)
{
    GtkIconTheme *icon_theme;
    ShowDesktopData *sdd = g_new0 (ShowDesktopData, 1);

    sdd->plugin = plugin;

    sdd->screen = netk_screen_get_default ();

    icon_theme = gtk_icon_theme_get_default ();
    sdd->icon =
        gtk_icon_theme_load_icon (icon_theme, SHOW_DESKTOP_ICON_NAME, 48, 0,
                                  NULL);
    sdd->image = xfce_scaled_image_new ();
    xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (sdd->image),
                                       sdd->icon);

    sdd->button = gtk_toggle_button_new ();
    gtk_container_add (GTK_CONTAINER (sdd->button), GTK_WIDGET (sdd->image));

    sdd->tooltips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (sdd->tooltips, sdd->button, TIP_ACTIVE, NULL);

    gtk_button_set_relief (GTK_BUTTON (sdd->button), GTK_RELIEF_NONE);
    gtk_widget_show_all (sdd->button);

    gtk_container_add (GTK_CONTAINER (plugin), sdd->button);

    xfce_panel_plugin_add_action_widget (plugin, sdd->button);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (showdesktop_free_data), sdd);

    update_button (sdd);

    if (!gdk_x11_screen_supports_net_wm_hint (
                gtk_widget_get_screen (sdd->button), 
                gdk_atom_intern ("_NET_SHOWING_DESKTOP", FALSE)))
    {
        g_critical ("showdesktop plugin: "
                "The window manager does not support _NET_SHOWING_DESKTOP");
    }
    else
    {
        g_signal_connect (sdd->button, "toggled", G_CALLBACK (button_toggled),
                          sdd);
        
        g_signal_connect (sdd->screen, "showing_desktop_changed",
                      G_CALLBACK (showing_desktop_changed), sdd);
    }
}

