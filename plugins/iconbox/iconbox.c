/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <gtk/gtk.h>

#include <libwnck/libwnck.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libwnck/window-action-menu.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-hvbox.h>

#define N_ICONBOX_CONNECTIONS  4
#define N_ICON_CONNECTIONS     4
#define URGENT_TIMEOUT         500

typedef struct
{
    XfcePanelPlugin *plugin;

    WnckScreen *wnck_screen;
    int connections[N_ICONBOX_CONNECTIONS];
    int screen_changed_id;
    GtkWidget *box;
    GtkWidget *handle;
    GtkWidget *iconbox;

    GSList *iconlist;
    GtkTooltips *icon_tooltips;

    int icon_size;
    guint only_hidden:1;
    guint all_workspaces:1;
    guint expand:1;
}
Iconbox;

typedef struct
{
    Iconbox *ib;

    WnckWindow *window;
    int connections[N_ICON_CONNECTIONS];

    GdkPixbuf *pb;

    GtkWidget *button;
    GtkWidget *image;
    gboolean was_minimized;
    guint urgent_id;
}
Icon;


static void iconbox_properties_dialog (XfcePanelPlugin *plugin,
                                       Iconbox *iconbox);

static void iconbox_construct (XfcePanelPlugin *plugin);


/* -------------------------------------------------------------------- *
 *                            Iconbox                                   *
 * -------------------------------------------------------------------- */

/* icons */
static void
update_visibility (Icon *icon, WnckWorkspace *optional_active_ws)
{
    WnckWorkspace *ws;

    gdk_flush ();

    if (wnck_window_is_skip_tasklist (icon->window))
    {
        gtk_widget_hide (icon->button);
        return;
    }

    if (icon->ib->only_hidden && !wnck_window_is_minimized (icon->window))
    {
        gtk_widget_hide (icon->button);
        return;
    }

    if (optional_active_ws)
    {
        ws = optional_active_ws;
    }
    else
    {
        ws = wnck_screen_get_active_workspace
                (wnck_window_get_screen (icon->window));
    }

    if (icon->ib->all_workspaces
        || wnck_window_is_sticky (icon->window)
        || ws == wnck_window_get_workspace (icon->window)
        || wnck_window_or_transient_needs_attention (icon->window))
    {
        gtk_widget_show (icon->button);
    }
    else
    {
        gtk_widget_hide (icon->button);
    }
}

static void
icon_update_image (Icon *icon)
{
    GdkPixbuf *scaled;

    g_return_if_fail (GDK_IS_PIXBUF (icon->pb));

    if (wnck_window_is_minimized (icon->window))
    {
        if (!icon->was_minimized)
        {
            /* copied from wnck-tasklist.c: dimm_icon */
            int x, y, w, h, pixel_stride, row_stride;
            guchar *row, *pixels;

            if (gdk_pixbuf_get_has_alpha (icon->pb))
                scaled = gdk_pixbuf_copy (icon->pb);
            else
                scaled = gdk_pixbuf_add_alpha (icon->pb, FALSE, 0, 0, 0);

            w = gdk_pixbuf_get_width (scaled);
            h = gdk_pixbuf_get_height (scaled);

            pixel_stride = 4;

            row = gdk_pixbuf_get_pixels (scaled);
            row_stride = gdk_pixbuf_get_rowstride (scaled);

            for (y = 0; y < h; y++)
            {
                pixels = row;

                for (x = 0; x < w; x++)
                {
                    pixels[3] /= 2;

                    pixels += pixel_stride;
                }

                row += row_stride;
            }

            xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image),
                                               scaled);

            g_object_unref (G_OBJECT (scaled));

            icon->was_minimized = TRUE;
        }
    }
    else if (icon->was_minimized)
    {
        xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image),
                                           icon->pb);

        icon->was_minimized = FALSE;
    }

    update_visibility (icon, NULL);
}

/* blinking */
static void
update_blink (Icon *icon, gboolean blink)
{
    GtkStyle   *style;
    GtkRcStyle *mod;
    GdkColor    c;

    style = gtk_widget_get_style (icon->button);
    mod   = gtk_widget_get_modifier_style (icon->button);
    c     = style->bg[GTK_STATE_SELECTED];

    if (blink)
    {
	gtk_button_set_relief (GTK_BUTTON (icon->button), GTK_RELIEF_NORMAL);

	if(mod->color_flags[GTK_STATE_NORMAL] & GTK_RC_BG)
        {
            mod->color_flags[GTK_STATE_NORMAL] &= ~(GTK_RC_BG);
            gtk_widget_modify_style(icon->button, mod);
        }
        else
        {
            mod->color_flags[GTK_STATE_NORMAL] |= GTK_RC_BG;
            mod->bg[GTK_STATE_NORMAL] = c;
            gtk_widget_modify_style(icon->button, mod);
        }
    }
    else
    {
        gtk_button_set_relief (GTK_BUTTON (icon->button), GTK_RELIEF_NONE);
	mod->color_flags[GTK_STATE_NORMAL] &= ~(GTK_RC_BG);
	gtk_widget_modify_style(icon->button, mod);
    }
}

static gboolean
urgent_timeout (Icon *icon)
{
    update_visibility (icon, NULL);
    update_blink (icon, TRUE);
    return TRUE;;
}

static void
queue_urgent_timeout (Icon *icon)
{
    if (icon->urgent_id == 0)
    {
        icon->urgent_id =
            g_timeout_add (URGENT_TIMEOUT, (GSourceFunc)urgent_timeout, icon);
    }
}

static void
unqueue_urgent_timeout (Icon *icon)
{
    if (icon->urgent_id > 0)
    {
        g_source_remove (icon->urgent_id);
        icon->urgent_id = 0;
        if (icon->button)
        {
            update_blink (icon, FALSE);
            update_visibility (icon, NULL);
        }
    }
}

/* callbacks */
static gboolean
icon_button_pressed (GtkWidget *button, GdkEventButton *ev, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (ev->button == 1)
    {
        if (wnck_window_is_active (icon->window))
        {
            wnck_window_minimize (icon->window);
        }
        else
        {
            WnckScreen    *scr;
            WnckWorkspace *aws, *ws;

            scr = wnck_window_get_screen (icon->window);
            aws = wnck_screen_get_active_workspace (scr);
            ws  = wnck_window_get_workspace (icon->window);

            if (aws != ws)
            {
                wnck_workspace_activate (ws, ev->time);
            }
            wnck_window_activate (icon->window, ev->time);
        }

        return TRUE;
    }
    else if (ev->button == 3)
    {
        GtkWidget *action_menu;

        action_menu = wnck_create_window_action_menu(icon->window);

        g_signal_connect(G_OBJECT(action_menu), "selection-done",
                         G_CALLBACK(gtk_widget_destroy), NULL);

        gtk_menu_popup(GTK_MENU(action_menu), NULL, NULL, NULL, NULL,
                       ev->button, ev->time);

        return TRUE;
    }

    return FALSE;
}

static void
icon_name_changed (WnckWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    gtk_tooltips_set_tip (icon->ib->icon_tooltips, icon->button,
                          wnck_window_get_name (window), NULL);
}

static void
icon_state_changed (WnckWindow *window, WnckWindowState changed_mask,
                    WnckWindowState new_state, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (changed_mask & WNCK_WINDOW_STATE_DEMANDS_ATTENTION
        || changed_mask & WNCK_WINDOW_STATE_URGENT)
    {
        if (wnck_window_or_transient_needs_attention (window))
        {
            queue_urgent_timeout (icon);
        }
        else
        {
            unqueue_urgent_timeout (icon);
        }
    }

    if (changed_mask & WNCK_WINDOW_STATE_MINIMIZED
        || changed_mask & WNCK_WINDOW_STATE_SKIP_TASKLIST)
    {
        update_visibility (icon, NULL);

        icon_update_image (icon);
    }
}

static void
icon_workspace_changed (WnckWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    update_visibility (icon, NULL);
}

static void
icon_icon_changed (WnckWindow *window, gpointer data)
{
    Icon *icon = (Icon *)data;

    if (icon->pb)
        g_object_unref (G_OBJECT (icon->pb));

    icon->pb = wnck_window_get_icon (icon->window);

    if (icon->pb)
        g_object_ref (G_OBJECT (icon->pb));

    /* make sure the icon is actually updated */
    icon->was_minimized = !wnck_window_is_minimized (icon->window);
    icon_update_image (icon);
}

static void
icon_destroy (Icon *icon)
{
    int i;

    unqueue_urgent_timeout (icon);

    for (i = 0; i < N_ICON_CONNECTIONS; i++)
    {
        if (icon->connections[i])
            g_signal_handler_disconnect (icon->window, icon->connections[i]);

        icon->connections[i] = 0;
    }

    if (icon->pb)
        g_object_unref (G_OBJECT (icon->pb));

    panel_slice_free (Icon, icon);
}

static Icon *
icon_new (WnckWindow *window, Iconbox *ib)
{
    Icon *icon = panel_slice_new0 (Icon);
    int i = 0;

    icon->ib = ib;

    icon->window = window;

    icon->button = xfce_create_panel_toggle_button ();

    g_signal_connect (icon->button, "button-press-event",
                      G_CALLBACK (icon_button_pressed), icon);

    icon->image = xfce_scaled_image_new ();
    gtk_widget_show (icon->image);
    gtk_container_add (GTK_CONTAINER (icon->button), icon->image);

    icon->pb = wnck_window_get_icon (window);
    if (icon->pb)
    {
        xfce_scaled_image_set_from_pixbuf (XFCE_SCALED_IMAGE (icon->image),
                                           icon->pb);
        g_object_ref (G_OBJECT (icon->pb));
    }

    icon->connections[i++] =
        g_signal_connect (window, "name-changed",
                                 G_CALLBACK (icon_name_changed), icon);

    icon->connections[i++] =
        g_signal_connect (window, "state-changed",
                                 G_CALLBACK (icon_state_changed), icon);

    icon->connections[i++] =
        g_signal_connect (window, "workspace-changed",
                                 G_CALLBACK (icon_workspace_changed), icon);

    icon->connections[i++] =
        g_signal_connect (window, "icon-changed",
                                 G_CALLBACK (icon_icon_changed), icon);

    g_assert (i == N_ICON_CONNECTIONS);

    if (wnck_window_is_skip_tasklist (window))
    {
        return icon;
    }

    icon_update_image (icon);

    gtk_tooltips_set_tip (ib->icon_tooltips, icon->button,
                          wnck_window_get_name (window), NULL);

    update_visibility (icon, NULL);

    return icon;
}

/* iconlist */
static void
iconbox_active_window_changed (WnckScreen *screen,
#ifdef HAVE_WNCK_TWO_POINT_TWENTY
                               WnckScreen *previous,
#endif
                               gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;
    WnckWindow *window = wnck_screen_get_active_window (screen);

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (icon->button),
                                      (window == icon->window));
    }
}

static void
iconbox_active_workspace_changed (WnckScreen *screen,
#ifdef HAVE_WNCK_TWO_POINT_TWENTY
                                  WnckWorkspace *previous_workspace,
#endif
                                  gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;
    WnckWorkspace *ws = wnck_screen_get_active_workspace (screen);

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        update_visibility (icon, ws);
    }
}

static void
iconbox_window_opened (WnckScreen *screen, WnckWindow *window, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    Icon *icon;

    icon = icon_new (window, ib);

    ib->iconlist = g_slist_append (ib->iconlist, icon);

    gtk_box_pack_start (GTK_BOX (ib->iconbox), icon->button, FALSE, FALSE, 0);

    if (wnck_window_or_transient_needs_attention (window))
    {
        queue_urgent_timeout (icon);
    }
}

static void
iconbox_window_closed (WnckScreen *screen, WnckWindow *window, gpointer data)
{
    Iconbox *ib = (Iconbox *)data;
    GSList *l;

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        if (window == icon->window)
        {
            gtk_widget_destroy (icon->button);
            icon->button = NULL;
            icon_destroy (icon);

            ib->iconlist = g_slist_delete_link (ib->iconlist, l);

            break;
        }
    }
}

static void
iconbox_init_icons (Iconbox * ib)
{
    int i = 0;
    GList *windows, *l;

    wnck_screen_force_update (ib->wnck_screen);

    ib->connections[i++] =
        g_signal_connect (ib->wnck_screen, "active_window_changed",
                                 G_CALLBACK (iconbox_active_window_changed),
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->wnck_screen, "active_workspace_changed",
                                 G_CALLBACK (iconbox_active_workspace_changed),
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->wnck_screen, "window_opened",
                                 G_CALLBACK (iconbox_window_opened),
                                 ib);

    ib->connections[i++] =
        g_signal_connect (ib->wnck_screen, "window_closed",
                                 G_CALLBACK (iconbox_window_closed),
                                 ib);

    g_assert (i == N_ICONBOX_CONNECTIONS);

    windows = wnck_screen_get_windows (ib->wnck_screen);

    for (l = windows; l != NULL; l = l->next)
    {
        WnckWindow *w = l->data;

        iconbox_window_opened (ib->wnck_screen, w, ib);
    }

    iconbox_active_window_changed (ib->wnck_screen,
#ifdef HAVE_WNCK_TWO_POINT_TWENTY
                                   NULL,
#endif
                                   ib);
}

/* cleanup */
static void
cleanup_icons (Iconbox *ib)
{
    int i;
    GSList *l;

    for (i = 0; i < N_ICONBOX_CONNECTIONS; i++)
    {
        if (ib->connections[i])
            g_signal_handler_disconnect (ib->wnck_screen, ib->connections[i]);

        ib->connections[i] = 0;
    }

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        icon_destroy ((Icon *)l->data);
    }

    g_slist_free (ib->iconlist);
    ib->iconlist = NULL;
}

static void
cleanup_iconbox (Iconbox *ib)
{
    cleanup_icons (ib);

    g_object_unref (G_OBJECT (ib->icon_tooltips));

    panel_slice_free (Iconbox, ib);
}


/* -------------------------------------------------------------------- *
 *                     Panel Plugin Interface                           *
 * -------------------------------------------------------------------- */

/* Register with the panel */

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (iconbox_construct);


/* Interface Implementation */

static void
iconbox_orientation_changed (XfcePanelPlugin *plugin,
                             GtkOrientation orientation,
                             Iconbox *iconbox)
{
    xfce_hvbox_set_orientation (XFCE_HVBOX(iconbox->box), orientation);
    xfce_hvbox_set_orientation (XFCE_HVBOX(iconbox->iconbox), orientation);

    gtk_widget_queue_draw (iconbox->handle);
}

static gboolean
iconbox_set_size (XfcePanelPlugin *plugin, int size, Iconbox *ib)
{
    GSList *l;
    Icon *icon = ib->iconlist ? ib->iconlist->data : NULL;
    GtkWidget *widget = icon ? icon->button : ib->iconbox;

    if (xfce_panel_plugin_get_orientation (plugin) ==
            GTK_ORIENTATION_HORIZONTAL)
    {
        ib->icon_size = size - 2 * widget->style->xthickness - 2;
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     -1, size);
    }
    else
    {
        ib->icon_size = size - 2 * widget->style->ythickness - 2;
        gtk_widget_set_size_request (GTK_WIDGET (plugin),
                                     size, -1);
    }

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        icon_update_image (icon);
    }

    return TRUE;
}

static void
iconbox_free_data (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    GtkWidget *dlg = g_object_get_data (G_OBJECT (plugin), "dialog");

    if (dlg)
        gtk_widget_destroy (dlg);

    g_signal_handler_disconnect (plugin, iconbox->screen_changed_id);

    cleanup_iconbox (iconbox);
}

static void
iconbox_read_rc_file (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    char *file;
    XfceRc *rc;
    int only_hidden = 0;
    int all_workspaces = 0;
    int expand = 1;

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc != NULL)
        {
            only_hidden = xfce_rc_read_int_entry (rc, "only_hidden", 0);
            all_workspaces = xfce_rc_read_int_entry (rc, "all_workspaces", 0);
            expand = xfce_rc_read_int_entry (rc, "expand", 1);
            xfce_rc_close (rc);
        }
    }

    iconbox->only_hidden = (only_hidden == 1);
    iconbox->all_workspaces = (all_workspaces == 1);
    iconbox->expand = (expand != 0);
}

static void
iconbox_write_rc_file (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    char *file;
    XfceRc *rc;

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (!rc)
        return;

    xfce_rc_write_int_entry (rc, "only_hidden", iconbox->only_hidden);
    xfce_rc_write_int_entry (rc, "all_workspaces", iconbox->all_workspaces);
    xfce_rc_write_int_entry (rc, "expand", iconbox->expand);

    xfce_rc_close (rc);
}

/* create widgets and connect to signals */
static gboolean
handle_expose (GtkWidget *widget, GdkEventExpose *ev, Iconbox *iconbox)
{
    if (GTK_WIDGET_DRAWABLE (widget))
    {
        GtkAllocation *allocation = &(widget->allocation);
        int x, y, w, h;
        gboolean horizontal;

        horizontal = (xfce_panel_plugin_get_orientation (iconbox->plugin)
                      == GTK_ORIENTATION_HORIZONTAL);

        x = allocation->x;
        y = allocation->y;
        w = allocation->width;
        h = allocation->height;

        if (horizontal)
        {
            y += widget->style->ythickness;
            h -= 2 * widget->style->ythickness;
        }
        else
        {
            x += widget->style->xthickness;
            w -= 2 * widget->style->xthickness;
        }

        gtk_paint_handle (widget->style, widget->window,
                          GTK_WIDGET_STATE (widget), GTK_SHADOW_NONE,
                          &(ev->area), widget, "handlebox",
                          x, y, w, h,
                          horizontal ? GTK_ORIENTATION_VERTICAL :
                                       GTK_ORIENTATION_HORIZONTAL);

        return TRUE;
    }

    return FALSE;
}

static void
iconbox_screen_changed (GtkWidget *plugin, GdkScreen *screen, Iconbox *ib)
{
    if (!screen)
        return;

    gtk_container_foreach (GTK_CONTAINER (ib->iconbox),
                           (GtkCallback) gtk_widget_destroy, NULL);
    cleanup_icons (ib);

    ib->wnck_screen = wnck_screen_get (gdk_screen_get_number (screen));

    iconbox_init_icons (ib);
}

static void
iconbox_construct (XfcePanelPlugin *plugin)
{
    Iconbox *iconbox = panel_slice_new0 (Iconbox);

    iconbox->plugin = plugin;

    iconbox_read_rc_file (plugin, iconbox);

    xfce_panel_plugin_set_expand (plugin, iconbox->expand);

    iconbox->box = xfce_hvbox_new (xfce_panel_plugin_get_orientation (plugin),
                                   FALSE, 0);
    gtk_container_set_reallocate_redraws (GTK_CONTAINER (iconbox->box), TRUE);
    gtk_widget_show (iconbox->box);
    gtk_container_add (GTK_CONTAINER (plugin), iconbox->box);

    iconbox->handle = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_set_size_request (iconbox->handle, 8, 8);
    gtk_widget_show (iconbox->handle);
    gtk_box_pack_start (GTK_BOX (iconbox->box), iconbox->handle,
                        FALSE, FALSE, 0);

    xfce_panel_plugin_add_action_widget (plugin, iconbox->handle);

    g_signal_connect (iconbox->handle, "expose-event",
                      G_CALLBACK (handle_expose), iconbox);

    iconbox->iconbox =
        xfce_hvbox_new (xfce_panel_plugin_get_orientation (plugin), FALSE, 0);
    gtk_widget_show (iconbox->iconbox);
    gtk_box_pack_start (GTK_BOX (iconbox->box), iconbox->iconbox,
                        FALSE, FALSE, 0);

    iconbox->icon_tooltips = gtk_tooltips_new ();
    g_object_ref (G_OBJECT (iconbox->icon_tooltips));
    gtk_object_sink (GTK_OBJECT (iconbox->icon_tooltips));

    g_signal_connect (plugin, "orientation-changed",
                      G_CALLBACK (iconbox_orientation_changed), iconbox);

    g_signal_connect (plugin, "size-changed",
                      G_CALLBACK (iconbox_set_size), iconbox);

    g_signal_connect (plugin, "free-data",
                      G_CALLBACK (iconbox_free_data), iconbox);

    g_signal_connect (plugin, "save",
                      G_CALLBACK (iconbox_write_rc_file), iconbox);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin",
                      G_CALLBACK (iconbox_properties_dialog), iconbox);

    iconbox->screen_changed_id =
        g_signal_connect (plugin, "screen-changed",
                          G_CALLBACK (iconbox_screen_changed), iconbox);

    iconbox_screen_changed (GTK_WIDGET (plugin),
                            gtk_widget_get_screen (GTK_WIDGET (plugin)),
                            iconbox);
}

/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void
only_hidden_toggled (GtkToggleButton *tb, Iconbox *ib)
{
    GSList *l;

    ib->only_hidden = gtk_toggle_button_get_active (tb);

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        update_visibility (icon, NULL);
    }
}

static void
all_workspaces_toggled (GtkToggleButton *tb, Iconbox *ib)
{
    GSList *l;

    ib->all_workspaces = gtk_toggle_button_get_active (tb);

    for (l = ib->iconlist; l != NULL; l = l->next)
    {
        Icon *icon = l->data;

        update_visibility (icon, NULL);
    }
}

static void
expand_toggled (GtkToggleButton *tb, Iconbox *ib)
{
    ib->expand = gtk_toggle_button_get_active (tb);

    xfce_panel_plugin_set_expand (ib->plugin, ib->expand);
}

static void
iconbox_dialog_response (GtkWidget *dlg, int reponse,
                         Iconbox *ib)
{
    g_object_set_data (G_OBJECT (ib->plugin), "dialog", NULL);

    gtk_widget_destroy (dlg);
    xfce_panel_plugin_unblock_menu (ib->plugin);
    iconbox_write_rc_file (ib->plugin, ib);
}

static void
iconbox_properties_dialog (XfcePanelPlugin *plugin, Iconbox *iconbox)
{
    GtkWidget *dlg, *vbox, *cb;

    xfce_panel_plugin_block_menu (plugin);

    dlg = xfce_titled_dialog_new_with_buttons (_("Icon Box"), NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                NULL);

    gtk_window_set_screen (GTK_WINDOW (dlg),
                           gtk_widget_get_screen (GTK_WIDGET (plugin)));

    g_object_set_data (G_OBJECT (plugin), "dialog", dlg);

    gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name (GTK_WINDOW (dlg), "xfce4-settings");

    g_signal_connect (dlg, "response", G_CALLBACK (iconbox_dialog_response),
                      iconbox);

    gtk_container_set_border_width (GTK_CONTAINER (dlg), 2);

    vbox = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), vbox,
                        TRUE, TRUE, 0);

    cb = gtk_check_button_new_with_mnemonic (
                _("Only show minimized applications"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  iconbox->only_hidden);
    g_signal_connect (cb, "toggled", G_CALLBACK (only_hidden_toggled),
                      iconbox);

    cb = gtk_check_button_new_with_mnemonic (
                _("Show applications of all workspaces"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  iconbox->all_workspaces);
    g_signal_connect (cb, "toggled", G_CALLBACK (all_workspaces_toggled),
                      iconbox);

    cb = gtk_check_button_new_with_mnemonic (_("Use all available space"));
    gtk_widget_show (cb);
    gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
                                  iconbox->expand);
    g_signal_connect (cb, "toggled", G_CALLBACK (expand_toggled),
                      iconbox);

    gtk_widget_show (dlg);
}
